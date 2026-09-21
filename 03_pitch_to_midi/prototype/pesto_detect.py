"""신경망 음정 검출기 PESTO를 yin.py와 같은 모양으로 감싼 것.

yin.track()과 똑같은 형식 [(frame_start, hz, confidence), ...], 똑같은 프레임으로 결과를 돌려준다.
그래서 to_midi.py, 평가 코드, 나중의 C++ 널 테스트에 YIN 대신 그대로 끼울 수 있다.
(03번 플러그인의 PitchDetector 인터페이스를 파이썬 쪽에서 똑같이 맞추는 것)

YIN과 다른 점 세 가지 — 빈칸도 이 세 가지다:
  ① 프레임 간격을 샘플 수가 아니라 밀리초(ms)로 받는다             → 빈칸 1
  ② 프레임 번호가 같아도 가리키는 시각이 YIN과 다르다               → 빈칸 2
  ③ 음정이 없어도 항상 Hz를 내고, 대신 확신도를 따로 준다
     → "음정 없음"은 우리가 확신도 기준으로 정해야 한다              → 빈칸 3

────────────────────────────────────────────────────────────────────────────
실습: 빈칸(___) 3개. `___` 세 글자만 지우고 그 자리에 쓴다.
  채울 때마다:  .venv\\Scripts\\python.exe practice\\check_pesto.py
  다 채우면:    .venv\\Scripts\\python.exe compare_detectors.py   (YIN vs PESTO 비교표)
────────────────────────────────────────────────────────────────────────────
"""

import numpy as np
import pesto
import torch

from yin import HOP_SIZE, WINDOW_SIZE

CONFIDENCE_THRESHOLD = 0.5


def step_size_ms(sample_rate: float) -> float:
    """HOP_SIZE 샘플이 몇 밀리초인가. PESTO는 프레임 간격을 ms로 받는다.

      샘플 수 ÷ 초당 샘플 수 = 초,    초 × 1000 = 밀리초

      512 ÷ 48000 = 0.01067초   →  × 1000  →  10.67ms
      512 ÷ 16000 = 0.032초     →  × 1000  →  32ms

    01번 아르페지에이터에서 ms를 샘플로 바꿨던 것(rateMs → samplesPerStep)의 반대 방향이다.
    """
    # ┌─ 빈칸 1 ─────────────────────────────────────────────────────────────┐
    # │ HOP_SIZE와 sample_rate로 위 계산을 쓴다.                             │
    # │ 예: sample_rate가 48000이면 10.666...이 나와야 한다.                  │
    # └───────────────────────────────────────────────────────────────────────┘
    
    return (HOP_SIZE / sample_rate) * 1000


def frame_offset() -> int:
    """YIN 프레임 k와 같은 시각을 가리키는 PESTO 프레임은 k + 몇 번인가.

    두 검출기는 프레임 번호가 같아도 가리키는 시각이 다르다:
      YIN   프레임 k : 샘플 k×512부터 2048개를 보고, 가운데인 k×512 + 1024의 음정으로 친다
      PESTO 프레임 j : 샘플 j×512 근처의 음정

    같은 시각이 되려면    j × 512  =  k × 512 + 1024
                          j        =  k + 1024 ÷ 512
                          j        =  k + 2

    (확인한 방법: 48000번째 샘플에서 음정이 바뀌는 신호를 넣었더니
     PESTO는 94번 프레임, 즉 94 × 512 = 48128번째 샘플에서 바뀌었다고 답했다)
    """
    # ┌─ 빈칸 2 ─────────────────────────────────────────────────────────────┐
    # │ 위의 "1024 ÷ 512"를 WINDOW_SIZE와 HOP_SIZE로 쓴다.                   │
    # │   1024 = 창의 절반 = WINDOW_SIZE를 2로 나눈 것                       │
    # │   512  = HOP_SIZE                                                    │
    # │ 숫자 2를 직접 쓰지 말 것 — 창 크기를 바꿔도 맞게 나와야 한다.        │
    # │ 결과는 정수여야 한다. 파이썬에서 정수 나눗셈은 // (7 // 2 → 3)       │
    # └───────────────────────────────────────────────────────────────────────┘
    return (WINDOW_SIZE // 2) // HOP_SIZE


def is_voiced(confidence: float) -> bool:
    """확신도가 기준 이상이면 "음정 있음"으로 본다.

    YIN은 음정이 없으면 스스로 0을 돌려줬지만, PESTO는 무음이나 잡음에도 아무 Hz나 낸다.
    (무음을 넣으면 237Hz, 잡음을 넣으면 216Hz를 낸다. 대신 확신도가 0.11, 0.02로 낮다)
    """
    # ┌─ 빈칸 3 ─────────────────────────────────────────────────────────────┐
    # │ confidence가 CONFIDENCE_THRESHOLD "이상"이면 True, 아니면 False.      │
    # │ 비교식 자체가 True/False를 돌려준다.  예: 3 >= 2  →  True            │
    # └───────────────────────────────────────────────────────────────────────┘
    return (confidence >= CONFIDENCE_THRESHOLD)


def track(audio: np.ndarray, sample_rate: float) -> list[tuple[int, float, float]]:
    """파일 전체 → [(frame_start, frequency_hz, confidence), ...]  (yin.track과 같은 형식, 같은 프레임)"""
    x = torch.from_numpy(np.ascontiguousarray(audio, dtype=np.float32))     # numpy 배열 → PyTorch 텐서
    _, pitch, confidence, _ = pesto.predict(x, sample_rate, step_size=step_size_ms(sample_rate))
    pitch, confidence = pitch.numpy(), confidence.numpy()                     # 텐서 → numpy 배열

    results = []
    for k, frame_start in enumerate(range(0, len(audio) - WINDOW_SIZE + 1, HOP_SIZE)):   # YIN과 같은 프레임 순회
        j = k + frame_offset()                      # 같은 시각을 가리키는 PESTO 프레임
        hz = float(pitch[j]) if is_voiced(confidence[j]) else 0.0
        results.append((frame_start, hz, float(confidence[j])))
    return results
