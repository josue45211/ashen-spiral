"""Run INSIDE Unreal Editor (commandlet, import only; no gameplay):
    UnrealEditor-Cmd.exe AshenSpiral.uproject /Engine/Maps/Entry -run=pythonscript -script=Tools/import_vault_assets.py -unattended -nosplash -NullRHI -NoSound -UTF8Output

Imports the assets of the 0.25 plan (Docs/0.25_ASSET_PLAN.md) from the Epic launcher vault. Nothing is copied into the repository: the source art is licence-gated (Gate 0) and large, so the
generated .uasset files stay local like the rest of Content/ (they are not tracked), and the game falls back to its built-in meshes when they are absent (ASAssets::HasMesh).

Phase A: the Molten Face Hammer for the elite hammer enemies. Its textures are embedded in the FBX, so they are extracted first; they are 2048 and are capped at 1024 in the editor.
Phase B: the arena concrete. Tools/make_concrete_tile.ps1 cuts a seamless 2048 tile out of the "Concrete column" pack first (Saved/VaultStage/T_ArenaConcrete.png); this script imports it and builds
M_ArenaConcrete, a world-aligned (triplanar) material, so the stretched, scaled cover blocks show the concrete at one scale on every face. No mesh changes: the blocks keep their own geometry and collision.
Set ASHEN_VAULT to override the vault folder. The report of what was imported, and every measured bound, is written to Saved/AshenVaultImport.json.
"""
from pathlib import Path
import json
import os
import struct
import traceback
import zlib

import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
VAULT = Path(os.environ.get("ASHEN_VAULT", r"C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\FabLibrary"))
STAGE = ROOT / "Saved" / "VaultStage"
REPORT = ROOT / "Saved" / "AshenVaultImport.json"
HAMMER_FBX = VAULT / "Molten_Face_Hammer_Weapon_FBX-b0be64dd" / "fbx" / "molten_face_hammer_fbx.fbx"
# The maul mesh of the game (SM_Weapon1) stands upright on +Z: pommel at -14, head top at 117.5, so a 131.5 cm weapon with the head wide on Y. The imported hammer is the same size and pose.
TARGET_LENGTH = 131.5
TARGET_MIN_Z = -14.0
TEXTURE_CAP = 1024
FOLDER = "/Game/Art/Imported/MoltenHammer"
MESH_FOLDER = "/Game/Art/Meshes"  # where ASAssets::Mesh looks
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"imported": [], "bounds": {}, "notes": []}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


# ---- A minimal binary FBX reader: enough to pull the embedded texture files and the raw vertex bounds ----
def read_nodes(b, pos, end, ver):
    nodes = []
    wide = ver >= 7500
    fmt, header = ("<QQQB", 25) if wide else ("<IIIB", 13)
    while pos < end:
        end_offset, prop_count, prop_len, name_len = struct.unpack_from(fmt, b, pos)
        if end_offset == 0:
            break
        p = pos + header
        name = b[p:p + name_len].decode("latin1")
        p += name_len
        props = []
        props_end = p + prop_len
        for _ in range(prop_count):
            t = chr(b[p])
            p += 1
            if t in "YCFDIL":
                size = {"Y": 2, "C": 1, "F": 4, "D": 8, "I": 4, "L": 8}[t]
                props.append((t, b[p:p + size]))
                p += size
            elif t in "fdlib":
                count, enc, clen = struct.unpack_from("<III", b, p)
                p += 12
                raw = b[p:p + clen]
                p += clen
                props.append((t, zlib.decompress(raw) if enc == 1 else raw, count))
            elif t in "SR":
                n = struct.unpack_from("<I", b, p)[0]
                p += 4
                props.append((t, b[p:p + n]))
                p += n
        p = props_end
        kids = read_nodes(b, p, end_offset, ver) if p < end_offset else []
        nodes.append((name, props, kids))
        pos = end_offset
    return nodes


