"""0.24 skeletal spike, step 1 (editor commandlet only, no gameplay): what does the Python API expose for the Mannequin, and what does the content look like once loaded?
Run with Saved/Scratch/run_spike.ps1 against the throwaway project that holds a copy of the engine's own Mannequin template content (Saved/Scratch/make_spike_project.ps1).
Everything is printed to the log with the prefix SPIKE; nothing is written into the real project.
"""
import json
import os
import unreal


def log(message):
    unreal.log("SPIKE " + str(message))


def has(name):
    return hasattr(unreal, name)


for name in ("AnimPoseExtensions", "AnimPose", "AnimPoseEvaluationOptions", "AnimPoseSpaces", "AnimDataEvalType", "AnimationLibrary", "SkeletalMeshEditorSubsystem", "AnimSequence"):
    log("unreal.%s available: %s" % (name, has(name)))

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(["/Game/Characters"], True)
assets = registry.get_assets_by_path("/Game/Characters", recursive=True)
log("assets found under /Game/Characters: %d" % len(assets))
kinds = {}
for a in assets:
    key = str(a.asset_class_path.asset_name)
    kinds[key] = kinds.get(key, 0) + 1
log("asset classes: %s" % json.dumps(kinds, sort_keys=True))

skeleton = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")
mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
log("skeleton: %s, mesh: %s" % (skeleton, mesh))
if skeleton is None:
    raise SystemExit("could not load SK_Mannequin")

# Reference pose through the pose API (out parameters come back as return values in Python).
pose = None
try:
    pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
    log("get_reference_pose returned: %s" % type(pose))
except Exception as exc:  # noqa
    log("get_reference_pose(skeleton) raised: %r" % exc)
    pose = unreal.AnimPose()
    try:
        unreal.AnimPoseExtensions.get_reference_pose(skeleton, pose)
        log("get_reference_pose(skeleton, pose) worked")
    except Exception as exc2:  # noqa
        log("get_reference_pose(skeleton, pose) raised: %r" % exc2)
        pose = None
if pose is not None:
    names = unreal.AnimPoseExtensions.get_bone_names(pose)
    log("bones: %d" % len(names))
    log("bone names: %s" % ", ".join(str(n) for n in names))
    for bone in ("pelvis", "spine_03", "head", "upperarm_l", "lowerarm_l", "hand_l", "hand_r", "thigh_l", "calf_l", "foot_l", "ball_l"):
        try:
            t = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
            log("ref %s world: loc=(%.1f, %.1f, %.1f) quat=(%.3f, %.3f, %.3f, %.3f)" % (bone, t.translation.x, t.translation.y, t.translation.z,
                                                                                         t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w))
        except Exception as exc:  # noqa
            log("ref %s failed: %r" % (bone, exc))

for path in ("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01", "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"):
    anim = unreal.load_asset(path)
    log("%s -> %s" % (path, anim))
    if anim is None:
        continue
    try:
        log("  length=%.3f  frames=%s  rate=%s" % (anim.get_editor_property("sequence_length"), unreal.AnimationLibrary.get_num_frames(anim), unreal.AnimationLibrary.get_sampling_frame_rate(anim)))
    except Exception as exc:  # noqa
        log("  length query failed: %r" % exc)
    try:
        options = unreal.AnimPoseEvaluationOptions()
        sampled = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, 0.2, options)
        log("  sampled pose at 0.2: %s" % type(sampled))
        t = unreal.AnimPoseExtensions.get_bone_pose(sampled, "hand_r", unreal.AnimPoseSpaces.WORLD)
        log("  hand_r world at 0.2: (%.1f, %.1f, %.1f)" % (t.translation.x, t.translation.y, t.translation.z))
    except Exception as exc:  # noqa
        log("  pose sampling raised: %r" % exc)
log("probe finished")
