#include "PluginProcessor.h"
#include "PluginEditor.h"

PitchToMidiEditor::PitchToMidiEditor (PitchToMidiProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 240);
    startTimerHz (30);
}

PitchToMidiEditor::~PitchToMidiEditor() {}

void PitchToMidiEditor::timerCallback()
{
    const float hz = audioProcessor.getDetectedFrequency();
    const float confidence = audioProcessor.getDetectedConfidence();

    // TODO: Hz를 음 이름(예: A4)으로도 보여주기 — Hz → MIDI 노트 번호 변환을 만든 뒤에.
    auto newText = hz > 0.0f ? juce::String (hz, 1) + " Hz  (conf " + juce::String (confidence, 2) + ")"
                             : juce::String ("-");

    if (newText != pitchText)
    {
        pitchText = newText;
        repaint();
    }
}

void PitchToMidiEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds().reduced (12);

    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawFittedText ("Pitch to MIDI", bounds.removeFromTop (34), juce::Justification::centred, 1);

    g.setColour (juce::Colours::grey);
    g.setFont (13.0f);
    g.drawFittedText ("Detected pitch", bounds.removeFromTop (22), juce::Justification::centred, 1);

    g.setColour (juce::Colours::aqua);
    g.setFont (22.0f);
    g.drawFittedText (pitchText, bounds, juce::Justification::centred, 2);
}

void PitchToMidiEditor::resized()
{
}
