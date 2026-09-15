#include "PluginProcessor.h"
#include "PluginEditor.h"

LiveChorusAudioProcessorEditor::LiveChorusAudioProcessorEditor (LiveChorusAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 400);
    addAndMakeVisible(thresholdSlider);
    thresholdSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 80, 20);
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "THRESHOLD", thresholdSlider);

    addAndMakeVisible(thresholdLabel);
    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    thresholdLabel.attachToComponent(&thresholdSlider, false);

    addAndMakeVisible(AttackSlider);
    AttackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
    AttackSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 80, 20);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "ATTACK", AttackSlider);
    
    addAndMakeVisible(attackLabel);
    attackLabel.setText("Attack", juce::dontSendNotification);
    attackLabel.attachToComponent(&AttackSlider, false);

    addAndMakeVisible(ReleaseSlider);
    ReleaseSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
    ReleaseSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 80, 20);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "RELEASE", ReleaseSlider);

    addAndMakeVisible(releaseLabel);
    releaseLabel.setText("Release", juce::dontSendNotification);
    releaseLabel.attachToComponent(&ReleaseSlider, false);

    addAndMakeVisible(bypassButton);
    bypassButton.setButtonText("Bypass");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "BYPASS", bypassButton);

    addAndMakeVisible(reductionSlider);
    reductionSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
    reductionSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 80, 20);
    reductionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "REDUCTION", reductionSlider);

    addAndMakeVisible(reductionLabel);
    reductionLabel.setText("Reduction", juce::dontSendNotification);
    reductionLabel.attachToComponent(&reductionSlider, false);

    startTimerHz(30);

}


LiveChorusAudioProcessorEditor::~LiveChorusAudioProcessorEditor() {}

void LiveChorusAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("LiveChorus - M0", getLocalBounds(), juce::Justification::centred, 1);

    // 레벨 미터: 배경(트랙) 먼저 그리고, 그 위에 현재 레벨만큼 채워진 막대를 그림
    g.setColour (juce::Colours::black);
    g.fillRect (meterBounds);

    float levelDb = audioProcessor.currentLeveldB.load();
    // -60dB(거의 무음) ~ 0dB(최대) 범위를 0.0~1.0 비율로 변환
    float level01 = juce::jlimit (0.0f, 1.0f, juce::jmap (levelDb, -60.0f, 0.0f, 0.0f, 1.0f));

    auto meterFill = meterBounds.withWidth ((int) (meterBounds.getWidth() * level01));
    g.setColour (juce::Colours::limegreen);
    g.fillRect (meterFill);
}

void LiveChorusAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    meterBounds = area.removeFromTop (24);
    area.removeFromTop (10); // 미터랑 노브 사이 여백

    thresholdSlider.setBounds (area.removeFromLeft(area.getWidth() / 4));
    AttackSlider.setBounds (area.removeFromLeft(area.getWidth() / 3));
    ReleaseSlider.setBounds (area.removeFromLeft(area.getWidth() / 2));
    reductionSlider.setBounds (area);
    bypassButton.setBounds (getWidth() - 100, getHeight() - 40, 80, 30);
}

void LiveChorusAudioProcessorEditor::timerCallback()
{
    repaint();
}