"""0.24.2 Phase F (the hero swap to the Unreal Mannequin): editor commandlet, import only, no gameplay.
    UnrealEditor-Cmd.exe AshenSpiral.uproject /Engine/Maps/Entry -run=pythonscript -script=<abs path>/Tools/build_hero_animation.py -unattended -nosplash -NullRHI -NoSound

Needs the template Mannequin content at /Game/Characters/Mannequins (Tools/import_mannequin.ps1 copies it from the engine's own Third Person template resources).
1. ABP_AshenHero: a copy of the template's ABP_Unarmed (strafing locomotion, jump/fall/land, foot IK) with an UPPER-BODY layer added, so an attack can play on the arms and torso
   while the legs keep running:  Main States -> Save cached pose 'HeroBase' -> (Use 'HeroBase' -> Slot 'UpperBody') blended from spine_01 up over (Use 'HeroBase')
   -> Slot 'DefaultSlot' (full body: dashes, smashes, the downed fall) -> foot IK -> Output.
2. The weapon clips: Paragon animations retargeted to Manny (Fab, licence cleared by Josue 2026-10-19), imported onto SK_Mannequin as /Game/Characters/Hero/Anims/<Hero>_<Clip>.
A report (what was built, every clip's length) goes to Saved/AshenHeroAnimation.json. Set ASHEN_VAULT to override the vault folder; ASHEN_HERO_STEPS=abp or =clips to run one step.
"""
import json
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
REPORT = ROOT / "Saved" / "AshenHeroAnimation.json"
VAULT = Path(os.environ.get("ASHEN_VAULT", r"C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\FabLibrary"))
PACK = VAULT / "Paragon_animations_retargeted_to_Manny-e6de87b1" / "fbx" / "paragonanimationsretarge_extracted" / "ParagonAnimationsRetargetedToManny"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
TEMPLATE_ABP = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"
HERO = "/Game/Characters/Hero"
ABP_NAME = "ABP_AshenHero"
ANIMS = HERO + "/Anims"
# (pack folder, clip path inside it): what each weapon and move plays. Names become <Hero>_<Clip> (the hero folder without "Manny").
CLIPS = [
    # Cinder Blade: a fast three-hit sword chain, the third a lunge (Feng Mao's clips do not import: "failed to create asset")
    ("AuroraManny", r"Attacks\Primary_Attack_Fast_A"), ("AuroraManny", r"Attacks\Primary_Attack_Fast_B"), ("AuroraManny", r"Attacks\Primary_Attack_Fast_C"),
    # Hoarfrost Maul: left swing, right swing, the planted smash; the leap and its landing
    ("GruxManny", r"Attack\PrimaryAttack_LA"), ("GruxManny", r"Attack\PrimaryAttack_RA"), ("GruxManny", r"Ability\DoublePain"),
    ("steelmanny", r"Ability\Steel_Ability_GroundSmah_Start"), ("steelmanny", r"Ability\Steel_Ability_GroundSmash_Loop"), ("steelmanny", r"Ability\Steel_Ability_GroundSmash_End"),
    ("terramanny", r"Attack\Primary_Melee_C_slow"),
    # Ash Bow
    ("SparrowManny", r"Attack\Primary_Fire_Fast"), ("SparrowManny", r"Attack\Primary_Fire_Med"),
    # Spiral Glaive (thrown blades) and Ember Staff (marks): casts
    ("gideonManny", r"Attack\Primary_Attack_A_Medium"), ("gideonManny", r"Attack\Primary_Attack_B_Medium"), ("gideonManny", r"Attack\Primary_Attack_C_Medium"), ("gideonManny", r"Attack\RMB_Cast"),
    # Rift Scythe: wide two-handed sweeps
    ("AuroraManny", r"Attacks\Primary_Attack_Slow_A"), ("AuroraManny", r"Attacks\Primary_Attack_Slow_B"), ("AuroraManny", r"Attacks\Primary_Attack_Slow_C"),
    ("KwangManny", r"Attack\PrimaryAttack_A_Slow"), ("KwangManny", r"Attack\PrimaryAttack_B_Slow"), ("KwangManny", r"Attack\PrimaryAttack_C_Slow"),
    # Cinder Scattergun, Tempest Rotary, Rift Mortar
    ("MurdockManny", r"Fire_Fast"), ("MurdockManny", r"Moving_Fire_Fast"), ("TwinBlastManny", r"DoubleShot_Fire_Rt_Fast"), ("WraithManny", r"Ability\Ability_Q_Fire"),
    # the dash: a directional evade; being hit: a flinch from the side the blow came from; a heavy blow
    ("TwinBlastManny", r"Evade_Fwd"), ("TwinBlastManny", r"Evade_Bwd"), ("TwinBlastManny", r"Evade_Left"), ("TwinBlastManny", r"Evade_Right"),
    ("TwinBlastManny", r"HitReact_Front"), ("TwinBlastManny", r"HitReact_Back"), ("TwinBlastManny", r"HitReact_Left"), ("TwinBlastManny", r"HitReact_Right"), ("TwinBlastManny", r"KnockBack"),
    # 0.24.2 Phase F: weapon stances, looped on the upper body between attacks so each weapon is carried ready (ASCharacterSkeletal.cpp, StanceFor)
    ("KwangManny", r"Idle"), ("AuroraManny", r"Idle"), ("GruxManny", r"TravelMode_Idle"), ("steelmanny", r"Steel_Idle_Combat"), ("SparrowManny", r"idle"),
    ("YinManny", r"Idle_Combat"), ("gideonManny", r"Idle"), ("MurdockManny", r"Idle"), ("WraithManny", r"Idle_Combat"), ("TwinBlastManny", r"Idle"),
]
report = {"abp": {}, "clips": [], "failed": [], "notes": []}
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
BEL = unreal.BlueprintEditorLibrary


