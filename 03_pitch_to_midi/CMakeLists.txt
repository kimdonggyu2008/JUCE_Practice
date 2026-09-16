# ----------------------------------------------------------------------------
# 플러그인: 오디오를 듣고 음정을 검출해 MIDI 노트로 내보낸다 (오디오 입력 → MIDI 출력).
# ----------------------------------------------------------------------------
juce_add_plugin(PitchToMidi
    COMPANY_NAME "Practice"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_STEP TRUE
    PLUGIN_CODE "Ptm1"
    FORMATS VST3
    PRODUCT_NAME "PitchToMidi")

juce_generate_juce_header(PitchToMidi)

target_sources(PitchToMidi
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PitchDetector.h
        Source/YinPitchDetector.cpp
        Source/YinPitchDetector.h
)

target_compile_definitions(PitchToMidi
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MSVC)
    target_compile_options(PitchToMidi PRIVATE /utf-8)
endif()

target_link_libraries(PitchToMidi
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_audio_basics
        juce::juce_graphics
        juce::juce_data_structures
        juce::juce_events
        juce::juce_core
)

# ----------------------------------------------------------------------------
# 콘솔 도구: wav 파일을 프레임 단위로 분석해 CSV로 쓴다.
# 호스트 없이 같은 입력에 항상 같은 출력이 나오므로, 파이썬 참조 구현과
# 숫자로 비교(널 테스트)하고 정확도를 측정하는 데 쓴다.
# 플러그인과 "같은 검출기 코드"를 컴파일해서 쓰는 게 핵심이다.
# ----------------------------------------------------------------------------
juce_add_console_app(PitchToMidiCli
    PRODUCT_NAME "PitchToMidiCli")

target_sources(PitchToMidiCli
    PRIVATE
        Tools/PitchCli.cpp
        Source/PitchDetector.h
        Source/YinPitchDetector.cpp
        Source/YinPitchDetector.h
)

target_compile_definitions(PitchToMidiCli
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

if(MSVC)
    target_compile_options(PitchToMidiCli PRIVATE /utf-8)
endif()

target_link_libraries(PitchToMidiCli
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_core
)
