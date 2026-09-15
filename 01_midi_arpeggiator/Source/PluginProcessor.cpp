#include "PluginProcessor.h"
#include "PluginEditor.h"

// MIDI 이펙트라 오디오 입출력 버스를 선언할 필요가 없음 — 기본 생성자만 호출.
// (LiveChorus 때는 .withInput/.withOutput으로 스테레오 버스를 선언했던 것과 비교해보기)
MidiArpeggiatorProcessor::MidiArpeggiatorProcessor()
{
}

MidiArpeggiatorProcessor::~MidiArpeggiatorProcessor() {}

void MidiArpeggiatorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void MidiArpeggiatorProcessor::releaseResources() {}

bool MidiArpeggiatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}

// MIDI 콜백. 지금은 아무것도 안 하고 들어온 MIDI를 그대로 통과시킴 (진짜 "빈 뼈대").
void MidiArpeggiatorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer, midiMessages);
    // TODO: 다음 단계에서 여기부터 채워나간다.
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
