#include "Girl.h"
#include "AnimeBlood.h"
#include <cmath>

using namespace HushGirl;

// TV-anime cel style: flat base colours, hard-edged two-tone shadows,
// crisp highlights and thin coloured line art.

namespace
{
    using juce::Colour;
    using juce::Graphics;
    using juce::Path;
    using juce::PathStrokeType;

    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float BOTTOM = 760.0f;

    Colour mix (Colour a, Colour b, float t) { return a.interpolatedWith (b, juce::jlimit (0.0f, 1.0f, t)); }

    struct Pal
    {
        Colour hair, hairSh, hairDeep, hairGloss, hairGlossEdge, hairLine;
        Colour skin, skinSh, skinDeep, skinLine, blush;
        Colour iris, irisDark, irisLight, lash, sclera, scleraSh;
        Colour cloth, clothSh, clothHi, clothLine;
        Colour ribbon, ribbonSh, silver;
    };

    Pal makePal (float rage)
    {
        Pal p;
        p.hair          = mix (Colour (0xffe4e7f3), Colour (0xffdcb6bc), rage);
        p.hairSh        = mix (Colour (0xff8f95ba), Colour (0xffa04a58), rage);
        p.hairDeep      = mix (Colour (0xff5a5e86), Colour (0xff560c1c), rage);
        p.hairGloss     = mix (Colour (0xfff8f9ff), Colour (0xffffe6e6), rage);
        p.hairGlossEdge = mix (Colour (0xffb9b2e6), Colour (0xffff5a6a), rage);
        p.hairLine      = mix (Colour (0xff3c4062), Colour (0xff2e0610), rage);

        p.skin     = Colour (0xfffbe7dc);
        p.skinSh   = mix (Colour (0xffefb9a6), Colour (0xffe6aaa6), rage);
        p.skinDeep = mix (Colour (0xffe0a596), Colour (0xffc0707a), rage);
        p.skinLine = mix (Colour (0xff86504a), Colour (0xff5a1018), rage);
        p.blush    = mix (Colour (0xffff8a9a), Colour (0xffff2a40), rage);

        p.iris      = mix (Colour (0xff6f8fd6), Colour (0xffd4142c), rage);
        p.irisDark  = mix (Colour (0xff33437f), Colour (0xff480008), rage);
        p.irisLight = mix (Colour (0xffc2d8ff), Colour (0xffffa050), rage);
        p.lash      = Colour (0xff1c1726);
        p.sclera    = Colour (0xffffffff);
        p.scleraSh  = mix (Colour (0xffcfd5ee), Colour (0xffe8c8c8), rage);

        p.cloth     = mix (Colour (0xff36418a), Colour (0xff3a0c18), rage);
        p.clothSh   = mix (Colour (0xff232b60), Colour (0xff1c040a), rage);
        p.clothHi   = mix (Colour (0xff5262b4), Colour (0xff6a1626), rage);
        p.clothLine = mix (Colour (0xff141a3e), Colour (0xff0c0004), rage);

        p.ribbon   = mix (Colour (0xff1f1b2e), Colour (0xff140006), rage);
        p.ribbonSh = mix (Colour (0xff0e0c16), Colour (0xff050002), rage);
        p.silver   = mix (Colour (0xffe8ecff), Colour (0xffffb0b0), rage);
        return p;
    }

    // fill a shape with a hard cel shadow (`shadow` is clipped into `shape`)
    void cel (Graphics& g, const Path& shape, Colour base, const Path& shadow, Colour shade, Colour line, float lineW)
    {
        g.setColour (base);
        g.fillPath (shape);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (shape);
            g.setColour (shade);
            g.fillPath (shadow);
        }
        if (lineW > 0.0f)
        {
            g.setColour (line);
            g.strokePath (shape, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }

    Path lock (float x0, float y0, float x1, float y1, float w, float bend, float bulge = 1.15f)
    {
        Path p;
        const float dy = y1 - y0, hw = w * 0.5f;
        p.startNewSubPath (x0 - hw, y0);
        p.cubicTo (x0 - hw * bulge + bend * 0.3f, y0 + dy * 0.35f, x1 - hw * 0.35f + bend, y0 + dy * 0.78f, x1, y1);
        p.cubicTo (x1 + hw * 0.35f + bend, y0 + dy * 0.78f, x0 + hw * bulge + bend * 0.3f, y0 + dy * 0.35f, x0 + hw, y0);
        p.closeSubPath();
        return p;
    }

    Path shifted (const Path& p, float dx, float dy = 0.0f)
    {
        Path s (p);
        s.applyTransform (juce::AffineTransform::translation (dx, dy));
        return s;
    }

    void strandLines (Graphics& g, const juce::Rectangle<float>& b, Colour c, int n, float seed)
    {
        g.setColour (c);
        for (int i = 0; i < n; ++i)
        {
            const float f = ((float) i + 0.5f) / (float) n;
            const float x0 = b.getX() + b.getWidth() * f;
            const float wob = 4.0f * std::sin (seed + (float) i * 1.9f);
            Path l;
            l.startNewSubPath (x0, b.getY() + b.getHeight() * (0.1f + 0.15f * std::abs (std::sin (seed + (float) i))));
            l.quadraticTo (x0 + wob, b.getCentreY(), x0 + wob * 0.4f, b.getBottom() - b.getHeight() * (0.1f + 0.2f * std::abs (std::cos (seed * 0.7f + (float) i))));
            g.strokePath (l, PathStrokeType (0.9f));
        }
    }

    //==========================================================================
    void drawBackHair (Graphics& g, const Pal& P, float t)
    {
        const float sway = 5.0f * std::sin (t * 0.9f);

        Path back;
        back.startNewSubPath (CX - 0.95f * R, HY - 0.55f * R);
        back.cubicTo (CX - 1.1f * R, HY - 1.5f * R, CX + 1.1f * R, HY - 1.5f * R, CX + 0.95f * R, HY - 0.55f * R);
        back.cubicTo (CX + 1.3f * R, HY + 0.8f * R, CX + 1.55f * R + sway, HY + 3.0f * R, CX + 1.7f * R + sway, HY + 4.7f * R);
        const int waves = 7;
        for (int i = 1; i <= waves; ++i)
        {
            const float f = (float) i / (float) waves;
            const float x = CX + 1.7f * R - 3.4f * R * f + sway;
            const float xm = x + 3.4f * R / (float) waves * 0.5f;
            const float y = HY + (4.7f + 0.35f * std::sin ((float) i * 2.1f)) * R;
            back.quadraticTo (xm, y + 0.45f * R, x, y);
        }
        back.cubicTo (CX - 1.55f * R + sway, HY + 3.0f * R, CX - 1.3f * R, HY + 0.8f * R, CX - 0.95f * R, HY - 0.55f * R);
        back.closeSubPath();

        g.setColour (P.hairSh);
        g.fillPath (back);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (back);

            const int n = 9;
            for (int i = 0; i < n; ++i)
            {
                const float f = (float) i / (float) (n - 1) * 2.0f - 1.0f;
                const float bend = sway * (0.6f + 0.4f * std::sin (t + (float) i)) + f * 14.0f;
                auto l = lock (CX + f * 0.9f * R, HY - 1.6f * R, CX + f * 1.5f * R + bend,
                               HY + (4.0f + 0.6f * std::sin ((float) i * 1.7f)) * R, 0.42f * R, bend);
                g.setColour (f < 0.2f ? P.hair : P.hair.interpolatedWith (P.hairSh, 0.4f));
                g.fillPath (l);
                g.setColour (P.hairSh);
                g.fillPath (shifted (l, (f < 0.0f ? 1.0f : -1.0f) * 0.12f * R));
            }
            g.setColour (P.hairDeep);
            g.fillEllipse (CX - 1.1f * R, HY + 2.0f * R, 2.2f * R, 1.6f * R);
            strandLines (g, back.getBounds(), P.hairLine.withAlpha (0.35f), 14, 1.3f);
        }
        g.setColour (P.hairLine);
        g.strokePath (back, PathStrokeType (1.5f));
    }

