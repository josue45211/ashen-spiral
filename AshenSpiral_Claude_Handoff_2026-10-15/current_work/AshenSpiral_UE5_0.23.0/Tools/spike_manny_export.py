"""0.24 skeletal spike, step 2 (editor commandlet only, no gameplay): sample the Mannequin's own animations offline and write them to JSON.
Run with Saved/Scratch/run_spike.ps1 (a throwaway project holding a copy of the engine's template content, never the real project). The JSON feeds
Tools/spike_manny_retarget.py, which retargets the motion onto Ashen Spiral's own 21-part armour rig (ASEnemyPose channels) so no skeletal asset is needed at runtime.
Component-space (world) bone transforms at 30 fps for the bones the rig cares about; roughly 6 MB for the curated animation list below.
"""
import json
import os
import unreal

OUT = os.environ.get("MANNY_SPIKE_OUT", r"C:\Games\AshenSpiral\AshenSpiral_Claude_Handoff_2026-10-15\current_work\AshenSpiral_UE5_0.23.0\Saved\Scratch\manny\manny_samples.json")
ROOT = "/Game/Characters/Mannequins/Anims/"
FPS = 30.0
BONES = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01", "head",
         "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",
         "thigh_l", "calf_l", "foot_l", "ball_l", "thigh_r", "calf_r", "foot_r", "ball_r", "weapon_l", "weapon_r"]
ANIMS = (
    ["Unarmed/Attack/MM_Attack_01", "Unarmed/Attack/MM_Attack_02", "Unarmed/Attack/MM_Attack_03", "Unarmed/Attack/MM_ChargedAttack", "Unarmed/MM_Idle",
     "Unarmed/Jump/MM_Dash", "Unarmed/Jump/MM_Jump", "Unarmed/Jump/MM_Land", "Unarmed/Jump/MM_Fall_Loop", "Unarmed/Jump/MM_WallJump"]
    + ["Unarmed/Walk/MF_Unarmed_Walk_" + d for d in ("Fwd", "Bwd", "Left", "Right", "Fwd_Left", "Fwd_Right", "Bwd_Left", "Bwd_Right")]
    + ["Unarmed/Jog/MF_Unarmed_Jog_" + d for d in ("Fwd", "Bwd", "Left", "Right")]
    + ["Death/MM_Death_Back_01", "Death/MM_Death_Front_01", "Death/MM_Death_Front_02", "Death/MM_Death_Front_03", "Death/MM_Death_Left_01", "Death/MM_Death_Right_01"]
    + ["Rifle/HitReact/MM_HitReact_Back_Med_01", "Rifle/HitReact/MM_HitReact_Front_Hvy_01", "Rifle/HitReact/MM_HitReact_Front_Lgt_01", "Rifle/HitReact/MM_HitReact_Front_Lgt_02",
       "Rifle/HitReact/MM_HitReact_Front_Med_01", "Rifle/HitReact/MM_HitReact_Front_Med_02", "Rifle/MM_Rifle_Fire", "Rifle/MM_Rifle_Reload", "Pistol/MM_Pistol_Fire"]
)


def log(message):
    unreal.log("SPIKE " + str(message))


def pack(transform):
    t, q = transform.translation, transform.rotation
    return [round(t.x, 3), round(t.y, 3), round(t.z, 3), round(q.x, 5), round(q.y, 5), round(q.z, 5), round(q.w, 5)]


registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(["/Game/Characters"], True)
skeleton = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")
ref_pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
space = unreal.AnimPoseSpaces.WORLD
data = {"fps": FPS, "bones": BONES, "ref": {b: pack(unreal.AnimPoseExtensions.get_bone_pose(ref_pose, b, space)) for b in BONES}, "anims": {}}
options = unreal.AnimPoseEvaluationOptions()
done = 0
for rel in ANIMS:
    path = ROOT + rel
    anim = unreal.load_asset(path)
    if anim is None:
        log("missing " + path)
        continue
    length = float(anim.get_editor_property("sequence_length"))
    count = max(2, int(length * FPS) + 1)
    frames = []
    for k in range(count):
        time = min(length, k / FPS)
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, time, options)
        frames.append([pack(unreal.AnimPoseExtensions.get_bone_pose(pose, b, space)) for b in BONES])
    notifies = []
    try:
        for event in anim.get_editor_property("notifies"):
            notifies.append({"name": str(event.get_editor_property("notify_name")), "time": float(event.get_editor_property("display_time"))})
    except Exception as exc:  # noqa - notify reading is a nice-to-have
        notifies.append({"error": repr(exc)})
    data["anims"][os.path.basename(rel)] = {"path": path, "length": round(length, 4), "frames": frames, "notifies": notifies}
    done += 1
    log("sampled %s: %.2f s, %d frames, %d notifies" % (os.path.basename(rel), length, count, len([n for n in notifies if "name" in n])))
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w") as handle:
    json.dump(data, handle, separators=(",", ":"))
log("wrote %s (%d animations, %.1f MB)" % (OUT, done, os.path.getsize(OUT) / 1048576.0))
