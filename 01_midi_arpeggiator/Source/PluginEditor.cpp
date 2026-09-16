#include "PluginProcessor.h"
#include "PluginEditor.h"

MidiArpeggiatorEditor::MidiArpeggiatorEditor (MidiArpeggiatorProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);

    rateSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
    rateSlider.setTextValueSuffix (" ms");

    rateLabel.setText ("Rate", juce::dontSendNotification);
    rateLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (rateLabel);
    // 초당 30번 timerCallback()을 불러달라고 등록. 소멸자에서 자동으로 멈춘다.
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                     audioProcessor.apvts, "RATE", rateSlider); 
    addAndMakeVisible (rateSlider);

    modeBox.addItemList(juce::StringArray {"Up", "Down", "Up-Down"}, 1);
    addAndMakeVisible(modeBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "MODE", modeBox);
    
    startTimerHz (30);
}

MidiArpeggiatorEditor::~MidiArpeggiatorEditor() {}

// 초당 30번 호출됨. 프로세서의 스냅샷을 읽어서 문자열을 만들고,
// 내용이 바뀐 경우에만 다시 그리도록 요청한다.
void MidiArpeggiatorEditor::timerCallback()
{
    const auto& held = audioProcessor.getNoteIsHeldSnapshot();

    juce::StringArray names;

    for (int note = 0; note < 128; ++note)
        if (held[(size_t) note].load (std::memory_order_relaxed))
            names.add (juce::MidiMessage::getMidiNoteName (note, true, true, 3)
                         + " (" + juce::String (note) + ")");

    auto newText = names.isEmpty() ? juce::String ("(none)")
                                   : names.joinIntoString (", ");

    if (newText != heldNotesText)
    {
        heldNotesText = newText;
        repaint();   // "다시 그려달라"고 요청만 하는 것. paint()는 JUCE가 알아서 부른다.
    }
}

void MidiArpeggiatorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds().reduced (12);

    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawFittedText ("MIDI Arpeggiator", bounds.removeFromTop (34),
                      juce::Justification::centred, 1);

    g.setColour (juce::Colours::grey);
    g.setFont (13.0f);
    g.drawFittedText ("Held notes", bounds.removeFromTop (22),
                      juce::Justification::centred, 1);

    bounds.removeFromBottom (bottomAreaHeight);   // 아래 컨트롤 자리는 비워둔다

    g.setColour (juce::Colours::aqua);
    g.setFont (17.0f);
    g.drawFittedText (heldNotesText, bounds, juce::Justification::centred, 5);

    
}

void MidiArpeggiatorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (12);

    // 아래에서 100px를 떼어내고, 그 안을 다시 잘라 나눠 쓴다.
    auto bottom = bounds.removeFromBottom (bottomAreaHeight);

    modeBox.setBounds   (bottom.removeFromBottom (30));   // 맨 아래 30
    bottom.removeFromBottom (8);                          // 사이 여백
    rateLabel.setBounds (bottom.removeFromTop (20));      // 남은 것 중 위 20
    rateSlider.setBounds (bottom);                        // 나머지 전부
}
