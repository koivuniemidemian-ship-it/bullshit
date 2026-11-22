#include "PluginEditor.h"
#include "Girl.h"
#include "AnimeBlood.h"
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
        AnimeBlood::splat (g, x, y, r, seed, dripLen, alpha);
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
    const float now = (float) (juce::Time::getMillisecondCounterHiRes() * 0.001);
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 20.0f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);
    const float rage = smoothstep (0.3f, 0.9f, pos);
    auto polar = [&] (float a, float rad) { return juce::Point<float> (c.x + std::sin (a) * rad, c.y - std::cos (a) * rad); };

    auto sparkle = [&] (float sx, float sy, float size, Colour col)
    {
        Path st;
        st.addStar ({ sx, sy }, 4, size * 0.18f, size, 0.0f);
        g.setColour (col.withAlpha (0.35f));
        g.fillEllipse (sx - size * 0.6f, sy - size * 0.6f, size * 1.2f, size * 1.2f);
        g.setColour (col);
        g.fillPath (st);
        g.setColour (juce::Colours::white);
        g.fillEllipse (sx - size * 0.15f, sy - size * 0.15f, size * 0.3f, size * 0.3f);
    };

    // scale numbers
    g.setFont (fontOf (11.0f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startA + (endA - startA) * (float) i / 10.0f;
        const auto p = polar (a, r + 13.0f);
        g.setColour (juce::Colours::white.withAlpha (i <= (int) std::round (pos * 10.0f) ? 0.95f : 0.4f));
        g.drawText (juce::String (i), juce::Rectangle<float> (p.x - 10.0f, p.y - 7.0f, 20.0f, 14.0f), juce::Justification::centred);
    }

    // glow + shadow
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.08f * (float) i));
        const float sr = r + (float) i * 2.5f;
        g.fillEllipse (c.x - sr, c.y - sr + 6.0f, sr * 2.0f, sr * 2.0f);
    }
    const auto glowCol = mix (Colour (0xffb89cff), Colour (0xffff2040), rage);
    for (int i = 3; i >= 1; --i)
    {
        const float gr = r + (float) i * 5.0f;
        g.setColour (glowCol.withAlpha (0.06f + 0.04f * std::sin (now * 2.0f)));
        g.fillEllipse (c.x - gr, c.y - gr, gr * 2.0f, gr * 2.0f);
    }

    // holographic rainbow ring (slowly shifting hues)
    const int segs = 96;
    for (int i = 0; i < segs; ++i)
    {
        const float a0 = 2.0f * pi * (float) i / (float) segs, a1 = 2.0f * pi * (float) (i + 1) / (float) segs;
        const float hue = std::fmod ((float) i / (float) segs + now * 0.05f, 1.0f);
        const auto col = mix (Colour::fromHSV (hue, 0.35f, 1.0f, 1.0f), Colour::fromHSV (std::fmod (hue * 0.1f, 0.06f), 0.85f, 0.95f, 1.0f), rage);
        Path seg;
        seg.addCentredArc (c.x, c.y, r * 0.93f, r * 0.93f, 0.0f, a0, a1 + 0.01f, true);
        g.setColour (col);
        g.strokePath (seg, juce::PathStrokeType (r * 0.14f));
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.5f);
    g.drawEllipse (c.x - r * 0.86f, c.y - r * 0.86f, r * 1.72f, r * 1.72f, 1.2f);

    // value arc: hot pink -> blood red, with a glowing head
    {
        Path val;
        val.addCentredArc (c.x, c.y, r * 0.93f, r * 0.93f, 0.0f, startA, angle, true);
        g.setColour (mix (Colour (0xffff4fb0), AnimeBlood::red, rage).withAlpha (0.85f));
        g.strokePath (val, juce::PathStrokeType (r * 0.07f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const auto head = polar (angle, r * 0.93f);
        sparkle (head.x, head.y, 9.0f, juce::Colours::white);
    }

    // knob body: pearlescent with glitter flakes
    const float bodyR = r * 0.78f;
    juce::ColourGradient pearl (mix (Colour (0xfffdf6ff), Colour (0xffffd0d0), rage), c.x - bodyR * 0.4f, c.y - bodyR * 0.5f,
                                mix (Colour (0xffa9a2d8), Colour (0xff5a0a18), rage), c.x + bodyR, c.y + bodyR, true);
    pearl.addColour (0.5, mix (Colour (0xffe0d4ff), Colour (0xffd06070), rage));
    g.setGradientFill (pearl);
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    juce::Random fl (2024);
    for (int i = 0; i < 140; ++i)
    {
        const float a = fl.nextFloat() * 2.0f * pi, d = std::sqrt (fl.nextFloat()) * bodyR * 0.95f;
        const float px = c.x + std::cos (a) * d, py = c.y + std::sin (a) * d;
        const float tw = std::pow (0.5f + 0.5f * std::sin (now * (1.5f + 3.0f * fl.nextFloat()) + fl.nextFloat() * 6.28f), 6.0f);
        const auto col = mix (Colour::fromHSV (fl.nextFloat(), 0.5f, 1.0f, 1.0f), Colour (0xffff6070), rage);
        g.setColour (col.withAlpha (0.35f + 0.65f * tw));
        const float s = 1.0f + 1.6f * fl.nextFloat();
        g.fillRect (px - s * 0.5f, py - s * 0.5f, s, s);
        if (tw > 0.85f)
            sparkle (px, py, 4.5f, juce::Colours::white);
    }

    // knurled rim that turns with the knob
    for (int i = 0; i < 48; ++i)
    {
        const float a = angle + 2.0f * pi * (float) i / 48.0f;
        const auto p1 = polar (a, bodyR * 0.9f), p2 = polar (a, bodyR);
        g.setColour ((i % 2 == 0) ? juce::Colours::white.withAlpha (0.6f) : Colour (0xff4a3a7a).withAlpha (0.5f));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.5f);
    }
    g.setColour (Colour (0xff2a1d3a));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 2.0f);

    // faceted crystal cap
    const float capR = r * 0.42f;
    const int facets = 8;
    const auto gemA = mix (Colour (0xff9fd8ff), Colour (0xffff3048), rage);
    const auto gemB = mix (Colour (0xffc7a6ff), Colour (0xff7a0012), rage);
    for (int i = 0; i < facets; ++i)
    {
        const float a0 = angle + 2.0f * pi * (float) i / (float) facets, a1 = angle + 2.0f * pi * (float) (i + 1) / (float) facets;
        Path tri;
        tri.addTriangle (c.x, c.y, c.x + std::sin (a0) * capR, c.y - std::cos (a0) * capR, c.x + std::sin (a1) * capR, c.y - std::cos (a1) * capR);
        const float light = 0.5f + 0.5f * std::cos (a0 + 0.8f);
        g.setColour (mix (gemB, gemA, light).brighter (0.15f * light));
        g.fillPath (tri);
    }
    Path table;
    for (int i = 0; i < facets; ++i)
    {
        const float a = angle + 2.0f * pi * (float) i / (float) facets;
        const float px = c.x + std::sin (a) * capR * 0.5f, py = c.y - std::cos (a) * capR * 0.5f;
        if (i == 0) table.startNewSubPath (px, py); else table.lineTo (px, py);
    }
    table.closeSubPath();
    g.setColour (gemA.brighter (0.3f).withAlpha (0.85f));
    g.fillPath (table);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.strokePath (table, juce::PathStrokeType (1.0f));
    for (int i = 0; i < facets; ++i)
    {
        const float a = angle + 2.0f * pi * (float) i / (float) facets;
        g.drawLine (c.x + std::sin (a) * capR * 0.5f, c.y - std::cos (a) * capR * 0.5f, c.x + std::sin (a) * capR, c.y - std::cos (a) * capR, 0.8f);
    }
    g.setColour (Colour (0xff2a1d3a));
    g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.8f);
    // silver crescent inlay
    const float mr = capR * 0.32f;
    g.setColour (juce::Colours::white);
    g.fillEllipse (c.x - mr, c.y - mr, mr * 2.0f, mr * 2.0f);
    g.setColour (gemA.brighter (0.3f));
    g.fillEllipse (c.x - mr * 0.45f, c.y - mr * 1.15f, mr * 1.8f, mr * 1.8f);

    // pointer gem
    const auto tip = polar (angle, bodyR * 0.72f);
    g.setColour (Colour (0xff2a1d3a));
    g.fillEllipse (tip.x - 6.0f, tip.y - 6.0f, 12.0f, 12.0f);
    g.setColour (mix (Colour (0xffff4fb0), AnimeBlood::bright, rage));
    g.fillEllipse (tip.x - 4.5f, tip.y - 4.5f, 9.0f, 9.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (tip.x - 2.5f, tip.y - 3.0f, 3.0f, 3.0f);

    // specular glare
    Path glare;
    glare.addCentredArc (c.x, c.y, bodyR * 0.82f, bodyR * 0.82f, 0.0f, -1.9f, -0.6f, true);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.strokePath (glare, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // twinkling star flares around the rim
    for (int i = 0; i < 6; ++i)
    {
        const float ph = now * 1.3f + (float) i * 1.7f;
        const float tw = std::pow (0.5f + 0.5f * std::sin (ph), 4.0f);
        if (tw < 0.05f) continue;
        const auto p = polar ((float) i * 1.05f + 0.3f, r * (0.98f + 0.08f * std::sin ((float) i)));
        sparkle (p.x, p.y, 4.0f + 9.0f * tw, mix (Colour (0xfffff2b0), Colour (0xffff8080), rage).withAlpha (tw));
    }

    // the knob bleeds too
    if (pos > 0.55f)
    {
        const float a = (pos - 0.55f) / 0.45f;
        AnimeBlood::splat (g, c.x + bodyR * 0.5f, c.y - bodyR * 0.45f, 5.0f + 7.0f * a, 31);
        juce::Random dr (17);
        for (int i = 0; i < 4; ++i)
        {
            const float ang = pi * (0.65f + 0.7f * dr.nextFloat());
            const auto p = polar (ang, r * 0.97f);
            AnimeBlood::drip (g, p.x, p.y - 2.0f, a * (8.0f + 22.0f * dr.nextFloat()), 5.0f);
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
            AnimeBlood::drip (g, x, y - 6.0f, len + 6.0f, wv);
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
        AnimeBlood::drip (g, x, -10.0f, len + 10.0f, wv);
    }

    Path band;   // wavy pooled band along the top
    band.startNewSubPath (0.0f, 0.0f);
    for (float x = 0.0f; x <= W; x += 10.0f)
        band.lineTo (x, (8.0f + 22.0f * amount) + 6.0f * std::sin (x * 0.05f + t));
    band.lineTo (W, 0.0f);
    band.closeSubPath();
    g.setColour (AnimeBlood::red);
    g.fillPath (band);
    g.setColour (AnimeBlood::dark);
    g.strokePath (band, juce::PathStrokeType (3.0f));

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
            g.setColour (AnimeBlood::red);
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
        g.setColour (AnimeBlood::red.withAlpha (0.92f));
        g.fillPath (frame);
        g.setColour (AnimeBlood::dark);
        g.strokePath (inner, juce::PathStrokeType (3.0f));

        g.setColour (Colour (0xffff0020).withAlpha (0.08f + 0.08f * std::sin (t * 10.0f)));
        g.fillAll();
    }

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, W * 0.33f, H * 0.5f,
                                             Colour (0xff1a0004).withAlpha (0.7f * amount), 0.0f, 0.0f, true));
    g.fillAll();
}
