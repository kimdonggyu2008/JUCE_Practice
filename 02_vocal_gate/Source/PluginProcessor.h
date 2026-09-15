#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// 지금은 딱 "선언"만. 파라미터도, 신호 처리도 아직 없음 — 다음 단계에서 하나씩 채움.
class LiveChorusAudioProcessor : public juce::AudioProcessor
{
public:
    LiveChorusAudioProcessor();
    ~LiveChorusAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LiveChorus"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> currentLeveldB{ -100.0f };

private:
    // 지금 블록에서 가장 큰 샘플의 절댓값 (0.0 ~ 1.0 사이).
    // processBlock이 끝나도 값이 사라지면 안 되니까 지역 변수가 아니라 멤버 변수로 둠.
    float currentLevel = 0.0f;

    //float thresholddB = -40.0f;
    bool isAboveThreshold = false;

    double currentSampleRate = 44100;
    //float attackMs = 10.0f;
    //float releaseMs = 100.0f;
    float gateGain = 0.0f;



    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveChorusAudioProcessor)
};
