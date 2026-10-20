"""Run INSIDE Unreal Editor (commandlet):
    UnrealEditor-Cmd.exe AshenSpiral.uproject /Engine/Maps/Entry -run=pythonscript -script=<absolute path>/Tools/build_boss_model.py -unattended -nosplash -NullRHI -NoSound

0.24.2 (Edison 08:30: "keep the visual polish going", "anything in Docs/0.25_ASSET_PLAN.md still unfinished"; the plan's held creatures, cleared once AI-made art was accepted):
the ward bosses as the Warrior Titan (Fab, a stylised armoured warrior on a humanoid rig with walk and run clips) instead of the 21-part armour rig. The Titan has only two clips,
so it borrows the heroes': the Mannequin's skeleton is retargeted onto the Titan's (IK rigs auto-characterised on both, a retargeter with chains mapped and the Titan's T-pose
aligned to the Mannequin's), and the hero animation blueprint and its clips are duplicated onto the Titan through it. Steps (ASHEN_BOSS_STEPS, comma separated, all by default):
  import    the Titan's mesh, textures and clips into /Game/Characters/Titan, and its material
  rig       IK_Mannequin, IK_Titan and RTG_MannyToTitan in /Game/Characters/Titan/Retarget
  retarget  ABP_AshenHero and the boss clips duplicated onto the Titan (prefix Titan_) in /Game/Characters/Titan/Anims
A report is written to Saved/AshenBossModel.json. Everything lands under Content/Characters, which git ignores (local only, like every vault import; the playtest zip carries it).
"""
import json
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
REPORT = ROOT / "Saved" / "AshenBossModel.json"
VAULT = Path(os.environ.get("ASHEN_VAULT", r"C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\FabLibrary"))
PACK = VAULT / "Warrior_Titan-9c2c6099" / "fbx" / "warrior_titan_extracted"
TITAN = "/Game/Characters/Titan"
ANIMS = TITAN + "/Anims"
RETARGET = TITAN + "/Retarget"
MANNY = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
HERO_ABP = "/Game/Characters/Hero/ABP_AshenHero"
HERO_ANIMS = "/Game/Characters/Hero/Anims"
# the hero clips a boss plays (ASEnemySkeletal.cpp: heavy swings, the ground smash, casts), with the rest of the smash, the flinches and two idles kept ready beside them;
# the Mannequin's death falls are added below
BOSS_CLIPS = ["Grux_PrimaryAttack_LA", "Grux_PrimaryAttack_RA", "Grux_DoublePain", "terra_Primary_Melee_C_slow",
              "steel_Steel_Ability_GroundSmah_Start", "steel_Steel_Ability_GroundSmash_Loop", "steel_Steel_Ability_GroundSmash_End",
              "gideon_RMB_Cast", "gideon_Primary_Attack_A_Medium", "Wraith_Ability_Q_Fire", "TwinBlast_HitReact_Front", "TwinBlast_KnockBack",
              "Grux_TravelMode_Idle", "steel_Steel_Idle_Combat"]
DEATHS = ["/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01", "/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01"]
report = {"imported": [], "notes": [], "failed": []}
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def log(message):
    unreal.log("ASHENBOSS " + str(message))
    report["notes"].append(str(message))


def import_fbx(source, destination, name, skeleton=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    if skeleton is None:  # the character itself: a skeletal mesh with its own skeleton
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", True)
        options.set_editor_property("import_animations", False)
        options.set_editor_property("create_physics_asset", False)
    else:  # a clip for that skeleton
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        options.set_editor_property("import_mesh", False)
        options.set_editor_property("import_animations", True)
        options.set_editor_property("skeleton", skeleton)
    task.set_editor_property("options", options)
    task.set_editor_property("factory", unreal.FbxFactory())
    tools.import_asset_tasks([task])
    return [str(p) for p in task.get_editor_property("imported_object_paths")]


def import_texture(path, name, kind):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", TITAN)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(TITAN + "/" + name)
    require(texture is not None, "texture import failed: " + str(path))
    texture.set_editor_property("max_texture_size", 2048)
    if kind != "colour":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if kind == "normal" else unreal.TextureCompressionSettings.TC_MASKS)
    assets.save_loaded_asset(texture, False)
    return texture


