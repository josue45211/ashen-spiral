"""Run INSIDE Unreal Editor (commandlet, import only; no gameplay):
    UnrealEditor-Cmd.exe AshenSpiral.uproject /Engine/Maps/Entry -run=pythonscript -script=Tools/import_vault_assets.py -unattended -nosplash -NullRHI -NoSound -UTF8Output

Imports the assets of the 0.25 plan (Docs/0.25_ASSET_PLAN.md) from the Epic launcher vault. Nothing is copied into the repository: the source art is licence-gated (Gate 0) and large, so the
generated .uasset files stay local like the rest of Content/ (they are not tracked), and the game falls back to its built-in meshes when they are absent (ASAssets::HasMesh).

Phase A: the Molten Face Hammer for the elite hammer enemies. Its textures are embedded in the FBX, so they are extracted first; they are 2048 and are capped at 1024 in the editor.
Phase B: the arena concrete. Tools/make_concrete_tile.ps1 cuts a seamless 2048 tile out of the "Concrete column" pack first (Saved/VaultStage/T_ArenaConcrete.png); this script imports it and builds
M_ArenaConcrete, a world-aligned (triplanar) material, so the stretched, scaled cover blocks show the concrete at one scale on every face. No mesh changes: the blocks keep their own geometry and collision.
Phase C: the Dragon Cave stone kit for the Endless arena skin. The pack is an Unreal project, so its static meshes (not its 5 GB of textures or its materials) are copied into the project, duplicated as
SM_DC_<name> under /Game/Art/Meshes with every material slot set to the arena concrete, and the copies are deleted again. The decor uses the meshes' own measured bounds (written to the report).
Phase E: audio. Concrete foley from the Game Animation Sample becomes /Game/Audio/Imported/Foley (the player's footsteps, ASFoleyRules.h): running steps S_Step01..12, and since 0.24.2 (Josue: "make
the footsteps more natural, like the default Unreal Engine character") walking steps S_StepWalk01..12, strafing S_StepStrafe01..08, backpedalling S_StepBack01..08, landings S_Land01..08 and jumps
S_Jump01..06. The Horror Template's monster
sound can become /Game/Audio/E_BossRoar, the file the boss-roar cue looks for first (Docs/AUDIO_MANIFEST_0.24.md), but only with ASHEN_IMPORT_MONSTER=1 (it is 6.4 s and unheard, so it is off); sounds are copied as sound waves, duplicated under their new names, and the staging copies deleted.
Phase G (0.24.2 aesthetics): exports the Dragon Cave ground textures (stone tiles, paving, the dirt mask) from the pack to PNG in Saved/VaultStage/DC; Tools/shrink_vault_textures.ps1 then
brings them to 2048 (the pack's sources are 4096-8192 and 5 GB in all).
Phase H (0.24.2 aesthetics): imports the shrunk textures and builds M_ArenaGround, a world-aligned ground of worn stone tiles and paving that blend through large drifts of dirt, so the Endless floor
shows no repeat; the game puts it on the Endless floor and the plain beyond the ruins (ASWorld::Add) when it exists here.
Phase I (0.24.2 aesthetics): the scenery's own textures (phase G exports them with the ground's; the shrink brings them to 2048), M_DecorPBR (the pack's PBR set-up rebuilt with a Tint the
game drives per sky stage), one MI_DC_<set> per texture set, and every SM_DC mesh's slots set to its own set (DECOR_SET_OF, read from the pack's meshes) instead of the concrete.
Phase J (0.24.2 aesthetics): the UDIM sets those pieces had in the pack (several tiles each), stitched into one atlas per set by Tools/stitch_udim_atlases.ps1, and M_DecorAtlas, which
finds each UDIM tile's cell per pixel (taking the mip from the original UV, so no seams): the pillars, walls, bridge pieces and inferno rocks get their own textures without virtual texturing.
Phase K (0.24.2 aesthetics): M_ArenaBlock, the pack's weathered concrete (colour, normal, ORM) projected from the three world axes, for the Endless cover blocks and walls.
Set ASHEN_VAULT to override the vault folder; set ASHEN_PHASES to a subset of the letters ABCEGHIJK to run only those phases (ABCE by default; G, then the shrink and the stitch, then H,
I, J and K on request). The report of what was imported, and every measured bound, is written to Saved/AshenVaultImport.json.
"""
from pathlib import Path
import json
import os
import shutil
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


