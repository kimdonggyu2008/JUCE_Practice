#pragma once

#include "PluginProcessor.h"

// 지금은 빈 창만 띄우는 최소 에디터. 컨트롤(슬라이더 등)은 파라미터가 생긴 뒤에 추가.
class LiveChorusAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    LiveChorusAudioProcessorEditor (LiveChorusAudioProcessor&);
    ~LiveChorusAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    LiveChorusAudioProcessor& audioProcessor;

    juce::Slider thresholdSlider;
    juce::Label thresholdLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;

    juce::Slider AttackSlider;
    juce::Label attackLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;

    juce::Slider ReleaseSlider;
    juce::Label releaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    juce::Slider reductionSlider;
    juce::Label reductionLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reductionAttachment;

    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // 레벨 미터를 그릴 영역. resized()에서 위치를 정하고, paint()에서 그때 정해진 영역에 그림.
    juce::Rectangle<int> meterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveChorusAudioProcessorEditor)
};
