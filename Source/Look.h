#pragma once
#include "PluginProcessor.h"
#include <functional>
#include <memory>
#include <vector>

//==============================================================================
// Palette (light backdrop by default, dark optional)
//==============================================================================
namespace pal
{
    inline bool light = true;

    inline const juce::Colour teal   { 0xff3ee6b5 };
    inline const juce::Colour violet { 0xff8b7cff };
    inline const juce::Colour sky    { 0xff2fb8ff };
    inline const juce::Colour amber  { 0xffffb454 };
    inline const juce::Colour coral  { 0xffff5d6c };
    inline const juce::Colour orange { 0xffff8a3d };
    inline const juce::Colour gold   { 0xffd9b866 };

    inline juce::Colour ink()  { return light ? juce::Colour (0xff1f2547) : juce::Colour (0xffe8edf0); }
    inline juce::Colour dim()  { return light ? juce::Colour (0xff535d86) : juce::Colour (0xff8a98a2); }
    inline juce::Colour overlay (float a) { return (light ? juce::Colour (0xff1f2547) : juce::Colours::white).withAlpha (a); }
    inline juce::Colour glassTop() { return light ? juce::Colours::white.withAlpha (0.62f) : juce::Colours::white.withAlpha (0.075f); }
    inline juce::Colour glassBot() { return light ? juce::Colours::white.withAlpha (0.34f) : juce::Colours::white.withAlpha (0.025f); }
    inline juce::Colour glassEdge() { return light ? juce::Colours::white.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.10f); }
    inline juce::Colour onAccent() { return juce::Colour (0xff06100d); }
}

inline juce::Font uiFont (float h, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), h, style));
}

inline void setColourProp (juce::Component& c, const char* key, juce::Colour col) { c.getProperties().set (key, (int) col.getARGB()); }
inline void setAccent (juce::Component& c, juce::Colour a, juce::Colour b) { setColourProp (c, "c1", a); setColourProp (c, "c2", b); }
inline juce::Colour getColourProp (const juce::Component& c, const char* key, juce::Colour def)
{
    return juce::Colour (static_cast<juce::uint32> (static_cast<int> (c.getProperties().getWithDefault (key, (int) def.getARGB()))));
}
inline juce::Colour getAccent (const juce::Component& c, const char* key, juce::Colour def) { return getColourProp (c, key, def); }

inline void repaintAll (juce::Component& c)
{
    c.repaint();
    for (auto* ch : c.getChildren()) repaintAll (*ch);
}

