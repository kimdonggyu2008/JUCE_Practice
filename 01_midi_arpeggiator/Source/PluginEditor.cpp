#include "PluginProcessor.h"
#include "PluginEditor.h"

MidiArpeggiatorEditor::MidiArpeggiatorEditor (MidiArpeggiatorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

MidiArpeggiatorEditor::~MidiArpeggiatorEditor() {}

void MidiArpeggiatorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("MIDI Arpeggiator - M0", getLocalBounds(), juce::Justification::centred, 1);
}

void MidiArpeggiatorEditor::resized()
{
}
