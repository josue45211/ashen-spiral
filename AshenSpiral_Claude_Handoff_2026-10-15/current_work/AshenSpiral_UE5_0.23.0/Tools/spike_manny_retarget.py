"""0.24 skeletal spike, step 3 (offline, numpy + Pillow like preview_armor.py): evaluate what the Mannequin's own animation can do for Ashen Spiral's armour rig.
Input: Saved/Scratch/manny/manny_samples.json from Tools/spike_manny_export.py (bone poses sampled in the editor commandlet).
Modes:
  rigid <animation> [frames...]   render the real armour parts rigidly driven by Manny's bones (limbs fitted joint to joint); a contact sheet per animation
  measure                         print Manny's proportions against the rig's (arm and leg lengths, heights)
Usage: python Tools/spike_manny_retarget.py <samples.json> <out_dir> measure|rigid <animation>...
"""
import json
import math
import sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import preview_armor as armor
import preview_enemy_poses as pep

# ---- Quaternions in Unreal's convention: (x, y, z, w), A*B applies B first ----


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx, aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def qconj(q):
    return (-q[0], -q[1], -q[2], q[3])


def qrot(q, v):
    qv = np.array(q[:3], float)
    t = 2.0 * np.cross(qv, v)
    return np.asarray(v, float) + q[3] * t + np.cross(qv, t)


def qfrom_rotator(pitch, yaw, roll):
    p, y, r = (math.radians(a) * .5 for a in (pitch, yaw, roll))
    sp, cp, sy, cy, sr, cr = math.sin(p), math.cos(p), math.sin(y), math.cos(y), math.sin(r), math.cos(r)
    return (cr * sp * sy - sr * cp * cy, -cr * sp * cy - sr * cp * sy, cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy)


def qto_rotator(q):
    """FQuat::Rotator() (UnrealMath.cpp): pitch, yaw, roll in degrees."""
    x, y, z, w = q
    singularity = z * x - w * y
    yaw_y, yaw_x = 2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z)
    if singularity < -0.4999995:
        pitch, yaw = -90.0, math.degrees(math.atan2(yaw_y, yaw_x))
        roll = (-yaw - 2.0 * math.degrees(math.atan2(x, w)) + 180) % 360 - 180
    elif singularity > 0.4999995:
        pitch, yaw = 90.0, math.degrees(math.atan2(yaw_y, yaw_x))
        roll = (yaw - 2.0 * math.degrees(math.atan2(x, w)) + 180) % 360 - 180
    else:
        pitch = math.degrees(math.asin(2.0 * singularity))
        yaw = math.degrees(math.atan2(yaw_y, yaw_x))
        roll = math.degrees(math.atan2(-2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y)))
    return pitch, yaw, roll


def qbetween(a, b):
    a = np.asarray(a, float)
    b = np.asarray(b, float)
    w = 1.0 + float(np.dot(a, b))
    if w >= 1e-6:
        c = np.cross(a, b)
        n = math.sqrt(c[0] ** 2 + c[1] ** 2 + c[2] ** 2 + w * w)
        return (c[0] / n, c[1] / n, c[2] / n, w / n)
    basis = np.array([0, 1, 0]) if abs(a[0]) > abs(a[1]) and abs(a[0]) > abs(a[2]) else np.array([-1, 0, 0])
    c = np.cross(a, basis)
    n = math.sqrt(float(np.dot(c, c)))
    return (c[0] / n, c[1] / n, c[2] / n, 0.0)


# The Mannequin faces +Y with its left on +X; the rig faces +X with its right on +Y. That is a yaw of -90 degrees about Z.
Q_FIG = qfrom_rotator(0, -90, 0)


def to_fig(transform, scale=1.0):
    """[x, y, z, qx, qy, qz, qw] in the Mannequin's component space -> (position, quaternion) in the rig's figure frame."""
    p = qrot(Q_FIG, np.array(transform[:3], float)) * scale
    return p, qmul(Q_FIG, tuple(transform[3:7]))


class Manny:
    def __init__(self, path):
        data = json.loads(Path(path).read_text())
        self.bones = data["bones"]
        self.index = {b: i for i, b in enumerate(self.bones)}
        self.ref = {b: to_fig(t) for b, t in data["ref"].items()}
        self.anims = data["anims"]

    def pose(self, anim, frame, scale=1.0):
        frames = self.anims[anim]["frames"]
        return {b: to_fig(frames[min(frame, len(frames) - 1)][i], scale) for b, i in self.index.items()}

    def ref_pose(self, scale=1.0):
        return {b: (p * scale, q) for b, (p, q) in self.ref.items()}


# ---- measure ----


def measure(manny):
    ref = manny.ref_pose()
    d = lambda a, b: float(np.linalg.norm(ref[a][0] - ref[b][0]))
    print("Mannequin reference pose (figure frame, cm):")
    print("  shoulder height  %.1f   hip height %.1f   head height %.1f" % (ref["upperarm_l"][0][2], ref["thigh_l"][0][2], ref["head"][0][2]))
    print("  shoulder half-width %.1f   hand half-width %.1f" % (abs(ref["upperarm_l"][0][1]), abs(ref["hand_l"][0][1])))
    print("  upper arm %.1f + forearm %.1f = %.1f   (the rig: 42 + 42 = 84)" % (d("upperarm_l", "lowerarm_l"), d("lowerarm_l", "hand_l"), d("upperarm_l", "lowerarm_l") + d("lowerarm_l", "hand_l")))
    print("  thigh %.1f + calf %.1f = %.1f   (the rig: 38 + 38 = 76)" % (d("thigh_l", "calf_l"), d("calf_l", "foot_l"), d("thigh_l", "calf_l") + d("calf_l", "foot_l")))
    print("  arm ratio (rig / Mannequin) %.2f   leg ratio %.2f" % (84.0 / (d("upperarm_l", "lowerarm_l") + d("lowerarm_l", "hand_l")), 76.0 / (d("thigh_l", "calf_l") + d("calf_l", "foot_l"))))


