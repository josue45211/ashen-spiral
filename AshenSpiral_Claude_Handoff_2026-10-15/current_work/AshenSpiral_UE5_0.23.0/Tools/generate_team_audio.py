"""0.26 Phase 2: the team skills' sounds, synthesised from nothing (standard library only; no samples). Mono 16-bit 44.1 kHz into SourceArt/Audio as S_Brand*, S_Tether*, S_Twin*.wav:
    python Tools/generate_team_audio.py [outdir]        (default SourceArt/Audio; a spectrogram PNG per sound goes to Saved/VaultStage/TeamAudio)
  Ashen Brand   (Mark & Pop)   S_BrandSet    the brand lands: a stamp, a bright metallic ping and a rising shimmer
                               S_BrandTick   a quarter of the brand filled: a small bright ping (the game raises its pitch with the fill)
                               S_BrandPop    the burst: a boom, a crack and a golden bell chord ringing out
                               S_BrandFizzle the brand runs out: a dull breath and a flat knock
  Ember Tether  (Link)         S_TetherOn    the line forms: a rising electric whine, crackle, a chime
                               S_TetherZap1..3  a lash striking an enemy: a sharp gated crackle over a falling chirp (three variants)
                               S_TetherEnd   the line ends: a falling zzzt and a soft pop
  Twin Collapse (Twin Cast)    S_TwinCall    a call for the partner: two rising bell tones
                               S_TwinLesser  the small collapse: a soft implosion thump and a violet shimmer
                               S_TwinConverge  the two casts joined: a swell that rises and tightens for .65 s, and stops where the blast begins
                               S_TwinBlast   the detonation: a sub drop, a roaring burst, a long chord ringing out in the room
Everything is TUNABLE in the specs. Whether they feel right is a human judgement (UNVERIFIED until Josue and P2 hear them in the game); the printout is what can be checked: peak, RMS, onset, where the energy is.
"""
import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_claw_audio as claw  # the filters, the resonator, the struck-metal partials, the mix and the finish
import make_boss_roars as roar      # the writers and the measurements

RATE = roar.RATE
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceArt" / "Audio"
STAGE = ROOT / "Saved" / "VaultStage" / "TeamAudio"
noise, smooth, band, ring, sweep_sine, mix, soft, finish = claw.noise, claw.smooth, claw.band, claw.ring, claw.sweep_sine, claw.mix, claw.soft, claw.finish
env_bell, expdecay, lowpass, highpass = claw.env_bell, claw.expdecay, claw.lowpass, claw.highpass


def rise(n, f0, f1, dur, amp, curve=2.0, start=0.0):
    """A tone whose pitch climbs from f0 to f1 over `dur` (accelerating with `curve`) and whose level climbs with it."""
    out = [0.0] * n
    phase = 0.0
    for i in range(int(start * RATE), min(n, int((start + dur) * RATE))):
        t = i / RATE - start
        u = min(1.0, t / dur) ** curve
        phase += 2 * math.pi * (f0 + (f1 - f0) * u) / RATE
        out[i] = amp * math.sin(phase) * math.sqrt(u + .02)
    return out


def gated(x, rng, rate_hz, depth=1.0):
    """The signal chopped by a random on/off gate (a crackle): `rate_hz` decisions a second."""
    n, out = len(x), []
    hold, state = max(1, int(RATE / rate_hz)), 1.0
    for i, v in enumerate(x):
        if i % hold == 0:
            state = 1.0 if rng.random() < .55 else 1.0 - depth
        out.append(v * state)
    return out


# ---------------------------------------------------------------- Ashen Brand

def brand_set():
    rng = random.Random(9101)
    dur = .34
    n = int(dur * RATE)
    stamp = sweep_sine(n, 230, 88, .03, .09, .50)
    ping = ring(n, [(1480, .13), (2230, .09), (3310, .05), (740, .07)], .09, start=.012)
    shimmer = band(noise(n, rng), lambda t: 2400 + 3200 * smooth(t / dur), lambda t: 1400, 8)
    shimmer = [v * .16 * env_bell(i / RATE, dur, .05, 1.3) for i, v in enumerate(shimmer)]
    click = highpass(noise(n, rng), 2500)
    click = [v * .5 * expdecay(i / RATE, 0.0, .004) for i, v in enumerate(click)]
    return finish(soft(mix(stamp, [v * 1.4 for v in ping], shimmer, click), 1.3), .80)


