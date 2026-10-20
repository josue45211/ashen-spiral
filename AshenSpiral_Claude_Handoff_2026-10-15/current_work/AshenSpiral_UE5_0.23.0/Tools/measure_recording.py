"""0.24.2: measures a recording of the game's own audio output (as.dev.record) against the events in the run's log, so a sound can be checked for loudness in the mix
without listening to it.
    python Tools/measure_recording.py <recording.wav> <run log> [pattern ...]
Prints the loudness (RMS, dBFS) of every 100 ms of the recording, and, for every log line matching a pattern (default: the roar cues and the dev commands), the loudest
100 ms within the 1.5 s after it, against the median loudness of the whole recording (the game's ordinary level). Standard library only.
"""
import array
import math
import re
import statistics
import sys
import wave
from datetime import datetime

STAMP = re.compile(r"^\[(\d{4})\.(\d{2})\.(\d{2})-(\d{2})\.(\d{2})\.(\d{2}):(\d{3})\]")


def stamp(line):
    m = STAMP.match(line)
    if not m:
        return None
    y, mo, d, h, mi, s, ms = (int(g) for g in m.groups())
    return datetime(y, mo, d, h, mi, s, ms * 1000).timestamp()


def main():
    wav, log = sys.argv[1], sys.argv[2]
    patterns = sys.argv[3:] or [r"AshenCue: ", r"AshenDev: > as\.dev\.(elite|boss|wound|killall)"]
    with wave.open(wav, "rb") as w:
        channels, width, rate, frames = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(frames)
    if width == 2:
        data = array.array("h", raw)
        scale = 32768.0
    elif width == 4:
        data = array.array("i", raw)
        scale = 2147483648.0
    else:
        raise SystemExit("unsupported sample width %d" % width)
    mono = [sum(data[i:i + channels]) / (channels * scale) for i in range(0, len(data), channels)]
    window = int(rate * .1)
    levels = []
    for start in range(0, len(mono), window):
        seg = mono[start:start + window]
        rms = math.sqrt(sum(s * s for s in seg) / max(1, len(seg)))
        levels.append(20 * math.log10(max(rms, 1e-9)))
    median = statistics.median(levels)
    print("recording: %.1f s, %d Hz, %d ch; median %.1f dBFS, loudest %.1f dBFS" % (len(mono) / rate, rate, channels, median, max(levels)))
    lines = open(log, encoding="utf-8", errors="replace").read().splitlines()
    begun = next((stamp(l) for l in lines if "AshenDev: recording the game's audio" in l), None)
    if begun is None:
        raise SystemExit("no recording start in the log")
    for line in lines:
        if not any(re.search(p, line) for p in patterns):
            continue
        t = stamp(line)
        if t is None:
            continue
        offset = t - begun
        first = int(offset / .1)
        span = levels[max(0, first):max(0, first) + 15]
        if not span:
            continue
        loudest = max(span)
        text = line.split("LogTemp: Display: ")[-1]
        print("%6.1f s  peak-100ms %6.1f dBFS (%+5.1f dB over the median)  %s" % (offset, loudest, loudest - median, text[:110]))
    print("timeline (dBFS per 100 ms, '#' = 2 dB above -60):")
    for i in range(0, len(levels), 5):
        chunk = levels[i:i + 5]
        print("%6.1f s %s" % (i * .1, " ".join("%5.1f" % v for v in chunk)), "#" * max(0, int((max(chunk) + 60) / 2)))


if __name__ == "__main__":
    main()