def walk(nodes, name):
    for n in nodes:
        if n[0] == name:
            yield n
        yield from walk(n[2], name)


def extract_textures(fbx):
    """Writes every embedded PNG/JPEG of the FBX to STAGE and returns {kind: path}, kind being BaseColor, Emissive, Normal or Metallic (from the file name)."""
    b = fbx.read_bytes()
    ver = struct.unpack_from("<I", b, 23)[0]
    nodes = read_nodes(b, 27, len(b), ver)
    found = {}
    STAGE.mkdir(parents=True, exist_ok=True)
    for video in walk(nodes, "Video"):
        name = video[1][1][1].split(b"\x00")[0].decode("latin1") if len(video[1]) > 1 else ""
        for content in walk(video[2], "Content"):
            blob = content[1][0][1] if content[1] else b""
            if blob[:4] != b"\x89PNG" and blob[:2] != b"\xff\xd8":
                continue
            kind = next((k for k in ("BaseColor", "Emissive", "Normal", "Metallic") if k.lower() in name.lower()), None)
            if kind is None:
                continue
            target = STAGE / ("T_MoltenHammer_" + kind + (".png" if blob[:4] == b"\x89PNG" else ".jpg"))
            target.write_bytes(blob)
            found[kind] = target
    return found


def array_of(nodes, name, fmt):
    """The first array property of the first node called name (searched below nodes), unpacked as doubles/ints, or None."""
    for n in walk(nodes, name):
        if n[1] and len(n[1][0]) == 3:
            return struct.unpack("<%d%s" % (n[1][0][2], fmt), n[1][0][1])
    return None