def brand_tick():
    rng = random.Random(9102)
    dur = .16
    n = int(dur * RATE)
    ping = ring(n, [(2200, .22), (3300, .11), (4400, .05)], .05)
    click = highpass(noise(n, rng), 3000)
    click = [v * .35 * expdecay(i / RATE, 0.0, .004) for i, v in enumerate(click)]
    return finish(mix(ping, click), .55)


def brand_pop():
    rng = random.Random(9103)
    dur = .95
    n = int(dur * RATE)
    boom = sweep_sine(n, 125, 44, .09, .22, .62)
    crack = highpass(noise(n, rng), 1400)
    crack = [v * 1.0 * expdecay(i / RATE, 0.0, .03) for i, v in enumerate(crack)]
    body = band(noise(n, rng), lambda t: 900 - 500 * smooth(t / .2), lambda t: 1800, 8)
    body = [v * .8 * env_bell(i / RATE, .30, .004, 1.5) for i, v in enumerate(body)]
    chord = ring(n, [(523, .17), (659, .15), (784, .14), (1047, .10), (1568, .06)], .38, start=.012)
    tail = band(noise(n, rng), lambda t: 5200, lambda t: 2600, 8)
    tail = [v * .10 * env_bell(i / RATE, dur, .05, 1.2) for i, v in enumerate(tail)]
    return finish(soft(mix(boom, crack, body, [v * 1.3 for v in chord], tail), 1.5), 1.0)


def brand_fizzle():
    rng = random.Random(9104)
    dur = .36
    n = int(dur * RATE)
    breath = band(noise(n, rng), lambda t: 1300 - 900 * smooth(t / dur), lambda t: 1100, 8)
    breath = [v * .7 * env_bell(i / RATE, dur, .03, 1.2) for i, v in enumerate(breath)]
    knock = ring(n, [(420, .16), (630, .08), (910, .04)], .06, start=.01)
    return finish(mix(breath, knock), .46)


# ---------------------------------------------------------------- Ember Tether

def tether_on():
    rng = random.Random(9201)
    dur = .62
    n = int(dur * RATE)
    whine = rise(n, 320, 1900, .34, .30, curve=1.6)
    whine = [v * (1.0 if i / RATE < .34 else expdecay(i / RATE, .34, .08)) for i, v in enumerate(whine)]
    crackle = gated(highpass(noise(n, rng), 1800), rng, 260, 1.0)
    crackle = [v * .55 * env_bell(i / RATE, dur, .03, 1.1) for i, v in enumerate(crackle)]
    hum = [.20 * math.sin(2 * math.pi * 92 * i / RATE) * env_bell(i / RATE, dur, .08, 1.0) for i in range(n)]
    chime = ring(n, [(1320, .12), (1980, .09), (2640, .05)], .22, start=.30)
    return finish(soft(mix(whine, crackle, hum, chime), 1.3), .82)


def tether_zap(seed):
    rng = random.Random(seed)
    dur = .15 + .02 * rng.random()
    n = int(dur * RATE)
    crackle = gated(highpass(noise(n, rng), 1500), rng, 900, 1.0)
    crackle = [v * 1.0 * expdecay(i / RATE, 0.0, .035) for i, v in enumerate(crackle)]
    chirp = sweep_sine(n, 3600 * (.9 + .2 * rng.random()), 900, .03, .05, .26)
    thud = sweep_sine(n, 170, 80, .02, .04, .22)
    return finish(soft(mix(crackle, chirp, thud), 1.2), .70)


def tether_end():
    rng = random.Random(9202)
    dur = .5
    n = int(dur * RATE)
    fall = [0.0] * n
    phase = 0.0
    for i in range(n):
        t = i / RATE
        phase += 2 * math.pi * (200 + 1200 * math.exp(-t / .12)) / RATE
        fall[i] = .26 * math.sin(phase) * math.exp(-t / .16)
    crackle = gated(highpass(noise(n, rng), 1800), rng, 220, 1.0)
    crackle = [v * .45 * expdecay(i / RATE, 0.0, .12) for i, v in enumerate(crackle)]
    pop = sweep_sine(n, 260, 90, .02, .05, .30)
    return finish(soft(mix(fall, crackle, pop), 1.2), .62)


