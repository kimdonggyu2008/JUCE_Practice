"""정답(f0)을 아는 테스트 신호를 만든다.

검출기를 평가하려면 '맞는 답'이 있어야 한다. 실제 녹음은 정답을 모르므로
먼저 합성 신호로 검증하고, 그다음에 실제 음성·악기 녹음으로 넘어간다.

각 신호마다 wav와 정답 csv(frame_start, frequency_hz)를 out/ 아래에 쓴다.
정답 csv의 프레임 규칙(WINDOW_SIZE, HOP_SIZE)은 yin.py·C++ 도구와 같아야 한다.
"""

from pathlib import Path

import numpy as np

SAMPLE_RATE = 48000
WINDOW_SIZE = 2048
HOP_SIZE = 512
OUT_DIR = Path(__file__).parent / "out"


def sine(frequency_hz: float, seconds: float) -> np.ndarray:
    """순수 사인파. 가장 쉬운 경우 — 여기서 틀리면 구현이 틀린 것."""
    raise NotImplementedError


def harmonic_tone(frequency_hz: float, seconds: float, harmonic_gains: list[float]) -> np.ndarray:
    """배음이 섞인 톤. 기본음이 약하고 배음이 강하면 옥타브 오류가 잘 난다."""
    raise NotImplementedError


def glide(start_hz: float, end_hz: float, seconds: float) -> np.ndarray:
    """주파수가 연속으로 변하는 신호. 음정이 움직일 때 따라가는지 본다.

    주의: 순간 주파수를 선형으로 바꾸려면 위상을 적분해서 만들어야 한다
    (sin(2π·f(t)·t)로 만들면 실제 주파수가 의도와 달라진다).
    """
    raise NotImplementedError


def silence(seconds: float) -> np.ndarray:
    """무음. '음정 없음(0)'이라고 답해야 한다."""
    raise NotImplementedError


def white_noise(seconds: float) -> np.ndarray:
    """백색 잡음. 음정이 없는데 억지로 답하지 않는지 본다."""
    raise NotImplementedError


def ground_truth_frames(frequency_of_time, num_samples: int) -> list[tuple[int, float]]:
    """프레임마다 정답 주파수를 만든다. frequency_of_time(t_sec) -> Hz (없으면 0)."""
    raise NotImplementedError


if __name__ == "__main__":
    OUT_DIR.mkdir(exist_ok=True)
    # TODO: 위 신호들을 만들어 out/*.wav 와 out/*_truth.csv 로 저장
