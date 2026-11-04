"""YJJFantasy 효과음·배경음 합성기. 외부 음원·라이브러리 없이 직접 만든다 (상업적 사용 자유).

실행 (엔진 Python):
    Engine/Binaries/ThirdParty/Python3/Win64/python.exe -I Tools/Audio/generate_audio.py Tools/Audio/Generated
결과: <출력 폴더>/*.wav (32kHz 16bit 모노). 임포트는 Tools/Content/build_content.py.

분위기: 맑은 종소리·하프·부드러운 패드·바람. 하늘섬의 몽글몽글한 판타지.
"""

import math
import os
import random
import struct
import sys
import wave

RATE = 32000
random.seed(20261104)
TAU = 2.0 * math.pi


# ---------------------------------------------------------------- 기본 재료

def silence(seconds):
    return [0.0] * int(seconds * RATE)


def mix(dst, src, start=0.0, gain=1.0):
    """dst에 src를 start초 위치부터 더한다 (필요하면 늘린다)"""
    offset = int(start * RATE)
    need = offset + len(src)
    if need > len(dst):
        dst.extend([0.0] * (need - len(dst)))
    for i, s in enumerate(src):
        dst[offset + i] += s * gain
    return dst


def note(name):
    names = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}
    pitch, octave = name[:-1], int(name[-1])
    return 440.0 * 2.0 ** ((names[pitch] + (octave - 4) * 12 - 9) / 12.0)


def bell(freq, length=1.2, volume=0.5, decay=0.5, bright=1.0):
    """종소리: 비정수배 배음이 빠르게 사라진다"""
    partials = [(1.0, 1.0, 1.0), (2.0, 0.5 * bright, 0.7), (2.76, 0.35 * bright, 0.5), (5.4, 0.18 * bright, 0.3), (8.93, 0.08 * bright, 0.2)]
    count = int(length * RATE)
    out = [0.0] * count
    for ratio, amp, dec in partials:
        f = freq * ratio
        if f > RATE * 0.45:
            continue
        w = TAU * f / RATE
        d = decay * dec
        for i in range(count):
            t = i / RATE
            out[i] += math.sin(w * i) * amp * math.exp(-t / d)
    attack = int(0.003 * RATE)
    for i in range(min(attack, count)):
        out[i] *= i / attack
    return [s * volume for s in out]


def harp(freq, length=1.6, volume=0.4):
    """하프: 부드러운 배음 + 긴 감쇠 (Karplus-Strong 대신 배음 합)"""
    count = int(length * RATE)
    out = [0.0] * count
    for k in range(1, 7):
        f = freq * k
        if f > RATE * 0.45:
            break
        amp = 1.0 / (k ** 1.6)
        w = TAU * f / RATE
        d = 0.9 / k ** 0.7
        for i in range(count):
            out[i] += math.sin(w * i) * amp * math.exp(-(i / RATE) / d)
    attack = int(0.004 * RATE)
    for i in range(min(attack, count)):
        out[i] *= i / attack
    return [s * volume for s in out]


def pad(freqs, length, volume=0.2, attack=0.8, release=1.2, detune=0.004):
    """패드: 살짝 어긋난 사인 여러 개 + 천천히 흔들리는 밝기"""
    count = int(length * RATE)
    out = [0.0] * count
    for f in freqs:
        for dt in (-detune, 0.0, detune):
            ff = f * (1.0 + dt)
            w = TAU * ff / RATE
            w2 = TAU * ff * 2.0 / RATE
            ph = random.random() * TAU
            for i in range(count):
                out[i] += math.sin(w * i + ph) * 0.6 + math.sin(w2 * i + ph) * 0.12
    norm = volume / (len(freqs) * 3)
    a = int(attack * RATE)
    r = int(release * RATE)
    for i in range(count):
        env = 1.0
        if i < a:
            env = i / a
        if i > count - r:
            env *= max(0.0, (count - i) / r)
        lfo = 0.85 + 0.15 * math.sin(TAU * 0.25 * i / RATE)
        out[i] *= env * norm * lfo
    return out


def noise(length, volume=0.5):
    return [random.uniform(-1.0, 1.0) * volume for _ in range(int(length * RATE))]


def lowpass(samples, cutoff):
    """한 극 저역 통과. cutoff는 숫자 또는 (i -> Hz) 함수"""
    out = []
    y = 0.0
    for i, x in enumerate(samples):
        c = cutoff(i) if callable(cutoff) else cutoff
        a = 1.0 - math.exp(-TAU * c / RATE)
        y += a * (x - y)
        out.append(y)
    return out