# The meshes of the 0.25 plan, phase C (ASDecorRules.h names the same ones; Tests/run_tests.py checks that every mesh it names is in this list).
DC_MESHES = [
    "LargeRock01", "LargeRock02", "LargeRock03", "LargeRock04", "LargeRock05", "LargeRock06", "LargeRock07",
    "CaveRock01", "CaveRock02", "CaveRock03", "CaveRock04",
    "InfernoRock01", "InfernoRock02", "InfernoRock03", "InfernoRock04", "InfernoRock05", "InfernoRock06", "InfernoRock07",
    "StoneworksPillar01", "StoneworksPillar02", "StoneworksPillar03", "StoneworksPillar04", "LargePillar01", "LargePillar02", "Column01", "Column02",
    "StoneworksStone01", "StoneworksStone02", "StoneworksStone03", "StoneWall01", "StoneWall02", "StoneWall03", "StoneWall04",
    "BridgeArc01", "BridgeArc02", "BridgeArc03", "BridgePillar01", "BridgePillar02",
    "DeadTree01", "DeadTree02", "DeadTree03", "DeadTree04", "DeadTree05", "StoneDebris01_",
    "Skull01", "Skull02", "Skeleton01", "Skeleton02", "Skeleton03", "Skeleton04", "Skeleton05", "Skeleton06", "CoffinStone01",
    "TreasurePile01", "TreasurePile02", "TreasurePile03", "TreasurePile04", "GoldenCoin01", "GoldenCoin02", "GoldenCup01", "Jewel01", "Jewel02", "Vase01",
]
DC_SOURCE = VAULT.parent / "DragonCaf79c64d07c70V1" / "data" / "Content" / "DragonCave" / "Meshes"
CONCRETE_FOLDER = "/Game/Art/Imported/ArenaConcrete"
DC_TEXTURE_SOURCE = VAULT.parent / "DragonCaf79c64d07c70V1" / "data" / "Content" / "DragonCave" / "Textures"
DC_STAGE = STAGE / "DC"                 # full-size exports (phase G)
DC_SMALL = DC_STAGE / "small"           # after Tools/shrink_vault_textures.ps1 (phase H reads these)
GROUND_FOLDER = "/Game/Art/Imported/Ground"
BLOCK_FOLDER = "/Game/Art/Imported/Block"
# (pack texture, kind): colour is sRGB; normal and masks are linear. ORM = ambient occlusion, roughness, metallic.
GROUND_TEXTURES = (("T_StoneDebris01_BC", "colour"), ("T_StoneDebris01_N", "normal"), ("T_StoneDebris01_ORM", "masks"),
                   ("T_StoneFloor01_BC", "colour"), ("T_StoneFloor01_N", "normal"), ("T_StoneFloor01_ORM", "masks"), ("T_StoneFloor01_MSK", "masks"))
# Phase I (0.24.2 aesthetics): the scenery wears its own textures again. Which texture set each SM_DC mesh had in the pack (read from the pack's meshes and material instances:
# every one of them is the pack's PBR master with three textures, colour + normal + ORM, on UV 0, nothing else set), by mesh-name prefix; the longest matching prefix wins.
DECOR_FOLDER = "/Game/Art/Imported/Decor"
# The pillars, walls, bridge pieces and inferno rocks are NOT here: their sets (T_Stoneworks, T_LargePillars, T_StoneWall, T_BridgeSet, T_InfernoRocks01) are UDIM virtual textures
# (several tiles, the meshes' UVs spanning them); virtual texturing would need Config/DefaultEngine.ini (never committed), so phase J draws them through stitched atlases instead.
DECOR_SET_OF = (("LargeRock01", "LargeRock01"), ("LargeRock02", "LargeRock02"), ("LargeRock03", "LargeRock02"), ("LargeRock04", "LargeRock02"),
                ("LargeRock05", "LargeRock03"), ("LargeRock06", "LargeRock03"), ("LargeRock07", "LargeRock03"),
                ("CaveRock01", "CaveRocks01"), ("CaveRock02", "CaveRocks01"), ("CaveRock03", "CaveRocks02"), ("CaveRock04", "CaveRocks03"),
                ("Column", "Concrete02"), ("DeadTree", "DeadTree"), ("CoffinStone", "CoffinStone01"),
                ("Skull", "Treasures01"), ("Skeleton", "Treasures01"), ("TreasurePile", "Treasures01"), ("GoldenCoin", "Treasures01"), ("GoldenCup", "Treasures01"),
                ("Vase", "Treasures01"))
# a set's textures are T_<set>_BC / _N / _ORM, except where the pack named them differently
DECOR_TEXTURE_STEM = {"LargeRock01": "T_LargeRock01", "LargeRock02": "T_LargeRock02", "LargeRock03": "T_LargeRock03", "CaveRocks01": "T_CaveRocks01",
                      "CaveRocks02": "T_CaveRocks02", "CaveRocks03": "T_CaveRocks03", "Concrete02": "T_Concrete02", "DeadTree": "T_DeadTree",
                      "CoffinStone01": "T_CoffinStone01", "Treasures01": "T_Treasures01"}
DECOR_TEXTURES = tuple((DECOR_TEXTURE_STEM[s] + suffix, kind) for s in sorted(set(DECOR_TEXTURE_STEM)) for suffix, kind in (("_BC", "colour"), ("_N", "normal"), ("_ORM", "masks")))
# Phase J (0.24.2 aesthetics): the UDIM sets, stitched into one atlas each by Tools/stitch_udim_atlases.ps1 (grid columns, rows; keep in step with that script), and the meshes that wear them.
DECOR_ATLASES = {"BridgeSet": ("T_BridgeSet", 4, 2), "InfernoRocks01": ("T_InfernoRocks01", 2, 2), "LargePillars": ("T_LargePillars", 2, 1),
                 "StoneWall": ("T_StoneWall", 2, 2), "Stoneworks": ("T_Stoneworks", 4, 2)}
DECOR_ATLAS_OF = (("InfernoRock", "InfernoRocks01"), ("Stoneworks", "Stoneworks"), ("LargePillar", "LargePillars"), ("BridgePillar", "LargePillars"),
                  ("StoneWall", "StoneWall"), ("BridgeArc", "BridgeSet"))
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
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE, True)  # 0.24.2: the Dragon Cave meshes are Nanite; without this flag they drew the engine's grey grid
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