def build_obj(fbx, target, target_length, target_min_z):
    """Writes the FBX's geometry as an OBJ (the format the project's own meshes use), scaled to target_length on Z and moved so its lowest point is target_min_z.
    The FBX node carries a x1000 scale, a tilt and a huge offset that the editor's importer would bake in or ignore, so the raw, upright vertices are used and the scale and shift are applied here.
    Returns the measured (min, max) of the written vertices."""
    b = fbx.read_bytes()
    ver = struct.unpack_from("<I", b, 23)[0]
    nodes = read_nodes(b, 27, len(b), ver)
    geometry = next(walk(nodes, "Geometry"))[2]
    flat = array_of(geometry, "Vertices", "d")
    index = array_of(geometry, "PolygonVertexIndex", "i")
    require(flat is not None and index is not None, "The FBX has no vertices or polygons")
    uvs = array_of(geometry, "UV", "d")
    uv_index = array_of(geometry, "UVIndex", "i")
    normals = array_of(geometry, "Normals", "d")
    low, high = min(flat[2::3]), max(flat[2::3])
    scale = target_length / (high - low)
    shift = target_min_z - low * scale
    # The game's weapons are held at the origin with the haft on the Z axis; centre the lower third (the haft) on X and Y so the hand grips the shaft, not the air beside it.
    haft = [i for i in range(0, len(flat), 3) if flat[i + 2] < low + (high - low) * 0.33]
    cx = sum(flat[i] for i in haft) / max(1, len(haft))
    cy = sum(flat[i + 1] for i in haft) / max(1, len(haft))
    lines = ["# Ashen Spiral import of a vault asset; local +Z up, scaled %.4f" % scale, "g maul"]
    for i in range(0, len(flat), 3):
        lines.append("v %.5f %.5f %.5f" % ((flat[i] - cx) * scale, (flat[i + 1] - cy) * scale, flat[i + 2] * scale + shift))
    corner = 0
    vt_count = vn_count = 0
    polygon = []
    use_uv = uvs is not None
    use_normal = normals is not None and len(normals) == 3 * len(index)
    for raw in index:
        last = raw < 0
        vertex = (~raw if last else raw)
        polygon.append((vertex, corner))
        corner += 1
        if last:
            parts = []
            for vertex, c in polygon:
                token = str(vertex + 1)
                if use_uv:
                    uv = uv_index[c] if uv_index is not None else c
                    lines.append("vt %.6f %.6f" % (uvs[2 * uv], uvs[2 * uv + 1]))
                    vt_count += 1
                    token += "/" + str(vt_count)
                if use_normal:
                    lines.append("vn %.6f %.6f %.6f" % (normals[3 * c], normals[3 * c + 1], normals[3 * c + 2]))
                    vn_count += 1
                    token += ("/" if use_uv else "//") + str(vn_count)
                parts.append(token)
            lines.append("f " + " ".join(parts))
            polygon = []
    target.write_text("\n".join(lines) + "\n")
    report["notes"].append("obj: %d vertices, %d polygon corners, uvs=%s normals=%s, scale %.4f shift %.3f" % (len(flat) // 3, len(index), use_uv, use_normal, scale, shift))
    return low * scale + shift, high * scale + shift


def import_texture(path, kind):
    destination = FOLDER
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", path.stem)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(destination + "/" + path.stem)
    require(texture is not None, "Texture import failed: " + str(path))
    texture.set_editor_property("max_texture_size", TEXTURE_CAP)
    if kind == "Normal":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif kind == "Metallic":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    assets.save_loaded_asset(texture, False)
    report["imported"].append(texture.get_path_name())
    return texture


def import_mesh(obj):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(obj))
    task.set_editor_property("destination_path", MESH_FOLDER)
    task.set_editor_property("destination_name", "SM_MoltenHammer")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", False)  # the game's own material is assigned in code
    options.set_editor_property("import_textures", False)   # extracted and imported separately (above)
    options.set_editor_property("import_animations", False)
    data = options.get_editor_property("static_mesh_import_data")
    for name, value in {
        "combine_meshes": True,
        "auto_generate_collision": False,  # an enemy's weapon never collides (ASAssets::Style turns collision off)
        "generate_lightmap_u_vs": False,
        "convert_scene": False,
        "convert_scene_unit": False,
        "import_uniform_scale": 1.0,  # the OBJ written by build_obj is already the right size and height
        "build_nanite": False,
    }.items():
        data.set_editor_property(name, value)
    data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.set_editor_property("options", options)
    task.set_editor_property("factory", unreal.FbxFactory())
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(MESH_FOLDER + "/SM_MoltenHammer")
    require(isinstance(mesh, unreal.StaticMesh), "Mesh import did not create SM_MoltenHammer")
    report["imported"].append(mesh.get_path_name())
    return mesh


def mesh_bounds(mesh):
    box = mesh.get_bounding_box()
    return {"min": [box.min.x, box.min.y, box.min.z], "max": [box.max.x, box.max.y, box.max.z]}


def create_material(textures):
    package = FOLDER + "/M_MoltenHammer"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_MoltenHammer", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_MoltenHammer")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", True)  # the importer's winding for this mesh is unconfirmed without a render, so never let a backface hide the weapon

    def node(cls, x, y, **properties):
        obj = edit.create_material_expression(material, cls, x, y)
        require(obj is not None, "Could not create material expression " + cls.__name__)
        for key, value in properties.items():
            obj.set_editor_property(key, value)
        return obj

    base = node(unreal.MaterialExpressionTextureSample, -700, -300, texture=textures["BaseColor"])
    glow = node(unreal.MaterialExpressionTextureSample, -700, 0, texture=textures["Emissive"])
    normal = node(unreal.MaterialExpressionTextureSample, -700, 300, texture=textures["Normal"], sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    metal = node(unreal.MaterialExpressionTextureSample, -700, 600, texture=textures["Metallic"], sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    boost = node(unreal.MaterialExpressionScalarParameter, -700, 850, parameter_name="Emission", default_value=5.0)
    rough = node(unreal.MaterialExpressionScalarParameter, -700, 950, parameter_name="Roughness", default_value=0.55)
    scaled = node(unreal.MaterialExpressionMultiply, -350, 0)
    require(edit.connect_material_expressions(glow, "RGB", scaled, "A"), "emissive A")
    require(edit.connect_material_expressions(boost, "", scaled, "B"), "emissive B")
    require(edit.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR), "base colour")
    require(edit.connect_material_property(scaled, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "emissive")
    require(edit.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL), "normal")
    require(edit.connect_material_property(metal, "R", unreal.MaterialProperty.MP_METALLIC), "metallic")
    require(edit.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS), "roughness")
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_MoltenHammer")
    report["imported"].append(material.get_path_name())
    return material


