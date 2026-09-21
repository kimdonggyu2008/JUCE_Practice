"""YIN 음정 검출의 참조 구현 (de Cheveigné & Kawahara, 2002).

여기서 먼저 단계별로 검증하고, 같은 단계를 ../Source/YinPitchDetector.cpp로 옮긴다.
속도는 신경 쓰지 않는다 — 읽기 쉽고 맞는 것이 목표. 중간 결과를 그래프로 확인할 것.

프레임 규칙은 C++ 도구(PitchToMidiCli)와 반드시 같아야 한다:
  WINDOW_SIZE = 2048, HOP_SIZE = 512, frame_start = 프레임 시작 샘플 인덱스
"""

import numpy as np

WINDOW_SIZE = 2048
HOP_SIZE = 512


def difference_function(frame: np.ndarray, max_tau: int) -> np.ndarray:
    """1단계: d(τ) = Σ (x[j] - x[j+τ])²  — 자기 자신을 τ만큼 밀어서 얼마나 다른지.

    프레임(2048개)을 반으로 나눠 쓴다:
      - 비교할 구간 길이 W = len(frame) - max_tau  (= 1024)
      - 미는 거리 τ = 0 ~ max_tau-1                (= 0 ~ 1023)
    그래야 τ를 최대로 밀어도 프레임 밖으로 안 나간다.
    """
    W = len(frame) - max_tau
    a = frame[:W]                          # 기준 구간 (앞 1024개)

    d = np.zeros(max_tau)
    for tau in range(max_tau):
        b = frame[tau:tau + W]             # τ만큼 민 구간
        d[tau] = np.sum((a - b) ** 2)      # 차이를 제곱해서 모두 더함
    return d


def cumulative_mean_normalized(d: np.ndarray) -> np.ndarray:
    """2단계: d'(τ) = d(τ) / ((1/τ) Σ_{j=1..τ} d(j)),  d'(0) = 1
    τ가 작을 때 생기는 거짓 골짜기를 없앤다.

    말로 풀면: "지금 값 ÷ 지금까지 값들의 평균".
      - 1보다 작다 = 평균보다 잘 겹친다
      - 0에 가깝다 = 거의 완벽히 겹친다
    신호의 크기(볼륨)와 상관없이 0~1 근처로 비교할 수 있게 된다.
    """
    d_prime = np.ones_like(d)                       # d'(0) = 1

    running_sum = np.cumsum(d[1:])                  # Σ_{j=1..τ} d(j) 를 τ마다 누적
    tau = np.arange(1, len(d))                      # τ = 1, 2, 3, ...
    mean = running_sum / tau                        # 지금까지의 평균

    # 무음이면 d가 전부 0 → 평균도 0 → 0으로 나누기. 그땐 1(= 안 겹침)로 둔다.
    d_prime[1:] = np.where(mean > 0, d[1:] / np.where(mean > 0, mean, 1), 1.0)
    return d_prime


def absolute_threshold(d_prime: np.ndarray, threshold: float) -> int:
    """3단계: d'이 threshold 아래로 내려가는 첫 골짜기의 τ. 없으면 -1 (음정 없음).

    "가장 작은 곳"이 아니라 "처음 충분히 작아지는 곳"을 고른다.
    두 주기(436)도 한 주기(218)만큼 잘 겹치기 때문에, 가장 작은 곳을 고르면
    한 옥타브 아래로 틀릴 수 있다.
    """
    tau = 2                                         # d'(0), d'(1)은 정의상 항상 1이라 건너뜀
    while tau < len(d_prime):
        if d_prime[tau] < threshold:
            # 기준 아래로 들어왔으면, 계속 내려가는 동안 따라가서 골짜기 바닥을 찾는다
            while tau + 1 < len(d_prime) and d_prime[tau + 1] < d_prime[tau]:
                tau += 1
            return tau
        tau += 1
    return -1


def parabolic_interpolation(d_prime: np.ndarray, tau: int) -> float:
    """4단계: 이웃 세 점으로 포물선을 맞춰 τ를 소수점까지 다듬는다.

    τ는 정수(샘플 단위)라 220Hz의 실제 주기 218.18을 218로밖에 못 잡는다.
    218이면 48000/218 = 220.18Hz — 0.18Hz 오차. 고음일수록 이 오차가 커진다.
    바닥 근처 세 점(τ-1, τ, τ+1)을 지나는 포물선의 꼭짓점으로 진짜 바닥을 추정한다.
    """
    if tau <= 0 or tau >= len(d_prime) - 1:
        return float(tau)

    s0, s1, s2 = d_prime[tau - 1], d_prime[tau], d_prime[tau + 1]
    denominator = s0 - 2 * s1 + s2
    if denominator == 0:                            # 세 점이 일직선이면 포물선이 없음
        return float(tau)

    return tau + (s0 - s2) / (2 * denominator)


def detect(frame: np.ndarray, sample_rate: float, threshold: float = 0.1) -> tuple[float, float]:
    """5단계: 프레임 하나 → (frequency_hz, confidence). 음정이 없으면 (0.0, 0.0).
    confidence = 1 - d'(τ)."""
    max_tau = len(frame) // 2

    d = difference_function(frame, max_tau)         # 1단계
    d_prime = cumulative_mean_normalized(d)         # 2단계
    tau = absolute_threshold(d_prime, threshold)    # 3단계

    if tau < 0:
        return 0.0, 0.0                             # 기준 아래로 내려간 곳이 없음 = 음정 없음

    better_tau = parabolic_interpolation(d_prime, tau)   # 4단계
    confidence = float(np.clip(1.0 - d_prime[tau], 0.0, 1.0))

    return sample_rate / better_tau, confidence     # 5단계: 주기(샘플) → 주파수(Hz)


def track(audio: np.ndarray, sample_rate: float) -> list[tuple[int, float, float]]:
    """파일 전체를 프레임 단위로 분석 → [(frame_start, frequency_hz, confidence), ...]
    프레임 순회는 C++ 도구와 같다: start + WINDOW_SIZE <= 전체 길이 인 동안 HOP_SIZE씩."""
    results = []
    for frame_start in range(0, len(audio) - WINDOW_SIZE + 1, HOP_SIZE):
        frame = audio[frame_start:frame_start + WINDOW_SIZE]
        hz, confidence = detect(frame, sample_rate)
        results.append((frame_start, hz, confidence))
    return results


if __name__ == "__main__":
    # 사용: python yin.py input.wav output.csv
    # 출력 형식은 C++ 도구(PitchToMidiCli)와 같게 맞춰, null_test.py로 바로 비교할 수 있게 한다.
    import sys
    import soundfile as sf

    audio, sr = sf.read(sys.argv[1], dtype="float32")
    if audio.ndim > 1:
        audio = audio[:, 0]                         # C++ 도구처럼 왼쪽 채널만

    with open(sys.argv[2], "w", newline="", encoding="utf-8") as f:
        f.write("frame_start,time_sec,frequency_hz,confidence\n")
        for frame_start, hz, confidence in track(audio, sr):
            f.write(f"{frame_start},{frame_start / sr:.6f},{hz:.4f},{confidence:.4f}\n")

    print(f"wrote {sys.argv[2]}")
