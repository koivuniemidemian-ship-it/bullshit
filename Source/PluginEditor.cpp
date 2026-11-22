#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
    const juce::Colour slime  { 0xff9be22d };
    const juce::Colour bile   { 0xffd8e04a };
    const juce::Colour bruise { 0xff4a2a5a };
    const juce::Colour crustC { 0xff6b4a2b };
    const juce::Colour ink    { 0xff1d1424 };
    const juce::Colour flesh  { 0xffe9c79a };
    const juce::Colour sweat  { 0xff9fd8ff };
    const juce::Colour tongue { 0xffe8708a };

    struct KnobSpec { const char* id; const char* name; };
    const KnobSpec specs[] = {
        { "moisture", "MOISTURE" }, { "stank", "STANK" },  { "crust",  "CRUST"  },
        { "gunk",     "GUNK"     }, { "ooze",  "OOZE"  },  { "volume", "VOLUME" }
    };

    const char* taglines[] = {
        "do not lick the plugin",
        "certified 100% moist",
        "now with extra crust",
        "your mix deserves to be damp",
        "side effects may include stank",
        "producers hate this one weird blob",
        "please hydrate responsibly",
        "warning: contains ooze",
        "keep refrigerated after opening",
        "he is sweating because he believes in you"
    };
    constexpr int numTaglines = (int) (sizeof (taglines) / sizeof (taglines[0]));

    juce::Font chunky (float size, int style = juce::Font::bold)
    {
        return juce::Font (juce::FontOptions (size, style));
    }

    void drawDrop (juce::Graphics& g, float x, float y, float s, float alpha)
    {
        juce::Path d;
        d.startNewSubPath (x, y - 1.8f * s);
        d.cubicTo (x + 0.4f * s, y - 1.0f * s, x + s, y - 0.3f * s, x + s, y + 0.2f * s);
        d.cubicTo (x + s, y + 0.8f * s, x + 0.5f * s, y + s, x, y + s);
        d.cubicTo (x - 0.5f * s, y + s, x - s, y + 0.8f * s, x - s, y + 0.2f * s);
        d.cubicTo (x - s, y - 0.3f * s, x - 0.4f * s, y - 1.0f * s, x, y - 1.8f * s);
        d.closeSubPath();

        g.setColour (sweat.withAlpha (alpha));
        g.fillPath (d);
        g.setColour (ink.withAlpha (alpha * 0.8f));
        g.strokePath (d, juce::PathStrokeType (1.2f));
        g.setColour (juce::Colours::white.withAlpha (alpha * 0.8f));
        g.fillEllipse (x - 0.5f * s, y - 0.2f * s, 0.35f * s, 0.5f * s);
    }
}

//==============================================================================
GooLookAndFeel::GooLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, bile.withAlpha (0.45f));
    setColour (juce::Slider::textBoxHighlightColourId, slime);
    setColour (juce::Label::textColourId, ink);
}

void GooLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);

    // Drip hanging off the knob, longer the higher it's turned
    const float dripLen = r * (0.2f + 0.9f * pos);
    g.setColour (bruise.darker (0.2f));
    g.fillRoundedRectangle (c.x + r * 0.25f, c.y + r * 0.4f, r * 0.16f, dripLen, r * 0.08f);
    g.fillEllipse (c.x + r * 0.21f, c.y + r * 0.4f + dripLen - r * 0.12f, r * 0.24f, r * 0.24f);

    // Wobbly blob body
    juce::Path blob;
    const int N = 48;
    for (int i = 0; i <= N; ++i)
    {
        const float th = juce::MathConstants<float>::twoPi * (float) i / (float) N;
        const float rr = r * (0.80f + 0.05f * std::sin (5.0f * th + pos * 9.0f)
                                    + 0.05f * pos * std::sin (9.0f * th));
        const float px = c.x + std::sin (th) * rr, py = c.y - std::cos (th) * rr;
        if (i == 0) blob.startNewSubPath (px, py); else blob.lineTo (px, py);
    }
    blob.closeSubPath();

    g.setGradientFill (juce::ColourGradient (bruise.brighter (0.4f), c.x - r, c.y - r,
                                             bruise.darker (0.4f), c.x + r, c.y + r, false));
    g.fillPath (blob);
    g.setColour (ink);
    g.strokePath (blob, juce::PathStrokeType (2.5f));

    // Arc track + slimy value arc
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, r * 0.98f, r * 0.98f, 0.0f, startA, endA, true);
    value.addCentredArc (c.x, c.y, r * 0.98f, r * 0.98f, 0.0f, startA, angle, true);
    const juce::PathStrokeType arcStroke (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (ink.withAlpha (0.25f));
    g.strokePath (track, arcStroke);
    g.setColour (slime);
    g.strokePath (value, arcStroke);

    // Pointer
    const juce::Point<float> tip (c.x + std::sin (angle) * r * 0.6f, c.y - std::cos (angle) * r * 0.6f);
    g.setColour (bile);
    g.drawLine (c.x, c.y, tip.x, tip.y, 4.0f);
    g.fillEllipse (tip.x - r * 0.1f, tip.y - r * 0.1f, r * 0.2f, r * 0.2f);
    g.setColour (crustC);
    g.fillEllipse (c.x - r * 0.14f, c.y - r * 0.14f, r * 0.28f, r * 0.28f);
}

void GooLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
{
    auto b = button.getLocalBounds().toFloat().reduced (2.0f);
    const bool on = button.getToggleState();
    const float corner = b.getHeight() * 0.5f;

    g.setColour (on ? slime : bruise.withAlpha (highlighted ? 0.8f : 0.6f));
    g.fillRoundedRectangle (b, corner);
    g.setColour (ink);
    g.drawRoundedRectangle (b, corner, 2.5f);
    g.setColour (on ? ink : bile);
    g.setFont (chunky (15.0f));
    g.drawText (on ? "SQUELCHING" : button.getButtonText(), b, juce::Justification::centred);
}

//==============================================================================
MoistBoiEditor::MoistBoiEditor (MoistBoiProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&goo);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
        addAndMakeVisible (k.slider);

        k.label.setText (specs[i].name, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (chunky (16.0f));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, specs[i].id, k.slider);
    }

    addAndMakeVisible (squelchButton);
    squelchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, "squelch", squelchButton);

    setSize (620, 480);
    startTimerHz (30);
}

MoistBoiEditor::~MoistBoiEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

float MoistBoiEditor::norm (const juce::String& id) const
{
    if (auto* param = proc.apvts.getParameter (id))
        return param->getValue();
    return 0.0f;
}

juce::Point<float> MoistBoiEditor::blobCentre() const { return blobArea.getCentre().translated (0.0f, -6.0f); }
float MoistBoiEditor::blobRadius() const { return juce::jmin (blobArea.getWidth(), blobArea.getHeight()) * 0.36f; }

void MoistBoiEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (80);
    area.removeFromBottom (36);

    auto left  = area.removeFromLeft (140).reduced (8, 0);
    auto right = area.removeFromRight (140).reduced (8, 0);

    auto place = [] (Knob& k, juce::Rectangle<int> r)
    {
        k.label.setBounds (r.removeFromTop (20));
        k.slider.setBounds (r);
    };

    const int h = left.getHeight() / 3;
    for (size_t i = 0; i < 3; ++i) place (knobs[i],     left.removeFromTop (h).reduced (0, 2));
    for (size_t i = 3; i < 6; ++i) place (knobs[i],     right.removeFromTop (h).reduced (0, 2));

    squelchButton.setBounds (area.removeFromBottom (48).withSizeKeepingCentre (136, 34));
    blobArea = area.toFloat();
}

