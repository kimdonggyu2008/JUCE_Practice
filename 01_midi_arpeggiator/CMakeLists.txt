juce_add_plugin(MidiArpeggiator
    COMPANY_NAME "Practice"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_STEP TRUE
    PLUGIN_CODE "Arp1"
    FORMATS VST3
    PRODUCT_NAME "MidiArpeggiator")

juce_generate_juce_header(MidiArpeggiator)

target_sources(MidiArpeggiator
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_definitions(MidiArpeggiator
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MSVC)
    target_compile_options(MidiArpeggiator PRIVATE /utf-8)
endif()

target_link_libraries(MidiArpeggiator
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
