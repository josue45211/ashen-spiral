"""0.25 phase D, step 3 (offline, outside Unreal): retarget sampled Paragon-on-Manny attack clips onto Ashen Spiral's own pose channels (ASEnemyPose.h) and write them as a header.
Input: Saved/Scratch/manny/paragon_samples.json from Tools/paragon_clip_export.py. Output: a generated C++ header of ability pose tracks (numbers only: no mesh, skeleton or animation file is shipped)
and, optionally, a Markdown table of what was chosen and why.
Usage: python Tools/paragon_clip_bake.py <samples.json> <output header> [<output doc.md>]
Needs numpy (like Tools/spike_manny_bake.py, which supplies the channel fitting; run it from a throwaway virtual environment, never globally).

Every ability's candidate clips are listed in CANDIDATES below. For each ability the baker bakes all of them, drops the ones that fail the gate (too little wind-up: the blow must start at least
MIN_LEAD seconds in, or the telegraph would be a snap; or too much of the motion clamped to the rig's limits) and keeps the first that passes (the lists are ordered by how well the clip suits the ability). An ability with no passing clip gets
none and plays its hand-made track.

What it does per clip: samples the clip's channels exactly like the death bake (root motion removed, relative to the first frame, hand motion rescaled by the arm ratio, body motion by the
leg ratio), finds the blow from the weapon hand's speed (the fastest stretch of the clip), and bends time so the approach plays over track time U 0..1 (the wind-up), the blow over U 1..2
(the strike) and the follow-through over U 2..2.8, easing to the neutral pose at U 3 (the recovery), which is how the game's tracks are laid out (ASEnemyPose.h). Every curve starts and ends at 0.
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import spike_manny_bake as B  # noqa: E402  (channel_series, fit_curve, cpp_curve: the same retargeting as the deaths)
from spike_manny_retarget import Manny  # noqa: E402

# The attack tracks' per-channel limits (Tests/enemy_pose_024_tests.cpp "every key stays inside its channel's sane range"); the baker clamps to them.
LIMIT = {"Plant": 1.0, "Bend": 120.0, "BendRoll": 120.0, "Crouch": 30.0, "LeanX": 40.0, "LeanY": 40.0, "FootLX": 40.0, "FootRX": 40.0, "Stance": 30.0}
HAND_LIMIT = 140.0
OTHER_LIMIT = 120.0
STRIKE_FRACTION = 0.4        # the blow is the stretch around the fastest hand motion where the hand moves at more than this share of its top speed
FOLLOW_END = 2.8             # the clip's last frame lands here; the last .2 of the track eases back to neutral
GLOW = [(0.0, 0.0), (1.0, 1.6), (1.3, 2.6), (2.2, 1.0), (3.0, 0.0)]  # the visor and core flare at the blow, as the hand-made tracks do
COUNTER_LEAD = 0.15          # riposte and blink strike are chained counter-strikes (the AI starts them the moment the ability before them ends), so a quick wind-up is the point
MIN_LEAD = 0.25              # seconds before the blow starts: shorter and the wind-up is a snap, not a telegraph (the player combo clips are 0.07-0.2 s)
MAX_BREACH = 0.30            # share of curve samples that had to be clamped to the channel limits; more than this and the retargeted body shape is no longer the clip's

# ability id, label, candidate clips (a "+" joins clips played back to back). Abilities 20-33 share one of these (ASEnemyPose.h TrackIndex); 1 dive and 2 molt pose the fliers' own wings.
GROUND_SMASH = "steel_Steel_Ability_GroundSmah_Start+steel_Steel_Ability_GroundSmash_Loop+steel_Steel_Ability_GroundSmash_End"
CANDIDATES = [
    (3, "slash", ["minions_Attack_A", "minions_Attack_B", "minions_Attack_C", "minions_Attack_D", "steel_Steel_Attack_Melee_A_Slow", "steel_Steel_Attack_Melee_C_Slow", "Narbash_Primary_Swing1_Slow",
                  "Narbash_Primary_Swing2_Slow", "Greystone_Attack_A_Slow", "Greystone_Attack_B_Med", "Countess_Primary_Attack_A_Slow", "Phase_Primary_Attack_A_Slow"]),
    (4, "shot", ["Wraith_Ability_RMB_Start+Wraith_Ability_Q_Fire", "Wraith_Ability_Q_Fire", "Countess_Ability_E"]),
    (5, "pounce", ["Phase_Ability_E", "Phase_Ability_E_Land", "steel_Steel_Ability_CowCatcher_Start"]),
    (6, "charge", ["steel_Steel_Attack_Melee_Charge_A", "steel_Steel_Ability_CowCatcher_Start", "Narbash_Q_Ability_Intro"]),
    (7, "blast", ["Countess_Ability_Q", "Narbash_RMB_Ability_Throw", "steel_Steel_Ability_Shield_Throw", "Countess_Ability_E"]),
    (8, "snipe", ["Wraith_Ability_Q_Targeting_Start+Wraith_Ability_Q_Fire", "Wraith_Ability_Q_Targeting_Start", "Wraith_Ability_RMB_Start"]),
    (9, "summon", ["Narbash_E_ability_intro", "Serath_R_Ability_Intro", "Serath_E_Ability_Wing_Flap"]),
    (10, "bossattack", ["steel_Steel_Ability_Ultimate_Intro", "Countess_Ability_Ultimate", "Narbash_R_Ability_Intro", "Serath_Q_Ability_Attack_A", "steel_Steel_Attack_GuardBreak_A"]),
    (11, "shove", ["steel_Steel_Ability_Bash", "steel_Steel_Attack_Melee_D_V1"]),
    (12, "backhop", ["Phase_Ability_Q", "Phase_Ability_R_Alt"]),
    (13, "parry", ["steel_Steel_Ability_ShieldBlock_Deploy"]),  # a stance with no blow of its own: the blow finder has nothing to find, so it stays hand-made unless a clip with a lead appears
    (14, "riposte*", ["steel_Steel_Attack_Melee_B_Slow", "steel_Steel_Attack_Melee_B", "Serath_Primary_Attack_B_Slow", "steel_Steel_Attack_Melee_A"]),
    (15, "pound", [GROUND_SMASH, "steel_Steel_Attack_GuardBreak_A"]),
    (16, "enrage", ["minions_Aggro_Transition_A", "Serath_R_Ability_Intro"]),
    (17, "blink", ["Phase_Ability_R_Alt", "Phase_Ability_Q"]),
    (18, "blinkstrike*", ["Phase_Primary_Attack_B_Medium", "steel_Steel_Attack_Melee_C", "steel_Steel_Attack_Melee_C_Normal_V1"]),
    (19, "meteors", ["Countess_Ability_E", "Serath_E_Ability_Wing_Flap", "Narbash_E_ability_intro"]),
]


ANGLES = {"Bend", "BendRoll", "TorsoPitch", "TorsoYaw", "TorsoRoll", "HeadPitch", "HeadYaw", "HeadRoll", "WeaponPitch", "WeaponYaw", "WeaponRoll"}


def limit_for(channel):
    if channel in LIMIT:
        return LIMIT[channel]
    return HAND_LIMIT if channel.startswith("Hand") else OTHER_LIMIT


def relax_keys(keys, limit):
    """Spread out keys that sit too close for the change between them: the tests (and the eye) want no step bigger than 30% of the channel's range per .005 of track time, and the curves ease
    with a smoothstep whose steepest point is 1.5 times the straight line. Later keys move later; the closing key at 3.0 never moves."""
    out = [keys[0]]
    for t, v in keys[1:]:
        need = 0.026 * abs(v - out[-1][1]) / limit
        out.append((max(t, out[-1][0] + need), v))
    if out[-1][0] > 3.0 + 1e-9:  # no room left: squeeze the middle back into the track
        scale = (3.0 - out[0][0]) / (out[-1][0] - out[0][0])
        out = [(out[0][0] + (t - out[0][0]) * scale, v) for t, v in out]
    return out


def combined_series(manny, clips):
    """Channel series over one clip or several played back to back (a synthetic animation of their frames)."""
    if len(clips) == 1:
        name = clips[0]
    else:
        name = "combo:" + "+".join(clips)
        frames = []
        for clip in clips:
            frames += manny.anims[clip]["frames"]
        manny.anims[name] = {"frames": frames, "length": len(frames) / 30.0}
    t_end = (len(manny.anims[name]["frames"]) - 1) / 30.0
    return name, B.channel_series(manny, name, t_end)


def find_blow(t, series):
    """(start, peak, end, top speed) of the blow: from the weapon hand's speed."""
    hand = np.stack([series["HandRX"], series["HandRY"], series["HandRZ"]], axis=1)
    speed = np.linalg.norm(np.diff(hand, axis=0), axis=1) / np.maximum(1e-6, np.diff(t))
    speed = np.convolve(speed, np.ones(3) / 3.0, mode="same")
    peak = int(np.argmax(speed))
    threshold = STRIKE_FRACTION * float(speed[peak])
    start = peak
    while start > 0 and speed[start - 1] >= threshold:
        start -= 1
    end = peak
    while end < len(speed) - 1 and speed[end + 1] >= threshold:
        end += 1
    start, end = max(1, start), min(len(t) - 2, end + 1)
    if end - start < 3:  # a very short blow still gets a visible strike window
        start, end = max(1, peak - 2), min(len(t) - 2, peak + 2)
    return float(t[start]), float(t[peak]), float(t[end]), float(speed[peak])


