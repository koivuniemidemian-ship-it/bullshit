#include "PluginEditor.h"
#include <cmath>

namespace
{
    using juce::Colour;
    using juce::Path;

    constexpr float pi = juce::MathConstants<float>::pi;

    // Figure geometry (stage is the left 440 px)
    constexpr float CX = 220.0f, HY = 150.0f, R = 52.0f;

    const Colour ink     { 0xff2a1d3a };
    const Colour skin    { 0xffffe9dd };
    const Colour skinSh  { 0xfff5c9bb };
    const Colour blood   { 0xff9a0012 };
    const Colour bloodHi { 0xffe0182e };
    const Colour bloodDk { 0xff4a0008 };

    const char* taglines[] = {
        "she was so calm a second ago",
        "turn it down. or else.",
        "your noise floor is on thin ice",
        "she will remove you from the mix",
        "sleepy at 0%. dangerous at 100%.",
        "be quiet, okay? <3"
    };
    constexpr int numTaglines = (int) (sizeof (taglines) / sizeof (taglines[0]));

    juce::Font fontOf (float size, int style = juce::Font::bold)
    {
        return juce::Font (juce::FontOptions (size, style));
    }

    float smoothstep (float e0, float e1, float x)
    {
        const float v = juce::jlimit (0.0f, 1.0f, (x - e0) / (e1 - e0));
        return v * v * (3.0f - 2.0f * v);
    }

    Colour mix (Colour a, Colour b, float t) { return a.interpolatedWith (b, juce::jlimit (0.0f, 1.0f, t)); }

    Path heart (float cx, float cy, float s)
    {
        Path h;
        h.startNewSubPath (cx, cy + s * 0.9f);
        h.cubicTo (cx - s * 1.2f, cy + s * 0.1f, cx - s * 0.9f, cy - s * 0.9f, cx, cy - s * 0.35f);
        h.cubicTo (cx + s * 0.9f, cy - s * 0.9f, cx + s * 1.2f, cy + s * 0.1f, cx, cy + s * 0.9f);
        h.closeSubPath();
        return h;
    }

    // A cartoon blood splat: spiky blob + satellite droplets
    Path splat (float x, float y, float r, int seed)
    {
        juce::Random rr (seed);
        Path p;
        const int n = 16;
        for (int i = 0; i < n; ++i)
        {
            const float a = 2.0f * pi * (float) i / (float) n;
            float rad = r * (0.65f + 0.45f * rr.nextFloat());
            if (i % 3 == 0) rad *= 1.35f + 0.4f * rr.nextFloat();
            const float px = x + std::cos (a) * rad, py = y + std::sin (a) * rad;
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
        p.closeSubPath();
        p = p.createPathWithRoundedCorners (r * 0.35f);

        for (int j = 0; j < 6; ++j)
        {
            const float a = rr.nextFloat() * 2.0f * pi;
            const float d = r * (1.4f + 1.3f * rr.nextFloat());
            const float s = r * (0.07f + 0.16f * rr.nextFloat());
            p.addEllipse (x + std::cos (a) * d - s, y + std::sin (a) * d - s, s * 2.0f, s * 2.0f);
        }
        return p;
    }

    void paintSplat (juce::Graphics& g, float x, float y, float r, int seed, float alpha = 1.0f)
    {
        auto p = splat (x, y, r, seed);
        g.setColour (bloodDk.withAlpha (alpha));
        g.fillPath (p, juce::AffineTransform::translation (1.5f, 2.0f));
        g.setColour (blood.withAlpha (alpha));
        g.fillPath (p);
        g.setColour (bloodHi.withAlpha (alpha));
        g.fillPath (p, juce::AffineTransform::scale (0.6f, 0.6f, x, y));
        g.setColour (juce::Colours::white.withAlpha (0.55f * alpha));
        g.fillEllipse (x - r * 0.35f, y - r * 0.4f, r * 0.22f, r * 0.14f);
    }
}

//==============================================================================
KawaiiLookAndFeel::KawaiiLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, ink.withAlpha (0.45f));
    setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (0x66ff4fa3));
    setColour (juce::Label::textColourId, juce::Colours::white);
}

void KawaiiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);
    const float rage = smoothstep (0.3f, 0.9f, pos);

    const auto accentA = mix (Colour (0xff4fd8ff), bloodHi, rage);
    const auto accentB = mix (Colour (0xffff6fc8), bloodDk, rage);

    // glow
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (accentA.withAlpha (0.08f * (float) i));
        const float gr = r + (float) i * 5.0f;
        g.fillEllipse (c.x - gr, c.y - gr, gr * 2.0f, gr * 2.0f);
    }

    g.setGradientFill (juce::ColourGradient (mix (Colour (0xfff4f1ff), Colour (0xff3a0a10), rage), c.x, c.y - r,
                                             mix (Colour (0xffb9c6ff), Colour (0xff120004), rage), c.x, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (ink);
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 3.0f);

    for (int i = 0; i <= 12; ++i)
    {
        const float a = startA + (endA - startA) * (float) i / 12.0f;
        const float r1 = r * 0.76f, r2 = r * (i % 6 == 0 ? 0.6f : 0.68f);
        g.setColour (mix (ink, juce::Colours::white, rage).withAlpha (0.55f));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, 2.0f);
    }

    Path track, value;
    track.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.0f, startA, endA, true);
    value.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.0f, startA, angle, true);
    const juce::PathStrokeType stroke (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (ink.withAlpha (0.2f));
    g.strokePath (track, stroke);
    g.setGradientFill (juce::ColourGradient (accentA, c.x - r, c.y, accentB, c.x + r, c.y, false));
    g.strokePath (value, stroke);

    const float cr = r * 0.4f;
    g.setColour (mix (juce::Colours::white, Colour (0xff200005), rage));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.setColour (ink);
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 2.5f);
    g.setColour (accentB.brighter (0.3f));
    g.fillPath (heart (c.x, c.y, cr * 0.5f));

    const float pr = r * 0.9f;
    const juce::Point<float> tip (c.x + std::sin (angle) * pr, c.y - std::cos (angle) * pr);
    g.setColour (juce::Colours::white);
    g.fillEllipse (tip.x - 8.0f, tip.y - 8.0f, 16.0f, 16.0f);
    g.setColour (ink);
    g.drawEllipse (tip.x - 8.0f, tip.y - 8.0f, 16.0f, 16.0f, 2.5f);
}

