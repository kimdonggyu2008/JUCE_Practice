cmake_minimum_required(VERSION 3.22)

project(JucePractice VERSION 0.0.1)

# JUCE를 루트에서 딱 한 번만 받아옴 — 하위 예제들은 이걸 공유해서 씀
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15 # 특정 버전에 고정 (master를 쓰면 예고 없이 빌드가 깨질 수 있음)
)
FetchContent_MakeAvailable(juce)

# AudioPluginHost (테스트용 DAW)
add_subdirectory(${juce_SOURCE_DIR}/extras/AudioPluginHost ${CMAKE_CURRENT_BINARY_DIR}/AudioPluginHost)

# ----------------------------------------------------------------------------
# 연습 예제들 — 새 예제를 추가할 땐 폴더 하나 만들고 아래에 한 줄 추가하면 됨
# ----------------------------------------------------------------------------
add_subdirectory(01_midi_arpeggiator)
add_subdirectory(02_vocal_gate)
add_subdirectory(03_pitch_to_midi)
