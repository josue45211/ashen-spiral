"""0.26 G5 (Josue's order after the 0.24.2 playtest): the boss roar "sounds like a zombie right after a big meal". The spec: VERY LOW pitched, like a lion; it must read as real danger; a SUDDEN
onset (the suddenness is what triggers the response); a little quieter is fine if it sounds right. This writes the three roar files from scratch instead of cutting the Horror Template's moan:
    python Tools/make_boss_roars.py [outdir]        (standard library only, like cut_roars.py; default outdir is Saved/VaultStage/Roar, where import phase L picks the files up)
The cue that matters most is E_BossRoar: it is the wind-up of almost every boss attack (ASEnemyRules.h: CueBossRoar), heard dozens of times a fight; E_BossPhase is the half-health roar (once);
E_EnrageRoar an elite brute's. Each is built the way a big cat's roar is made, and measured against that spec (the printout and a spectrogram PNG per roar):
  - a glottal pulse train at a very low fundamental (about 100 Hz falling to 55) with jitter, shimmer and PERIOD DOUBLING (every other pulse weaker: the subharmonic that makes a roar growl
    an octave lower and rattle), the pitch barking up for a moment at the start;
  - four formants that swing open fast (the jaw dropping: F1 330 -> 900 Hz) and close slowly, plus a chest resonance near 115 Hz;
  - turbulence noise (rasp), with an explosive burst in the first 30 ms;
  - an amplitude envelope that is full in under 10 ms (sudden), holds with a 8 Hz flutter, decays, and leaves a couple of low grunts behind it;
  - a falling sub sine for chest weight, soft saturation for density, a little room.
Everything is TUNABLE in ROARS. Whether it sounds like a lion and not a synthesiser is a human judgement (UNVERIFIED until Josue and P2 hear it); the numbers below are what can be checked.
"""
import array
import cmath
import math
import random
import sys
import wave
import zlib
from pathlib import Path

RATE = 44100
STAGE = Path(__file__).resolve().parents[1] / "Saved" / "VaultStage" / "Roar"
PEAK = 10 ** (-1.0 / 20)  # normalised to -1 dBFS

# name: all times in seconds, pitches in Hz
ROARS = {
    # the wind-up of a boss attack: sudden, deep, one full-throated roar and two low grunts after it
    "E_BossRoar": dict(dur=1.45, seed=11, f_start=124, f_end=68, f_tau=.30, bark=.12, harsh=(.26, .46, .28), noise=.14, drive=2.6, flutter=(8.5, .18), body=.62, decay=.26, tilt=1500, rattle=.24, crest=2.7,
                       sub=(58, 40, .26, .22), grunts=[(.98, .55, .085, 58), (1.22, .40, .075, 52)], room=(.14, .8), level=1.0),
    # the half-health roar: the deepest and longest, a second weight landing under it
    "E_BossPhase": dict(dur=2.6, seed=23, f_start=108, f_end=58, f_tau=.65, bark=.15, harsh=(.30, .52, .32), noise=.16, drive=2.8, flutter=(7.5, .22), body=1.25, decay=.40, tilt=1400, rattle=.28, crest=2.8,
                        sub=(50, 36, .55, .32), grunts=[(1.70, .60, .11, 50), (2.05, .50, .10, 46), (2.32, .38, .09, 42)], room=(.22, 1.2), level=1.0),
    # an elite brute raging: shorter, higher and snarlier
    "E_EnrageRoar": dict(dur=.95, seed=37, f_start=156, f_end=96, f_tau=.22, bark=.12, harsh=(.20, .38, .22), noise=.20, drive=2.6, flutter=(10.0, .16), body=.46, decay=.22, tilt=1700, rattle=.22, crest=2.4,
                         sub=None, grunts=[(.70, .50, .07, 78)], room=(.08, .5), level=1.0),
}


def smooth(x):
    x = min(1.0, max(0.0, x))
    return x * x * (3 - 2 * x)


