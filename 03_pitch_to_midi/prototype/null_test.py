"""널 테스트: 파이썬 참조 구현과 C++ 구현의 출력이 같은지 숫자로 확인한다.

같은 wav를
  - yin.py로 분석한 결과
  - PitchToMidiCli(C++)로 분석한 결과
를 frame_start로 줄 맞춰 비교하고, 프레임별 차이의 최댓값을 보고한다.

float32(C++)와 float64(numpy) 차이 때문에 완전히 0은 아니다.
허용 오차를 정하고, 넘는 프레임이 있으면 그 프레임을 찍어 원인을 찾는다.
이 테스트를 통과해야 "파이썬에서 검증한 알고리즘을 C++로 정확히 옮겼다"고 말할 수 있다.
"""


def compare(python_csv: str, cpp_csv: str, tolerance_hz: float = 0.01) -> bool:
    raise NotImplementedError


if __name__ == "__main__":
    # TODO: 인자로 받은 두 csv를 비교해 통과/실패와 최대 차이 출력
    pass
