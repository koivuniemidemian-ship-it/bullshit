#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour ink     { 0xff3a2440 };
    const juce::Colour skin    { 0xffffe6d8 };
    const juce::Colour hairC   { 0xff3b2a6b };
    const juce::Colour hairHi  { 0xff6a56a8 };
    const juce::Colour sweater { 0xff3e7c86 };
    const juce::Colour hotPink { 0xffff4fa3 };
    const juce::Colour cream   { 0xfffff4f8 };
    const juce::Colour lilac   { 0xffe2ccff };

    const char* taglines[] = {
        "this is a library.",
        "your noise floor has been reported",
        "please lower your frequencies",
        "quiet hours are 24/7",
        "she has heard enough",
        "late fees apply to reverb tails",
        "no talking. no hiss. no hum.",
        "your snare is overdue"
    };
    constexpr int numTaglines = (int) (sizeof (taglines) / sizeof (taglines[0]));

    juce::Font fontOf (float size, int style = juce::Font::bold)
    {
        return juce::Font (juce::FontOptions (size, style));
    }

    void drawDrop (juce::Graphics& g, float x, float y, float s)
    {
        juce::Path d;
        d.startNewSubPath (x, y - 1.8f * s);
        d.cubicTo (x + 0.4f * s, y - 1.0f * s, x + s, y - 0.3f * s, x + s, y + 0.2f * s);
        d.cubicTo (x + s, y + 0.8f * s, x + 0.5f * s, y + s, x, y + s);
        d.cubicTo (x - 0.5f * s, y + s, x - s, y + 0.8f * s, x - s, y + 0.2f * s);
        d.cubicTo (x - s, y - 0.3f * s, x - 0.4f * s, y - 1.0f * s, x, y - 1.8f * s);
        d.closeSubPath();
        g.setColour (juce::Colour (0xffa8dcff));
        g.fillPath (d);
        g.setColour (ink);
        g.strokePath (d, juce::PathStrokeType (2.0f));
    }

    juce::Path heart (float cx, float cy, float s)
    {
        juce::Path h;
        h.startNewSubPath (cx, cy + s * 0.9f);
        h.cubicTo (cx - s * 1.2f, cy + s * 0.1f, cx - s * 0.9f, cy - s * 0.9f, cx, cy - s * 0.35f);
        h.cubicTo (cx + s * 0.9f, cy - s * 0.9f, cx + s * 1.2f, cy + s * 0.1f, cx, cy + s * 0.9f);
        h.closeSubPath();
        return h;
    }
}

//==============================================================================
KawaiiLookAndFeel::KawaiiLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, cream.withAlpha (0.7f));
    setColour (juce::Slider::textBoxHighlightColourId, hotPink.withAlpha (0.4f));
    setColour (juce::Label::textColourId, ink);
}

void KawaiiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);
    const auto accent = hotPink.interpolatedWith (juce::Colour (0xffe0102a), juce::jlimit (0.0f, 1.0f, (pos - 0.6f) * 2.5f));

    g.setColour (ink.withAlpha (0.25f));
    g.fillEllipse (c.x - r + 3.0f, c.y - r + 5.0f, r * 2.0f, r * 2.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x, c.y - r, lilac, c.x, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (ink);
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 3.0f);

    // tick marks
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startA + (endA - startA) * (float) i / 10.0f;
        const float r1 = r * 0.78f, r2 = r * (i % 5 == 0 ? 0.62f : 0.70f);
        g.setColour (ink.withAlpha (0.5f));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                    c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, 2.0f);
    }

    juce::Path track, value;
    track.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.0f, startA, endA, true);
    value.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.0f, startA, angle, true);
    const juce::PathStrokeType stroke (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (ink.withAlpha (0.15f));
    g.strokePath (track, stroke);
    g.setColour (accent);
    g.strokePath (value, stroke);

    // cap with a heart
    const float cr = r * 0.42f;
    g.setColour (cream);
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.setColour (ink);
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 2.5f);
    g.setColour (accent);
    g.fillPath (heart (c.x, c.y, cr * 0.5f));

    // pointer dot
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

    knobLabel.setText ("SHHH-O-METER", juce::dontSendNotification);
    knobLabel.setJustificationType (juce::Justification::centred);
    knobLabel.setFont (fontOf (17.0f));
    addAndMakeVisible (knobLabel);

    for (auto& b : bandsUI)
        b = -100.0f;

    setSize (560, 670);
    startTimerHz (30);
}