def log(message):
    unreal.log("ASHENHERO " + str(message))
    report["notes"].append(str(message))


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def pins(node):
    return [(str(p.get_pin_name()), int(p.get_pin_direction().value)) for p in node.list_all_pins()]


def find_pin(node, name, output):
    for p in node.list_all_pins():
        if str(p.get_pin_name()) == name and (int(p.get_pin_direction().value) == 1) == output:
            return p
    raise RuntimeError("no %s pin %s on %s; pins %s" % ("output" if output else "input", name, node.get_name(), pins(node)))


def build_abp():
    path = HERO + "/" + ABP_NAME
    assets.make_directory(HERO)
    if assets.does_asset_exist(path):
        assets.delete_asset(path)
    template = unreal.load_asset(TEMPLATE_ABP)
    require(template is not None, "the template Mannequin content is missing (run Tools/import_mannequin.ps1)")
    abp = tools.duplicate_asset(ABP_NAME, HERO, template)
    require(abp is not None, "could not duplicate ABP_Unarmed")
    ed = unreal.BlueprintGraphEditor.get_graph_editor_by_name(abp, "AnimGraph")
    nodes = {n.get_name(): n for n in ed.list_all_nodes()}
    main = [n for n in nodes.values() if n.get_node_title() == "Main States"][0]
    default_slot = [n for n in nodes.values() if "Slot" in n.get_name()][0]
    main_pose = find_pin(main, "Pose", True)
    slot_source = find_pin(default_slot, "Source", False)
    main_pose.break_pin_links()
    # cache the locomotion so it can feed both sides of the layer
    save = ed.create_node_from_name("Animation|CachedPoses|NewSavecachedpose...", unreal.Vector2D(200, -300), [main_pose])
    require(save is not None, "no Save cached pose node")
    try:
        save.set_editor_property("cache_name", "HeroBase")
    except Exception as e:
        log("cache_name: %s" % e)
    if not main_pose.list_connected_pins():
        main_pose.try_create_connection(find_pin(save, "Pose", False))
    # the use-cached-pose actions exist once the save node does
    available = ed.list_available_nodes([slot_source])
    use_names = [a for a in available if "UseCachedPose" in a.replace(" ", "") or "Usecachedpose" in a]
    log("use-cached-pose actions: %s" % use_names[:6])
    use_action = [a for a in use_names if "HeroBase" in a] or use_names
    require(use_action, "no Use cached pose action")
    use_base = ed.create_node_from_name(use_action[0], unreal.Vector2D(600, -100), [])
    use_upper = ed.create_node_from_name(use_action[0], unreal.Vector2D(600, 100), [])
    require(use_base is not None and use_upper is not None, "no Use cached pose nodes")
    upper = ed.create_node_from_name("Animation|Montage|Slot'DefaultSlot'", unreal.Vector2D(900, 100), [find_pin(use_upper, "Pose", True)])
    require(upper is not None, "no slot node")
    node = upper.get_editor_property("node")
    node.set_editor_property("slot_name", "UpperBody")
    upper.set_editor_property("node", node)
    layer = ed.create_node_from_name("Animation|Blends|Layeredblendperbone", unreal.Vector2D(1200, 0), [slot_source])
    require(layer is not None, "no layered blend node")
    log("layer pins %s" % pins(layer))
    links = [(find_pin(use_upper, "Pose", True), find_pin(upper, "Source", False)),
             (find_pin(use_base, "Pose", True), find_pin(layer, "BasePose", False)),
             (find_pin(upper, "Pose", True), find_pin(layer, "BlendPoses_0", False)),
             (find_pin(layer, "Pose", True), slot_source)]
    for a, b in links:
        if b in a.list_connected_pins():
            continue
        a.break_pin_links() if False else None
        ok = a.try_create_connection(b)
        require(ok, "could not link %s -> %s" % (a.get_pin_name(), b.get_pin_name()))
    # from the first spine bone up: arms, torso and head follow the attack; mesh-space rotation keeps the torso aimed while the hips turn with the run
    blend = layer.get_editor_property("node")
    setup = blend.get_editor_property("layer_setup")
    pose = unreal.InputBlendPose()
    pose.set_editor_property("branch_filters", [unreal.BranchFilter(bone_name="spine_01", blend_depth=0)])
    blend.set_editor_property("layer_setup", [pose])
    blend.set_editor_property("mesh_space_rotation_blend", True)
    layer.set_editor_property("node", blend)
    ok = BEL.compile_blueprint(abp)
    report["abp"] = {"path": abp.get_path_name(), "compiled": bool(ok), "nodes": [n.get_name() for n in ed.list_all_nodes()]}
    assets.save_loaded_asset(abp, False)
    log("ABP_AshenHero built, compile=%s" % ok)


