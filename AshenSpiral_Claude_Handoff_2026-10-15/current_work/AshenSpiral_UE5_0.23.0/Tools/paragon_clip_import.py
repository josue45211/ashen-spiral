"""0.25 phase D, step 1 (editor commandlet only, no gameplay): import Paragon-retargeted-to-Manny animation clips from the vault into the THROWAWAY spike project
(Saved/Scratch/spike_project, made by Tools/spike_make_project.ps1), onto the Mannequin skeleton that project already holds. The real project is never touched and nothing is
shipped: Tools/paragon_clip_export.py samples these clips to JSON and Tools/paragon_clip_bake.py turns the motion into numeric pose curves for the game's own rig, as the 0.24
Mannequin death bake did (Docs/SKELETAL_SPIKE_0.24.md).
Run: Saved/Scratch/run_spike.ps1 -Script Tools/paragon_clip_import.py -LogName paragon_import
Set ASHEN_VAULT to override the vault folder. A report is written to Saved/Scratch/manny/paragon_import.json.
"""
import json
import os
from pathlib import Path

import unreal

VAULT = Path(os.environ.get("ASHEN_VAULT", r"C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\FabLibrary"))
PACK = VAULT / "Paragon_animations_retargeted_to_Manny-e6de87b1" / "fbx" / "paragonanimationsretarge_extracted" / "ParagonAnimationsRetargetedToManny"
OUT = Path(os.environ.get("MANNY_SPIKE_OUT", r"C:\Games\AshenSpiral\AshenSpiral_Claude_Handoff_2026-10-15\current_work\AshenSpiral_UE5_0.23.0\Saved\Scratch\manny")) / "paragon_import.json"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
DEST = "/Game/Paragon"
# (hero folder, clip path inside it without .FBX), grouped by the ability they are candidates for (Tools/paragon_clip_bake.py holds the mapping). Chosen for readable anticipation:
# the slow and heavy variants, the ability intros and the minions' own attacks; heroes' fast combo links have almost no wind-up and are left out.
CLIPS = [
    # slash, riposte (a melee swing)
    ("minionsManny", r"Attack_A"), ("minionsManny", r"Attack_B"), ("minionsManny", r"Attack_C"), ("minionsManny", r"Attack_D"),
    ("steelmanny", r"Attack\Steel_Attack_Melee_A_Slow"), ("steelmanny", r"Attack\Steel_Attack_Melee_B_Slow"), ("steelmanny", r"Attack\Steel_Attack_Melee_C_Slow"),
    ("steelmanny", r"Attack\Steel_Attack_Melee_A"), ("steelmanny", r"Attack\Steel_Attack_Melee_B"), ("steelmanny", r"Attack\Steel_Attack_Melee_C"),
    ("NarbashManny", r"Attack\Primary_Swing1_Slow"), ("NarbashManny", r"Attack\Primary_Swing2_Slow"), ("GreystoneManny", r"Attack\Attack_A_Slow"), ("GreystoneManny", r"Attack\Attack_B_Med"),
    ("CountessManny", r"Attack\Primary_Attack_A_Slow"), ("PhaseManny", r"Attack\Primary_Attack_A_Slow"), ("PhaseManny", r"Attack\Primary_Attack_B_Medium"),
    ("SerathManny", r"Attack\Primary_Attack_B_Slow"),
    # shove, guard break, parry
    ("steelmanny", r"Ability\Steel_Ability_Bash"), ("steelmanny", r"Attack\Steel_Attack_GuardBreak_A"), ("steelmanny", r"Attack\Steel_Attack_Melee_D_V1"),
    ("steelmanny", r"Ability\Steel_Ability_ShieldBlock_Deploy"),
    # ground pound
    ("steelmanny", r"Ability\Steel_Ability_GroundSmah_Start"), ("steelmanny", r"Ability\Steel_Ability_GroundSmash_Loop"), ("steelmanny", r"Ability\Steel_Ability_GroundSmash_End"),
    # charge, boss attacks, roar
    ("steelmanny", r"Attack\Steel_Attack_Melee_Charge_A"), ("steelmanny", r"Ability\Steel_Ability_CowCatcher_Start"), ("NarbashManny", r"Ability\Q_Ability_Intro"),
    ("steelmanny", r"Ability\Steel_Ability_Ultimate_Intro"), ("CountessManny", r"Ability\Ability_Ultimate"), ("NarbashManny", r"Ability\R_Ability_Intro"),
    ("SerathManny", r"Attack\Q_Ability_Attack_A"), ("minionsManny", r"Aggro_Transition_A"),
    # leap, blink, blast, shot, snipe, summon, calls
    ("PhaseManny", r"Ability\Ability_E"), ("PhaseManny", r"Ability\Ability_E_Land"), ("PhaseManny", r"Ability\Ability_Q"), ("PhaseManny", r"Ability\Ability_R_Alt"),
    ("CountessManny", r"Ability\Ability_Q"), ("CountessManny", r"Ability\Ability_E"), ("NarbashManny", r"Ability\RMB_Ability_Throw"), ("steelmanny", r"Ability\Steel_Ability_Shield_Throw"),
    ("WraithManny", r"Ability\Ability_Q_Fire"), ("WraithManny", r"Ability\Ability_RMB_Start"), ("WraithManny", r"Ability\Ability_Q_Targeting_Start"),
    ("NarbashManny", r"Ability\E_ability_intro"), ("SerathManny", r"Ability\R_Ability_Intro"), ("SerathManny", r"Ability\E_Ability_Wing_Flap"),
]
report = {"imported": [], "failed": []}
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(message):
    unreal.log("PARAGON " + str(message))


def main():
    skeleton = unreal.load_asset(SKELETON)
    if skeleton is None:
        raise RuntimeError("The spike project has no " + SKELETON + "; run Tools/spike_make_project.ps1 first")
    assets.make_directory(DEST)
    for hero, clip in CLIPS:
        source = PACK / hero / (clip + ".FBX")
        name = hero.replace("Manny", "").replace("manny", "") + "_" + Path(clip).name
        if not source.is_file():
            report["failed"].append(name + " (no file)")
            log("missing " + str(source))
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", DEST)
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
        asset = unreal.load_asset(DEST + "/" + name)
        if isinstance(asset, unreal.AnimSequence):
            length = float(asset.get_editor_property("sequence_length"))
            report["imported"].append({"name": name, "length": round(length, 3)})
            log("imported %s: %.2f s" % (name, length))
        else:
            report["failed"].append(name)
            log("FAILED " + name)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2))
    log("done: %d imported, %d failed" % (len(report["imported"]), len(report["failed"])))


main()