def bake(manny, ability, label, clips):
    """(lines, stats) for one candidate clip (or clips played back to back)."""
    name, (t, series) = combined_series(manny, clips)
    t_start, t_peak, t_end, top = find_blow(t, series)
    last = float(t[-1])
    u = np.interp(t, [0.0, t_start, t_end, last], [0.0, 1.0, 2.0, FOLLOW_END])
    lines = ["        // %d %s: %s; the blow is %.2f-%.2f s of %.2f s (the fastest hand reaches %.0f cm/s)" % (ability, label, "+".join(clips), t_start, t_end, last, top), "        {{"]
    kept = 0
    clamped = total = 0
    for channel in B.CHANNELS:
        if channel == "Plant":
            keys = [(0.0, 0.0), (0.3, 1.0), (2.6, 1.0), (3.0, 0.0)]  # planted for the whole attack, as the hand-made tracks are
        else:
            raw = series[channel]
            if channel in ANGLES:  # an angle that crosses +-180 must not jump by 360 (clamped, that is a +120 to -120 flip in one frame)
                raw = np.degrees(np.unwrap(np.radians(raw)))
            v = np.clip(raw, -limit_for(channel), limit_for(channel))
            clamped += int(np.sum(np.abs(raw) > limit_for(channel) + 1e-6))
            total += len(raw)
            if float(np.max(np.abs(v))) < 0.6:
                continue
            keys = B.fit_curve(u, v, channel, max_keys=10)
            keys[0] = (0.0, 0.0)
            keys.append((3.0, 0.0))
            keys = [(k, float(np.clip(val, -limit_for(channel), limit_for(channel)))) for k, val in keys]
            keys = relax_keys(keys, limit_for(channel))
        lines.append(B.cpp_curve(channel, keys))
        kept += 1
    glow = ",".join("{%.3ff,%.2ff}" % (k, val) for k, val in GLOW)
    lines.append("            {ChGlow,         {%s,{-1.f,0.f}}}," % glow)
    lines.append("            {-1,{}}")
    lines.append("        }},")
    stats = {"clips": "+".join(clips), "lead": t_start, "blow_end": t_end, "length": last, "speed": top, "curves": kept + 1, "breach": clamped / max(1, total)}
    return lines, stats