def import_clips():
    skeleton = unreal.load_asset(SKELETON)
    require(skeleton is not None, "no " + SKELETON)
    assets.make_directory(ANIMS)
    for hero, clip in CLIPS:
        source = PACK / hero / (clip + ".FBX")
        name = hero.replace("Manny", "").replace("manny", "") + "_" + Path(clip).name
        if assets.does_asset_exist(ANIMS + "/" + name) and os.environ.get("ASHEN_FORCE_IMPORT") != "1":
            asset = unreal.load_asset(ANIMS + "/" + name)
            report["clips"].append({"name": name, "length": round(float(asset.get_editor_property("sequence_length")), 3), "kept": True})
            continue
        if not source.is_file():
            report["failed"].append(name + " (no file)")
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", ANIMS)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        options = unreal.FbxImportUI()
        options.set_editor_property("automated_import_should_detect_type", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        options.set_editor_property("import_mesh", False)
        options.set_editor_property("import_animations", True)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("skeleton", skeleton)
        task.set_editor_property("options", options)
        task.set_editor_property("factory", unreal.FbxFactory())
        tools.import_asset_tasks([task])
        asset = unreal.load_asset(ANIMS + "/" + name)
        if isinstance(asset, unreal.AnimSequence):
            report["clips"].append({"name": name, "length": round(float(asset.get_editor_property("sequence_length")), 3)})
        else:
            report["failed"].append(name)


try:
    steps = os.environ.get("ASHEN_HERO_STEPS", "abp,clips")
    for step, fn in (("abp", build_abp), ("clips", import_clips)):
        if step in steps:
            try:
                fn()
            except Exception:
                report["failed"].append(step + ": " + traceback.format_exc())
                unreal.log_error(traceback.format_exc())
finally:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2))
    unreal.log("ASHENHERO done - see " + str(REPORT))