def import_dragon_cave():
    if not DC_SOURCE.is_dir():
        report["notes"].append("phase C skipped: no Dragon Cave pack at " + str(DC_SOURCE))
        return
    concrete = unreal.load_asset(CONCRETE_FOLDER + "/M_ArenaConcrete")
    require(concrete is not None, "Phase C needs M_ArenaConcrete (phase B) first")
    if all(assets.does_asset_exist("/Game/Art/Meshes/SM_DC_" + name) for name in DC_MESHES) and os.environ.get("ASHEN_FORCE_IMPORT") != "1":
        report["notes"].append("phase C: all %d SM_DC meshes already imported (set ASHEN_FORCE_IMPORT=1 to redo)" % len(DC_MESHES))
        return
    staging = ROOT / "Content" / "DragonCave" / "Meshes"
    staging.mkdir(parents=True, exist_ok=True)
    missing = []
    for name in DC_MESHES:
        source = DC_SOURCE / ("SM_" + name + ".uasset")
        if source.is_file():
            shutil.copy2(source, staging / source.name)
        else:
            missing.append(name)
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/DragonCave"], True)
    done, failed = [], []
    bounds = report.setdefault("dragon_cave_bounds", {})
    for name in DC_MESHES:
        original = unreal.load_asset("/Game/DragonCave/Meshes/SM_" + name)
        if not isinstance(original, unreal.StaticMesh):
            failed.append(name)
            continue
        copy = tools.duplicate_asset("SM_DC_" + name, "/Game/Art/Meshes", original)
        if copy is None:
            failed.append(name)
            continue
        for slot in range(len(copy.get_editor_property("static_materials"))):
            copy.set_material(slot, concrete)  # the pack's own materials and textures stay behind
        assets.save_loaded_asset(copy, False)
        box = copy.get_bounding_box()
        bounds[name] = {"min": [box.min.x, box.min.y, box.min.z], "max": [box.max.x, box.max.y, box.max.z], "tris": copy.get_num_triangles(0) if hasattr(copy, "get_num_triangles") else -1}
        done.append(name)
        report["imported"].append(copy.get_path_name())
    assets.delete_directory("/Game/DragonCave")
    shutil.rmtree(ROOT / "Content" / "DragonCave", ignore_errors=True)
    report["notes"].append("phase C: %d meshes imported, %d failed to load %s, %d missing from the pack %s" % (len(done), len(failed), failed, len(missing), missing))
    require(not failed and not missing, "Some Dragon Cave meshes did not import: failed %s, missing %s" % (failed, missing))


def export_dc_textures():
    """Phase G: the pack's textures are Unreal assets, so they are copied in, written out as PNG at full size, and the copies deleted (nothing of the pack stays in Content)."""
    if not DC_TEXTURE_SOURCE.is_dir():
        report["notes"].append("phase G skipped: no Dragon Cave textures at " + str(DC_TEXTURE_SOURCE))
        return
    staging = ROOT / "Content" / "DragonCave" / "Textures"
    staging.mkdir(parents=True, exist_ok=True)
    DC_STAGE.mkdir(parents=True, exist_ok=True)
    # ASHEN_DC_EXPORT=T_A,T_B exports other textures of the pack instead (to look at candidates); by default the ground set and the scenery's sets (phase I)
    names = [n.strip() for n in os.environ.get("ASHEN_DC_EXPORT", "").split(",") if n.strip()] or [name for name, _ in GROUND_TEXTURES + DECOR_TEXTURES + BLOCK_TEXTURES]
    names = [n for n in names if not (DC_STAGE / (n + ".png")).is_file() or os.environ.get("ASHEN_FORCE_IMPORT") == "1"]  # already exported: kept
    for name in names:
        shutil.copy2(DC_TEXTURE_SOURCE / (name + ".uasset"), staging / (name + ".uasset"))
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/DragonCave"], True)
    written = []
    for name in names:
        texture = unreal.load_asset("/Game/DragonCave/Textures/" + name)
        require(isinstance(texture, unreal.Texture2D), "Not a texture: " + name)
        task = unreal.AssetExportTask()
        task.set_editor_property("object", texture)
        task.set_editor_property("filename", str(DC_STAGE / (name + ".png")))
        task.set_editor_property("automated", True)
        task.set_editor_property("prompt", False)
        task.set_editor_property("replace_identical", True)
        task.set_editor_property("exporter", unreal.TextureExporterPNG())
        require(unreal.Exporter.run_asset_export_task(task), "PNG export failed: " + name)
        written.append("%s %dx%d" % (name, texture.blueprint_get_size_x(), texture.blueprint_get_size_y()))
    assets.delete_directory("/Game/DragonCave")
    shutil.rmtree(ROOT / "Content" / "DragonCave", ignore_errors=True)
    report["notes"].append("phase G: exported " + ", ".join(written) + " to " + str(DC_STAGE) + "; now run Tools/shrink_vault_textures.ps1, then phase H")


def import_ground_texture(name, kind, folder=GROUND_FOLDER):
    source = DC_SMALL / (name + ".png")
    require(source.is_file(), "Missing %s: run phase G and Tools/shrink_vault_textures.ps1 first" % source)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", folder)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(folder + "/" + name)
    require(texture is not None, "Texture import failed: " + name)
    if kind == "normal":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif kind == "masks":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    assets.save_loaded_asset(texture, False)
    report["imported"].append(texture.get_path_name())
    return texture


