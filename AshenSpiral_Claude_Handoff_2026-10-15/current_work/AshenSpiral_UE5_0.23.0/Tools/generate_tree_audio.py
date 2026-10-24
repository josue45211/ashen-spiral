"""0.26 Phase 3: the skill tree's sounds, synthesised from nothing (standard library only; no samples). Mono 16-bit 44.1 kHz into SourceArt/Audio as S_Tree*.wav:
    python Tools/generate_tree_audio.py [outdir]        (default SourceArt/Audio; a spectrogram PNG per sound goes to Saved/VaultStage/TreeAudio)
  S_TreeUnlock    a node bought in the hub: a thump and a struck golden bell, an answering chime a fifth above and a handful of sparkle (the page's burst)
  S_TreeCapstone  a capstone bought: a sub drop and a crack, a bell, a wide major chord ringing out, a glissando up through it and a long shimmer (the page's bigger burst)
  S_TreeWard      Shared Ward: a shield rising: a glassy swell that settles on a two-note ring
  S_TreePredator  Predator's refund: a snap, a low growl and a whoosh rising into a short bell
  S_TreePierce    Piercing Rounds opening: a thin zing climbing through a gust, and a ringing tick
  S_TreeEcho      a Synergy Echo: a soft thump and one bright ping said four times, each quieter
Everything is TUNABLE in the specs. Whether they feel right is a human judgement (UNVERIFIED until Josue and P2 hear them in the game); the printout is what can be checked: peak, RMS, onset, where the energy is.
"""
import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_claw_audio as claw   # the filters, the resonator, the struck-metal partials, the mix and the finish
import generate_team_audio as team   # the rising tone
import make_boss_roars as roar       # the writers and the measurements

RATE = roar.RATE
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceArt" / "Audio"
STAGE = ROOT / "Saved" / "VaultStage" / "TreeAudio"
noise, smooth, band, ring, sweep_sine, mix, soft, finish = claw.noise, claw.smooth, claw.band, claw.ring, claw.sweep_sine, claw.mix, claw.soft, claw.finish
env_bell, expdecay, lowpass, highpass, rise = claw.env_bell, claw.expdecay, claw.lowpass, claw.highpass, team.rise


def after(x, t_end, tau):
    """The signal as it is until t_end, then dying away with time constant tau (a tone that is let go of)."""
    return [v * (1.0 if i / RATE < t_end else expdecay(i / RATE, t_end, tau)) for i, v in enumerate(x)]


def tree_unlock():
    rng = random.Random(9401)
    dur = .62
    n = int(dur * RATE)
    thump = sweep_sine(n, 210, 72, .025, .10, .55)
    clang = ring(n, [(988, .16), (1480, .12), (2217, .08), (3140, .05), (740, .08)], .14, start=.008)
    chime = ring(n, [(1319, .12), (1976, .09), (2637, .06)], .30, start=.07)
    sparkle = band(noise(n, rng), lambda t: 4200 + 4000 * smooth(t / dur), lambda t: 2600, 8)
    sparkle = [v * .14 * env_bell(i / RATE, dur, .02, 1.4) for i, v in enumerate(sparkle)]
    tick = highpass(noise(n, rng), 3000)
    tick = [v * .45 * expdecay(i / RATE, 0.0, .004) for i, v in enumerate(tick)]
    return finish(soft(mix(thump, [v * 1.3 for v in clang], chime, sparkle, tick), 1.3), .86)


def tree_capstone():
    rng = random.Random(9402)
    dur = 2.4
    n = int(dur * RATE)
    sub = sweep_sine(n, 100, 32, .10, .50, .80)
    crack = highpass(noise(n, rng), 1400)
    crack = [v * 1.0 * expdecay(i / RATE, 0.0, .03) for i, v in enumerate(crack)]
    clang = ring(n, [(988, .16), (1480, .12), (2217, .09), (3140, .05)], .35)
    chord = ring(n, [(262, .17), (392, .15), (523, .16), (659, .12), (784, .12), (1047, .09), (1568, .05)], .95, start=.02)
    glide = rise(n, 500, 3200, .70, .13, curve=1.4, start=.05)
    glide = [v * env_bell(i / RATE - .05, .78, .10, 1.2) if i / RATE >= .05 else 0.0 for i, v in enumerate(glide)]
    room = lowpass(noise(n, rng), 1000)
    room = [v * .55 * expdecay(i / RATE, .05, .55) for i, v in enumerate(room)]
    shimmer = band(noise(n, rng), lambda t: 5400, lambda t: 2800, 8)
    shimmer = [v * .12 * env_bell(i / RATE, 1.8, .10, 1.4) for i, v in enumerate(shimmer)]
    return finish(soft(mix(sub, [v * 1.1 for v in crack], [v * 1.3 for v in clang], [v * 1.3 for v in chord], glide, room, shimmer), 1.6), 1.0, fade_ms=60)