ShizukaEditor::~ShizukaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ShizukaEditor::resized()
{
    stage = juce::Rectangle<float> (0.0f, 0.0f, (float) getWidth(), 470.0f);
    knobLabel.setBounds (0, 474, getWidth(), 22);
    knob.setBounds (getWidth() / 2 - 85, 496, 170, 140);
}

float ShizukaEditor::hush() const
{
    if (auto* param = proc.apvts.getParameter ("hush"))
        return param->getValue();
    return 0.0f;
}

int ShizukaEditor::moodFor (float k) const
{
    if (k < 0.001f) return 0;   // dozing
    if (k < 0.30f)  return 1;   // polite
    if (k < 0.60f)  return 2;   // annoyed
    if (k < 0.85f)  return 3;   // angry
    return 4;                   // final form
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
    const float k = hush();
    const int mood = moodFor (k);

    drawBackground (g, k);

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (stage.toNearestInt());

        drawSpectrum (g, k);

        if (mood == 4)   // screen shake
            g.addTransform (juce::AffineTransform::translation ((rng.nextFloat() - 0.5f) * 8.0f,
                                                                (rng.nextFloat() - 0.5f) * 8.0f));
        drawEffects (g, k, mood);
        drawGirl (g, mood);
        drawBubble (g, mood);
    }

    g.setColour (ink.withAlpha (0.85f));
    g.setFont (fontOf (15.0f, juce::Font::italic));
    g.drawText (taglines[tagline], getLocalBounds().removeFromBottom (32), juce::Justification::centred);
}

void ShizukaEditor::drawBackground (juce::Graphics& g, float k)
{
    const float rage = juce::jlimit (0.0f, 1.0f, (k - 0.55f) / 0.45f);
    const auto top = juce::Colour (0xffffd6e8).interpolatedWith (juce::Colour (0xff2a0010), rage);
    const auto bot = juce::Colour (0xffd9c8ff).interpolatedWith (juce::Colour (0xff7a0020), rage);
    g.setGradientFill (juce::ColourGradient (top, 0.0f, 0.0f, bot, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // sparkles fade out as she gets angrier
    juce::Random sp (99);
    for (int i = 0; i < 26; ++i)
    {
        const float x = sp.nextFloat() * (float) getWidth();
        const float y = sp.nextFloat() * (float) getHeight();
        const float s = 4.0f + sp.nextFloat() * 6.0f;
        const float twinkle = 0.5f + 0.5f * std::sin (t * 2.0f + (float) i);
        juce::Path star;
        star.addStar ({ x, y }, 4, s * 0.3f, s, t * 0.3f);
        g.setColour (juce::Colours::white.withAlpha ((1.0f - rage) * 0.7f * twinkle));
        g.fillPath (star);
    }

    // knob area panel
    g.setColour (cream.withAlpha (0.35f));
    g.fillRoundedRectangle (150.0f, 472.0f, (float) getWidth() - 300.0f, 168.0f, 24.0f);
}

void ShizukaEditor::drawSpectrum (juce::Graphics& g, float k)
{
    const auto area = stage.reduced (16.0f, 0.0f).withTrimmedTop (150.0f).withTrimmedBottom (6.0f);
    const float bw = area.getWidth() / (float) ShizukaProcessor::numBands;
    const float thresh = ShizukaProcessor::thresholdDbFor (k);
    auto yFor = [&] (float db) { return area.getBottom() - area.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 100.0f) / 100.0f); };

    for (int i = 0; i < ShizukaProcessor::numBands; ++i)
    {
        const float db = bandsUI[(size_t) i];
        const float y = yFor (db);
        const bool silenced = k > 0.001f && db < thresh;
        g.setColour (silenced ? ink.withAlpha (0.12f) : juce::Colour (0xff7fe7ff).interpolatedWith (hotPink, (float) i / 48.0f).withAlpha (0.45f));
        g.fillRoundedRectangle (area.getX() + (float) i * bw + 1.0f, y, bw - 2.0f, area.getBottom() - y, 2.0f);
    }

    if (k > 0.001f)
    {
        const float ty = yFor (thresh);
        const float dashes[] = { 10.0f, 6.0f };
        g.setColour (hotPink.withAlpha (0.9f));
        g.drawDashedLine ({ area.getX(), ty, area.getRight(), ty }, dashes, 2, 3.0f);
        g.setFont (fontOf (13.0f));
        g.drawText ("QUIET LINE", juce::Rectangle<float> (area.getRight() - 100.0f, ty - 18.0f, 100.0f, 16.0f), juce::Justification::centredRight);
    }
}