def choose(manny, ability, label, candidates):
    """Bake the candidates in order; return ((lines, stats) of the first that passes the gate, or None, [(clip, reason it was passed over)]). A "*" on the label means a counter-strike (COUNTER_LEAD)."""
    best, rejected = None, []
    need = COUNTER_LEAD if label.endswith("*") else MIN_LEAD
    for clips in candidates:
        parts = clips.split("+")
        if any(c not in manny.anims for c in parts):
            rejected.append((clips, "not in the samples"))
            continue
        lines, stats = bake(manny, ability, label.rstrip("*"), parts)
        if best is not None:
            rejected.append((clips, "a clip earlier in the list already passed"))
        elif stats["lead"] < need:
            rejected.append((clips, "wind-up %.2f s (needs %.2f)" % (stats["lead"], need)))
        elif stats["breach"] > MAX_BREACH:
            rejected.append((clips, "%.0f%% of samples beyond the rig's limits" % (100 * stats["breach"])))
        else:
            best = (lines, stats)
    return best, rejected


HEADER = """// GENERATED by Tools/paragon_clip_bake.py from Paragon animations retargeted to the Mannequin (sampled offline in the editor commandlet by Tools/paragon_clip_export.py).
// Do not edit by hand. These are numeric pose curves over the rig's channels (ASEnemyPose.h), not anyone's assets: the clips, mesh and skeleton are never shipped; the motion is
// retargeted onto Ashen Spiral's own armour rig. Source: the Fab pack "Paragon animations retargeted to Manny" (Epic's Paragon heroes' animations; Josue cleared its licence on 2026-10-19,
// Docs/CREDITS.md). Which clip each ability uses, and why the others were passed over: Docs/PARAGON_CLIPS_0.25.md. Track time U is laid out as ASEnemyPose.h describes: wind-up 0..1,
// the blow 1..2, the follow-through 2..3, and every curve starts and ends at 0.
#pragma once
#include "ASEnemyPose.h"
namespace ASEnemyPose {
// OFF by default, on purpose: nothing here can be played, and the offline previews are no proof the clips read better than the hand-made poses. Every ability plays its hand-made track
// (TrackFor) until this is set to true for a playtest, where Josue and P2 judge them by eye. To keep the hand-made pose for ONE ability even then, delete its line from the two switches below.
constexpr bool BakedClipsOn=false;
constexpr int ClipCount=%(count)d;
inline const Track& ClipTrack(int Index) {
    static constexpr Track Table[ClipCount]={
%(tracks)s
    };
    return Table[std::clamp(Index,0,ClipCount-1)];
}
// Which baked track an ability would play (a track index as TrackIndex resolves it, so the abilities that share a body language share a clip too), or -1 when it has no clip.
inline int BakedClipIndexFor(int TrackIdx) {
    switch(TrackIdx) {
%(cases)s
    default: return -1;
    }
}
// What the game uses: the same, but -1 for everything while the switch is off.
inline int ClipIndexFor(int TrackIdx) {return BakedClipsOn?BakedClipIndexFor(TrackIdx):-1;}
// The track an ability actually plays: the baked clip where there is one, the hand-made track (TrackFor) otherwise.
inline const Track& PlayedTrack(int Ability) {
    const int Clip=ClipIndexFor(TrackIndex(Ability));
    return Clip>=0?ClipTrack(Clip):TrackFor(Ability);
}
// The track the baked clip WOULD play with the switch ignored (the hand-made one where the ability has no clip): for the offline previews and the tests.
inline const Track& BakedTrack(int Ability) {
    const int Clip=BakedClipIndexFor(TrackIndex(Ability));
    return Clip>=0?ClipTrack(Clip):TrackFor(Ability);
}
}
"""


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    manny = Manny(sys.argv[1])
    tracks, cases, rows = [], [], []
    unused = set(manny.anims)
    for ability, label, candidates in CANDIDATES:
        best, rejected = choose(manny, ability, label, candidates)
        if best is None:
            print("%-12s no clip passes: %s" % (label, "; ".join("%s (%s)" % r for r in rejected)))
            rows.append((ability, label.rstrip("*"), None, rejected))
            continue
        lines, stats = best
        print("%-12s %-60s %2d curves; wind-up %.2f s, blow to %.2f s of %.2f s, %.0f cm/s, %.0f%% clamped" %
              (label, stats["clips"], stats["curves"], stats["lead"], stats["blow_end"], stats["length"], stats["speed"], 100 * stats["breach"]))
        cases.append("    case %d: return %d; // %s: %s" % (ability, len(cases), label, stats["clips"]))
        tracks += lines
        rows.append((ability, label.rstrip("*"), stats, rejected))
        for clip in stats["clips"].split("+"):
            unused.discard(clip)
    if not cases:
        print("no ability has a clip that passes the gate; nothing written")
        return
    text = HEADER % {"count": len(cases), "tracks": "\n".join(tracks), "cases": "\n".join(cases)}
    Path(sys.argv[2]).write_bytes(text.encode("utf-8"))
    print("wrote", sys.argv[2], "(%d baked tracks)" % len(cases))
    if len(sys.argv) > 3:
        doc = ["# Paragon clip bake (0.25 phase D)", "",
               "Generated by `Tools/paragon_clip_bake.py` from %d sampled Paragon-on-Manny clips (`Tools/paragon_clip_import.py`, `Tools/paragon_clip_export.py`). Nothing is shipped as animation: only numeric "
               "pose curves over the armour rig's channels (`ASEnemyClipTracks.h`). **The baked clips are OFF by default (`BakedClipsOn=false`): every ability plays its hand-made track until someone sets the "
               "switch to true for a playtest.** Abilities 20-33 (elite mutations, ward bosses) share the body language of one of the abilities below (`TrackIndex`), so they follow it. Abilities 1 (dive) and "
               "2 (molt) pose the fliers' wings and have no clip." % len([c for c in manny.anims if not c.startswith("combo:")]), "",
               "Gate: the blow starts at least %.2f s into the clip (%.2f s for the chained counter-strikes, riposte and blink strike), so the wind-up is a telegraph and not a snap, and at most %d%% of curve samples are clamped to the rig's limits. "
               "The candidates are tried in order (best suited first) and the first that passes is kept; an ability with none keeps its hand-made track." % (MIN_LEAD, COUNTER_LEAD, int(MAX_BREACH * 100)), "",
               "**To try them in a playtest:** set `BakedClipsOn=true` in `Source/AshenSpiral/Public/ASEnemyClipTracks.h` and rebuild; both players need the same build. To keep the hand-made pose for one ability "
               "while the rest play clips, delete its `case` line from `BakedClipIndexFor`. To go back, set it to false. Offline previews of the clips: `export_enemy_poses.exe baked` (see `Tools/preview_enemy_poses.py`); "
               "they are geometry sheets, not engine captures, and the first look at them was not convincing for the ground pound (it ends in a deep hunch) and the slash (the arm reaches straight out), which is "
               "why this is a playtest question and not a default.", "",
               "| Ability | Clip played when on | Wind-up | Blow ends | Clamped | Passed over (reason) |", "|---|---|---|---|---|---|"]
        for ability, label, stats, rejected in rows:
            why = "; ".join("`%s`: %s" % r for r in rejected) or "-"
            if stats is None:
                doc.append("| %d %s | **none: hand-made track** | - | - | - | %s |" % (ability, label, why))
            else:
                doc.append("| %d %s | `%s` | %.2f s | %.2f s | %.0f%% | %s |" % (ability, label, stats["clips"], stats["lead"], stats["blow_end"], 100 * stats["breach"], why))
        doc += ["", "Sampled but not used by any ability: " + (", ".join("`%s`" % c for c in sorted(unused) if not c.startswith("combo:")) or "none") + ".", ""]
        Path(sys.argv[3]).write_bytes("\n".join(doc).encode("utf-8"))
        print("wrote", sys.argv[3])


if __name__ == "__main__":
    main()
