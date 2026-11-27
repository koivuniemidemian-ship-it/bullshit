#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    constexpr int designW = 480;   // size the HTML is laid out at; the page scales itself to the window
    constexpr int designH = 1107;
}

juce::WebBrowserComponent::Options LeGooEditor::makeOptions()
{
    return juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const juce::String& url) { return getResource (url); })
        .withEventListener ("knob", [this] (const juce::var& v)
        {
            auto* p = proc.gooParam;
            if (p == nullptr) return;
            const float value = juce::jlimit (0.0f, 100.0f, (float) (double) v.getProperty ("value", 35.0));
            const bool wrap = ! gestureOpen;
            if (wrap) p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            if (wrap) p->endChangeGesture();
        })
        .withEventListener ("gesture", [this] (const juce::var& v)
        {
            auto* p = proc.gooParam;
            if (p == nullptr) return;
            const bool begin = (bool) v.getProperty ("begin", false);
            if (begin && ! gestureOpen)      { p->beginChangeGesture(); gestureOpen = true; }
            else if (! begin && gestureOpen) { p->endChangeGesture();   gestureOpen = false; }
        });
}

LeGooEditor::LeGooEditor (LeGooProcessor& p)
    : AudioProcessorEditor (&p), proc (p), browser (makeOptions())
{
    addAndMakeVisible (browser);
    browser.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, true);
    setResizeLimits (designW * 55 / 100, designH * 55 / 100, designW * 13 / 10, designH * 13 / 10);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) designW / (double) designH);
    setSize (designW * 78 / 100, designH * 78 / 100);

    startTimerHz (60);
}

LeGooEditor::~LeGooEditor()
{
    stopTimer();
    if (gestureOpen && proc.gooParam != nullptr)
        proc.gooParam->endChangeGesture();
}

void LeGooEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0c0a0e));
}

void LeGooEditor::resized()
{
    browser.setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> LeGooEditor::getResource (const juce::String& url)
{
    const auto path = url.fromLastOccurrenceOf ("/", false, false);
    if (path.isEmpty() || path == "index.html")
    {
        std::vector<std::byte> bytes ((size_t) BinaryData::index_htmlSize);
        std::memcpy (bytes.data(), BinaryData::index_html, bytes.size());
        return juce::WebBrowserComponent::Resource { std::move (bytes), "text/html" };
    }
    return std::nullopt;
}

void LeGooEditor::timerCallback()
{
    // latest samples from the processor's ring buffer, oldest first
    constexpr int N = LeGooProcessor::scopeSize;
    std::array<float, N> latest {};
    const int w = proc.scopeWrite.load (std::memory_order_acquire);
    for (int i = 0; i < N; ++i)
        latest[(size_t) i] = proc.scope[(size_t) ((w + i) % N)].load (std::memory_order_relaxed);

    juce::Array<juce::var> wave;
    for (int i = 0; i < 128; ++i)
        wave.add (latest[(size_t) (N - 512 + i * 4)]);

    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (latest.begin(), latest.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) N);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const double sr = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
    juce::Array<juce::var> spec;
    for (int b = 0; b < (int) bands.size(); ++b)
    {
        const double f0 = 30.0 * std::pow (16000.0 / 30.0, b / 40.0);
        const double f1 = 30.0 * std::pow (16000.0 / 30.0, (b + 1) / 40.0);
        const int i0 = juce::jlimit (1, N / 2 - 1, (int) (f0 / sr * N));
        const int i1 = juce::jlimit (i0 + 1, N / 2, (int) (f1 / sr * N));
        float m = 0.0f;
        for (int i = i0; i < i1; ++i) m = juce::jmax (m, fftData[(size_t) i]);
        const float db = juce::Decibels::gainToDecibels (m / (N * 0.25f), -90.0f);
        const float v = juce::jlimit (0.0f, 1.0f, (db + 80.0f) / 80.0f);
        bands[(size_t) b] = juce::jmax (v, bands[(size_t) b] * 0.85f);
        spec.add (bands[(size_t) b]);
    }

    // host position, extrapolated to "now" and pulled back by one block so the bounce lines up with what you hear
    const bool playing = proc.hostPlaying.load() && proc.hostHasPpq.load();
    const double bpm = proc.hostBpm.load();
    double ppq = proc.hostPpq.load();
    if (playing)
    {
        const double elapsed = (juce::Time::getMillisecondCounterHiRes() - proc.stampMs.load()) / 1000.0;
        ppq += (elapsed - proc.blockSeconds.load() - proc.getLatencySamples() / sr) * bpm / 60.0;
    }

    auto* o = new juce::DynamicObject();
    o->setProperty ("goo", proc.apvts.getRawParameterValue ("goo")->load());
    o->setProperty ("inDb", proc.inDb.load());
    o->setProperty ("outDb", proc.outDb.load());
    o->setProperty ("bpm", bpm);
    o->setProperty ("ppq", ppq);
    o->setProperty ("playing", playing);
    o->setProperty ("num", proc.timeSigNum.load());
    o->setProperty ("wave", wave);
    o->setProperty ("spec", spec);
    browser.emitEventIfBrowserIsVisible ("state", juce::var (o));
}