CONCRETE_FOLDER = "/Game/Art/Imported/ArenaConcrete"
CONCRETE_TILE = STAGE / "T_ArenaConcrete.png"


def create_concrete_material(tile):
    """A triplanar concrete: three planar projections of the tile (XY for floors and tops, XZ and YZ for walls) blended by the surface normal, tinted by the game per block."""
    package = CONCRETE_FOLDER + "/M_ArenaConcrete"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_ArenaConcrete", CONCRETE_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_ArenaConcrete")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)

    def node(cls, x, y, **properties):
        obj = edit.create_material_expression(material, cls, x, y)
        require(obj is not None, "Could not create material expression " + cls.__name__)
        for key, value in properties.items():
            obj.set_editor_property(key, value)
        return obj

    def wire(a, output, b, input_name):
        require(edit.connect_material_expressions(a, output, b, input_name), "Material connection failed: " + input_name)

    tint = node(unreal.MaterialExpressionVectorParameter, -1400, -300, parameter_name="Tint", default_value=unreal.LinearColor(0.6, 0.58, 0.6, 1))
    rough = node(unreal.MaterialExpressionScalarParameter, -1400, 1000, parameter_name="Roughness", default_value=0.88)
    metal = node(unreal.MaterialExpressionScalarParameter, -1400, 1100, parameter_name="Metallic", default_value=0.0)
    tiling = node(unreal.MaterialExpressionScalarParameter, -1400, 0, parameter_name="TileScale", default_value=0.0025)  # one tile per 4 m
    position = node(unreal.MaterialExpressionWorldPosition, -1400, 150)
    scaled = node(unreal.MaterialExpressionMultiply, -1150, 100)
    wire(position, "", scaled, "A")
    wire(tiling, "", scaled, "B")

    def projection(name, x, y, channels):
        mask = node(unreal.MaterialExpressionComponentMask, x, y, r=channels[0], g=channels[1], b=channels[2], a=False)
        wire(scaled, "", mask, "")
        sample = node(unreal.MaterialExpressionTextureSample, x + 250, y, texture=tile)
        wire(mask, "", sample, "UVs")
        return sample

    top = projection("XY", -900, -100, (True, True, False))     # faces whose normal points along Z
    front = projection("XZ", -900, 250, (True, False, True))    # normal along Y
    side = projection("YZ", -900, 600, (False, True, True))     # normal along X
    normal = node(unreal.MaterialExpressionVertexNormalWS, -1400, 450)
    magnitude = node(unreal.MaterialExpressionAbs, -1150, 450)
    sharp = node(unreal.MaterialExpressionPower, -950, 900)
    exponent = node(unreal.MaterialExpressionConstant, -1150, 950, r=4.0)
    ones = node(unreal.MaterialExpressionConstant3Vector, -1150, 1150, constant=unreal.LinearColor(1, 1, 1, 1))
    total = node(unreal.MaterialExpressionDotProduct, -700, 950)
    weights = node(unreal.MaterialExpressionDivide, -500, 900)
    wire(normal, "", magnitude, "")
    wire(magnitude, "", sharp, "Base")
    wire(exponent, "", sharp, "Exp")
    wire(sharp, "", total, "A")
    wire(ones, "", total, "B")
    wire(sharp, "", weights, "A")
    wire(total, "", weights, "B")
    blend = None
    for sample, channel in ((side, (True, False, False)), (front, (False, True, False)), (top, (False, False, True))):
        weight = node(unreal.MaterialExpressionComponentMask, -300, 900, r=channel[0], g=channel[1], b=channel[2], a=False)
        wire(weights, "", weight, "")
        term = node(unreal.MaterialExpressionMultiply, -100, 300)
        wire(sample, "RGB", term, "A")
        wire(weight, "", term, "B")
        if blend is None:
            blend = term
        else:
            added = node(unreal.MaterialExpressionAdd, 100, 300)
            wire(blend, "", added, "A")
            wire(term, "", added, "B")
            blend = added
    colour = node(unreal.MaterialExpressionMultiply, 300, 100)
    wire(blend, "", colour, "A")
    wire(tint, "", colour, "B")
    require(edit.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR), "base colour")
    require(edit.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS), "roughness")
    require(edit.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC), "metallic")
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)  # the cover blocks are instanced
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_ArenaConcrete")
    report["imported"].append(material.get_path_name())
    return material


