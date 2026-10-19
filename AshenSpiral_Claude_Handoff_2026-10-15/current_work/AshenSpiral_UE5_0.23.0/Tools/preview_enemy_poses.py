"""Render the enemy pose solver's output with the actual OBJ armour meshes; not an Unreal screenshot.
Usage:  python Tools/preview_enemy_poses.py poses.json [output_directory]
poses.json comes from Tools/export_enemy_poses.cpp, which uses the same ASEnemyPose.h the game compiles. Writes one contact sheet per ability (wind-up, strike and recovery
frames in a side view above a three-quarter view) plus a summary sheet. Requires numpy and Pillow outside Unreal only (see preview_armor.py); Setup never imports this module.
"""
from pathlib import Path
import json
import math
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import preview_armor as armor

ROOT = Path(__file__).resolve().parents[1]
SLOT_MESH = ['SM_Torso', 'SM_EnemyHelm', 'SM_Pelvis', 'SM_Arm', 'SM_Forearm', 'SM_Arm', 'SM_Forearm', 'SM_Thigh', 'SM_Greave', 'SM_Thigh', 'SM_Greave', 'SM_Boot', 'SM_Boot',
             'SM_Pauldron', 'SM_Pauldron', 'SM_Cape', 'SM_Visor', 'SM_Hand', 'SM_Hand', 'SM_EnemyCape', 'SM_ArmorCore']
DARK = (.27, .32, .39)
METAL = (.50, .55, .62)
CLOTH = (.24, .14, .19)
FIRE = (1.0, .09, .03)
SLOT_COLOR = [METAL, METAL, DARK, DARK, METAL, DARK, METAL, DARK, METAL, DARK, METAL, DARK, DARK, METAL, METAL, CLOTH, FIRE, DARK, DARK, CLOTH, FIRE]
GLOWING = {16, 20}
ABILITY_NAMES = {0: 'neutral', 1: 'dive', 2: 'molt', 3: 'slash', 4: 'shot', 5: 'pounce', 6: 'charge', 7: 'blast', 8: 'snipe', 9: 'summon', 10: 'boss attack', 11: 'shove', 12: 'back-hop', 13: 'parry', 14: 'riposte', 15: 'ground pound', 16: 'enrage', 17: 'rift blink', 18: 'blink strike', 19: 'meteor rain',
                 100: 'death front 1', 101: 'death front 2', 102: 'death front 3', 103: 'death back', 104: 'death left', 105: 'death right'}


def rotate(q, v):
    qv = np.array(q[:3])
    t = 2.0 * np.cross(qv, v)
    return v + q[3] * t + np.cross(qv, t)


def place(faces, part):
    lx, ly, lz, qx, qy, qz, qw, sx, sy, sz = part
    v = faces.reshape(-1, 3) * np.array([sx, sy, sz])
    v = rotate((qx, qy, qz, qw), v) + np.array([lx, ly, lz])
    return v.reshape(faces.shape)


def items_for(sample):
    items = []
    if 'blob' in sample:  # a single squashing body (the leaper and the broodcaller)
        name = 'SM_RiftLeaper' if sample['kind'] == 5 else 'SM_Broodcaller'
        color = (.55, .30, .85) if sample['kind'] == 5 else (.30, .75, .35)
        return [(place(armor.mesh(name), sample['blob']), color, False)]
    parts = sample['parts']
    glow = min(1.0, max(0.0, sample.get('glow', 0.0)))
    for slot, name in enumerate(SLOT_MESH):
        color = SLOT_COLOR[slot]
        if slot in GLOWING:
            color = tuple(min(1.0, c * (1.0 + glow)) for c in color)
        items.append((place(armor.mesh(name), parts[slot]), color, slot in GLOWING))
    items.append((place(armor.mesh('SM_Weapon%d' % sample['weapon']), parts[21]), METAL, False))
    if sample.get('shield'):
        items.append((place(armor.mesh('SM_EnemyShield'), parts[22]), METAL, False))
    return items