    //==========================================================================
    void drawBody (Graphics& g, const Pal& P, int mood, float rage)
    {
        // ---- neck, shoulders and two arms hanging at her sides
        Path skin;
        skin.startNewSubPath (CX - 0.24f * R, HY + 0.7f * R);
        skin.lineTo (CX - 0.26f * R, HY + 1.3f * R);
        for (int pass = 0; pass < 2; ++pass)
        {
            const float f = pass == 0 ? -1.0f : 1.0f;
            auto X = [&] (float v) { return CX + f * v * R; };
            if (pass == 0)
            {
                skin.cubicTo (X (0.6f), HY + 1.48f * R, X (1.05f), HY + 1.5f * R, X (1.28f), HY + 1.8f * R);
                skin.cubicTo (X (1.42f), HY + 2.1f * R, X (1.45f), HY + 3.0f * R, X (1.48f), HY + 4.2f * R);
                skin.cubicTo (X (1.5f), HY + 4.8f * R, X (1.46f), HY + 5.4f * R, X (1.44f), BOTTOM);
                skin.lineTo (X (1.1f), BOTTOM);
                skin.cubicTo (X (1.12f), HY + 5.2f * R, X (1.16f), HY + 4.6f * R, X (1.14f), HY + 4.2f * R);
                skin.cubicTo (X (1.12f), HY + 3.2f * R, X (1.06f), HY + 2.6f * R, X (0.96f), HY + 2.25f * R);
            }
            else
            {
                skin.lineTo (X (0.96f), HY + 2.25f * R);
                skin.cubicTo (X (1.06f), HY + 2.6f * R, X (1.12f), HY + 3.2f * R, X (1.14f), HY + 4.2f * R);
                skin.cubicTo (X (1.16f), HY + 4.6f * R, X (1.12f), HY + 5.2f * R, X (1.1f), BOTTOM);
                skin.lineTo (X (1.44f), BOTTOM);
                skin.cubicTo (X (1.46f), HY + 5.4f * R, X (1.5f), HY + 4.8f * R, X (1.48f), HY + 4.2f * R);
                skin.cubicTo (X (1.45f), HY + 3.0f * R, X (1.42f), HY + 2.1f * R, X (1.28f), HY + 1.8f * R);
                skin.cubicTo (X (1.05f), HY + 1.5f * R, X (0.6f), HY + 1.48f * R, X (0.26f), HY + 1.3f * R);
            }
        }
        skin.lineTo (CX + 0.24f * R, HY + 0.7f * R);
        skin.closeSubPath();

        Path skinShadow;
        skinShadow.startNewSubPath (CX - 0.3f * R, HY + 0.7f * R);
        skinShadow.lineTo (CX + 0.3f * R, HY + 0.7f * R);
        skinShadow.lineTo (CX + 0.3f * R, HY + 1.05f * R);
        skinShadow.quadraticTo (CX, HY + 1.2f * R, CX - 0.3f * R, HY + 0.98f * R);
        skinShadow.closeSubPath();
        skinShadow.addRectangle (CX + 0.1f * R, HY + 0.7f * R, 0.2f * R, 0.7f * R);
        // inner strip of the left arm, most of the right arm (light from the upper left)
        skinShadow.addRectangle (CX - 1.24f * R, HY + 2.2f * R, 0.2f * R, BOTTOM);
        Path rightArm;
        rightArm.startNewSubPath (CX + 1.05f * R, HY + 2.0f * R);
        rightArm.cubicTo (CX + 1.3f * R, HY + 2.4f * R, CX + 1.28f * R, HY + 3.6f * R, CX + 1.3f * R, BOTTOM);
        rightArm.lineTo (CX + 1.0f * R, BOTTOM);
        rightArm.lineTo (CX + 1.0f * R, HY + 2.0f * R);
        rightArm.closeSubPath();
        skinShadow.addPath (rightArm);
        skinShadow.addEllipse (CX + 1.1f * R, HY + 1.7f * R, 0.4f * R, 0.5f * R);
        // elbow creases
        cel (g, skin, P.skin, skinShadow, P.skinSh, P.skinLine, 1.5f);
        g.setColour (P.skinLine.withAlpha (0.6f));
        g.drawLine (CX - 1.42f * R, HY + 4.15f * R, CX - 1.32f * R, HY + 4.2f * R, 1.2f);
        g.drawLine (CX + 1.42f * R, HY + 4.15f * R, CX + 1.32f * R, HY + 4.2f * R, 1.2f);

        // shoulder highlights + collarbone hints
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillEllipse (CX - 1.25f * R, HY + 1.75f * R, 0.14f * R, 0.06f * R);
        g.setColour (P.skinSh);
        g.drawLine (CX - 0.55f * R, HY + 1.55f * R, CX - 0.3f * R, HY + 1.62f * R, 1.6f);
        g.drawLine (CX + 0.55f * R, HY + 1.55f * R, CX + 0.3f * R, HY + 1.62f * R, 1.6f);

        // ---- fitted high-neck sleeveless dress
        Path dress;
        dress.startNewSubPath (CX - 0.3f * R, HY + 1.2f * R);
        dress.lineTo (CX + 0.3f * R, HY + 1.2f * R);
        dress.lineTo (CX + 0.33f * R, HY + 1.4f * R);
        dress.cubicTo (CX + 0.68f * R, HY + 1.58f * R, CX + 0.9f * R, HY + 1.85f * R, CX + 0.98f * R, HY + 2.25f * R);
        dress.cubicTo (CX + 0.98f * R, HY + 2.8f * R, CX + 0.84f * R, HY + 3.3f * R, CX + 0.82f * R, HY + 3.7f * R);
        dress.cubicTo (CX + 0.84f * R, HY + 4.3f * R, CX + 0.98f * R, HY + 4.9f * R, CX + 1.05f * R, BOTTOM);
        dress.lineTo (CX - 1.05f * R, BOTTOM);
        dress.cubicTo (CX - 0.98f * R, HY + 4.9f * R, CX - 0.84f * R, HY + 4.3f * R, CX - 0.82f * R, HY + 3.7f * R);
        dress.cubicTo (CX - 0.84f * R, HY + 3.3f * R, CX - 0.98f * R, HY + 2.8f * R, CX - 0.98f * R, HY + 2.25f * R);
        dress.cubicTo (CX - 0.9f * R, HY + 1.85f * R, CX - 0.68f * R, HY + 1.58f * R, CX - 0.33f * R, HY + 1.4f * R);
        dress.closeSubPath();

        Path dressShadow;
        dressShadow.startNewSubPath (CX + 0.42f * R, HY + 1.45f * R);
        dressShadow.cubicTo (CX + 0.55f * R, HY + 2.4f * R, CX + 0.4f * R, HY + 3.2f * R, CX + 0.48f * R, HY + 3.8f * R);
        dressShadow.cubicTo (CX + 0.55f * R, HY + 4.4f * R, CX + 0.6f * R, HY + 5.0f * R, CX + 0.66f * R, BOTTOM);
        dressShadow.lineTo (CX + 1.3f * R, BOTTOM);
        dressShadow.lineTo (CX + 1.3f * R, HY + 1.45f * R);
        dressShadow.closeSubPath();
        dressShadow.addEllipse (CX - 0.45f * R, HY + 1.32f * R, 0.9f * R, 0.2f * R);
        for (int i = 0; i < 3; ++i)   // hip fold wedges
        {
            const float x = CX + (-0.6f + 0.35f * (float) i) * R;
            Path fold;
            fold.startNewSubPath (x, HY + 4.0f * R);
            fold.lineTo (x + 0.07f * R, BOTTOM);
            fold.lineTo (x - 0.09f * R, BOTTOM);
            fold.closeSubPath();
            dressShadow.addPath (fold);
        }
        cel (g, dress, P.cloth, dressShadow, P.clothSh, P.clothLine, 1.8f);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (dress);
            Path hi;
            hi.startNewSubPath (CX - 0.8f * R, HY + 2.35f * R);
            hi.cubicTo (CX - 0.68f * R, HY + 2.9f * R, CX - 0.66f * R, HY + 3.6f * R, CX - 0.7f * R, BOTTOM);
            hi.lineTo (CX - 0.6f * R, BOTTOM);
            hi.cubicTo (CX - 0.56f * R, HY + 3.6f * R, CX - 0.58f * R, HY + 2.9f * R, CX - 0.68f * R, HY + 2.35f * R);
            hi.closeSubPath();
            g.setColour (P.clothHi);
            g.fillPath (hi);

            const float dashes[] = { 3.0f, 3.0f };
            for (int side = -1; side <= 1; side += 2)
            {
                const float f = (float) side;
                g.setColour (P.clothLine.withAlpha (0.7f));
                g.drawLine (CX + f * 0.42f * R, HY + 1.6f * R, CX + f * 0.36f * R, BOTTOM, 1.2f);
                g.setColour (P.clothHi.withAlpha (0.6f));
                g.drawDashedLine ({ CX + f * 0.46f * R, HY + 1.7f * R, CX + f * 0.4f * R, BOTTOM }, dashes, 2, 1.0f);
            }
            for (int i = 0; i < 5; ++i)
            {
                const float by = HY + (1.75f + 0.42f * (float) i) * R;
                g.setColour (P.silver);
                g.fillEllipse (CX - 0.05f * R, by, 0.1f * R, 0.1f * R);
                g.setColour (P.clothLine);
                g.drawEllipse (CX - 0.05f * R, by, 0.1f * R, 0.1f * R, 1.0f);
                g.setColour (juce::Colours::white);
                g.fillEllipse (CX - 0.03f * R, by + 0.015f * R, 0.03f * R, 0.03f * R);
            }

            if (rage > 0.0f)
            {
                juce::Random st (404);
                const int n = (int) (rage * 11.0f);
                for (int i = 0; i < n; ++i)
                    AnimeBlood::splat (g, CX + (st.nextFloat() - 0.5f) * 2.0f * R, HY + (1.8f + st.nextFloat() * 4.0f) * R,
                                       (0.07f + 0.13f * st.nextFloat()) * R, 1000 + i, rage * (8.0f + 40.0f * st.nextFloat()));
            }
        }

