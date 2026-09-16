#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PitchDetector.h"

#include <atomic>
#include <memory>

// 오디오를 듣고 음정을 검출해 MIDI 노트로 내보낸다.
// 오디오 자체는 건드리지 않고 그대로 통과시킨다 — "듣기만 하는" 플러그인.
class PitchToMidiProcessor : public juce::AudioProcessor
{
public:
    PitchToMidiProcessor();
    ~PitchToMidiProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PitchToMidi"; }

    bool acceptsMidi() const override  { return false; }   // MIDI는 안 받는다
    bool producesMidi() const override { return true; }    // MIDI를 만들어낸다
    bool isMidiEffect() const override { return false; }   // 오디오 입력이 있으니 MIDI 이펙트가 아니다
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // UI가 읽어가는 최근 검출 결과. 오디오 스레드가 쓰고 UI 스레드가 읽으므로 atomic.
    float getDetectedFrequency() const  { return detectedHz.load (std::memory_order_relaxed); }
    float getDetectedConfidence() const { return detectedConfidence.load (std::memory_order_relaxed); }

    // 한 번에 분석할 샘플 수. 호스트 블록(보통 128~1024)과는 별개다.
    static constexpr int analysisWindowSize = 2048;

private:
    // 구체적인 방법(YIN / 나중의 신경망)은 모르고 인터페이스만 들고 있다.
    std::unique_ptr<PitchDetector> detector;

    std::atomic<float> detectedHz { 0.0f };
    std::atomic<float> detectedConfidence { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchToMidiProcessor)
};
