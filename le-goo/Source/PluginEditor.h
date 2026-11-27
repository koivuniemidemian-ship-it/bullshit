#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

// The whole interface is the Le Goo HTML page, shown in a native web view.
// C++ -> page: a "state" event ~60x per second (knob, levels, host tempo + position, scope, spectrum).
// Page -> C++: "knob" (new value 0..100) and "gesture" (true = drag started, false = ended).
class LeGooEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit LeGooEditor (LeGooProcessor&);
    ~LeGooEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    juce::WebBrowserComponent::Options makeOptions();

    LeGooProcessor& proc;
    juce::WebBrowserComponent browser;

    juce::dsp::FFT fft { 11 };
    juce::dsp::WindowingFunction<float> window { 2048, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, 4096> fftData {};
    std::array<float, 40> bands {};
    bool gestureOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LeGooEditor)
};
