#include "PluginEditor.h"
#include <cmath>

#if defined (HUSH_HAS_ART)
 #include "BinaryData.h"
#endif

namespace
{
    using juce::Colour;
    using juce::Path;

    constexpr float pi = juce::MathConstants<float>::pi;

    // Character geometry inside the left 440 px "stage"
    constexpr float CX = 220.0f, HY = 150.0f, R = 52.0f;
    constexpr float W  = 700.0f, H = 740.0f;

    const Colour ink     { 0xff2a1d3a };
    const Colour skin    { 0xfffff0e8 };
    const Colour skinSh  { 0xfff6cdc0 };
    const Colour blood   { 0xff9a0012 };
    const Colour bloodHi { 0xffe0182e };
    const Colour bloodDk { 0xff4a0008 };

    const char* taglines[] = {
        "she was so calm a second ago",
        "turn it down. or else.",
        "the moon is watching your mix",
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

    // Tapered hair strand / ribbon shape
    Path strand (float x0, float y0, float x1, float y1, float w, float bend)
    {
        Path p;
        const float dy = y1 - y0;
        p.startNewSubPath (x0 - w * 0.5f, y0);
        p.cubicTo (x0 - w * 0.5f + bend * 0.3f, y0 + dy * 0.35f, x1 - w * 0.3f + bend, y0 + dy * 0.72f, x1, y1);
        p.cubicTo (x1 + w * 0.3f + bend, y0 + dy * 0.72f, x0 + w * 0.5f + bend * 0.3f, y0 + dy * 0.35f, x0 + w * 0.5f, y0);
        p.closeSubPath();
        return p;
    }

    Path splat (float x, float y, float r, int seed)
    {
        juce::Random rr (seed);
        Path p;
        const int n = 18;
        for (int i = 0; i < n; ++i)
        {
            const float a = 2.0f * pi * (float) i / (float) n;
            float rad = r * (0.65f + 0.45f * rr.nextFloat());
            if (i % 3 == 0) rad *= 1.4f + 0.5f * rr.nextFloat();
            const float px = x + std::cos (a) * rad, py = y + std::sin (a) * rad;
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
        p.closeSubPath();
        p = p.createPathWithRoundedCorners (r * 0.35f);

        for (int j = 0; j < 8; ++j)
        {
            const float a = rr.nextFloat() * 2.0f * pi;
            const float d = r * (1.4f + 1.5f * rr.nextFloat());
            const float s = r * (0.06f + 0.16f * rr.nextFloat());
            p.addEllipse (x + std::cos (a) * d - s, y + std::sin (a) * d - s, s * 2.0f, s * 2.0f);
        }
        return p;
    }

    void paintSplat (juce::Graphics& g, float x, float y, float r, int seed, float alpha = 1.0f, float dripLen = 0.0f)
    {
        auto p = splat (x, y, r, seed);
        if (dripLen > 0.0f)
        {
            const float dw = juce::jmax (2.5f, r * 0.28f);
            p.addRoundedRectangle (x - dw * 0.5f, y, dw, r + dripLen, dw * 0.5f);
            p.addEllipse (x - dw * 0.8f, y + r + dripLen - dw * 0.6f, dw * 1.6f, dw * 1.8f);
        }
        g.setColour (bloodDk.withAlpha (alpha));
        g.fillPath (p, juce::AffineTransform::translation (1.5f, 2.0f));
        g.setColour (blood.withAlpha (alpha));
        g.fillPath (p);
        g.setColour (bloodHi.withAlpha (alpha));
        g.fillPath (splat (x, y, r * 0.55f, seed + 7));
        g.setColour (juce::Colours::white.withAlpha (0.55f * alpha));
        g.fillEllipse (x - r * 0.35f, y - r * 0.4f, r * 0.24f, r * 0.15f);
    }

    void drawCloud (juce::Graphics& g, float x, float y, float s, Colour top, Colour bot, Colour rim, float hi)
    {
        static const float offs[][3] = { { -1.25f, 0.25f, 0.55f }, { -0.7f, -0.15f, 0.75f }, { -0.05f, -0.45f, 0.95f },
                                         {  0.65f, -0.2f, 0.8f  }, {  1.25f,  0.2f, 0.6f  }, { -0.35f,  0.25f, 0.7f  },
                                         {  0.4f,  0.28f, 0.72f }, {  1.7f,  0.35f, 0.4f  }, { -1.75f,  0.38f, 0.38f } };
        Path p;
        for (auto& o : offs)
            p.addEllipse (x + o[0] * s - o[2] * s, y + o[1] * s - o[2] * s, 2.0f * o[2] * s, 2.0f * o[2] * s);

        g.setColour (rim);
        g.fillPath (p, juce::AffineTransform::translation (0.0f, -3.0f));
        g.setGradientFill (juce::ColourGradient (top, x, y - s, bot, x, y + 0.9f * s, false));
        g.fillPath (p);

        if (hi > 0.0f)
            for (int i = 0; i < 5; ++i)
            {
                const auto& o = offs[i];
                const float rr = o[2] * s * 0.55f;
                g.setColour (juce::Colours::white.withAlpha (hi));
                g.fillEllipse (x + o[0] * s - rr - o[2] * s * 0.25f, y + o[1] * s - rr - o[2] * s * 0.35f, rr * 2.0f, rr * 1.6f);
            }
    }

    float ridge (float x, float base, float amp, float f1, float p1, float f2, float p2)
    {
        const float a = std::pow (0.5f + 0.5f * std::sin (x * f1 + p1), 2.0f);
        const float b = std::pow (0.5f + 0.5f * std::sin (x * f2 + p2), 3.0f);
        const float jag = 0.05f * std::sin (x * 0.31f + p1 * 3.0f) + 0.035f * std::sin (x * 0.173f + p2) + 0.02f * std::sin (x * 0.71f);
        return base - amp * (0.62f * a + 0.45f * b + jag);
    }
}

//==============================================================================
//  The knob: machined metal, knurled edge, LED ring, glossy moon cap
//==============================================================================
KawaiiLookAndFeel::KawaiiLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x88100818));
    setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (0x66ff4fa3));
    setColour (juce::Label::textColourId, juce::Colours::white);
}

void KawaiiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 20.0f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);
    const float rage = smoothstep (0.3f, 0.9f, pos);

    const float ledR = r * 0.9f, bodyR = r * 0.72f, capR = r * 0.4f;
    auto polar = [&] (float a, float rad) { return juce::Point<float> (c.x + std::sin (a) * rad, c.y - std::cos (a) * rad); };

    // scale numbers
    g.setFont (fontOf (11.0f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startA + (endA - startA) * (float) i / 10.0f;
        const auto p = polar (a, r + 13.0f);
        g.setColour (juce::Colours::white.withAlpha (i <= (int) std::round (pos * 10.0f) ? 0.95f : 0.4f));
        g.drawText (juce::String (i), juce::Rectangle<float> (p.x - 10.0f, p.y - 7.0f, 20.0f, 14.0f), juce::Justification::centred);
    }

    // drop shadow
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.08f * (float) i));
        const float sr = r + (float) i * 2.5f;
        g.fillEllipse (c.x - sr, c.y - sr + 6.0f, sr * 2.0f, sr * 2.0f);
    }

    // bezel
    g.setGradientFill (juce::ColourGradient (Colour (0xff3a3550), c.x, c.y - r, Colour (0xff0b0912), c.x, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    Path rimHi;
    rimHi.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0.0f, -1.2f, 1.2f, true);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.strokePath (rimHi, juce::PathStrokeType (2.0f));
    g.setColour (juce::Colours::black);
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.0f);

    // LED ring
    const int leds = 31;
    for (int i = 0; i < leds; ++i)
    {
        const float f = (float) i / (float) (leds - 1);
        const float a = startA + (endA - startA) * f;
        const auto p = polar (a, ledR);
        const bool lit = f <= pos + 0.001f && pos > 0.001f;
        const Colour col = f < 0.5f ? mix (Colour (0xff4fe3ff), Colour (0xffff6fd0), f * 2.0f)
                                    : mix (Colour (0xffff6fd0), Colour (0xffff1030), (f - 0.5f) * 2.0f);
        if (lit)
        {
            g.setColour (col.withAlpha (0.25f));
            g.fillEllipse (p.x - 7.0f, p.y - 7.0f, 14.0f, 14.0f);
            g.setColour (col);
            g.fillEllipse (p.x - 3.5f, p.y - 3.5f, 7.0f, 7.0f);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillEllipse (p.x - 1.5f, p.y - 2.0f, 2.5f, 2.5f);
        }
        else
        {
            g.setColour (Colour (0xff22202e));
            g.fillEllipse (p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f);
        }
    }

    // knob body: brushed chrome turning blood-tinted
    const auto chromeHi = mix (Colour (0xfff4f6ff), Colour (0xffffc0c0), rage);
    const auto chromeLo = mix (Colour (0xff7d8296), Colour (0xff5a1018), rage);
    g.setGradientFill (juce::ColourGradient (chromeHi, c.x - bodyR * 0.35f, c.y - bodyR * 0.45f, chromeLo, c.x + bodyR, c.y + bodyR, true));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // knurled grip, rotates with the knob
    for (int i = 0; i < 64; ++i)
    {
        const float a = angle + 2.0f * pi * (float) i / 64.0f;
        const auto p1 = polar (a, bodyR * 0.86f), p2 = polar (a, bodyR * 0.99f);
        g.setColour ((i % 2 == 0) ? juce::Colours::black.withAlpha (0.35f) : juce::Colours::white.withAlpha (0.35f));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.6f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.5f);

    // engraved pointer
    const auto pa = polar (angle, capR * 1.1f), pb = polar (angle, bodyR * 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawLine (pa.x, pa.y, pb.x, pb.y, 4.5f);
    g.setColour (mix (Colour (0xff4fe3ff), bloodHi, rage));
    g.drawLine (pa.x + 0.5f, pa.y - 0.5f, pb.x + 0.5f, pb.y - 0.5f, 2.0f);

    // glossy cap with a crescent moon that turns to blood
    g.setGradientFill (juce::ColourGradient (mix (Colour (0xff3b4a9e), Colour (0xff4a0008), rage), c.x, c.y - capR,
                                             mix (Colour (0xff10163a), Colour (0xff120004), rage), c.x, c.y + capR, false));
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
    const float mr = capR * 0.55f;
    g.setColour (mix (Colour (0xfffff3b0), bloodHi, rage));
    g.fillEllipse (c.x - mr, c.y - mr, mr * 2.0f, mr * 2.0f);
    g.setColour (mix (Colour (0xff2c3a86), Colour (0xff3a0008), rage));
    g.fillEllipse (c.x - mr * 0.55f, c.y - mr * 1.1f, mr * 1.8f, mr * 1.8f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.fillEllipse (c.x - capR * 0.6f, c.y - capR * 0.85f, capR * 1.0f, capR * 0.5f);
    g.setColour (juce::Colours::black);
    g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.5f);

    // the knob bleeds too
    if (pos > 0.55f)
    {
        const float a = (pos - 0.55f) / 0.45f;
        paintSplat (g, c.x + bodyR * 0.45f, c.y - bodyR * 0.5f, 6.0f + 8.0f * a, 31, 0.95f);
        juce::Random dr (17);
        for (int i = 0; i < 4; ++i)
        {
            const float ang = pi * (0.65f + 0.7f * dr.nextFloat());
            const juce::Point<float> p (c.x + std::sin (ang) * r * 0.97f, c.y - std::cos (ang) * r * 0.97f);
            const float len = a * (8.0f + 22.0f * dr.nextFloat());
            g.setColour (blood);
            g.fillRoundedRectangle (p.x - 2.5f, p.y - 2.0f, 5.0f, len + 2.0f, 2.5f);
            g.fillEllipse (p.x - 4.0f, p.y + len - 3.0f, 8.0f, 9.0f);
        }
    }
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

#if defined (HUSH_HAS_ART)
    int size = 0;
    if (auto* d = BinaryData::getNamedResource ("girl_calm_png", size))
        artCalm = juce::ImageCache::getFromMemory (d, size);
    if (auto* d = BinaryData::getNamedResource ("girl_rage_png", size))
        artRage = juce::ImageCache::getFromMemory (d, size);
#endif

    for (auto& b : bandsUI)
        b = -100.0f;

    setSize ((int) W, (int) H);
    startTimerHz (30);
}

ShizukaEditor::~ShizukaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ShizukaEditor::resized()
{
    knobLabel.setBounds (450, 408, 240, 26);
    knob.setBounds (464, 432, 212, 226);
}

float ShizukaEditor::knobValue() const
{
    if (auto* param = proc.apvts.getParameter ("hush"))
        return param->getValue();
    return 0.0f;
}

int ShizukaEditor::moodFor (float k)
{
    if (k < 0.001f) return 0;
    if (k < 0.30f)  return 1;
    if (k < 0.55f)  return 2;
    if (k < 0.82f)  return 3;
    return 4;
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

    drawSky (g, k, rage);
    drawMoon (g, k);
    drawClouds (g, 0, rage);
    drawMountains (g, rage);
    drawClouds (g, 1, rage);
    drawCritters (g, rage);
    drawBloodPool (g, k);

    {
        juce::Graphics::ScopedSaveState save (g);
        if (mood == 4)
            g.addTransform (juce::AffineTransform::translation ((rng.nextFloat() - 0.5f) * 7.0f, (rng.nextFloat() - 0.5f) * 7.0f));

        if (artCalm.isValid())
            drawArt (g, rage);
        else
            drawGirl (g, mood, rage);
    }

    drawPanel (g, k, mood, rage);
    drawBlood (g, k, mood);
}

//==============================================================================
//  Background: sky -> blood moon, clouds, mountains
//==============================================================================
void ShizukaEditor::drawSky (juce::Graphics& g, float k, float rage)
{
    juce::ColourGradient sky (mix (Colour (0xff2a63d0), Colour (0xff060002), rage), 0.0f, 0.0f,
                              mix (Colour (0xffffe6d6), Colour (0xffd0201a), rage), 0.0f, 480.0f, false);
    sky.addColour (0.55, mix (Colour (0xff86bdf5), Colour (0xff4a0010), rage));
    g.setGradientFill (sky);
    g.fillAll();

    const float starA = smoothstep (0.35f, 0.8f, k);
    if (starA > 0.0f)
    {
        juce::Random sr (11);
        for (int i = 0; i < 90; ++i)
        {
            const float x = sr.nextFloat() * W, y = sr.nextFloat() * 330.0f;
            const float tw = 0.5f + 0.5f * std::sin (t * (1.0f + sr.nextFloat() * 3.0f) + (float) i);
            const float s = 1.0f + sr.nextFloat() * 1.8f;
            g.setColour ((i % 4 == 0 ? Colour (0xffff8080) : juce::Colours::white).withAlpha (starA * tw));
            g.fillEllipse (x, y, s, s);
        }
    }

    const float calm = 1.0f - rage;
    if (calm > 0.0f)
    {
        g.setGradientFill (juce::ColourGradient (Colour (0xfffff6d8).withAlpha (0.8f * calm), 560.0f, 90.0f,
                                                 Colour (0x00fff6d8), 780.0f, 90.0f, true));
        g.fillEllipse (340.0f, -130.0f, 440.0f, 440.0f);
    }
}

void ShizukaEditor::drawMoon (juce::Graphics& g, float k)
{
    const float rise = smoothstep (0.25f, 0.85f, k);
    if (rise <= 0.001f)
        return;

    const float mr = 112.0f;
    const float mx = CX, my = 620.0f - 470.0f * rise;

    for (int i = 6; i >= 1; --i)
    {
        const float gr = mr * (1.0f + 0.32f * (float) i);
        g.setColour (Colour (0xffff2a1a).withAlpha (0.055f * rise));
        g.fillEllipse (mx - gr, my - gr, gr * 2.0f, gr * 2.0f);
    }

    juce::ColourGradient disc (Colour (0xffff7a4a), mx - mr * 0.25f, my - mr * 0.3f, Colour (0xff6a0008), mx + mr, my + mr, true);
    disc.addColour (0.55, Colour (0xffd0141e));
    g.setGradientFill (disc);
    g.fillEllipse (mx - mr, my - mr, mr * 2.0f, mr * 2.0f);

    juce::Random cr (3);
    for (int i = 0; i < 14; ++i)
    {
        const float a = cr.nextFloat() * 2.0f * pi, d = cr.nextFloat() * mr * 0.8f;
        const float s = mr * (0.06f + 0.18f * cr.nextFloat());
        g.setColour (Colour (0xff4a0008).withAlpha (0.28f));
        g.fillEllipse (mx + std::cos (a) * d - s, my + std::sin (a) * d - s * 0.8f, s * 2.0f, s * 1.6f);
        g.setColour (Colour (0xffff9a7a).withAlpha (0.18f));
        g.drawEllipse (mx + std::cos (a) * d - s, my + std::sin (a) * d - s * 0.8f, s * 2.0f, s * 1.6f, 1.0f);
    }
    g.setColour (Colour (0xff3a0004).withAlpha (0.8f));
    g.drawEllipse (mx - mr, my - mr, mr * 2.0f, mr * 2.0f, 3.0f);

    const float bleed = smoothstep (0.6f, 1.0f, k);
    if (bleed > 0.0f)
    {
        juce::Random dr (8);
        for (int i = 0; i < 9; ++i)
        {
            const float a = pi * (0.15f + 0.7f * dr.nextFloat());
            const float x = mx + std::cos (a) * mr * 0.96f, y = my + std::sin (a) * mr * 0.96f;
            const float wv = 5.0f + 7.0f * dr.nextFloat();
            const float len = bleed * (40.0f + 180.0f * dr.nextFloat()) * (0.94f + 0.06f * std::sin (t * 1.3f + (float) i));
            g.setColour (Colour (0xff8a000e));
            g.fillRoundedRectangle (x - wv * 0.5f, y - 6.0f, wv, len + 6.0f, wv * 0.5f);
            g.fillEllipse (x - wv * 0.85f, y + len - wv, wv * 1.7f, wv * 2.0f);
        }
    }
}