class Resonator:
    """A two-pole band-pass at unit peak gain whose centre and bandwidth can change every few samples."""

    def __init__(self):
        self.y1 = self.y2 = 0.0
        self.a1 = self.a2 = 0.0
        self.g = 1.0

    def tune(self, f, bw):
        f = min(f, RATE * 0.45)
        r = math.exp(-math.pi * bw / RATE)
        th = 2 * math.pi * f / RATE
        self.a1, self.a2 = 2 * r * math.cos(th), -r * r
        self.g = (1 - r) * math.sqrt(1 + r * r - 2 * r * math.cos(2 * th))

    def tick(self, x):
        y = self.g * x + self.a1 * self.y1 + self.a2 * self.y2
        self.y2, self.y1 = self.y1, y
        return y


def pulse_train(n, f0, harsh, rng):
    """Impulses at f0(t) with timing jitter, amplitude shimmer and period doubling (every other pulse weaker by harsh(t))."""
    x = [0.0] * (n + 2)
    pos, k = 0.0, 0
    while pos < n:
        i, fr = int(pos), pos - int(pos)
        t = pos / RATE
        a = (1.0 - (harsh(t) if k % 2 else 0.0)) * (1.0 + 0.22 * (rng.random() * 2 - 1))
        x[i] += a * (1 - fr)
        x[i + 1] += a * fr
        pos += RATE / (f0(t) * (1 + 0.04 * (rng.random() * 2 - 1)))
        k += 1
    return x[:n]


def bump(t, start, length):
    """A half-sine swell: 0 outside [start, start+length], 1 at the middle."""
    u = (t - start) / length
    return math.sin(math.pi * u) ** 2 if 0.0 <= u <= 1.0 else 0.0


