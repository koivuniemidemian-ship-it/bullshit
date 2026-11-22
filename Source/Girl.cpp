#include "Girl.h"
#include <cmath>

using namespace HushGirl;

namespace
{
    using juce::Colour;
    using juce::Graphics;
    using juce::Path;
    using juce::PathStrokeType;

    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float BOTTOM = 760.0f;

    Colour mix (Colour a, Colour b, float t) { return a.interpolatedWith (b, juce::jlimit (0.0f, 1.0f, t)); }

    struct Palette
    {
        Colour hairHi, hairBase, hairMid, hairDark, hairLine, hairTint;
        Colour skin, skinSh, skinSh2, skinLine, blush;
        Colour irisTop, irisMid, irisBot, irisLine;
        Colour cloth, clothHi, clothSh, clothLine;
        Colour lace, laceSh, ribbon, ribbonSh, gold, gem, rim;
    };

    Palette makePalette (float rage)
    {
        Palette p;
        p.hairHi    = mix (Colour (0xffffffff), Colour (0xffffd6d6), rage);
        p.hairBase  = mix (Colour (0xffdfe3f4), Colour (0xffd9a8ae), rage);
        p.hairMid   = mix (Colour (0xffa2aad2), Colour (0xff9a3a48), rage);
        p.hairDark  = mix (Colour (0xff5d6596), Colour (0xff4a0a16), rage);
        p.hairLine  = mix (Colour (0xff363b66), Colour (0xff2a0008), rage);
        p.hairTint  = mix (Colour (0xffa9d4ff), Colour (0xffff2a3a), rage);   // reflected sky / moon light

        p.skin      = mix (Colour (0xfffff3ec), Colour (0xfff7e2de), rage);
        p.skinSh    = mix (Colour (0xfff7cfc4), Colour (0xffe0a8a8), rage);
        p.skinSh2   = mix (Colour (0xffe9ada6), Colour (0xffb86a72), rage);
        p.skinLine  = mix (Colour (0xff9a5a5a), Colour (0xff5a1018), rage);
        p.blush     = mix (Colour (0xffff8fa8), Colour (0xffff3048), rage);

        p.irisTop   = mix (Colour (0xff2a2a8a), Colour (0xff4a0008), rage);
        p.irisMid   = mix (Colour (0xff6a5cff), Colour (0xffd0101e), rage);
        p.irisBot   = mix (Colour (0xff9ff2ff), Colour (0xffffa040), rage);
        p.irisLine  = mix (Colour (0xff15123f), Colour (0xff1a0004), rage);

        p.cloth     = mix (Colour (0xff343a86), Colour (0xff2a0a14), rage);
        p.clothHi   = mix (Colour (0xff5d66c4), Colour (0xff6a1424), rage);
        p.clothSh   = mix (Colour (0xff1b1e4f), Colour (0xff12020a), rage);
        p.clothLine = mix (Colour (0xff0e0f2e), Colour (0xff080002), rage);

        p.lace      = mix (Colour (0xffffffff), Colour (0xfff0d8d8), rage);
        p.laceSh    = mix (Colour (0xffc9cdea), Colour (0xffb08088), rage);
        p.ribbon    = mix (Colour (0xffff5f8f), Colour (0xffb0001a), rage);
        p.ribbonSh  = mix (Colour (0xffc22a5c), Colour (0xff4a0008), rage);
        p.gold      = mix (Colour (0xffffd76a), Colour (0xffff9a5a), rage);
        p.gem       = mix (Colour (0xff6fe0ff), Colour (0xffff1030), rage);
        p.rim       = mix (Colour (0xffcfe6ff), Colour (0xffff2a1a), rage);
        return p;
    }

    const Colour blood   { 0xff9a0012 };
    const Colour bloodHi { 0xffe0182e };
    const Colour bloodDk { 0xff4a0008 };

    // Soft airbrushed blob (radial gradient fading to transparent)
    void softBlob (Graphics& g, float x, float y, float rx, float ry, Colour c)
    {
        juce::ColourGradient gr (c, x, y, c.withAlpha (0.0f), x + rx, y, true);
        g.setGradientFill (gr);
        g.fillEllipse (x - rx, y - ry, rx * 2.0f, ry * 2.0f);
    }

    // A hair clump: tapered, slightly bulging, curved
    Path clump (float x0, float y0, float x1, float y1, float w, float bend, float bulge = 1.0f)
    {
        Path p;
        const float dy = y1 - y0;
        const float hw = w * 0.5f;
        p.startNewSubPath (x0 - hw, y0);
        p.cubicTo (x0 - hw * bulge + bend * 0.35f, y0 + dy * 0.3f,
                   x1 - hw * 0.45f + bend,          y0 + dy * 0.75f,
                   x1, y1);
        p.cubicTo (x1 + hw * 0.45f + bend,          y0 + dy * 0.75f,
                   x0 + hw * bulge + bend * 0.35f, y0 + dy * 0.3f,
                   x0 + hw, y0);
        p.closeSubPath();
        return p;
    }

