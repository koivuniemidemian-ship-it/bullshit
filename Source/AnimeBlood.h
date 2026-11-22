#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// Cel-shaded "anime" blood: flat saturated red, a hard dark shadow edge,
// crisp white highlights, tapered splash streaks and glossy drips.
namespace AnimeBlood
{
    inline const juce::Colour red    { 0xffd40a24 };
    inline const juce::Colour dark   { 0xff6e0010 };
    inline const juce::Colour bright { 0xffff3a4a };

    inline juce::Path shape (float x, float y, float r, int seed)
    {
        constexpr float pi = juce::MathConstants<float>::pi;
        juce::Random rr (seed);
        juce::Path p;

        const int n = 14;
        for (int i = 0; i < n; ++i)
        {
            const float a = 2.0f * pi * (float) i / (float) n + rr.nextFloat() * 0.2f;
            const float rad = r * (0.7f + 0.35f * rr.nextFloat());
            const float px = x + std::cos (a) * rad, py = y + std::sin (a) * rad;
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
        p.closeSubPath();
        p = p.createPathWithRoundedCorners (r * 0.2f);

        // tapered splash streaks with a droplet at the tip
        const int streaks = 5 + rr.nextInt (4);
        for (int i = 0; i < streaks; ++i)
        {
            const float a = rr.nextFloat() * 2.0f * pi;
            const float len = r * (1.4f + 1.6f * rr.nextFloat());
            const float w = r * (0.18f + 0.15f * rr.nextFloat());
            const juce::Point<float> dir (std::cos (a), std::sin (a)), perp (-dir.y, dir.x);
            const auto base = juce::Point<float> (x, y) + dir * (r * 0.6f);
            const auto tip  = juce::Point<float> (x, y) + dir * len;
            juce::Path s;
            s.startNewSubPath (base + perp * w);
            s.lineTo (tip);
            s.lineTo (base - perp * w);
            s.closeSubPath();
            p.addPath (s);
            const float d = w * (0.6f + 0.5f * rr.nextFloat());
            const auto drop = juce::Point<float> (x, y) + dir * (len + d * 1.4f);
            p.addEllipse (drop.x - d, drop.y - d, d * 2.0f, d * 2.0f);
        }

        // loose droplets
        for (int j = 0; j < 7; ++j)
        {
            const float a = rr.nextFloat() * 2.0f * pi;
            const float d = r * (1.3f + 1.8f * rr.nextFloat());
            const float s = r * (0.05f + 0.12f * rr.nextFloat());
            p.addEllipse (x + std::cos (a) * d - s, y + std::sin (a) * d - s, s * 2.0f, s * 2.0f);
        }
        return p;
    }

    inline void drip (juce::Graphics& g, float x, float y, float len, float w)
    {
        juce::Path d;
        d.addRoundedRectangle (x - w * 0.5f, y, w, len, w * 0.5f);
        const float bw = w * 1.7f;
        d.addEllipse (x - bw * 0.5f, y + len - bw * 0.45f, bw, bw * 1.15f);

        g.setColour (red);
        g.fillPath (d);
        {
            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (d);
            g.setColour (dark);
            g.fillRect (x + w * 0.1f, y, w * 1.2f, len + bw);   // hard shadow on the right
        }
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillRoundedRectangle (x - w * 0.32f, y + 2.0f, juce::jmax (1.0f, w * 0.18f), len * 0.8f, w * 0.1f);
        g.fillEllipse (x - bw * 0.3f, y + len - bw * 0.1f, bw * 0.28f, bw * 0.24f);
    }

    inline void splat (juce::Graphics& g, float x, float y, float r, int seed, float dripLen = 0.0f, float alpha = 1.0f)
    {
        const auto p = shape (x, y, r, seed);

        g.setColour (red.withAlpha (alpha));
        g.fillPath (p);
        {
            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (p);
            // hard cel shadow: everything except an up-left shifted copy goes dark
            g.setColour (dark.withAlpha (alpha));
            g.fillRect (p.getBounds().expanded (2.0f));
            g.setColour (red.withAlpha (alpha));
            g.fillPath (p, juce::AffineTransform::translation (-r * 0.16f, -r * 0.2f));
            // brighter core
            g.setColour (bright.withAlpha (alpha));
            g.fillEllipse (x - r * 0.55f, y - r * 0.6f, r * 0.8f, r * 0.7f);
        }
        // crisp highlights
        g.setColour (juce::Colours::white.withAlpha (0.9f * alpha));
        g.fillEllipse (x - r * 0.45f, y - r * 0.5f, r * 0.3f, r * 0.18f);
        g.fillEllipse (x + r * 0.05f, y - r * 0.15f, r * 0.1f, r * 0.1f);

        if (dripLen > 0.0f)
            drip (g, x + r * 0.1f, y + r * 0.3f, dripLen + r * 0.5f, juce::jmax (3.0f, r * 0.28f));
    }
}
