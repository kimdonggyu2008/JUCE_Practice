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

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "MODE", "Mode", juce::StringArray {"Up", "Down", "Up-Down"},0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "GATE", "Gate", juce::NormalisableRange<float> (0.05f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool> ("LATCH", "Latch", false));

    return { params.begin(), params.end()};
}

void MidiArpeggiatorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    heldNotes.reserve(128);

    latchedNotes.reserve(128);

    currentSampleRate = sampleRate;

    samplesSinceLastStep = 0;
    samplesUntilNoteOff = -1;
    lastPlayedNote = -1;
    needsAllNotesOff = true;
    heldNotes.clear();

    latchedNotes.clear();
}

void MidiArpeggiatorProcessor::releaseResources() {}

bool MidiArpeggiatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}

// 들어온 note-on/off로 노트 목록을 갱신한다. 원본 MIDI는 건드리지 않는다(const).
void MidiArpeggiatorProcessor::updateHeldNotes (const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if(message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();

            // 처음 누르는 순간 latch 화음 비우기
            if(heldNotes.empty())
                latchedNotes.clear();

            // 음 높이 순으로 유지되도록 제자리에 끼워 넣는다 (Up/Down 모드의 전제).
            auto pos = std::lower_bound (heldNotes.begin(), heldNotes.end(), noteNumber);
            heldNotes.insert (pos, noteNumber);

            //중복 안하고 래치 목록에 넣기
            auto lpos = std::lower_bound (latchedNotes.begin(), latchedNotes.end(), noteNumber);
            if (lpos == latchedNotes.end() || *lpos != noteNumber)
                latchedNotes.insert (lpos, noteNumber);

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
}

// 바이패스 중. 목록은 계속 갱신해야 해제했을 때 "유령 노트"가 안 생기고,
// 울리던 아르페지오 노트는 여기서 끄지 않으면 신디에서 영원히 울린다.
void MidiArpeggiatorProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer);

    updateHeldNotes (midiMessages);

    if (needsAllNotesOff)
    {
        midiMessages.addEvent(juce::MidiMessage::allNotesOff (1), 0);
        needsAllNotesOff = false;
    }

    if (lastPlayedNote >= 0)
    {
        midiMessages.addEvent (juce::MidiMessage::noteOff (1, lastPlayedNote), 0);
        lastPlayedNote = -1;
        samplesUntilNoteOff = -1;
    }

    // midiMessages.clear()를 부르지 않는다 — 바이패스는 "입력을 그대로 통과"라는 뜻이다.
}

// MIDI 콜백. 들어온 노트로 목록을 갱신하고, 원본은 버린 뒤 아르페지오를 새로 만들어 내보낸다.
void MidiArpeggiatorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer);

    updateHeldNotes (midiMessages);

    midiMessages.clear();

    if (needsAllNotesOff)
    {
        midiMessages.addEvent(juce::MidiMessage::allNotesOff (1), 0);
        needsAllNotesOff = false;
    }

    const float rateMs = apvts.getRawParameterValue("RATE")->load();
    const int mode = (int) apvts.getRawParameterValue("MODE")->load();

    const bool latch = apvts.getRawParameterValue ("LATCH")->load() > 0.5f;
    const auto& notes = latch ? latchedNotes : heldNotes;

    samplesPerStep = (int) (currentSampleRate * rateMs / 1000.0f);

    const int numSamples = buffer.getNumSamples();
    const float gate = apvts.getRawParameterValue("GATE")->load();
    const int gateSamples = juce::jmax (1,(int) (samplesPerStep*gate));

    // prepareToPlay 전에 불릴 일은 없지만, 0이면 아래 %에서 0으로 나누게 되므로 방어.
    if (samplesPerStep <= 0)
        return;
    
    // 예약된 note-off가 이번 블록 안에 들어오면 그 자리에서 끈다.
    if (lastPlayedNote >= 0 && samplesUntilNoteOff >= 0 && samplesUntilNoteOff < numSamples)
    {
        midiMessages.addEvent (juce::MidiMessage::noteOff (1, lastPlayedNote), samplesUntilNoteOff);
        lastPlayedNote = -1;
        samplesUntilNoteOff = -1;
    }

    if (samplesSinceLastStep + numSamples >= samplesPerStep)
    {
        // 박자 경계가 이 블록의 몇 번째 샘플에 걸리는지.
        const int offset = juce::jlimit (0, numSamples - 1, samplesPerStep - samplesSinceLastStep);

        // Gate가 1.0이라 아직 안 꺼졌으면 여기서 끈다 (안전망).
        if (lastPlayedNote >= 0)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, lastPlayedNote), offset);
            lastPlayedNote = -1;
            samplesUntilNoteOff = -1;
        }

        // 다음 차례 노트를 켠다. %는 "고르기 직전"의 크기로 계산해야 범위를 벗어나지 않는다.
        if (! notes.empty())
        {
            const int n = (int) notes.size();
            if (mode == 0)
            {
                currentStepIndex = (currentStepIndex+1)%n;
            }
            
            else if (mode ==1)
            {
                currentStepIndex = (currentStepIndex - 1 + n) % n;
            }
            else if (mode == 2)
            {
                if (currentStepIndex == 0)
                {
                    stepDirection = 1;
                }
                else if (currentStepIndex == n - 1)
                {
                    stepDirection = -1;
                }
                currentStepIndex += stepDirection;
            }
            currentStepIndex = juce::jlimit (0, n-1, currentStepIndex);
            lastPlayedNote = notes[(size_t) currentStepIndex];
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, lastPlayedNote, velocityForNote[(size_t) lastPlayedNote]), offset);

            // gateSamples 뒤에 끄도록 예약한다.
            samplesUntilNoteOff = offset + gateSamples;

            // 짧은 노트라 같은 블록 안에서 끝나면 바로 여기서 끈다.
            if (samplesUntilNoteOff < numSamples)
            {
                midiMessages.addEvent (juce::MidiMessage::noteOff (1, lastPlayedNote), samplesUntilNoteOff);
                lastPlayedNote = -1;
                samplesUntilNoteOff = -1;
            }
        }
    }

    // 예약이 남아 있으면 이번 블록만큼 당겨둔다 (다음 블록 기준으로 환산).
    if (samplesUntilNoteOff >= 0)
        samplesUntilNoteOff -= numSamples;

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
