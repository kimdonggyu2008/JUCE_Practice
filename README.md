# JUCE Practice

JUCE/C++로 오디오 플러그인을 하나씩 만들어보면서 익히는 연습용 저장소입니다. 예제마다 폴더를 분리해서, 각 폴더가 독립적으로 빌드되는 하나의 플러그인입니다.

## 구조

```
juce_practice/
├── CMakeLists.txt          # 루트: JUCE를 한 번만 받아오고, 각 예제 폴더를 추가
├── 01_midi_arpeggiator/    # MIDI 노트를 순서대로 쪼개서 내보내는 아르페지에이터
│   ├── CMakeLists.txt
│   └── Source/
└── ...
```

새 예제는 폴더 하나 만들고, 루트 `CMakeLists.txt`에 `add_subdirectory(폴더이름)` 한 줄만 추가하면 됩니다. JUCE 소스는 루트에서 한 번만 받아오므로 예제마다 중복으로 받지 않습니다.

## 빌드

```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

빌드 결과물(VST3)은 `build/<예제이름>_artefacts/Debug/VST3/`에 생깁니다. 테스트는 같이 빌드되는 `AudioPluginHost.exe`(`build/AudioPluginHost/AudioPluginHost_artefacts/Debug/`)로 합니다.

## 예제 목록

| 폴더 | 내용 | 배우는 핵심 |
| --- | --- | --- |
| `01_midi_arpeggiator` | 들어온 MIDI 노트를 순서대로 재생하는 아르페지에이터 | `MidiBuffer`, note-on/off 이벤트, MIDI 이펙트 플러그인 구조 |
| `02_vocal_gate` | 보컬 트랙의 조용한 구간/노이즈를 자동으로 줄여주는 노이즈 게이트 | dB 변환, envelope follower(attack/release smoothing), `AudioProcessorValueTreeState`(파라미터·상태 저장), 커스텀 UI(슬라이더/버튼/레벨 미터) |
