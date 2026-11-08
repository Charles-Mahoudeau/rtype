#!/usr/bin/env python3
"""Synthesizes the sounds of Haxball: kicks, bounces, the referee's whistle, the countdown beeps, the crowd's cheer and
the menu's click, and a looping background music. Everything is made from sines and noise, so the game owns its sounds.

Run it from anywhere (needs numpy): `python3 games/haxball/tools/make_sounds.py`. It overwrites assets/sounds/*.wav,
16-bit mono at 44.1 kHz. The same seed gives the same files.
"""

import wave
from pathlib import Path

import numpy as np

SOUNDS = Path(__file__).resolve().parent.parent / "assets" / "sounds"
RATE = 44100
rng = np.random.default_rng(7)


def times(seconds: float) -> np.ndarray:
    return np.arange(int(seconds * RATE)) / RATE


def sweep(start: float, end: float, seconds: float) -> np.ndarray:
    """A sine whose frequency glides from `start` to `end` (exponentially)."""
    t = times(seconds)
    frequency = start * (end / start) ** (t / seconds)
    return np.sin(2 * np.pi * np.cumsum(frequency) / RATE)


def bandpass(signal: np.ndarray, low: float, high: float) -> np.ndarray:
    """Keeps the frequencies between `low` and `high`, with soft edges (half an octave), through the spectrum."""
    spectrum = np.fft.rfft(signal)
    frequency = np.fft.rfftfreq(len(signal), 1 / RATE)
    octaves = np.log2(np.maximum(frequency, 1.0))
    below = np.clip((octaves - np.log2(low)) / 0.5 + 1, 0, 1)
    above = np.clip((np.log2(high) - octaves) / 0.5 + 1, 0, 1)
    return np.fft.irfft(spectrum * below * above, len(signal))


def envelope(length: int, attack: float, decay: float) -> np.ndarray:
    """Rises in `attack` seconds, then falls exponentially with the time constant `decay`."""
    t = np.arange(length) / RATE
    rise = np.clip(t / max(attack, 1e-6), 0, 1)
    return rise * np.exp(-np.maximum(t - attack, 0) / decay)


