#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

ShizukaProcessor::ShizukaProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    hushP = apvts.getRawParameterValue ("hush");

    for (auto& b : bands)
        b.store (-100.0f);

    // sqrt-Hann for analysis and synthesis: Hann^2 overlap-added at 75% sums to 2
    window.resize ((size_t) fftSize);
    float wsum = 0.0f;
    for (int n = 0; n < fftSize; ++n)
    {
        window[(size_t) n] = std::sqrt (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) n / (float) fftSize));
        wsum += window[(size_t) n];
    }
    magNorm = 2.0f / wsum;

    fftData.assign ((size_t) (2 * fftSize), 0.0f);
    rawGain.assign ((size_t) numBins, 1.0f);
    mags.assign ((size_t) numBins, 0.0f);

    // Measure the FFT round-trip gain so we don't depend on backend scaling conventions
    std::vector<float> test ((size_t) (2 * fftSize), 0.0f);
    std::copy (window.begin(), window.end(), test.begin());
    fft.performRealOnlyForwardTransform (test.data());
    fft.performRealOnlyInverseTransform (test.data());
    const float got = test[(size_t) fftSize / 2];
    const float ifftScale = std::abs (got) > 1.0e-9f ? window[(size_t) fftSize / 2] / got : 1.0f;
    synthScale = ifftScale * 0.5f;
}

juce::AudioProcessorValueTreeState::ParameterLayout ShizukaProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto attrs = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
    {
        if (v < 0.001f) return String ("off");
        return String (thresholdDbFor (v), 1) + " dB";
    });

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "hush", 1 }, "Hush",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, attrs));

    return { p.begin(), p.end() };
}

bool ShizukaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void ShizukaProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    setLatencySamples (fftSize);

    for (auto& c : chans)
    {
        c.in.assign ((size_t) fftSize, 0.0f);
        c.out.assign ((size_t) fftSize, 0.0f);
        c.gain.assign ((size_t) numBins, 1.0f);
        c.idx = 0;
    }

    bandEdges.assign ((size_t) numBands + 1, 1);
    for (int b = 0; b <= numBands; ++b)
    {
        const double f = 40.0 * std::pow (18000.0 / 40.0, (double) b / (double) numBands);
        int bin = juce::jlimit (1, numBins - 1, (int) std::round (f * fftSize / sr));
        if (b > 0 && bin <= bandEdges[(size_t) b - 1])
            bin = juce::jmin (numBins - 1, bandEdges[(size_t) b - 1] + 1);
        bandEdges[(size_t) b] = bin;
    }
}

void ShizukaProcessor::processFrame (Channel& ch, float hush, bool analyse)
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    for (int n = 0; n < fftSize; ++n)
        fftData[(size_t) n] = ch.in[(size_t) n] * window[(size_t) n];

    fft.performRealOnlyForwardTransform (fftData.data());

    const bool active  = hush > 0.001f;
    const float thresh = thresholdDbFor (hush);
    const float knee   = 6.0f;
    const float floorG = juce::Decibels::decibelsToGain (-80.0f * hush);

    for (int b = 0; b < numBins; ++b)
    {
        const float re = fftData[(size_t) (2 * b)], im = fftData[(size_t) (2 * b + 1)];
        const float mag = std::sqrt (re * re + im * im) * magNorm;
        mags[(size_t) b] = mag;

        float g = 1.0f;
        if (active)
        {
            const float db = 20.0f * std::log10 (mag + 1.0e-9f);
            const float t = juce::jlimit (0.0f, 1.0f, (db - (thresh - knee)) / knee);
            g = floorG + (1.0f - floorG) * t;
        }
        rawGain[(size_t) b] = g;
    }

    float inE = 0.0f, outE = 0.0f;
    for (int b = 0; b < numBins; ++b)
    {
        // Smooth across neighbouring bins and over time to cut down "musical noise"
        const float sm = 0.25f * rawGain[(size_t) juce::jmax (b - 1, 0)]
                       + 0.50f * rawGain[(size_t) b]
                       + 0.25f * rawGain[(size_t) juce::jmin (b + 1, numBins - 1)];
        float& gs = ch.gain[(size_t) b];
        gs += (sm - gs) * (sm > gs ? 0.7f : 0.2f);

        fftData[(size_t) (2 * b)]     *= gs;
        fftData[(size_t) (2 * b + 1)] *= gs;

        const float m = mags[(size_t) b];
        inE  += m * m;
        outE += m * m * gs * gs;
    }
    // Mirror onto the negative-frequency half in case the backend reads it
    for (int b = numBins; b < fftSize; ++b)
    {
        const float gs = ch.gain[(size_t) (fftSize - b)];
        fftData[(size_t) (2 * b)]     *= gs;
        fftData[(size_t) (2 * b + 1)] *= gs;
    }

    if (analyse)
    {
        for (int i = 0; i < numBands; ++i)
        {
            const int lo = bandEdges[(size_t) i];
            const int hi = juce::jmax (lo + 1, bandEdges[(size_t) i + 1]);
            float mx = 0.0f;
            for (int b = lo; b < hi && b < numBins; ++b)
                mx = juce::jmax (mx, mags[(size_t) b]);
            bands[(size_t) i].store (20.0f * std::log10 (mx + 1.0e-9f));
        }
        const float amount = inE > 1.0e-10f ? juce::jlimit (0.0f, 1.0f, 1.0f - outE / inE) : 0.0f;
        gatedAmount.store (gatedAmount.load() * 0.7f + amount * 0.3f);
    }

    fft.performRealOnlyInverseTransform (fftData.data());

    std::copy (ch.out.begin() + hop, ch.out.end(), ch.out.begin());
    std::fill (ch.out.end() - hop, ch.out.end(), 0.0f);
    for (int n = 0; n < fftSize; ++n)
        ch.out[(size_t) n] += fftData[(size_t) n] * window[(size_t) n] * synthScale;
}

void ShizukaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    for (int c = numIn; c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    const int numCh = juce::jmin (numIn, 2);
    const float hush = hushP->load();
    float peak = 0.0f;

    for (int c = 0; c < numCh; ++c)
    {
        auto& ch = chans[(size_t) c];
        float* d = buffer.getWritePointer (c);

        for (int i = 0; i < numSamples; ++i)
        {
            ch.in[(size_t) (fftSize - hop + ch.idx)] = d[i];
            d[i] = ch.out[(size_t) ch.idx];

            if (++ch.idx == hop)
            {
                processFrame (ch, hush, c == 0);
                std::copy (ch.in.begin() + hop, ch.in.end(), ch.in.begin());
                ch.idx = 0;
            }
        }
        peak = juce::jmax (peak, buffer.getMagnitude (c, 0, numSamples));
    }
    outputLevel.store (peak);
}

juce::AudioProcessorEditor* ShizukaProcessor::createEditor()
{
    return new ShizukaEditor (*this);
}

void ShizukaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ShizukaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShizukaProcessor();
}