def create_material(colour, normal, metallic, roughness):
    """M_Titan: the model's own PBR maps (colour x Tint, normal, metallic, roughness) for a skinned mesh; the game sets Tint per ward."""
    package = TITAN + "/M_Titan"
    material = unreal.load_asset(package) if assets.does_asset_exist(package) else None
    if material is None:
        material = tools.create_asset("M_Titan", TITAN, unreal.Material, unreal.MaterialFactoryNew())
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)

    def node(cls, x, y, **properties):
        obj = edit.create_material_expression(material, cls, x, y)
        require(obj is not None, "material node " + cls.__name__)
        for key, value in properties.items():
            obj.set_editor_property(key, value)
        return obj

    def sample(texture, x, y, sampler):
        return node(unreal.MaterialExpressionTextureSample, x, y, texture=texture, sampler_type=sampler)

    c = sample(colour, -700, -300, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    n = sample(normal, -700, 0, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    m = sample(metallic, -700, 250, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    r = sample(roughness, -700, 500, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    tint = node(unreal.MaterialExpressionVectorParameter, -700, -550, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    glow = node(unreal.MaterialExpressionVectorParameter, -700, 750, parameter_name="Glow", default_value=unreal.LinearColor(0, 0, 0, 1))
    tinted = node(unreal.MaterialExpressionMultiply, -350, -400)
    require(edit.connect_material_expressions(c, "RGB", tinted, "A") and edit.connect_material_expressions(tint, "", tinted, "B"), "tint")
    require(edit.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR), "base colour")
    require(edit.connect_material_property(n, "RGB", unreal.MaterialProperty.MP_NORMAL), "normal")
    require(edit.connect_material_property(m, "R", unreal.MaterialProperty.MP_METALLIC), "metallic")
    require(edit.connect_material_property(r, "R", unreal.MaterialProperty.MP_ROUGHNESS), "roughness")
    require(edit.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "glow")  # the game's hit flash, rage and phase-two pulse
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH, True)
    edit.layout_material_expressions(material)
    edit.recompile_material(material)
    require(assets.save_loaded_asset(material, False), "save M_Titan")
    return material


def step_import():
    require(PACK.is_dir(), "No Warrior Titan pack at " + str(PACK))
    assets.make_directory(TITAN)
    assets.make_directory(ANIMS)
    biped = PACK / "biped"
    paths = import_fbx(biped / "Character_output.fbx", TITAN, "SK_Titan")
    log("character import: %s" % paths)
    mesh = unreal.load_asset(TITAN + "/SK_Titan")
    require(isinstance(mesh, unreal.SkeletalMesh), "SK_Titan did not import as a skeletal mesh")
    skeleton = mesh.get_editor_property("skeleton")
    # the import task saves the mesh but not the skeleton it made: without this the skeleton is gone in the next session and every clip loads without one
    require(skeleton is not None and assets.save_loaded_asset(skeleton, False), "could not save the Titan's skeleton")
    require((ROOT / "Content/Characters/Titan" / (skeleton.get_name() + ".uasset")).is_file(), "the Titan's skeleton did not reach disk")
    bounds = mesh.get_bounds()
    log("SK_Titan: skeleton %s, %d bones, bounds origin %s extent %s" % (skeleton.get_name(), len(mesh.get_editor_property("skeleton").get_editor_property("bone_tree")) if hasattr(skeleton, "get_editor_property") else -1,
                                                                       bounds.origin, bounds.box_extent))
    for clip in ("Animation_Walking_withSkin", "Animation_Running_withSkin"):
        imported = import_fbx(biped / (clip + ".fbx"), ANIMS, "Titan_" + clip.split("_")[1], skeleton)
        log("clip %s -> %s" % (clip, imported))
    textured = PACK / "Warrior_Titan_0124124852_texture_fbx"  # the same colour map the rigged FBX embeds (byte for byte); the pack ships it loose only here
    colour = import_texture(textured / "Warrior_Titan_0124124852_texture.png", "T_Titan_BC", "colour")
    normal = import_texture(biped / "texture_0_normal.png", "T_Titan_N", "normal")
    metallic = import_texture(biped / "texture_0_metallic.png", "T_Titan_M", "masks")
    roughness = import_texture(biped / "texture_0_roughness.png", "T_Titan_R", "masks")
    material = create_material(colour, normal, metallic, roughness)
    # the array's elements come back as copies: each is changed and the whole array written back (setting a property on the loop variable alone left the slot empty,
    # and the Titan drew in the engine's default clay)
    slots = []
    for slot in mesh.get_editor_property("materials"):
        slot.set_editor_property("material_interface", material)
        slots.append(slot)
    mesh.set_editor_property("materials", slots)
    require(all(s.get_editor_property("material_interface") == material for s in mesh.get_editor_property("materials")), "M_Titan did not reach SK_Titan's slots")
    require(assets.save_loaded_asset(mesh, False), "save SK_Titan")
    report["imported"].append(mesh.get_path_name())
    log("SK_Titan dressed with M_Titan on %d slot(s)" % len(slots))


def clear_output(folder, names):
    """This script's own earlier output goes from disk before anything loads it: an asset deleted after loading stays in memory in a commandlet, and its replacement
    was made under a numbered name (IK_Titan1) that nothing uses."""
    for name in names:
        require(unreal.find_object(None, "%s/%s.%s" % (folder, name, name)) is None, "%s is already loaded: run the rig step in a session of its own" % name)
        path = ROOT / "Content" / folder[len("/Game/"):] / (name + ".uasset")
        if path.is_file():
            path.unlink()


def make_rig(name, mesh_path):
    rig = tools.create_asset(name, RETARGET, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())  # (create_new_ik_rig_asset would number the name past a stale registry entry)
    require(rig is not None, "could not create " + name)
    controller = unreal.IKRigController.get_controller(rig)
    require(controller.set_skeletal_mesh(unreal.load_asset(mesh_path)), "set mesh on " + name)
    controller.apply_auto_generated_retarget_definition()
    chains = [str(c.get_editor_property("chain_name")) for c in controller.get_retarget_chains()]
    log("%s: retarget root %s, %d chains %s" % (name, controller.get_retarget_root(), len(chains), chains))
    assets.save_loaded_asset(rig, False)
    return rig


def step_rig():
    assets.make_directory(RETARGET)
    clear_output(RETARGET, ("RTG_MannyToTitan", "IK_Mannequin", "IK_Titan"))
    source = make_rig("IK_Mannequin", MANNY)
    target = make_rig("IK_Titan", TITAN + "/SK_Titan")
    rtg = tools.create_asset("RTG_MannyToTitan", RETARGET, unreal.IKRetargeter, unreal.IKRetargetFactory())
    require(rtg is not None, "could not create the retargeter")
    controller = unreal.IKRetargeterController.get_controller(rtg)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target)
    controller.add_default_ops()
    # the Titan has no root bone (its skeleton starts at Hips), so the default Root Motion op took Hips for the root and wrote the Mannequin's floor-level root onto it:
    # every clip had the hips pinned to the floor. The game moves the boss itself (in-place clips), so the op goes
    for index in reversed(range(controller.get_num_retarget_ops())):
        if isinstance(controller.get_op_controller(index), unreal.IKRetargetRootMotionController):
            require(controller.remove_retarget_op(index), "could not remove the Root Motion op")
            log("RTG_MannyToTitan: Root Motion op removed (the Titan's root is its hips)")
    controller.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, source)
    controller.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, target)
    controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    controller.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)  # the Titan's T-pose turned to match the Mannequin's A-pose
    log("RTG_MannyToTitan: %d ops" % controller.get_num_retarget_ops())
    assets.save_loaded_asset(rtg, False)