def tree_ward():
    rng = random.Random(9403)
    dur = .5
    n = int(dur * RATE)
    tone = after(rise(n, 420, 1180, .28, .22, curve=1.3), .28, .09)
    glass = ring(n, [(1560, .10), (2340, .07), (3510, .04)], .20, start=.26)
    air = band(noise(n, rng), lambda t: 3600, lambda t: 2400, 8)
    air = [v * .10 * env_bell(i / RATE, dur, .05, 1.2) for i, v in enumerate(air)]
    thud = sweep_sine(n, 180, 90, .03, .07, .25)
    return finish(soft(mix(tone, glass, air, thud), 1.2), .70)


def tree_predator():
    rng = random.Random(9404)
    dur = .42
    n = int(dur * RATE)
    snick = highpass(noise(n, rng), 2200)
    snick = [v * .9 * expdecay(i / RATE, 0.0, .012) for i, v in enumerate(snick)]
    growl = sweep_sine(n, 150, 60, .04, .12, .50)
    whoosh = band(noise(n, rng), lambda t: 500 + 3200 * smooth(t / .2), lambda t: 1400, 8)
    whoosh = [v * .5 * env_bell(i / RATE, .26, .02, 1.2) for i, v in enumerate(whoosh)]
    bell = ring(n, [(1175, .10), (1760, .07)], .12, start=.05)
    return finish(soft(mix(snick, growl, whoosh, bell), 1.3), .78)


def tree_pierce():
    rng = random.Random(9405)
    dur = .70
    n = int(dur * RATE)
    zing = after(rise(n, 1100, 4400, .22, .26, curve=1.3), .22, .05)
    wind = band(noise(n, rng), lambda t: 2400 + 3000 * smooth(t / .3), lambda t: 1200, 8)
    wind = [v * .30 * env_bell(i / RATE, .5, .04, 1.2) for i, v in enumerate(wind)]
    glint = ring(n, [(3520, .07), (5280, .04)], .30, start=.20)
    tick = highpass(noise(n, rng), 3500)
    tick = [v * .4 * expdecay(i / RATE, 0.0, .003) for i, v in enumerate(tick)]
    return finish(soft(mix(zing, wind, glint, tick), 1.2), .74)


def tree_echo():
    rng = random.Random(9406)
    dur = .85
    n = int(dur * RATE)
    thump = sweep_sine(n, 150, 56, .04, .10, .45)
    pings = mix(*[ring(n, [(1568, a), (2349, a * .6)], .08, start=t0) for t0, a in ((0.0, .20), (.11, .12), (.22, .07), (.33, .04))])
    shimmer = band(noise(n, rng), lambda t: 4800, lambda t: 2400, 8)
    shimmer = [v * .06 * env_bell(i / RATE, dur, .03, 1.3) for i, v in enumerate(shimmer)]
    return finish(soft(mix(thump, [v * 1.4 for v in pings], shimmer), 1.2), .68)


SOUNDS = {
    "S_TreeUnlock": tree_unlock, "S_TreeCapstone": tree_capstone, "S_TreeWard": tree_ward,
    "S_TreePredator": tree_predator, "S_TreePierce": tree_pierce, "S_TreeEcho": tree_echo,
}


def main():
    args = sys.argv[1:]
    out = Path(args[0]) if args else OUT
    out.mkdir(parents=True, exist_ok=True)
    STAGE.mkdir(parents=True, exist_ok=True)
    for name, make in SOUNDS.items():
        x = make()
        roar.write_mono(out / (name + ".wav"), x)
        roar.spectrogram(x, STAGE / (name + ".png"), top_hz=9000, sx=3, sy=1)
        a, b, c, d, centroid = roar.band_shares(x)
        print("%-16s %.2f s  peak %.1f dBFS  RMS %.1f dBFS  onset %2d ms  energy <120 Hz %2.0f%%, 120-400 %2.0f%%, 400-1500 %2.0f%%, 1.5-5k %2.0f%%  centroid %4.0f Hz"
              % (name, len(x) / RATE, 20 * math.log10(max(abs(v) for v in x)), roar.rms_db(x), roar.onset_ms(x), 100 * a, 100 * b, 100 * c, 100 * d, centroid))


if __name__ == "__main__":
    main()
