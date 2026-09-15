#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiArpeggiatorProcessor)
};