def create_ground_material(t):
    """M_ArenaGround: a ruined courtyard. Rubble-strewn dirt everywhere, and the old stone slabs (the paving's own mask cuts them out) lying on it wherever a slow, large-scale noise
    says the paving survived; both are projected from above in world space (the floor's own UVs do not matter) at unrelated periods, so nothing shows a repeat. Tint (from the game)
    colours it per arena."""
    package = GROUND_FOLDER + "/M_ArenaGround"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_ArenaGround", GROUND_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_ArenaGround")
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

    position = node(unreal.MaterialExpressionWorldPosition, -2000, 0)
    flat = node(unreal.MaterialExpressionComponentMask, -1800, 0, r=True, g=True, b=False, a=False)
    wire(position, "", flat, "")

    def scaled(name, default, y):
        scale = node(unreal.MaterialExpressionScalarParameter, -1800, y, parameter_name=name, default_value=default)
        uv = node(unreal.MaterialExpressionMultiply, -1600, y)
        wire(flat, "", uv, "A")
        wire(scale, "", uv, "B")
        return uv

    dirt_uv = scaled("DirtScale", 1.0 / 350.0, -400)       # rubble-strewn dirt: one sheet per 3.5 m
    slab_uv = scaled("SlabScale", 1.0 / 520.0, 200)        # the old paving at an unrelated period, so the two never line up
    # each sampler must match its texture's compression (TC_Masks textures need the Masks sampler), or the material fails to compile and the engine draws its grey grid instead
    samplers = {"normal": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, "masks": unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, "colour": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR}

    def sample(name, uv, x, y, kind):
        node_ = node(unreal.MaterialExpressionTextureSample, x, y, texture=t[name], sampler_type=samplers[kind])
        wire(uv, "", node_, "UVs")
        return node_

    dirt_bc = sample("T_StoneDebris01_BC", dirt_uv, -1300, -700, "colour")
    dirt_n = sample("T_StoneDebris01_N", dirt_uv, -1300, -450, "normal")
    dirt_orm = sample("T_StoneDebris01_ORM", dirt_uv, -1300, -200, "masks")
    slab_bc = sample("T_StoneFloor01_BC", slab_uv, -1300, 100, "colour")
    slab_n = sample("T_StoneFloor01_N", slab_uv, -1300, 350, "normal")
    slab_orm = sample("T_StoneFloor01_ORM", slab_uv, -1300, 600, "masks")
    slab_cut = sample("T_StoneFloor01_MSK", slab_uv, -1300, 850, "masks")   # red: where a slab is, black: the gaps between them
    # where the paving survived: a slow two-level noise over the floor (about 24 m per feature), shifted by Paving (higher: more courtyard; at -.3 about half the floor is paved,
    # since a whole slab needs noise + Paving above .2, and its chipped edges fade in just below that)
    noise = node(unreal.MaterialExpressionNoise, -1300, 1150, scale=0.0004, levels=2, output_min=0.0, output_max=1.0, quality=1)  # unconnected, it samples the world position
    paving = node(unreal.MaterialExpressionScalarParameter, -1300, 1400, parameter_name="Paving", default_value=-0.3)
    survived = node(unreal.MaterialExpressionAdd, -1050, 1150)
    wire(noise, "", survived, "A")
    wire(paving, "", survived, "B")
    # a slab shows where its cut is solid and the paving survived there: (cut + survived - 1.2) sharpened, so the slabs break up at the edge of a patch, not fade
    both = node(unreal.MaterialExpressionAdd, -850, 950)
    wire(slab_cut, "R", both, "A")
    wire(survived, "", both, "B")
    shifted = node(unreal.MaterialExpressionSubtract, -700, 950, const_b=1.2)
    wire(both, "", shifted, "A")
    steep = node(unreal.MaterialExpressionMultiply, -550, 950, const_b=5.0)
    wire(shifted, "", steep, "A")
    alpha = node(unreal.MaterialExpressionSaturate, -400, 950)
    wire(steep, "", alpha, "")

    def mix(a, b, x, y):
        lerp = node(unreal.MaterialExpressionLinearInterpolate, x, y)
        wire(a, "RGB", lerp, "A")
        wire(b, "RGB", lerp, "B")
        wire(alpha, "", lerp, "Alpha")
        return lerp

    colour = mix(dirt_bc, slab_bc, -300, -400)
    normal = mix(dirt_n, slab_n, -300, 0)
    masks = mix(dirt_orm, slab_orm, -300, 400)
    tint = node(unreal.MaterialExpressionVectorParameter, -300, -650, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    # Ash: how far the pack's warm brown earth is drained toward grey (an ashen land); 0 keeps the pack's colour
    ash = node(unreal.MaterialExpressionScalarParameter, -300, -800, parameter_name="Ash", default_value=0.4)
    ashen = node(unreal.MaterialExpressionDesaturation, -150, -550)
    wire(colour, "", ashen, "")
    wire(ash, "", ashen, "Fraction")
    tinted = node(unreal.MaterialExpressionMultiply, -100, -450)
    wire(ashen, "", tinted, "A")
    wire(tint, "", tinted, "B")
    occlusion = node(unreal.MaterialExpressionComponentMask, -100, 350, r=True, g=False, b=False, a=False)
    roughness = node(unreal.MaterialExpressionComponentMask, -100, 450, r=False, g=True, b=False, a=False)
    wire(masks, "", occlusion, "")
    wire(masks, "", roughness, "")
    require(edit.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR), "base colour")
    require(edit.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL), "normal")
    require(edit.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS), "roughness")
    require(edit.connect_material_property(occlusion, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION), "occlusion")
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)  # the floor is an instanced batch
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_ArenaGround")
    report["imported"].append(material.get_path_name())
    return material


def import_ground():
    """Phase H: the shrunk ground textures and M_ArenaGround."""
    assets.make_directory(GROUND_FOLDER)
    textures = {name: import_ground_texture(name, kind) for name, kind in GROUND_TEXTURES}
    create_ground_material(textures)