        // collar band with piping
        const juce::Rectangle<float> collar (CX - 0.29f * R, HY + 1.16f * R, 0.58f * R, 0.2f * R);
        g.setColour (P.cloth);
        g.fillRoundedRectangle (collar, 0.05f * R);
        g.setColour (P.clothSh);
        g.fillRect (collar.withTrimmedLeft (collar.getWidth() * 0.65f).reduced (0.0f, 1.5f));
        g.setColour (P.clothHi);
        g.drawLine (collar.getX() + 3.0f, collar.getY() + 3.0f, collar.getRight() - 3.0f, collar.getY() + 3.0f, 1.2f);
        g.setColour (P.clothLine);
        g.drawRoundedRectangle (collar, 0.05f * R, 1.5f);

        // velvet choker with a crescent charm
        const float cy = HY + 0.95f * R;
        g.setColour (P.ribbon);
        g.fillRect (CX - 0.25f * R, cy, 0.5f * R, 0.09f * R);
        g.setColour (P.ribbonSh);
        g.fillRect (CX + 0.08f * R, cy, 0.17f * R, 0.09f * R);
        const float mr = 0.07f * R;
        g.setColour (P.silver);
        g.fillEllipse (CX - mr, cy + 0.08f * R, mr * 2.0f, mr * 2.0f);
        g.setColour (P.skin);
        g.fillEllipse (CX - mr * 0.3f, cy + 0.06f * R, mr * 1.7f, mr * 1.7f);
        g.setColour (P.clothLine);
        g.drawEllipse (CX - mr, cy + 0.08f * R, mr * 2.0f, mr * 2.0f, 0.8f);

