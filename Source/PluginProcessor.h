#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class MoistBoiProcessor : public juce::AudioProcessor
{
public:
    MoistBoiProcessor();
    ~MoistBoiProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Peak output level, read by the UI so the blob's mouth can flap.
    std::atomic<float> outputLevel { 0.0f };

private:
    struct ChannelState
    {
        float crushHold = 0.0f, crushPhase = 0.0f;
        float lp1 = 0.0f, lp2 = 0.0f;
        float dcX = 0.0f, dcY = 0.0f;
    };

    std::atomic<float>* moistureP = nullptr;
    std::atomic<float>* stankP    = nullptr;
    std::atomic<float>* crustP    = nullptr;
    std::atomic<float>* gunkP     = nullptr;
    std::atomic<float>* oozeP     = nullptr;
    std::atomic<float>* volumeP   = nullptr;
    std::atomic<float>* squelchP  = nullptr;

    // 2^2 = 4x oversampling around the nonlinear stage
    juce::dsp::Oversampling<float> oversampling { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };

    juce::dsp::DryWetMixer<float> mixer { 64 };
    juce::SmoothedValue<float> outGain;
    std::array<ChannelState, 2> chans;

    float lastDrive = 1.0f, lastStank = 0.0f;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoistBoiProcessor)
};
