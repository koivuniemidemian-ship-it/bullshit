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

    float knobValue() const;
    static int moodFor (float k);

    void drawBackground (juce::Graphics&, float k, float rage);
    void drawGirl (juce::Graphics&, int mood, float rage);
    void drawHead (juce::Graphics&, int mood, float rage);
    void drawPanel (juce::Graphics&, float k, int mood, float rage);
    void drawBlood (juce::Graphics&, float k, int mood);

    ShizukaProcessor& proc;
    KawaiiLookAndFeel lnf;

    juce::Slider knob;
    juce::Label knobLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment;

    std::array<float, ShizukaProcessor::numBands> bandsUI {};
    juce::Random rng;
    float t = 0.0f, gatedUI = 0.0f;
    int tagline = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShizukaEditor)
};