def step_retarget():
    rtg = unreal.load_asset(RETARGET + "/RTG_MannyToTitan")
    require(rtg is not None, "run the rig step first")
    source_mesh = unreal.load_asset(MANNY)
    target_mesh = unreal.load_asset(TITAN + "/SK_Titan")
    with_blueprint = os.environ.get("ASHEN_BOSS_ABP") == "1"  # the hero blueprint too (its blendspaces' clips failed to retarget in 5.8: "Unable to retrieve target USkeleton")
    wanted = ([unreal.load_asset(HERO_ABP)] if with_blueprint else []) + [unreal.load_asset(HERO_ANIMS + "/" + c) for c in BOSS_CLIPS] + [unreal.load_asset(d) for d in DEATHS]
    missing = [str(i) for i, a in enumerate(wanted) if a is None]
    require(not missing, "missing source assets at %s" % missing)
    assets.make_directory(ANIMS)
    try:
        unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            [unreal.EditorAssetLibrary.find_asset_data(a.get_path_name()) for a in wanted], source_mesh, target_mesh, rtg,
            search="", replace="", prefix="Titan_", suffix="", target_path=ANIMS, use_source_path=False, include_referenced_assets=with_blueprint, overwrite_existing_files=True)
    except RuntimeError as error:
        # each duplicate briefly has no skeleton before it is given the Titan's, and the data model reports that as an error, which Python raises after the call; the
        # retarget itself completes - every result is checked below
        log("retarget reported (checked below): %s" % str(error).splitlines()[0][:200])
    unreal.EditorAssetLibrary.save_directory(ANIMS, only_if_is_dirty=False, recursive=True)
    target_skeleton = target_mesh.get_editor_property("skeleton")
    good, bad = [], []
    for source in wanted:
        path = ANIMS + "/Titan_" + source.get_name()
        result = unreal.load_asset(path) if assets.does_asset_exist(path) else None
        if result is None:
            bad.append(source.get_name() + " (missing)")
            continue
        skeleton = result.get_editor_property("target_skeleton" if isinstance(result, unreal.AnimBlueprint) else "skeleton")
        if skeleton != target_skeleton:
            bad.append("%s (skeleton %s)" % (source.get_name(), skeleton.get_name() if skeleton else None))
            continue
        good.append("%s %.2fs" % (result.get_name(), result.get_play_length()) if hasattr(result, "get_play_length") else result.get_name())
        report["imported"].append(result.get_path_name())
    log("retargeted onto the Titan: %d good %s; %d bad %s" % (len(good), good, len(bad), bad))
    require(not bad, "some retargets failed: %s" % bad)
    # the hips ride at the Titan's own height (its walk carries them at about 87 cm), not on the floor
    options = unreal.AnimPoseEvaluationOptions()
    for name in ("Titan_Walking", "Titan_Grux_TravelMode_Idle", "Titan_Grux_PrimaryAttack_LA", "Titan_Wraith_Ability_Q_Fire"):
        clip = unreal.load_asset(ANIMS + "/" + name)
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(clip, clip.get_play_length() * .5, options)
        hips = unreal.AnimPoseExtensions.get_bone_pose(pose, "Hips", unreal.AnimPoseSpaces.WORLD).translation
        head = unreal.AnimPoseExtensions.get_bone_pose(pose, "Head", unreal.AnimPoseSpaces.WORLD).translation
        log("%s mid-clip: hips at %.0f cm, head at %.0f cm" % (name, hips.z, head.z))
        require(40 < hips.z < head.z, "%s: the hips are at %.0f cm (head %.0f)" % (name, hips.z, head.z))


try:
    steps = [s.strip() for s in os.environ.get("ASHEN_BOSS_STEPS", "import,rig,retarget").split(",") if s.strip()]
    for name in steps:
        {"import": step_import, "rig": step_rig, "retarget": step_retarget}[name]()
    report["ok"] = True
except Exception:
    report["ok"] = False
    report["error"] = traceback.format_exc()
    unreal.log_error(report["error"])
finally:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2))
    unreal.log("AshenBossModel: " + ("OK" if report.get("ok") else "FAILED") + " - see " + str(REPORT))