void ShizukaEditor::drawEffects (juce::Graphics& g, float k, int mood)
{
    const float cx = stage.getCentreX(), cy = 215.0f;

    if (mood >= 3)   // speed lines
    {
        const float a = juce::jlimit (0.0f, 1.0f, (k - 0.6f) / 0.3f);
        juce::Random lr ((juce::int64) (t * 10.0f));
        for (int i = 0; i < 40; ++i)
        {
            const float ang = juce::MathConstants<float>::twoPi * (float) i / 40.0f + lr.nextFloat() * 0.1f;
            const float r1 = 190.0f + lr.nextFloat() * 40.0f, r2 = 420.0f;
            g.setColour (juce::Colours::white.withAlpha (0.35f * a));
            g.drawLine (cx + std::cos (ang) * r1, cy + std::sin (ang) * r1, cx + std::cos (ang) * r2, cy + std::sin (ang) * r2, 2.0f + lr.nextFloat() * 3.0f);
        }
    }

    if (mood == 4)   // rage aura
    {
        juce::Path aura;
        const int N = 90;
        for (int i = 0; i <= N; ++i)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) i / (float) N;
            const float r = 165.0f * (1.0f + 0.12f * std::abs (std::sin (7.0f * th + t * 6.0f)) + 0.05f * std::sin (13.0f * th - t * 9.0f));
            const float px = cx + std::cos (th) * r * 1.05f, py = cy + 30.0f + std::sin (th) * r * 1.1f;
            if (i == 0) aura.startNewSubPath (px, py); else aura.lineTo (px, py);
        }
        aura.closeSubPath();
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (juce::Colour (0xffff3a2a).withAlpha (0.12f * (float) i));
            g.strokePath (aura, juce::PathStrokeType ((float) i * 7.0f));
        }
    }
}