        if (mood >= 3)
        {
            AnimeBlood::splat (g, CX + 0.6f * R, HY + 1.7f * R, 0.1f * R, 55, rage * 0.35f * R);
            AnimeBlood::splat (g, CX - 0.85f * R, HY + 1.9f * R, 0.08f * R, 56, rage * 0.3f * R);
            AnimeBlood::splat (g, CX - 1.35f * R, HY + 2.8f * R, 0.09f * R, 57, rage * 0.5f * R);
        }
    }

    //==========================================================================
    void drawLongLocks (Graphics& g, const Pal& P, float t, int mood, float rage)
    {
        const float sway = 4.0f * std::sin (t);
        for (int side = -1; side <= 1; side += 2)
        {
            const float f = (float) side;
            for (int j = 0; j < 2; ++j)
            {
                const float x0 = CX + f * (0.86f - 0.08f * (float) j) * R;
                const float bend = sway * (0.6f + 0.3f * (float) j) + f * (16.0f + 8.0f * (float) j);
                const float y1 = HY + (3.7f - 0.6f * (float) j) * R;
                const float x1 = CX + f * (1.0f + 0.15f * (float) j) * R + bend * 0.4f;
                auto l = lock (x0, HY - 0.3f * R, x1, y1, (0.34f - 0.05f * (float) j) * R, bend, 1.25f);
                cel (g, l, P.hair, shifted (l, -f * 0.1f * R), P.hairSh, P.hairLine, 1.3f);
                {
                    Graphics::ScopedSaveState s (g);
                    g.reduceClipRegion (l);
                    strandLines (g, l.getBounds(), P.hairLine.withAlpha (0.4f), 3, f + (float) j);
                    const auto b = l.getBounds();
                    g.setColour (P.hairGloss);
                    g.fillEllipse (b.getCentreX() - b.getWidth() * 0.12f + f * 2.0f, b.getY() + b.getHeight() * 0.18f, b.getWidth() * 0.2f, b.getHeight() * 0.12f);
                }
                Path curl;
                curl.addCentredArc (x1 - f * 6.0f, y1 - 4.0f, 7.0f, 6.0f, 0.0f, f > 0 ? pi * 0.2f : -pi * 0.2f, f > 0 ? pi * 1.3f : -pi * 1.3f, true);
                g.setColour (P.hairLine);
                g.strokePath (curl, PathStrokeType (1.2f));

                if (mood >= 3 && j == 0)
                    AnimeBlood::drip (g, x1, y1 - 2.0f, rage * (10.0f + 16.0f * (0.5f + 0.5f * std::sin (t * 2.0f + f))), 4.0f);
            }
        }
    }

    //==========================================================================
    void drawEye (Graphics& g, const Pal& P, float ex, float ey, float inner, int mood, float t, int side)
    {
        const float ox = ex - inner * 0.25f * R, ix = ex + inner * 0.2f * R;

        if (mood == 0)
        {
            Path closed;
            closed.startNewSubPath (ox - inner * 0.02f * R, ey - 0.02f * R);
            closed.quadraticTo (ex, ey + 0.12f * R, ix, ey);
            g.setColour (P.lash);
            g.strokePath (closed, PathStrokeType (3.2f, PathStrokeType::curved, PathStrokeType::rounded));
            for (int l = 0; l < 4; ++l)
            {
                const float u = 0.1f + 0.14f * (float) l;
                const float lx = ox + (ix - ox) * u, ly = ey + 0.07f * R * std::sin (u * pi) - 0.01f * R;
                g.drawLine (lx, ly, lx - inner * 0.02f * R, ly + 0.07f * R, 1.2f);
            }
            g.drawLine (ox - inner * 0.02f * R, ey - 0.02f * R, ox - inner * 0.08f * R, ey - 0.07f * R, 2.4f);
            return;
        }

        Path eye;
        eye.startNewSubPath (ix, ey + 0.02f * R);
        eye.cubicTo (ix - inner * 0.05f * R, ey - 0.18f * R, ox + inner * 0.12f * R, ey - 0.27f * R, ox, ey - 0.07f * R);
        eye.cubicTo (ox + inner * 0.02f * R, ey + 0.11f * R, ex - inner * 0.04f * R, ey + 0.22f * R, ex + inner * 0.06f * R, ey + 0.2f * R);
        eye.cubicTo (ix - inner * 0.04f * R, ey + 0.17f * R, ix, ey + 0.09f * R, ix, ey + 0.02f * R);
        eye.closeSubPath();

        g.setColour (P.sclera);
        g.fillPath (eye);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (eye);

            g.setColour (P.scleraSh);
            g.fillEllipse (ex - 0.35f * R, ey - 0.45f * R, 0.7f * R, 0.4f * R);

            const float icx = ex + inner * 0.015f * R, icy = ey + 0.02f * R;
            const float iw = 0.34f * R, ih = 0.42f * R;
            Path iris;
            iris.addEllipse (icx - iw * 0.5f, icy - ih * 0.5f, iw, ih);
            g.setColour (P.iris);
            g.fillPath (iris);
            {
                Graphics::ScopedSaveState s2 (g);
                g.reduceClipRegion (iris);
                g.setColour (P.irisDark);
                g.fillEllipse (icx - iw * 0.75f, icy - ih * 1.05f, iw * 1.5f, ih * 0.95f);
                g.setColour (P.irisLight);
                g.fillEllipse (icx - iw * 0.4f, icy + ih * 0.12f, iw * 0.8f, ih * 0.45f);
                g.setColour (P.iris);
                g.fillEllipse (icx - iw * 0.32f, icy + ih * 0.02f, iw * 0.64f, ih * 0.38f);
                g.setColour (P.irisDark.withAlpha (0.35f));
                for (int l = 0; l < 16; ++l)
                {
                    const float a = 2.0f * pi * (float) l / 16.0f;
                    g.drawLine (icx + std::cos (a) * iw * 0.15f, icy + std::sin (a) * ih * 0.13f,
                                icx + std::cos (a) * iw * 0.45f, icy + std::sin (a) * ih * 0.45f, 0.6f);
                }
            }

            g.setColour (P.lash);
            if (mood == 4)
                g.fillEllipse (icx - 0.022f * R, icy - ih * 0.32f, 0.044f * R, ih * 0.64f);
            else if (mood == 3)
                g.fillEllipse (icx - 0.025f * R, icy - 0.03f * R, 0.05f * R, 0.06f * R);
            else
                g.fillEllipse (icx - 0.06f * R, icy - 0.08f * R, 0.12f * R, 0.16f * R);

            if (mood == 3)
            {
                g.setColour (P.irisDark);
                g.drawEllipse (icx - iw * 0.3f, icy - ih * 0.3f, iw * 0.6f, ih * 0.6f, 1.0f);
            }
            g.setColour (P.irisDark.darker (0.4f));
            g.strokePath (iris, PathStrokeType (1.4f));

            if (mood <= 2)
            {
                g.setColour (juce::Colours::white);
                g.fillEllipse (icx - iw * 0.38f, icy - ih * 0.3f, iw * 0.3f, ih * 0.2f);
                g.fillEllipse (icx + iw * 0.14f, icy + ih * 0.14f, iw * 0.12f, iw * 0.12f);
                g.setColour (juce::Colours::white.withAlpha (0.7f + 0.3f * std::sin (t * 3.0f + (float) side)));
                g.fillEllipse (icx + iw * 0.2f, icy - ih * 0.22f, iw * 0.07f, iw * 0.07f);
            }

            if (mood == 2)
            {
                g.setColour (P.skin);
                g.fillRect (ex - 0.4f * R, ey - 0.4f * R, 0.8f * R, 0.29f * R);
            }
        }

        if (mood == 4)
            for (int i = 3; i >= 1; --i)
            {
                g.setColour (AnimeBlood::bright.withAlpha (0.12f));
                const float gw = 0.36f * R + (float) i * 7.0f, gh = 0.4f * R + (float) i * 6.0f;
                g.fillEllipse (ex - gw * 0.5f, ey - gh * 0.5f, gw, gh);
            }

        const float drop = (mood == 2) ? 0.1f * R : 0.0f;
        Path lash;
        lash.startNewSubPath (ix + inner * 0.01f * R, ey + 0.03f * R + drop * 0.3f);
        lash.cubicTo (ix - inner * 0.05f * R, ey - 0.19f * R + drop, ox + inner * 0.12f * R, ey - 0.3f * R + drop, ox - inner * 0.06f * R, ey - 0.09f * R + drop);
        lash.lineTo (ox - inner * 0.11f * R, ey - 0.15f * R + drop);
        lash.cubicTo (ox + inner * 0.1f * R, ey - 0.37f * R + drop, ix - inner * 0.02f * R, ey - 0.25f * R + drop, ix + inner * 0.01f * R, ey + 0.03f * R + drop * 0.3f);
        lash.closeSubPath();
        g.setColour (P.lash);
        g.fillPath (lash);
        for (int l = 0; l < 3; ++l)
        {
            const float bx = ox + inner * (0.03f + 0.05f * (float) l) * R;
            const float by = ey - (0.13f + 0.05f * (float) l) * R + drop;
            Path flick;
            flick.startNewSubPath (bx, by);
            flick.quadraticTo (bx - inner * 0.05f * R, by - 0.05f * R, bx - inner * (0.11f - 0.02f * (float) l) * R, by - (0.05f + 0.012f * (float) l) * R);
            g.strokePath (flick, PathStrokeType (2.0f - 0.35f * (float) l, PathStrokeType::curved, PathStrokeType::rounded));
        }
        Path lower;
        lower.startNewSubPath (ox + inner * 0.01f * R, ey);
        lower.quadraticTo (ox + inner * 0.04f * R, ey + 0.16f * R, ex + inner * 0.02f * R, ey + 0.21f * R);
        g.setColour (P.lash.withAlpha (0.8f));
        g.strokePath (lower, PathStrokeType (1.3f));
        g.drawLine (ox + inner * 0.05f * R, ey + 0.1f * R, ox + inner * 0.03f * R, ey + 0.15f * R, 1.0f);
        g.drawLine (ox + inner * 0.11f * R, ey + 0.15f * R, ox + inner * 0.1f * R, ey + 0.2f * R, 1.0f);
        Path crease;
        crease.startNewSubPath (ix - inner * 0.04f * R, ey - 0.19f * R + drop);
        crease.quadraticTo (ex, ey - 0.38f * R + drop, ox + inner * 0.03f * R, ey - 0.2f * R + drop);
        g.setColour (P.skinLine.withAlpha (0.55f));
        g.strokePath (crease, PathStrokeType (1.0f));

        if (mood == 4)
            AnimeBlood::drip (g, ex + inner * 0.03f * R, ey + 0.17f * R,
                              0.65f * R + 0.3f * R * (0.5f + 0.5f * std::sin (t * 1.6f + (float) side)), 4.5f);
    }

    //==========================================================================
    void drawFace (Graphics& g, const Pal& P, int mood, float rage, float pulse, float t)
    {
        Path face;
        face.startNewSubPath (CX - 0.8f * R, HY - 0.45f * R);
        face.cubicTo (CX - 0.82f * R, HY + 0.1f * R, CX - 0.7f * R, HY + 0.48f * R, CX - 0.45f * R, HY + 0.75f * R);
        face.cubicTo (CX - 0.27f * R, HY + 0.93f * R, CX - 0.08f * R, HY + 1.0f * R, CX, HY + 1.01f * R);
        face.cubicTo (CX + 0.08f * R, HY + 1.0f * R, CX + 0.27f * R, HY + 0.93f * R, CX + 0.45f * R, HY + 0.75f * R);
        face.cubicTo (CX + 0.7f * R, HY + 0.48f * R, CX + 0.82f * R, HY + 0.1f * R, CX + 0.8f * R, HY - 0.45f * R);
        face.cubicTo (CX + 0.8f * R, HY - 1.1f * R, CX - 0.8f * R, HY - 1.1f * R, CX - 0.8f * R, HY - 0.45f * R);
        face.closeSubPath();

        Path faceShadow;
        faceShadow.startNewSubPath (CX - R, HY - R);
        faceShadow.lineTo (CX + R, HY - R);
        faceShadow.lineTo (CX + R, HY - 0.02f * R);
        const int notches = 10;
        for (int i = 0; i <= notches; ++i)
        {
            const float x = CX + R - 2.0f * R * (float) i / (float) notches;
            faceShadow.lineTo (x, HY - 0.06f * R + ((i % 2 == 0) ? 0.04f * R : -0.06f * R));
        }
        faceShadow.closeSubPath();
        Path cheek;
        cheek.startNewSubPath (CX + 0.62f * R, HY - 0.1f * R);
        cheek.cubicTo (CX + 0.62f * R, HY + 0.35f * R, CX + 0.5f * R, HY + 0.62f * R, CX + 0.25f * R, HY + 0.92f * R);
        cheek.lineTo (CX + R, HY + R);
        cheek.lineTo (CX + R, HY - 0.1f * R);
        cheek.closeSubPath();
        faceShadow.addPath (cheek);
        cel (g, face, P.skin, faceShadow, P.skinSh, P.skinLine, 0.0f);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (face);
            for (int side = -1; side <= 1; side += 2)
            {
                const float bx = CX + (float) side * 0.45f * R, by = HY + 0.48f * R;
                g.setGradientFill (juce::ColourGradient (P.blush.withAlpha (mood == 2 ? 0.65f : 0.5f), bx, by,
                                                         P.blush.withAlpha (0.0f), bx + 0.25f * R, by, true));
                g.fillEllipse (bx - 0.25f * R, by - 0.12f * R, 0.5f * R, 0.24f * R);
                g.setColour (P.blush.darker (0.2f).withAlpha (0.7f));
                for (int i = 0; i < 4; ++i)
                {
                    const float x = bx + ((float) i - 1.5f) * 0.055f * R;
                    g.drawLine (x + 0.025f * R, by - 0.05f * R, x - 0.02f * R, by + 0.05f * R, 1.0f);
                }
            }
            if (mood >= 3)
            {
                g.setColour (Colour (0xff1a0612).withAlpha (mood == 4 ? 0.62f : 0.45f));
                g.fillRect (CX - R, HY - R, 2.0f * R, 1.18f * R);
            }
        }
        g.setColour (P.skinLine);
        g.strokePath (face, PathStrokeType (1.3f));
        Path chin;
        chin.addCentredArc (CX, HY + 0.62f * R, 0.4f * R, 0.39f * R, 0.0f, pi * 0.72f, pi * 1.28f, true);
        g.strokePath (chin, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));

        for (int side = -1; side <= 1; side += 2)
            drawEye (g, P, CX + (float) side * 0.36f * R, HY + 0.18f * R, (float) -side, mood, t, side);

        const float tilt[] = { -0.02f, -0.03f, 0.06f, -0.01f, 0.1f };
        for (int side = -1; side <= 1; side += 2)
        {
            const float f = (float) side, inner = -f;
            const float bx = CX + f * 0.36f * R, by = HY - 0.2f * R;
            Path brow;
            brow.startNewSubPath (bx - inner * 0.17f * R, by + 0.02f * R);
            brow.quadraticTo (bx, by - 0.05f * R, bx + inner * 0.15f * R, by + tilt[mood] * R);
            g.setColour (P.hairLine);
            g.strokePath (brow, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        Path nose;
        nose.addTriangle (CX + 0.01f * R, HY + 0.36f * R, CX + 0.05f * R, HY + 0.47f * R, CX - 0.01f * R, HY + 0.47f * R);
        g.setColour (P.skinDeep);
        g.fillPath (nose);

        const float my = HY + 0.7f * R;
        g.setColour (P.skinLine);
        if (mood <= 1)
        {
            Path m;
            m.startNewSubPath (CX - 0.1f * R, my - 0.01f * R);
            m.quadraticTo (CX, my + (mood == 1 ? 0.06f : 0.04f) * R, CX + 0.1f * R, my - 0.01f * R);
            g.strokePath (m, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (P.skinSh);
            g.drawLine (CX - 0.04f * R, my + 0.08f * R, CX + 0.04f * R, my + 0.08f * R, 1.2f);
        }
        else if (mood == 2)
        {
            Path m;
            m.startNewSubPath (CX - 0.08f * R, my + 0.02f * R);
            m.quadraticTo (CX, my - 0.02f * R, CX + 0.08f * R, my + 0.02f * R);
            g.strokePath (m, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
            Path d;
            const float dx = CX + 0.92f * R, dy = HY - 0.25f * R, ds = 0.09f * R;
            d.startNewSubPath (dx, dy - ds * 1.8f);
            d.cubicTo (dx + ds, dy - 0.3f * ds, dx + ds, dy + ds, dx, dy + ds);
            d.cubicTo (dx - ds, dy + ds, dx - ds, dy - 0.3f * ds, dx, dy - ds * 1.8f);
            g.setColour (Colour (0xffd6efff));
            g.fillPath (d);
            g.setColour (Colour (0xff6a9ac8));
            g.strokePath (d, PathStrokeType (1.2f));
            g.setColour (juce::Colours::white);
            g.fillEllipse (dx - ds * 0.5f, dy - ds * 0.2f, ds * 0.4f, ds * 0.5f);
        }
        else
        {
            const float w = (mood == 4 ? 0.48f : 0.32f) * R * (mood == 4 ? 1.0f + 0.2f * pulse : 1.0f);
            const float h = (mood == 4 ? 0.2f : 0.05f) * R * (mood == 4 ? 1.0f + 0.3f * pulse : 1.0f);
            Path m;
            m.startNewSubPath (CX - w * 0.5f, my - 0.05f * R);
            m.quadraticTo (CX, my + 0.03f * R, CX + w * 0.5f, my - 0.05f * R);
            m.quadraticTo (CX, my + h + 0.09f * R, CX - w * 0.5f, my - 0.05f * R);
            m.closeSubPath();
            g.setColour (Colour (0xff4a0c18));
            g.fillPath (m);
            if (mood == 4)
            {
                Graphics::ScopedSaveState s (g);
                g.reduceClipRegion (m);
                g.setColour (juce::Colours::white);
                g.fillRect (CX - w * 0.5f, my - 0.06f * R, w, 0.065f * R);
                g.setColour (Colour (0xffd8607a));
                g.fillEllipse (CX - w * 0.22f, my + h * 0.5f, w * 0.44f, h * 0.6f);
                for (int side = -1; side <= 1; side += 2)
                {
                    Path fang;
                    const float fx = CX + (float) side * w * 0.3f;
                    fang.addTriangle (fx - 0.03f * R, my - 0.01f * R, fx + 0.03f * R, my - 0.01f * R, fx, my + 0.07f * R);
                    g.setColour (juce::Colours::white);
                    g.fillPath (fang);
                }
            }
            g.setColour (P.skinLine);
            g.strokePath (m, PathStrokeType (1.4f));
            if (mood == 4)
                AnimeBlood::drip (g, CX + w * 0.42f, my - 0.03f * R, 0.25f * R + 0.15f * R * (0.5f + 0.5f * std::sin (t * 2.0f)), 3.5f);
        }

        if (mood >= 3)
        {
            AnimeBlood::splat (g, CX + 0.52f * R, HY + 0.58f * R, 0.07f * R * (mood == 4 ? 1.6f : 1.0f), 77, 0.2f * R * rage);
            if (mood == 4)
            {
                AnimeBlood::splat (g, CX - 0.56f * R, HY + 0.64f * R, 0.09f * R, 78, 0.28f * R);
                AnimeBlood::splat (g, CX - 0.12f * R, HY - 0.45f * R, 0.06f * R, 79, 0.42f * R);
            }
        }
    }

    //==========================================================================
    void drawFrontHair (Graphics& g, const Pal& P, float t, float rage)
    {
        Path crown;
        crown.startNewSubPath (CX - 0.96f * R, HY + 0.05f * R);
        crown.lineTo (CX - 0.92f * R, HY - 0.6f * R);
        crown.cubicTo (CX - 1.08f * R, HY - 1.5f * R, CX + 1.08f * R, HY - 1.5f * R, CX + 0.92f * R, HY - 0.6f * R);
        crown.lineTo (CX + 0.96f * R, HY + 0.05f * R);
        crown.cubicTo (CX + 0.6f * R, HY - 0.7f * R, CX - 0.6f * R, HY - 0.7f * R, CX - 0.96f * R, HY + 0.05f * R);
        crown.closeSubPath();
        Path crownSh;
        crownSh.addEllipse (CX + 0.15f * R, HY - 1.2f * R, 1.2f * R, 1.4f * R);
        cel (g, crown, P.hair, crownSh, P.hairSh, P.hairLine, 1.4f);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (crown);
            g.setColour (P.hairLine.withAlpha (0.4f));
            for (int i = 0; i < 12; ++i)
            {
                const float f = (float) i / 11.0f * 2.0f - 1.0f;
                Path sw;
                sw.startNewSubPath (CX + f * 0.1f * R, HY - 1.28f * R);
                sw.quadraticTo (CX + f * 0.75f * R, HY - 1.05f * R, CX + f * 1.0f * R, HY - 0.15f * R);
                g.strokePath (sw, PathStrokeType (0.9f));
            }
        }

        // hime-cut side locks framing the face
        for (int side = -1; side <= 1; side += 2)
        {
            const float f = (float) side;
            Path sl;
            sl.startNewSubPath (CX + f * 0.62f * R, HY - 0.55f * R);
            sl.cubicTo (CX + f * 0.84f * R, HY - 0.3f * R, CX + f * 0.88f * R, HY + 0.4f * R, CX + f * 0.84f * R, HY + 1.0f * R);
            sl.lineTo (CX + f * 0.78f * R, HY + 0.94f * R);
            sl.lineTo (CX + f * 0.72f * R, HY + 1.02f * R);
            sl.lineTo (CX + f * 0.6f * R, HY + 0.96f * R);
            sl.cubicTo (CX + f * 0.64f * R, HY + 0.4f * R, CX + f * 0.62f * R, HY - 0.1f * R, CX + f * 0.5f * R, HY - 0.5f * R);
            sl.closeSubPath();
            cel (g, sl, P.hair, shifted (sl, -f * 0.09f * R), P.hairSh, P.hairLine, 1.3f);
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (sl);
            strandLines (g, sl.getBounds(), P.hairLine.withAlpha (0.45f), 3, f * 3.0f);
        }

        // fringe with separated strands
        Path fringe;
        fringe.startNewSubPath (CX - 0.9f * R, HY - 0.25f * R);
        fringe.cubicTo (CX - 0.95f * R, HY - 1.2f * R, CX + 0.95f * R, HY - 1.2f * R, CX + 0.9f * R, HY - 0.25f * R);
        struct Tip { float x, y; };
        const Tip tips[] = {
            {  0.9f, -0.25f }, {  0.8f, -0.02f }, {  0.72f, -0.16f }, {  0.6f, -0.04f }, {  0.5f, -0.14f },
            {  0.38f, -0.03f }, {  0.28f, -0.12f }, {  0.17f, 0.02f }, {  0.1f, -0.11f }, { 0.02f, 0.14f },
            { -0.04f, -0.1f }, { -0.14f, 0.0f }, { -0.24f, -0.12f }, { -0.36f, -0.02f }, { -0.46f, -0.14f },
            { -0.58f, -0.03f }, { -0.68f, -0.15f }, { -0.78f, -0.01f }, { -0.9f, -0.25f }
        };
        const int nTips = (int) (sizeof (tips) / sizeof (tips[0]));
        for (const auto& tp : tips)
            fringe.lineTo (CX + tp.x * R, HY + tp.y * R + 1.5f * std::sin (t * 1.3f + tp.x * 5.0f));
        fringe.closeSubPath();

        Path fringeSh;
        fringeSh.startNewSubPath (CX - R, HY - 0.32f * R);
        for (int i = 0; i <= 12; ++i)
        {
            const float x = CX - R + 2.0f * R * (float) i / 12.0f;
            fringeSh.lineTo (x, HY + ((i % 2 == 0) ? -0.3f : -0.2f) * R);
        }
        fringeSh.lineTo (CX + R, HY + 0.3f * R);
        fringeSh.lineTo (CX - R, HY + 0.3f * R);
        fringeSh.closeSubPath();
        fringeSh.addEllipse (CX + 0.45f * R, HY - 1.0f * R, 0.7f * R, 1.1f * R);
        cel (g, fringe, P.hair, fringeSh, P.hairSh, P.hairLine, 1.4f);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (fringe);
            g.setColour (P.hairLine.withAlpha (0.6f));
            for (int i = 3; i + 1 < nTips; i += 4)
            {
                Path l;
                l.startNewSubPath (CX + tips[i].x * 0.35f * R, HY - 1.0f * R);
                l.quadraticTo (CX + tips[i].x * 0.8f * R, HY - 0.6f * R, CX + tips[i].x * R, HY + (tips[i].y - 0.08f) * R);
                g.strokePath (l, PathStrokeType (1.0f));
            }
            g.setColour (P.hairDeep.withAlpha (0.7f));
            for (int i = 2; i < nTips; i += 2)
                g.fillEllipse (CX + tips[i].x * R - 0.04f * R, HY + (tips[i].y - 0.04f) * R, 0.08f * R, 0.06f * R);

            // glossy highlight: a broken arc of sharp diamond strokes
            for (int i = 0; i < 11; ++i)
            {
                const float a = -0.95f + 1.9f * (float) i / 10.0f;
                const float x = CX + std::sin (a) * 0.78f * R;
                const float y = HY - 0.7f * R - std::cos (a) * 0.24f * R;
                const float len = (0.12f + 0.06f * std::abs (std::sin ((float) i * 2.3f))) * R;
                const float wd = 0.035f * R;
                Path d;
                d.startNewSubPath (x, y - len * 0.5f);
                d.lineTo (x + wd, y);
                d.lineTo (x, y + len * 0.5f);
                d.lineTo (x - wd, y);
                d.closeSubPath();
                d.applyTransform (juce::AffineTransform::rotation (a * 0.6f, x, y));
                g.setColour (P.hairGlossEdge);
                g.fillPath (d, juce::AffineTransform::translation (0.0f, 2.5f));
                g.setColour (P.hairGloss.withAlpha (1.0f - 0.35f * rage));
                g.fillPath (d);
            }
        }

        // ahoge
        Path ahoge;
        ahoge.startNewSubPath (CX + 0.04f * R, HY - 1.18f * R);
        ahoge.cubicTo (CX + 0.02f * R, HY - 1.6f * R, CX + 0.45f * R + 3.0f * std::sin (t * 2.2f), HY - 1.72f * R, CX + 0.38f * R, HY - 1.38f * R);
        g.setColour (P.hair);
        g.strokePath (ahoge, PathStrokeType (4.5f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (P.hairLine);
        g.strokePath (ahoge, PathStrokeType (1.0f));

        // black velvet ribbon with tails
        {
            const float rx = CX + 0.88f * R, ry = HY - 0.98f * R;
            for (int side = -1; side <= 1; side += 2)
            {
                const float f = (float) side;
                const float sway = 2.5f * std::sin (t * 1.4f + f);
                Path tail;
                tail.startNewSubPath (rx + f * 0.03f * R, ry + 0.04f * R);
                tail.cubicTo (rx + f * 0.12f * R + 0.1f * R, ry + 0.3f * R, rx + f * 0.18f * R + 0.15f * R + sway, ry + 0.5f * R, rx + f * 0.22f * R + 0.2f * R + sway, ry + 0.7f * R);
                tail.lineTo (rx + f * 0.12f * R + 0.2f * R + sway, ry + 0.62f * R);
                tail.lineTo (rx + f * 0.07f * R + 0.2f * R + sway, ry + 0.72f * R);
                tail.cubicTo (rx + f * 0.06f * R + 0.12f * R, ry + 0.45f * R, rx + 0.04f * R, ry + 0.22f * R, rx - f * 0.03f * R, ry + 0.05f * R);
                tail.closeSubPath();
                g.setColour (P.ribbon);
                g.fillPath (tail);
                g.setColour (P.lash);
                g.strokePath (tail, PathStrokeType (1.0f));

                Path loop;
                loop.startNewSubPath (rx, ry);
                loop.cubicTo (rx + f * 0.18f * R, ry - 0.38f * R, rx + f * 0.52f * R, ry - 0.34f * R, rx + f * 0.46f * R, ry - 0.02f * R);
                loop.cubicTo (rx + f * 0.4f * R, ry + 0.15f * R, rx + f * 0.16f * R, ry + 0.1f * R, rx, ry);
                loop.closeSubPath();
                g.setColour (P.ribbon);
                g.fillPath (loop);
                {
                    Graphics::ScopedSaveState s (g);
                    g.reduceClipRegion (loop);
                    g.setColour (P.ribbonSh);
                    g.fillRect (juce::Rectangle<float> (rx, ry - 0.05f * R, f * 0.6f * R, 0.3f * R).withWidth (0.6f * R).withX (f > 0 ? rx : rx - 0.6f * R));
                }
                g.setColour (P.lash);
                g.strokePath (loop, PathStrokeType (1.2f));
                g.setColour (juce::Colours::white.withAlpha (0.55f));
                g.drawLine (rx + f * 0.12f * R, ry - 0.2f * R, rx + f * 0.34f * R, ry - 0.26f * R, 2.0f);
            }
            g.setColour (P.ribbonSh);
            g.fillEllipse (rx - 0.07f * R, ry - 0.08f * R, 0.14f * R, 0.16f * R);
        }

        // crescent moon pin
        {
            const float hx = CX - 0.6f * R, hy = HY - 0.66f * R, hr = 0.13f * R;
            g.setColour (P.silver);
            g.fillEllipse (hx - hr, hy - hr, hr * 2.0f, hr * 2.0f);
            g.setColour (P.hair);
            g.fillEllipse (hx - hr * 0.35f, hy - hr * 1.15f, hr * 1.7f, hr * 1.7f);
            g.setColour (P.hairLine);
            g.drawEllipse (hx - hr, hy - hr, hr * 2.0f, hr * 2.0f, 1.0f);
            Path star;
            star.addStar ({ hx + 0.18f * R, hy + 0.12f * R }, 4, 0.02f * R, 0.075f * R, 0.0f);
            g.setColour (mix (Colour (0xff9fe8ff), Colour (0xffff4050), rage));
            g.fillPath (star);
        }
    }
}

//==============================================================================
void HushGirl::draw (Graphics& g, float t, int mood, float rage, float pulse)
{
    const Pal P = makePal (rage);

    drawBackHair (g, P, t);
    drawBody (g, P, mood, rage);
    drawLongLocks (g, P, t, mood, rage);

    const float tilt = (mood == 4 ? 0.1f : 0.04f) * std::sin (t * (mood == 4 ? 2.4f : 0.7f)) + (mood >= 3 ? 0.08f : 0.03f);
    Graphics::ScopedSaveState s (g);
    g.addTransform (juce::AffineTransform::rotation (tilt, CX, HY + 0.9f * R));
    drawFace (g, P, mood, rage, pulse, t);
    drawFrontHair (g, P, t, rage);
}