# ---------------------------------------------------------------- Twin Collapse

def twin_call():
    rng = random.Random(9301)
    dur = .52
    n = int(dur * RATE)
    first = ring(n, [(880, .20), (1760, .07), (2640, .04)], .26)
    second = ring(n, [(1318.5, .21), (2637, .07), (3956, .035)], .30, start=.11)
    shimmer = band(noise(n, rng), lambda t: 4200, lambda t: 2200, 8)
    shimmer = [v * .05 * env_bell(i / RATE, dur, .06, 1.2) for i, v in enumerate(shimmer)]
    return finish(mix(first, second, shimmer), .78)


def twin_lesser():
    rng = random.Random(9302)
    dur = .46
    n = int(dur * RATE)
    thump = sweep_sine(n, 140, 50, .05, .16, .52)
    shimmer = ring(n, [(660, .09), (990, .07), (1485, .045)], .20, start=.02)
    inward = band(noise(n, rng), lambda t: 3000 - 2400 * smooth(t / .28), lambda t: 1500, 8)
    inward = [v * .45 * env_bell(i / RATE, .32, .10, 1.3) for i, v in enumerate(inward)]
    return finish(soft(mix(thump, shimmer, inward), 1.3), .74)


def twin_converge():
    rng = random.Random(9303)
    dur = .65
    n = int(dur * RATE)
    tone = rise(n, 90, 760, dur, .34, curve=2.2)
    swell = band(noise(n, rng), lambda t: 400 + 3800 * (t / dur) ** 2, lambda t: 1000 + 1800 * (t / dur), 8)
    swell = [v * .75 * (i / n) ** 1.6 for i, v in enumerate(swell)]
    sub = [.34 * math.sin(2 * math.pi * (38 + 40 * (i / n)) * i / RATE) * (i / n) ** 1.2 for i in range(n)]
    shiver = ring(n, [(1180, .05), (1770, .04)], 5.0, start=.20)
    shiver = [v * (i / n) ** 2 for i, v in enumerate(shiver)]
    x = mix(tone, swell, sub, shiver)
    cut = int(.012 * RATE)  # it stops where the blast begins (a short fade, no tail: the blast's own sound takes over)
    for i in range(cut):
        x[-1 - i] *= i / cut
    return finish(soft(x, 1.3), .86, fade_ms=2)


def twin_blast():
    rng = random.Random(9304)
    dur = 2.1
    n = int(dur * RATE)
    sub = sweep_sine(n, 88, 28, .12, .60, .80)
    roarband = band(noise(n, rng), lambda t: 2400 * math.exp(-t / .25) + 220, lambda t: 2200, 8)
    roarband = [v * 1.1 * env_bell(i / RATE, .55, .006, 1.3) for i, v in enumerate(roarband)]
    crack = highpass(noise(n, rng), 1200)
    crack = [v * 1.0 * expdecay(i / RATE, 0.0, .025) for i, v in enumerate(crack)]
    chord = ring(n, [(392, .20), (587, .15), (784, .13), (1175, .09), (1568, .06)], .85, start=.015)
    room = lowpass(noise(n, rng), 900)
    room = [v * .65 * expdecay(i / RATE, .05, .45) for i, v in enumerate(room)]
    shimmer = band(noise(n, rng), lambda t: 5600, lambda t: 2800, 8)
    shimmer = [v * .12 * env_bell(i / RATE, 1.2, .08, 1.4) for i, v in enumerate(shimmer)]
    return finish(soft(mix(sub, roarband, [v * 1.2 for v in crack], [v * 1.2 for v in chord], room, shimmer), 1.6), 1.0, fade_ms=40)


SOUNDS = {
    "S_BrandSet": brand_set, "S_BrandTick": brand_tick, "S_BrandPop": brand_pop, "S_BrandFizzle": brand_fizzle,
    "S_TetherOn": tether_on, "S_TetherZap1": lambda: tether_zap(9211), "S_TetherZap2": lambda: tether_zap(9212), "S_TetherZap3": lambda: tether_zap(9213), "S_TetherEnd": tether_end,
    "S_TwinCall": twin_call, "S_TwinLesser": twin_lesser, "S_TwinConverge": twin_converge, "S_TwinBlast": twin_blast,
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
