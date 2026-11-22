#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

// HUSH-CHAN: a one-knob spectral gate. Each FFT bin that falls below the
// threshold gets pushed down, so noise, room tone and reverb tails vanish
// while the loud parts stay.
class ShizukaProcessor : public juce::AudioProcessor
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;   // 2048
    static constexpr int hop      = fftSize / 4;     // 75% overlap
    static constexpr int numBins  = fftSize / 2 + 1;
    static constexpr int numBands = 48;              // for the UI spectrum

    ShizukaProcessor();
    ~ShizukaProcessor() override = default;

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
    static float thresholdDbFor (float hush) noexcept { return -90.0f + 80.0f * hush; }

    juce::AudioProcessorValueTreeState apvts;

    // Values read by the editor
    std::array<std::atomic<float>, numBands> bands;   // input spectrum, dB
    std::atomic<float> gatedAmount { 0.0f };           // 0..1 share of energy removed
    std::atomic<float> outputLevel { 0.0f };

private:
    struct Channel
    {
        std::vector<float> in, out, gain;
        int idx = 0;
    };

    void processFrame (Channel& ch, float hush, bool analyse);

    std::atomic<float>* hushP = nullptr;

    juce::dsp::FFT fft { fftOrder };
    std::vector<float> window, fftData, rawGain, mags;
    std::vector<int> bandEdges;
    std::array<Channel, 2> chans;

    float magNorm = 1.0f, synthScale = 0.5f;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShizukaProcessor)
};