void ShizukaEditor::drawClouds (juce::Graphics& g, int layer, float rage)
{
    const int n = layer == 0 ? 7 : 5;
    const float scale = layer == 0 ? 26.0f : 46.0f;
    const float speed = layer == 0 ? 4.0f : 9.0f;
    juce::Random cr (layer == 0 ? 51 : 52);

    const auto top = mix (juce::Colours::white, Colour (0xff4a0a14), rage);
    const auto bot = mix (layer == 0 ? Colour (0xffcfdcf7) : Colour (0xffb9c6ee), Colour (0xff12000a), rage);
    const auto rim = mix (Colour (0xfffff4e0), Colour (0xffff3020), rage).withAlpha (layer == 0 ? 0.6f : 0.85f);

    for (int i = 0; i < n; ++i)
    {
        const float span = W + 360.0f;
        const float x = std::fmod (cr.nextFloat() * span + t * speed * (0.7f + 0.6f * cr.nextFloat()), span) - 180.0f;
        const float y = layer == 0 ? 50.0f + cr.nextFloat() * 220.0f : 250.0f + cr.nextFloat() * 160.0f;
        const float s = scale * (0.75f + 0.6f * cr.nextFloat());
        drawCloud (g, x, y, s, top, bot, rim, 0.35f * (1.0f - rage));
    }
}

void ShizukaEditor::drawMountains (juce::Graphics& g, float rage)
{
    struct Layer { float base, amp, f1, p1, f2, p2; Colour calm, angry; };
    const Layer layers[] = {
        { 430.0f, 170.0f, 0.011f, 0.6f, 0.023f, 2.1f, Colour (0xffa6c0ea), Colour (0xff3a0710) },
        { 490.0f, 120.0f, 0.015f, 2.4f, 0.031f, 0.3f, Colour (0xff6c8fc9), Colour (0xff22040a) },
        { 560.0f,  70.0f, 0.009f, 1.1f, 0.021f, 4.0f, Colour (0xff5d9a72), Colour (0xff120205) },
    };

    for (int li = 0; li < 3; ++li)
    {
        const auto& L = layers[li];
        Path m;
        m.startNewSubPath (0.0f, H);
        for (float x = 0.0f; x <= W; x += 4.0f)
            m.lineTo (x, ridge (x, L.base, L.amp, L.f1, L.p1, L.f2, L.p2));
        m.lineTo (W, H);
        m.closeSubPath();

        const auto col = mix (L.calm, L.angry, rage);
        g.setGradientFill (juce::ColourGradient (col.brighter (0.15f), 0.0f, L.base - L.amp, col.darker (0.25f), 0.0f, L.base + 60.0f, false));
        g.fillPath (m);

        {
            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (m);

            if (li < 2)   // snow caps that turn to blood
            {
                const auto cap = mix (juce::Colours::white, Colour (0xffb0101a), rage);
                g.setGradientFill (juce::ColourGradient (cap.withAlpha (0.95f), 0.0f, L.base - L.amp * 1.05f,
                                                         cap.withAlpha (0.0f), 0.0f, L.base - L.amp * 0.5f, false));
                g.fillRect (0.0f, 0.0f, W, L.base);
            }

            g.setColour (juce::Colours::black.withAlpha (0.12f));   // shadow facets
            for (float x = 20.0f; x < W; x += 70.0f)
            {
                Path facet;
                const float py = ridge (x, L.base, L.amp, L.f1, L.p1, L.f2, L.p2);
                facet.addTriangle (x, py, x + 40.0f, H, x - 10.0f, H);
                g.fillPath (facet);
            }

            if (li == 2)   // flowers on the near hill
            {
                const Colour fl[] = { Colour (0xffff8fc8), Colour (0xfffff07a), Colour (0xffffffff), Colour (0xffb08cff) };
                juce::Random fr (77);
                for (int i = 0; i < 70; ++i)
                {
                    const float x = fr.nextFloat() * W;
                    const float y = ridge (x, L.base, L.amp, L.f1, L.p1, L.f2, L.p2) + 6.0f + fr.nextFloat() * 120.0f;
                    g.setColour (mix (fl[i % 4], Colour (0xff8a000e), rage).withAlpha (0.9f));
                    g.fillEllipse (x, y, 4.0f, 4.0f);
                }
            }
        }

        if (rage > 0.0f)   // moon-lit red rim
        {
            Path edge;
            for (float x = 0.0f; x <= W; x += 4.0f)
            {
                const float y = ridge (x, L.base, L.amp, L.f1, L.p1, L.f2, L.p2);
                if (x == 0.0f) edge.startNewSubPath (x, y); else edge.lineTo (x, y);
            }
            g.setColour (Colour (0xffff3020).withAlpha (0.6f * rage));
            g.strokePath (edge, juce::PathStrokeType (2.0f));
        }
    }
}

void ShizukaEditor::drawCritters (juce::Graphics& g, float rage)
{
    juce::Random br (13);
    for (int i = 0; i < 6; ++i)
    {
        const float span = W + 100.0f;
        const float x = std::fmod (br.nextFloat() * span + t * (20.0f + 15.0f * br.nextFloat()), span) - 50.0f;
        const float y = 80.0f + br.nextFloat() * 180.0f + 6.0f * std::sin (t * 2.0f + (float) i);
        const float flap = std::sin (t * 9.0f + (float) i * 1.7f);
        const float s = 6.0f + 4.0f * br.nextFloat();

        if (rage < 0.5f)   // birds
        {
            Path bird;
            bird.startNewSubPath (x - s, y - flap * s * 0.5f);
            bird.quadraticTo (x - s * 0.4f, y - s * 0.2f, x, y);
            bird.quadraticTo (x + s * 0.4f, y - s * 0.2f, x + s, y - flap * s * 0.5f);
            g.setColour (Colour (0xff3a4a7a).withAlpha (1.0f - rage * 2.0f));
            g.strokePath (bird, juce::PathStrokeType (1.8f));
        }
        else   // bats
        {
            Path bat;
            bat.addEllipse (x - s * 0.25f, y - s * 0.25f, s * 0.5f, s * 0.6f);
            bat.addTriangle (x - s * 0.2f, y, x - s * 1.6f, y - flap * s * 0.8f, x - s * 0.9f, y + s * 0.4f);
            bat.addTriangle (x + s * 0.2f, y, x + s * 1.6f, y - flap * s * 0.8f, x + s * 0.9f, y + s * 0.4f);
            g.setColour (juce::Colours::black.withAlpha ((rage - 0.5f) * 2.0f));
            g.fillPath (bat);
        }
    }
}

