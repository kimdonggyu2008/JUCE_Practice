juce_add_plugin(LiveChorus
    COMPANY_NAME "MyCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_STEP TRUE
    PLUGIN_CODE "LCho"
    FORMATS VST3
    PRODUCT_NAME "LiveChorus")

juce_generate_juce_header(LiveChorus)

target_sources(LiveChorus
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_definitions(LiveChorus
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MSVC)
    target_compile_options(LiveChorus PRIVATE /utf-8)
endif()

target_link_libraries(LiveChorus
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
