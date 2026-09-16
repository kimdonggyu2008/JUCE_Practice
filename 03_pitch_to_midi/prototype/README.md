# prototype — 파이썬 참조 구현과 검증 도구

C++ 플러그인으로 옮기기 전에 알고리즘을 여기서 먼저 검증한다.

## 준비

```powershell
cd 03_pitch_to_midi/prototype
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

## 파일

| 파일 | 역할 |
| --- | --- |
| `signals.py` | 정답을 아는 테스트 신호(wav + 정답 csv)를 `out/`에 만든다 |
| `yin.py` | YIN 참조 구현. 단계별 중간 결과를 그래프로 확인하며 짠다 |
| `evaluate.py` | 검출 결과를 정답과 비교해 정확도 지표를 낸다 |
| `null_test.py` | 파이썬 결과와 C++ 결과가 같은지 확인한다 |

## 흐름

1. `signals.py` → 테스트 신호 생성
2. `yin.py` 작성 → `evaluate.py`로 정확도 확인
3. 같은 단계를 `../Source/YinPitchDetector.cpp`로 옮김
4. `PitchToMidiCli out/xxx.wav out/xxx_cpp.csv` 로 C++ 결과 생성
5. `null_test.py`로 두 결과 비교 → 통과해야 포팅 완료

프레임 규칙(`WINDOW_SIZE=2048`, `HOP_SIZE=512`)은 세 곳(파이썬·C++ 도구·정답 csv)이 같아야 한다.
