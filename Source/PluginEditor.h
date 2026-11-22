#pragma once

#include "PluginProcessor.h"
#include <vector>

class GooLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GooLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class MoistBoiEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MoistBoiEditor (MoistBoiProcessor&);
    ~MoistBoiEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Droplet
    {
        juce::Point<float> pos, vel;
        float life = 1.0f, size = 4.0f;
    };

    void timerCallback() override;
    void drawTitle (juce::Graphics&, juce::Rectangle<float> area);
    void drawBlob (juce::Graphics&);
    float norm (const juce::String& paramID) const;
    juce::Point<float> blobCentre() const;
    float blobRadius() const;

    MoistBoiProcessor& proc;
    GooLookAndFeel goo;

    std::array<Knob, 6> knobs;
    juce::ToggleButton squelchButton { "SQUELCH" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> squelchAttachment;

    std::vector<Droplet> droplets;
    juce::Random rng;
    juce::Rectangle<float> blobArea;
    float t = 0.0f, level = 0.0f;
    int tagline = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoistBoiEditor)
};