def bandpass(samples, low, high):
    lo = lowpass(samples, low)
    hi = lowpass(samples, high)
    return [h - l for h, l in zip(hi, lo)]


def envelope(samples, attack, decay):
    count = len(samples)
    a = max(1, int(attack * RATE))
    out = []
    for i, s in enumerate(samples):
        t = i / RATE
        env = min(i / a, 1.0) * math.exp(-max(0.0, t - attack) / decay)
        out.append(s * env)
    return out


def sweep(f0, f1, length, volume=0.5, curve=1.0, wobble=0.0):
    count = int(length * RATE)
    out = []
    phase = 0.0
    for i in range(count):
        a = (i / max(count - 1, 1)) ** curve
        f = f0 * (f1 / f0) ** a
        if wobble:
            f *= 1.0 + wobble * math.sin(TAU * 18.0 * i / RATE)
        phase += TAU * f / RATE
        out.append(math.sin(phase) * volume)
    return out


def reverb(samples, wet=0.3, room=0.82, tail=1.5):
    """간단한 Schroeder 잔향: 콤 필터 4개 + 올패스 2개"""
    src = samples + [0.0] * int(tail * RATE)
    combs = [1557, 1617, 1491, 1422]
    acc = [0.0] * len(src)
    for delay in combs:
        d = int(delay * RATE / 44100)
        buf = [0.0] * d
        idx = 0
        lp = 0.0
        for i, x in enumerate(src):
            y = buf[idx]
            lp = y * 0.7 + lp * 0.3
            buf[idx] = x + lp * room
            idx = (idx + 1) % d
            acc[i] += y
    for delay in (225, 556):
        d = int(delay * RATE / 44100)
        buf = [0.0] * d
        idx = 0
        out = []
        for x in acc:
            b = buf[idx]
            y = -x + b
            buf[idx] = x + b * 0.5
            idx = (idx + 1) % d
            out.append(y)
        acc = out
    return [s + a * wet * 0.25 for s, a in zip(src, acc)]


def normalize(samples, peak=0.85):
    m = max(1e-6, max(abs(s) for s in samples))
    return [s * peak / m for s in samples]


def fade_out(samples, seconds):
    n = int(seconds * RATE)
    count = len(samples)
    for i in range(max(0, count - n), count):
        samples[i] *= (count - i) / n
    return samples


def write(path, samples):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        frames = bytearray()
        for s in samples:
            v = max(-1.0, min(1.0, s))
            frames += struct.pack("<h", int(v * 32767))
        w.writeframes(bytes(frames))


# ---------------------------------------------------------------- 효과음

def sfx_stardust():
    out = bell(note("E6"), 0.9, 0.5, 0.35)
    mix(out, bell(note("B6"), 0.7, 0.3, 0.25), 0.05)
    mix(out, envelope(bandpass(noise(0.3, 0.4), 4000, 9000), 0.005, 0.08), 0.0)
    return normalize(reverb(out, 0.35, 0.75, 0.6), 0.7)


def sfx_lantern():
    out = silence(0.1)
    for i, n in enumerate(["C5", "E5", "G5", "C6", "E6"]):
        mix(out, bell(note(n), 2.2, 0.45, 0.9), i * 0.09)
    mix(out, pad([note("C4"), note("G4"), note("E5")], 2.6, 0.5, 0.15, 1.6))
    shimmer = bandpass(noise(2.2, 0.5), 5000, 11000)
    shimmer = [s * math.sin(math.pi * i / len(shimmer)) for i, s in enumerate(shimmer)]
    mix(out, shimmer, 0.2, 0.35)
    mix(out, sweep(120, 60, 0.6, 0.6), 0.0)
    return normalize(reverb(out, 0.5, 0.86, 1.8), 0.85)


def sfx_jump():
    n = bandpass(noise(0.22, 1.0), 400, 1800)
    n = envelope(n, 0.02, 0.07)
    return normalize(mix(n, envelope(sweep(260, 420, 0.12, 0.3), 0.005, 0.05)), 0.5)


def sfx_land():
    thump = envelope(sweep(110, 55, 0.18, 1.0), 0.002, 0.06)
    grit = envelope(lowpass(noise(0.12, 0.6), 1500), 0.001, 0.03)
    return normalize(mix(thump, grit), 0.6)