void MoistBoiEditor::timerCallback()
{
    t += 1.0f / 30.0f;

    const float target = juce::jlimit (0.0f, 1.0f, proc.outputLevel.load());
    level += (target - level) * (target > level ? 0.5f : 0.12f);
    tagline = (int) (t / 4.0f) % numTaglines;

    // Spray sweat when it gets too moist
    const float moist = norm ("moisture");
    const auto c = blobCentre();
    const float R = blobRadius();
    if (moist > 0.45f && droplets.size() < 200 && rng.nextFloat() < (moist - 0.45f) * 2.5f)
    {
        const int n = 1 + (moist > 0.8f ? rng.nextInt (3) : 0);
        for (int i = 0; i < n; ++i)
        {
            const float a = -juce::MathConstants<float>::halfPi + (rng.nextFloat() - 0.5f) * 2.4f;
            Droplet d;
            d.pos  = { c.x + std::cos (a) * R, c.y + std::sin (a) * R * 0.9f };
            d.vel  = { std::cos (a) * (1.5f + rng.nextFloat() * 3.0f), std::sin (a) * (2.0f + rng.nextFloat() * 3.0f) - 2.0f };
            d.size = 3.0f + rng.nextFloat() * 3.0f;
            droplets.push_back (d);
        }
    }

    for (auto& d : droplets)
    {
        d.vel.y += 0.35f;
        d.pos += d.vel;
        d.life -= 0.02f;
    }
    const float bottom = (float) getHeight();
    droplets.erase (std::remove_if (droplets.begin(), droplets.end(),
                                    [bottom] (const Droplet& d) { return d.life <= 0.0f || d.pos.y > bottom; }),
                    droplets.end());

    repaint();
}

void MoistBoiEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (bile, 0.0f, 0.0f, slime.darker (0.35f), 0.0f, bounds.getHeight(), false));
    g.fillAll();

    // Grime stains (fixed seed so they don't flicker)
    juce::Random spots (1337);
    for (int i = 0; i < 40; ++i)
    {
        const float s = 8.0f + spots.nextFloat() * 40.0f;
        g.setColour (crustC.withAlpha (0.04f + spots.nextFloat() * 0.08f));
        g.fillEllipse (spots.nextFloat() * bounds.getWidth(), spots.nextFloat() * bounds.getHeight(), s, s * (0.6f + spots.nextFloat() * 0.6f));
    }

    // Knob column panels
    g.setColour (bruise.withAlpha (0.12f));
    g.fillRoundedRectangle (6.0f, 82.0f, 136.0f, bounds.getHeight() - 124.0f, 18.0f);
    g.fillRoundedRectangle (bounds.getWidth() - 142.0f, 82.0f, 136.0f, bounds.getHeight() - 124.0f, 18.0f);

    drawTitle (g, bounds.withHeight (80.0f));
    drawBlob (g);

    for (const auto& d : droplets)
        drawDrop (g, d.pos.x, d.pos.y, d.size, juce::jlimit (0.0f, 1.0f, d.life));

    g.setColour (ink.withAlpha (0.8f));
    g.setFont (chunky (15.0f, juce::Font::italic));
    g.drawText (taglines[tagline], bounds.withTop (bounds.getBottom() - 36.0f), juce::Justification::centred);
}

void MoistBoiEditor::drawTitle (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float moist = norm ("moisture");
    auto textArea = area.withSizeKeepingCentre (340.0f, 56.0f).translated (0.0f, 2.0f);

    // Drips under the letters
    g.setColour (slime.darker (0.25f));
    for (int k = 0; k < 10; ++k)
    {
        const float x = textArea.getX() + 20.0f + (float) k * 31.0f;
        const float len = 4.0f + 14.0f * (0.5f + 0.5f * std::sin (t * 0.8f + (float) k * 1.7f)) * (0.3f + moist);
        const float y = textArea.getBottom() - 12.0f;
        g.fillRoundedRectangle (x, y, 5.0f, len, 2.5f);
        g.fillEllipse (x - 1.5f, y + len - 4.0f, 8.0f, 8.0f);
    }

    g.setFont (chunky (52.0f));
    g.setColour (ink);
    g.drawText ("MOIST BOI", textArea.translated (3.0f, 3.0f), juce::Justification::centred);
    g.setColour (slime.brighter (0.2f));
    g.drawText ("MOIST BOI", textArea, juce::Justification::centred);
}

