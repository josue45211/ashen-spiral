"""0.26 Twin Claws: the claws' sounds, synthesised from nothing (standard library only; no samples). Mono 16-bit 44.1 kHz into SourceArt/Audio as S_Claw*.wav:
    python Tools/generate_claw_audio.py [outdir]        (default SourceArt/Audio; a spectrogram PNG per sound goes to Saved/VaultStage/ClawAudio)
  S_ClawSwing1..3   a rake through the air: a rising band of wind, a thin metallic "shing" as the blades cross, a soft low body (three variants; one plays at random per swing)
  S_ClawHit1..3     a claw landing: a sharp "tch" transient, a wet tearing "rrip" (noise tremolo'd fast), a scrape ring and a short thud
  S_PounceCoil      the crouch: leather creak, a breath in, claws drawn across stone
  S_PounceLaunch    the leap: a sweeping whoosh with a short predator snarl and a high shing
  S_PounceLand      touching down: a sub thump, gravel thrown, claws digging in
  S_PounceStrike    the arrival blow: the biggest hit, a low boom under a tearing sweep and a long metallic ring
  S_PounceRelease   letting go: an exhale and a small tick
Everything is TUNABLE in the specs. Whether they feel right is a human judgement (UNVERIFIED until Josue and P2 hear them in the game); the printout is what can be checked: peak, RMS, onset, where the energy is.
"""
import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import make_boss_roars as roar  # the resonator, the room, the writers and the measurements

RATE = roar.RATE
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceArt" / "Audio"
STAGE = ROOT / "Saved" / "VaultStage" / "ClawAudio"
PEAK = 10 ** (-2.0 / 20)  # normalised to -2 dBFS


def noise(n, rng):
    return [rng.random() * 2 - 1 for _ in range(n)]


def smooth(x):
    return roar.smooth(x)


def env_bell(t, dur, attack, power=2.0):
    """Rises in `attack` seconds, falls to nothing at `dur`."""
    if t < 0 or t >= dur:
        return 0.0
    rise = smooth(t / attack) if attack > 0 else 1.0
    fall = smooth((dur - t) / max(1e-6, dur - attack)) ** power if t > attack else 1.0
    return rise * fall


def expdecay(t, start, tau):
    return math.exp(-(t - start) / tau) if t >= start else 0.0


def band(x, f_of_t, bw_of_t, step=16):
    """The signal through a band-pass whose centre and width move with time (retuned every `step` samples)."""
    r = roar.Resonator()
    out = []
    for i, v in enumerate(x):
        if i % step == 0:
            t = i / RATE
            r.tune(f_of_t(t), bw_of_t(t))
        out.append(r.tick(v))
    return out


def highpass(x, fc):
    a = math.exp(-2 * math.pi * fc / RATE)
    y, prev_in, prev_out = [], 0.0, 0.0
    for v in x:
        o = a * (prev_out + v - prev_in)
        y.append(o)
        prev_in, prev_out = v, o
    return y


def lowpass(x, fc):
    a = 1 - math.exp(-2 * math.pi * fc / RATE)
    y, s = [], 0.0
    for v in x:
        s += a * (v - s)
        y.append(s)
    return y


def ring(n, partials, tau, start=0.0):
    """Struck metal: inharmonic partials (Hz, amplitude) that each die away exponentially."""
    out = [0.0] * n
    for f, a in partials:
        for i in range(int(start * RATE), n):
            t = i / RATE - start
            out[i] += a * math.sin(2 * math.pi * f * t) * math.exp(-t / (tau * (1.0 + 300.0 / f)))
    return out


def sweep_sine(n, f0, f1, tau_f, tau_a, amp, start=0.0):
    out = [0.0] * n
    phase = 0.0
    for i in range(int(start * RATE), n):
        t = i / RATE - start
        f = f1 + (f0 - f1) * math.exp(-t / tau_f)
        phase += 2 * math.pi * f / RATE
        out[i] = amp * math.sin(phase) * math.exp(-t / tau_a)
    return out


def mix(*layers):
    n = max(len(l) for l in layers)
    out = [0.0] * n
    for l in layers:
        for i, v in enumerate(l):
            out[i] += v
    return out


