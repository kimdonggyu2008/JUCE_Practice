#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

// MIDI 이펙트라 오디오 입출력 버스를 선언할 필요가 없음 (LiveChorus의 .withInput/.withOutput과 비교).
// apvts는 기본 생성자가 없으므로 반드시 초기화 리스트에서 만들어야 한다.
MidiArpeggiatorProcessor::MidiArpeggiatorProcessor()
    : apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

MidiArpeggiatorProcessor::~MidiArpeggiatorProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout MidiArpeggiatorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RATE", "Rate", juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f), 150.0f));

    return { params.begin(), params.end()};
}

void MidiArpeggiatorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    heldNotes.reserve(128);

    currentSampleRate = sampleRate;

    samplesSinceLastStep = 0;
    lastPlayedNote = -1;
    heldNotes.clear();
}

void MidiArpeggiatorProcessor::releaseResources() {}

bool MidiArpeggiatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}

// MIDI 콜백. 들어온 노트로 목록을 갱신하고, 원본은 버린 뒤 아르페지오를 새로 만들어 내보낸다.
void MidiArpeggiatorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer);
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if(message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            // 음 높이 순으로 유지되도록 제자리에 끼워 넣는다 (Up/Down 모드의 전제).
            auto pos = std::lower_bound (heldNotes.begin(), heldNotes.end(), noteNumber);
            heldNotes.insert (pos, noteNumber);

            velocityForNote[(size_t) noteNumber] = message.getVelocity();
            noteIsHeld[(size_t) noteNumber].store (true, std::memory_order_relaxed);
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            auto it = std::find(heldNotes.begin(), heldNotes.end(), noteNumber);

            if (it != heldNotes.end())
                heldNotes.erase(it);

            noteIsHeld[(size_t) noteNumber].store (false, std::memory_order_relaxed);
        }
    }

    midiMessages.clear();

    const float rateMs = apvts.getRawParameterValue("RATE")->load();
    samplesPerStep = (int) (currentSampleRate * rateMs / 1000.0f);

    const int numSamples = buffer.getNumSamples();

    // prepareToPlay 전에 불릴 일은 없지만, 0이면 아래 %에서 0으로 나누게 되므로 방어.
    if (samplesPerStep <= 0)
        return;

    if (samplesSinceLastStep + numSamples >= samplesPerStep)
    {
        // 박자 경계가 이 블록의 몇 번째 샘플에 걸리는지.
        const int offset = juce::jlimit (0, numSamples - 1, samplesPerStep - samplesSinceLastStep);

        // 울리던 노트를 먼저 끈다.
        if (lastPlayedNote >= 0)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, lastPlayedNote), offset);
            lastPlayedNote = -1;
        }

        // 다음 차례 노트를 켠다. %는 "고르기 직전"의 크기로 계산해야 범위를 벗어나지 않는다.
        if (! heldNotes.empty())
        {
            currentStepIndex = (currentStepIndex + 1) % (int) heldNotes.size();
            lastPlayedNote = heldNotes[(size_t) currentStepIndex];
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, lastPlayedNote, velocityForNote[(size_t) lastPlayedNote]), offset);
        }
    }

    // 시간은 조건과 무관하게 매 블록 흐른다 — 반드시 if 바깥.
    samplesSinceLastStep = (samplesSinceLastStep + numSamples) % samplesPerStep;
}

juce::AudioProcessorEditor* MidiArpeggiatorProcessor::createEditor()
{
    return new MidiArpeggiatorEditor (*this);
}

void MidiArpeggiatorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MidiArpeggiatorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// JUCE가 플러그인 인스턴스를 만들 때 호출하는 진입점(entry point) 함수.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiArpeggiatorProcessor();
}