# ---- rigid skin ----
SIZE = 1.0


def tf(loc, rot, scale=(1, 1, 1)):
    return list(map(float, loc)) + list(map(float, rot)) + list(map(float, scale))


def segment(from_p, to_p, mesh_length):
    v = np.asarray(from_p, float) - np.asarray(to_p, float)
    length = float(np.linalg.norm(v))
    rot = qbetween(np.array([0, 0, 1.0]), v / length if length > 1e-6 else np.array([0, 0, 1.0]))
    return tf((np.asarray(from_p) + np.asarray(to_p)) * .5, rot, (1, 1, length / mesh_length))


def rigid_sample(manny, anim, frame, scale):
    """Our 21 parts driven by the Mannequin's bones: limbs fitted joint to joint, everything else rigidly attached to its bone."""
    cur = manny.pose(anim, frame, scale)
    ref = manny.ref_pose(scale)

    def delta(bone):
        return qmul(cur[bone][1], qconj(ref[bone][1]))

    def rigid(bone, offset=(0, 0, 0), local_rot=(0, 0, 0, 1)):
        d = delta(bone)
        return tf(cur[bone][0] + qrot(d, np.array(offset, float)), qmul(d, local_rot))

    parts = [None] * 23
    parts[pep_slot("Torso")] = rigid("spine_03")
    parts[pep_slot("Head")] = tf(cur["head"][0], delta("head"), (.95, 1, .85))
    parts[pep_slot("Pelvis")] = rigid("pelvis")
    parts[3] = segment(cur["upperarm_l"][0], cur["lowerarm_l"][0], 42)
    parts[4] = segment(cur["lowerarm_l"][0], cur["hand_l"][0], 42)
    parts[5] = segment(cur["upperarm_r"][0], cur["lowerarm_r"][0], 42)
    parts[6] = segment(cur["lowerarm_r"][0], cur["hand_r"][0], 42)
    parts[7] = segment(cur["thigh_l"][0], cur["calf_l"][0], 38)
    parts[8] = segment(cur["calf_l"][0], cur["foot_l"][0], 38)
    parts[9] = segment(cur["thigh_r"][0], cur["calf_r"][0], 38)
    parts[10] = segment(cur["calf_r"][0], cur["foot_r"][0], 38)
    parts[11] = rigid("foot_l", (3, 0, -2))
    parts[12] = rigid("foot_r", (3, 0, -2))
    parts[13] = rigid("clavicle_l", (0, -4, 6))
    parts[14] = rigid("clavicle_r", (0, 4, 6))
    parts[15] = rigid("spine_03", (-22, 0, 30))
    parts[16] = tf(cur["head"][0], delta("head"), (.95, 1, .85))
    parts[17] = rigid("hand_l")
    parts[18] = rigid("hand_r")
    parts[19] = rigid("spine_03", (-25, 0, -25))
    parts[20] = rigid("spine_03")
    parts[21] = rigid("hand_r")
    parts[22] = rigid("hand_l", (14, 0, 15))
    # the figure's own transform is left at identity here: the Mannequin's bones already carry the proportions
    return {"kind": 0, "ability": 0, "u": float(frame), "size": 1.0, "weapon": 0, "shield": False, "glow": 0.0, "parts": parts}


def pep_slot(name):
    return {"Torso": 0, "Head": 1, "Pelvis": 2}[name]


def rigid_sheet(manny, anim, frames, out):
    scale = 133.0 / 143.6  # bring the Mannequin's shoulder height to the rig's
    cell = (300, 480)
    im = Image.new("RGB", (cell[0] * len(frames), cell[1] + 46), "#18202b")
    d = ImageDraw.Draw(im)
    font = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", 20) if Path("C:/Windows/Fonts/consola.ttf").exists() else ImageFont.load_default()
    d.text((12, 8), "RIGID SKIN / %s / %.2f s / armour parts driven by the Mannequin's bones" % (anim, manny.anims[anim]["length"]), font=font, fill="#d4eaff")
    for i, f in enumerate(frames):
        sample = rigid_sample(manny, anim, f, scale)
        sample["size"] = 1.0
        angle = -.72
        frm = Image.new("RGB", cell, "#2a3441")
        armor.draw_model(frm, pep.items_for(sample), (cell[0] * .46, cell[1] - 34), angle, 1.7, .06)
        im.paste(frm, (i * cell[0], 46))
        d.text((i * cell[0] + 8, 50), "frame %d  (%.2f s)" % (f, f / 30.0), font=font, fill="#8ba8bc")
    im.save(out)


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return
    manny = Manny(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    mode = sys.argv[3]
    if mode == "measure":
        measure(manny)
    elif mode == "rigid":
        for anim in sys.argv[4:]:
            n = len(manny.anims[anim]["frames"])
            frames = sorted(set(int(round(k * (n - 1) / 7.0)) for k in range(8)))
            rigid_sheet(manny, anim, frames, out / ("rigid_%s.png" % anim))
            print("wrote", out / ("rigid_%s.png" % anim))


if __name__ == "__main__":
    main()
