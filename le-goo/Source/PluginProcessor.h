#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class LeGooProcessor : public juce::AudioProcessor
{
public:
    LeGooProcessor();
    ~LeGooProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Le Goo"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter* gooParam = nullptr;

    // ---- read by the editor (message thread) ----
    static constexpr int scopeSize = 2048;
    std::array<std::atomic<float>, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> inDb { -100.0f }, outDb { -100.0f };
    std::atomic<double> hostBpm { 120.0 }, hostPpq { 0.0 }, stampMs { 0.0 };
    std::atomic<bool> hostPlaying { false }, hostHasPpq { false };
    std::atomic<int> timeSigNum { 4 };
    std::atomic<double> blockSeconds { 0.0 };

private:
    std::atomic<float>* gooValue = nullptr;

    double sr = 44100.0;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highPass, lowShelf;
    juce::dsp::StateVariableTPTFilter<float> lowPass;
    juce::dsp::DryWetMixer<float> dryWet { 64 };
    juce::SmoothedValue<float> kSmooth, outGain;
    float lastShelfDb = -1000.0f;

    void storeScope (const juce::AudioBuffer<float>& buffer);
    static float rmsDb (const juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LeGooProcessor)
};