def frame(sample, size, angle, elevation=.06):
    im = Image.new('RGB', size, '#2a3441')
    scale = 1.7 * (0.9 / sample['size'] if sample['size'] > 1.0 else 1.0)
    side_view = abs(angle + math.pi / 2) < .01  # the figure faces right in the side view, so keep room in front of it
    armor.draw_model(im, items_for(sample), (size[0] * (.34 if side_view else .46), size[1] - 34), angle, scale, elevation)
    return im


def sheet(samples, ability, font, small):
    columns = len(samples)
    cell = (300, 480)
    im = Image.new('RGB', (cell[0] * columns, cell[1] * 2 + 46), '#18202b')
    d = ImageDraw.Draw(im)
    d.text((14, 8), 'ENEMY POSES / %s / kind %d' % (ABILITY_NAMES.get(ability, str(ability)).upper(), samples[0]['kind']), font=font, fill='#d4eaff')
    for i, s in enumerate(samples):
        for row, angle in enumerate((-math.pi / 2, -.72)):
            im.paste(frame(s, cell, angle), (i * cell[0], 46 + row * cell[1]))
        phase = 'death' if ability >= 100 else ('wind-up' if s['u'] < 1.0 else ('strike' if s['u'] < 2.0 else 'recovery'))
        d.text((i * cell[0] + 10, 52), 'u %.2f  %s' % (s['u'], phase), font=small, fill='#8ba8bc')
    return im


def summary(groups, font, small):
    """One overview sheet: a row per ability, the key frames (rest, wind-up peak, mid strike, impact, recovery, rest) in the three-quarter view."""
    wanted = (0.0, 0.85, 1.12, 1.45, 2.2, 3.0)
    cell = (230, 340)
    rows = [(key, samples) for key, samples in groups.items() if 0 < key[1] < 100]  # abilities only; the baked deaths have their own sheets
    im = Image.new('RGB', (cell[0] * len(wanted) + 190, 44 + cell[1] * len(rows)), '#18202b')
    d = ImageDraw.Draw(im)
    d.text((14, 8), 'ENEMY POSES 0.24 / the offline preview of the data the game poses from', font=font, fill='#d4eaff')
    for r, ((kind, ability), samples) in enumerate(rows):
        d.text((12, 44 + r * cell[1] + 14), '%s' % ABILITY_NAMES.get(ability, str(ability)).upper(), font=font, fill='#d4eaff')
        d.text((12, 44 + r * cell[1] + 44), 'kind %d' % kind, font=small, fill='#8ba8bc')
        for c, u in enumerate(wanted):
            nearest = min(samples, key=lambda s: abs(s['u'] - u))
            fr = frame(nearest, cell, -.72)
            im.paste(fr, (190 + c * cell[0], 44 + r * cell[1]))
            d.text((190 + c * cell[0] + 8, 44 + r * cell[1] + 6), 'u %.2f' % nearest['u'], font=small, fill='#8ba8bc')
    return im


def main():
    path = Path(sys.argv[1])
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else path.parent
    out.mkdir(parents=True, exist_ok=True)
    data = json.loads(path.read_text())
    font = ImageFont.truetype('C:/Windows/Fonts/consola.ttf', 22) if Path('C:/Windows/Fonts/consola.ttf').exists() else ImageFont.load_default()
    small = ImageFont.truetype('C:/Windows/Fonts/consola.ttf', 15) if Path('C:/Windows/Fonts/consola.ttf').exists() else ImageFont.load_default()
    groups = {}
    for s in data['samples']:
        groups.setdefault((s['kind'], s['ability']), []).append(s)
    written = []
    for (kind, ability), samples in groups.items():
        if ability == 0 and len(groups) > 1:
            continue
        name = 'poses_%s_kind%d.png' % (ABILITY_NAMES.get(ability, str(ability)).replace(' ', '_'), kind)
        sheet(samples, ability, font, small).save(out / name)
        written.append(name)
    summary(groups, font, small).save(out / 'ENEMY_POSES_0.24_PREVIEW.png')
    written.append('ENEMY_POSES_0.24_PREVIEW.png')
    print('Rendered', len(written), 'pose sheets into', out, ':', ', '.join(written))


if __name__ == '__main__':
    main()