void ShizukaEditor::drawGirl (juce::Graphics& g, int mood)
{
    const float cx = stage.getCentreX(), cy = 215.0f, R = 105.0f;
    const float bottom = stage.getBottom();
    const float pulse = 1.0f + 0.25f * gatedUI;

    // --- back hair + bun + pencil
    g.setColour (hairC);
    g.fillRoundedRectangle (cx - 1.02f * R, cy - 1.0f * R, 2.04f * R, 2.15f * R, 0.9f * R);
    g.fillEllipse (cx - 0.35f * R, cy - 1.38f * R, 0.8f * R, 0.68f * R);

    juce::Path pencil;
    pencil.addRoundedRectangle (-0.75f * R, -0.06f * R, 1.5f * R, 0.12f * R, 0.03f * R);
    pencil.applyTransform (juce::AffineTransform::rotation (-0.45f).translated (cx + 0.05f * R, cy - 1.04f * R));
    g.setColour (juce::Colour (0xffffd04a));
    g.fillPath (pencil);
    g.setColour (ink);
    g.strokePath (pencil, juce::PathStrokeType (2.0f));

    // --- neck, sweater, turtleneck
    g.setColour (skin.darker (0.08f));
    g.fillRect (cx - 0.2f * R, cy + 0.6f * R, 0.4f * R, 0.35f * R);

    juce::Path body;
    body.startNewSubPath (cx - 0.3f * R, cy + 1.05f * R);
    body.cubicTo (cx - 0.9f * R, cy + 1.15f * R, cx - 1.35f * R, cy + 1.3f * R, cx - 1.5f * R, cy + 1.9f * R);
    body.lineTo (cx - 1.55f * R, bottom + 10.0f);
    body.lineTo (cx + 1.55f * R, bottom + 10.0f);
    body.lineTo (cx + 1.5f * R, cy + 1.9f * R);
    body.cubicTo (cx + 1.35f * R, cy + 1.3f * R, cx + 0.9f * R, cy + 1.15f * R, cx + 0.3f * R, cy + 1.05f * R);
    body.closeSubPath();
    g.setColour (sweater);
    g.fillPath (body);
    g.setColour (ink);
    g.strokePath (body, juce::PathStrokeType (3.0f));

    const juce::Rectangle<float> collar (cx - 0.4f * R, cy + 0.78f * R, 0.8f * R, 0.42f * R);
    g.setColour (sweater.darker (0.2f));
    g.fillRoundedRectangle (collar, 0.16f * R);
    g.setColour (ink);
    g.drawRoundedRectangle (collar, 0.16f * R, 2.5f);
    g.setColour (ink.withAlpha (0.3f));
    for (int i = 1; i < 6; ++i)
    {
        const float x = collar.getX() + collar.getWidth() * (float) i / 6.0f;
        g.drawLine (x, collar.getY() + 6.0f, x, collar.getBottom() - 6.0f, 1.5f);
    }

    // name badge
    const juce::Rectangle<float> badge (cx + 0.55f * R, cy + 1.5f * R, 0.8f * R, 0.3f * R);
    g.setColour (cream);
    g.fillRoundedRectangle (badge, 4.0f);
    g.setColour (ink);
    g.drawRoundedRectangle (badge, 4.0f, 2.0f);
    g.setFont (fontOf (11.0f));
    g.drawText ("LIBRARIAN", badge, juce::Justification::centred);

    // --- face
    juce::Path face;
    face.startNewSubPath (cx - 0.8f * R, cy - 0.3f * R);
    face.cubicTo (cx - 0.8f * R, cy + 0.35f * R, cx - 0.35f * R, cy + 0.85f * R, cx, cy + 0.95f * R);
    face.cubicTo (cx + 0.35f * R, cy + 0.85f * R, cx + 0.8f * R, cy + 0.35f * R, cx + 0.8f * R, cy - 0.3f * R);
    face.cubicTo (cx + 0.8f * R, cy - 1.0f * R, cx - 0.8f * R, cy - 1.0f * R, cx - 0.8f * R, cy - 0.3f * R);
    face.closeSubPath();
    g.setColour (skin);
    g.fillPath (face);
    g.setColour (ink);
    g.strokePath (face, juce::PathStrokeType (2.5f));

    // blush / anger flush
    if (mood >= 3)
    {
        g.setColour (juce::Colour (0xffff3030).withAlpha (mood == 4 ? 0.35f : 0.22f));
        g.fillEllipse (cx - 0.7f * R, cy + 0.25f * R, 1.4f * R, 0.3f * R);
    }
    else
    {
        g.setColour (hotPink.withAlpha (0.45f));
        for (int s = -1; s <= 1; s += 2)
            for (int i = 0; i < 3; ++i)
            {
                const float bx = cx + (float) s * 0.5f * R + (float) i * 0.07f * R - 0.07f * R;
                g.drawLine (bx, cy + 0.42f * R, bx - 0.04f * R, cy + 0.5f * R, 2.0f);
            }
    }

    // --- bangs
    juce::Path bangs;
    bangs.startNewSubPath (cx - 0.92f * R, cy + 0.05f * R);
    bangs.cubicTo (cx - 1.0f * R, cy - 1.15f * R, cx + 1.0f * R, cy - 1.15f * R, cx + 0.92f * R, cy + 0.05f * R);
    const int spikes = 7;
    for (int i = 0; i <= spikes * 2; ++i)
    {
        const float x = cx + 0.85f * R - 1.7f * R * (float) i / (float) (spikes * 2);
        const float y = (i % 2 == 0) ? cy - 0.32f * R : cy - 0.08f * R;
        bangs.lineTo (x, y);
    }
    bangs.closeSubPath();
    g.setColour (hairC);
    g.fillPath (bangs);

    // side locks
    for (int s = -1; s <= 1; s += 2)
    {
        juce::Path lock;
        const float x0 = cx + (float) s * 0.88f * R;
        lock.startNewSubPath (x0, cy - 0.4f * R);
        lock.quadraticTo (x0 + (float) s * 0.08f * R, cy + 0.4f * R, x0 - (float) s * 0.05f * R, cy + 1.0f * R);
        lock.lineTo (x0 - (float) s * 0.2f * R, cy + 0.6f * R);
        lock.quadraticTo (x0 - (float) s * 0.18f * R, cy, x0 - (float) s * 0.22f * R, cy - 0.4f * R);
        lock.closeSubPath();
        g.fillPath (lock);
    }

    juce::Path shine;
    shine.addCentredArc (cx - 0.1f * R, cy - 0.45f * R, 0.55f * R, 0.3f * R, 0.0f, -1.3f, -0.4f, true);
    g.setColour (hairHi.withAlpha (0.8f));
    g.strokePath (shine, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // --- eyes
    const float eyeY = cy + 0.12f * R;
    const float rage = mood == 4 ? 1.0f : (mood == 3 ? 0.6f : 0.0f);
    const float lidDrop[] = { 0.0f, 0.0f, 0.38f, 0.1f, 0.0f };
    const float browIn[]  = { 0.0f, -0.03f, 0.06f, 0.12f, 0.17f };

    for (int s = -1; s <= 1; s += 2)
    {
        const float ex = cx + (float) s * 0.36f * R;
        const float ew = 0.3f * R, eh = 0.34f * R;
        const float innerSign = (float) -s;   // direction toward the nose

        if (mood == 0)
        {
            juce::Path closed;   // sleepy closed eyes
            closed.addCentredArc (ex, eyeY - 0.02f * R, ew * 0.45f, eh * 0.25f, 0.0f,
                                  juce::MathConstants<float>::halfPi + 0.9f, juce::MathConstants<float>::halfPi * 3.0f - 0.9f, true);
            g.setColour (ink);
            g.strokePath (closed, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            juce::Path eye;
            eye.addEllipse (ex - ew * 0.5f, eyeY - eh * 0.5f, ew, eh);
            g.setColour (juce::Colours::white);
            g.fillPath (eye);

            {
                juce::Graphics::ScopedSaveState clip (g);
                g.reduceClipRegion (eye);

                const auto irisTop = juce::Colour (0xff6b4bd6).interpolatedWith (juce::Colour (0xffd0102a), rage);
                const auto irisBot = juce::Colour (0xffff8fc8).interpolatedWith (juce::Colour (0xffff8a3a), rage);
                const float iw = ew * 0.66f, ih = eh * 0.92f;
                const float ix = ex + innerSign * 0.02f * R, iy = eyeY + eh * 0.05f;
                g.setGradientFill (juce::ColourGradient (irisTop, ix, iy - ih * 0.5f, irisBot, ix, iy + ih * 0.5f, false));
                g.fillEllipse (ix - iw * 0.5f, iy - ih * 0.5f, iw, ih);

                const float pw = iw * (0.45f - 0.28f * rage), ph = ih * (0.5f - 0.3f * rage);
                g.setColour (ink);
                g.fillEllipse (ix - pw * 0.5f, iy - ph * 0.5f, pw, ph);

                g.setColour (juce::Colours::white);
                g.fillEllipse (ix - iw * 0.35f, iy - ih * 0.35f, iw * 0.32f, iw * 0.32f);
                g.fillEllipse (ix + iw * 0.08f, iy + ih * 0.12f, iw * 0.14f, iw * 0.14f);

                if (lidDrop[mood] > 0.0f)   // unimpressed half-lid
                {
                    g.setColour (skin);
                    g.fillRect (ex - ew, eyeY - eh, ew * 2.0f, eh * (0.5f + lidDrop[mood]));
                }
            }

            const float lidY = eyeY - eh * 0.5f + eh * lidDrop[mood];
            juce::Path lash;
            lash.startNewSubPath (ex - innerSign * ew * 0.62f, lidY + eh * 0.1f);
            lash.quadraticTo (ex, lidY - eh * 0.18f, ex + innerSign * ew * 0.6f, lidY + eh * 0.12f + browIn[mood] * R * 0.3f);
            g.setColour (ink);
            g.strokePath (lash, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawLine (ex - ew * 0.3f, eyeY + eh * 0.52f, ex + ew * 0.3f, eyeY + eh * 0.52f, 1.5f);
        }

        // glasses
        const float gr = 0.23f * R;
        g.setColour (ink);
        g.drawEllipse (ex - gr, eyeY - gr, gr * 2.0f, gr * 2.0f, 2.5f);
        if (mood >= 3)
        {
            g.setColour (juce::Colours::white.withAlpha (mood == 4 ? 0.95f : 0.5f));
            if (mood == 4)
                g.fillEllipse (ex - gr, eyeY - gr, gr * 2.0f, gr * 2.0f);   // the classic opaque glare
            g.setColour (juce::Colours::white);
            g.drawLine (ex - gr * 0.5f, eyeY + gr * 0.2f, ex + gr * 0.1f, eyeY - gr * 0.5f, 3.0f);
            g.drawLine (ex - gr * 0.1f, eyeY + gr * 0.5f, ex + gr * 0.5f, eyeY - gr * 0.2f, 2.0f);
        }

        // brows (drawn over the bangs, anime style)
        const float by = cy - 0.2f * R;
        juce::Path brow;
        brow.startNewSubPath (ex - innerSign * 0.17f * R, by);
        brow.quadraticTo (ex, by - 0.06f * R, ex + innerSign * 0.15f * R, by + browIn[mood] * R);
        g.setColour (ink);
        g.strokePath (brow, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (ink);
    g.drawLine (cx - 0.13f * R, eyeY - 0.02f * R, cx + 0.13f * R, eyeY - 0.02f * R, 2.5f);   // glasses bridge

    // nose
    g.drawLine (cx + 0.02f * R, cy + 0.36f * R, cx - 0.02f * R, cy + 0.4f * R, 2.0f);

    // --- mouth
    const float my = cy + 0.58f * R;
    if (mood == 0)
    {
        g.fillEllipse (cx - 0.04f * R, my - 0.03f * R, 0.08f * R, 0.07f * R);
    }
    else if (mood == 1)
    {
        juce::Path m;   // little cat smile
        m.startNewSubPath (cx - 0.14f * R, my - 0.02f * R);
        m.quadraticTo (cx - 0.07f * R, my + 0.07f * R, cx, my);
        m.quadraticTo (cx + 0.07f * R, my + 0.07f * R, cx + 0.14f * R, my - 0.02f * R);
        g.strokePath (m, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else if (mood == 2)
    {
        juce::Path m;   // unimpressed
        m.startNewSubPath (cx - 0.13f * R, my + 0.02f * R);
        m.quadraticTo (cx, my - 0.03f * R, cx + 0.13f * R, my + 0.03f * R);
        g.strokePath (m, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        const float w = (mood == 4 ? 0.46f : 0.3f) * R * pulse;
        const float h = (mood == 4 ? 0.32f : 0.18f) * R * pulse;
        juce::Path m;
        m.startNewSubPath (cx - w * 0.5f, my - h * 0.2f);
        m.lineTo (cx + w * 0.5f, my - h * 0.2f);
        m.quadraticTo (cx + w * 0.3f, my + h, cx, my + h);
        m.quadraticTo (cx - w * 0.3f, my + h, cx - w * 0.5f, my - h * 0.2f);
        m.closeSubPath();
        g.setColour (juce::Colour (0xff5a0a1a));
        g.fillPath (m);
        {
            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (m);
            g.setColour (juce::Colour (0xffe8708a));
            g.fillEllipse (cx - w * 0.3f, my + h * 0.45f, w * 0.6f, h * 0.7f);
        }
        if (mood == 4)   // fangs
        {
            g.setColour (juce::Colours::white);
            for (int s = -1; s <= 1; s += 2)
            {
                juce::Path fang;
                const float fx = cx + (float) s * w * 0.3f;
                fang.addTriangle (fx - 0.04f * R, my - h * 0.2f, fx + 0.04f * R, my - h * 0.2f, fx, my + h * 0.25f);
                g.fillPath (fang);
            }
        }
        g.setColour (ink);
        g.strokePath (m, juce::PathStrokeType (2.5f));
    }

    // --- extras
    if (mood == 2)
        drawDrop (g, cx + 0.98f * R, cy - 0.35f * R, 0.12f * R);

    if (mood >= 3)   // anger vein
    {
        const juce::Point<float> v (cx + 0.5f * R, cy - 0.62f * R);
        const float vs = 0.1f * R * (mood == 4 ? 1.0f + 0.15f * std::sin (t * 12.0f) : 1.0f);
        g.setColour (juce::Colour (0xffe0102a));
        for (int q = 0; q < 4; ++q)
        {
            const float a = juce::MathConstants<float>::halfPi * (float) q + 0.785f;
            const juce::Point<float> dir (std::cos (a), std::sin (a)), perp (-dir.y, dir.x);
            juce::Path arc;
            const auto p1 = v + dir * (vs * 0.5f) + perp * (vs * 0.6f);
            const auto p2 = v + dir * (vs * 0.5f) - perp * (vs * 0.6f);
            const auto ctrl = v + dir * (vs * 0.15f);
            arc.startNewSubPath (p1);
            arc.quadraticTo (ctrl, p2);
            g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    if (mood == 0)   // floating zzz
    {
        g.setColour (ink.withAlpha (0.7f));
        for (int i = 0; i < 3; ++i)
        {
            const float ph = std::fmod (t * 0.4f + (float) i / 3.0f, 1.0f);
            g.setFont (fontOf (14.0f + ph * 14.0f));
            g.drawText ("z", juce::Rectangle<float> (cx + 0.9f * R + ph * 40.0f, cy - 0.9f * R - ph * 60.0f, 30.0f, 30.0f), juce::Justification::centred);
        }
    }
}

void ShizukaEditor::drawBubble (juce::Graphics& g, int mood)
{
    const char* lines[] = { "zzz...", "shh~", "Shhh!", "SHHHHH!!", "SILENCE." };
    const float sizes[] = { 18.0f, 24.0f, 28.0f, 32.0f, 38.0f };

    const juce::Rectangle<float> box (stage.getRight() - 215.0f, 22.0f, 195.0f, 78.0f);
    const auto c = box.getCentre();

    juce::Path bubble;
    if (mood == 4)
    {
        const int pts = 22;
        for (int i = 0; i <= pts * 2; ++i)
        {
            const float a = juce::MathConstants<float>::pi * (float) i / (float) pts;
            const float r = (i % 2 == 0) ? 1.0f : 0.78f;
            const float px = c.x + std::cos (a) * box.getWidth() * 0.55f * r;
            const float py = c.y + std::sin (a) * box.getHeight() * 0.62f * r;
            if (i == 0) bubble.startNewSubPath (px, py); else bubble.lineTo (px, py);
        }
        bubble.closeSubPath();
    }
    else
    {
        bubble.addEllipse (box);
        juce::Path tail;
        tail.addTriangle (box.getX() + 40.0f, box.getBottom() - 12.0f, box.getX() + 70.0f, box.getBottom() - 6.0f, box.getX() + 10.0f, box.getBottom() + 26.0f);
        bubble.addPath (tail);
    }

    g.setColour (mood == 4 ? juce::Colour (0xffe0102a) : juce::Colours::white);
    g.fillPath (bubble);
    g.setColour (ink);
    g.strokePath (bubble, juce::PathStrokeType (3.0f));

    g.setColour (mood == 4 ? juce::Colours::white : ink);
    g.setFont (fontOf (sizes[mood]));
    g.drawText (lines[mood], box, juce::Justification::centred);
}
