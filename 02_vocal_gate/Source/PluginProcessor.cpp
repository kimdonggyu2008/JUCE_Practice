#include "PluginProcessor.h"
#include "PluginEditor.h"

// 생성자: "이 플러그인의 입출력이 어떻게 생겼는지"만 선언.
// 파라미터도, 내부 상태도 아직 없음.
LiveChorusAudioProcessor::LiveChorusAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
                      , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

LiveChorusAudioProcessor::~LiveChorusAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LiveChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "THRESHOLD", "Threshold", juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -40.0f ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "ATTACK", "Attack", juce::NormalisableRange<float> (1.0f, 1000.0f, 1.0f), 10.0f ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "RELEASE", "Release", juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f), 100.0f ));

    params.push_back (std::make_unique<juce::AudioParameterBool> ("BYPASS", "Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("REDUCTION","Reduction", juce::NormalisableRange<float> (0.0f, 80.0f, 0.1f), 60.0f ));

    return { params.begin(), params.end()};
}

// 재생이 시작되기 직전, 호스트가 샘플레이트/블록 크기를 알려주며 한 번 호출.
// 지금은 아무것도 준비할 게 없어서 비워둠.
void LiveChorusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    currentSampleRate = sampleRate;
}

void LiveChorusAudioProcessor::releaseResources() {}

// 이 플러그인이 어떤 채널 구성(모노/스테레오)을 지원하는지 호스트에게 알려주는 함수.
bool LiveChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

// 오디오 콜백. 지금은 아무 처리도 안 하고 입력을 그대로 통과시킴 (진짜 "빈 뼈대").
void LiveChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (buffer, midiMessages);

    if (apvts.getRawParameterValue("BYPASS")->load() > 0.5f)
        return;
    

    int num_buffers = buffer.getNumChannels();
    int samples_per_buffer = buffer.getNumSamples();
    float max_value = 0.0f;
    for (int channel = 0; channel < num_buffers; ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < samples_per_buffer; ++sample)
        {
            float sampleValue = channelData[sample];
            max_value = std::max(max_value, std::abs(sampleValue));
        }
    }
    currentLevel = max_value;
    currentLeveldB = juce::Decibels::gainToDecibels(currentLevel, -100.0f);
    float thresholddB = apvts.getRawParameterValue("THRESHOLD")->load();


    isAboveThreshold = currentLeveldB > thresholddB;

    float reduction_gain = apvts.getRawParameterValue("REDUCTION")->load();

    reduction_gain = juce::Decibels::decibelsToGain(-reduction_gain);

    float target = isAboveThreshold ? 1.0f : reduction_gain;

    float timeMs = (target > gateGain) ? apvts.getRawParameterValue("ATTACK")->load() : apvts.getRawParameterValue("RELEASE")->load();

    float timeInSeconds = timeMs / 1000.0f;
    float coefficient = std::exp(-(float) samples_per_buffer / (timeInSeconds * (float) currentSampleRate));

    gateGain = target + (gateGain - target) * coefficient;


    for (int channel =0 ; channel < num_buffers; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < samples_per_buffer; ++sample)
        {
            channelData[sample] *= gateGain;
        }
    }

    

    return;
}

juce::AudioProcessorEditor* LiveChorusAudioProcessor::createEditor()
{
    return new LiveChorusAudioProcessorEditor (*this);
}

void LiveChorusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LiveChorusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// JUCE가 플러그인 인스턴스를 만들 때 호출하는 진입점(entry point) 함수.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LiveChorusAudioProcessor();
}