def create_decor_material(default_colour, default_normal, default_masks):
    """M_DecorPBR: the pack's own PBR set-up rebuilt (colour x Tint, normal, ORM = occlusion, roughness, metallic, all on UV 0). The game drives Tint per sky stage.
    Each parameter's default texture must match its sampler (a colour texture under the Masks sampler fails the whole material, and the engine then draws its grey grid on every
    instance), so the defaults are one of the imported sets."""
    package = DECOR_FOLDER + "/M_DecorPBR"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_DecorPBR", DECOR_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_DecorPBR")
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

    colour = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, -300, parameter_name="BaseColor", texture=default_colour,
                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    normal = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, 0, parameter_name="Normal", texture=default_normal,
                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    masks = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, 300, parameter_name="ORM", texture=default_masks,
                 sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    tint = node(unreal.MaterialExpressionVectorParameter, -900, -550, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    tinted = node(unreal.MaterialExpressionMultiply, -500, -400)
    wire(colour, "RGB", tinted, "A")
    wire(tint, "", tinted, "B")
    require(edit.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR), "base colour")
    require(edit.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL), "normal")
    require(edit.connect_material_property(masks, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION), "occlusion")
    require(edit.connect_material_property(masks, "G", unreal.MaterialProperty.MP_ROUGHNESS), "roughness")
    require(edit.connect_material_property(masks, "B", unreal.MaterialProperty.MP_METALLIC), "metallic")
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)  # the scenery is instanced
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE, True)  # and its meshes are Nanite (a material without the flag draws as the engine's grey grid)
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_DecorPBR")
    report["imported"].append(material.get_path_name())
    return material


def decor_set(mesh):
    best = ""
    for prefix, name in DECOR_SET_OF:
        if mesh.startswith(prefix) and len(prefix) > len(best):
            best, found = prefix, name
    return found if best else None


