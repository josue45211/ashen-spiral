"""0.24.2 (Edison: "resolve the roar so it is loud and clear in-engine"): cuts the boss and elite roars out of the Horror Template's monster sound.
    python Tools/cut_roars.py            (after Tools/import_vault_assets.py phase L has exported Saved/VaultStage/Roar/SW_Monster.wav)
Standard library only. Measured first (6.36 s, stereo, 24 kHz): a strong onset at 0.2-0.9 s, a hissy middle, and the deepest, loudest growl at 4.4-5.5 s (lowest zero-crossing
rate with the most energy). Each roar is a window of the source, mixed to mono (it plays at the monster's position), deepened by resampling, given a little more low end, saturated
for density (louder at the same peak) and normalised; short fades keep the cut edges silent. The results are written next to the source for phase L to import. TUNABLE below.
"""
import array
import math
import wave
from pathlib import Path

STAGE = Path(__file__).resolve().parents[1] / "Saved" / "VaultStage" / "Roar"
# name: (start s, end s, pitch factor (<1 deeper and longer), low-end boost, drive, fade in s, fade out s)
ROARS = {
    "E_BossRoar": (4.35, 5.55, .84, .9, 2.2, .03, .30),    # a boss winding up: the big growl, deeper (about 1.4 s)
    "E_BossPhase": (3.85, 5.60, .78, 1.1, 2.6, .05, .45),  # the half-health roar: the swell into the growl, deepest and densest (about 2.2 s)
    "E_EnrageRoar": (0.15, 1.00, .95, .6, 1.8, .02, .22),  # an elite raging: the sharp onset, near natural pitch (about 0.9 s)
}
PEAK = 10 ** (-1.0 / 20)  # normalise to -1 dBFS


def read_mono(path):
    with wave.open(str(path), "rb") as w:
        channels, width, rate, frames = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        assert width == 2, "16-bit PCM expected"
        data = array.array("h", w.readframes(frames))
    mono = [sum(data[i:i + channels]) / (channels * 32768.0) for i in range(0, len(data), channels)]
    return mono, rate


def resample(x, factor):
    """Plays x at factor speed (linear interpolation): below 1 it is deeper and longer."""
    out_len = int(len(x) / factor)
    out = []
    for i in range(out_len):
        p = i * factor
        j = int(p)
        f = p - j
        a = x[j] if j < len(x) else 0.0
        b = x[j + 1] if j + 1 < len(x) else a
        out.append(a + (b - a) * f)
    return out


def low_boost(x, rate, amount, cutoff=180.0):
    """Adds amount x a one-pole low-pass of the signal (about 6 dB more weight below the cutoff at amount 1)."""
    k = 1.0 - math.exp(-2.0 * math.pi * cutoff / rate)
    y, lp = [], 0.0
    for s in x:
        lp += k * (s - lp)
        y.append(s + amount * lp)
    return y


def process(x, rate, start, end, pitch, boost, drive, fade_in, fade_out):
    x = x[int(start * rate):int(end * rate)]
    x = resample(x, pitch)
    x = low_boost(x, rate, boost)
    peak = max(abs(s) for s in x) or 1.0
    x = [math.tanh(drive * s / peak) / math.tanh(drive) for s in x]  # soft saturation: a denser roar, louder at the same peak
    n_in, n_out = int(fade_in * rate), int(fade_out * rate)
    for i in range(min(n_in, len(x))):
        x[i] *= i / n_in
    for i in range(min(n_out, len(x))):
        x[-1 - i] *= i / n_out
    peak = max(abs(s) for s in x) or 1.0
    return [s * PEAK / peak for s in x]


def write_mono(path, x, rate):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(array.array("h", (int(max(-1.0, min(1.0, s)) * 32767) for s in x)).tobytes())


def rms_db(x):
    return 20 * math.log10(max(1e-9, math.sqrt(sum(s * s for s in x) / max(1, len(x)))))


def main():
    source, rate = read_mono(STAGE / "SW_Monster.wav")
    print("source %.2f s at %d Hz, RMS %.1f dBFS" % (len(source) / rate, rate, rms_db(source)))
    for name, spec in ROARS.items():
        roar = process(source, rate, *spec)
        write_mono(STAGE / (name + ".wav"), roar, rate)
        print("%s: %.2f s, RMS %.1f dBFS (peak -1.0)" % (name, len(roar) / rate, rms_db(roar)))


if __name__ == "__main__":
    main()
