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
    juce::Slider rateSlider;
    juce::Label rateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;

    // 아래쪽 컨트롤 영역의 높이. paint()와 resized()가 같은 값을 써야 겹치지 않는다.
    static constexpr int bottomAreaHeight = 178;

    juce::ComboBox modeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;


    juce::Slider gateSlider;
    juce::Label gateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateAttachment;

    juce::ToggleButton latchButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment;

    juce::ComboBox divisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;

    juce::ToggleButton syncButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiArpeggiatorEditor)
};