void MoistBoiEditor::drawBlob (juce::Graphics& g)
{
    const float moist  = norm ("moisture");
    const float stank  = norm ("stank");
    const float crustN = norm ("crust");
    const float gunkN  = norm ("gunk");
    const bool squelch = squelchButton.getToggleState();

    const auto c = blobCentre();
    const float R = blobRadius();
    const float wob = 0.03f + 0.10f * stank + 0.12f * level;

    // Puddle grows with moisture
    const float puddleW = R * (1.6f + 1.2f * moist);
    g.setColour (sweat.withAlpha (0.25f + 0.35f * moist));
    g.fillEllipse (c.x - puddleW * 0.5f, c.y + R * 0.78f, puddleW, R * 0.32f);
    g.setColour (ink.withAlpha (0.25f));
    g.fillEllipse (c.x - R * 1.0f, c.y + R * 0.8f, R * 2.0f, R * 0.25f);

    // Body
    juce::Path body;
    const int N = 72;
    for (int i = 0; i <= N; ++i)
    {
        const float th = juce::MathConstants<float>::twoPi * (float) i / (float) N;
        const float r = R * (1.0f + wob * std::sin (3.0f * th + t * 1.3f)
                                  + 0.5f * wob * std::sin (5.0f * th - t * 2.3f)
                                  + 0.04f * std::sin (2.0f * th + t * 0.7f));
        float px = c.x + std::cos (th) * r * 1.08f;
        float py = c.y + std::sin (th) * r * (0.92f + 0.04f * std::sin (t * 2.0f));
        if (std::sin (th) > 0.0f)
            py = c.y + (py - c.y) * 0.85f; // squish the bottom flat like he's sitting in it
        if (i == 0) body.startNewSubPath (px, py); else body.lineTo (px, py);
    }
    body.closeSubPath();
    body = body.createPathWithRoundedCorners (10.0f);

    const auto top = flesh.interpolatedWith (slime, juce::jmin (1.0f, 0.35f * gunkN + (squelch ? 0.4f : 0.0f)));
    const auto bot = top.darker (0.5f).interpolatedWith (crustC, crustN * 0.6f);
    g.setGradientFill (juce::ColourGradient (top, c.x - R * 0.5f, c.y - R, bot, c.x + R * 0.5f, c.y + R, false));
    g.fillPath (body);
    g.setColour (ink);
    g.strokePath (body, juce::PathStrokeType (4.0f));

    // Crusty patches
    juce::Random cr (42);
    const int nSpots = (int) (crustN * 18.0f);
    for (int i = 0; i < nSpots; ++i)
    {
        const float a = cr.nextFloat() * juce::MathConstants<float>::twoPi;
        const float dist = R * (0.3f + 0.55f * cr.nextFloat());
        const float s = R * (0.05f + 0.08f * cr.nextFloat());
        g.setColour (crustC.withAlpha (0.65f));
        g.fillEllipse (c.x + std::cos (a) * dist - s, c.y + std::sin (a) * dist * 0.8f - s * 0.7f, s * 2.0f, s * 1.4f);
    }

    // Wet shine
    g.setColour (juce::Colours::white.withAlpha (0.3f + 0.4f * moist));
    g.fillEllipse (c.x - R * 0.7f, c.y - R * 0.72f, R * 0.45f, R * 0.2f);

    // Blush
    g.setColour (tongue.withAlpha (0.25f + 0.3f * moist));
    g.fillEllipse (c.x - R * 0.75f, c.y + R * 0.05f, R * 0.3f, R * 0.15f);
    g.fillEllipse (c.x + R * 0.45f, c.y + R * 0.02f, R * 0.3f, R * 0.15f);

    // Googly eyes (mismatched on purpose)
    auto eye = [&] (float ex, float ey, float er, float phase)
    {
        g.setColour (juce::Colours::white);
        g.fillEllipse (ex - er, ey - er, er * 2.0f, er * 2.0f);
        g.setColour (ink);
        g.drawEllipse (ex - er, ey - er, er * 2.0f, er * 2.0f, 3.0f);

        float ox, oy;
        if (squelch) { ox = std::cos (t * 6.0f + phase) * 0.45f; oy = std::sin (t * 6.0f + phase) * 0.45f; }
        else         { ox = std::sin (t * 1.7f + phase) * 0.4f;  oy = std::cos (t * 1.1f + phase) * 0.3f; }

        const float pr = er * (0.45f - 0.2f * moist);
        g.fillEllipse (ex + ox * er - pr, ey + oy * er - pr, pr * 2.0f, pr * 2.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (ex + ox * er - pr * 0.5f, ey + oy * er - pr * 0.6f, pr * 0.5f, pr * 0.5f);
    };
    eye (c.x - R * 0.35f, c.y - R * 0.22f, R * 0.22f, 0.0f);
    eye (c.x + R * 0.32f, c.y - R * 0.27f, R * 0.16f, 1.3f);

    // Nervous brow sweat lines
    if (moist > 0.6f)
    {
        g.setColour (ink.withAlpha ((moist - 0.6f) * 2.5f));
        for (int i = 0; i < 3; ++i)
        {
            const float x = c.x - R * 0.15f + (float) i * R * 0.14f;
            g.drawLine (x, c.y - R * 0.62f, x + R * 0.05f, c.y - R * 0.5f, 2.5f);
        }
    }

    // Mouth flaps with the output level
    const float mx = c.x, my = c.y + R * 0.35f, mw = R * 0.7f;
    const float open = juce::jlimit (0.0f, 1.0f, 0.05f + level * 0.9f);
    if (open > 0.15f || squelch)
    {
        const float mh = R * 0.35f * juce::jmax (open, 0.35f);
        juce::Rectangle<float> m (mx - mw * 0.5f, my - mh * 0.3f, mw, mh);
        g.setColour (ink);
        g.fillEllipse (m);
        g.setColour (tongue);
        if (squelch)
            g.fillRoundedRectangle (mx - mw * 0.18f, my + mh * 0.2f, mw * 0.36f, mh * 0.9f + R * 0.1f * (0.5f + 0.5f * std::sin (t * 4.0f)), mw * 0.18f);
        else
            g.fillEllipse (m.reduced (mw * 0.2f, 0.0f).withTrimmedTop (mh * 0.5f));
    }
    else
    {
        juce::Path smile;
        smile.startNewSubPath (mx - mw * 0.5f, my);
        const int segs = 6;
        for (int i = 1; i <= segs; ++i)
        {
            const float px = mx - mw * 0.5f + mw * (float) i / (float) segs;
            const float py = my + std::sin ((float) i * 1.9f + t * 3.0f) * R * 0.03f + std::sin ((float) i / (float) segs * juce::MathConstants<float>::pi) * R * 0.08f;
            smile.lineTo (px, py);
        }
        g.setColour (ink);
        g.strokePath (smile, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Sweat beads sliding down his face
    const int beads = (int) (moist * 7.0f);
    juce::Random br (7);
    for (int k = 0; k < beads; ++k)
    {
        const float xOff = (br.nextFloat() - 0.5f) * R * 1.4f;
        const float progress = std::fmod (t * 0.25f * (1.0f + (float) k * 0.13f) + (float) k * 0.37f, 1.0f);
        const float y = c.y - R * 0.65f + progress * R * 1.1f;
        drawDrop (g, c.x + xOff, y, 3.5f + 2.0f * moist, 1.0f - progress);
    }
}
