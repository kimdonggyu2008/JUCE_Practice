#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "YinPitchDetector.h"

// 오디오 입력·출력 버스를 선언한다 (02_vocal_gate와 같은 방식). 01번처럼 MIDI만 다루는 플러그인과 다른 점.
PitchToMidiProcessor::PitchToMidiProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      detector (std::make_unique<YinPitchDetector>())
{
}

PitchToMidiProcessor::~PitchToMidiProcessor() {}

void PitchToMidiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    detector->prepare (sampleRate, analysisWindowSize);

    // TODO: 분석 창을 모아둘 버퍼를 여기서 미리 잡는다 (오디오 스레드 할당 금지).
    // TODO: detector->getLatencySamples()를 호스트에 알릴지 결정한다 (setLatencySamples).
    //       MIDI를 만드는 플러그인에서 지연 보상이 어떻게 동작하는지 먼저 확인할 것.
}

void PitchToMidiProcessor::releaseResources() {}

// 입력과 출력의 채널 구성이 같고, 모노나 스테레오면 받아준다.
bool PitchToMidiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void PitchToMidiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // 오디오는 건드리지 않으므로 입력이 그대로 출력으로 나간다.
    midiMessages.clear();

    // TODO(1) 블록 크기와 분석 창 크기가 다르다.
    //         들어온 샘플을 분석 창 버퍼에 이어 붙이고, 창이 찼을 때만 분석한다.
    //         (01번의 "블록은 시간을 들여다보는 창" 이야기와 같은 문제)
    // TODO(2) 창이 차면 detector->detect(...) 호출.
    // TODO(3) 결과를 detectedHz / detectedConfidence에 저장 (UI 표시용).
    // TODO(4) Hz → MIDI 노트 번호로 바꾸고, 음정이 바뀌면 note-off → note-on을 내보낸다.
    //         01번에서 배운 "노트를 잊기 전에 반드시 끈다"를 여기서도 지킬 것.
    juce::ignoreUnused (buffer);
}

juce::AudioProcessorEditor* PitchToMidiProcessor::createEditor()
{
    return new PitchToMidiEditor (*this);
}

void PitchToMidiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void PitchToMidiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchToMidiProcessor();
}
