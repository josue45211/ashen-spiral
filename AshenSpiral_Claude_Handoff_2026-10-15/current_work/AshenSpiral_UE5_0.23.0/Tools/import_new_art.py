"""Run INSIDE Unreal Editor (Tools/ImportNewArt.ps1 does that): imports the named files from SourceArt into the project without redoing the whole kit
(bootstrap_unreal.py re-imports every mesh, texture and sound, about ninety assets).
ASHEN_IMPORT is a ';'-separated list of paths relative to SourceArt, e.g.  Meshes/SM_Weapon9.obj;Audio/S_ClawSwing.wav
Meshes get the master material (M_Surface) on every slot and are checked against mesh_bounds.json like the bootstrap does; sounds are imported as they are.
"""
from pathlib import Path
import json
import os
import traceback
import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ART = ROOT / "SourceArt"
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def import_one(path, destination, kind):
    package = destination + "/" + path.stem
    if assets.does_asset_exist(package):
        assets.delete_asset(package)  # a fresh import, as bootstrap_unreal.py does for meshes
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", path.stem)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    if kind == "mesh":
        options = unreal.FbxImportUI()
        options.set_editor_property("automated_import_should_detect_type", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("import_animations", False)
        data = options.get_editor_property("static_mesh_import_data")
        for name, value in {"combine_meshes": True, "auto_generate_collision": True, "generate_lightmap_u_vs": False, "convert_scene": False,
                            "convert_scene_unit": False, "import_uniform_scale": 1.0, "build_nanite": False}.items():
            data.set_editor_property(name, value)
        data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task.set_editor_property("options", options)
        task.set_editor_property("factory", unreal.FbxFactory())
    tools.import_asset_tasks([task])
    asset = unreal.load_asset(package)
    require(asset is not None, "Import did not create {}. Source: {}".format(package, path))
    return asset


def main():
    wanted = [item.strip() for item in os.environ.get("ASHEN_IMPORT", "").split(";") if item.strip()]
    require(wanted, "ASHEN_IMPORT is empty: name the files to import, relative to SourceArt")
    material = unreal.load_asset("/Game/Art/Materials/M_Surface")
    expected = json.loads((ART / "mesh_bounds.json").read_text()) if (ART / "mesh_bounds.json").exists() else {}
    done = []
    for rel in wanted:
        path = ART / rel
        require(path.is_file(), "No such source file: " + str(path))
        suffix = path.suffix.lower()
        if suffix == ".obj":
            mesh = import_one(path, "/Game/Art/Meshes", "mesh")
            require(isinstance(mesh, unreal.StaticMesh), path.stem + " is not a static mesh")
            for slot in range(max(1, len(mesh.get_editor_property("static_materials")))):
                mesh.set_material(slot, material)
            if path.stem in expected:
                bounds = mesh.get_bounding_box()
                actual = [bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, bounds.max.z - bounds.min.z]
                target = expected[path.stem]["extent"]
                require(all(abs(a - b) <= max(2.0, abs(b) * 0.03) for a, b in zip(actual, target)),
                        "Mesh scale/axis mismatch for {}: imported {}, expected {} cm".format(path.stem, actual, target))
            require(assets.save_loaded_asset(mesh, False), "Could not save " + path.stem)
            done.append(mesh.get_path_name())
        elif suffix == ".wav":
            sound = import_one(path, "/Game/Audio", "audio")
            done.append(sound.get_path_name())
        else:
            raise RuntimeError("Unsupported file type: " + rel)
    unreal.log("ASHEN IMPORT COMPLETE: {}".format(", ".join(done)))


try:
    main()
except Exception:
    unreal.log_error("ASHEN IMPORT FAILED\n" + traceback.format_exc())
    raise
