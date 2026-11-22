#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Draws the Hush-chan character (half-body portrait) into the left stage.
//   t     : animation time in seconds
//   mood  : 0 serene, 1 sweet, 2 pouting, 3 yandere, 4 full rage
//   rage  : 0..1 continuous colour shift toward blood red
//   pulse : 0..1 how much audio is being gated (mouth / glow intensity)
namespace HushGirl
{
    constexpr float CX = 215.0f;   // face centre x
    constexpr float HY = 250.0f;   // face centre y
    constexpr float R  = 92.0f;    // head scale

    void draw (juce::Graphics& g, float t, int mood, float rage, float pulse);
}