//==============================================================================
ShizukaEditor::ShizukaEditor (ShizukaProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    addAndMakeVisible (knob);
    knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "hush", knob);

    knobLabel.setText ("ANGER", juce::dontSendNotification);
    knobLabel.setJustificationType (juce::Justification::centred);
    knobLabel.setFont (fontOf (20.0f));
    addAndMakeVisible (knobLabel);

    for (auto& b : bandsUI)
        b = -100.0f;

    setSize (700, 740);
    startTimerHz (30);
}

ShizukaEditor::~ShizukaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ShizukaEditor::resized()
{
    knobLabel.setBounds (450, 418, 240, 26);
    knob.setBounds (480, 446, 180, 200);
}

float ShizukaEditor::knobValue() const
{
    if (auto* param = proc.apvts.getParameter ("hush"))
        return param->getValue();
    return 0.0f;
}

int ShizukaEditor::moodFor (float k)
{
    if (k < 0.001f) return 0;   // serene
    if (k < 0.30f)  return 1;   // sweet
    if (k < 0.55f)  return 2;   // pouting
    if (k < 0.82f)  return 3;   // yandere
    return 4;                   // full bloodbath
}

void ShizukaEditor::timerCallback()
{
    t += 1.0f / 30.0f;
    for (size_t i = 0; i < bandsUI.size(); ++i)
    {
        const float target = proc.bands[i].load();
        bandsUI[i] += (target - bandsUI[i]) * (target > bandsUI[i] ? 0.6f : 0.15f);
    }
    gatedUI += (proc.gatedAmount.load() - gatedUI) * 0.2f;
    tagline = (int) (t / 4.0f) % numTaglines;
    repaint();
}

void ShizukaEditor::paint (juce::Graphics& g)
{
    const float k = knobValue();
    const int mood = moodFor (k);
    const float rage = smoothstep (0.3f, 0.9f, k);

    drawBackground (g, k, rage);

    {
        juce::Graphics::ScopedSaveState save (g);
        if (mood == 4)
            g.addTransform (juce::AffineTransform::translation ((rng.nextFloat() - 0.5f) * 7.0f, (rng.nextFloat() - 0.5f) * 7.0f));
        drawGirl (g, mood, rage);
    }

    drawPanel (g, k, mood, rage);
    drawBlood (g, k, mood);
}

//==============================================================================
void ShizukaEditor::drawBackground (juce::Graphics& g, float k, float rage)
{
    const float W = (float) getWidth(), H = (float) getHeight();

    g.setGradientFill (juce::ColourGradient (mix (Colour (0xff0d1b4d), Colour (0xff1a0004), rage), 0.0f, 0.0f,
                                             mix (Colour (0xff4f9dff), Colour (0xff8a0010), rage), 0.0f, H, false));
    g.fillAll();

    // colourful bokeh, turning red
    const Colour palette[] = { Colour (0xffff6fc8), Colour (0xff4fe3ff), Colour (0xffffe15a), Colour (0xffb18cff), Colour (0xff7dffb0) };
    juce::Random bk (7);
    for (int i = 0; i < 26; ++i)
    {
        const float x = bk.nextFloat() * W;
        const float y = std::fmod (bk.nextFloat() * H - t * (6.0f + 10.0f * bk.nextFloat()) + H * 4.0f, H + 60.0f) - 30.0f;
        const float s = 12.0f + bk.nextFloat() * 46.0f;
        const auto col = mix (palette[i % 5], bloodHi, rage);
        g.setColour (col.withAlpha (0.12f + 0.12f * bk.nextFloat()));
        g.fillEllipse (x - s * 0.5f, y - s * 0.5f, s, s);
    }

    // sparkles / sakura petals while she's calm
    const float calm = 1.0f - rage;
    juce::Random sp (21);
    for (int i = 0; i < 22; ++i)
    {
        const float x = std::fmod (sp.nextFloat() * W + t * 14.0f * (0.5f + sp.nextFloat()), W);
        const float y = std::fmod (sp.nextFloat() * H + t * 22.0f * (0.5f + sp.nextFloat()), H);
        if (i % 2 == 0)
        {
            Path star;
            star.addStar ({ x, y }, 4, 1.5f, 6.0f + 3.0f * std::sin (t * 3.0f + (float) i), t);
            g.setColour (juce::Colours::white.withAlpha (0.8f * calm));
            g.fillPath (star);
        }
        else
        {
            g.setColour (Colour (0xffffb7d9).withAlpha (0.8f * calm));
            g.fillEllipse (x, y, 9.0f, 5.0f);
        }
    }

    // floor shadow
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (CX - 75.0f, HY + 10.4f * R, 150.0f, 22.0f);

    // dark vignette with rage
    juce::ColourGradient vig (juce::Colours::transparentBlack, W * 0.33f, H * 0.5f,
                              juce::Colours::black.withAlpha (0.75f * smoothstep (0.5f, 1.0f, k)), 0.0f, 0.0f, true);
    g.setGradientFill (vig);
    g.fillAll();
}

