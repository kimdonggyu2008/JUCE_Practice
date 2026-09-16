"""검출 결과를 정답과 비교해 정확도를 숫자로 낸다.

음정 검출에서 흔히 쓰는 지표:
  - Voicing error        : 음정이 있는데 없다고 했거나, 없는데 있다고 한 프레임 비율
  - Gross Pitch Error    : 음정 있는 프레임 중 50 cents(반음의 절반) 넘게 틀린 비율 — 대부분 옥타브 오류
  - Fine pitch error     : 크게 틀리지 않은 프레임들의 평균 오차(cents)

cents = 1200 · log2(f_est / f_true). 100 cents = 반음 하나.
Hz 차이가 아니라 cents로 재는 이유: 110Hz에서 2Hz 오차와 880Hz에서 2Hz 오차는 귀에 전혀 다르게 들린다.
"""


def cents_error(f_est: float, f_true: float) -> float:
    raise NotImplementedError


def evaluate(estimated_csv: str, truth_csv: str) -> dict:
    """frame_start로 줄을 맞춰 위 지표들을 계산해 dict로 돌려준다."""
    raise NotImplementedError


if __name__ == "__main__":
    # TODO: 인자로 받은 두 csv를 비교해 결과 출력
    pass
