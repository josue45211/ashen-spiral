"""0.24 skeletal spike, step 4 (offline): retarget Mannequin animation onto Ashen Spiral's own pose channels (ASEnemyPose.h) and write it out as data.
Input: Saved/Scratch/manny/manny_samples.json from Tools/spike_manny_export.py. Output: a generated C++ header of pose curves (no Epic asset is shipped, only numbers).
Usage: python Tools/spike_manny_bake.py <samples.json> deaths <output header>
What it does for every frame: removes root motion, expresses the pose relative to the first frame (so every curve starts at the rig's neutral pose), rescales hand motion by the
arm-length ratio and body motion by the leg-length ratio (the rig's arms are 1.5x longer than the Mannequin's), and fits each channel with at most 11 keys.
"""
import math
import sys
from pathlib import Path
import numpy as np
from spike_manny_retarget import Manny, qbetween, qconj, qmul, qto_rotator

ARM_RATIO = 84.0 / 55.1   # the rig's reach over the Mannequin's, so hand motion keeps its meaning with longer arms
LEG_RATIO = 76.0 / 85.6   # and for the legs: body drops and leans scale with them
CHANNELS = ["Crouch", "LeanX", "LeanY", "Bend", "BendRoll", "TorsoYaw", "HeadPitch", "HeadYaw", "HandLX", "HandLY", "HandLZ", "HandRX", "HandRY", "HandRZ",
            "WeaponPitch", "WeaponYaw", "WeaponRoll", "Plant", "FootLX", "FootRX", "Stance"]
LIMITS = {"Crouch": 80, "LeanX": 80, "LeanY": 80, "Bend": 120, "BendRoll": 120, "TorsoYaw": 120, "HeadPitch": 120, "HeadYaw": 120, "WeaponPitch": 120, "WeaponYaw": 120,
          "WeaponRoll": 120, "FootLX": 40, "FootRX": 40, "Stance": 30, "Plant": 1}
TOLERANCE = {"Plant": .02, "Crouch": 2.0, "LeanX": 2.5, "LeanY": 2.5, "FootLX": 2.0, "FootRX": 2.0, "Stance": 1.5}
DEATHS = [("MM_Death_Front_01", "Front 1"), ("MM_Death_Front_02", "Front 2"), ("MM_Death_Front_03", "Front 3"), ("MM_Death_Back_01", "Back 1"),
          ("MM_Death_Left_01", "Left 1"), ("MM_Death_Right_01", "Right 1")]
DEATH_SECONDS = 1.0   # the first second of each clip holds the collapse; the game plays it over EnemyDeathSeconds


def unit(v):
    return v / max(1e-9, float(np.linalg.norm(v)))


def spine_swing(v0, v):
    """The bend of the upper body as Unreal pitch and roll (degrees): the swing rotation taking the first frame's spine direction to this frame's. Unlike a lean angle taken
    from the vector's components it stays well defined when the body bends past horizontal. Pitch is negative for a forward lean, roll positive for a lean to the right."""
    pitch, _yaw, roll = qto_rotator(qbetween(unit(v0), unit(v)))
    return pitch, roll


def channel_series(manny, anim, t_end):
    """Per-frame channel values (root motion removed, relative to the first frame) for the first t_end seconds of an animation."""
    data = manny.anims[anim]
    count = min(len(data["frames"]), int(round(t_end * 30)) + 1)
    first = manny.pose(anim, 0)
    series = {c: [] for c in CHANNELS}
    times = []

    def at(pose, root_shift, bone):
        p = pose[bone][0].copy()
        p[:2] -= root_shift
        return p

    zero = np.zeros(2)
    base = {b: at(first, zero, b) for b in manny.bones}
    spine0 = base["spine_05"] - base["pelvis"]
    for k in range(count):
        pose = manny.pose(anim, k)
        shift = pose["root"][0][:2] - first["root"][0][:2]
        p = {b: at(pose, shift, b) for b in manny.bones}
        times.append(k / 30.0)
        hip = p["pelvis"] - base["pelvis"]
        series["Crouch"].append(-hip[2] * LEG_RATIO)
        series["LeanX"].append(2.0 * hip[0] * LEG_RATIO)
        series["LeanY"].append(2.0 * hip[1] * LEG_RATIO)
        bend, roll = spine_swing(spine0, p["spine_05"] - p["pelvis"])
        series["Bend"].append(bend)
        series["BendRoll"].append(roll)
        chest = qmul(pose["spine_03"][1], qconj(first["spine_03"][1]))
        series["TorsoYaw"].append(qto_rotator(chest)[1])
        head = qto_rotator(qmul(pose["head"][1], qconj(first["head"][1])))
        series["HeadPitch"].append(head[0] - bend)
        series["HeadYaw"].append(head[1])
        for side, tag in (("l", "L"), ("r", "R")):
            v = (p["hand_" + side] - p["upperarm_" + side]) - (base["hand_" + side] - base["upperarm_" + side])
            for axis, name in enumerate("XYZ"):
                series["Hand" + tag + name].append(float(v[axis]) * ARM_RATIO)
        weapon = qto_rotator(qmul(pose["hand_r"][1], qconj(first["hand_r"][1])))
        series["WeaponPitch"].append(weapon[0])
        series["WeaponYaw"].append(weapon[1])
        series["WeaponRoll"].append(weapon[2])
        series["Plant"].append(min(1.0, k / 3.0))
        series["FootLX"].append((p["foot_l"][0] - base["foot_l"][0]) * LEG_RATIO)
        series["FootRX"].append((p["foot_r"][0] - base["foot_r"][0]) * LEG_RATIO)
        series["Stance"].append(((p["foot_r"][1] - p["foot_l"][1]) - (base["foot_r"][1] - base["foot_l"][1])) * .5 * LEG_RATIO)
    return np.array(times), {c: np.array(v, float) for c, v in series.items()}