//==============================================================================
void ShizukaEditor::drawGirl (juce::Graphics& g, int mood, float rage)
{
    const float sway = 4.0f * std::sin (t * 1.2f);
    const float breath = 1.5f * std::sin (t * 2.0f);

    const auto hairBase = mix (Colour (0xff7fb2ff), Colour (0xffd0102a), rage);
    const auto hairDark = mix (Colour (0xff3d58c9), Colour (0xff4a0008), rage);
    const auto hairHi   = mix (Colour (0xffd6ecff), Colour (0xffff8a8a), rage);
    const auto knit     = mix (Colour (0xfff7f4ff), Colour (0xfff0e4e4), rage);
    const auto knitSh   = mix (Colour (0xffc7c4f2), Colour (0xffc99a9a), rage);
    const auto tights   = mix (Colour (0xff2b2f6b), Colour (0xff1a0508), rage);

    // ---------------- back hair
    Path bh;
    bh.startNewSubPath (CX - 0.95f * R, HY - 0.2f * R);
    bh.cubicTo (CX - 1.15f * R, HY - 1.35f * R, CX + 1.15f * R, HY - 1.35f * R, CX + 0.95f * R, HY - 0.2f * R);
    bh.cubicTo (CX + 1.3f * R, HY + 1.5f * R, CX + 1.5f * R + sway, HY + 3.5f * R, CX + 1.35f * R + sway, HY + 5.6f * R);
    for (int i = 1; i <= 8; ++i)
    {
        const float x = CX + 1.35f * R - 2.7f * R * (float) i / 8.0f + sway;
        const float y = HY + 5.6f * R + ((i % 2 == 1) ? -0.4f * R : 0.05f * R) + (i == 4 ? 0.2f * R : 0.0f);
        bh.lineTo (x, y);
    }
    bh.cubicTo (CX - 1.5f * R + sway, HY + 3.5f * R, CX - 1.3f * R, HY + 1.5f * R, CX - 0.95f * R, HY - 0.2f * R);
    bh.closeSubPath();
    g.setGradientFill (juce::ColourGradient (hairBase, CX, HY - R, hairDark, CX, HY + 5.6f * R, false));
    g.fillPath (bh);
    g.setColour (ink);
    g.strokePath (bh, juce::PathStrokeType (2.5f));

    // ---------------- sleeves (hands clasped behind her back)
    for (int s = -1; s <= 1; s += 2)
    {
        const float f = (float) s;
        Path sl;
        sl.startNewSubPath (CX + f * 1.2f * R, HY + 1.75f * R);
        sl.cubicTo (CX + f * 1.62f * R, HY + 2.4f * R, CX + f * 1.55f * R, HY + 3.8f * R, CX + f * 1.25f * R, HY + 4.9f * R);
        sl.lineTo (CX + f * 0.85f * R, HY + 4.75f * R);
        sl.cubicTo (CX + f * 1.0f * R, HY + 3.8f * R, CX + f * 0.95f * R, HY + 2.8f * R, CX + f * 0.9f * R, HY + 2.3f * R);
        sl.closeSubPath();
        g.setColour (knitSh);
        g.fillPath (sl);
        g.setColour (ink);
        g.strokePath (sl, juce::PathStrokeType (2.0f));
    }

    // ---------------- legs: tights + rainbow leg warmers + shoes
    const Colour warmerCols[] = { Colour (0xffffa6d1), Colour (0xffa6f0ff), Colour (0xffd2b8ff), Colour (0xfffff1a6), Colour (0xffb8ffd2) };
    for (int s = -1; s <= 1; s += 2)
    {
        const float f = (float) s;
        auto X = [&] (float v) { return CX + f * v * R; };

        Path leg;
        leg.startNewSubPath (X (0.95f), HY + 5.8f * R);
        leg.cubicTo (X (1.0f),  HY + 7.2f * R, X (0.72f), HY + 8.0f * R, X (0.62f), HY + 8.4f * R);
        leg.cubicTo (X (0.75f), HY + 9.2f * R, X (0.55f), HY + 10.2f * R, X (0.5f), HY + 10.5f * R);
        leg.lineTo (X (0.22f), HY + 10.5f * R);
        leg.cubicTo (X (0.2f),  HY + 9.6f * R, X (0.12f), HY + 9.0f * R, X (0.18f), HY + 8.4f * R);
        leg.cubicTo (X (0.1f),  HY + 7.6f * R, X (0.05f), HY + 6.8f * R, X (0.04f), HY + 5.8f * R);
        leg.closeSubPath();
        g.setColour (tights);
        g.fillPath (leg);
        g.setColour (ink);
        g.strokePath (leg, juce::PathStrokeType (2.0f));
        g.setColour (juce::Colours::white.withAlpha (0.22f));   // sheen
        g.drawLine (X (0.62f), HY + 6.4f * R, X (0.52f), HY + 8.1f * R, 4.0f);

        // shoe
        const juce::Rectangle<float> shoe (juce::jmin (X (0.18f), X (0.6f)), HY + 10.45f * R, 0.42f * R, 0.3f * R);
        g.setColour (mix (Colour (0xffff8fc8), Colour (0xff120004), rage));
        g.fillRoundedRectangle (shoe, 0.12f * R);
        g.setColour (ink);
        g.drawRoundedRectangle (shoe, 0.12f * R, 2.0f);

        // chunky leg warmer, bunched in rings
        const float lx = X (0.38f);
        const int rings = 7;
        for (int i = 0; i < rings; ++i)
        {
            const float y0 = HY + (8.6f + 0.26f * (float) i) * R;
            const float hw = (0.27f + 0.04f * std::sin ((float) i * 1.7f)) * R;
            const juce::Rectangle<float> ring (lx - hw, y0, hw * 2.0f, 0.32f * R);
            g.setColour (mix (warmerCols[(i + (s > 0 ? 2 : 0)) % 5], Colour (0xff5a0010), rage));
            g.fillRoundedRectangle (ring, 0.14f * R);
            g.setColour (ink.withAlpha (0.8f));
            g.drawRoundedRectangle (ring, 0.14f * R, 1.5f);
            g.setColour (ink.withAlpha (0.18f));
            for (int j = -2; j <= 2; ++j)
                g.drawLine (lx + (float) j * hw * 0.35f, y0 + 4.0f, lx + (float) j * hw * 0.35f, y0 + 0.32f * R - 4.0f, 1.0f);
        }
    }

    // ---------------- sweater dress
    Path dress;
    dress.startNewSubPath (CX - 0.35f * R, HY + 1.35f * R);
    dress.cubicTo (CX - 0.9f * R, HY + 1.45f * R, CX - 1.25f * R, HY + 1.6f * R, CX - 1.3f * R, HY + 2.0f * R);
    dress.cubicTo (CX - 1.2f * R, HY + 3.0f * R, CX - 0.9f * R + breath * 0.3f, HY + 3.6f * R, CX - 0.92f * R, HY + 3.9f * R);
    dress.cubicTo (CX - 1.1f * R, HY + 4.8f * R, CX - 1.3f * R, HY + 5.6f * R, CX - 1.35f * R, HY + 6.3f * R);
    dress.quadraticTo (CX, HY + 6.5f * R, CX + 1.35f * R, HY + 6.3f * R);
    dress.cubicTo (CX + 1.3f * R, HY + 5.6f * R, CX + 1.1f * R, HY + 4.8f * R, CX + 0.92f * R, HY + 3.9f * R);
    dress.cubicTo (CX + 0.9f * R - breath * 0.3f, HY + 3.6f * R, CX + 1.2f * R, HY + 3.0f * R, CX + 1.3f * R, HY + 2.0f * R);
    dress.cubicTo (CX + 1.25f * R, HY + 1.6f * R, CX + 0.9f * R, HY + 1.45f * R, CX + 0.35f * R, HY + 1.35f * R);
    dress.closeSubPath();

    g.setColour (knit);
    g.fillPath (dress);
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (dress);

        // cel shading on her left side + under the collar
        g.setColour (knitSh.withAlpha (0.7f));
        g.fillRect (CX + 0.55f * R, HY + 1.3f * R, 1.0f * R, 5.3f * R);
        g.fillEllipse (CX - 0.8f * R, HY + 1.35f * R, 1.6f * R, 0.5f * R);

        // cable-knit braids
        g.setColour (knitSh);
        for (int c = -1; c <= 1; ++c)
        {
            const float bx = CX + (float) c * 0.5f * R;
            for (float y = HY + 2.1f * R; y < HY + 5.9f * R; y += 0.22f * R)
            {
                Path chev;
                chev.startNewSubPath (bx - 0.1f * R, y);
                chev.lineTo (bx, y + 0.12f * R);
                chev.lineTo (bx + 0.1f * R, y);
                g.strokePath (chev, juce::PathStrokeType (1.6f));
            }
            g.drawLine (bx - 0.16f * R, HY + 2.0f * R, bx - 0.16f * R, HY + 6.0f * R, 1.2f);
            g.drawLine (bx + 0.16f * R, HY + 2.0f * R, bx + 0.16f * R, HY + 6.0f * R, 1.2f);
        }

        // ribbed hem
        g.setColour (knitSh.darker (0.1f));
        g.fillRect (CX - 1.5f * R, HY + 5.95f * R, 3.0f * R, 0.6f * R);
        g.setColour (ink.withAlpha (0.25f));
        for (float x = CX - 1.4f * R; x < CX + 1.4f * R; x += 0.12f * R)
            g.drawLine (x, HY + 6.0f * R, x, HY + 6.5f * R, 1.0f);

        // blood soaking into the sweater
        const int stains = (int) (rage * 9.0f);
        juce::Random st (404);
        for (int i = 0; i < stains; ++i)
        {
            const float x = CX + (st.nextFloat() - 0.5f) * 2.2f * R;
            const float y = HY + (2.0f + st.nextFloat() * 4.0f) * R;
            paintSplat (g, x, y, (0.12f + 0.2f * st.nextFloat()) * R, 1000 + i);
        }
    }
    g.setColour (ink);
    g.strokePath (dress, juce::PathStrokeType (2.5f));

    // neck + rolled turtleneck
    g.setColour (skinSh);
    g.fillRect (CX - 0.2f * R, HY + 0.75f * R, 0.4f * R, 0.4f * R);
    const juce::Rectangle<float> collar (CX - 0.58f * R, HY + 0.95f * R, 1.16f * R, 0.58f * R);
    g.setColour (knit);
    g.fillRoundedRectangle (collar, 0.26f * R);
    g.setColour (ink);
    g.drawRoundedRectangle (collar, 0.26f * R, 2.5f);
    g.setColour (knitSh);
    for (int i = 1; i < 8; ++i)
    {
        const float x = collar.getX() + collar.getWidth() * (float) i / 8.0f;
        g.drawLine (x, collar.getY() + 5.0f, x, collar.getBottom() - 5.0f, 1.5f);
    }

    drawHead (g, mood, rage);

    // long front locks falling over the sweater
    for (int s = -1; s <= 1; s += 2)
    {
        const float f = (float) s;
        Path lock;
        const float x0 = CX + f * 0.86f * R;
        lock.startNewSubPath (x0, HY - 0.45f * R);
        lock.cubicTo (x0 + f * 0.15f * R, HY + 0.8f * R, x0 + f * 0.1f * R + sway * 0.5f, HY + 2.6f * R, x0 - f * 0.05f * R + sway * 0.6f, HY + 3.7f * R);
        lock.cubicTo (x0 - f * 0.2f * R, HY + 2.8f * R, x0 - f * 0.22f * R, HY + 1.0f * R, x0 - f * 0.28f * R, HY - 0.4f * R);
        lock.closeSubPath();
        g.setGradientFill (juce::ColourGradient (hairBase, CX, HY, hairDark, CX, HY + 3.7f * R, false));
        g.fillPath (lock);
        g.setColour (ink);
        g.strokePath (lock, juce::PathStrokeType (2.0f));
        g.setColour (hairHi.withAlpha (0.6f));
        g.drawLine (x0 - f * 0.08f * R, HY + 0.4f * R, x0 - f * 0.04f * R + sway * 0.4f, HY + 2.4f * R, 2.0f);
    }
}

