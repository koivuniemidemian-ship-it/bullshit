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

    void drawLayer (juce::Graphics&, const juce::Image& calm, const juce::Image& angry, float rage, juce::Rectangle<float> dest);
    void drawMoon (juce::Graphics&, float k);
    void drawCritters (juce::Graphics&, float rage);
    void drawGirl (juce::Graphics&, int mood, float rage);
    void drawSprite (juce::Graphics&, const juce::Image&, float cx, float cy, float size, float angle, float alpha);
    void drawDrip (juce::Graphics&, int which, float x, float topY, float len, float width);
    void drawPanel (juce::Graphics&, float k, int mood, float rage);
    void drawBlood (juce::Graphics&, float k, int mood);

    ShizukaProcessor& proc;
    KawaiiLookAndFeel lnf;

    juce::Slider knob;
    juce::Label knobLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment;

    juce::Image sky[2], mountains[2], girl[2], moonImg;
    std::array<juce::Image, 6> splats;
    std::array<juce::Image, 3> drips;

    std::array<float, ShizukaProcessor::numBands> bandsUI {};
    juce::Random rng;
    float t = 0.0f, gatedUI = 0.0f;
    int tagline = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShizukaEditor)
};
