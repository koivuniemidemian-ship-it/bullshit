#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    inline float smoothstep (float a, float b, float x)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (x - a) / (b - a));
        return t * t * (3.0f - 2.0f * t);
    }

    // The Le Goo transfer curve: warm tanh -> fuzz -> wavefold + bitcrush, morphing with k (0..1).
    struct Shaper
    {
        float aW, tW, aF, aG, steps, wF, wG;

        explicit Shaper (float k)
        {
            aW = 1.0f + k * 14.0f;  tW = std::tanh (aW);
            aF = 1.0f + k * 70.0f;
            aG = 1.0f + k * 7.0f;
            steps = std::pow (2.0f, std::round (9.0f - k * 5.0f));
            wF = smoothstep (0.35f, 0.70f, k);
            wG = smoothstep (0.75f, 0.97f, k);
        }

        inline float operator() (float x) const noexcept
        {
            x = juce::jlimit (-1.0f, 1.0f, x);
            const float w = std::tanh (aW * x) / tW;
            const float f = (x >= 0.0f ? 1.0f : -1.0f) * (1.0f - std::exp (-std::abs (aF * x)));
            const float base = w * (1.0f - wF) + f * wF;
            if (wG <= 0.0f)
                return base;
            float g = std::sin (x * aG * juce::MathConstants<float>::halfPi);
            g = std::round (g * steps) / steps;
            return base * (1.0f - wG) + g * wG;
        }
    };
}

LeGooProcessor::LeGooProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    gooValue = apvts.getRawParameterValue ("goo");
    gooParam = apvts.getParameter ("goo");
}

juce::AudioProcessorValueTreeState::ParameterLayout LeGooProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "goo", 1 }, "Le Goo",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 35.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    return layout;
}

bool LeGooProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void LeGooProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const auto numCh = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, numCh };

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) samplesPerBlock);

    *highPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 35.0f);
    highPass.prepare (spec);

    lowPass.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPass.prepare (spec);
    lowPass.setCutoffFrequency (16000.0f);

    *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, 110.0f, 0.707f, 1.0f);
    lowShelf.prepare (spec);
    lastShelfDb = 0.0f;

    const auto latency = oversampling->getLatencyInSamples();
    dryWet.prepare (spec);
    dryWet.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    dryWet.setWetLatency (latency);
    setLatencySamples ((int) std::round (latency));

    kSmooth.reset (sampleRate, 0.05);
    kSmooth.setCurrentAndTargetValue (gooValue->load() / 100.0f);
    outGain.reset (sampleRate, 0.05);
    outGain.setCurrentAndTargetValue (1.0f);

    blockSeconds = samplesPerBlock / sampleRate;
}

float LeGooProcessor::rmsDb (const juce::AudioBuffer<float>& buffer)
{
    float sum = 0.0f;
    const int n = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* d = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i) sum += d[i] * d[i];
    }
    const float rms = std::sqrt (sum / (float) juce::jmax (1, n * buffer.getNumChannels()));
    return juce::Decibels::gainToDecibels (rms, -100.0f);
}

void LeGooProcessor::storeScope (const juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    const int chans = buffer.getNumChannels();
    int w = scopeWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < chans; ++ch) s += buffer.getSample (ch, i);
        scope[(size_t) w].store (s / (float) juce::jmax (1, chans), std::memory_order_relaxed);
        w = (w + 1) % scopeSize;
    }
    scopeWrite.store (w, std::memory_order_release);
}

void LeGooProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // ---- host transport: the bounce follows the session tempo exactly ----
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) hostBpm = *bpm;
            if (auto ppq = pos->getPpqPosition()) { hostPpq = *ppq; hostHasPpq = true; }
            else hostHasPpq = false;
            if (auto sig = pos->getTimeSignature()) timeSigNum = juce::jmax (1, sig->numerator);
            hostPlaying = pos->getIsPlaying();
        }
    }
    stampMs = juce::Time::getMillisecondCounterHiRes();
    blockSeconds = buffer.getNumSamples() / sr;

    inDb = rmsDb (buffer);

    const float target = gooValue->load() / 100.0f;
    kSmooth.setTargetValue (target);
    const float k = kSmooth.skip (buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);
    block = block.getSubsetChannelBlock (0, (size_t) numOut);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    const float wet = smoothstep (0.0f, 0.12f, k);
    dryWet.setWetMixProportion (wet);
    dryWet.pushDrySamples (block);

    highPass.process (ctx);

    auto up = oversampling->processSamplesUp (block);
    const Shaper shape (k);
    for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
    {
        float* d = up.getChannelPointer (ch);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
            d[i] = shape (d[i]);
    }
    oversampling->processSamplesDown (block);

    lowPass.setCutoffFrequency (16000.0f * std::pow (5000.0f / 16000.0f, k));
    lowPass.process (ctx);

    const float shelfDb = smoothstep (0.6f, 1.0f, k) * 6.0f;
    if (std::abs (shelfDb - lastShelfDb) > 0.05f)
    {
        *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            sr, 110.0f, 0.707f, juce::Decibels::decibelsToGain (shelfDb));
        lastShelfDb = shelfDb;
    }
    lowShelf.process (ctx);

    dryWet.mixWetSamples (block);

    outGain.setTargetValue (juce::Decibels::decibelsToGain (-1.0f - target * 11.0f));
    if (outGain.isSmoothing())
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float g = outGain.getNextValue();
            for (int ch = 0; ch < numOut; ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * g);
        }
    }
    else
    {
        buffer.applyGain (outGain.getTargetValue());
    }

    outDb = rmsDb (buffer);
    storeScope (buffer);
}

void LeGooProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LeGooProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* LeGooProcessor::createEditor()
{
    return new LeGooEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LeGooProcessor();
}