void ShizukaEditor::drawHead (juce::Graphics& g, int mood, float rage)
{
    const auto hairBase = mix (Colour (0xff7fb2ff), Colour (0xffd0102a), rage);
    const auto hairDark = mix (Colour (0xff3d58c9), Colour (0xff4a0008), rage);
    const auto hairHi   = mix (Colour (0xffd6ecff), Colour (0xffff8a8a), rage);
    const float pulse = 1.0f + 0.3f * gatedUI;

    // ---------------- face
    Path face;
    face.startNewSubPath (CX - 0.8f * R, HY - 0.3f * R);
    face.cubicTo (CX - 0.8f * R, HY + 0.35f * R, CX - 0.35f * R, HY + 0.85f * R, CX, HY + 0.95f * R);
    face.cubicTo (CX + 0.35f * R, HY + 0.85f * R, CX + 0.8f * R, HY + 0.35f * R, CX + 0.8f * R, HY - 0.3f * R);
    face.cubicTo (CX + 0.8f * R, HY - 1.0f * R, CX - 0.8f * R, HY - 1.0f * R, CX - 0.8f * R, HY - 0.3f * R);
    face.closeSubPath();
    g.setColour (skin);
    g.fillPath (face);

    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (face);
        g.setColour (skinSh.withAlpha (0.6f));   // hair shadow on forehead
        g.fillRect (CX - R, HY - R, 2.0f * R, 0.75f * R);

        if (mood >= 3)   // the yandere shadow over the eyes
        {
            g.setGradientFill (juce::ColourGradient (Colour (0xff1a0010).withAlpha (mood == 4 ? 0.75f : 0.55f), CX, HY - 0.5f * R,
                                                     juce::Colours::transparentBlack, CX, HY + 0.45f * R, false));
            g.fillRect (CX - R, HY - R, 2.0f * R, 1.6f * R);
        }
    }
    g.setColour (ink);
    g.strokePath (face, juce::PathStrokeType (2.2f));

    // blush
    g.setColour (mix (Colour (0xffff7fb0), Colour (0xffff2040), rage).withAlpha (0.45f));
    g.fillEllipse (CX - 0.68f * R, HY + 0.38f * R, 0.38f * R, 0.16f * R);
    g.fillEllipse (CX + 0.30f * R, HY + 0.38f * R, 0.38f * R, 0.16f * R);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    for (int s = -1; s <= 1; s += 2)
        for (int i = 0; i < 3; ++i)
        {
            const float bx = CX + (float) s * 0.49f * R + ((float) i - 1.0f) * 0.07f * R;
            g.drawLine (bx + 0.02f * R, HY + 0.41f * R, bx - 0.02f * R, HY + 0.5f * R, 1.2f);
        }

    // ---------------- eyes
    const float eyeY = HY + 0.14f * R;
    const float browIn[] = { -0.02f, -0.03f, 0.07f, 0.0f, 0.1f };
    for (int s = -1; s <= 1; s += 2)
    {
        const float ex = CX + (float) s * 0.35f * R;
        const float ew = 0.36f * R, eh = 0.44f * R;
        const float inner = (float) -s;

        if (mood == 0)
        {
            Path closed;   // serene closed eyes, curved like smiling
            closed.addCentredArc (ex, eyeY + 0.05f * R, ew * 0.48f, eh * 0.25f, 0.0f, -pi * 0.5f + 0.25f, pi * 0.5f - 0.25f, true);
            g.setColour (ink);
            g.strokePath (closed, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawLine (ex - inner * ew * 0.5f, eyeY + 0.02f * R, ex - inner * ew * 0.68f, eyeY - 0.06f * R, 2.5f);
        }
        else
        {
            const float squint = (mood == 2) ? 0.3f : 0.0f;
            Path eye;
            eye.addEllipse (ex - ew * 0.5f, eyeY - eh * 0.5f, ew, eh);
            g.setColour (juce::Colours::white);
            g.fillPath (eye);

            {
                juce::Graphics::ScopedSaveState clip (g);
                g.reduceClipRegion (eye);

                const auto irisTop = mix (Colour (0xff2550e0), Colour (0xffb0001a), rage);
                const auto irisMid = mix (Colour (0xff5fa8ff), Colour (0xffff2a3a), rage);
                const auto irisBot = mix (Colour (0xff9ff3ff), Colour (0xffffb04a), rage);
                const float iw = ew * 0.72f, ih = eh * 0.9f;
                const float ix = ex + inner * 0.02f * R, iy = eyeY + eh * 0.06f;

                juce::ColourGradient ig (irisTop, ix, iy - ih * 0.5f, irisBot, ix, iy + ih * 0.5f, false);
                ig.addColour (0.5, irisMid);
                g.setGradientFill (ig);
                g.fillEllipse (ix - iw * 0.5f, iy - ih * 0.5f, iw, ih);
                g.setColour (irisTop.darker (0.4f));
                g.drawEllipse (ix - iw * 0.5f, iy - ih * 0.5f, iw, ih, 2.0f);

                if (mood >= 3)   // crazy concentric rings, pinprick pupil
                {
                    g.setColour (bloodDk.withAlpha (0.7f));
                    g.drawEllipse (ix - iw * 0.32f, iy - ih * 0.32f, iw * 0.64f, ih * 0.64f, 1.5f);
                    g.drawEllipse (ix - iw * 0.2f, iy - ih * 0.2f, iw * 0.4f, ih * 0.4f, 1.2f);
                    g.setColour (juce::Colours::black);
                    g.fillEllipse (ix - 2.0f, iy - 3.0f, 4.0f, 6.0f);
                }
                else
                {
                    g.setColour (ink);
                    g.fillEllipse (ix - iw * 0.22f, iy - ih * 0.25f, iw * 0.44f, ih * 0.5f);
                    // sparkly highlights
                    g.setColour (juce::Colours::white);
                    g.fillEllipse (ix - iw * 0.38f, iy - ih * 0.38f, iw * 0.34f, iw * 0.34f);
                    g.fillEllipse (ix + iw * 0.12f, iy + ih * 0.14f, iw * 0.14f, iw * 0.14f);
                    Path tw;
                    tw.addStar ({ ix + iw * 0.18f, iy - ih * 0.2f }, 4, 1.0f, 4.0f, 0.0f);
                    g.fillPath (tw);
                }

                if (squint > 0.0f)
                {
                    g.setColour (skin);
                    g.fillRect (ex - ew, eyeY - eh, ew * 2.0f, eh * (0.5f + squint));
                }
            }

            if (mood == 4)   // glowing eyes
            {
                for (int i = 3; i >= 1; --i)
                {
                    g.setColour (bloodHi.withAlpha (0.12f * (float) i));
                    const float gw = ew * (0.5f + 0.25f * (float) i), gh = eh * (0.5f + 0.2f * (float) i);
                    g.fillEllipse (ex - gw * 0.5f, eyeY - gh * 0.5f, gw, gh);
                }
            }

            // upper lash line with an outer flick
            const float lidY = eyeY - eh * 0.5f + eh * squint;
            Path lash;
            lash.startNewSubPath (ex + inner * ew * 0.55f, lidY + eh * 0.12f);
            lash.quadraticTo (ex, lidY - eh * 0.16f, ex - inner * ew * 0.62f, lidY + eh * 0.05f);
            lash.lineTo (ex - inner * ew * 0.78f, lidY - eh * 0.08f);
            g.setColour (ink);
            g.strokePath (lash, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawLine (ex - inner * ew * 0.3f, eyeY + eh * 0.5f, ex + inner * ew * 0.25f, eyeY + eh * 0.52f, 1.5f);
        }

        // brows
        const float by = HY - 0.26f * R;
        Path brow;
        brow.startNewSubPath (ex - inner * 0.16f * R, by);
        brow.quadraticTo (ex, by - 0.07f * R, ex + inner * 0.15f * R, by + browIn[mood] * R);
        g.setColour (hairDark.darker (0.3f));
        g.strokePath (brow, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // nose
    g.setColour (skinSh.darker (0.2f));
    g.drawLine (CX + 0.02f * R, HY + 0.38f * R, CX - 0.02f * R, HY + 0.42f * R, 2.0f);

    // ---------------- mouth
    const float my = HY + 0.6f * R;
    g.setColour (ink);
    if (mood <= 1)
    {
        Path m;
        m.startNewSubPath (CX - 0.1f * R, my);
        m.quadraticTo (CX, my + 0.07f * R, CX + 0.1f * R, my);
        g.strokePath (m, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else if (mood == 2)
    {
        Path m;   // wobbly pout
        m.startNewSubPath (CX - 0.1f * R, my + 0.02f * R);
        m.quadraticTo (CX - 0.05f * R, my - 0.03f * R, CX, my + 0.01f * R);
        m.quadraticTo (CX + 0.05f * R, my - 0.03f * R, CX + 0.1f * R, my + 0.02f * R);
        g.strokePath (m, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        // unsettling wide smile -> huge crazy grin
        const float w = (mood == 4 ? 0.62f : 0.42f) * R * (mood == 4 ? pulse : 1.0f);
        const float h = (mood == 4 ? 0.26f : 0.08f) * R * (mood == 4 ? pulse : 1.0f);
        Path m;
        m.startNewSubPath (CX - w * 0.5f, my - 0.04f * R);
        m.quadraticTo (CX, my + 0.06f * R, CX + w * 0.5f, my - 0.04f * R);
        m.quadraticTo (CX, my + h + 0.12f * R, CX - w * 0.5f, my - 0.04f * R);
        m.closeSubPath();
        g.setColour (Colour (0xff3a0010));
        g.fillPath (m);
        if (mood == 4)
        {
            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (m);
            g.setColour (juce::Colours::white);
            g.fillRect (CX - w * 0.5f, my - 0.06f * R, w, 0.08f * R);   // teeth
            g.setColour (ink.withAlpha (0.4f));
            for (int i = 1; i < 6; ++i)
                g.drawLine (CX - w * 0.5f + w * (float) i / 6.0f, my - 0.06f * R, CX - w * 0.5f + w * (float) i / 6.0f, my + 0.02f * R, 1.0f);
        }
        g.setColour (ink);
        g.strokePath (m, juce::PathStrokeType (2.0f));
    }

    // blood on her cheek
    if (mood >= 3)
    {
        paintSplat (g, CX + 0.5f * R, HY + 0.55f * R, 0.1f * R * (mood == 4 ? 1.6f : 1.0f), 77);
        g.setColour (blood);
        const float drip = 0.15f * R + (mood == 4 ? 0.25f * R * (0.5f + 0.5f * std::sin (t)) : 0.0f);
        g.fillRoundedRectangle (CX + 0.47f * R, HY + 0.55f * R, 3.0f, drip, 1.5f);
    }

    // ---------------- bangs
    Path bangs;
    bangs.startNewSubPath (CX - 0.93f * R, HY + 0.05f * R);
    bangs.cubicTo (CX - 1.02f * R, HY - 1.2f * R, CX + 1.02f * R, HY - 1.2f * R, CX + 0.93f * R, HY + 0.05f * R);
    const float tipY[] = { 0.0f, -0.42f, -0.12f, -0.48f, -0.05f, -0.45f, 0.08f, -0.44f, -0.1f, -0.46f, 0.02f };
    const int nTips = (int) (sizeof (tipY) / sizeof (tipY[0]));
    for (int i = 0; i < nTips; ++i)
    {
        const float x = CX + 0.86f * R - 1.72f * R * (float) i / (float) (nTips - 1);
        bangs.lineTo (x, HY + (tipY[i] - 0.05f) * R);
    }
    bangs.closeSubPath();
    g.setGradientFill (juce::ColourGradient (hairBase.brighter (0.1f), CX, HY - R, hairDark, CX, HY, false));
    g.fillPath (bangs);
    g.setColour (ink);
    g.strokePath (bangs, juce::PathStrokeType (2.2f));

    // glossy hair ring + sparkles
    Path ring;
    ring.addCentredArc (CX, HY - 0.55f * R, 0.7f * R, 0.28f * R, 0.0f, -1.25f, 1.25f, true);
    g.setColour (hairHi.withAlpha (0.85f));
    g.strokePath (ring, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    for (int i = 0; i < 3; ++i)
    {
        Path sp;
        sp.addStar ({ CX - 0.4f * R + (float) i * 0.4f * R, HY - 0.6f * R - (float) (i % 2) * 0.08f * R }, 4, 1.0f, 4.0f + 1.5f * std::sin (t * 4.0f + (float) i), 0.0f);
        g.setColour (juce::Colours::white.withAlpha (1.0f - rage));
        g.fillPath (sp);
    }

    // hair clips: a star and a heart
    Path starClip;
    starClip.addStar ({ CX - 0.62f * R, HY - 0.5f * R }, 5, 0.08f * R, 0.17f * R, 0.0f);
    g.setColour (mix (Colour (0xffffe15a), Colour (0xff2a0004), rage));
    g.fillPath (starClip);
    g.setColour (ink);
    g.strokePath (starClip, juce::PathStrokeType (1.5f));
    auto hc = heart (CX + 0.66f * R, HY - 0.42f * R, 0.13f * R);
    g.setColour (mix (Colour (0xffff5fae), bloodHi, rage));
    g.fillPath (hc);
    g.setColour (ink);
    g.strokePath (hc, juce::PathStrokeType (1.5f));
}

//==============================================================================
void ShizukaEditor::drawPanel (juce::Graphics& g, float k, int mood, float rage)
{
    const juce::Rectangle<float> panel (448.0f, 16.0f, 236.0f, 708.0f);
    g.setColour (mix (Colour (0xff0d1b4d), juce::Colours::black, rage).withAlpha (0.45f));
    g.fillRoundedRectangle (panel, 22.0f);
    g.setColour (mix (Colour (0xff9fe8ff), bloodHi, rage).withAlpha (0.6f));
    g.drawRoundedRectangle (panel, 22.0f, 2.0f);

    // title with gradient
    const juce::Rectangle<float> title (panel.getX(), 26.0f, panel.getWidth(), 50.0f);
    g.setGradientFill (juce::ColourGradient (mix (Colour (0xffff8fd0), bloodHi, rage), title.getX(), 0.0f,
                                             mix (Colour (0xff7fe8ff), Colour (0xffff7a3a), rage), title.getRight(), 0.0f, false));
    g.setFont (fontOf (40.0f));
    g.drawText ("SHIZUKA", title, juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (fontOf (13.0f, juce::Font::italic));
    g.drawText ("spectral gate", title.translated (0.0f, 36.0f).withHeight (18.0f), juce::Justification::centred);

    // mini spectrum with her quiet line
    const juce::Rectangle<float> spec (462.0f, 112.0f, 208.0f, 140.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (spec, 10.0f);
    const auto area = spec.reduced (8.0f);
    const float bw = area.getWidth() / (float) ShizukaProcessor::numBands;
    const float thresh = ShizukaProcessor::thresholdDbFor (k);
    auto yFor = [&] (float db) { return area.getBottom() - area.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 100.0f) / 100.0f); };
    for (int i = 0; i < ShizukaProcessor::numBands; ++i)
    {
        const float db = bandsUI[(size_t) i];
        const float y = yFor (db);
        const bool cut = k > 0.001f && db < thresh;
        const auto col = cut ? juce::Colours::white.withAlpha (0.12f)
                             : mix (Colour::fromHSV ((float) i / 48.0f * 0.8f, 0.6f, 1.0f, 1.0f), bloodHi, rage).withAlpha (0.85f);
        g.setColour (col);
        g.fillRect (area.getX() + (float) i * bw + 0.5f, y, bw - 1.0f, area.getBottom() - y);
    }
    if (k > 0.001f)
    {
        const float ty = yFor (thresh);
        const float dashes[] = { 6.0f, 4.0f };
        g.setColour (mix (Colour (0xffff8fd0), bloodHi, rage));
        g.drawDashedLine ({ area.getX(), ty, area.getRight(), ty }, dashes, 2, 2.5f);
    }

    // speech bubble
    const char* lines[] = { "~ so peaceful ~", "hehe, shh~", "...hey. too loud.", "why won't you\nbe QUIET?", "SILENCE." };
    const juce::Rectangle<float> bub (462.0f, 272.0f, 208.0f, 110.0f);
    Path bp;
    if (mood == 4)
    {
        const auto c = bub.getCentre();
        for (int i = 0; i <= 40; ++i)
        {
            const float a = pi * (float) i / 20.0f;
            const float rr = (i % 2 == 0) ? 1.0f : 0.8f;
            const float px = c.x + std::cos (a) * bub.getWidth() * 0.52f * rr, py = c.y + std::sin (a) * bub.getHeight() * 0.55f * rr;
            if (i == 0) bp.startNewSubPath (px, py); else bp.lineTo (px, py);
        }
        bp.closeSubPath();
    }
    else
    {
        bp.addRoundedRectangle (bub, 24.0f);
    }
    g.setColour (mood == 4 ? blood : juce::Colours::white);
    g.fillPath (bp);
    g.setColour (ink);
    g.strokePath (bp, juce::PathStrokeType (3.0f));
    g.setColour (mood == 4 ? juce::Colours::white : ink);
    g.setFont (fontOf (mood == 4 ? 34.0f : 20.0f));
    g.drawFittedText (lines[mood], bub.reduced (12.0f).toNearestInt(), juce::Justification::centred, 3);

    // tagline
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (fontOf (14.0f, juce::Font::italic));
    g.drawFittedText (taglines[tagline], juce::Rectangle<int> (456, 662, 220, 44), juce::Justification::centred, 2);
}

//==============================================================================
void ShizukaEditor::drawBlood (juce::Graphics& g, float k, int mood)
{
    const float W = (float) getWidth(), H = (float) getHeight();
    const float amount = smoothstep (0.4f, 1.0f, k);
    if (amount <= 0.0f)
        return;

    // drips running down from the top edge
    juce::Random dr (5);
    const int drips = 22;
    for (int i = 0; i < drips; ++i)
    {
        const float x = W * ((float) i + 0.5f) / (float) drips + (dr.nextFloat() - 0.5f) * 20.0f;
        const float wv = 5.0f + dr.nextFloat() * 10.0f;
        const float len = H * 0.55f * amount * (0.2f + 0.8f * dr.nextFloat()) * (0.92f + 0.08f * std::sin (t * 1.5f + (float) i));
        g.setColour (blood);
        g.fillRoundedRectangle (x - wv * 0.5f, -10.0f, wv, len + 10.0f, wv * 0.5f);
        g.fillEllipse (x - wv * 0.8f, len - wv * 0.8f, wv * 1.6f, wv * 1.8f);
        g.setColour (bloodHi.withAlpha (0.7f));
        g.fillRoundedRectangle (x - wv * 0.25f, -10.0f, wv * 0.2f, len, 1.0f);
    }
    // pooled band along the top
    g.setColour (blood);
    g.fillRect (0.0f, 0.0f, W, 6.0f + 14.0f * amount);

    // splatters all over the screen
    const int splats = (int) (amount * 30.0f);
    juce::Random sr (1234);
    for (int i = 0; i < splats; ++i)
    {
        const float x = sr.nextFloat() * W, y = sr.nextFloat() * H;
        const float r = 8.0f + sr.nextFloat() * 34.0f;
        paintSplat (g, x, y, r, 500 + i, 0.92f);
    }

    // blood rain at full rage
    if (mood == 4)
    {
        juce::Random rr (99);
        for (int i = 0; i < 50; ++i)
        {
            const float x = rr.nextFloat() * W;
            const float speed = 260.0f + rr.nextFloat() * 380.0f;
            const float y = std::fmod (t * speed + rr.nextFloat() * H, H + 60.0f) - 30.0f;
            const float s = 2.0f + rr.nextFloat() * 3.0f;
            g.setColour (bloodHi.withAlpha (0.85f));
            g.fillRoundedRectangle (x - s * 0.5f, y - s * 4.0f, s, s * 5.0f, s * 0.5f);
        }

        // red flash pulse
        g.setColour (Colour (0xffff0020).withAlpha (0.08f + 0.08f * std::sin (t * 10.0f)));
        g.fillAll();
    }
}