void ShizukaEditor::drawBloodPool (juce::Graphics& g, float k)
{
    const float a = smoothstep (0.4f, 1.0f, k);
    if (a <= 0.0f)
        return;
    const float pw = 60.0f + 380.0f * a, ph = 18.0f + 40.0f * a;
    const float py = HY + 10.35f * R;
    Path pool = splat (CX, py, 1.0f, 900);
    pool.applyTransform (juce::AffineTransform::scale (pw * 0.32f, ph * 0.32f, CX, py));
    g.setColour (bloodDk.withAlpha (0.95f));
    g.fillPath (pool);
    g.setColour (blood);
    g.fillEllipse (CX - pw * 0.4f, py - ph * 0.35f, pw * 0.8f, ph * 0.7f);
    g.setColour (bloodHi.withAlpha (0.6f));
    g.fillEllipse (CX - pw * 0.25f, py - ph * 0.25f, pw * 0.3f, ph * 0.2f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillEllipse (CX - pw * 0.18f, py - ph * 0.22f, pw * 0.12f, ph * 0.08f);
}

//==============================================================================
//  Custom artwork (art/girl_calm.png, art/girl_rage.png)
//==============================================================================
void ShizukaEditor::drawArt (juce::Graphics& g, float rage)
{
    const juce::Rectangle<float> dest (0.0f, 30.0f, 440.0f, 710.0f);
    const juce::RectanglePlacement place (juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);

    if (artRage.isValid())
    {
        g.setOpacity (1.0f - rage);
        g.drawImage (artCalm, dest, place);
        g.setOpacity (rage);
        g.drawImage (artRage, dest, place);
        g.setOpacity (1.0f);
    }
    else
    {
        g.setOpacity (1.0f);
        g.drawImage (artCalm, dest, place);
        if (rage > 0.0f)   // tint the calm art blood-red using its alpha as a mask
        {
            g.setColour (Colour (0xffc0001a).withAlpha (rage * 0.55f));
            g.drawImage (artCalm, dest, place, true);
        }
    }
}

//==============================================================================
//  The character
//==============================================================================
void ShizukaEditor::drawGirl (juce::Graphics& g, int mood, float rage)
{
    const float sway = 5.0f * std::sin (t * 1.1f);

    const auto hairBase = mix (Colour (0xffeee9ff), Colour (0xffc0102a), rage);
    const auto hairMid  = mix (Colour (0xffbfbcf2), Colour (0xff7a0014), rage);
    const auto hairDark = mix (Colour (0xff7f80d4), Colour (0xff3a0008), rage);
    const auto dress    = mix (Colour (0xff8fd0ff), Colour (0xff7a1020), rage * 0.75f);
    const auto dressSh  = mix (Colour (0xff4f8fdb), Colour (0xff3a0410), rage * 0.75f);
    const auto lace     = mix (juce::Colours::white, Colour (0xffe8c0c0), rage);
    const auto ribbon   = mix (Colour (0xffff7fc0), Colour (0xff9a0012), rage);

    // ---------------- back hair: a mass + individual wind-blown strands
    Path mass;
    mass.startNewSubPath (CX - 0.95f * R, HY - 0.25f * R);
    mass.cubicTo (CX - 1.15f * R, HY - 1.35f * R, CX + 1.15f * R, HY - 1.35f * R, CX + 0.95f * R, HY - 0.25f * R);
    mass.cubicTo (CX + 1.4f * R, HY + 2.0f * R, CX + 1.75f * R + sway, HY + 4.5f * R, CX + 1.6f * R + sway, HY + 7.0f * R);
    mass.lineTo (CX - 1.6f * R + sway, HY + 7.0f * R);
    mass.cubicTo (CX - 1.75f * R + sway, HY + 4.5f * R, CX - 1.4f * R, HY + 2.0f * R, CX - 0.95f * R, HY - 0.25f * R);
    mass.closeSubPath();
    g.setGradientFill (juce::ColourGradient (hairMid, CX, HY - R, hairDark, CX, HY + 7.0f * R, false));
    g.fillPath (mass);

    const int strands = 15;
    for (int i = 0; i < strands; ++i)
    {
        const float f = (float) i / (float) (strands - 1) * 2.0f - 1.0f;
        const float x0 = CX + f * 0.95f * R;
        const float ph = (float) i * 0.7f;
        const float bend = sway * (0.6f + 0.4f * std::sin (t * 1.3f + ph));
        const float x1 = CX + f * 1.75f * R + bend;
        const float y1 = HY + (6.6f + 0.7f * std::sin ((float) i * 2.3f)) * R;
        auto sp = strand (x0, HY - 0.3f * R, x1, y1, 0.55f * R, bend + f * 6.0f);
        g.setGradientFill (juce::ColourGradient ((i % 2 == 0 ? hairBase : hairMid), CX, HY, hairDark, CX, y1, false));
        g.fillPath (sp);
        g.setColour (ink.withAlpha (0.55f));
        g.strokePath (sp, juce::PathStrokeType (1.2f));
        g.setColour (hairDark.withAlpha (0.5f));
        g.drawLine (x0 + bend * 0.2f, HY + 1.5f * R, x1 - (x1 - x0) * 0.1f, y1 - 0.6f * R, 1.0f);

        if (mood >= 3 && i % 3 == 0)   // blood dripping off hair tips
        {
            const float len = rage * (8.0f + 18.0f * (0.5f + 0.5f * std::sin (t * 2.0f + ph)));
            g.setColour (blood);
            g.fillRoundedRectangle (x1 - 2.0f, y1 - 4.0f, 4.0f, len + 4.0f, 2.0f);
            g.fillEllipse (x1 - 3.5f, y1 + len - 2.0f, 7.0f, 8.0f);
        }
    }

    // ---------------- arms (hands clasped behind her back)
    for (int s = -1; s <= 1; s += 2)
    {
        const float f = (float) s;
        Path arm;
        arm.startNewSubPath (CX + f * 1.15f * R, HY + 2.1f * R);
        arm.quadraticTo (CX + f * 1.42f * R, HY + 3.0f * R, CX + f * 1.3f * R, HY + 3.7f * R);
        arm.quadraticTo (CX + f * 1.15f * R, HY + 4.2f * R, CX + f * 0.8f * R, HY + 4.35f * R);
        g.setColour (ink);
        g.strokePath (arm, juce::PathStrokeType (0.3f * R + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (skin);
        g.strokePath (arm, juce::PathStrokeType (0.3f * R, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (skinSh.withAlpha (0.8f));
        g.drawLine (CX + f * 1.25f * R, HY + 3.1f * R, CX + f * 1.22f * R, HY + 3.6f * R, 3.0f);
        if (mood >= 3)
            paintSplat (g, CX + f * 1.3f * R, HY + 3.3f * R, 0.07f * R, 300 + s, 1.0f, rage * 12.0f);
    }

    // ---------------- long skirt with ruffled, wind-blown hem
    const float wy = HY + 3.9f * R, hemY = HY + 10.2f * R;

    for (int s = -1; s <= 1; s += 2)   // shoes peeking out
    {
        const juce::Rectangle<float> shoe (CX + (float) s * 0.35f * R - 0.24f * R, hemY - 0.05f * R, 0.48f * R, 0.26f * R);
        g.setColour (mix (Colour (0xffffffff), Colour (0xff120004), rage));
        g.fillRoundedRectangle (shoe, 0.12f * R);
        g.setColour (ink);
        g.drawRoundedRectangle (shoe, 0.12f * R, 1.8f);
    }

    g.setColour (lace);   // lace petticoat scallops
    for (int i = 0; i < 18; ++i)
    {
        const float x = CX - 1.75f * R + 3.5f * R * (float) i / 17.0f + sway;
        g.fillEllipse (x - 0.13f * R, hemY - 0.05f * R, 0.26f * R, 0.24f * R);
    }

    Path skirt;
    skirt.startNewSubPath (CX - 0.62f * R, wy);
    skirt.cubicTo (CX - 0.95f * R, wy + 2.0f * R, CX - 1.5f * R + sway * 0.5f, hemY - 2.5f * R, CX - 1.75f * R + sway, hemY);
    const int folds = 12;
    for (int i = 1; i <= folds; ++i)
    {
        const float x = CX - 1.75f * R + 3.5f * R * (float) i / (float) folds + sway;
        const float xm = x - 3.5f * R / (float) folds * 0.5f;
        const float wave = 0.05f * R * std::sin (t * 3.0f + (float) i);
        skirt.quadraticTo (xm, hemY + 0.2f * R + wave, x, hemY + wave);
    }
    skirt.cubicTo (CX + 1.5f * R + sway * 0.5f, hemY - 2.5f * R, CX + 0.95f * R, wy + 2.0f * R, CX + 0.62f * R, wy);
    skirt.closeSubPath();

    g.setGradientFill (juce::ColourGradient (dress.brighter (0.1f), CX - R, wy, dressSh, CX + 1.5f * R, hemY, false));
    g.fillPath (skirt);
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (skirt);

        for (int f = -3; f <= 3; ++f)   // soft folds
        {
            Path fold;
            fold.startNewSubPath (CX + (float) f * 0.18f * R, wy + 0.3f * R);
            fold.quadraticTo (CX + (float) f * 0.45f * R + sway * 0.3f, hemY - 3.0f * R, CX + (float) f * 0.52f * R + sway, hemY + 0.2f * R);
            g.setColour (dressSh.withAlpha (0.45f));
            g.strokePath (fold, juce::PathStrokeType (0.16f * R));
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.strokePath (fold, juce::PathStrokeType (2.0f), juce::AffineTransform::translation (0.1f * R, 0.0f));
        }
        g.setColour (dressSh.withAlpha (0.35f));
        g.fillRect (CX + 0.5f * R, wy, 1.5f * R, hemY - wy + R);

        g.setColour (lace.withAlpha (0.9f));   // lace trim band
        g.fillRect (CX - 2.0f * R, hemY - 0.45f * R, 4.0f * R, 0.12f * R);

        if (rage > 0.0f)
        {
            g.setGradientFill (juce::ColourGradient (blood.withAlpha (0.0f), 0.0f, hemY - 3.0f * R, blood.withAlpha (0.9f * rage), 0.0f, hemY, false));
            g.fillRect (CX - 2.0f * R, hemY - 3.0f * R, 4.0f * R, 3.4f * R);

            const int n = (int) (rage * 14.0f);
            juce::Random st (404);
            for (int i = 0; i < n; ++i)
            {
                const float x = CX + (st.nextFloat() - 0.5f) * 2.6f * R;
                const float y = wy + st.nextFloat() * 5.5f * R;
                paintSplat (g, x, y, (0.1f + 0.2f * st.nextFloat()) * R, 1000 + i, 1.0f, rage * (10.0f + 50.0f * st.nextFloat()));
            }
        }
    }
    g.setColour (ink);
    g.strokePath (skirt, juce::PathStrokeType (2.2f));

    // ---------------- bodice
    Path bodice;
    bodice.startNewSubPath (CX - 0.32f * R, HY + 1.3f * R);
    bodice.cubicTo (CX - 0.8f * R, HY + 1.4f * R, CX - 1.15f * R, HY + 1.6f * R, CX - 1.2f * R, HY + 2.0f * R);
    bodice.cubicTo (CX - 1.05f * R, HY + 2.8f * R, CX - 0.75f * R, HY + 3.4f * R, CX - 0.64f * R, HY + 3.95f * R);
    bodice.lineTo (CX + 0.64f * R, HY + 3.95f * R);
    bodice.cubicTo (CX + 0.75f * R, HY + 3.4f * R, CX + 1.05f * R, HY + 2.8f * R, CX + 1.2f * R, HY + 2.0f * R);
    bodice.cubicTo (CX + 1.15f * R, HY + 1.6f * R, CX + 0.8f * R, HY + 1.4f * R, CX + 0.32f * R, HY + 1.3f * R);
    bodice.closeSubPath();
    g.setGradientFill (juce::ColourGradient (dress.brighter (0.15f), CX - R, HY + 1.4f * R, dress, CX + R, HY + 3.9f * R, false));
    g.fillPath (bodice);
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (bodice);
        g.setColour (dressSh.withAlpha (0.45f));
        g.fillRect (CX + 0.45f * R, HY + 1.3f * R, R, 2.7f * R);
        g.setColour (dressSh.withAlpha (0.5f));   // pintucks
        for (int i = -2; i <= 2; ++i)
            g.drawLine (CX + (float) i * 0.14f * R, HY + 1.75f * R, CX + (float) i * 0.12f * R, HY + 3.8f * R, 1.2f);
        g.setColour (lace);   // pearl buttons
        for (int i = 0; i < 4; ++i)
            g.fillEllipse (CX - 0.05f * R, HY + (1.9f + 0.45f * (float) i) * R, 0.1f * R, 0.1f * R);
        if (mood >= 3)
        {
            juce::Random st (606);
            for (int i = 0; i < (int) (rage * 6.0f); ++i)
                paintSplat (g, CX + (st.nextFloat() - 0.5f) * 1.8f * R, HY + (1.7f + st.nextFloat() * 2.0f) * R,
                            (0.1f + 0.15f * st.nextFloat()) * R, 2000 + i, 1.0f, rage * 25.0f);
        }
    }
    g.setColour (ink);
    g.strokePath (bodice, juce::PathStrokeType (2.2f));

    // puffy sleeves with lace cuffs
    for (int s = -1; s <= 1; s += 2)
    {
        const float sx = CX + (float) s * 1.18f * R, sy = HY + 2.15f * R;
        const juce::Rectangle<float> puff (sx - 0.36f * R, sy - 0.32f * R, 0.72f * R, 0.62f * R);
        g.setGradientFill (juce::ColourGradient (dress.brighter (0.2f), sx, puff.getY(), dressSh, sx, puff.getBottom(), false));
        g.fillEllipse (puff);
        g.setColour (ink);
        g.drawEllipse (puff, 2.0f);
        g.setColour (dressSh.withAlpha (0.6f));
        for (int i = -1; i <= 1; ++i)
            g.drawLine (sx + (float) i * 0.14f * R, puff.getY() + 0.1f * R, sx + (float) i * 0.18f * R, puff.getBottom() - 0.1f * R, 1.3f);
        g.setColour (lace);
        for (int i = 0; i < 5; ++i)
            g.fillEllipse (puff.getX() + (float) i * 0.14f * R, puff.getBottom() - 0.1f * R, 0.16f * R, 0.14f * R);
    }

    // neck, choker with moon charm, frilled neckline
    g.setColour (skin);
    g.fillRect (CX - 0.18f * R, HY + 0.78f * R, 0.36f * R, 0.6f * R);
    g.setColour (skinSh);
    g.fillEllipse (CX - 0.22f * R, HY + 0.78f * R, 0.44f * R, 0.22f * R);
    g.setColour (ink);
    g.drawLine (CX - 0.18f * R, HY + 0.85f * R, CX - 0.18f * R, HY + 1.3f * R, 1.5f);
    g.drawLine (CX + 0.18f * R, HY + 0.85f * R, CX + 0.18f * R, HY + 1.3f * R, 1.5f);
    g.setColour (ribbon);
    g.fillRect (CX - 0.19f * R, HY + 1.02f * R, 0.38f * R, 0.08f * R);
    g.setColour (mix (Colour (0xffffe27a), Colour (0xffff4040), rage));
    g.fillEllipse (CX - 0.06f * R, HY + 1.1f * R, 0.12f * R, 0.12f * R);
    g.setColour (lace);
    for (int i = 0; i < 9; ++i)
    {
        const float x = CX - 0.42f * R + 0.105f * R * (float) i;
        const float y = HY + 1.28f * R + 0.018f * R * std::abs ((float) i - 4.0f);
        g.fillEllipse (x - 0.07f * R, y, 0.14f * R, 0.14f * R);
    }

    // sash and bow
    const juce::Rectangle<float> sash (CX - 0.68f * R, HY + 3.78f * R, 1.36f * R, 0.26f * R);
    g.setColour (ribbon);
    g.fillRoundedRectangle (sash, 0.08f * R);
    g.setColour (ink);
    g.drawRoundedRectangle (sash, 0.08f * R, 1.8f);
    const float bx = CX - 0.35f * R, by = HY + 3.91f * R;
    for (int s = -1; s <= 1; s += 2)
    {
        Path loop;
        loop.addEllipse (-0.28f * R, -0.13f * R, 0.56f * R, 0.26f * R);
        loop.applyTransform (juce::AffineTransform::rotation ((float) s * 0.35f).translated (bx + (float) s * 0.26f * R, by - 0.03f * R));
        g.setColour (ribbon);
        g.fillPath (loop);
        g.setColour (ink);
        g.strokePath (loop, juce::PathStrokeType (1.8f));

        Path tail = strand (bx + (float) s * 0.05f * R, by, bx + (float) s * 0.28f * R + sway * 0.6f, by + 1.3f * R, 0.18f * R, sway * 0.3f);
        g.setColour (ribbon);
        g.fillPath (tail);
        g.setColour (ink);
        g.strokePath (tail, juce::PathStrokeType (1.5f));
    }
    g.setColour (ribbon.darker (0.2f));
    g.fillEllipse (bx - 0.1f * R, by - 0.11f * R, 0.2f * R, 0.22f * R);

    drawHead (g, mood, rage);

    // long locks falling in front of the shoulders
    for (int s = -1; s <= 1; s += 2)
    {
        const float f = (float) s;
        for (int j = 0; j < 2; ++j)
        {
            const float x0 = CX + f * (0.82f - 0.12f * (float) j) * R;
            const float bend = sway * (0.5f + 0.25f * (float) j);
            auto lock = strand (x0, HY - 0.45f * R, x0 + f * (0.3f + 0.15f * (float) j) * R + bend, HY + (4.4f - 0.5f * (float) j) * R, 0.3f * R, bend);
            g.setGradientFill (juce::ColourGradient (hairBase, CX, HY, hairMid, CX, HY + 4.4f * R, false));
            g.fillPath (lock);
            g.setColour (ink);
            g.strokePath (lock, juce::PathStrokeType (1.5f));
            g.setColour (juce::Colours::white.withAlpha (0.5f * (1.0f - rage)));
            g.drawLine (x0 - f * 0.03f * R, HY + 0.5f * R, x0 + f * 0.12f * R + bend * 0.5f, HY + 2.4f * R, 1.5f);
        }
    }
}

void ShizukaEditor::drawHead (juce::Graphics& g, int mood, float rage)
{
    const auto hairBase = mix (Colour (0xffeee9ff), Colour (0xffc0102a), rage);
    const auto hairMid  = mix (Colour (0xffbfbcf2), Colour (0xff7a0014), rage);
    const auto hairDark = mix (Colour (0xff7f80d4), Colour (0xff3a0008), rage);
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
        g.setColour (skinSh.withAlpha (0.65f));
        g.fillRect (CX - R, HY - R, 2.0f * R, 0.72f * R);
        g.fillEllipse (CX + 0.45f * R, HY - 0.3f * R, 0.6f * R, 1.4f * R);

        if (mood >= 3)   // yandere shadow
        {
            g.setGradientFill (juce::ColourGradient (Colour (0xff1a0010).withAlpha (mood == 4 ? 0.75f : 0.55f), CX, HY - 0.5f * R,
                                                     juce::Colours::transparentBlack, CX, HY + 0.45f * R, false));
            g.fillRect (CX - R, HY - R, 2.0f * R, 1.6f * R);
        }
    }
    g.setColour (ink);
    g.strokePath (face, juce::PathStrokeType (2.0f));

    // gradient blush
    for (int s = -1; s <= 1; s += 2)
    {
        const float bx = CX + (float) s * 0.48f * R, by = HY + 0.46f * R;
        const auto bc = mix (Colour (0xffff7fb0), Colour (0xffff2040), rage);
        g.setGradientFill (juce::ColourGradient (bc.withAlpha (0.6f), bx, by, bc.withAlpha (0.0f), bx + 0.24f * R, by, true));
        g.fillEllipse (bx - 0.24f * R, by - 0.12f * R, 0.48f * R, 0.24f * R);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        for (int i = 0; i < 3; ++i)
        {
            const float x = bx + ((float) i - 1.0f) * 0.07f * R;
            g.drawLine (x + 0.02f * R, by - 0.04f * R, x - 0.02f * R, by + 0.05f * R, 1.2f);
        }
    }

    // ---------------- eyes
    const float eyeY = HY + 0.14f * R;
    const float browIn[] = { -0.02f, -0.03f, 0.07f, 0.0f, 0.1f };
    for (int s = -1; s <= 1; s += 2)
    {
        const float ex = CX + (float) s * 0.35f * R;
        const float ew = 0.38f * R, eh = 0.46f * R;
        const float inner = (float) -s;

        if (mood == 0)
        {
            Path closed;
            closed.addCentredArc (ex, eyeY + 0.04f * R, ew * 0.5f, eh * 0.25f, 0.0f, -pi * 0.5f + 0.2f, pi * 0.5f - 0.2f, true);
            g.setColour (ink);
            g.strokePath (closed, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            for (int l = 0; l < 3; ++l)
            {
                const float lx = ex - inner * ew * (0.2f + 0.14f * (float) l);
                g.drawLine (lx, eyeY + 0.1f * R, lx - inner * 0.04f * R, eyeY + 0.17f * R, 1.5f);
            }
        }
        else
        {
            const float squint = (mood == 2) ? 0.28f : 0.0f;
            Path eye;
            eye.addEllipse (ex - ew * 0.5f, eyeY - eh * 0.5f, ew, eh);
            g.setGradientFill (juce::ColourGradient (Colour (0xffd9dcf2), ex, eyeY - eh * 0.5f, juce::Colours::white, ex, eyeY, false));
            g.fillPath (eye);

            {
                juce::Graphics::ScopedSaveState clip (g);
                g.reduceClipRegion (eye);

                const auto irisTop = mix (Colour (0xff1b3fb0), Colour (0xff8a0010), rage);
                const auto irisMid = mix (Colour (0xff3fa0ff), Colour (0xffff2030), rage);
                const auto irisBot = mix (Colour (0xffa8fff0), Colour (0xffffb040), rage);
                const float iw = ew * 0.76f, ih = eh * 0.9f;
                const float ix = ex + inner * 0.02f * R, iy = eyeY + eh * 0.06f;

                juce::ColourGradient ig (irisTop, ix, iy - ih * 0.5f, irisBot, ix, iy + ih * 0.5f, false);
                ig.addColour (0.5, irisMid);
                g.setGradientFill (ig);
                g.fillEllipse (ix - iw * 0.5f, iy - ih * 0.5f, iw, ih);

                g.setColour (irisTop.withAlpha (0.35f));   // radial striations
                for (int l = 0; l < 14; ++l)
                {
                    const float a = 2.0f * pi * (float) l / 14.0f;
                    g.drawLine (ix + std::cos (a) * iw * 0.16f, iy + std::sin (a) * ih * 0.16f,
                                ix + std::cos (a) * iw * 0.46f, iy + std::sin (a) * ih * 0.46f, 0.8f);
                }
                g.setColour (irisTop.darker (0.5f));
                g.drawEllipse (ix - iw * 0.5f, iy - ih * 0.5f, iw, ih, 2.0f);
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.fillRect (ex - ew, eyeY - eh * 0.5f, ew * 2.0f, eh * 0.18f);

                if (mood >= 3)
                {
                    g.setColour (bloodDk.withAlpha (0.75f));
                    g.drawEllipse (ix - iw * 0.32f, iy - ih * 0.32f, iw * 0.64f, ih * 0.64f, 1.5f);
                    g.drawEllipse (ix - iw * 0.2f, iy - ih * 0.2f, iw * 0.4f, ih * 0.4f, 1.2f);
                    g.setColour (juce::Colours::black);
                    if (mood == 4)
                        g.fillEllipse (ix - 1.8f, iy - ih * 0.3f, 3.6f, ih * 0.6f);   // slit pupil
                    else
                        g.fillEllipse (ix - 2.0f, iy - 3.0f, 4.0f, 6.0f);
                }
                else
                {
                    g.setColour (irisTop.darker (0.6f));
                    g.fillEllipse (ix - iw * 0.2f, iy - ih * 0.22f, iw * 0.4f, ih * 0.46f);
                    g.setColour (irisMid.withAlpha (0.6f));
                    g.drawEllipse (ix - iw * 0.2f, iy - ih * 0.22f, iw * 0.4f, ih * 0.46f, 1.0f);
                    g.setColour (juce::Colours::white);
                    g.fillEllipse (ix - iw * 0.4f, iy - ih * 0.4f, iw * 0.36f, iw * 0.3f);
                    g.fillEllipse (ix + iw * 0.14f, iy + ih * 0.16f, iw * 0.14f, iw * 0.14f);
                    Path tw;
                    tw.addStar ({ ix + iw * 0.2f, iy - ih * 0.18f }, 4, 1.0f, 4.0f + std::sin (t * 4.0f), 0.0f);
                    g.fillPath (tw);
                }

                if (squint > 0.0f)
                {
                    g.setColour (skin);
                    g.fillRect (ex - ew, eyeY - eh, ew * 2.0f, eh * (0.5f + squint));
                }
            }

            if (mood == 4)
                for (int i = 3; i >= 1; --i)
                {
                    g.setColour (bloodHi.withAlpha (0.13f * (float) i));
                    const float gw = ew * (0.55f + 0.25f * (float) i), gh = eh * (0.5f + 0.2f * (float) i);
                    g.fillEllipse (ex - gw * 0.5f, eyeY - gh * 0.5f, gw, gh);
                }

            // thick upper lid + lash flicks
            const float lidY = eyeY - eh * 0.5f + eh * squint;
            Path lid;
            lid.startNewSubPath (ex + inner * ew * 0.58f, lidY + eh * 0.16f);
            lid.quadraticTo (ex, lidY - eh * 0.2f, ex - inner * ew * 0.66f, lidY + eh * 0.06f);
            lid.lineTo (ex - inner * ew * 0.62f, lidY + eh * 0.14f);
            lid.quadraticTo (ex, lidY - eh * 0.06f, ex + inner * ew * 0.55f, lidY + eh * 0.2f);
            lid.closeSubPath();
            g.setColour (ink);
            g.fillPath (lid);
            for (int l = 0; l < 3; ++l)
            {
                const float lx = ex - inner * ew * (0.48f + 0.08f * (float) l);
                const float ly = lidY + eh * (0.02f + 0.04f * (float) l);
                g.drawLine (lx, ly, lx - inner * 0.12f * R, ly - (0.1f - 0.03f * (float) l) * R, 2.0f);
            }
            g.setColour (ink.withAlpha (0.5f));   // double-eyelid crease
            Path crease;
            crease.startNewSubPath (ex + inner * ew * 0.4f, lidY - eh * 0.08f);
            crease.quadraticTo (ex, lidY - eh * 0.28f, ex - inner * ew * 0.5f, lidY - eh * 0.06f);
            g.strokePath (crease, juce::PathStrokeType (1.2f));
            g.setColour (ink);   // lower lashes
            g.drawLine (ex - inner * ew * 0.35f, eyeY + eh * 0.5f, ex + inner * ew * 0.2f, eyeY + eh * 0.52f, 1.4f);
            g.drawLine (ex - inner * ew * 0.42f, eyeY + eh * 0.44f, ex - inner * ew * 0.5f, eyeY + eh * 0.52f, 1.2f);

            if (mood == 4)   // blood tears
            {
                const float len = 0.5f * R + 0.35f * R * (0.5f + 0.5f * std::sin (t * 1.7f + (float) s));
                g.setColour (blood);
                g.fillRoundedRectangle (ex - inner * 0.05f * R - 2.0f, eyeY + eh * 0.45f, 4.0f, len, 2.0f);
                g.fillEllipse (ex - inner * 0.05f * R - 3.5f, eyeY + eh * 0.45f + len - 3.0f, 7.0f, 8.0f);
                g.setColour (bloodHi);
                g.fillRoundedRectangle (ex - inner * 0.05f * R - 0.8f, eyeY + eh * 0.45f, 1.2f, len * 0.8f, 0.6f);
            }
        }

        const float by = HY - 0.26f * R;
        Path brow;
        brow.startNewSubPath (ex - inner * 0.16f * R, by);
        brow.quadraticTo (ex, by - 0.07f * R, ex + inner * 0.15f * R, by + browIn[mood] * R);
        g.setColour (hairDark.darker (0.3f));
        g.strokePath (brow, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // nose with a tiny highlight
    g.setColour (skinSh.darker (0.25f));
    g.drawLine (CX + 0.03f * R, HY + 0.38f * R, CX - 0.01f * R, HY + 0.43f * R, 1.8f);
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.fillEllipse (CX - 0.035f * R, HY + 0.33f * R, 0.04f * R, 0.04f * R);

    // ---------------- mouth
    const float my = HY + 0.62f * R;
    if (mood == 0)
    {
        Path m;
        m.startNewSubPath (CX - 0.09f * R, my);
        m.quadraticTo (CX, my + 0.06f * R, CX + 0.09f * R, my);
        g.setColour (ink);
        g.strokePath (m, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else if (mood == 1)
    {
        Path m;   // small open happy smile
        m.startNewSubPath (CX - 0.11f * R, my - 0.02f * R);
        m.quadraticTo (CX, my + 0.02f * R, CX + 0.11f * R, my - 0.02f * R);
        m.quadraticTo (CX, my + 0.18f * R, CX - 0.11f * R, my - 0.02f * R);
        m.closeSubPath();
        g.setColour (Colour (0xffc8405a));
        g.fillPath (m);
        g.setColour (Colour (0xffff9aa8));
        g.fillEllipse (CX - 0.05f * R, my + 0.05f * R, 0.1f * R, 0.06f * R);
        g.setColour (ink);
        g.strokePath (m, juce::PathStrokeType (1.6f));
    }
    else if (mood == 2)
    {
        Path m;   // pout
        m.startNewSubPath (CX - 0.1f * R, my + 0.02f * R);
        m.quadraticTo (CX - 0.05f * R, my - 0.03f * R, CX, my + 0.01f * R);
        m.quadraticTo (CX + 0.05f * R, my - 0.03f * R, CX + 0.1f * R, my + 0.02f * R);
        g.setColour (ink);
        g.strokePath (m, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
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
            g.fillRect (CX - w * 0.5f, my - 0.06f * R, w, 0.08f * R);
            g.setColour (ink.withAlpha (0.4f));
            for (int i = 1; i < 6; ++i)
                g.drawLine (CX - w * 0.5f + w * (float) i / 6.0f, my - 0.06f * R, CX - w * 0.5f + w * (float) i / 6.0f, my + 0.02f * R, 1.0f);
            g.setColour (Colour (0xffe8708a));
            g.fillEllipse (CX - w * 0.2f, my + h * 0.5f, w * 0.4f, h * 0.6f);
        }
        g.setColour (ink);
        g.strokePath (m, juce::PathStrokeType (2.0f));

        if (mood == 4)   // drip from the corner of her grin
        {
            const float len = 0.3f * R + 0.2f * R * (0.5f + 0.5f * std::sin (t * 2.2f));
            g.setColour (blood);
            g.fillRoundedRectangle (CX + w * 0.42f, my, 3.5f, len, 1.75f);
            g.fillEllipse (CX + w * 0.42f - 2.0f, my + len - 3.0f, 7.5f, 8.0f);
        }
    }

    if (mood >= 3)   // blood on her face
    {
        paintSplat (g, CX + 0.5f * R, HY + 0.55f * R, 0.1f * R * (mood == 4 ? 1.7f : 1.0f), 77, 1.0f, 0.2f * R * rage);
        if (mood == 4)
        {
            paintSplat (g, CX - 0.55f * R, HY + 0.62f * R, 0.12f * R, 78, 1.0f, 0.25f * R);
            paintSplat (g, CX - 0.2f * R, HY - 0.45f * R, 0.09f * R, 79, 1.0f, 0.35f * R);
        }
    }

    // ---------------- bangs with strand detail
    Path bangs;
    bangs.startNewSubPath (CX - 0.93f * R, HY + 0.05f * R);
    bangs.cubicTo (CX - 1.02f * R, HY - 1.2f * R, CX + 1.02f * R, HY - 1.2f * R, CX + 0.93f * R, HY + 0.05f * R);
    const float tipY[] = { 0.05f, -0.42f, -0.1f, -0.5f, 0.12f, -0.46f, -0.02f, -0.5f, 0.08f, -0.44f, -0.08f, -0.46f, 0.04f };
    const int nTips = (int) (sizeof (tipY) / sizeof (tipY[0]));
    for (int i = 0; i < nTips; ++i)
    {
        const float x = CX + 0.88f * R - 1.76f * R * (float) i / (float) (nTips - 1);
        bangs.lineTo (x, HY + (tipY[i] - 0.05f) * R);
    }
    bangs.closeSubPath();
    g.setGradientFill (juce::ColourGradient (hairBase, CX, HY - R, hairMid, CX, HY, false));
    g.fillPath (bangs);
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (bangs);
        g.setColour (hairDark.withAlpha (0.55f));
        for (int i = 0; i < nTips; i += 2)
        {
            const float x = CX + 0.88f * R - 1.76f * R * (float) i / (float) (nTips - 1);
            g.drawLine (x * 0.7f + CX * 0.3f, HY - 0.85f * R, x, HY + (tipY[i] - 0.1f) * R, 1.2f);
        }
    }
    g.setColour (ink);
    g.strokePath (bangs, juce::PathStrokeType (2.0f));

    // ahoge
    Path ahoge;
    ahoge.startNewSubPath (CX + 0.05f * R, HY - 0.98f * R);
    ahoge.cubicTo (CX + 0.1f * R, HY - 1.5f * R, CX + 0.55f * R + 3.0f * std::sin (t * 2.0f), HY - 1.55f * R, CX + 0.45f * R, HY - 1.25f * R);
    g.setColour (hairBase);
    g.strokePath (ahoge, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (ink);
    g.strokePath (ahoge, juce::PathStrokeType (1.2f));

    // jagged angel-ring highlight + sparkles
    Path ring;
    for (int i = 0; i <= 24; ++i)
    {
        const float a = -1.25f + 2.5f * (float) i / 24.0f;
        const float px = CX + std::sin (a) * 0.7f * R;
        const float py = HY - 0.55f * R - std::cos (a) * 0.11f * R + ((i % 2 == 0) ? 0.0f : 0.05f * R);
        if (i == 0) ring.startNewSubPath (px, py); else ring.lineTo (px, py);
    }
    g.setColour (juce::Colours::white.withAlpha (0.85f - 0.45f * rage));
    g.strokePath (ring, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    for (int i = 0; i < 3; ++i)
    {
        Path sp;
        sp.addStar ({ CX - 0.4f * R + (float) i * 0.4f * R, HY - 0.62f * R - (float) (i % 2) * 0.08f * R }, 4, 1.0f,
                    4.0f + 1.5f * std::sin (t * 4.0f + (float) i), 0.0f);
        g.setColour (juce::Colours::white.withAlpha (1.0f - rage));
        g.fillPath (sp);
    }

    // golden crescent hairpin with a star
    {
        const float hx = CX - 0.66f * R, hy = HY - 0.48f * R, hr = 0.16f * R;
        g.setColour (mix (Colour (0xffffd75a), Colour (0xffff3030), rage));
        g.fillEllipse (hx - hr, hy - hr, hr * 2.0f, hr * 2.0f);
        g.setColour (hairBase);
        g.fillEllipse (hx - hr * 0.4f, hy - hr * 1.15f, hr * 1.7f, hr * 1.7f);
        g.setColour (ink);
        g.drawEllipse (hx - hr, hy - hr, hr * 2.0f, hr * 2.0f, 1.2f);
        Path pinStar;
        pinStar.addStar ({ hx - 0.22f * R, hy + 0.2f * R }, 5, 0.04f * R, 0.09f * R, 0.0f);
        g.setColour (mix (Colour (0xff9ff0ff), Colour (0xffff6060), rage));
        g.fillPath (pinStar);
    }
    // ribbon bow
    {
        const float rx = CX + 0.7f * R, ry = HY - 0.42f * R;
        const auto rc = mix (Colour (0xffff7fc0), Colour (0xff2a0004), rage);
        for (int s = -1; s <= 1; s += 2)
        {
            Path loop;
            loop.addEllipse (-0.16f * R, -0.09f * R, 0.32f * R, 0.18f * R);
            loop.applyTransform (juce::AffineTransform::rotation ((float) s * 0.5f).translated (rx + (float) s * 0.14f * R, ry));
            g.setColour (rc);
            g.fillPath (loop);
            g.setColour (ink);
            g.strokePath (loop, juce::PathStrokeType (1.3f));
        }
        g.setColour (rc.darker (0.2f));
        g.fillEllipse (rx - 0.06f * R, ry - 0.06f * R, 0.12f * R, 0.12f * R);
    }
}

//==============================================================================
void ShizukaEditor::drawPanel (juce::Graphics& g, float k, int mood, float rage)
{
    const juce::Rectangle<float> panel (448.0f, 16.0f, 236.0f, 708.0f);
    g.setGradientFill (juce::ColourGradient (mix (Colour (0x88182a66), Colour (0x99200008), rage), 0.0f, panel.getY(),
                                             mix (Colour (0x880a1030), Colour (0xcc080002), rage), 0.0f, panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 22.0f);
    g.setColour (mix (Colour (0xffbfe8ff), bloodHi, rage).withAlpha (0.7f));
    g.drawRoundedRectangle (panel, 22.0f, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (panel.reduced (5.0f), 18.0f, 1.0f);

    // glowing title
    const juce::Rectangle<float> title (panel.getX(), 26.0f, panel.getWidth(), 48.0f);
    g.setFont (fontOf (34.0f));
    const auto glow = mix (Colour (0xff7fe8ff), bloodHi, rage);
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (glow.withAlpha (0.12f));
        g.drawText ("HUSH-CHAN", title.translated ((float) i, 0.0f), juce::Justification::centred);
        g.drawText ("HUSH-CHAN", title.translated ((float) -i, 0.0f), juce::Justification::centred);
        g.drawText ("HUSH-CHAN", title.translated (0.0f, (float) i), juce::Justification::centred);
    }
    g.setGradientFill (juce::ColourGradient (mix (Colour (0xffffb0e0), Colour (0xffff4040), rage), title.getX(), 0.0f,
                                             mix (Colour (0xffb0f4ff), Colour (0xffff9a40), rage), title.getRight(), 0.0f, false));
    g.drawText ("HUSH-CHAN", title, juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (fontOf (13.0f, juce::Font::italic));
    g.drawText ("~ spectral gate ~", title.translated (0.0f, 34.0f).withHeight (18.0f), juce::Justification::centred);

    // spectrum display
    const juce::Rectangle<float> spec (462.0f, 106.0f, 208.0f, 140.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (spec, 10.0f);
    g.setColour (glow.withAlpha (0.4f));
    g.drawRoundedRectangle (spec, 10.0f, 1.2f);
    const auto area = spec.reduced (8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (int i = 1; i < 5; ++i)
        g.drawHorizontalLine ((int) (area.getY() + area.getHeight() * (float) i / 5.0f), area.getX(), area.getRight());
    const float bw = area.getWidth() / (float) ShizukaProcessor::numBands;
    const float thresh = ShizukaProcessor::thresholdDbFor (k);
    auto yFor = [&] (float db) { return area.getBottom() - area.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 100.0f) / 100.0f); };
    for (int i = 0; i < ShizukaProcessor::numBands; ++i)
    {
        const float db = bandsUI[(size_t) i];
        const float y = yFor (db);
        const bool cut = k > 0.001f && db < thresh;
        const auto col = mix (Colour::fromHSV (0.5f + (float) i / 48.0f * 0.45f, 0.55f, 1.0f, 1.0f), bloodHi, rage);
        const juce::Rectangle<float> bar (area.getX() + (float) i * bw + 0.5f, y, bw - 1.0f, area.getBottom() - y);
        if (cut)
            g.setColour (juce::Colours::white.withAlpha (0.1f));
        else
            g.setGradientFill (juce::ColourGradient (col, 0.0f, y, col.withAlpha (0.25f), 0.0f, area.getBottom(), false));
        g.fillRect (bar);
        if (! cut)
        {
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillRect (bar.getX(), y - 1.5f, bar.getWidth(), 1.5f);
        }
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
    const juce::Rectangle<float> bub (462.0f, 264.0f, 208.0f, 110.0f);
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
    g.setColour (mood == 4 ? blood : juce::Colours::white.withAlpha (0.95f));
    g.fillPath (bp);
    g.setColour (ink);
    g.strokePath (bp, juce::PathStrokeType (3.0f));
    g.setColour (mood == 4 ? juce::Colours::white : ink);
    g.setFont (fontOf (mood == 4 ? 34.0f : 20.0f));
    g.drawFittedText (lines[mood], bub.reduced (12.0f).toNearestInt(), juce::Justification::centred, 3);

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (fontOf (14.0f, juce::Font::italic));
    g.drawFittedText (taglines[tagline], juce::Rectangle<int> (456, 668, 220, 44), juce::Justification::centred, 2);
}

//==============================================================================
//  Screen-space blood: drips, splatter, smears, rain, frame
//==============================================================================
void ShizukaEditor::drawBlood (juce::Graphics& g, float k, int mood)
{
    const float amount = smoothstep (0.35f, 1.0f, k);
    if (amount <= 0.0f)
        return;

    juce::Random dr (5);   // drips from the top edge
    const int drips = 30;
    for (int i = 0; i < drips; ++i)
    {
        const float x = W * ((float) i + 0.5f) / (float) drips + (dr.nextFloat() - 0.5f) * 18.0f;
        const float wv = 5.0f + dr.nextFloat() * 14.0f;
        const float len = H * 0.7f * amount * (0.15f + 0.85f * dr.nextFloat()) * (0.92f + 0.08f * std::sin (t * 1.5f + (float) i));
        g.setColour (bloodDk);
        g.fillRoundedRectangle (x - wv * 0.5f + 1.5f, -10.0f, wv, len + 10.0f, wv * 0.5f);
        g.setColour (blood);
        g.fillRoundedRectangle (x - wv * 0.5f, -10.0f, wv, len + 10.0f, wv * 0.5f);
        g.fillEllipse (x - wv * 0.85f, len - wv * 0.8f, wv * 1.7f, wv * 1.9f);
        g.setColour (bloodHi.withAlpha (0.7f));
        g.fillRoundedRectangle (x - wv * 0.25f, -10.0f, wv * 0.22f, len, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.fillEllipse (x - wv * 0.4f, len - wv * 0.4f, wv * 0.35f, wv * 0.3f);
    }

    Path band;   // wavy pooled band along the top
    band.startNewSubPath (0.0f, 0.0f);
    for (float x = 0.0f; x <= W; x += 10.0f)
        band.lineTo (x, (8.0f + 22.0f * amount) + 6.0f * std::sin (x * 0.05f + t));
    band.lineTo (W, 0.0f);
    band.closeSubPath();
    g.setColour (blood);
    g.fillPath (band);

    if (mood >= 3)   // smears
    {
        juce::Random sm (321);
        for (int i = 0; i < 5; ++i)
        {
            const float x = sm.nextFloat() * W, y = sm.nextFloat() * H;
            Path smear;
            smear.addRoundedRectangle (-60.0f, -9.0f, 120.0f + sm.nextFloat() * 80.0f, 18.0f, 9.0f);
            smear.applyTransform (juce::AffineTransform::rotation (-0.6f + sm.nextFloat() * 1.2f).translated (x, y));
            g.setColour (blood.withAlpha (0.45f * amount));
            g.fillPath (smear);
        }
    }

    const int splats = (int) (amount * 46.0f);   // splatters everywhere
    juce::Random sr (1234);
    for (int i = 0; i < splats; ++i)
    {
        const float x = sr.nextFloat() * W, y = sr.nextFloat() * H;
        const float r = 8.0f + sr.nextFloat() * 40.0f;
        const float drip = sr.nextFloat() < 0.4f ? amount * (20.0f + 60.0f * sr.nextFloat()) : 0.0f;
        paintSplat (g, x, y, r, 500 + i, 0.93f, drip);
    }

    if (mood >= 3)   // blood rain
    {
        juce::Random rr (99);
        const int drops = mood == 4 ? 90 : 35;
        for (int i = 0; i < drops; ++i)
        {
            const float x = rr.nextFloat() * W;
            const float speed = 280.0f + rr.nextFloat() * 420.0f;
            const float y = std::fmod (t * speed + rr.nextFloat() * H, H + 60.0f) - 30.0f;
            const float s = 2.0f + rr.nextFloat() * 3.5f;
            g.setColour (bloodHi.withAlpha (0.85f));
            g.fillRoundedRectangle (x - s * 0.5f, y - s * 5.0f, s, s * 6.0f, s * 0.5f);
        }
    }

    if (mood == 4)
    {
        // dripping blood frame around the whole window
        Path frame;
        frame.addRectangle (0.0f, 0.0f, W, H);
        Path inner;
        inner.startNewSubPath (26.0f, 30.0f);
        for (float x = 26.0f; x <= W - 26.0f; x += 12.0f)
            inner.lineTo (x, 30.0f + 10.0f * std::abs (std::sin (x * 0.09f + t)));
        for (float y = 30.0f; y <= H - 26.0f; y += 12.0f)
            inner.lineTo (W - 22.0f - 8.0f * std::abs (std::sin (y * 0.08f - t)), y);
        for (float x = W - 26.0f; x >= 26.0f; x -= 12.0f)
            inner.lineTo (x, H - 22.0f - 8.0f * std::abs (std::sin (x * 0.07f + t)));
        for (float y = H - 26.0f; y >= 30.0f; y -= 12.0f)
            inner.lineTo (22.0f + 8.0f * std::abs (std::sin (y * 0.1f - t)), y);
        inner.closeSubPath();
        frame.addPath (inner);
        frame.setUsingNonZeroWinding (false);
        g.setColour (blood.withAlpha (0.9f));
        g.fillPath (frame);

        g.setColour (Colour (0xffff0020).withAlpha (0.08f + 0.08f * std::sin (t * 10.0f)));
        g.fillAll();
    }

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, W * 0.33f, H * 0.5f,
                                             Colour (0xff1a0004).withAlpha (0.7f * amount), 0.0f, 0.0f, true));
    g.fillAll();
}