def fade(signal: np.ndarray, seconds: float = 0.01) -> np.ndarray:
    """Fades both ends, so no sound starts or stops with a click."""
    count = min(int(seconds * RATE), len(signal) // 2)
    ramp = np.linspace(0, 1, count)
    signal = signal.copy()
    signal[:count] *= ramp
    signal[-count:] *= ramp[::-1]
    return signal


def save(name: str, signal: np.ndarray, peak: float = 0.9, loop: bool = False) -> None:
    if not loop:  # a loop keeps its ends, so it joins itself without a gap or a dip
        signal = fade(signal)
    signal = signal / max(np.max(np.abs(signal)), 1e-9) * peak
    data = (signal * 32767).astype("<i2").tobytes()
    with wave.open(str(SOUNDS / name), "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(data)


def kick() -> np.ndarray:
    """A thump (a falling low sine) and the click of the foot on the ball."""
    seconds = 0.18
    body = sweep(180, 55, seconds) * envelope(int(seconds * RATE), 0.002, 0.05)
    click = bandpass(rng.standard_normal(int(seconds * RATE)), 1500, 6000) * envelope(int(seconds * RATE), 0.0005, 0.006)
    return body + 0.8 * click


def bounce() -> np.ndarray:
    """A shorter, higher knock: the ball on a board."""
    seconds = 0.12
    body = sweep(420, 180, seconds) * envelope(int(seconds * RATE), 0.001, 0.025)
    click = bandpass(rng.standard_normal(int(seconds * RATE)), 2000, 8000) * envelope(int(seconds * RATE), 0.0005, 0.004)
    return body + 0.5 * click


def whistle(seconds: float) -> np.ndarray:
    """A pea whistle: a high tone trilled by the pea, with breath."""
    t = times(seconds)
    trill = 1 + 0.035 * np.sin(2 * np.pi * 34 * t)
    tone = np.sin(2 * np.pi * np.cumsum(2950 * trill) / RATE)
    tone += 0.25 * np.sin(2 * 2 * np.pi * np.cumsum(2950 * trill) / RATE)
    breath = bandpass(rng.standard_normal(len(t)), 2500, 4500) * 0.05
    shape = np.clip(t / 0.03, 0, 1) * np.clip((seconds - t) / 0.06, 0, 1)
    return (tone * (0.8 + 0.2 * np.sin(2 * np.pi * 34 * t)) + breath) * shape


def beep(frequency: float, seconds: float) -> np.ndarray:
    t = times(seconds)
    return (np.sin(2 * np.pi * frequency * t) + 0.2 * np.sin(4 * np.pi * frequency * t)) * envelope(len(t), 0.004, seconds / 3)


def cheer() -> np.ndarray:
    """A crowd: many voices shouting at their own pitch, over the roar of the stands, swelling then dying down."""
    seconds = 3.2
    t = times(seconds)
    voices = np.zeros(len(t))
    for _ in range(60):
        pitch = rng.uniform(140, 420)
        start = rng.uniform(0, 0.5)
        length = rng.uniform(1.2, 2.6)
        vibrato = 1 + 0.03 * np.sin(2 * np.pi * rng.uniform(4, 7) * t + rng.uniform(0, 6))
        phase = 2 * np.pi * np.cumsum(pitch * vibrato) / RATE
        saw = sum(np.sin(harmonic * phase) / harmonic for harmonic in range(1, 9))
        shape = np.clip((t - start) / 0.15, 0, 1) * np.clip((start + length - t) / 0.6, 0, 1)
        voices += saw * shape * rng.uniform(0.5, 1)
    voices = bandpass(voices, 300, 2200)
    roar = bandpass(rng.standard_normal(len(t)), 250, 1600)
    roar *= np.std(voices) * 0.6 / np.std(roar)
    swell = np.clip(t / 0.25, 0, 1) * np.clip((seconds - t) / 1.6, 0, 1)
    return (voices + roar) * swell


def click() -> np.ndarray:
    seconds = 0.05
    return sweep(1800, 900, seconds) * envelope(int(seconds * RATE), 0.0005, 0.008)


def music() -> np.ndarray:
    """The background loop: 8 bars at 120 BPM in A minor (Am F C G, twice), a bass, a plucked arpeggio and a soft
    kick and hat. Every note's tail wraps around to the start, so the loop is seamless."""
    beat = 0.5
    bars = 8
    total = int(bars * 4 * beat * RATE)
    out = np.zeros(total)

    def put(signal: np.ndarray, start: float) -> None:
        begin = int(start * RATE)
        for index in range(0, len(signal), total):  # wraps past the end
            chunk = signal[index:index + total]
            positions = (begin + index + np.arange(len(chunk))) % total
            np.add.at(out, positions, chunk)

    def note(midi: int, seconds: float, decay: float, harmonics: float = 0.3) -> np.ndarray:
        t = times(seconds)
        frequency = 440 * 2 ** ((midi - 69) / 12)
        tone = np.sin(2 * np.pi * frequency * t) + harmonics * np.sin(4 * np.pi * frequency * t)
        return tone * envelope(len(t), 0.005, decay)

    chords = [(45, (57, 60, 64)), (41, (53, 57, 60)), (48, (60, 64, 67)), (43, (55, 59, 62))]  # bass, triad
    for bar in range(bars):
        bass, triad = chords[bar % 4]
        start = bar * 4 * beat
        for step in range(8):  # eighth notes
            at = start + step * beat / 2
            put(0.5 * note(bass, 0.4, 0.12, 0.15), at) if step % 2 == 0 else None
            pluck = triad[[0, 1, 2, 1, 0, 1, 2, 2][step]] + (12 if step % 4 == 3 else 0)
            put(0.22 * note(pluck + 12, 0.5, 0.1, 0.5), at)
        for kick_beat in (0, 2):
            thump = sweep(120, 45, 0.2) * envelope(int(0.2 * RATE), 0.002, 0.06)
            put(0.7 * thump, start + kick_beat * beat)
        for hat in range(8):
            noise = bandpass(rng.standard_normal(int(0.05 * RATE)), 6000, 12000)
            put(0.06 * noise * envelope(len(noise), 0.0005, 0.012), start + hat * beat / 2 + beat / 4)
    return out


def main() -> None:
    SOUNDS.mkdir(parents=True, exist_ok=True)
    save("kick.wav", kick())
    save("bounce.wav", bounce(), 0.7)
    save("whistle.wav", whistle(0.45), 0.7)
    save("whistle_long.wav", np.concatenate([whistle(0.3), np.zeros(int(0.12 * RATE)), whistle(0.3),
                                             np.zeros(int(0.12 * RATE)), whistle(0.9)]), 0.7)
    save("beep.wav", beep(880, 0.16), 0.5)
    save("cheer.wav", cheer(), 0.8)
    save("click.wav", click(), 0.5)
    save("music.wav", music(), 0.5, loop=True)


if __name__ == "__main__":
    main()