def rdp(t, v, eps):
    """Ramer-Douglas-Peucker on (t, v): the indices of the points to keep."""
    keep = {0, len(t) - 1}
    stack = [(0, len(t) - 1)]
    while stack:
        a, b = stack.pop()
        if b <= a + 1:
            continue
        slope = (v[b] - v[a]) / (t[b] - t[a]) if t[b] > t[a] else 0.0
        errors = np.abs(v[a + 1:b] - (v[a] + slope * (t[a + 1:b] - t[a])))
        i = int(np.argmax(errors))
        if errors[i] > eps:
            index = a + 1 + i
            keep.add(index)
            stack.append((a, index))
            stack.append((index, b))
    return sorted(keep)


def fit_curve(t, v, channel, max_keys=11):
    eps = TOLERANCE.get(channel, 2.0)
    while True:
        idx = rdp(t, v, eps)
        if len(idx) <= max_keys:
            break
        eps *= 1.25
    limit = LIMITS.get(channel, 150)
    return [(float(t[i]), float(np.clip(v[i], -limit, limit))) for i in idx]


def cpp_curve(channel, keys):
    body = ",".join("{%.3ff,%.2ff}" % (round(k, 3), round(val, 2)) for k, val in keys)
    return "            {Ch%s,%s{%s,{-1.f,0.f}}}," % (channel, " " * max(1, 13 - len(channel)), body)


HEADER = """// GENERATED by Tools/spike_manny_bake.py from the Mannequin template's own death animations (sampled offline in the editor commandlet by Tools/spike_manny_export.py).
// Do not edit by hand. These are numeric pose curves over the rig's channels (ASEnemyPose.h), not Epic's assets: the Manny mesh, skeleton and animation files are never
// shipped; the motion is retargeted onto Ashen Spiral's own armour rig. Source content: Epic's template Mannequin animations, licensed for use within Unreal Engine based
// products (see Docs/SKELETAL_SPIKE_0.24.md). Track time U runs 0..1 over the death and every curve starts at 0 (the walking pose).
#pragma once
#include "ASEnemyPose.h"
namespace ASEnemyPose {
constexpr int DeathVariants=6;     // 0..2 front (doubled over), 3 back, 4 left, 5 right, as named by the Mannequin clips
constexpr float DeathTrackEnd=1.f;
inline const Track& DeathTrack(int Variant) {
    static constexpr Track Table[DeathVariants]={
%s
    };
    return Table[std::clamp(Variant,0,DeathVariants-1)];
}
}
"""


def bake_deaths(manny, header):
    lines = []
    for anim, label in DEATHS:
        t, series = channel_series(manny, anim, DEATH_SECONDS)
        u = t / t[-1]
        lines.append("        // %s (Mannequin %s, its first %.2f s): root motion removed, motion relative to the first frame, arm and leg lengths rescaled" % (label, anim, t[-1]))
        lines.append("        {{")
        kept = 0
        for c in CHANNELS:
            v = series[c]
            if float(np.max(np.abs(v))) < (.05 if c == "Plant" else .6):
                continue
            keys = fit_curve(u, v, c)
            keys[0] = (0.0, 0.0)
            lines.append(cpp_curve(c, keys))
            kept += 1
        lines.append("            {-1,{}}")
        lines.append("        }},")
        print("%-18s %2d curves; at the end: crouch %4.0f lean %4.0f/%4.0f bend %4.0f roll %4.0f handR %4.0f,%4.0f,%4.0f" % (
            anim, kept, series["Crouch"][-1], series["LeanX"][-1], series["LeanY"][-1], series["Bend"][-1], series["BendRoll"][-1],
            series["HandRX"][-1], series["HandRY"][-1], series["HandRZ"][-1]))
    Path(header).write_bytes((HEADER % "\n".join(lines)).encode("utf-8"))
    print("wrote", header)


def main():
    if len(sys.argv) < 4 or sys.argv[2] != "deaths":
        print(__doc__)
        return
    bake_deaths(Manny(sys.argv[1]), sys.argv[3])


if __name__ == "__main__":
    main()
