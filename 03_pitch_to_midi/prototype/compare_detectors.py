"""YIN과 PESTO를 같은 신호, 같은 프레임, 같은 기준으로 비교한다.

    .venv\\Scripts\\python.exe compare_detectors.py

두 검출기가 같은 형식(track)으로 결과를 내기 때문에 비교 코드는 하나로 충분하다.
나중에 C++로 옮긴 PESTO도, 신경망을 바꿔 끼워도 이 표에 그대로 한 줄 더 붙이면 된다.
"""

import csv
import sys

import numpy as np
import soundfile as sf

import pesto_detect
import yin
from signals import OUT_DIR, SAMPLE_RATE, sine
from to_midi import hz_to_midi, note_name

sys.stdout.reconfigure(encoding="utf-8")

DETECTORS = {"YIN": yin.track, "PESTO": pesto_detect.track}
SIGNALS = ["sine_220", "sine_880", "harmonic_110", "vibrato_c4", "glide_110_440", "silence", "noise"]


def cents(estimate_hz: float, truth_hz: float) -> float:
    return 1200 * np.log2(estimate_hz / truth_hz)


def score(frames, truth: dict) -> str:
    """음정 있음/없음 판단 정확도, 평균 오차(cents), 50 cents 넘게 틀린 프레임 수."""
    voicing_right = sum((hz > 0) == (truth[s] > 0) for s, hz, _ in frames)
    errors = [cents(hz, truth[s]) for s, hz, _ in frames if hz > 0 and truth[s] > 0]
    gross = sum(abs(e) > 50 for e in errors)
    mean = f"{np.mean(np.abs(errors)):6.2f}" if errors else "     -"
    return f"있음/없음 {voicing_right:3d}/{len(frames):<3d}  평균오차 {mean}c  크게틀림 {gross:3d}"


if __name__ == "__main__":
    print("① 테스트 신호 (YIN이 파이썬 반복문이라 몇 초 걸립니다)\n")
    for name in SIGNALS:
        audio, sr = sf.read(OUT_DIR / f"{name}.wav", dtype="float32")
        with open(OUT_DIR / f"{name}_truth.csv", encoding="utf-8") as f:
            truth = {int(r["frame_start"]): float(r["frequency_hz"]) for r in csv.DictReader(f)}
        print(f"■ {name}")
        for label, track in DETECTORS.items():
            print(f"    {label:5s}  {score(track(audio, sr), truth)}")

    print("\n② 잡음 섞기 — 220Hz 사인 + 백색 잡음, 전체 프레임 중 맞은(±50 cents) 프레임 수\n")
    rng = np.random.default_rng(0)
    clean = sine(220, 2.0)
    print(f"    {'신호 대 잡음':>10} | " + " | ".join(f"{label:>6}" for label in DETECTORS))
    for snr_db in [20, 10, 5, 0, -5]:
        noise = rng.standard_normal(len(clean))
        noise *= np.sqrt(np.mean(clean ** 2) / 10 ** (snr_db / 10)) / np.sqrt(np.mean(noise ** 2))
        x = (clean + noise).astype(np.float32)
        counts = []
        for track in DETECTORS.values():
            frames = track(x, SAMPLE_RATE)
            counts.append(sum(hz > 0 and abs(cents(hz, 220)) < 50 for _, hz, _ in frames))
        print(f"    {snr_db:+8d}dB | " + " | ".join(f"{n:6d}" for n in counts) + f"   (전체 {len(frames)})")

    print("\n③ 화음 — 가운데 프레임이 어떤 음이라고 답하나\n")
    for label, x in [("C4 + G4", sine(261.63, 1) + sine(392.0, 1)), ("C4 + E4", sine(261.63, 1) + sine(329.63, 1))]:
        answers = []
        for det, track in DETECTORS.items():
            frames = track(x.astype(np.float32), SAMPLE_RATE)
            hz = frames[len(frames) // 2][1]
            answers.append(f"{det} {note_name(round(hz_to_midi(hz))) if hz > 0 else '-':4s}")
        print(f"    {label}:  " + "   ".join(answers))