def import_concrete():
    if not CONCRETE_TILE.is_file():
        report["notes"].append("phase B skipped: run Tools/make_concrete_tile.ps1 first (no " + str(CONCRETE_TILE) + ")")
        return
    assets.make_directory(CONCRETE_FOLDER)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(CONCRETE_TILE))
    task.set_editor_property("destination_path", CONCRETE_FOLDER)
    task.set_editor_property("destination_name", "T_ArenaConcrete")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    tile = unreal.load_asset(CONCRETE_FOLDER + "/T_ArenaConcrete")
    require(tile is not None, "Concrete tile import failed")
    tile.set_editor_property("max_texture_size", 2048)
    assets.save_loaded_asset(tile, False)
    report["imported"].append(tile.get_path_name())
    create_concrete_material(tile)


def main():
    require(HAMMER_FBX.is_file(), "The Molten Face Hammer FBX is not in the vault: " + str(HAMMER_FBX))
    for folder in (FOLDER, MESH_FOLDER):
        assets.make_directory(folder)
    STAGE.mkdir(parents=True, exist_ok=True)
    obj = STAGE / "SM_MoltenHammer.obj"
    build_obj(HAMMER_FBX, obj, TARGET_LENGTH, TARGET_MIN_Z)
    found = extract_textures(HAMMER_FBX)
    require(len(found) == 4, "Expected four embedded textures, found " + ", ".join(sorted(found)))
    textures = {kind: import_texture(path, kind) for kind, path in found.items()}
    mesh = import_mesh(obj)
    bounds = mesh_bounds(mesh)
    report["bounds"]["SM_MoltenHammer"] = bounds
    create_material(textures)
    # The weapon must stand like SM_Weapon1: a 131.5 cm weapon from about -14 to 117.5 on Z. Report (and fail) if the import did not come out that way.
    length = bounds["max"][2] - bounds["min"][2]
    report["notes"].append("imported z %.2f..%.2f (length %.2f)" % (bounds["min"][2], bounds["max"][2], length))
    require(abs(length - TARGET_LENGTH) < 8.0, "SM_MoltenHammer came out %.1f cm long, expected about %.1f; adjust the import scale" % (length, TARGET_LENGTH))
    require(abs(bounds["min"][2] - TARGET_MIN_Z) < 8.0, "SM_MoltenHammer starts at z %.1f, expected about %.1f; adjust the import translation" % (bounds["min"][2], TARGET_MIN_Z))


try:
    main()
    import_concrete()
    report["ok"] = True
except Exception:
    report["ok"] = False
    report["error"] = traceback.format_exc()
    unreal.log_error(report["error"])
finally:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2))
    unreal.log("AshenVaultImport: " + ("OK" if report.get("ok") else "FAILED") + " - see " + str(REPORT))
