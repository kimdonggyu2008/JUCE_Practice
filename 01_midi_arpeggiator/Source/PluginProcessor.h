#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <vector>

// MIDI 이펙트: 오디오 신호는 안 건드리고, 들어온 MIDI 노트를 가공해서 내보냄.
// 지금은 딱 "선언"만 — 다음 단계에서 하나씩 채움.
class MidiArpeggiatorProcessor : public juce::AudioProcessor
{
public:
    MidiArpeggiatorProcessor();
    ~MidiArpeggiatorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MidiArpeggiator"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // UI가 화면에 그리려고 읽어가는 "지금 눌려있는 노트" 스냅샷.
    // 오디오 스레드가 쓰고 UI 스레드가 읽으므로 atomic이어야 한다 (아래 설명 참고).
    const std::array<std::atomic<bool>, 128>& getNoteIsHeldSnapshot() const { return noteIsHeld; }

private:
    // 눌린 순서가 유지되는 진짜 목록. 나중에 아르페지오 순서를 만들 때 이걸 쓴다.
    std::vector<int> heldNotes;
    std::array<juce::uint8, 128> velocityForNote {};

    // 위 목록을 UI가 안전하게 읽을 수 있게 복제해둔 것 (노트 번호 0~127 → 눌림 여부).
    std::array<std::atomic<bool>, 128> noteIsHeld {};

    double currentSampleRate = 44100.0;   // DAW가 알려주는 샘플레이트
    int samplesPerStep = 0;               // 한 박자의 길이 (샘플 수)
    int samplesSinceLastStep = 0;         // 지난 박자 이후 흐른 샘플 수
    int lastPlayedNote = -1;              // 지금 울리고 있는 노트 (-1 = 없음)

    int currentStepIndex = 0;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiArpeggiatorProcessor)
};
