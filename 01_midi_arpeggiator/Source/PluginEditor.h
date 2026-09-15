#pragma once

#include "PluginProcessor.h"

// 지금은 "눌려있는 노트"를 글자로 보여주기만 하는 디버그용 화면.
// juce::Timer를 상속하면 timerCallback()이 일정 간격으로 자동 호출된다.
class MidiArpeggiatorEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    MidiArpeggiatorEditor (MidiArpeggiatorProcessor&);
    ~MidiArpeggiatorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MidiArpeggiatorProcessor& audioProcessor;

    // 화면에 그릴 문자열. timerCallback이 갱신하고 paint가 읽는다 (둘 다 UI 스레드).
    juce::String heldNotesText { "(none)" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiArpeggiatorEditor)
};