    void paintClump (Graphics& g, const Path& p, const Palette& P, float yTop, float yBot, bool front, float shadeSide)
    {
        const auto b = p.getBounds();
        juce::ColourGradient gr (front ? P.hairBase : P.hairMid, 0.0f, yTop, P.hairDark, 0.0f, yBot, false);
        gr.addColour (0.35, front ? P.hairBase.interpolatedWith (P.hairMid, 0.3f) : P.hairMid);
        g.setGradientFill (gr);
        g.fillPath (p);

        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (p);
            // soft side shading (light from the upper left)
            const float lx = shadeSide > 0.0f ? b.getX() : b.getRight();
            const float dx = shadeSide > 0.0f ? b.getRight() : b.getX();
            juce::ColourGradient side (P.hairTint.withAlpha (0.18f), lx, 0.0f, P.hairDark.withAlpha (0.55f), dx, 0.0f, false);
            side.addColour (0.45, juce::Colours::transparentBlack);
            g.setGradientFill (side);
            g.fillRect (b);
            // fine strand lines
            g.setColour (P.hairDark.withAlpha (0.4f));
            const float cx = b.getCentreX();
            for (int i = -1; i <= 1; ++i)
                g.drawLine (cx + (float) i * b.getWidth() * 0.16f, b.getY() + b.getHeight() * 0.2f,
                            cx + (float) i * b.getWidth() * 0.08f, b.getBottom() - b.getHeight() * 0.12f, 0.8f);
            g.setColour (P.hairHi.withAlpha (0.35f));
            g.drawLine (cx - shadeSide * b.getWidth() * 0.2f, b.getY() + b.getHeight() * 0.1f,
                        cx - shadeSide * b.getWidth() * 0.12f, b.getY() + b.getHeight() * 0.45f, 1.2f);
        }
        g.setColour (P.hairLine.withAlpha (0.85f));
        g.strokePath (p, PathStrokeType (1.3f));
    }

    Path splatPath (float x, float y, float r, int seed)
    {
        juce::Random rr (seed);
        Path p;
        const int n = 16;
        for (int i = 0; i < n; ++i)
        {
            const float a = 2.0f * pi * (float) i / (float) n;
            float rad = r * (0.65f + 0.45f * rr.nextFloat());
            if (i % 3 == 0) rad *= 1.35f + 0.45f * rr.nextFloat();
            const float px = x + std::cos (a) * rad, py = y + std::sin (a) * rad;
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
        p.closeSubPath();
        p = p.createPathWithRoundedCorners (r * 0.35f);
        for (int j = 0; j < 6; ++j)
        {
            const float a = rr.nextFloat() * 2.0f * pi;
            const float d = r * (1.3f + 1.2f * rr.nextFloat());
            const float s = r * (0.06f + 0.14f * rr.nextFloat());
            p.addEllipse (x + std::cos (a) * d - s, y + std::sin (a) * d - s, s * 2.0f, s * 2.0f);
        }
        return p;
    }

    void bloodSplat (Graphics& g, float x, float y, float r, int seed, float drip = 0.0f, float alpha = 1.0f)
    {
        auto p = splatPath (x, y, r, seed);
        if (drip > 0.0f)
        {
            const float dw = juce::jmax (2.5f, r * 0.3f);
            p.addRoundedRectangle (x - dw * 0.5f, y, dw, r + drip, dw * 0.5f);
            p.addEllipse (x - dw * 0.8f, y + r + drip - dw * 0.6f, dw * 1.6f, dw * 1.8f);
        }
        g.setColour (bloodDk.withAlpha (alpha));
        g.fillPath (p, juce::AffineTransform::translation (1.0f, 1.5f));
        g.setColour (blood.withAlpha (alpha));
        g.fillPath (p);
        g.setColour (bloodHi.withAlpha (0.8f * alpha));
        g.fillPath (splatPath (x - r * 0.1f, y - r * 0.1f, r * 0.5f, seed + 3));
        g.setColour (juce::Colours::white.withAlpha (0.6f * alpha));
        g.fillEllipse (x - r * 0.4f, y - r * 0.45f, r * 0.25f, r * 0.14f);
    }

    void drip (Graphics& g, float x, float y, float len, float w)
    {
        g.setColour (blood);
        g.fillRoundedRectangle (x - w * 0.5f, y, w, len, w * 0.5f);
        g.fillEllipse (x - w * 0.9f, y + len - w * 0.8f, w * 1.8f, w * 2.0f);
        g.setColour (bloodHi);
        g.fillRoundedRectangle (x - w * 0.15f, y, w * 0.3f, len * 0.85f, w * 0.15f);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.fillEllipse (x - w * 0.5f, y + len - w * 0.3f, w * 0.45f, w * 0.4f);
    }

    //==========================================================================
    void drawBackHair (Graphics& g, const Palette& P, float t)
    {
        const float sway = 6.0f * std::sin (t * 0.9f);

        Path back;
        back.startNewSubPath (CX - 0.92f * R, HY - 0.6f * R);
        back.cubicTo (CX - 1.08f * R, HY - 1.5f * R, CX + 1.08f * R, HY - 1.5f * R, CX + 0.92f * R, HY - 0.6f * R);
        back.cubicTo (CX + 1.4f * R, HY + 0.6f * R, CX + 1.8f * R + sway, HY + 2.8f * R, CX + 1.95f * R + sway, BOTTOM);
        back.lineTo (CX - 1.95f * R + sway, BOTTOM);
        back.cubicTo (CX - 1.8f * R + sway, HY + 2.8f * R, CX - 1.4f * R, HY + 0.6f * R, CX - 0.92f * R, HY - 0.6f * R);
        back.closeSubPath();
        g.setGradientFill (juce::ColourGradient (P.hairMid, CX, HY - R, P.hairDark.darker (0.3f), CX, BOTTOM, false));
        g.fillPath (back);

        // layered clumps fanning out, clipped to the hair silhouette
        Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (back);
        const int n = 18;
        for (int pass = 0; pass < 2; ++pass)
            for (int i = 0; i < n; ++i)
            {
                if ((i % 2) != pass) continue;
                const float f = (float) i / (float) (n - 1) * 2.0f - 1.0f;
                const float ph = (float) i * 1.37f;
                const float bend = sway * (0.5f + 0.5f * std::sin (t * 1.1f + ph)) + f * 18.0f;
                const float x0 = CX + f * 0.85f * R;
                const float x1 = CX + f * (1.55f + 0.25f * std::sin (ph)) * R + bend;
                const float y1 = HY + (3.4f + 1.6f * (0.5f + 0.5f * std::sin (ph * 2.1f))) * R;
                const float y0 = HY - 1.7f * R;
                auto c = clump (x0, y0, x1, juce::jmin (y1, BOTTOM + 40.0f), (0.5f + 0.15f * std::cos (ph)) * R, bend, 1.15f);
                paintClump (g, c, P, HY - R, y1, pass == 1, f < 0.0f ? 1.0f : -1.0f);
            }
    }

    void drawBody (Graphics& g, const Palette& P, int mood, float rage, float t)
    {
        // ---- neck
        Path neck;
        neck.startNewSubPath (CX - 0.25f * R, HY + 0.7f * R);
        neck.lineTo (CX - 0.28f * R, HY + 1.55f * R);
        neck.lineTo (CX + 0.28f * R, HY + 1.55f * R);
        neck.lineTo (CX + 0.25f * R, HY + 0.7f * R);
        neck.closeSubPath();
        g.setColour (P.skin);
        g.fillPath (neck);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (neck);
            g.setGradientFill (juce::ColourGradient (P.skinSh2, CX, HY + 0.75f * R, P.skinSh.withAlpha (0.0f), CX, HY + 1.25f * R, false));
            g.fillRect (CX - R, HY + 0.6f * R, 2.0f * R, 0.8f * R);
            softBlob (g, CX + 0.25f * R, HY + 1.1f * R, 0.2f * R, 0.5f * R, P.skinSh.withAlpha (0.8f));
        }
        g.setColour (P.skinLine.withAlpha (0.7f));
        g.drawLine (CX - 0.25f * R, HY + 0.75f * R, CX - 0.28f * R, HY + 1.45f * R, 1.4f);
        g.drawLine (CX + 0.25f * R, HY + 0.75f * R, CX + 0.28f * R, HY + 1.45f * R, 1.4f);

        // ---- torso silhouette (dress), arms resting at her sides
        Path body;
        body.startNewSubPath (CX - 0.42f * R, HY + 1.38f * R);
        body.cubicTo (CX - 0.9f * R, HY + 1.42f * R, CX - 1.38f * R, HY + 1.55f * R, CX - 1.58f * R, HY + 1.95f * R);
        body.cubicTo (CX - 1.78f * R, HY + 2.5f * R, CX - 1.78f * R, HY + 3.6f * R, CX - 1.72f * R, BOTTOM);
        body.lineTo (CX + 1.72f * R, BOTTOM);
        body.cubicTo (CX + 1.78f * R, HY + 3.6f * R, CX + 1.78f * R, HY + 2.5f * R, CX + 1.58f * R, HY + 1.95f * R);
        body.cubicTo (CX + 1.38f * R, HY + 1.55f * R, CX + 0.9f * R, HY + 1.42f * R, CX + 0.42f * R, HY + 1.38f * R);
        body.closeSubPath();

        g.setGradientFill (juce::ColourGradient (P.clothHi, CX - 1.2f * R, HY + 1.5f * R, P.cloth, CX + 0.5f * R, HY + 3.5f * R, false));
        g.fillPath (body);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (body);

            // arms: separated from torso by soft shadow folds
            for (int side = -1; side <= 1; side += 2)
            {
                const float f = (float) side;
                Path seam;
                seam.startNewSubPath (CX + f * 1.15f * R, HY + 2.25f * R);
                seam.cubicTo (CX + f * 1.2f * R, HY + 3.0f * R, CX + f * 1.22f * R, HY + 4.0f * R, CX + f * 1.25f * R, BOTTOM);
                g.setColour (P.clothSh.withAlpha (0.8f));
                g.strokePath (seam, PathStrokeType (0.16f * R));
                g.setColour (P.clothLine.withAlpha (0.8f));
                g.strokePath (seam, PathStrokeType (1.4f));
                // sleeve highlight
                softBlob (g, CX + f * 1.5f * R, HY + 2.9f * R, 0.18f * R, 0.9f * R, P.clothHi.withAlpha (f < 0.0f ? 0.7f : 0.25f));
            }

            // big soft shadow on the side away from the light, under the collar
            softBlob (g, CX + 1.1f * R, HY + 3.2f * R, 1.0f * R, 2.2f * R, P.clothSh.withAlpha (0.75f));
            softBlob (g, CX, HY + 1.75f * R, 1.0f * R, 0.35f * R, P.clothSh.withAlpha (0.7f));

            // vertical fabric folds
            for (int i = -3; i <= 3; ++i)
            {
                if (i == 0) continue;
                const float x = CX + (float) i * 0.26f * R;
                Path fold;
                fold.startNewSubPath (x, HY + 3.0f * R);
                fold.quadraticTo (x + (float) i * 3.0f, HY + 4.0f * R, x + (float) i * 6.0f, BOTTOM);
                g.setColour (P.clothSh.withAlpha (0.35f));
                g.strokePath (fold, PathStrokeType (5.0f));
                g.setColour (P.clothHi.withAlpha (0.18f));
                g.strokePath (fold, PathStrokeType (1.5f), juce::AffineTransform::translation (4.0f, 0.0f));
            }

            // corset lacing panel down the front
            const juce::Rectangle<float> panel (CX - 0.3f * R, HY + 2.55f * R, 0.6f * R, BOTTOM - (HY + 2.55f * R));
            g.setColour (P.clothSh.withAlpha (0.55f));
            g.fillRect (panel);
            g.setColour (P.clothLine.withAlpha (0.8f));
            g.drawRect (panel, 1.2f);
            for (int i = 0; i < 6; ++i)
            {
                const float y = panel.getY() + 14.0f + (float) i * 26.0f;
                g.setColour (P.lace.withAlpha (0.85f));
                g.drawLine (panel.getX() + 6.0f, y, panel.getRight() - 6.0f, y + 13.0f, 1.6f);
                g.drawLine (panel.getRight() - 6.0f, y, panel.getX() + 6.0f, y + 13.0f, 1.6f);
                g.setColour (P.gold);
                g.fillEllipse (panel.getX() + 3.0f, y - 2.5f, 5.0f, 5.0f);
                g.fillEllipse (panel.getRight() - 8.0f, y - 2.5f, 5.0f, 5.0f);
            }

            // blood soaking the dress
            if (rage > 0.0f)
            {
                juce::Random st (404);
                const int n = (int) (rage * 12.0f);
                for (int i = 0; i < n; ++i)
                    bloodSplat (g, CX + (st.nextFloat() - 0.5f) * 3.0f * R, HY + (2.0f + st.nextFloat() * 4.2f) * R,
                                (0.08f + 0.16f * st.nextFloat()) * R, 1000 + i, rage * (8.0f + 45.0f * st.nextFloat()));
            }
        }
        // moon rim light along the right edge
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (juce::Rectangle<int> ((int) CX, 0, 400, 800));
            g.setColour (P.rim.withAlpha (0.35f + 0.4f * rage));
            g.strokePath (body, PathStrokeType (3.0f), juce::AffineTransform::translation (-2.0f, 0.0f));
        }
        g.setColour (P.clothLine);
        g.strokePath (body, PathStrokeType (2.0f));

        // ---- puffed shoulders
        for (int side = -1; side <= 1; side += 2)
        {
            const float f = (float) side;
            const float sx = CX + f * 1.4f * R, sy = HY + 1.98f * R;
            Path puff;
            puff.addEllipse (sx - 0.36f * R, sy - 0.34f * R, 0.72f * R, 0.62f * R);
            g.setGradientFill (juce::ColourGradient (P.clothHi.brighter (0.1f), sx - f * 0.2f * R, sy - 0.3f * R, P.clothSh, sx + f * 0.3f * R, sy + 0.4f * R, false));
            g.fillPath (puff);
            {
                Graphics::ScopedSaveState s (g);
                g.reduceClipRegion (puff);
                for (int i = -2; i <= 2; ++i)   // gathers
                {
                    Path gather;
                    gather.startNewSubPath (sx + (float) i * 0.12f * R, sy - 0.36f * R);
                    gather.quadraticTo (sx + (float) i * 0.2f * R, sy, sx + (float) i * 0.16f * R, sy + 0.34f * R);
                    g.setColour (P.clothSh.withAlpha (0.55f));
                    g.strokePath (gather, PathStrokeType (2.5f));
                    g.setColour (P.clothHi.withAlpha (0.35f));
                    g.strokePath (gather, PathStrokeType (1.0f), juce::AffineTransform::translation (2.5f, 0.0f));
                }
            }
            g.setColour (P.clothLine);
            g.strokePath (puff, PathStrokeType (1.8f));
            // lace trim along the bottom of the puff
            Path trim;
            trim.addCentredArc (sx, sy - 0.03f * R, 0.35f * R, 0.3f * R, 0.0f, pi * 0.62f, pi * 1.38f, true);
            g.setColour (P.lace);
            g.strokePath (trim, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (P.laceSh);
            g.strokePath (trim, PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, 2.0f));
        }

        // ---- tall lace stand collar
        Path collar;
        collar.addRoundedRectangle (CX - 0.38f * R, HY + 1.0f * R, 0.76f * R, 0.5f * R, 0.08f * R);
        g.setGradientFill (juce::ColourGradient (P.lace, CX - 0.3f * R, HY + R, P.laceSh, CX + 0.4f * R, HY + 1.5f * R, false));
        g.fillPath (collar);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (collar);
            g.setColour (P.laceSh.withAlpha (0.7f));
            for (int i = 1; i < 9; ++i)
                g.drawLine (CX - 0.38f * R + (float) i * 0.084f * R, HY + 1.05f * R, CX - 0.38f * R + (float) i * 0.084f * R, HY + 1.45f * R, 0.8f);
            softBlob (g, CX, HY + 1.02f * R, 0.5f * R, 0.12f * R, P.skinSh2.withAlpha (0.5f));   // jaw shadow
        }
        g.setColour (P.laceSh.darker (0.4f));
        g.strokePath (collar, PathStrokeType (1.3f));
        for (int i = 0; i < 10; ++i)   // scalloped top edge
        {
            const float x = CX - 0.38f * R + (float) i * 0.085f * R;
            g.setColour (P.lace);
            g.fillEllipse (x - 0.01f * R, HY + 0.95f * R, 0.1f * R, 0.1f * R);
            g.setColour (P.laceSh);
            g.drawEllipse (x - 0.01f * R, HY + 0.95f * R, 0.1f * R, 0.1f * R, 0.7f);
        }

        // ---- lace edge under the collar
        for (int i = 0; i < 9; ++i)
        {
            const float x = CX - 0.4f * R + (float) i * 0.1f * R;
            g.setColour (P.lace);
            g.fillEllipse (x - 0.01f * R, HY + 1.44f * R, 0.12f * R, 0.1f * R);
            g.setColour (P.laceSh);
            g.drawEllipse (x - 0.01f * R, HY + 1.44f * R, 0.12f * R, 0.1f * R, 0.7f);
        }

        // ---- ribbon bow at the throat with a crescent-moon brooch
        const float bx = CX, by = HY + 1.38f * R;
        for (int side = -1; side <= 1; side += 2)
        {
            const float f = (float) side;
            Path loop;
            loop.startNewSubPath (bx, by);
            loop.cubicTo (bx + f * 0.25f * R, by - 0.32f * R, bx + f * 0.62f * R, by - 0.22f * R, bx + f * 0.5f * R, by + 0.04f * R);
            loop.cubicTo (bx + f * 0.42f * R, by + 0.2f * R, bx + f * 0.2f * R, by + 0.14f * R, bx, by);
            loop.closeSubPath();
            g.setGradientFill (juce::ColourGradient (P.ribbon.brighter (0.2f), bx, by - 0.2f * R, P.ribbonSh, bx + f * 0.5f * R, by + 0.15f * R, false));
            g.fillPath (loop);
            g.setColour (P.ribbonSh.darker (0.4f));
            g.strokePath (loop, PathStrokeType (1.3f));

            const float sway = 3.0f * std::sin (t * 1.4f + f);
            Path tail;
            tail.startNewSubPath (bx + f * 0.04f * R, by + 0.05f * R);
            tail.cubicTo (bx + f * 0.12f * R, by + 0.4f * R, bx + f * 0.2f * R + sway, by + 0.8f * R, bx + f * 0.24f * R + sway, by + 1.05f * R);
            tail.lineTo (bx + f * 0.12f * R + sway, by + 0.95f * R);
            tail.lineTo (bx + f * 0.06f * R + sway, by + 1.08f * R);
            tail.cubicTo (bx + f * 0.06f * R, by + 0.7f * R, bx, by + 0.35f * R, bx - f * 0.04f * R, by + 0.06f * R);
            tail.closeSubPath();
            g.setGradientFill (juce::ColourGradient (P.ribbon, bx, by, P.ribbonSh, bx, by + R, false));
            g.fillPath (tail);
            g.setColour (P.ribbonSh.darker (0.4f));
            g.strokePath (tail, PathStrokeType (1.2f));
        }
        const float mr = 0.13f * R;
        g.setColour (P.gold);
        g.fillEllipse (bx - mr, by - mr, mr * 2.0f, mr * 2.0f);
        g.setColour (P.gem);
        g.fillEllipse (bx - mr * 0.55f, by - mr * 0.55f, mr * 1.1f, mr * 1.1f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (bx - mr * 0.4f, by - mr * 0.45f, mr * 0.4f, mr * 0.3f);
        g.setColour (P.gold.darker (0.5f));
        g.drawEllipse (bx - mr, by - mr, mr * 2.0f, mr * 2.0f, 1.2f);

        if (mood >= 3)   // collar splattered
        {
            bloodSplat (g, CX + 0.2f * R, HY + 1.2f * R, 0.09f * R, 55, rage * 0.3f * R);
            bloodSplat (g, CX - 0.25f * R, HY + 1.8f * R, 0.08f * R, 56, rage * 0.25f * R);
        }
    }

    void drawSideLocks (Graphics& g, const Palette& P, float t, int mood, float rage)
    {
        const float sway = 4.0f * std::sin (t * 1.0f);
        for (int side = -1; side <= 1; side += 2)
        {
            const float f = (float) side;
            for (int j = 0; j < 3; ++j)
            {
                const float x0 = CX + f * (0.8f - 0.1f * (float) j) * R;
                const float bend = sway * (0.6f + 0.2f * (float) j) + f * (10.0f + 6.0f * (float) j);
                const float y1 = HY + (3.6f - 0.7f * (float) j) * R;
                const float x1 = CX + f * (1.05f + 0.12f * (float) j) * R + bend * 0.3f;
                auto c = clump (x0, HY - 0.35f * R, x1, y1, (0.3f - 0.04f * (float) j) * R, bend, 1.2f);
                paintClump (g, c, P, HY - 0.4f * R, y1, true, -f);
                if (mood >= 3 && j == 0)
                    drip (g, x1, y1 - 4.0f, rage * (10.0f + 14.0f * (0.5f + 0.5f * std::sin (t * 2.0f + f))), 4.0f);
            }
        }
    }

    //==========================================================================
    void drawEye (Graphics& g, const Palette& P, float ex, float ey, float inner, int mood, float t, float rage, int side)
    {
        const float ox = ex - inner * 0.25f * R, ix = ex + inner * 0.21f * R;

        if (mood == 0)   // gently closed, smiling
        {
            Path closed;
            closed.startNewSubPath (ox, ey);
            closed.quadraticTo (ex, ey + 0.13f * R, ix, ey - 0.01f * R);
            g.setColour (P.irisLine);
            g.strokePath (closed, PathStrokeType (3.2f, PathStrokeType::curved, PathStrokeType::rounded));
            for (int l = 0; l < 4; ++l)
            {
                const float lx = ox + (ix - ox) * (0.12f + 0.12f * (float) l);
                const float ly = ey + 0.06f * R * (float) l / 3.0f + 0.02f * R;
                g.drawLine (lx, ly, lx - inner * 0.03f * R, ly + 0.08f * R, 1.3f);
            }
            g.drawLine (ox, ey, ox - inner * 0.07f * R, ey - 0.05f * R, 2.2f);
            return;
        }

        Path eye;
        eye.startNewSubPath (ix, ey + 0.02f * R);
        eye.cubicTo (ix - inner * 0.05f * R, ey - 0.2f * R, ox + inner * 0.12f * R, ey - 0.3f * R, ox, ey - 0.08f * R);
        eye.cubicTo (ox + inner * 0.02f * R, ey + 0.12f * R, ex - inner * 0.05f * R, ey + 0.26f * R, ex + inner * 0.06f * R, ey + 0.23f * R);
        eye.cubicTo (ix - inner * 0.04f * R, ey + 0.2f * R, ix, ey + 0.1f * R, ix, ey + 0.02f * R);
        eye.closeSubPath();

        g.setGradientFill (juce::ColourGradient (Colour (0xffd5d8ef), ex, ey - 0.25f * R, juce::Colours::white, ex, ey + 0.05f * R, false));
        g.fillPath (eye);

        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (eye);

            const float icx = ex + inner * 0.01f * R, icy = ey + 0.01f * R;
            const float iw = 0.36f * R, ih = 0.46f * R;
            const juce::Rectangle<float> irisR (icx - iw * 0.5f, icy - ih * 0.5f, iw, ih);

            juce::ColourGradient ig (P.irisTop, icx, irisR.getY(), P.irisBot, icx, irisR.getBottom(), false);
            ig.addColour (0.55, P.irisMid);
            g.setGradientFill (ig);
            g.fillEllipse (irisR);

            softBlob (g, icx, icy + ih * 0.28f, iw * 0.45f, ih * 0.22f, P.irisBot.withAlpha (0.85f));   // light pool

            g.setColour (P.irisTop.withAlpha (0.4f));   // striations
            for (int l = 0; l < 18; ++l)
            {
                const float a = 2.0f * pi * (float) l / 18.0f;
                g.drawLine (icx + std::cos (a) * iw * 0.14f, icy + std::sin (a) * ih * 0.12f,
                            icx + std::cos (a) * iw * 0.47f, icy + std::sin (a) * ih * 0.47f, 0.7f);
            }

            // pupil
            if (mood == 4)
            {
                g.setColour (Colour (0xff0a0002));
                g.fillEllipse (icx - 0.025f * R, icy - ih * 0.34f, 0.05f * R, ih * 0.68f);
            }
            else
            {
                const float pw = (mood == 3 ? 0.06f : 0.14f) * R, ph = (mood == 3 ? 0.08f : 0.2f) * R;
                g.setGradientFill (juce::ColourGradient (P.irisLine, icx, icy - ph, P.irisTop, icx, icy + ph, false));
                g.fillEllipse (icx - pw * 0.5f, icy - ph * 0.5f, pw, ph);
            }

            if (mood == 3)   // empty yandere rings
            {
                g.setColour (P.irisLine.withAlpha (0.6f));
                g.drawEllipse (irisR.reduced (iw * 0.17f, ih * 0.17f), 1.2f);
                g.drawEllipse (irisR.reduced (iw * 0.3f, ih * 0.3f), 1.0f);
            }

            g.setColour (P.irisLine);
            g.drawEllipse (irisR, 1.8f);

            // shadow cast by the upper lid
            g.setGradientFill (juce::ColourGradient (Colour (0xff1a1040).withAlpha (0.65f), ex, ey - 0.3f * R,
                                                     Colour (0x001a1040), ex, ey - 0.02f * R, false));
            g.fillRect (ex - 0.4f * R, ey - 0.4f * R, 0.8f * R, 0.4f * R);

            // catchlights
            if (mood <= 2)
            {
                g.setColour (juce::Colours::white);
                g.fillEllipse (icx - iw * 0.42f, icy - ih * 0.34f, iw * 0.36f, ih * 0.22f);
                g.fillEllipse (icx + iw * 0.12f, icy + ih * 0.16f, iw * 0.13f, iw * 0.13f);
                g.setColour (juce::Colours::white.withAlpha (0.8f));
                g.fillEllipse (icx + iw * 0.22f, icy - ih * 0.18f, iw * 0.08f, iw * 0.08f);
                Path tw;
                tw.addStar ({ icx - iw * 0.08f, icy + ih * 0.26f }, 4, 0.8f, 3.5f + 1.0f * std::sin (t * 4.0f + (float) side), 0.0f);
                g.fillPath (tw);
            }
            else if (mood == 3)
            {
                g.setColour (juce::Colours::white.withAlpha (0.5f));
                g.fillEllipse (icx - iw * 0.32f, icy - ih * 0.28f, iw * 0.12f, iw * 0.1f);
            }

            if (mood == 2)   // half-lidded, unimpressed
            {
                g.setColour (P.skin);
                g.fillRect (ex - 0.4f * R, ey - 0.4f * R, 0.8f * R, 0.3f * R);
                g.setColour (P.skinSh);
                g.fillRect (ex - 0.4f * R, ey - 0.12f * R, 0.8f * R, 0.03f * R);
            }
        }

        if (mood == 4)   // glow
            for (int i = 3; i >= 1; --i)
                softBlob (g, ex, ey, 0.2f * R + (float) i * 6.0f, 0.24f * R + (float) i * 5.0f, bloodHi.withAlpha (0.12f));

        // ---- lash line: thick tapered upper lid with flicks
        const float lidDrop = (mood == 2) ? 0.1f * R : 0.0f;
        Path lash;
        lash.startNewSubPath (ix + inner * 0.01f * R, ey + 0.03f * R + lidDrop * 0.3f);
        lash.cubicTo (ix - inner * 0.05f * R, ey - 0.21f * R + lidDrop, ox + inner * 0.12f * R, ey - 0.33f * R + lidDrop, ox - inner * 0.05f * R, ey - 0.1f * R + lidDrop);
        lash.lineTo (ox - inner * 0.1f * R, ey - 0.16f * R + lidDrop);
        lash.cubicTo (ox + inner * 0.1f * R, ey - 0.4f * R + lidDrop, ix - inner * 0.02f * R, ey - 0.27f * R + lidDrop, ix + inner * 0.01f * R, ey + 0.03f * R + lidDrop * 0.3f);
        lash.closeSubPath();
        g.setColour (P.irisLine);
        g.fillPath (lash);
        for (int l = 0; l < 4; ++l)   // flicks
        {
            const float bx = ox + inner * (0.02f + 0.05f * (float) l) * R;
            const float by = ey - (0.12f + 0.06f * (float) l) * R + lidDrop;
            Path flick;
            flick.startNewSubPath (bx, by);
            flick.quadraticTo (bx - inner * 0.06f * R, by - 0.06f * R, bx - inner * (0.13f - 0.02f * (float) l) * R, by - (0.06f + 0.015f * (float) l) * R);
            g.strokePath (flick, PathStrokeType (2.2f - 0.3f * (float) l, PathStrokeType::curved, PathStrokeType::rounded));
        }
        // lower lashes
        Path lower;
        lower.startNewSubPath (ox + inner * 0.02f * R, ey + 0.02f * R);
        lower.quadraticTo (ox + inner * 0.04f * R, ey + 0.18f * R, ex + inner * 0.02f * R, ey + 0.24f * R);
        g.setColour (P.irisLine.withAlpha (0.75f));
        g.strokePath (lower, PathStrokeType (1.4f));
        for (int l = 0; l < 3; ++l)
        {
            const float lx = ox + inner * (0.05f + 0.06f * (float) l) * R, ly = ey + (0.1f + 0.05f * (float) l) * R;
            g.drawLine (lx, ly, lx - inner * 0.02f * R, ly + 0.05f * R, 1.0f);
        }
        // double eyelid crease
        Path crease;
        crease.startNewSubPath (ix - inner * 0.04f * R, ey - 0.2f * R + lidDrop);
        crease.quadraticTo (ex, ey - 0.42f * R + lidDrop, ox + inner * 0.02f * R, ey - 0.22f * R + lidDrop);
        g.setColour (P.skinLine.withAlpha (0.45f));
        g.strokePath (crease, PathStrokeType (1.1f));

        if (mood == 4)   // blood tears
        {
            const float len = 0.65f * R + 0.35f * R * (0.5f + 0.5f * std::sin (t * 1.6f + (float) side));
            drip (g, ex + inner * 0.04f * R, ey + 0.2f * R, len, 4.5f);
        }
    }

    void drawFace (Graphics& g, const Palette& P, int mood, float rage, float pulse, float t)
    {
        // ---- face shape
        Path face;
        face.startNewSubPath (CX - 0.8f * R, HY - 0.45f * R);
        face.cubicTo (CX - 0.82f * R, HY + 0.1f * R, CX - 0.7f * R, HY + 0.5f * R, CX - 0.44f * R, HY + 0.78f * R);
        face.cubicTo (CX - 0.26f * R, HY + 0.96f * R, CX - 0.08f * R, HY + 1.02f * R, CX, HY + 1.03f * R);
        face.cubicTo (CX + 0.08f * R, HY + 1.02f * R, CX + 0.26f * R, HY + 0.96f * R, CX + 0.44f * R, HY + 0.78f * R);
        face.cubicTo (CX + 0.7f * R, HY + 0.5f * R, CX + 0.82f * R, HY + 0.1f * R, CX + 0.8f * R, HY - 0.45f * R);
        face.cubicTo (CX + 0.8f * R, HY - 1.1f * R, CX - 0.8f * R, HY - 1.1f * R, CX - 0.8f * R, HY - 0.45f * R);
        face.closeSubPath();

        g.setGradientFill (juce::ColourGradient (P.skin, CX - 0.3f * R, HY - 0.3f * R, P.skin.interpolatedWith (P.skinSh, 0.35f), CX + 0.8f * R, HY + R, false));
        g.fillPath (face);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (face);
            // hair shadow on the forehead and soft side shading
            g.setGradientFill (juce::ColourGradient (P.skinSh2.withAlpha (0.8f), CX, HY - 0.5f * R, P.skinSh.withAlpha (0.0f), CX, HY - 0.05f * R, false));
            g.fillRect (CX - R, HY - 1.2f * R, 2.0f * R, 1.2f * R);
            softBlob (g, CX + 0.75f * R, HY + 0.2f * R, 0.3f * R, 0.8f * R, P.skinSh.withAlpha (0.8f));
            softBlob (g, CX - 0.78f * R, HY + 0.1f * R, 0.15f * R, 0.6f * R, P.skinSh.withAlpha (0.5f));
            // blush with airbrush gradient
            const float blushA = (mood == 0 || mood == 1) ? 0.7f : (mood == 2 ? 0.85f : 0.6f);
            softBlob (g, CX - 0.46f * R, HY + 0.5f * R, 0.26f * R, 0.13f * R, P.blush.withAlpha (blushA));
            softBlob (g, CX + 0.46f * R, HY + 0.5f * R, 0.26f * R, 0.13f * R, P.blush.withAlpha (blushA));

            if (mood >= 3)   // yandere shadow over the eyes
            {
                g.setGradientFill (juce::ColourGradient (Colour (0xff12000c).withAlpha (mood == 4 ? 0.8f : 0.6f), CX, HY - 0.5f * R,
                                                         Colour (0x0012000c), CX, HY + 0.4f * R, false));
                g.fillRect (CX - R, HY - R, 2.0f * R, 1.5f * R);
            }
        }
        g.setColour (P.skinLine.withAlpha (0.8f));
        g.strokePath (face, PathStrokeType (1.6f));

        // blush hatching
        g.setColour (juce::Colours::white.withAlpha (0.65f));
        for (int side = -1; side <= 1; side += 2)
            for (int i = 0; i < 4; ++i)
            {
                const float x = CX + (float) side * 0.46f * R + ((float) i - 1.5f) * 0.06f * R;
                g.drawLine (x + 0.025f * R, HY + 0.45f * R, x - 0.02f * R, HY + 0.54f * R, 1.0f);
            }

        // ---- eyes
        for (int side = -1; side <= 1; side += 2)
            drawEye (g, P, CX + (float) side * 0.37f * R, HY + 0.17f * R, (float) -side, mood, t, rage, side);

        // ---- brows (thin, silver)
        const float browTilt[] = { -0.02f, -0.03f, 0.06f, -0.01f, 0.09f };
        for (int side = -1; side <= 1; side += 2)
        {
            const float f = (float) side, inner = -f;
            const float bx = CX + f * 0.37f * R, by = HY - 0.25f * R;
            Path brow;
            brow.startNewSubPath (bx - inner * 0.18f * R, by + 0.02f * R);
            brow.quadraticTo (bx, by - 0.06f * R, bx + inner * 0.16f * R, by + browTilt[mood] * R);
            g.setColour (P.hairLine.withAlpha (0.8f));
            g.strokePath (brow, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // ---- nose
        g.setColour (P.skinSh2);
        Path nose;
        nose.startNewSubPath (CX + 0.02f * R, HY + 0.4f * R);
        nose.quadraticTo (CX + 0.05f * R, HY + 0.47f * R, CX - 0.01f * R, HY + 0.49f * R);
        g.strokePath (nose, PathStrokeType (1.4f));
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (CX - 0.03f * R, HY + 0.36f * R, 0.035f * R, 0.035f * R);

        // ---- mouth
        const float my = HY + 0.7f * R;
        g.setColour (P.skinLine);
        if (mood == 0)
        {
            Path m;
            m.startNewSubPath (CX - 0.08f * R, my);
            m.quadraticTo (CX, my + 0.05f * R, CX + 0.08f * R, my);
            g.strokePath (m, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        else if (mood == 1)
        {
            Path m;   // soft ":3" smile
            m.startNewSubPath (CX - 0.12f * R, my - 0.01f * R);
            m.quadraticTo (CX - 0.06f * R, my + 0.06f * R, CX, my);
            m.quadraticTo (CX + 0.06f * R, my + 0.06f * R, CX + 0.12f * R, my - 0.01f * R);
            g.strokePath (m, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        else if (mood == 2)
        {
            Path m;   // pout
            m.startNewSubPath (CX - 0.09f * R, my + 0.02f * R);
            m.quadraticTo (CX, my - 0.03f * R, CX + 0.09f * R, my + 0.02f * R);
            g.strokePath (m, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
            // anime sweat drop
            Path dropP;
            const float dx = CX + 0.95f * R, dy = HY - 0.2f * R, ds = 0.1f * R;
            dropP.startNewSubPath (dx, dy - ds * 1.8f);
            dropP.cubicTo (dx + ds, dy - 0.4f * ds, dx + ds, dy + ds, dx, dy + ds);
            dropP.cubicTo (dx - ds, dy + ds, dx - ds, dy - 0.4f * ds, dx, dy - ds * 1.8f);
            g.setColour (Colour (0xffbfe6ff));
            g.fillPath (dropP);
            g.setColour (Colour (0xff4a7aa8));
            g.strokePath (dropP, PathStrokeType (1.3f));
        }
        else
        {
            const float w = (mood == 4 ? 0.5f : 0.34f) * R * (mood == 4 ? 1.0f + 0.25f * pulse : 1.0f);
            const float h = (mood == 4 ? 0.22f : 0.06f) * R * (mood == 4 ? 1.0f + 0.3f * pulse : 1.0f);
            Path m;
            m.startNewSubPath (CX - w * 0.5f, my - 0.05f * R);
            m.quadraticTo (CX, my + 0.04f * R, CX + w * 0.5f, my - 0.05f * R);
            m.quadraticTo (CX, my + h + 0.1f * R, CX - w * 0.5f, my - 0.05f * R);
            m.closeSubPath();
            g.setColour (Colour (0xff3a0010));
            g.fillPath (m);
            if (mood == 4)
            {
                Graphics::ScopedSaveState s (g);
                g.reduceClipRegion (m);
                g.setColour (juce::Colours::white);
                g.fillRect (CX - w * 0.5f, my - 0.06f * R, w, 0.07f * R);
                g.setColour (Colour (0xffe8708a));
                g.fillEllipse (CX - w * 0.22f, my + h * 0.5f, w * 0.44f, h * 0.6f);
                // fangs
                for (int side = -1; side <= 1; side += 2)
                {
                    Path fang;
                    const float fx = CX + (float) side * w * 0.3f;
                    fang.addTriangle (fx - 0.03f * R, my, fx + 0.03f * R, my, fx, my + 0.08f * R);
                    g.setColour (juce::Colours::white);
                    g.fillPath (fang);
                }
            }
            g.setColour (P.skinLine.darker (0.3f));
            g.strokePath (m, PathStrokeType (1.6f));
            if (mood == 4)
                drip (g, CX + w * 0.4f, my - 0.02f * R, 0.25f * R + 0.15f * R * (0.5f + 0.5f * std::sin (t * 2.0f)), 3.5f);
        }

        // blood on her face
        if (mood >= 3)
        {
            bloodSplat (g, CX + 0.52f * R, HY + 0.6f * R, 0.08f * R * (mood == 4 ? 1.6f : 1.0f), 77, 0.25f * R * rage);
            if (mood == 4)
            {
                bloodSplat (g, CX - 0.58f * R, HY + 0.66f * R, 0.1f * R, 78, 0.3f * R);
                bloodSplat (g, CX - 0.15f * R, HY - 0.5f * R, 0.07f * R, 79, 0.45f * R);
            }
        }
    }

    void drawFrontHair (Graphics& g, const Palette& P, float t, float rage)
    {
        // crown: same rounded top as the back hair so the silhouette stays smooth
        Path crown;
        crown.startNewSubPath (CX - 0.96f * R, HY - 0.05f * R);
        crown.lineTo (CX - 0.92f * R, HY - 0.6f * R);
        crown.cubicTo (CX - 1.08f * R, HY - 1.5f * R, CX + 1.08f * R, HY - 1.5f * R, CX + 0.92f * R, HY - 0.6f * R);
        crown.lineTo (CX + 0.96f * R, HY - 0.05f * R);
        crown.cubicTo (CX + 0.6f * R, HY - 0.75f * R, CX - 0.6f * R, HY - 0.75f * R, CX - 0.96f * R, HY - 0.05f * R);
        crown.closeSubPath();
        g.setGradientFill (juce::ColourGradient (P.hairBase, CX - 0.4f * R, HY - 1.3f * R, P.hairMid, CX + 0.6f * R, HY - 0.2f * R, false));
        g.fillPath (crown);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (crown);
            // strands sweeping from the crown toward the face
            for (int i = 0; i < 14; ++i)
            {
                const float f = (float) i / 13.0f * 2.0f - 1.0f;
                Path sw;
                sw.startNewSubPath (CX + f * 0.15f * R, HY - 1.3f * R);
                sw.quadraticTo (CX + f * 0.7f * R, HY - 1.1f * R, CX + f * 1.0f * R, HY - 0.2f * R);
                g.setColour (P.hairDark.withAlpha (i % 2 == 0 ? 0.45f : 0.25f));
                g.strokePath (sw, PathStrokeType (i % 2 == 0 ? 1.2f : 2.5f));
            }
            softBlob (g, CX + 0.6f * R, HY - 0.6f * R, 0.5f * R, 0.6f * R, P.hairDark.withAlpha (0.4f));
        }
        g.setColour (P.hairLine.withAlpha (0.85f));
        g.strokePath (crown, PathStrokeType (1.3f));

        // bang clumps: (tip x, tip y, width, bend)
        struct Bang { float tx, ty, w, bend; };
        const Bang bangs[] = {
            { -0.9f,  0.35f, 0.32f, -10.0f }, {  0.9f,  0.35f, 0.32f,  10.0f },
            { -0.72f, 0.1f,  0.34f,  -6.0f }, {  0.72f, 0.12f, 0.34f,   6.0f },
            { -0.5f, -0.02f, 0.36f,  -3.0f }, {  0.5f,  0.0f,  0.36f,   3.0f },
            { -0.28f, 0.06f, 0.34f,   2.0f }, {  0.3f,  0.04f, 0.34f,  -2.0f },
            { -0.08f, 0.0f,  0.3f,    4.0f }, {  0.12f, 0.32f, 0.16f,  -6.0f },   // long strand between the eyes
        };
        Path headClip;
        headClip.startNewSubPath (CX - 1.3f * R, HY + 0.6f * R);
        headClip.lineTo (CX - 1.3f * R, HY - 0.6f * R);
        headClip.lineTo (CX - 0.92f * R, HY - 0.6f * R);
        headClip.cubicTo (CX - 1.08f * R, HY - 1.5f * R, CX + 1.08f * R, HY - 1.5f * R, CX + 0.92f * R, HY - 0.6f * R);
        headClip.lineTo (CX + 1.3f * R, HY - 0.6f * R);
        headClip.lineTo (CX + 1.3f * R, HY + 0.6f * R);
        headClip.closeSubPath();
        {
        Graphics::ScopedSaveState bangClip (g);
        g.reduceClipRegion (headClip);

        int idx = 0;
        for (const auto& b : bangs)
        {
            const float wob = 1.5f * std::sin (t * 1.5f + (float) idx);
            const float x0 = CX + b.tx * 0.35f * R;
            const float y0 = HY - 1.45f * R;
            auto c = clump (x0, y0, CX + b.tx * R + wob, HY + b.ty * R, b.w * R, b.bend, 1.25f);
            paintClump (g, c, P, y0, HY + b.ty * R, true, b.tx >= 0.0f ? -1.0f : 1.0f);
            ++idx;
        }
        }

        // angel-ring highlight: jagged band across the crown
        Path ring;
        const int zig = 22;
        for (int i = 0; i <= zig; ++i)
        {
            const float a = -1.1f + 2.2f * (float) i / (float) zig;
            const float x = CX + std::sin (a) * 0.78f * R;
            const float y = HY - 0.82f * R - std::cos (a) * 0.18f * R + ((i % 2 == 0) ? -0.05f * R : 0.06f * R);
            if (i == 0) ring.startNewSubPath (x, y); else ring.lineTo (x, y);
        }
        for (int i = zig; i >= 0; --i)
        {
            const float a = -1.1f + 2.2f * (float) i / (float) zig;
            const float x = CX + std::sin (a) * 0.74f * R;
            const float y = HY - 0.74f * R - std::cos (a) * 0.15f * R + ((i % 2 == 0) ? 0.03f * R : -0.02f * R);
            ring.lineTo (x, y);
        }
        ring.closeSubPath();
        g.setColour (P.hairHi.withAlpha (0.75f - 0.35f * rage));
        g.fillPath (ring);
        g.setColour (P.hairTint.withAlpha (0.35f));
        g.strokePath (ring, PathStrokeType (1.5f), juce::AffineTransform::translation (0.0f, 3.0f));

        // flyaway strands
        g.setColour (P.hairMid.withAlpha (0.8f));
        for (int i = 0; i < 6; ++i)
        {
            const float f = (float) i / 5.0f * 2.0f - 1.0f;
            Path fly;
            const float x0 = CX + f * 0.8f * R, y0 = HY - 1.15f * R + std::abs (f) * 0.4f * R;
            fly.startNewSubPath (x0, y0);
            fly.quadraticTo (x0 + f * 0.3f * R, y0 - 0.25f * R + 3.0f * std::sin (t * 2.0f + (float) i),
                             x0 + f * 0.55f * R, y0 - 0.05f * R);
            g.strokePath (fly, PathStrokeType (1.0f));
        }

        // ahoge
        Path ahoge;
        ahoge.startNewSubPath (CX + 0.05f * R, HY - 1.15f * R);
        ahoge.cubicTo (CX + 0.05f * R, HY - 1.6f * R, CX + 0.5f * R + 3.0f * std::sin (t * 2.2f), HY - 1.7f * R, CX + 0.42f * R, HY - 1.35f * R);
        g.setColour (P.hairBase);
        g.strokePath (ahoge, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (P.hairLine);
        g.strokePath (ahoge, PathStrokeType (1.0f));

        // big ribbon bow on the right side
        {
            const float rx = CX + 0.78f * R, ry = HY - 0.85f * R;
            for (int side = -1; side <= 1; side += 2)
            {
                const float f = (float) side;
                Path loop;
                loop.startNewSubPath (rx, ry);
                loop.cubicTo (rx + f * 0.2f * R, ry - 0.4f * R, rx + f * 0.55f * R, ry - 0.35f * R, rx + f * 0.48f * R, ry - 0.02f * R);
                loop.cubicTo (rx + f * 0.42f * R, ry + 0.16f * R, rx + f * 0.18f * R, ry + 0.12f * R, rx, ry);
                loop.closeSubPath();
                g.setGradientFill (juce::ColourGradient (P.ribbon.brighter (0.25f), rx, ry - 0.3f * R, P.ribbonSh, rx + f * 0.4f * R, ry + 0.1f * R, false));
                g.fillPath (loop);
                g.setColour (P.ribbonSh.darker (0.4f));
                g.strokePath (loop, PathStrokeType (1.3f));
                g.setColour (juce::Colours::white.withAlpha (0.35f));
                g.drawLine (rx + f * 0.12f * R, ry - 0.18f * R, rx + f * 0.35f * R, ry - 0.24f * R, 2.0f);
            }
            g.setColour (P.ribbonSh);
            g.fillEllipse (rx - 0.07f * R, ry - 0.08f * R, 0.14f * R, 0.16f * R);
        }

        // crescent moon hairpin on the left
        {
            const float hx = CX - 0.62f * R, hy = HY - 0.62f * R, hr = 0.14f * R;
            g.setColour (P.gold);
            g.fillEllipse (hx - hr, hy - hr, hr * 2.0f, hr * 2.0f);
            g.setColour (P.hairBase);
            g.fillEllipse (hx - hr * 0.35f, hy - hr * 1.15f, hr * 1.7f, hr * 1.7f);
            g.setColour (P.gold.darker (0.5f));
            g.drawEllipse (hx - hr, hy - hr, hr * 2.0f, hr * 2.0f, 1.0f);
            Path star;
            star.addStar ({ hx + 0.2f * R, hy + 0.12f * R }, 4, 0.025f * R, 0.08f * R, 0.0f);
            g.setColour (P.gem);
            g.fillPath (star);
        }
    }
}

//==============================================================================
void HushGirl::draw (Graphics& g, float t, int mood, float rage, float pulse)
{
    const Palette P = makePalette (rage);

    drawBackHair (g, P, t);
    drawBody (g, P, mood, rage, t);
    drawSideLocks (g, P, t, mood, rage);

    // gentle head tilt; much more unhinged at full rage
    const float tilt = (mood == 4 ? 0.12f : 0.05f) * std::sin (t * (mood == 4 ? 2.4f : 0.7f)) + (mood >= 3 ? 0.08f : 0.0f);
    Graphics::ScopedSaveState s (g);
    g.addTransform (juce::AffineTransform::rotation (tilt, CX, HY + 0.9f * R));
    drawFace (g, P, mood, rage, pulse, t);
    drawFrontHair (g, P, t, rage);
}