def sfx_glide():
    n = noise(0.7, 1.0)
    n = lowpass(n, lambda i: 300 + 2500 * math.sin(math.pi * min(i / (0.7 * RATE), 1.0)))
    n = envelope(n, 0.05, 0.25)
    flap = envelope(bandpass(noise(0.08, 1.0), 200, 1200), 0.002, 0.03)
    mix(n, flap, 0.0, 0.8)
    mix(n, flap, 0.07, 0.6)
    return normalize(n, 0.6)


def sfx_updraft():
    n = noise(1.4, 1.0)
    n = bandpass(n, 300, 0)  # placeholder, 아래서 덮어씀
    n = lowpass(noise(1.4, 1.0), lambda i: 400 + 3500 * (i / (1.4 * RATE)))
    n = [s * math.sin(math.pi * i / len(n)) for i, s in enumerate(n)]
    mix(n, sweep(300, 900, 1.2, 0.12), 0.1)
    return normalize(reverb(n, 0.3), 0.6)


def sfx_bounce():
    b = sweep(180, 620, 0.32, 0.8, 0.6, wobble=0.06)
    b = envelope(b, 0.005, 0.14)
    mix(b, bell(note("G5"), 0.4, 0.2, 0.2), 0.03)
    return normalize(b, 0.7)


def sfx_swing():
    n = bandpass(noise(0.22, 1.0), 900, 5000)
    n = [s * math.sin(math.pi * min(i / (0.18 * RATE), 1.0)) for i, s in enumerate(n)]
    return normalize(envelope(n, 0.01, 0.08), 0.55)


def sfx_slime_hit():
    b = sweep(420, 160, 0.16, 0.8, wobble=0.12)
    b = envelope(b, 0.002, 0.06)
    mix(b, envelope(lowpass(noise(0.1, 0.6), 2000), 0.001, 0.03))
    return normalize(b, 0.65)


def sfx_slime_pop():
    pop = envelope(sweep(700, 120, 0.12, 1.0), 0.001, 0.04)
    out = silence(0.05)
    mix(out, pop)
    for i, n in enumerate(["G6", "D7"]):
        mix(out, bell(note(n), 0.5, 0.25, 0.2), 0.06 + i * 0.06)
    return normalize(reverb(out, 0.3, 0.7, 0.5), 0.7)


def sfx_hurt():
    a = envelope(sweep(520, 260, 0.28, 0.7), 0.003, 0.12)
    b = envelope(sweep(390, 195, 0.28, 0.4), 0.003, 0.12)
    return normalize(mix(a, b), 0.6)


def sfx_respawn():
    out = silence(0.05)
    for i, n in enumerate(["C5", "G5", "C6", "G6"]):
        mix(out, bell(note(n), 1.0, 0.35, 0.5), i * 0.12)
    return normalize(reverb(out, 0.5, 0.82, 1.0), 0.6)


def sfx_shrine():
    out = pad([note("A2"), note("E3"), note("A3"), note("C#4"), note("E4")], 3.6, 0.6, 1.0, 1.8)
    for i, n in enumerate(["A4", "C#5", "E5", "A5"]):
        mix(out, bell(note(n), 2.0, 0.3, 0.9), 0.6 + i * 0.25)
    return normalize(reverb(out, 0.6, 0.88, 2.5), 0.8)


def sfx_finale():
    out = silence(0.1)
    chords = [["C4", "G4", "E5"], ["A3", "E4", "C5"], ["F3", "C4", "A4"], ["G3", "D4", "B4", "D5"], ["C4", "G4", "C5", "E5", "G5"]]
    for i, chord in enumerate(chords):
        length = 2.2 if i < 4 else 5.0
        mix(out, pad([note(n) for n in chord], length, 0.6, 0.4, 1.4), i * 1.6)
        for j, n in enumerate(chord):
            mix(out, bell(note(n) * 2, 2.0, 0.22, 0.8), i * 1.6 + j * 0.11)
    for i in range(16):
        n = ["C6", "E6", "G6", "C7"][i % 4]
        mix(out, bell(note(n), 1.0, 0.12, 0.5), 6.4 + i * 0.12)
    mix(out, sweep(80, 40, 2.0, 0.5), 0.0)
    return normalize(fade_out(reverb(out, 0.6, 0.88, 3.0), 2.0), 0.85)


def sfx_zone():
    out = silence(0.05)
    for i, n in enumerate(["E5", "G5", "D6"]):
        mix(out, harp(note(n), 1.8, 0.4), i * 0.16)
    return normalize(reverb(out, 0.5, 0.85, 1.5), 0.6)


