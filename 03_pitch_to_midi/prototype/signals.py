"""정답(f0)을 아는 테스트 신호를 만든다.

검출기를 평가하려면 '맞는 답'이 있어야 한다. 실제 녹음은 정답을 모르므로
먼저 합성 신호로 검증하고, 그다음에 실제 음성·악기 녹음으로 넘어간다.

각 신호마다 out/ 아래에
  - xxx.wav        : 32비트 float (16비트면 반올림 때문에 파이썬과 C++이 읽는 값이 달라질 수 있음)
  - xxx_truth.csv  : frame_start, frequency_hz (음정이 없으면 0)
를 쓴다.

프레임 규칙은 yin.py·C++ 도구(PitchToMidiCli)와 같아야 한다:
  - 프레임 = frame_start부터 WINDOW_SIZE개 샘플
  - 다음 프레임은 HOP_SIZE만큼 뒤
  - 프레임의 정답 = 창 "가운데" 시점의 주파수
"""

from pathlib import Path

import numpy as np
import soundfile as sf

SAMPLE_RATE = 48000
WINDOW_SIZE = 2048
HOP_SIZE = 512
OUT_DIR = Path(__file__).parent / "out"


def sine(frequency_hz: float, seconds: float) -> np.ndarray:
    """순수 사인파. 가장 쉬운 경우 — 여기서 틀리면 구현이 틀린 것."""
    num_samples = int(SAMPLE_RATE * seconds)          # 2초면 96000개
    t = np.arange(num_samples) / SAMPLE_RATE          # 각 샘플의 시각(초): 0, 1/48000, 2/48000, ...
    return 0.5 * np.sin(2 * np.pi * frequency_hz * t) # 진폭 0.5 (1.0이면 배음 더할 때 넘침)


def harmonic_tone(frequency_hz: float, seconds: float, harmonic_gains: list[float]) -> np.ndarray:
    """배음이 섞인 톤. 기본음이 약하고 배음이 강하면 옥타브 오류가 잘 난다.

    harmonic_gains[0]은 기본음(1배), [1]은 2배음, [2]는 3배음 ... 의 크기.
    예: 110Hz, [0.2, 1.0, 0.8, 0.5] → 110Hz는 약하고 220Hz가 제일 셈.
    """
    num_samples = int(SAMPLE_RATE * seconds)
    t = np.arange(num_samples) / SAMPLE_RATE

    x = np.zeros(num_samples)
    for k, gain in enumerate(harmonic_gains, start=1):     # k = 1, 2, 3, ...
        x += gain * np.sin(2 * np.pi * k * frequency_hz * t)  # k번째 배음 = 기본 주파수의 k배

    return 0.5 * x / np.max(np.abs(x))    # 가장 큰 값이 0.5가 되도록 맞춤


def glide(start_hz: float, end_hz: float, seconds: float) -> np.ndarray:
    """주파수가 연속으로 변하는 신호. 음정이 움직일 때 따라가는지 본다.

    주파수는 "위상이 변하는 속도"다. 주파수가 계속 변하면
    sin(2π·f·t)로는 실제 주파수가 의도와 달라지므로, 위상을 누적(cumsum)해서 만든다.
    """
    num_samples = int(SAMPLE_RATE * seconds)
    f = np.linspace(start_hz, end_hz, num_samples)      # 샘플마다의 순간 주파수: 110 ... 440
    phase = 2 * np.pi * np.cumsum(f) / SAMPLE_RATE      # 한 샘플마다 위상이 f/48000 바퀴씩 전진
    return 0.5 * np.sin(phase)


def vibrato(center_hz: float, depth_cents: float, rate_hz: float, seconds: float) -> np.ndarray:
    """비브라토 — 사람 목소리처럼 음정이 주기적으로 위아래로 흔들린다.

    depth_cents : 흔들리는 폭. 60이면 ±60 cents — MIDI 반올림 경계(±50)를 넘나든다.
    rate_hz     : 1초에 몇 번 흔들리나. 노래는 보통 5~7Hz.
    주파수가 계속 변하므로 glide와 같이 위상을 누적해서 만든다.
    """
    num_samples = int(SAMPLE_RATE * seconds)
    t = np.arange(num_samples) / SAMPLE_RATE
    f = center_hz * 2 ** (depth_cents / 1200 * np.sin(2 * np.pi * rate_hz * t))
    phase = 2 * np.pi * np.cumsum(f) / SAMPLE_RATE
    return 0.5 * np.sin(phase)


def silence(seconds: float) -> np.ndarray:
    """무음. '음정 없음(0)'이라고 답해야 한다."""
    return np.zeros(int(SAMPLE_RATE * seconds))


def white_noise(seconds: float) -> np.ndarray:
    """백색 잡음. 음정이 없는데 억지로 답하지 않는지 본다."""
    rng = np.random.default_rng(seed=0)    # 시드 고정 → 실행할 때마다 똑같은 잡음 (재현 가능)
    return rng.uniform(-0.3, 0.3, int(SAMPLE_RATE * seconds))


def ground_truth_frames(frequency_of_time, num_samples: int) -> list[tuple[int, float]]:
    """프레임마다 정답 주파수를 만든다.

    frequency_of_time : 시각(초)을 넣으면 그때의 정답 주파수(Hz)를 돌려주는 함수. 음정이 없으면 0.
    프레임 순회 방식은 C++ 도구의 `for (start = 0; start + windowSize <= numSamples; start += hop)`와 같다.
    """
    rows = []
    for frame_start in range(0, num_samples - WINDOW_SIZE + 1, HOP_SIZE):
        center_sec = (frame_start + WINDOW_SIZE / 2) / SAMPLE_RATE
        rows.append((frame_start, frequency_of_time(center_sec)))
    return rows


def save(name: str, audio: np.ndarray, frequency_of_time) -> None:
    """out/name.wav 와 out/name_truth.csv 를 쓴다."""
    sf.write(OUT_DIR / f"{name}.wav", audio.astype(np.float32), SAMPLE_RATE, subtype="FLOAT")

    with open(OUT_DIR / f"{name}_truth.csv", "w", newline="", encoding="utf-8") as f:
        f.write("frame_start,frequency_hz\n")
        for frame_start, hz in ground_truth_frames(frequency_of_time, len(audio)):
            f.write(f"{frame_start},{hz:.4f}\n")


if __name__ == "__main__":
    OUT_DIR.mkdir(exist_ok=True)

    # 세 번째 인자는 "시각 t(초) → 정답 주파수" 규칙. lambda는 이름 없는 한 줄짜리 함수.
    save("sine_220",      sine(220, 2.0),                                  lambda t: 220.0)
    save("sine_880",      sine(880, 2.0),                                  lambda t: 880.0)
    save("harmonic_110",  harmonic_tone(110, 2.0, [0.2, 1.0, 0.8, 0.5]),   lambda t: 110.0)
    save("glide_110_440", glide(110, 440, 3.0),                            lambda t: 110 + (440 - 110) * t / 3.0)
    save("vibrato_c4",    vibrato(261.63, 60, 5.5, 2.0),                   lambda t: 261.63 * 2 ** (60 / 1200 * np.sin(2 * np.pi * 5.5 * t)))
    save("silence",       silence(1.0),                                    lambda t: 0.0)
    save("noise",         white_noise(1.0),                                lambda t: 0.0)

    print(f"wrote test signals to {OUT_DIR}")
