#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

// MIDI 이펙트라 오디오 입출력 버스를 선언할 필요가 없음 — 기본 생성자만 호출.
// (LiveChorus 때는 .withInput/.withOutput으로 스테레오 버스를 선언했던 것과 비교해보기)
MidiArpeggiatorProcessor::MidiArpeggiatorProcessor()
{
}

MidiArpeggiatorProcessor::~MidiArpeggiatorProcessor() {}

void MidiArpeggiatorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate;
    samplesPerStep = (int) (sampleRate * 0.15);

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

// MIDI 콜백. 지금은 "눌려있는 노트 목록"을 관리하기만 하고, MIDI는 그대로 통과시킨다.
void MidiArpeggiatorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer);
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if(message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            heldNotes.push_back (noteNumber);
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
    juce::ignoreUnused (destData);
}

void MidiArpeggiatorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

// JUCE가 플러그인 인스턴스를 만들 때 호출하는 진입점(entry point) 함수.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiArpeggiatorProcessor();
}