def synth(spec):
    rng = random.Random(spec["seed"])
    dur = spec["dur"]
    n = int(dur * RATE)
    f_start, f_end, f_tau, bark = spec["f_start"], spec["f_end"], spec["f_tau"], spec["bark"]
    harsh_a, harsh_b, harsh_c = spec["harsh"]
    grunts = spec["grunts"]

    def f0(t):  # falls from f_start to f_end, barks up a little in the first 60 ms, and each grunt is lower still
        f = f_end + (f_start - f_end) * math.exp(-t / f_tau)
        f *= 1.0 + bark * math.sin(math.pi * min(1.0, t / .06))
        for (at, amp, length, pitch) in grunts:
            w = bump(t, at, length)
            if w > 0:
                f = f * (1 - w) + pitch * w
        return f

    def harsh(t):  # period doubling grows toward the roar's middle and relaxes at its end
        u = t / dur
        return harsh_a + (harsh_b - harsh_a) * smooth(u / .35) - (harsh_b - harsh_c) * smooth((u - .5) / .4)

    source = pulse_train(n, f0, harsh, rng)
    k_tilt = 1 - math.exp(-2 * math.pi * spec["tilt"] / RATE)  # the glottal spectrum falls about 12 dB an octave: two low-passes
    s1 = s2 = 0.0
    for i in range(n):
        s1 += k_tilt * (source[i] - s1)
        s2 += k_tilt * (s1 - s2)
        source[i] = s2 * 18.0
    tone = [Resonator() for _ in range(6)]
    air = [Resonator() for _ in range(4)]
    flutter_hz, flutter_depth = spec["flutter"]
    body, decay = spec["body"], spec["decay"]

    def envelope(t):
        a = smooth(t / .008)                                                    # full in 8 ms: sudden
        if t < body:
            return a * (1.0 - .10 * (t / body)) * (1.0 + flutter_depth * math.sin(2 * math.pi * flutter_hz * t))
        return a * 0.9 * math.exp(-(t - body) / decay) * (1.0 + .6 * flutter_depth * math.sin(2 * math.pi * flutter_hz * t))

    def grunt_gate(t):
        g = 0.0
        for (at, amp, length, pitch) in grunts:
            g = max(g, amp * bump(t, at, length))
        return g

    mix = [0.0] * n
    for i in range(n):
        t = i / RATE
        if i % 48 == 0:  # a millisecond: the jaw swings open fast and closes slowly
            open_ = smooth(t / .07) - .55 * smooth((t - body * .6) / (dur * .5))
            tone[0].tune(330 + 590 * open_, 90 + 40 * open_)
            tone[1].tune(980 + 430 * open_, 130)
            tone[2].tune(2350 - 250 * open_, 220)
            tone[3].tune(3400, 330)
            tone[4].tune(115, 70)
            tone[5].tune(230 + 60 * open_, 150)   # the body of the voice: where a big chest and throat put the weight of a roar
            air[0].tune(900 + 300 * open_, 420)
            air[1].tune(2700, 900)
            air[2].tune(5200, 1800)
            air[3].tune(700 + 350 * open_, 600)
        s = source[i]
        voiced = tone[0].tick(s) * 1.35 + tone[1].tick(s) * 1.2 + tone[2].tick(s) * .5 + tone[3].tick(s) * .2 + tone[4].tick(s) * .6 + tone[5].tick(s) * 1.1
        w = rng.gauss(0.0, 1.0)
        rattle = air[3].tick(w) * (.55 + .45 * math.sin(2 * math.pi * 31 * t + 2.0 * math.sin(2 * math.pi * 3.3 * t)))
        rasp = air[0].tick(w) * .9 + air[1].tick(w) * .22 + air[2].tick(w) * .05 + rattle * spec["rattle"]
        env = envelope(t)
        burst = math.exp(-t / .018)  # the explosive start: a burst of noise in the first 30 ms
        env_n = spec["noise"] * env + .8 * burst
        gate = max(env, grunt_gate(t) * .9)
        mix[i] = voiced * gate * 1.0 + rasp * env_n * max(gate, .0) * .6
    # the chest: a sine that sinks (sub_start -> sub_end) and dies away
    if spec["sub"]:
        sub_a, sub_b, sub_tau, sub_level = spec["sub"]
        peak0 = max(abs(v) for v in mix) or 1.0
        phase = 0.0
        for i in range(n):
            t = i / RATE
            f = sub_b + (sub_a - sub_b) * math.exp(-t / .22)
            phase += 2 * math.pi * f / RATE
            mix[i] += peak0 * sub_level * 0.5 * math.sin(phase) * smooth(t / .006) * math.exp(-t / sub_tau)
    # soft saturation for density, then keep what the ear wants: nothing under 28 Hz, nothing over 6.5 kHz
    peak = max(abs(v) for v in mix) or 1.0
    d = spec["drive"]
    mix = [math.tanh(d * v / peak + .12 * (v / peak) ** 2) for v in mix]
    hp, lp, prev_x, prev_y = 0.0, 0.0, 0.0, 0.0
    k_hp = math.exp(-2 * math.pi * 28 / RATE)
    k_lp = 1 - math.exp(-2 * math.pi * 6500 / RATE)
    out = []
    for v in mix:
        prev_y = k_hp * (prev_y + v - prev_x)
        prev_x = v
        lp += k_lp * (prev_y - lp)
        out.append(lp)
    out = reverb(out, *spec["room"])
    peak = max(abs(v) for v in out) or 1.0
    c = spec["crest"]
    out = [math.tanh(c * v / peak) / math.tanh(c) for v in out]  # a soft limiter: the body comes up under the onset
    peak = max(abs(v) for v in out) or 1.0
    return [v * PEAK * spec["level"] / peak for v in out]


def reverb(x, wet, rt60):
    """A small Schroeder room: four combs and two all-passes, wet mixed under the dry."""
    if wet <= 0:
        return x
    delays = [int(RATE * d) for d in (.0297, .0371, .0411, .0437)]
    combs = [([0.0] * d, 10 ** (-3.0 * d / RATE / rt60)) for d in delays]
    idx = [0] * 4
    ap = [([0.0] * int(RATE * .005), .7), ([0.0] * int(RATE * .0017), .7)]
    ap_idx = [0, 0]
    n_tail = int(rt60 * RATE * .5)
    out = []
    for i in range(len(x) + n_tail):
        v = x[i] if i < len(x) else 0.0
        acc = 0.0
        for c in range(4):
            buf, g = combs[c]
            y = buf[idx[c]]
            buf[idx[c]] = v + g * y
            idx[c] = (idx[c] + 1) % len(buf)
            acc += y
        acc *= .25
        for a in range(2):
            buf, g = ap[a]
            y = buf[ap_idx[a]]
            z = acc + g * y
            buf[ap_idx[a]] = z
            acc = y - g * z
            ap_idx[a] = (ap_idx[a] + 1) % len(buf)
        out.append(v * (1 - wet * .5) + acc * wet * 2.2)
    return out


