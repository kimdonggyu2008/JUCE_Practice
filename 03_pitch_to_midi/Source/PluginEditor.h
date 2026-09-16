#pragma once

#include "PluginProcessor.h"

// 최근 검출된 음정을 보여주는 화면. 01번과 같은 방식으로 타이머가 주기적으로 읽어간다.
class PitchToMidiEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit PitchToMidiEditor (PitchToMidiProcessor&);
    ~PitchToMidiEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PitchToMidiProcessor& audioProcessor;

    juce::String pitchText { "-" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchToMidiEditor)
};
