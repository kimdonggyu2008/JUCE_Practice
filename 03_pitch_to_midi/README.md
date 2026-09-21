# 03_pitch_to_midi — 오디오에서 음정을 검출해 MIDI로 내보내기

노래나 악기 소리를 듣고, 음정을 추정해서 MIDI 노트로 바꾼다. **오디오 입력 → MIDI 출력** 플러그인.

| 예제 | 입력 → 출력 |
| --- | --- |
| 01_midi_arpeggiator | MIDI → MIDI |
| 02_vocal_gate | 오디오 → 오디오 |
| **03_pitch_to_midi** | **오디오 → MIDI** |

앞의 둘은 정보를 가공하는 방향이지만, 이건 섞여 있는 소리에서 **음정이라는 의도를 되찾는** 역문제다.

## 목표

**같은 자리에 방법을 바꿔 끼워, 같은 실시간 조건에서 비교한다.**

```
PitchDetector (공통 인터페이스)
├── YinPitchDetector      ← 1단계: 고전 DSP. 파이썬에서 검증 → C++로 옮김 → 널 테스트
└── (신경망 검출기)        ← 2단계: 파이썬에서 학습 → 가중치를 내보내 C++ 추론 엔진으로 실행
```

두 방법은 C++로 옮기는 방식이 다르다.

| | 고전 DSP (YIN) | 학습된 모델 |
| --- | --- | --- |
| 파이썬의 역할 | 알고리즘 설계·검증 | 모델 학습 |
| C++로 옮기는 것 | 알고리즘을 직접 다시 구현 | 가중치를 내보내서 로드 |
| 검증 | 파이썬 vs C++ 출력 비교 | PyTorch vs C++ 추론 출력 비교 |

비교할 것: **정확도**(Gross Pitch Error, cents 오차) · **지연**(샘플) · **CPU**(한 코어 대비 %).

## 구조

```
03_pitch_to_midi/
├── Source/
│   ├── PitchDetector.h          공통 인터페이스
│   ├── YinPitchDetector.*       YIN 구현
│   ├── PluginProcessor.*        오디오 입력 → 검출 → MIDI 출력
│   └── PluginEditor.*           검출 결과 표시
├── Tools/
│   └── PitchCli.cpp             wav → csv 오프라인 분석 (널 테스트·정확도 측정용)
└── prototype/                   파이썬 참조 구현과 검증 도구 (prototype/README.md 참고)
```

플러그인과 콘솔 도구는 **같은 검출기 코드**를 컴파일해서 쓴다. 그래서 도구로 검증한 결과가 플러그인에도 그대로 성립한다.

## 진행 순서

1. [x] `prototype/signals.py` — 정답을 아는 테스트 신호 (사인·배음·글리산도·비브라토·무음·잡음)
2. [x] `prototype/yin.py` — YIN 참조 구현. 합성 신호에서 옥타브 오류 없음, 평균 오차 0.1 cents 이하 (글리산도 -6.5 cents는 정답 규칙 차이)
3. [x] `prototype/to_midi.py` — 음정 곡선 → MIDI 노트 (단순 반올림 vs 히스테리시스·최소 길이). `demo_to_midi.py`로 .mid와 재합성 wav 생성
4. [ ] `prototype/evaluate.py` — 정확도 지표
5. [x] `Source/YinPitchDetector.cpp` — C++로 옮기기 (핵심 다섯 줄은 파이썬 원본을 보고 직접 번역)
6. [x] `prototype/null_test.py` — 파이썬 vs C++ 비교 통과 (테스트 신호 7개 전부, 최대 차이 0.0001Hz 미만). 빌드부터 비교까지 명령 하나
7. [ ] `PluginProcessor` — 분석 창 버퍼, Hz → MIDI 노트, note-on/off
8. [ ] 실시간 측정 — 지연·CPU
9. [ ] (2단계) 신경망 검출기를 같은 인터페이스로 추가해 비교

## 빌드

```powershell
cmake --build build --target PitchToMidi_VST3 --config Debug   # 플러그인
cmake --build build --target PitchToMidiCli --config Debug     # 분석 도구
```

## 알려진 제약

- 오디오 이펙트 플러그인이 MIDI를 내보내는 동작은 **호스트마다 지원이 다르다.** AudioPluginHost에서는 동작하지만, DAW에 따라 MIDI 출력을 다른 트랙으로 보낼 수 없을 수 있다.