def soft(x, drive):
    if drive <= 0:
        return x
    k = 1.0 / math.tanh(drive)
    return [math.tanh(v * drive) * k for v in x]


def finish(x, level=1.0, fade_ms=6):
    peak = max(abs(v) for v in x) or 1.0
    y = [v * PEAK * level / peak for v in x]
    fade = int(RATE * fade_ms / 1000)
    for i in range(min(fade, len(y))):
        y[-1 - i] *= i / fade
        y[i] *= min(1.0, i / max(1, fade // 3))  # a few samples of attack: no click, still sudden
    return y


# ---------------------------------------------------------------- the sounds

def swing(seed):
    rng = random.Random(seed)
    dur = .20 + .02 * rng.random()
    n = int(dur * RATE)
    lo, hi = 900 * (.9 + .2 * rng.random()), 3600 * (.9 + .2 * rng.random())
    wind = band(noise(n, rng), lambda t: lo + (hi - lo) * smooth(t / (dur * .85)), lambda t: 1300, 8)
    wind = [v * env_bell(i / RATE, dur, dur * .30) for i, v in enumerate(wind)]
    base = 2000 * (.92 + .16 * rng.random())
    shing = ring(n, [(base, .11), (base * 1.55, .07), (base * 2.2, .05)], .030, start=dur * .26)
    body = sweep_sine(n, 170 * (.9 + .2 * rng.random()), 82, .035, .045, .24, start=dur * .10)
    x = mix([v * 1.0 for v in wind], shing, body)
    return finish(soft(x, 1.1), .62)


def hit(seed, heavy=1.0):
    rng = random.Random(seed)
    dur = .26 * (.85 + .35 * heavy)
    n = int(dur * RATE)
    tch = highpass(noise(n, rng), 1600)
    tch = [v * 1.5 * expdecay(i / RATE, 0.0, .016) for i, v in enumerate(tch)]
    rip_src = noise(n, rng)
    rip = band(rip_src, lambda t: 1500 * (.9 + .2 * rng.random() * 0 + .1), lambda t: 2200, 8)
    fmod = 38 + 14 * rng.random()
    rip = [v * (.55 + .45 * math.sin(2 * math.pi * fmod * i / RATE) ** 2) * expdecay(i / RATE, 0.004, .060 * heavy) for i, v in enumerate(rip)]
    base = 2450 * (.9 + .2 * rng.random())
    scrape = ring(n, [(base, .17), (base * 1.41, .10), (base * 2.03, .06), (base * .62, .08)], .034 * heavy, start=.006)
    thud = sweep_sine(n, 150 * (.9 + .2 * rng.random()), 70, .030, .040 * heavy, .20)
    x = mix([v * 1.0 for v in tch], [v * 1.6 for v in rip], [v * 1.3 for v in scrape], thud)
    return finish(soft(x, 1.3), .85)


def coil():
    rng = random.Random(7001)
    dur = .22
    n = int(dur * RATE)
    breath = band(noise(n, rng), lambda t: 500 + 1300 * smooth(t / dur), lambda t: 900, 8)
    breath = [v * env_bell(i / RATE, dur, dur * .65, 1.2) * .55 for i, v in enumerate(breath)]
    creak = []
    phase = 0.0
    for i in range(n):
        t = i / RATE
        f = 240 + 70 * math.sin(2 * math.pi * 17 * t) + 90 * t
        phase += 2 * math.pi * f / RATE
        creak.append(.13 * math.sin(phase) * env_bell(t, dur, dur * .3))
    scrape = band(noise(n, rng), lambda t: 2600, lambda t: 1600, 8)
    scrape = [v * (.30 * smooth((t - dur * .55) / (dur * .3)) * smooth((dur - t) / (dur * .15)) if (t := i / RATE) > dur * .55 else 0.0) for i, v in enumerate(scrape)]
    return finish(mix(breath, creak, scrape), .55)


def launch():
    rng = random.Random(7002)
    dur = .46
    n = int(dur * RATE)
    whoosh = band(noise(n, rng), lambda t: 380 + 2600 * smooth(t / .26), lambda t: 1500, 8)
    whoosh = [v * env_bell(i / RATE, dur, .03, 1.4) for i, v in enumerate(whoosh)]
    # a short predator snarl: a glottal pulse train at 95 -> 150 Hz through two formants, 140 ms
    snarl = [0.0] * n
    f1, f2 = roar.Resonator(), roar.Resonator()
    pos, k = 0.0, 0
    pulses = [0.0] * n
    while pos < int(.15 * RATE):
        t = pos / RATE
        pulses[int(pos)] += (1.0 - (.4 if k % 2 else 0.0)) * (1 + .2 * (rng.random() * 2 - 1))
        pos += RATE / ((95 + 55 * smooth(t / .15)) * (1 + .03 * (rng.random() * 2 - 1)))
        k += 1
    for i in range(n):
        t = i / RATE
        if i % 16 == 0:
            f1.tune(520 + 260 * smooth(t / .1), 220)
            f2.tune(1450 + 500 * smooth(t / .1), 340)
        snarl[i] = (f1.tick(pulses[i]) * 1.0 + f2.tick(pulses[i]) * .8) * env_bell(t, .15, .01, 1.0) * .55
    base = 2300
    shing = ring(n, [(base, .09), (base * 1.5, .06)], .05, start=.04)
    x = mix(whoosh, snarl, shing)
    return finish(soft(x, 1.3), .8)


def land():
    rng = random.Random(7003)
    dur = .50
    n = int(dur * RATE)
    thump = sweep_sine(n, 95, 38, .06, .12, .50)
    gravel = [0.0] * n
    for _ in range(11):
        at = rng.random() * .16
        for i in range(int(at * RATE), min(n, int((at + .03) * RATE))):
            gravel[i] += (rng.random() * 2 - 1) * math.exp(-((i / RATE) - at) / .008) * (.3 + .4 * rng.random())
    gravel = band(gravel, lambda t: 2200, lambda t: 2500, 8)
    dig = band(noise(n, rng), lambda t: 3100 - 900 * t, lambda t: 1300, 8)
    dig = [v * .5 * smooth(t / .02) * math.exp(-t / .07) if (t := i / RATE) >= .0 else 0.0 for i, v in enumerate(dig)]
    body = [v * 1.0 for v in lowpass(noise(n, rng), 260)]
    body = [v * 1.3 * math.exp(-(i / RATE) / .05) for i, v in enumerate(body)]
    return finish(soft(mix(thump, [v * 2.2 for v in gravel], [v * 1.6 for v in dig], [v * .45 for v in body]), 1.4), .95)


def strike():
    base = hit(7004, heavy=1.9)
    rng = random.Random(7005)
    n = int(.62 * RATE)
    base = base + [0.0] * (n - len(base))
    boom = sweep_sine(n, 90, 42, .08, .16, .42)
    tear = band(noise(n, rng), lambda t: 3200 - 2500 * smooth(t / .2), lambda t: 2400, 8)
    tear = [v * .8 * env_bell(i / RATE, .26, .01, 1.3) for i, v in enumerate(tear)]
    metal = ring(n, [(2150, .16), (3240, .10), (4620, .07), (1330, .09)], .06, start=.004)
    x = mix([v * 1.5 for v in base], boom, [v * 1.3 for v in tear], [v * 1.4 for v in metal])
    return finish(soft(x, 1.5), 1.0)


def release():
    rng = random.Random(7006)
    dur = .22
    n = int(dur * RATE)
    breath = band(noise(n, rng), lambda t: 1500 - 900 * smooth(t / dur), lambda t: 1000, 8)
    breath = [v * env_bell(i / RATE, dur, .04, 1.3) * .6 for i, v in enumerate(breath)]
    tick = highpass(noise(n, rng), 2500)
    tick = [v * .35 * expdecay(i / RATE, .0, .012) for i, v in enumerate(tick)]
    return finish(mix(breath, tick), .45)


SOUNDS = {
    "S_ClawSwing1": lambda: swing(8101), "S_ClawSwing2": lambda: swing(8102), "S_ClawSwing3": lambda: swing(8103),
    "S_ClawHit1": lambda: hit(8201), "S_ClawHit2": lambda: hit(8202), "S_ClawHit3": lambda: hit(8203),
    "S_PounceCoil": coil, "S_PounceLaunch": launch, "S_PounceLand": land, "S_PounceStrike": strike, "S_PounceRelease": release,
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