# ---------------------------------------------------------------- 배경음

def bgm():
    """64초 반복: 패드 화음 진행 + 하프 아르페지오 + 종소리 멜로디 (C장조 5음계)"""
    bar = 4.0  # 한 마디 4초 (느긋한 왈츠 느낌으로 3박 대신 8분음표 아르페지오)
    progression = [
        ["C3", "G3", "E4", "B4"],   # Cmaj7
        ["A2", "E3", "C4", "G4"],   # Am7
        ["F2", "C3", "A3", "E4"],   # Fmaj7
        ["G2", "D3", "B3", "E4"],   # G6
    ]
    melody = [
        ["E5", None, "G5", "A5", None, "G5", "E5", None],
        ["C5", None, "D5", "E5", None, None, "G5", None],
        ["A5", None, "G5", "E5", None, "D5", "C5", None],
        ["D5", None, "E5", "G5", None, None, None, None],
    ]
    loops = 4
    length = bar * len(progression) * loops
    out = [0.0] * int(length * RATE)
    for loop in range(loops):
        for b, chord in enumerate(progression):
            t0 = (loop * len(progression) + b) * bar
            mix(out, pad([note(n) for n in chord], bar + 1.0, 0.5, 1.0, 1.4), t0)
            # 하프 아르페지오 (8분음표)
            arp = [chord[0], chord[1], chord[2], chord[3], chord[2], chord[1], chord[2], chord[3]]
            for k, n in enumerate(arp):
                f = note(n) * (2 if k % 4 == 3 else 1)
                mix(out, harp(f * 2, 1.4, 0.22), t0 + k * bar / 8)
            # 멜로디는 두 번째 바퀴부터
            if loop >= 1:
                line = melody[b] if loop != 2 else melody[(b + 2) % 4]
                for k, n in enumerate(line):
                    if n:
                        mix(out, bell(note(n), 1.6, 0.18, 0.7, 0.6), t0 + k * bar / 8)
    out = out[: int(length * RATE)]
    wet = reverb(out, 0.55, 0.86, 0.0)[: len(out)]
    # 반복 이음새: 끝 1.5초를 앞에 겹친다
    seam = int(1.5 * RATE)
    for i in range(seam):
        a = i / seam
        wet[i] = wet[i] * a + wet[len(wet) - seam + i] * (1 - a)
    wet = wet[: len(wet) - seam]
    return normalize(wet, 0.6)


def ambience_wind():
    """20초 바람 반복: 천천히 숨 쉬는 저역 노이즈"""
    length = 21.0
    n = noise(length, 1.0)
    n = lowpass(n, lambda i: 250 + 450 * (0.5 + 0.5 * math.sin(TAU * i / (RATE * 7.0))) + 250 * (0.5 + 0.5 * math.sin(TAU * i / (RATE * 3.1))))
    n = [s * (0.6 + 0.4 * math.sin(TAU * i / (RATE * 10.5))) for i, s in enumerate(n)]
    seam = int(1.0 * RATE)
    for i in range(seam):
        a = i / seam
        n[i] = n[i] * a + n[len(n) - seam + i] * (1 - a)
    n = n[: len(n) - seam]
    return normalize(n, 0.5)


SOUNDS = {
    "SFX_Stardust": sfx_stardust,
    "SFX_LanternLit": sfx_lantern,
    "SFX_Jump": sfx_jump,
    "SFX_Land": sfx_land,
    "SFX_GlideOpen": sfx_glide,
    "SFX_Updraft": sfx_updraft,
    "SFX_Bounce": sfx_bounce,
    "SFX_Swing": sfx_swing,
    "SFX_SlimeHit": sfx_slime_hit,
    "SFX_SlimePop": sfx_slime_pop,
    "SFX_Hurt": sfx_hurt,
    "SFX_Respawn": sfx_respawn,
    "SFX_ShrineAwake": sfx_shrine,
    "SFX_Finale": sfx_finale,
    "SFX_ZoneEnter": sfx_zone,
    "AMB_Wind": ambience_wind,
    "BGM_SkyIslands": bgm,
}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "Generated")
    os.makedirs(out_dir, exist_ok=True)
    only = set(sys.argv[2:])
    for name, fn in SOUNDS.items():
        if only and name not in only:
            continue
        path = os.path.join(out_dir, name + ".wav")
        write(path, fn())
        print("[Audio]", name, os.path.getsize(path) // 1024, "KB", flush=True)


if __name__ == "__main__":
    main()
