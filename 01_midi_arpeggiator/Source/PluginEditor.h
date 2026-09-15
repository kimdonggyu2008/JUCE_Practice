#pragma once

#include "PluginProcessor.h"

// 지금은 빈 창만 띄우는 최소 에디터. 컨트롤은 파라미터가 생긴 뒤에 추가.
class MidiArpeggiatorEditor : public juce::AudioProcessorEditor
{
public:
    MidiArpeggiatorEditor (MidiArpeggiatorProcessor&);
    ~MidiArpeggiatorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MidiArpeggiatorProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiArpeggiatorEditor)
};
