import math
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "audio"
SAMPLE_RATE = 44100


def write_wave(path, samples):
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "w") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        data = bytearray()
        for sample in samples:
            value = max(-1.0, min(1.0, sample))
            data += int(value * 32767).to_bytes(2, "little", signed=True)
        f.writeframes(bytes(data))


def tone(freq, duration, volume=0.35, decay=True):
    total = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total):
        t = i / SAMPLE_RATE
        env = 1.0
        if decay:
            env = max(0.0, 1.0 - i / total)
        samples.append(math.sin(2 * math.pi * freq * t) * volume * env)
    return samples


def chord(freqs, duration, volume=0.22):
    total = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total):
        t = i / SAMPLE_RATE
        env = 0.75 + 0.25 * math.sin(2 * math.pi * 2 * t)
        value = 0.0
        for freq in freqs:
            value += math.sin(2 * math.pi * freq * t)
        samples.append(value / len(freqs) * volume * env)
    return samples


def concat(parts):
    samples = []
    for part in parts:
        samples.extend(part)
    return samples


def pause(duration):
    return [0.0] * int(SAMPLE_RATE * duration)


def make_collision():
    total = int(SAMPLE_RATE * 0.35)
    samples = []
    seed = 1
    for i in range(total):
        seed = (seed * 1103515245 + 12345) & 0x7fffffff
        noise = (seed / 0x7fffffff) * 2 - 1
        env = max(0.0, 1.0 - i / total)
        samples.append(noise * 0.45 * env)
    return samples


def make_arrow_hit():
    total = int(SAMPLE_RATE * 0.18)
    samples = []
    seed = 7
    for i in range(total):
        t = i / SAMPLE_RATE
        seed = (seed * 1103515245 + 12345) & 0x7fffffff
        noise = (seed / 0x7fffffff) * 2 - 1
        env = max(0.0, 1.0 - i / total)
        whistle = math.sin(2 * math.pi * (980 - 480 * t) * t)
        samples.append((whistle * 0.28 + noise * 0.18) * env)
    return samples


def make_bgm():
    notes = [261.63, 329.63, 392.00, 523.25, 392.00, 329.63, 293.66, 349.23]
    parts = []
    for note in notes:
        parts.append(chord([note, note * 2], 0.28, 0.14))
        parts.append(pause(0.04))
    return concat(parts)


def main():
    write_wave(OUT / "bgm.wav", make_bgm())
    write_wave(OUT / "start.wav", concat([tone(440, 0.08), tone(660, 0.10)]))
    write_wave(OUT / "eat_food.wav", tone(640, 0.16))
    write_wave(OUT / "eat_bonus.wav", concat([tone(660, 0.08), tone(880, 0.12)]))
    write_wave(OUT / "eat_speed.wav", concat([tone(720, 0.06), tone(960, 0.06), tone(1200, 0.08)]))
    write_wave(OUT / "eat_slow.wav", concat([tone(420, 0.10), tone(280, 0.16)]))
    write_wave(OUT / "shield.wav", concat([tone(523, 0.08), tone(784, 0.16)]))
    write_wave(OUT / "trap.wav", concat([tone(180, 0.08), tone(120, 0.16)]))
    write_wave(OUT / "collision.wav", make_collision())
    write_wave(OUT / "bow.wav", concat([tone(520, 0.05), tone(740, 0.07), tone(980, 0.08)]))
    write_wave(OUT / "arrow_hit.wav", make_arrow_hit())
    print(f"Generated audio in {OUT}")


if __name__ == "__main__":
    main()