# ---- analysis: what can be measured without listening ----

def fft(x):
    n = len(x)
    if n == 1:
        return x
    even, odd = fft(x[0::2]), fft(x[1::2])
    t = [cmath.exp(-2j * math.pi * k / n) * odd[k] for k in range(n // 2)]
    return [even[k] + t[k] for k in range(n // 2)] + [even[k] - t[k] for k in range(n // 2)]


def spectrum(x, start, n=4096):
    seg = x[start:start + n]
    seg = seg + [0.0] * (n - len(seg))
    win = [seg[i] * (0.5 - 0.5 * math.cos(2 * math.pi * i / (n - 1))) for i in range(n)]
    return [abs(c) for c in fft(win)[:n // 2]]


def onset_ms(x):
    """From the first moment the envelope clears -40 dB (re its peak) to -6 dB: how sudden the start is."""
    win = int(RATE * .001)
    env = [math.sqrt(sum(v * v for v in x[i:i + win]) / win) for i in range(0, len(x) - win, win)]
    peak = max(env) or 1.0
    t0 = next(i for i, e in enumerate(env) if e >= peak * 10 ** (-40 / 20))
    t1 = next(i for i, e in enumerate(env) if e >= peak * 10 ** (-6 / 20))
    return t1 - t0


def f0_track(x, hop=.05):
    """The period of the strongest low-frequency repetition (autocorrelation of the signal decimated to 5.5 kHz), every hop seconds: (time, Hz, strength)."""
    d = 8
    y = [sum(x[i:i + d]) / d for i in range(0, len(x) - d, d)]
    rate = RATE / d
    win, lo, hi = int(.09 * rate), int(rate / 150), int(rate / 28)
    out = []
    for start in range(0, len(y) - win - hi, int(hop * rate)):
        seg = y[start:start + win + hi]
        e0 = sum(v * v for v in seg[:win]) or 1e-12
        best, lag_best = 0.0, 0
        for lag in range(lo, hi):
            c = sum(seg[i] * seg[i + lag] for i in range(win)) / e0
            if c > best:
                best, lag_best = c, lag
        out.append((start / rate, rate / lag_best if lag_best else 0.0, best))
    return out


def band_shares(x):
    spec = [0.0] * 2048
    count = 0
    for start in range(0, max(1, len(x) - 4096), 2048):
        s = spectrum(x, start)
        spec = [a + b * b for a, b in zip(spec, s)]
        count += 1
    total = sum(spec) or 1.0
    hz = RATE / 4096
    def share(a, b):
        return sum(spec[int(a / hz):int(b / hz)]) / total
    centroid = sum(i * hz * v for i, v in enumerate(spec)) / total
    return share(20, 120), share(120, 400), share(400, 1500), share(1500, 5000), centroid


def a_weight(f):
    f2 = f * f
    ra = (12194.0 ** 2 * f2 * f2) / ((f2 + 20.6 ** 2) * math.sqrt((f2 + 107.7 ** 2) * (f2 + 737.9 ** 2)) * (f2 + 12194.0 ** 2))
    return ra  # linear magnitude (1.0 at 1 kHz after the +2 dB normalisation is applied by the caller)


def a_level(x):
    """Perceived loudness, A-weighted, in dB (full scale = 0): the mean A-weighted power over 4096-sample windows."""
    hz = RATE / 4096
    weights = [a_weight(max(i * hz, 1.0)) ** 2 for i in range(2048)]
    total, count = 0.0, 0
    for start in range(0, max(1, len(x) - 4096), 2048):
        s = spectrum(x, start)
        total += sum(w * v * v for w, v in zip(weights, s))
        count += 1
    return 10 * math.log10(max(1e-12, total / max(1, count) / (4096 * 2048 * .375)))


def png(path, rows):
    h, w = len(rows), len(rows[0])
    raw = b"".join(b"\x00" + bytes(c for px in row for c in px) for row in rows)
    def chunk(tag, data):
        c = tag + data
        return len(data).to_bytes(4, "big") + c + (zlib.crc32(c) & 0xffffffff).to_bytes(4, "big")
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", w.to_bytes(4, "big") + h.to_bytes(4, "big") + b"\x08\x02\x00\x00\x00") + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def spectrogram(x, path, top_hz=2500, sx=4, sy=3):
    n, hop = 2048, int(RATE * .01)
    frames = [spectrum(x, s, n) for s in range(0, max(1, len(x) - n), hop)]
    hz = RATE / n
    rows_bins = int(top_hz / hz)
    peak = max(max(f[:rows_bins]) for f in frames) or 1.0
    def colour(db):  # -70 dB black ... 0 dB white, through purple and orange
        u = min(1.0, max(0.0, (db + 70) / 70))
        return (int(255 * min(1, u * 1.6)), int(255 * max(0, u * 1.6 - .6)), int(255 * (math.sin(u * math.pi) * .7 + max(0, u * 2 - 1.3))))
    rows = []
    for b in range(rows_bins - 1, -1, -1):
        row = [colour(20 * math.log10(max(f[b] / peak, 1e-4))) for f in frames for _ in range(sx)]
        rows.extend([row] * sy)
    png(path, rows)


def rms_db(x):
    return 20 * math.log10(max(1e-9, math.sqrt(sum(v * v for v in x) / max(1, len(x)))))


def report(name, x):
    onset = onset_ms(x)
    track = f0_track(x)
    strong = [(t, f, c) for (t, f, c) in track if c > .35]
    f_lo = min((f for _, f, _ in strong), default=0)
    f_med = sorted(f for _, f, _ in strong)[len(strong) // 2] if strong else 0
    a, b, c, d, centroid = band_shares(x)
    print("%-13s %.2f s  RMS %.1f dBFS, A-weighted %.1f dB  onset(-40 -> -6 dB) %d ms | pitch (periodic frames %d): median %.0f Hz, lowest %.0f Hz | energy 20-120 Hz %.0f%%, 120-400 %.0f%%, 400-1500 %.0f%%, 1.5-5k %.0f%%, centroid %.0f Hz"
          % (name, len(x) / RATE, rms_db(x), a_level(x), onset, len(strong), f_med, f_lo, 100 * a, 100 * b, 100 * c, 100 * d, centroid))


def read_mono(path):
    with wave.open(str(path), "rb") as w:
        ch, width, rate, frames = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        data = array.array("h", w.readframes(frames))
    mono = [sum(data[i:i + ch]) / (ch * 32768.0) for i in range(0, len(data), ch)]
    if rate != RATE:  # linear resample for a fair comparison
        step = rate / RATE
        mono = [mono[min(len(mono) - 1, int(i * step))] for i in range(int(len(mono) / step))]
    return mono


def write_mono(path, x):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(array.array("h", (int(max(-1.0, min(1.0, s)) * 32767) for s in x)).tobytes())


def main():
    args = sys.argv[1:]
    only = [a for a in args if a in ROARS]           # roar names limit the run to those
    folders = [a for a in args if a not in ROARS]    # the first other argument is the output folder
    out = Path(folders[0]) if folders else STAGE
    out.mkdir(parents=True, exist_ok=True)
    for name, spec in ROARS.items():
        if only and name not in only:
            continue
        old = out / (name + ".wav")
        if old.is_file() and not (out / (name + "_0242.wav")).is_file():
            old.replace(out / (name + "_0242.wav"))  # keep the 0.24.2 cut (the Horror Template's moan) beside the new file for comparison
        x = synth(spec)
        write_mono(out / (name + ".wav"), x)
        report(name, x)
        spectrogram(x, out / (name + "_spectrogram.png"))
    for name in ROARS:
        old = out / (name + "_0242.wav")
        if old.is_file() and (not only or name in only):
            report(name + "(0.24.2)", read_mono(old))


if __name__ == "__main__":
    main()
