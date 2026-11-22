#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    juce::String percent (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }

    // The core "goo" waveshaper: biased (asymmetric) tanh, a sine wavefolder
    // that fades in with STANK, and an optional rectifier fuzz (SQUELCH).
    inline float goo (float x, float drive, float stank, bool squelch) noexcept
    {
        const float bias = stank * 0.7f;
        float y = std::tanh (x * drive + bias) - std::tanh (bias);

        const float foldGain = 1.0f + stank * 5.0f;
        const float folded   = std::sin (y * foldGain * juce::MathConstants<float>::halfPi);
        y += stank * stank * (folded - y);

        if (squelch)
        {
            const float r = std::abs (y) * 1.8f - 0.6f;   // full-wave rectify = octave-up nastiness
            y = 0.35f * y + 0.65f * std::tanh (r * 2.0f);
        }
        return y;
    }
}

MoistBoiProcessor::MoistBoiProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    moistureP = apvts.getRawParameterValue ("moisture");
    stankP    = apvts.getRawParameterValue ("stank");
    crustP    = apvts.getRawParameterValue ("crust");
    gunkP     = apvts.getRawParameterValue ("gunk");
    oozeP     = apvts.getRawParameterValue ("ooze");
    volumeP   = apvts.getRawParameterValue ("volume");
    squelchP  = apvts.getRawParameterValue ("squelch");
}

juce::AudioProcessorValueTreeState::ParameterLayout MoistBoiProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (percent);

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "moisture", 1 }, "Moisture",
                    NormalisableRange<float> (0.0f, 48.0f, 0.01f), 18.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "stank", 1 }, "Stank",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "crust", 1 }, "Crust",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gunk", 1 }, "Gunk",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "ooze", 1 }, "Ooze",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume",
                    NormalisableRange<float> (-24.0f, 12.0f, 0.01f), -6.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "squelch", 1 }, "Squelch", false));

    return { p.begin(), p.end() };
}

bool MoistBoiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void MoistBoiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);

    const int latency = (int) std::round (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    mixer.prepare (spec);
    mixer.reset();
    mixer.setWetLatency ((float) latency);

    outGain.reset (sampleRate, 0.05);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (volumeP->load()));

    lastDrive = juce::Decibels::decibelsToGain (moistureP->load());
    lastStank = stankP->load();

    for (auto& c : chans)
        c = {};
}

void MoistBoiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    for (int c = numIn; c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    const int numCh = juce::jmin (numIn, 2);
    if (numCh == 0 || numSamples == 0)
        return;

    const float driveTarget = juce::Decibels::decibelsToGain (moistureP->load());
    const float stankTarget = stankP->load();
    const float crust       = crustP->load();
    const float gunk        = gunkP->load();
    const bool  squelch     = squelchP->load() > 0.5f;
    outGain.setTargetValue (juce::Decibels::decibelsToGain (volumeP->load()));

    juce::dsp::AudioBlock<float> block (buffer);
    auto io = block.getSubsetChannelBlock (0, (size_t) numCh);

    mixer.setWetMixProportion (oozeP->load());
    mixer.pushDrySamples (io);

    // ---- Oversampled nonlinear stage ------------------------------------
    {
        auto up = oversampling.processSamplesUp (io);
        const int upN = (int) up.getNumSamples();
        const float d0 = lastDrive, s0 = lastStank;

        for (size_t c = 0; c < up.getNumChannels(); ++c)
        {
            float* x = up.getChannelPointer (c);
            for (int i = 0; i < upN; ++i)
            {
                const float t = (float) (i + 1) / (float) upN;   // linear ramp = zipper-free knob moves
                x[i] = goo (x[i], d0 + (driveTarget - d0) * t, s0 + (stankTarget - s0) * t, squelch);
            }
        }
        lastDrive = driveTarget;
        lastStank = stankTarget;

        oversampling.processSamplesDown (io);
    }

    // ---- Base-rate stage: CRUST (bitcrush + sample-hold), GUNK (lowpass), DC block
    const float holdLen = 1.0f + crust * crust * 31.0f;
    const float steps   = std::pow (2.0f, (16.0f - crust * 13.0f) - 1.0f);
    const float cutoff  = juce::jmin (20000.0f * std::pow (400.0f / 20000.0f, gunk), (float) sr * 0.45f);
    const float a       = std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sr);

    for (int c = 0; c < numCh; ++c)
    {
        auto& st = chans[(size_t) c];
        float* x = io.getChannelPointer ((size_t) c);

        for (int i = 0; i < numSamples; ++i)
        {
            float v = x[i];

            if (crust > 0.001f)
            {
                st.crushPhase += 1.0f;
                if (st.crushPhase >= holdLen)
                {
                    st.crushPhase -= holdLen;
                    st.crushHold = std::round (v * steps) / steps;
                }
                v = st.crushHold;
            }

            st.lp1 = v + a * (st.lp1 - v);
            st.lp2 = st.lp1 + a * (st.lp2 - st.lp1);
            v = st.lp2;

            const float dc = v - st.dcX + 0.995f * st.dcY;
            st.dcX = v;
            st.dcY = dc;
            x[i] = dc;
        }
    }

    io.multiplyBy (outGain);
    mixer.mixWetSamples (io);

    float peak = 0.0f;
    for (int c = 0; c < numCh; ++c)
        peak = juce::jmax (peak, buffer.getMagnitude (c, 0, numSamples));
    outputLevel.store (peak);
}

juce::AudioProcessorEditor* MoistBoiProcessor::createEditor()
{
    return new MoistBoiEditor (*this);
}

void MoistBoiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MoistBoiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoistBoiProcessor();
}