//==============================================================================
class AuroraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AuroraLookAndFeel()
    {
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff1f2547));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfff4f5fb));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xff1f2547));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffc9d2f2));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff10163a));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (14.f, juce::Font::bold); }
    juce::Font getPopupMenuFont() override { return uiFont (15.f); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        const int style = (int) s.getProperties().getWithDefault ("style", 0);
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float toA = startA + pos * (endA - startA);
        const bool hot = s.isMouseOverOrDragging();

        if (style == 1)      drawAmpKnob (g, c, r, startA, endA, toA, s, hot);
        else if (style == 2) drawPedalKnob (g, c, r, startA, endA, toA, s, hot);
        else                 drawChromeKnob (g, c, r, pos, startA, endA, toA, s, hot);
    }

    //----------------------------------------------------------------------------------------------
    static void drawChromeKnob (juce::Graphics& g, juce::Point<float> c, float r, float pos, float startA, float endA,
                                float toA, juce::Slider& s, bool hot)
    {
        const auto c1 = getAccent (s, "c1", pal::teal), c2 = getAccent (s, "c2", pal::violet);
        const float arcW = juce::jmax (3.f, r * 0.09f), arcR = r - arcW;

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startA, endA, true);
        g.setColour (pal::overlay (0.13f));
        g.strokePath (track, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (pos > 0.002f)
        {
            juce::Path v;
            v.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startA, toA, true);
            g.setGradientFill (juce::ColourGradient (c1.withAlpha (hot ? 0.34f : 0.22f), c.x - arcR, c.y,
                                                     c2.withAlpha (hot ? 0.34f : 0.22f), c.x + arcR, c.y, false));
            g.strokePath (v, juce::PathStrokeType (arcW * 2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setGradientFill (juce::ColourGradient (c1, c.x - arcR, c.y, c2, c.x + arcR, c.y, false));
            g.strokePath (v, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float bodyR = arcR - arcW * 2.2f;
        juce::Rectangle<float> body (c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f);
        g.setColour (juce::Colours::black.withAlpha (pal::light ? 0.18f : 0.45f));
        g.fillEllipse (body.translated (0.f, 3.f));
        const auto hi = pal::light ? juce::Colour (0xfffdfdff) : juce::Colour (0xff3a4650);
        const auto lo = pal::light ? juce::Colour (0xffbcc6e4) : juce::Colour (0xff0b141a);
        g.setGradientFill (juce::ColourGradient (hi, c.x - bodyR * 0.3f, c.y - bodyR * 0.4f, lo, c.x - bodyR * 0.3f + bodyR * 1.5f, c.y - bodyR * 0.4f, true));
        g.fillEllipse (body);
        g.setColour (hot ? c1.withAlpha (0.8f) : pal::overlay (0.22f));
        g.drawEllipse (body, 1.f);

        const float capR = bodyR * 0.62f;
        g.setGradientFill (juce::ColourGradient (pal::light ? juce::Colour (0xffeef1fb) : juce::Colour (0xff1c2a33), c.x, c.y - capR,
                                                 pal::light ? juce::Colour (0xffcdd5ee) : juce::Colour (0xff0d161c), c.x, c.y + capR, false));
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2.f, capR * 2.f);

        juce::Path p;
        p.startNewSubPath (c.x + std::sin (toA) * bodyR * 0.42f, c.y - std::cos (toA) * bodyR * 0.42f);
        p.lineTo (c.x + std::sin (toA) * bodyR * 0.92f, c.y - std::cos (toA) * bodyR * 0.92f);
        g.setColour (pal::ink());
        g.strokePath (p, juce::PathStrokeType (juce::jmax (2.f, r * 0.06f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Amp knob: dark knurled body, thin gold ring, printed 0-10 scale
    static void drawAmpKnob (juce::Graphics& g, juce::Point<float> c, float r, float startA, float endA, float toA,
                             juce::Slider& s, bool hot)
    {
        const auto body1 = getColourProp (s, "k1", juce::Colour (0xff2b1d16));
        const auto cap1  = getColourProp (s, "k2", juce::Colour (0xff1a110c));
        const auto ptr   = getColourProp (s, "k3", juce::Colour (0xfff4efe6));
        const auto tick  = getColourProp (s, "tick", juce::Colour (0xff3a2a14));

        for (int k = 0; k <= 10; ++k)
        {
            const float a = startA + (float) k / 10.f * (endA - startA);
            const bool major = (k % 5) == 0;
            const float r0 = r * (major ? 0.80f : 0.87f), r1 = r * 0.98f;
            g.setColour (tick.withAlpha (major ? 0.95f : 0.6f));
            g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, major ? 1.8f : 1.1f);
        }

        const float br = r * 0.68f;
        juce::Rectangle<float> body (c.x - br, c.y - br, br * 2.f, br * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (body.translated (0.f, 3.f).expanded (1.f));
        g.setGradientFill (juce::ColourGradient (body1.brighter (0.55f), c.x - br * 0.5f, c.y - br * 0.6f, body1.darker (0.5f), c.x + br * 0.7f, c.y + br * 0.9f, true));
        g.fillEllipse (body);

        g.setColour (juce::Colours::black.withAlpha (0.30f));
        for (int k = 0; k < 32; ++k)
        {
            const float a = (float) k / 32.f * 6.2831853f;
            g.drawLine (c.x + std::sin (a) * br * 0.84f, c.y - std::cos (a) * br * 0.84f, c.x + std::sin (a) * br * 0.99f, c.y - std::cos (a) * br * 0.99f, 1.f);
        }
        g.setColour (pal::gold.withAlpha (hot ? 1.f : 0.75f));
        g.drawEllipse (body.expanded (1.5f), 1.4f);

        const float cr = br * 0.60f;
        g.setGradientFill (juce::ColourGradient (cap1.brighter (0.35f), c.x, c.y - cr, cap1.darker (0.3f), c.x, c.y + cr, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);

        juce::Path p;
        p.startNewSubPath (c.x + std::sin (toA) * br * 0.20f, c.y - std::cos (toA) * br * 0.20f);
        p.lineTo (c.x + std::sin (toA) * br * 0.96f, c.y - std::cos (toA) * br * 0.96f);
        g.setColour (ptr);
        g.strokePath (p, juce::PathStrokeType (juce::jmax (2.4f, r * 0.07f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Pedal knob: teal-grey ridged cap with printed scale dots
    static void drawPedalKnob (juce::Graphics& g, juce::Point<float> c, float r, float startA, float endA, float toA,
                               juce::Slider& s, bool hot)
    {
        const auto body1 = getColourProp (s, "k1", juce::Colour (0xff5f8d94));
        const auto cap1  = getColourProp (s, "k2", juce::Colour (0xff86adb3));
        const auto ptr   = getColourProp (s, "k3", juce::Colour (0xfff7f4ec));
        const auto tick  = getColourProp (s, "tick", juce::Colour (0xff4a4036));

        for (int k = 0; k <= 10; ++k)
        {
            const float a = startA + (float) k / 10.f * (endA - startA);
            const float r0 = r * ((k % 5) == 0 ? 0.84f : 0.90f), r1 = r * 0.99f;
            g.setColour (tick.withAlpha (0.85f));
            g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 1.1f);
        }

        const float br = r * 0.72f;
        juce::Rectangle<float> body (c.x - br, c.y - br, br * 2.f, br * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillEllipse (body.translated (0.f, 3.f));
        g.setGradientFill (juce::ColourGradient (body1.brighter (0.3f), c.x - br * 0.5f, c.y - br * 0.6f, body1.darker (0.45f), c.x + br * 0.6f, c.y + br * 0.9f, true));
        g.fillEllipse (body);

        g.setColour (juce::Colours::black.withAlpha (0.20f));
        for (int k = 0; k < 28; ++k)
        {
            const float a = (float) k / 28.f * 6.2831853f;
            g.drawLine (c.x + std::sin (a) * br * 0.80f, c.y - std::cos (a) * br * 0.80f, c.x + std::sin (a) * br * 0.99f, c.y - std::cos (a) * br * 0.99f, 1.f);
        }
        g.setColour (hot ? juce::Colours::white.withAlpha (0.6f) : juce::Colours::white.withAlpha (0.22f));
        g.drawEllipse (body, 1.f);

        const float cr = br * 0.66f;
        g.setGradientFill (juce::ColourGradient (cap1.brighter (0.25f), c.x - cr * 0.4f, c.y - cr * 0.6f, body1, c.x + cr * 0.5f, c.y + cr, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);

        juce::Path p;
        p.startNewSubPath (c.x + std::sin (toA) * br * 0.18f, c.y - std::cos (toA) * br * 0.18f);
        p.lineTo (c.x + std::sin (toA) * br * 0.94f, c.y - std::cos (toA) * br * 0.94f);
        g.setColour (ptr);
        g.strokePath (p, juce::PathStrokeType (juce::jmax (2.2f, r * 0.07f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    //----------------------------------------------------------------------------------------------
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        auto area = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const auto c1 = getAccent (b, "c1", pal::teal), c2 = getAccent (b, "c2", pal::violet);
        const float ph = 22.f, pw = 40.f;
        juce::Rectangle<float> pill (area.getX() + 2.f, area.getCentreY() - ph * 0.5f, pw, ph);

        if (on)
        {
            g.setColour (c1.withAlpha (0.25f));
            g.fillRoundedRectangle (pill.expanded (3.f), ph * 0.5f + 3.f);
            g.setGradientFill (juce::ColourGradient (c1, pill.getX(), 0.f, c2, pill.getRight(), 0.f, false));
        }
        else
            g.setColour (pal::overlay (hl ? 0.24f : 0.17f));
        g.fillRoundedRectangle (pill, ph * 0.5f);

        const float d = ph - 6.f;
        g.setColour (on || pal::light ? juce::Colours::white : pal::dim());
        g.fillEllipse (on ? pill.getRight() - d - 3.f : pill.getX() + 3.f, pill.getY() + 3.f, d, d);

        g.setColour (on ? pal::ink() : pal::dim());
        g.setFont (uiFont (13.f, juce::Font::bold));
        g.drawText (b.getButtonText(), (int) pill.getRight() + 8, 0, b.getWidth() - (int) pill.getRight() - 8, b.getHeight(), juce::Justification::centredLeft);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const auto c1 = getAccent (b, "c1", pal::teal), c2 = getAccent (b, "c2", pal::violet);
        g.setColour (pal::light ? juce::Colours::white.withAlpha (down ? 0.9f : hl ? 0.8f : 0.6f) : juce::Colours::white.withAlpha (down ? 0.2f : hl ? 0.13f : 0.06f));
        g.fillRoundedRectangle (r, 11.f);
        g.setGradientFill (juce::ColourGradient (c1.withAlpha (0.95f), r.getX(), 0.f, c2.withAlpha (0.95f), r.getRight(), 0.f, false));
        g.drawRoundedRectangle (r, 11.f, 1.4f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setColour (pal::ink());
        g.setFont (uiFont (14.f, juce::Font::bold));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
    }
};

//==============================================================================
// Knob: three layouts. Standard (name above, value below), Compact (knob left, text right),
// Printed (name printed under the knob, value bubble on hover - for amps and pedals)
//==============================================================================
class Knob : public juce::Component
{
public:
    enum Layout { Standard, Compact, Printed };

    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& name,
          const juce::String& suffix = {}, int decimals = 1, double scale = 1.0, Layout lay = Standard)
        : attach (apvts, id, slider), nameText (name), suffixText (suffix), dec (decimals), scl (scale), layout (lay)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        slider.onValueChange = [this] { repaint(); };
        slider.addMouseListener (this, false);
        addAndMakeVisible (slider);
    }

    void setName (const juce::String& n) { nameText = n; repaint(); }
    void setAccent (juce::Colour a, juce::Colour b) { ::setAccent (slider, a, b); repaint(); }
    void setStyle (int s) { slider.getProperties().set ("style", s); repaint(); }
    void setSkin (juce::Colour body, juce::Colour cap, juce::Colour pointer, juce::Colour tick)
    {
        setColourProp (slider, "k1", body); setColourProp (slider, "k2", cap);
        setColourProp (slider, "k3", pointer); setColourProp (slider, "tick", tick);
        repaint();
    }
    void setTextColours (juce::Colour name) { nameCol = name; useCols = true; repaint(); }

    void resized() override
    {
        auto b = getLocalBounds();
        if (layout == Compact) slider.setBounds (b.removeFromLeft (b.getHeight()));
        else if (layout == Printed) slider.setBounds (b.withTrimmedBottom (18));
        else slider.setBounds (b.withTrimmedTop (18).withTrimmedBottom (20));
    }

    void paint (juce::Graphics& g) override
    {
        const bool hot = slider.isMouseOverOrDragging();
        const auto vc = hot ? pal::ink() : pal::dim();

        if (layout == Compact)
        {
            auto t = getLocalBounds().withTrimmedLeft (getHeight() + 6);
            g.setColour (pal::dim());  g.setFont (uiFont (13.f));
            g.drawText (nameText, t.removeFromTop (t.getHeight() / 2).withTrimmedTop (4), juce::Justification::bottomLeft);
            g.setColour (pal::ink());  g.setFont (uiFont (14.f, juce::Font::bold));
            g.drawText (valueString(), t, juce::Justification::topLeft);
            return;
        }
        if (layout == Printed)
        {
            g.setColour (useCols ? nameCol : pal::dim());
            g.setFont (uiFont (10.5f, juce::Font::bold).withExtraKerningFactor (0.10f));
            g.drawText (nameText.toUpperCase(), 0, getHeight() - 17, getWidth(), 16, juce::Justification::centred);
            return;
        }
        g.setColour (pal::dim());  g.setFont (uiFont (13.f));
        g.drawText (nameText, 0, 0, getWidth(), 18, juce::Justification::centred);
        g.setColour (vc);  g.setFont (uiFont (13.f, juce::Font::bold));
        g.drawText (valueString(), 0, getHeight() - 20, getWidth(), 20, juce::Justification::centred);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        if (layout != Printed || ! slider.isMouseOverOrDragging()) return;
        const auto txt = valueString();
        const float w = 18.f + (float) txt.length() * 7.5f;
        juce::Rectangle<float> bub (((float) getWidth() - w) * 0.5f, -22.f, w, 20.f);
        g.setColour (juce::Colour (0xf0151a24));
        g.fillRoundedRectangle (bub, 10.f);
        g.setColour (juce::Colours::white);
        g.setFont (uiFont (12.f, juce::Font::bold));
        g.drawText (txt, bub.toNearestInt(), juce::Justification::centred);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { repaint(); }

private:
    juce::String valueString() const
    {
        const double num = slider.getValue() * scl;
        return (dec == 0 ? juce::String (juce::roundToInt (num)) : juce::String (num, dec)) + suffixText;
    }

    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attach;
    juce::String nameText, suffixText;
    int dec; double scl; Layout layout;
    juce::Colour nameCol; bool useCols = false;
};

//==============================================================================
// Chip selector bound to a choice parameter
//==============================================================================
class ChipSelector : public juce::Component
{
public:
    ChipSelector (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, int columns, float fontSize = 13.f)
        : param (*apvts.getParameter (id)), cols (columns), fsz (fontSize),
          attach (param, [this] (float v) { index = (int) std::lround (v); repaint(); if (onChange) onChange (index); })
    {
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (&param)) choices = cp->choices;
        attach.sendInitialUpdate();
    }

    int getIndex() const { return index; }
    void setAccent (juce::Colour a, juce::Colour b) { c1 = a; c2 = b; repaint(); }
    int rowsNeeded() const { return (choices.size() + cols - 1) / cols; }
    std::function<void (int)> onChange;

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < choices.size(); ++i)
        {
            auto r = chipRect (i);
            const bool sel = i == index;
            if (sel)
            {
                g.setColour (c1.withAlpha (0.25f));
                g.fillRoundedRectangle (r.expanded (2.f), 12.f);
                g.setGradientFill (juce::ColourGradient (c1, r.getX(), 0.f, c2, r.getRight(), 0.f, false));
                g.fillRoundedRectangle (r, 10.f);
                g.setColour (pal::onAccent());
            }
            else
            {
                g.setColour (pal::overlay (i == hover ? 0.16f : 0.09f));
                g.fillRoundedRectangle (r, 10.f);
                g.setColour (i == hover ? pal::ink() : pal::dim());
            }
            g.setFont (uiFont (fsz, juce::Font::bold));
            g.drawText (choices[i], r.toNearestInt(), juce::Justification::centred);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override { setHover (hit (e.position)); }
    void mouseExit (const juce::MouseEvent&) override { setHover (-1); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = hit (e.position);
        if (i >= 0) attach.setValueAsCompleteGesture ((float) i);
    }

private:
    juce::Rectangle<float> chipRect (int i) const
    {
        const int rows = rowsNeeded();
        const float gap = 6.f;
        const float w = ((float) getWidth() - gap * (float) (cols - 1)) / (float) cols;
        const float h = ((float) getHeight() - gap * (float) (rows - 1)) / (float) rows;
        return { (float) (i % cols) * (w + gap), (float) (i / cols) * (h + gap), w, h };
    }
    int hit (juce::Point<float> p) const
    {
        for (int i = 0; i < choices.size(); ++i) if (chipRect (i).contains (p)) return i;
        return -1;
    }
    void setHover (int h) { if (h != hover) { hover = h; repaint(); } }

    juce::RangedAudioParameter& param;
    juce::StringArray choices;
    int cols, index = 0, hover = -1;
    float fsz;
    juce::Colour c1 { pal::teal }, c2 { pal::violet };
    juce::ParameterAttachment attach;
};

//==============================================================================
// Dropdown bound to a choice parameter (styled like a preset box)
//==============================================================================
class ChoiceButton : public juce::Component
{
public:
    ChoiceButton (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& caption)
        : param (*apvts.getParameter (id)), cap (caption),
          attach (param, [this] (float v) { index = (int) std::lround (v); repaint(); })
    {
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (&param)) choices = cp->choices;
        attach.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.f);
        g.setColour (pal::light ? juce::Colours::white.withAlpha (over ? 0.95f : 0.82f) : juce::Colours::white.withAlpha (over ? 0.13f : 0.07f));
        g.fillRoundedRectangle (r, 10.f);
        g.setColour (pal::overlay (0.14f));
        g.drawRoundedRectangle (r, 10.f, 1.f);
        g.setColour (pal::dim());   g.setFont (uiFont (11.5f, juce::Font::bold).withExtraKerningFactor (0.06f));
        g.drawText (cap.toUpperCase(), getLocalBounds().withTrimmedLeft (16).withTrimmedBottom (getHeight() / 2), juce::Justification::bottomLeft);
        g.setColour (pal::ink());  g.setFont (uiFont (16.f, juce::Font::bold));
        g.drawText (choices[index], getLocalBounds().withTrimmedLeft (16).withTrimmedTop (getHeight() / 2 - 3), juce::Justification::topLeft);
        juce::Path tri;
        const float cx = (float) getWidth() - 24.f, cy = (float) getHeight() * 0.5f;
        tri.addTriangle (cx - 6.f, cy - 3.f, cx + 6.f, cy - 3.f, cx, cy + 4.f);
        g.setColour (pal::dim());
        g.fillPath (tri);
    }

    void mouseEnter (const juce::MouseEvent&) override { over = true; repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { over = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override
    {
        juce::PopupMenu m;
        for (int i = 0; i < choices.size(); ++i) m.addItem (i + 1, choices[i], true, i == index);
        juce::Component::SafePointer<ChoiceButton> sp (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                         [sp] (int r) { if (sp != nullptr && r > 0) sp->attach.setValueAsCompleteGesture ((float) (r - 1)); });
    }

private:
    juce::RangedAudioParameter& param;
    juce::StringArray choices;
    juce::String cap;
    int index = 0;
    bool over = false;
    juce::ParameterAttachment attach;
};

//==============================================================================
// Hexagonal gold footswitch with steel cap, bound to a bool parameter
//==============================================================================
class HexSwitch : public juce::Component
{
public:
    HexSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
        : attach (*apvts.getParameter (id), [this] (float v) { on = v > 0.5f; repaint(); })
    {
        attach.sendInitialUpdate();
    }

    bool isOn() const { return on; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const auto c = b.getCentre();
        const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 2.f;
        const float dy = down ? 2.f : 0.f;

        juce::Path hex;
        for (int k = 0; k < 6; ++k)
        {
            const float a = juce::MathConstants<float>::pi / 6.f + (float) k * juce::MathConstants<float>::pi / 3.f;
            const float px = c.x + std::cos (a) * R, py = c.y + dy + std::sin (a) * R;
            if (k == 0) hex.startNewSubPath (px, py); else hex.lineTo (px, py);
        }
        hex.closeSubPath();

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillPath (hex, juce::AffineTransform::translation (0.f, 4.f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeacb77), c.x - R * 0.6f, c.y - R * 0.8f, juce::Colour (0xff8f6a22), c.x + R * 0.7f, c.y + R * 0.9f, false));
        g.fillPath (hex);
        g.setColour (juce::Colour (0xff5b4210).withAlpha (0.8f));
        g.strokePath (hex, juce::PathStrokeType (1.2f));

        const float cr = R * 0.60f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (c.x - cr, c.y + dy - cr + 2.f, cr * 2.f, cr * 2.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff1f4f7), c.x - cr * 0.5f, c.y + dy - cr * 0.7f, juce::Colour (0xff6c767f), c.x + cr * 0.6f, c.y + dy + cr, true));
        g.fillEllipse (c.x - cr, c.y + dy - cr, cr * 2.f, cr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawEllipse (c.x - cr, c.y + dy - cr, cr * 2.f, cr * 2.f, 1.f);
    }

    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); attach.setValueAsCompleteGesture (on ? 0.f : 1.f); }
    void mouseUp (const juce::MouseEvent&) override { down = false; repaint(); }

private:
    bool on = false, down = false;
    juce::ParameterAttachment attach;
};

//==============================================================================
// Level meter (stereo) and LED
//==============================================================================
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (AuroraProcessor& p, bool horiz = false) : proc (p), horizontal (horiz) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::Rectangle<float> bar = horizontal
                ? juce::Rectangle<float> (b.getX(), b.getY() + (float) ch * (b.getHeight() * 0.5f + 1.f), b.getWidth(), b.getHeight() * 0.5f - 1.f)
                : juce::Rectangle<float> (b.getX() + (float) ch * (b.getWidth() * 0.5f + 1.f), b.getY(), b.getWidth() * 0.5f - 1.f, b.getHeight());
            g.setColour (pal::overlay (0.13f));
            g.fillRoundedRectangle (bar, 3.f);

            const float db = juce::jlimit (-60.f, 0.f, juce::Decibels::gainToDecibels (lvl[ch], -60.f));
            const float frac = (db + 60.f) / 60.f;
            if (frac < 0.01f) continue;

            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (bar, 3.f);
            g.reduceClipRegion (clip);
            const auto fill = horizontal ? bar.withWidth (bar.getWidth() * frac) : bar.withTrimmedTop (bar.getHeight() * (1.f - frac));
            g.reduceClipRegion (fill.toNearestInt());

            juce::ColourGradient grad = horizontal
                ? juce::ColourGradient (pal::teal, bar.getX(), 0.f, pal::coral, bar.getRight(), 0.f, false)
                : juce::ColourGradient (pal::coral, 0.f, bar.getY(), pal::teal, 0.f, bar.getBottom(), false);
            grad.addColour (horizontal ? 0.78 : 0.22, pal::amber);
            g.setGradientFill (grad);
            g.fillRect (bar);
        }
    }

private:
    void timerCallback() override
    {
        for (int ch = 0; ch < 2; ++ch) lvl[ch] = juce::jmax (proc.meterPeak[ch].load(), lvl[ch] * 0.86f);
        repaint();
    }

    AuroraProcessor& proc;
    bool horizontal;
    float lvl[2] { 0.f, 0.f };
};

class Led : public juce::Component, private juce::Timer
{
public:
    explicit Led (std::function<bool()> source) : src (std::move (source)) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (3.f);
        if (on) { g.setColour (pal::teal.withAlpha (0.35f)); g.fillEllipse (b.expanded (3.f)); }
        g.setColour (on ? pal::teal : pal::overlay (0.2f));
        g.fillEllipse (b);
    }

private:
    void timerCallback() override { const bool now = src(); if (now != on) { on = now; repaint(); } }
    std::function<bool()> src;
    bool on = false;
};
