#include "PluginEditor.h"
#include "Girl.h"
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
    constexpr float CX = HushGirl::CX, HY = HushGirl::HY;
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

    const float mr = 150.0f;
    const float mx = CX, my = 640.0f - 420.0f * rise;

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
//  The character (see Girl.cpp)
//==============================================================================
void ShizukaEditor::drawGirl (juce::Graphics& g, int mood, float rage)
{
    HushGirl::draw (g, t, mood, rage, gatedUI);
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
        const float len = H * 0.42f * amount * (0.12f + 0.88f * dr.nextFloat()) * (0.92f + 0.08f * std::sin (t * 1.5f + (float) i));
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

    const int splats = (int) (amount * 34.0f);   // splatters everywhere (her face stays visible)
    juce::Random sr (1234);
    const juce::Rectangle<float> faceZone (CX - 120.0f, HY - 120.0f, 240.0f, 240.0f);
    for (int i = 0; i < splats; ++i)
    {
        const float x = sr.nextFloat() * W, y = sr.nextFloat() * H;
        if (faceZone.contains (x, y) || juce::Rectangle<float> (455.0f, 255.0f, 225.0f, 130.0f).contains (x, y))
            continue;
        const float r = 8.0f + sr.nextFloat() * 40.0f;
        const float drip = sr.nextFloat() < 0.4f ? amount * (20.0f + 60.0f * sr.nextFloat()) : 0.0f;
        paintSplat (g, x, y, r, 500 + i, 0.93f, drip);
    }

    if (mood >= 3)   // blood rain
    {
        juce::Random rr (99);
        const int drops = mood == 4 ? 70 : 30;
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