def import_decor():
    """Phase I: the scenery's own textures (exported by phase G, shrunk to 2048), one material instance per texture set, and every SM_DC mesh's slots set to its own set."""
    assets.make_directory(DECOR_FOLDER)
    textures = {name: import_ground_texture(name, kind, DECOR_FOLDER) for name, kind in DECOR_TEXTURES}
    parent = create_decor_material(textures["T_LargeRock01_BC"], textures["T_LargeRock01_N"], textures["T_LargeRock01_ORM"])
    errors = unreal.MaterialEditingLibrary.get_statistics(parent) if hasattr(unreal.MaterialEditingLibrary, "get_statistics") else None
    report["notes"].append("phase I: M_DecorPBR statistics %s" % errors)
    edit = unreal.MaterialEditingLibrary
    instances = {}
    for set_name, stem in DECOR_TEXTURE_STEM.items():
        package = DECOR_FOLDER + "/MI_DC_" + set_name
        if assets.does_asset_exist(package):
            assets.delete_asset(package)  # rebuilt from scratch: an instance made before its parent existed keeps no parameters
        mi = tools.create_asset("MI_DC_" + set_name, DECOR_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        require(mi is not None, "Could not create MI_DC_" + set_name)
        edit.set_material_instance_parent(mi, parent)
        values = []
        for parameter, suffix in (("BaseColor", "_BC"), ("Normal", "_N"), ("ORM", "_ORM")):
            if not edit.set_material_instance_texture_parameter_value(mi, parameter, textures[stem + suffix]):
                info = unreal.MaterialParameterInfo()
                info.set_editor_property("name", parameter)
                value = unreal.TextureParameterValue()
                value.set_editor_property("parameter_info", info)
                value.set_editor_property("parameter_value", textures[stem + suffix])
                values.append(value)
        if values:  # the library refused (its parameter cache was stale): the values are written straight into the instance
            mi.set_editor_property("texture_parameter_values", values)
            report["notes"].append("phase I: MI_DC_%s textures written directly (%d)" % (set_name, len(values)))
        edit.update_material_instance(mi)
        assets.save_loaded_asset(mi, False)
        instances[set_name] = mi
        report["imported"].append(mi.get_path_name())
    dressed, bare = [], []
    for name in DC_MESHES:
        path = "/Game/Art/Meshes/SM_DC_" + name
        mesh = unreal.load_asset(path) if assets.does_asset_exist(path) else None
        set_name = decor_set(name)
        if mesh is None or set_name is None:
            bare.append(name)
            continue
        for slot in range(len(mesh.get_editor_property("static_materials"))):
            mesh.set_material(slot, instances[set_name])
        assets.save_loaded_asset(mesh, False)
        dressed.append(name)
    report["notes"].append("phase I: %d meshes wear their own textures; left on the concrete: %s" % (len(dressed), bare))


def create_atlas_material(default_colour, default_normal, default_masks):
    """M_DecorAtlas: M_DecorPBR for a UDIM set stitched into an atlas. Per pixel, UDIM tile t = floor(u) + Stride * floor(v) is found in cell (t mod Cols, t div Cols) of the
    atlas; the samples take their mip from the derivatives of the original UV (scaled to the cell), so the jump between cells leaves no seam."""
    package = DECOR_FOLDER + "/M_DecorAtlas"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_DecorAtlas", DECOR_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_DecorAtlas")
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

    uv = node(unreal.MaterialExpressionTextureCoordinate, -1600, 0)
    cols = node(unreal.MaterialExpressionScalarParameter, -1600, 200, parameter_name="Cols", default_value=4.0)
    rows = node(unreal.MaterialExpressionScalarParameter, -1600, 300, parameter_name="Rows", default_value=2.0)
    stride = node(unreal.MaterialExpressionScalarParameter, -1600, 400, parameter_name="Stride", default_value=4.0)
    inputs = []
    for name in ("UV", "Cols", "Rows", "Stride"):
        item = unreal.CustomInput()
        item.set_editor_property("input_name", name)
        inputs.append(item)
    remap = node(unreal.MaterialExpressionCustom, -1300, 100, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT2, description="UDIM to atlas",
                 code="float2 Tile=floor(UV);float T=Tile.x+Stride*Tile.y;float2 Cell=float2(fmod(T,Cols),floor(T/Cols));return (Cell+frac(UV))/float2(Cols,Rows);")
    remap.set_editor_property("inputs", inputs)
    wire(uv, "", remap, "UV")
    wire(cols, "", remap, "Cols")
    wire(rows, "", remap, "Rows")
    wire(stride, "", remap, "Stride")
    grid = node(unreal.MaterialExpressionAppendVector, -1300, 350)
    wire(cols, "", grid, "A")
    wire(rows, "", grid, "B")
    ddx = node(unreal.MaterialExpressionDDX, -1300, 450)
    ddy = node(unreal.MaterialExpressionDDY, -1300, 550)
    wire(uv, "", ddx, "Value")
    wire(uv, "", ddy, "Value")
    ddx_cell = node(unreal.MaterialExpressionDivide, -1100, 450)
    ddy_cell = node(unreal.MaterialExpressionDivide, -1100, 550)
    wire(ddx, "", ddx_cell, "A")
    wire(grid, "", ddx_cell, "B")
    wire(ddy, "", ddy_cell, "A")
    wire(grid, "", ddy_cell, "B")
    samplers = {"BaseColor": (default_colour, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR), "Normal": (default_normal, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
                "ORM": (default_masks, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)}
    samples = {}
    for row, (name, (texture, sampler)) in enumerate(samplers.items()):
        sample = node(unreal.MaterialExpressionTextureSampleParameter2D, -800, -300 + row * 300, parameter_name=name, texture=texture, sampler_type=sampler,
                      mip_value_mode=unreal.TextureMipValueMode.TMVM_DERIVATIVE)
        wire(remap, "", sample, "UVs")
        wire(ddx_cell, "", sample, "DDX(UVs)")
        wire(ddy_cell, "", sample, "DDY(UVs)")
        samples[name] = sample
    tint = node(unreal.MaterialExpressionVectorParameter, -800, -550, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    tinted = node(unreal.MaterialExpressionMultiply, -500, -400)
    wire(samples["BaseColor"], "RGB", tinted, "A")
    wire(tint, "", tinted, "B")
    require(edit.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR), "base colour")
    require(edit.connect_material_property(samples["Normal"], "RGB", unreal.MaterialProperty.MP_NORMAL), "normal")
    require(edit.connect_material_property(samples["ORM"], "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION), "occlusion")
    require(edit.connect_material_property(samples["ORM"], "G", unreal.MaterialProperty.MP_ROUGHNESS), "roughness")
    require(edit.connect_material_property(samples["ORM"], "B", unreal.MaterialProperty.MP_METALLIC), "metallic")
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE, True)
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_DecorAtlas")
    report["imported"].append(material.get_path_name())
    return material


def import_decor_atlases():
    """Phase J: the stitched UDIM atlases, M_DecorAtlas, one MI_DC_<set> per atlas set (its grid as parameters), and the pillars, walls, bridge pieces and inferno rocks dressed."""
    assets.make_directory(DECOR_FOLDER)
    textures = {}
    for set_name, (stem, cols, rows) in DECOR_ATLASES.items():
        for suffix, kind in (("_BC", "colour"), ("_N", "normal"), ("_ORM", "masks")):
            name = stem + suffix + "_Atlas"
            textures[name] = import_ground_texture(name, kind, DECOR_FOLDER)
    first = DECOR_ATLASES["LargePillars"][0]
    parent = create_atlas_material(textures[first + "_BC_Atlas"], textures[first + "_N_Atlas"], textures[first + "_ORM_Atlas"])
    edit = unreal.MaterialEditingLibrary
    instances = {}
    for set_name, (stem, cols, rows) in DECOR_ATLASES.items():
        package = DECOR_FOLDER + "/MI_DC_" + set_name
        if assets.does_asset_exist(package):
            assets.delete_asset(package)
        mi = tools.create_asset("MI_DC_" + set_name, DECOR_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        require(mi is not None, "Could not create MI_DC_" + set_name)
        edit.set_material_instance_parent(mi, parent)
        texture_values = []
        for parameter, suffix in (("BaseColor", "_BC"), ("Normal", "_N"), ("ORM", "_ORM")):
            info = unreal.MaterialParameterInfo()
            info.set_editor_property("name", parameter)
            value = unreal.TextureParameterValue()
            value.set_editor_property("parameter_info", info)
            value.set_editor_property("parameter_value", textures[stem + suffix + "_Atlas"])
            texture_values.append(value)
        mi.set_editor_property("texture_parameter_values", texture_values)
        scalar_values = []
        for parameter, number in (("Cols", float(cols)), ("Rows", float(rows)), ("Stride", 4.0)):
            info = unreal.MaterialParameterInfo()
            info.set_editor_property("name", parameter)
            value = unreal.ScalarParameterValue()
            value.set_editor_property("parameter_info", info)
            value.set_editor_property("parameter_value", number)
            scalar_values.append(value)
        mi.set_editor_property("scalar_parameter_values", scalar_values)
        edit.update_material_instance(mi)
        assets.save_loaded_asset(mi, False)
        instances[set_name] = mi
        report["imported"].append(mi.get_path_name())
    dressed = []
    for name in DC_MESHES:
        set_name = next((s for prefix, s in DECOR_ATLAS_OF if name.startswith(prefix)), None)
        path = "/Game/Art/Meshes/SM_DC_" + name
        if set_name is None or not assets.does_asset_exist(path):
            continue
        mesh = unreal.load_asset(path)
        for slot in range(len(mesh.get_editor_property("static_materials"))):
            mesh.set_material(slot, instances[set_name])
        assets.save_loaded_asset(mesh, False)
        dressed.append(name)
    report["notes"].append("phase J: %d meshes wear their UDIM sets through atlases: %s" % (len(dressed), dressed))


BLOCK_TEXTURES = (("T_ConcreteWall03_BC", "colour"), ("T_ConcreteWall03_N", "normal"), ("T_ConcreteWall03_ORM", "masks"))


def create_block_material(colour_tex, normal_tex, masks_tex):
    """M_ArenaBlock: weathered concrete for the Endless cover blocks and walls, projected from the three world axes (triplanar) so a block of any size and stretch shows it at one
    scale on every face - colour, ambient occlusion, roughness and the relief of its normal map (blended per axis in world space; the material takes a world-space normal)."""
    package = BLOCK_FOLDER + "/M_ArenaBlock"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_ArenaBlock", BLOCK_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    require(material is not None, "Could not create M_ArenaBlock")
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)
    material.set_editor_property("tangent_space_normal", False)

    def node(cls, x, y, **properties):
        obj = edit.create_material_expression(material, cls, x, y)
        require(obj is not None, "Could not create material expression " + cls.__name__)
        for key, value in properties.items():
            obj.set_editor_property(key, value)
        return obj

    def wire(a, output, b, input_name):
        require(edit.connect_material_expressions(a, output, b, input_name), "Material connection failed: " + input_name)

    position = node(unreal.MaterialExpressionWorldPosition, -1400, 0)
    normal_ws = node(unreal.MaterialExpressionVertexNormalWS, -1400, 150)
    scale = node(unreal.MaterialExpressionScalarParameter, -1400, 300, parameter_name="TileScale", default_value=1.0 / 380.0)  # one sheet per 3.8 m
    textures = {"Colour": (colour_tex, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR), "Normal": (normal_tex, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
                "ORM": (masks_tex, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)}
    objects = {}
    for row, (name, (tex, sampler)) in enumerate(textures.items()):
        objects[name] = node(unreal.MaterialExpressionTextureObjectParameter, -1400, 450 + row * 150, parameter_name=name, texture=tex, sampler_type=sampler)
    weights = "float3 W=pow(abs(N),4);W/=dot(W,1);"
    colour_code = weights + ("float3 A=Texture2DSample(Tex,TexSampler,P.yz*S).rgb,B=Texture2DSample(Tex,TexSampler,P.xz*S).rgb,C=Texture2DSample(Tex,TexSampler,P.xy*S).rgb;"
                             "return A*W.x+B*W.y+C*W.z;")
    normal_code = weights + ("float2 a=Texture2DSample(Tex,TexSampler,P.yz*S).xy*2-1,b=Texture2DSample(Tex,TexSampler,P.xz*S).xy*2-1,c=Texture2DSample(Tex,TexSampler,P.xy*S).xy*2-1;"
                             "float3 ta=float3(a,sqrt(saturate(1-dot(a,a)))),tb=float3(b,sqrt(saturate(1-dot(b,b)))),tc=float3(c,sqrt(saturate(1-dot(c,c))));"
                             "float3 X=float3(ta.z*sign(N.x),ta.x,ta.y),Y=float3(tb.x,tb.z*sign(N.y),tb.y),Z=float3(tc.x,tc.y,tc.z*sign(N.z));"
                             "return normalize(X*W.x+Y*W.y+Z*W.z);")
    customs = {}
    for row, (name, code) in enumerate((("Colour", colour_code), ("ORM", colour_code), ("Normal", normal_code))):
        inputs = []
        for pin in ("Tex", "P", "N", "S"):
            item = unreal.CustomInput()
            item.set_editor_property("input_name", pin)
            inputs.append(item)
        custom = node(unreal.MaterialExpressionCustom, -900, -200 + row * 300, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, description="Triplanar " + name, code=code)
        custom.set_editor_property("inputs", inputs)
        wire(objects[name], "", custom, "Tex")
        wire(position, "", custom, "P")
        wire(normal_ws, "", custom, "N")
        wire(scale, "", custom, "S")
        customs[name] = custom
    tint = node(unreal.MaterialExpressionVectorParameter, -900, -450, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    tinted = node(unreal.MaterialExpressionMultiply, -600, -300)
    wire(customs["Colour"], "", tinted, "A")
    wire(tint, "", tinted, "B")
    occlusion = node(unreal.MaterialExpressionComponentMask, -600, 0, r=True, g=False, b=False, a=False)
    roughness = node(unreal.MaterialExpressionComponentMask, -600, 100, r=False, g=True, b=False, a=False)
    wire(customs["ORM"], "", occlusion, "")
    wire(customs["ORM"], "", roughness, "")
    metallic = node(unreal.MaterialExpressionScalarParameter, -600, 200, parameter_name="Metallic", default_value=0.0)
    require(edit.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR), "base colour")
    require(edit.connect_material_property(customs["Normal"], "", unreal.MaterialProperty.MP_NORMAL), "normal")
    require(edit.connect_material_property(occlusion, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION), "occlusion")
    require(edit.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS), "roughness")
    require(edit.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC), "metallic")
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE, True)
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "Could not save M_ArenaBlock")
    report["imported"].append(material.get_path_name())
    return material


