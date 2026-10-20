"""0.25 phase D, step 2 (editor commandlet only, no gameplay): sample the Paragon clips imported by Tools/paragon_clip_import.py into the throwaway spike project, at 30 fps for the
bones the rig cares about, and write them to JSON for Tools/paragon_clip_bake.py. Same sampler as Tools/spike_manny_export.py (the format is identical), different clip list.
Run: Saved/Scratch/run_spike.ps1 -Script Tools/paragon_clip_export.py -LogName paragon_export
"""

import json
import os
import unreal

OUT = os.environ.get("MANNY_SPIKE_OUT", r"C:\Games\AshenSpiral\AshenSpiral_Claude_Handoff_2026-10-15\current_work\AshenSpiral_UE5_0.23.0\Saved\Scratch\manny\paragon_samples.json")
ROOT = "/Game/Paragon/"
FPS = 30.0
BONES = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01", "head",
         "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",
         "thigh_l", "calf_l", "foot_l", "ball_l", "thigh_r", "calf_r", "foot_r", "ball_r", "weapon_l", "weapon_r"]
IMPORT_REPORT = os.path.join(os.path.dirname(OUT), "paragon_import.json")
ANIMS = [entry["name"] for entry in json.load(open(IMPORT_REPORT))["imported"]]  # exactly the clips Tools/paragon_clip_import.py imported


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
