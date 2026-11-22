#pragma once

#include "PluginProcessor.h"

class KawaiiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KawaiiLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
};

class ShizukaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ShizukaEditor (ShizukaProcessor&);
    ~ShizukaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    float hush() const;
    int moodFor (float k) const;
    void drawBackground (juce::Graphics&, float k);
    void drawSpectrum (juce::Graphics&, float k);
    void drawEffects (juce::Graphics&, float k, int mood);
    void drawGirl (juce::Graphics&, int mood);
    void drawBubble (juce::Graphics&, int mood);

    ShizukaProcessor& proc;
    KawaiiLookAndFeel lnf;

    juce::Slider knob;
    juce::Label knobLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment;

    std::array<float, ShizukaProcessor::numBands> bandsUI {};
    juce::Rectangle<float> stage;
    juce::Random rng;
    float t = 0.0f, gatedUI = 0.0f;
    int tagline = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShizukaEditor)
};