def import_block():
    """Phase K: the weathered-concrete set (exported by phase G, shrunk to 2048) and M_ArenaBlock."""
    assets.make_directory(BLOCK_FOLDER)
    textures = {name: import_ground_texture(name, kind, BLOCK_FOLDER) for name, kind in BLOCK_TEXTURES}
    create_block_material(textures["T_ConcreteWall03_BC"], textures["T_ConcreteWall03_N"], textures["T_ConcreteWall03_ORM"])


GASP_STEPS = VAULT.parent / "GameAnimationSample_5.8" / "data" / "Content" / "Audio" / "Foley" / "SoundWaves" / "Sneaker" / "Concrete"
HORROR_ROAR = VAULT.parent / "FREEComp8a724140adb8V1" / "data" / "Content" / "HorrorTemplate" / "Audio" / "SW_Monster.uasset"
STEP_VARIANTS = 12
# (GASP clip stem, how many, our name prefix): the running set keeps its old S_StepNN names so older code still finds it.
STEP_SETS = (("run", 12, "S_Step"), ("walk", 12, "S_StepWalk"), ("strafe", 8, "S_StepStrafe"), ("runBackwds", 8, "S_StepBack"), ("land", 8, "S_Land"), ("jump", 6, "S_Jump"))


def import_audio():
    staged = []
    steps = ROOT / "Content" / "Audio" / "Foley" / "SoundWaves" / "Sneaker" / "Concrete"
    horror = ROOT / "Content" / "HorrorTemplate" / "Audio"
    wanted = []  # (staged package, new folder, new name)
    if GASP_STEPS.is_dir():
        steps.mkdir(parents=True, exist_ok=True)
        for clip, count, prefix in STEP_SETS:
            for n in range(1, count + 1):
                source = GASP_STEPS / ("Foley_fs_1p_sneaker_concrete_%s_%02d.uasset" % (clip, n))
                if source.is_file():
                    shutil.copy2(source, steps / source.name)
                    wanted.append(("/Game/Audio/Foley/SoundWaves/Sneaker/Concrete/" + source.stem, "/Game/Audio/Imported/Foley", "%s%02d" % (prefix, n)))
                else:
                    report["notes"].append("phase E: missing " + source.name)
    else:
        report["notes"].append("phase E: no Game Animation Sample audio at " + str(GASP_STEPS))
    # The Horror Template's monster sound is 6.4 s, stereo, 24 kHz and unheard (nothing here can listen), too long and too unknown for the 1.5 s boss roar: it is only imported when asked for.
    if HORROR_ROAR.is_file() and os.environ.get("ASHEN_IMPORT_MONSTER") == "1":
        horror.mkdir(parents=True, exist_ok=True)
        shutil.copy2(HORROR_ROAR, horror / HORROR_ROAR.name)
        wanted.append(("/Game/HorrorTemplate/Audio/SW_Monster", "/Game/Audio", "E_BossRoar"))
    else:
        report["notes"].append("phase E: no Horror Template monster sound at " + str(HORROR_ROAR))
    if not wanted:
        return
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Audio", "/Game/HorrorTemplate"], True)
    audio = report.setdefault("audio", {})
    failed = []
    for package, folder, name in wanted:
        original = unreal.load_asset(package)
        if not isinstance(original, unreal.SoundWave):
            failed.append(package)
            continue
        assets.make_directory(folder)
        if assets.does_asset_exist(folder + "/" + name) and os.environ.get("ASHEN_FORCE_IMPORT") != "1":  # already imported by an earlier run: keep it
            audio[name] = {"kept": True}
            continue
        copy = tools.duplicate_asset(name, folder, original)
        if copy is None:
            failed.append(package)
            continue
        assets.save_loaded_asset(copy, False)
        info = {}
        for key, prop in (("duration", "duration"), ("channels", "num_channels"), ("sample_rate", "sample_rate")):
            try:
                info[key] = round(float(copy.get_editor_property(prop)), 3)
            except Exception:  # a property this engine build does not expose is just left out of the report
                pass
        audio[name] = info
        report["imported"].append(copy.get_path_name())
    assets.delete_directory("/Game/Audio/Foley")
    assets.delete_directory("/Game/HorrorTemplate")
    shutil.rmtree(ROOT / "Content" / "Audio" / "Foley", ignore_errors=True)
    shutil.rmtree(ROOT / "Content" / "HorrorTemplate", ignore_errors=True)
    report["notes"].append("phase E: %d sounds imported, failed: %s" % (len(audio), failed))
    require(not failed, "Some audio did not import: " + ", ".join(failed))


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
    # Each phase stands alone: one failing (a missing pack, say) must not stop the others.
    errors = []
    chosen = os.environ.get("ASHEN_PHASES", "ABCE").upper()
    for letter, phase in (("A", main), ("B", import_concrete), ("C", import_dragon_cave), ("E", import_audio), ("G", export_dc_textures), ("H", import_ground), ("I", import_decor), ("J", import_decor_atlases), ("K", import_block)):
        if letter not in chosen:
            continue
        try:
            phase()
        except Exception:
            errors.append(phase.__name__ + ": " + traceback.format_exc())
            unreal.log_error(errors[-1])
    report["ok"] = not errors
    if errors:
        report["error"] = "\n".join(errors)
except Exception:
    report["ok"] = False
    report["error"] = traceback.format_exc()
    unreal.log_error(report["error"])
finally:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2))
    unreal.log("AshenVaultImport: " + ("OK" if report.get("ok") else "FAILED") + " - see " + str(REPORT))
