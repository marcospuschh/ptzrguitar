#pragma once
#include "Look.h"

//==============================================================================
// Helpers
//==============================================================================
inline float textW (const juce::Font& f, const juce::String& s)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (f, s, 0.f, 0.f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}

inline void drawGlass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (pal::light) { g.setColour (juce::Colours::black.withAlpha (0.07f)); g.fillRoundedRectangle (r.translated (0.f, 5.f), rad); }
    g.setGradientFill (juce::ColourGradient (pal::glassTop(), 0.f, r.getY(), pal::glassBot(), 0.f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (pal::glassEdge());
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.f);
}

inline void drawScrew (juce::Graphics& g, float x, float y, float r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f3f5), x - r * 0.4f, y - r * 0.5f, juce::Colour (0xff7d858d), x + r, y + r, true));
    g.fillEllipse (x - r, y - r, r * 2.f, r * 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine (x - r * 0.6f, y + r * 0.1f, x + r * 0.6f, y - r * 0.1f, 1.2f);
}

//==============================================================================
// Pedal
//==============================================================================
struct KnobSpec { const char* id; const char* label; const char* suffix; int dec; double scale; };

class PedalView : public juce::Component
{
public:
    PedalView (AuroraProcessor& p, const juce::String& title, int pattern, const juce::String& onId,
               juce::Colour body, const std::vector<KnobSpec>& specs)
        : name (title), pat (pattern), bodyCol (body), sw (p.apvts, onId),
          onAtt (*p.apvts.getParameter (onId), [this] (float v) { on = v > 0.5f; repaint(); })
    {
        for (auto& s : specs)
        {
            knobs.push_back (std::make_unique<Knob> (p.apvts, s.id, s.label, s.suffix, s.dec, s.scale, Knob::Printed));
            knobs.back()->setStyle (2);
            knobs.back()->setTextColours (juce::Colour (0xff4a4036));
            addAndMakeVisible (*knobs.back());
        }
        addAndMakeVisible (sw);
        onAtt.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (10.f, 4.f);
        for (int i = 0; i < 4; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (pal::light ? 0.05f : 0.12f));
            g.fillRoundedRectangle (r.translated (0.f, 6.f + (float) i * 2.f).expanded ((float) i * 2.f, 0.f), 26.f);
        }

        // side jacks
        for (int side = 0; side < 2; ++side)
        {
            const float x = side == 0 ? r.getX() - 10.f : r.getRight() - 4.f;
            juce::Rectangle<float> j (x, r.getY() + r.getHeight() * 0.44f, 14.f, 40.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeacb77), j.getX(), 0.f, juce::Colour (0xff8f6a22), j.getRight(), 0.f, false));
            g.fillRoundedRectangle (j, 4.f);
            g.setColour (juce::Colour (0xff5b4210).withAlpha (0.7f));
            g.drawRoundedRectangle (j, 4.f, 1.f);
        }

        g.setGradientFill (juce::ColourGradient (bodyCol.brighter (0.10f), 0.f, r.getY(), bodyCol.darker (0.14f), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 24.f);
        g.setColour (juce::Colour (0xff3b3228).withAlpha (0.35f));
        g.drawRoundedRectangle (r, 24.f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (r.reduced (2.f), 22.f, 1.f);

        // wooden footswitch panel
        const auto panel = panelRect().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff86452a), 0.f, panel.getY(), juce::Colour (0xff4b2216), 0.f, panel.getBottom(), false));
        g.fillRoundedRectangle (panel, 12.f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addRoundedRectangle (panel, 12.f);
            g.reduceClipRegion (clip);
            drawPattern (g, panel);
        }
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (panel, 12.f, 1.5f);

        // LED
        const juce::Point<float> led (panel.getCentreX(), panel.getY() + 20.f);
        if (on) { g.setColour (pal::teal.withAlpha (0.35f)); g.fillEllipse (led.x - 13.f, led.y - 13.f, 26.f, 26.f); }
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xffbffff0) : juce::Colour (0xff5a4a40), led.x - 2.f, led.y - 2.f,
                                                 on ? pal::teal : juce::Colour (0xff2c2018), led.x + 4.f, led.y + 5.f, true));
        g.fillEllipse (led.x - 6.f, led.y - 6.f, 12.f, 12.f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (led.x - 6.f, led.y - 6.f, 12.f, 12.f, 1.f);

        // name banner with chevrons
        const auto font = uiFont (19.f, juce::Font::bold).withExtraKerningFactor (0.14f);
        const float tw = textW (font, name.toUpperCase());
        const float by = panel.getY() - 38.f, cx = r.getCentreX();
        g.setColour (juce::Colour (0xff3b3228));
        g.setFont (font);
        g.drawText (name.toUpperCase(), (int) (cx - tw * 0.5f - 4.f), (int) by, (int) tw + 8, 28, juce::Justification::centred);
        for (int side = 0; side < 2; ++side)
            for (int k = 0; k < 4; ++k)
            {
                g.setColour (juce::Colour (0xff3b3228).withAlpha (0.30f + 0.17f * (float) (3 - k)));
                const float dir = side == 0 ? -1.f : 1.f;   // arrows point toward the name
                const float x0 = cx + dir * (tw * 0.5f + 16.f + (float) k * 9.f), y0 = by + 14.f;
                const float tip = -dir * 3.f;
                juce::Path ch;
                ch.startNewSubPath (x0 - tip, y0 - 5.f);
                ch.lineTo (x0 + tip, y0);
                ch.lineTo (x0 - tip, y0 + 5.f);
                g.strokePath (ch, juce::PathStrokeType (1.6f));
            }

        if (! on)
        {
            g.setColour (juce::Colours::black.withAlpha (0.06f));
            g.fillRoundedRectangle (r, 24.f);
        }
    }

    void resized() override
    {
        const auto panel = panelRect();
        sw.setBounds (panel.getCentreX() - 34, panel.getY() + 40, 68, 68);

        auto r = getLocalBounds().reduced (10, 4);
        const int top = r.getY() + 18, bottom = panel.getY() - 46;
        const int n = (int) knobs.size(), rows = (n + 1) / 2;
        const int cw = (r.getWidth() - 12) / 2, ch = juce::jmin (118, (bottom - top) / juce::jmax (1, rows));
        for (int i = 0; i < n; ++i)
        {
            const int row = i / 2, inRow = juce::jmin (2, n - row * 2);
            const int x0 = r.getX() + 6 + (2 - inRow) * cw / 2;
            knobs[(size_t) i]->setBounds (x0 + (i % 2) * cw, top + row * ch, cw, ch);
        }
    }

private:
    juce::Rectangle<int> panelRect() const
    {
        auto r = getLocalBounds().reduced (10, 4);
        return { r.getX() + 14, r.getBottom() - 16 - 128, r.getWidth() - 28, 128 };
    }

    void drawPattern (juce::Graphics& g, juce::Rectangle<float> p) const
    {
        const auto lite = juce::Colour (0xffd89a66).withAlpha (0.28f);
        const auto dark = juce::Colours::black.withAlpha (0.28f);
        switch (pat)
        {
            case 0:   // grooves
                for (float x = p.getX(); x < p.getRight(); x += 6.f)
                { g.setColour (dark); g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
                  g.setColour (lite); g.drawVerticalLine ((int) x + 2, p.getY(), p.getBottom()); }
                break;
            case 1:   // concentric rings
                for (float rr = 6.f; rr < 140.f; rr += 8.f)
                { g.setColour (dark); g.drawEllipse (p.getCentreX() - rr, p.getCentreY() + 10.f - rr, rr * 2.f, rr * 2.f, 1.4f);
                  g.setColour (lite); g.drawEllipse (p.getCentreX() - rr - 1.5f, p.getCentreY() + 8.5f - rr, rr * 2.f, rr * 2.f, 1.f); }
                break;
            case 2:   // diamond lattice
                for (float x = p.getX() - 140.f; x < p.getRight() + 140.f; x += 16.f)
                { g.setColour (dark);
                  g.drawLine (x, p.getY(), x + 128.f, p.getBottom(), 1.4f); g.drawLine (x + 128.f, p.getY(), x, p.getBottom(), 1.4f); }
                break;
            case 3:   // drops
                for (float y = p.getY() + 6.f, row = 0; y < p.getBottom(); y += 13.f, row += 1.f)
                    for (float x = p.getX() + (std::fmod (row, 2.f) > 0.5f ? 9.f : 0.f); x < p.getRight(); x += 18.f)
                    { g.setColour (dark); g.fillEllipse (x, y, 7.f, 9.f); g.setColour (lite); g.drawEllipse (x - 1.f, y - 1.f, 7.f, 9.f, 1.f); }
                break;
            case 4:   // waves
                for (float y = p.getY() + 4.f; y < p.getBottom(); y += 9.f)
                { juce::Path w; w.startNewSubPath (p.getX(), y);
                  for (float x = p.getX(); x <= p.getRight(); x += 4.f) w.lineTo (x, y + 3.f * std::sin (x * 0.12f + y * 0.05f));
                  g.setColour (dark); g.strokePath (w, juce::PathStrokeType (1.4f)); }
                break;
            default:  // triangles
                for (float y = p.getY(); y < p.getBottom(); y += 14.f) { g.setColour (dark); g.drawHorizontalLine ((int) y, p.getX(), p.getRight()); }
                for (float x = p.getX() - 80.f; x < p.getRight() + 80.f; x += 16.f)
                { g.setColour (dark); g.drawLine (x, p.getY(), x + 56.f, p.getBottom(), 1.f); g.drawLine (x + 56.f, p.getY(), x, p.getBottom(), 1.f); }
                break;
        }
    }

    juce::String name;
    int pat;
    juce::Colour bodyCol;
    bool on = false;
    HexSwitch sw;
    juce::ParameterAttachment onAtt;
    std::vector<std::unique_ptr<Knob>> knobs;
};

//==============================================================================
// Mic placement: a speaker with a draggable microphone (centre to edge)
//==============================================================================
class MicPlacement : public juce::Component
{
public:
    MicPlacement (AuroraProcessor& p, const juce::String& prefix)
        : posAtt (*p.apvts.getParameter (prefix + "micPos"), [this] (float v) { pos = v; repaint(); }),
          micAtt (*p.apvts.getParameter (prefix + "mic"), [this] (float v) { micIdx = (int) std::lround (v); repaint(); })
    {
        posAtt.sendInitialUpdate();
        micAtt.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const auto c = b.getCentre();
        const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.36f;

        auto baffle = b.reduced (3.f);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRoundedRectangle (baffle.translated (0.f, 4.f), 20.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b2026), 0.f, baffle.getY(), juce::Colour (0xff0b0e11), 0.f, baffle.getBottom(), false));
        g.fillRoundedRectangle (baffle, 20.f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addRoundedRectangle (baffle, 20.f);
            g.reduceClipRegion (clip);
            g.setColour (juce::Colours::white.withAlpha (0.035f));
            for (float x = baffle.getX(); x < baffle.getRight(); x += 7.f) g.drawVerticalLine ((int) x, baffle.getY(), baffle.getBottom());
            for (float y = baffle.getY(); y < baffle.getBottom(); y += 7.f) g.drawHorizontalLine ((int) y, baffle.getX(), baffle.getRight());
        }
        g.setColour (pal::gold.withAlpha (0.55f));
        g.drawRoundedRectangle (baffle, 20.f, 1.4f);

        g.setColour (juce::Colour (0xff0a0c0e));
        g.fillEllipse (c.x - R * 1.14f, c.y - R * 1.14f, R * 2.28f, R * 2.28f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff343b43), c.x - R * 0.4f, c.y - R * 0.5f,
                                                 juce::Colour (0xff101418), c.x + R * 0.8f, c.y + R * 0.9f, true));
        g.fillEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f);
        for (int k = 1; k < 6; ++k)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            const float rr = R * (float) k / 6.f * 0.95f;
            g.drawEllipse (c.x - rr, c.y - rr, rr * 2.f, rr * 2.f, 1.f);
        }
        const float dcR = R * 0.26f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff59616a), c.x - dcR * 0.4f, c.y - dcR * 0.5f,
                                                 juce::Colour (0xff1b2026), c.x + dcR, c.y + dcR, true));
        g.fillEllipse (c.x - dcR, c.y - dcR, dcR * 2.f, dcR * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawLine (c.x, c.y, c.x + R, c.y, 1.f);

        static const juce::uint32 bodyC[5] { 0xff2a2d31, 0xff1d1f22, 0xff9aa3ab, 0xff1e2f44, 0xffc9a24d };
        static const juce::uint32 headC[5] { 0xffaab2ba, 0xffc9ccd0, 0xff3a3f45, 0xff9fb4c8, 0xffe8d9a0 };
        const float tx = c.x + pos * R, ty = c.y;
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (0.5f, tx, ty));
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (tx - 8.f, ty - 78.f, 18.f, 64.f, 7.f);
        g.setColour (juce::Colour (bodyC[juce::jlimit (0, 4, micIdx)]));
        g.fillRoundedRectangle (tx - 9.f, ty - 80.f, 18.f, 64.f, 7.f);
        g.setColour (juce::Colour (headC[juce::jlimit (0, 4, micIdx)]));
        g.fillRoundedRectangle (tx - 10.f, ty - 26.f, 20.f, 26.f, 8.f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        for (float yy = ty - 22.f; yy < ty - 2.f; yy += 4.f) g.drawHorizontalLine ((int) yy, tx - 9.f, tx + 9.f);
    }

    void mouseDown (const juce::MouseEvent& e) override { posAtt.beginGesture(); update (e); }
    void mouseDrag (const juce::MouseEvent& e) override { update (e); }
    void mouseUp (const juce::MouseEvent&) override { posAtt.endGesture(); }

private:
    void update (const juce::MouseEvent& e)
    {
        const float R = juce::jmin ((float) getWidth(), (float) getHeight()) * 0.36f;
        posAtt.setValueAsPartOfGesture (juce::jlimit (0.f, 1.f, (e.position.x - (float) getWidth() * 0.5f) / R));
    }

    float pos = 0.2f;
    int micIdx = 0;
    juce::ParameterAttachment posAtt, micAtt;
};

//==============================================================================
// Amp: a hero amp head with the controls on its brass plate, plus a cabinet section
//==============================================================================
class AmpPanel : public juce::Component
{
public:
    static constexpr int W = 1140, H = 520;

    AmpPanel (AuroraProcessor& p, const juce::String& prefix, int slot)
        : proc (p), irSlot (slot),
          models (p.apvts, prefix + "model", 4, 13.f),
          cabs (p.apvts, prefix + "cab", 5, 12.5f),
          mics (p.apvts, prefix + "mic", 5, 12.5f),
          micView (p, prefix),
          kLevel (p.apvts, prefix + "level", "Level", " dB", 1),
          kPan (p.apvts, prefix + "pan", "Pan", "", 2)
    {
        static const char* ids[6] { "gain", "bass", "mid", "treble", "pres", "master" };
        for (int i = 0; i < 6; ++i)
        {
            knobs[i] = std::make_unique<Knob> (p.apvts, prefix + ids[i], ids[i], "", 1, 1.0, Knob::Printed);
            knobs[i]->setStyle (1);
            addAndMakeVisible (*knobs[i]);
        }
        models.setAccent (pal::amber, pal::orange);
        cabs.setAccent (pal::amber, pal::orange);
        mics.setAccent (pal::amber, pal::orange);
        addAndMakeVisible (models); addAndMakeVisible (cabs); addAndMakeVisible (mics);
        addAndMakeVisible (micView); addAndMakeVisible (kLevel); addAndMakeVisible (kPan);

        irBtn.setButtonText ("Load impulse response");
        irBtn.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Choose a cabinet impulse response", juce::File {}, "*.wav;*.aif;*.aiff;*.flac");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      auto f = fc.getResult();
                                      if (f.existsAsFile()) { proc.loadIR (irSlot, f); updateIR(); }
                                  });
        };
        addAndMakeVisible (irBtn);
        irLabel.setFont (uiFont (13.f));
        irLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (irLabel);

        models.onChange = [this] (int) { applyModel(); };
        cabs.onChange = [this] (int) { updateIR(); resized(); repaint(); };
        applyModel();
        updateIR();
    }

    void setDual (bool d) { dual = d; kPan.setVisible (d); resized(); }

    void paint (juce::Graphics& g) override
    {
        const int m = juce::jlimit (0, 3, models.getIndex());
        const auto& s = skin (m);
        const auto& mm = ampModel (m);
        irLabel.setColour (juce::Label::textColourId, pal::dim());

        // floor shadow
        g.setColour (juce::Colours::black.withAlpha (pal::light ? 0.10f : 0.30f));
        g.fillEllipse (130.f, 346.f, 880.f, 34.f);

        // handle
        juce::Path h;
        h.startNewSubPath (480.f, 54.f);
        h.cubicTo (480.f, 4.f, 660.f, 4.f, 660.f, 54.f);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.strokePath (h, juce::PathStrokeType (12.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), juce::AffineTransform::translation (0.f, 3.f));
        g.setColour (juce::Colour (s.handle));
        g.strokePath (h, juce::PathStrokeType (11.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.strokePath (h, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), juce::AffineTransform::translation (0.f, -2.f));
        for (float x : { 462.f, 642.f })
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeacb77), x, 46.f, juce::Colour (0xff8f6a22), x + 36.f, 62.f, false));
            g.fillRoundedRectangle (x, 46.f, 36.f, 14.f, 4.f);
        }

        // feet
        g.setColour (juce::Colour (0xff17181a));
        g.fillRoundedRectangle (170.f, 352.f, 56.f, 14.f, 5.f);
        g.fillRoundedRectangle (914.f, 352.f, 56.f, 14.f, 5.f);

        // body
        const juce::Rectangle<float> body (110.f, 50.f, 920.f, 308.f);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (body.translated (0.f, 6.f), 28.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (s.body1), 0.f, body.getY(), juce::Colour (s.body2), 0.f, body.getBottom(), false));
        g.fillRoundedRectangle (body, 28.f);
        {   // tolex grain
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addRoundedRectangle (body, 28.f);
            g.reduceClipRegion (clip);
            juce::Random rnd (3);
            for (int i = 0; i < 1400; ++i)
            {
                g.setColour ((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha (0.045f));
                g.fillRect (body.getX() + rnd.nextFloat() * body.getWidth(), body.getY() + rnd.nextFloat() * body.getHeight(), 1.6f, 1.6f);
            }
        }
        g.setColour (juce::Colour (s.trim).withAlpha (0.9f));
        g.drawRoundedRectangle (body.reduced (7.f), 22.f, 2.2f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawRoundedRectangle (body, 28.f, 1.2f);

        // grille / artwork
        const juce::Rectangle<float> gr (138.f, 76.f, 864.f, 164.f);
        if (m == 2) drawAurora (g, gr); else drawGrille (g, gr, m);
        g.setColour (juce::Colour (s.trim).withAlpha (0.95f));
        g.drawRoundedRectangle (gr.expanded (3.f), 14.f, 2.6f);

        // name badge
        {
            const auto font = uiFont (13.f, juce::Font::bold).withExtraKerningFactor (0.16f);
            const juce::String t = juce::String (mm.name).toUpperCase();
            const float bw = textW (font, t) + 40.f;
            juce::Rectangle<float> badge (gr.getCentreX() - bw * 0.5f, gr.getBottom() - 34.f, bw, 24.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeacb77), badge.getX(), badge.getY(), juce::Colour (0xff9a762a), badge.getRight(), badge.getBottom(), false));
            g.fillRoundedRectangle (badge, 6.f);
            g.setColour (juce::Colour (0xff3a2a14));
            g.setFont (font);
            g.drawText (t, badge.toNearestInt(), juce::Justification::centred);
        }

        // control plate
        const juce::Rectangle<float> pl (138.f, 252.f, 864.f, 92.f);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (pl.translated (0.f, 3.f), 8.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (s.plate1), 0.f, pl.getY(), juce::Colour (s.plate2), 0.f, pl.getBottom(), false));
        g.fillRoundedRectangle (pl, 8.f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        for (float y = pl.getY() + 2.f; y < pl.getBottom(); y += 3.f) g.drawHorizontalLine ((int) y, pl.getX() + 4.f, pl.getRight() - 4.f);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawRoundedRectangle (pl, 8.f, 1.2f);
        for (float sx : { pl.getX() + 12.f, pl.getRight() - 12.f })
            for (float sy : { pl.getY() + 12.f, pl.getBottom() - 12.f }) drawScrew (g, sx, sy, 4.5f);

        // input jack
        const juce::Point<float> jk (pl.getX() + 52.f, pl.getCentreY() - 8.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9edf1), jk.x - 8.f, jk.y - 8.f, juce::Colour (0xff626b73), jk.x + 12.f, jk.y + 12.f, true));
        g.fillEllipse (jk.x - 13.f, jk.y - 13.f, 26.f, 26.f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillEllipse (jk.x - 6.f, jk.y - 6.f, 12.f, 12.f);
        g.setColour (juce::Colour (s.ink));
        g.setFont (uiFont (10.5f, juce::Font::bold).withExtraKerningFactor (0.1f));
        g.drawText ("INPUT", (int) jk.x - 30, (int) jk.y + 18, 60, 14, juce::Justification::centred);

        // power lamp + switch
        const juce::Point<float> lamp (pl.getRight() - 78.f, pl.getCentreY() - 16.f);
        g.setColour (juce::Colour (0xff53f0b6).withAlpha (0.35f));
        g.fillEllipse (lamp.x - 12.f, lamp.y - 12.f, 24.f, 24.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd7ffee), lamp.x - 3.f, lamp.y - 3.f, juce::Colour (0xff1fb98a), lamp.x + 6.f, lamp.y + 7.f, true));
        g.fillEllipse (lamp.x - 6.f, lamp.y - 6.f, 12.f, 12.f);
        const juce::Rectangle<float> sw (pl.getRight() - 56.f, pl.getCentreY() - 14.f, 22.f, 22.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeef1f4), sw.getX(), sw.getY(), juce::Colour (0xff6c747c), sw.getRight(), sw.getBottom(), true));
        g.fillEllipse (sw);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawEllipse (sw, 1.2f);
        g.setColour (juce::Colour (s.ink));
        g.setFont (uiFont (10.5f, juce::Font::bold).withExtraKerningFactor (0.1f));
        g.drawText ("POWER", (int) pl.getRight() - 100, (int) pl.getCentreY() + 14, 80, 14, juce::Justification::centred);

        // ---- cabinet section ----
        drawGlass (g, { 0.f, 380.f, (float) W, (float) H - 380.f }, 22.f);
        g.setColour (pal::dim());
        g.setFont (uiFont (11.5f, juce::Font::bold).withExtraKerningFactor (0.12f));
        g.drawText ("CABINET", 168, 386, 200, 16, juce::Justification::centredLeft);
        g.drawText ("MICROPHONE", 168, 436, 200, 16, juce::Justification::centredLeft);
        if (cabs.getIndex() != 4)
        {
            g.setFont (uiFont (12.f));
            g.drawText ("Drag the microphone over the speaker. Centre is brighter and tighter, the edge is darker and smoother.",
                        168, 492, 620, 16, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        models.setBounds (0, 0, 440, 30);

        const int m = juce::jlimit (0, 3, models.getIndex());
        const auto& mm = ampModel (m);
        int count = 0;
        for (int i = 0; i < 6; ++i) if (mm.uses[i]) ++count;
        const int areaX = 138 + 112, areaW = 864 - 112 - 124;
        const int slotW = juce::jmin (118, areaW / juce::jmax (1, count));
        int x = areaX + (areaW - slotW * count) / 2;
        for (int i = 0; i < 6; ++i)
            if (mm.uses[i]) { knobs[i]->setBounds (x, 258, slotW, 82); x += slotW; }

        micView.setBounds (16, 390, 128, 118);
        cabs.setBounds (168, 404, 610, 28);
        mics.setBounds (168, 454, 610, 28);
        const bool irMode = cabs.getIndex() == 4;
        irBtn.setVisible (irMode); irLabel.setVisible (irMode);
        irBtn.setBounds (168, 488, 200, 26);
        irLabel.setBounds (378, 488, 400, 26);

        kLevel.setBounds (820, 388, 120, 124);
        kPan.setBounds (960, 388, 120, 124);
    }

private:
    struct Skin { juce::uint32 body1, body2, trim, plate1, plate2, ink, kBody, kCap, kPtr, kTick, handle; };
    static const Skin& skin (int i)
    {
        static const Skin s[4] {
            { 0xff24262b, 0xff131417, 0xffc3c9d0, 0xffdfe3e8, 0xffaab2bb, 0xff1e252d, 0xff25282c, 0xff15171a, 0xffffffff, 0xff2a3038, 0xff17181a },
            { 0xff4d3727, 0xff2d2016, 0xffd8b25c, 0xffe3c987, 0xffb08d45, 0xff3a2a14, 0xff2e1f17, 0xff1c120d, 0xfff7ecd2, 0xff3a2a14, 0xff5e3c24 },
            { 0xfff3ead9, 0xffd8ccb4, 0xffd8b25c, 0xffe0c27a, 0xffa9843d, 0xff3a2a14, 0xff2d1c28, 0xff170e14, 0xfff7ecd2, 0xff3a2a14, 0xff4a2f20 },
            { 0xffe9792a, 0xffb85410, 0xfff1d9a8, 0xff2d2d2f, 0xff161617, 0xfff0e4c8, 0xffeadfc4, 0xffcdbf9b, 0xff2a2a2c, 0xffe8dcc0, 0xff1a1a1a } };
        return s[i];
    }

    void drawGrille (juce::Graphics& g, juce::Rectangle<float> r, int m) const
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (r, 10.f);
        g.reduceClipRegion (clip);
        if (m == 0)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffa3aec4), 0.f, r.getY(), juce::Colour (0xff6b7992), 0.f, r.getBottom(), false));
            g.fillRect (r);
            for (float y = r.getY(); y < r.getBottom(); y += 3.f) { g.setColour (juce::Colours::black.withAlpha (0.13f)); g.drawHorizontalLine ((int) y, r.getX(), r.getRight()); }
            for (float x = r.getX(); x < r.getRight(); x += 3.f) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom()); }
        }
        else if (m == 1)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd5b47e), 0.f, r.getY(), juce::Colour (0xff9a7845), 0.f, r.getBottom(), false));
            g.fillRect (r);
            g.setColour (juce::Colour (0xff3a2a14).withAlpha (0.45f));
            for (float x = r.getX() - 170.f; x < r.getRight() + 170.f; x += 22.f)
            { g.drawLine (x, r.getY(), x + 170.f, r.getBottom(), 1.6f); g.drawLine (x + 170.f, r.getY(), x, r.getBottom(), 1.6f); }
            g.setColour (juce::Colour (0xfff3dca8).withAlpha (0.35f));
            for (float x = r.getX(); x < r.getRight(); x += 22.f)
                for (float y = r.getY() + 11.f; y < r.getBottom(); y += 22.f) g.fillEllipse (x - 1.5f, y - 1.5f, 3.f, 3.f);
        }
        else
        {
            g.setColour (juce::Colour (0xff1b1b1c));
            g.fillRect (r);
            for (float y = r.getY(), row = 0; y < r.getBottom(); y += 8.f, row += 1.f)
                for (float x = r.getX() + (std::fmod (row, 2.f) > 0.5f ? 8.f : 0.f); x < r.getRight(); x += 16.f)
                { g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillRect (x, y, 8.f, 3.f);
                  g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (x, y + 4.f, 8.f, 3.f); }
        }
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (1.f), 10.f, 3.f);
    }

    // Original artwork: aurora over mountains
    void drawAurora (juce::Graphics& g, juce::Rectangle<float> r) const
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (r, 10.f);
        g.reduceClipRegion (clip);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050b1a), 0.f, r.getY(), juce::Colour (0xff17424d), 0.f, r.getBottom(), false));
        g.fillRect (r);

        juce::Random rnd (11);
        for (int i = 0; i < 90; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.2f + rnd.nextFloat() * 0.6f));
            const float sz = 0.8f + rnd.nextFloat() * 1.6f;
            g.fillEllipse (r.getX() + rnd.nextFloat() * r.getWidth(), r.getY() + rnd.nextFloat() * r.getHeight() * 0.6f, sz, sz);
        }

        auto curtain = [&] (float y0, float amp, float freq, float ph, float hgt, juce::Colour c, float a)
        {
            juce::Path p;
            auto edge = [&] (float x) { return y0 + amp * std::sin (x * freq + ph) + amp * 0.45f * std::sin (x * freq * 2.4f - ph * 1.7f); };
            p.startNewSubPath (r.getX(), edge (r.getX()));
            for (float x = r.getX() + 6.f; x <= r.getRight(); x += 6.f) p.lineTo (x, edge (x));
            p.lineTo (r.getRight(), edge (r.getRight()) + hgt);
            p.lineTo (r.getX(), edge (r.getX()) + hgt);
            p.closeSubPath();
            g.setGradientFill (juce::ColourGradient (c.withAlpha (a), 0.f, y0 - amp, c.withAlpha (0.f), 0.f, y0 + hgt, false));
            g.fillPath (p);
        };
        curtain (r.getY() + 46.f, 20.f, 0.010f, 0.6f, 130.f, pal::teal, 0.85f);
        curtain (r.getY() + 34.f, 26.f, 0.007f, 2.4f, 120.f, pal::violet, 0.65f);
        curtain (r.getY() + 70.f, 14.f, 0.013f, 4.1f, 90.f, pal::sky, 0.45f);

        // moon
        g.setColour (juce::Colour (0xfff4ecd8).withAlpha (0.9f));
        g.fillEllipse (r.getX() + 60.f, r.getY() + 26.f, 22.f, 22.f);
        g.setColour (juce::Colour (0xfff4ecd8).withAlpha (0.18f));
        g.fillEllipse (r.getX() + 48.f, r.getY() + 14.f, 46.f, 46.f);

        // mountains
        auto ridge = [&] (float base, float amp, float seed, juce::Colour c)
        {
            juce::Path p;
            p.startNewSubPath (r.getX(), r.getBottom());
            for (float x = r.getX(); x <= r.getRight() + 8.f; x += 8.f)
                p.lineTo (x, r.getBottom() - base - amp * (0.55f * std::sin (x * 0.011f + seed) + 0.30f * std::sin (x * 0.027f + seed * 2.3f) + 0.15f * std::sin (x * 0.061f + seed * 4.1f)));
            p.lineTo (r.getRight(), r.getBottom());
            p.closeSubPath();
            g.setColour (c);
            g.fillPath (p);
        };
        ridge (44.f, 36.f, 1.2f, juce::Colour (0xff0e2230));
        ridge (22.f, 26.f, 3.7f, juce::Colour (0xff08111c));
        g.setColour (pal::gold.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (5.f), 7.f, 1.f);
    }

    void applyModel()
    {
        const int m = juce::jlimit (0, 3, models.getIndex());
        const auto& mm = ampModel (m);
        const auto& s = skin (m);
        for (int i = 0; i < 6; ++i)
        {
            knobs[i]->setVisible (mm.uses[i]);
            knobs[i]->setName (mm.label[i]);
            knobs[i]->setSkin (juce::Colour (s.kBody), juce::Colour (s.kCap), juce::Colour (s.kPtr), juce::Colour (s.kTick));
            knobs[i]->setTextColours (juce::Colour (s.ink));
        }
        resized();
        repaint();
    }

    void updateIR()
    {
        const auto n = proc.getIRName (irSlot);
        irLabel.setText (n.isEmpty() ? "No file loaded. Pick a .wav cabinet IR." : n, juce::dontSendNotification);
        irLabel.setColour (juce::Label::textColourId, pal::dim());
    }

    AuroraProcessor& proc;
    int irSlot;
    bool dual = false;
    ChipSelector models, cabs, mics;
    MicPlacement micView;
    std::unique_ptr<Knob> knobs[6];
    Knob kLevel, kPan;
    juce::TextButton irBtn;
    juce::Label irLabel;
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
// Effect card: title, on/off, before/after amp, type chips, knobs
//==============================================================================
struct FxKnob { const char* id; const char* label; const char* suffix; int dec; double scale; int typeMask; const char* altLabel = nullptr; int altType = -1; };

class FxCard : public juce::Component
{
public:
    FxCard (AuroraProcessor& p, const juce::String& title, const juce::String& sub, const char* onId, const char* posId,
            const char* typeId, int typeCols, juce::Colour a1c, juce::Colour a2c, const std::vector<FxKnob>& ks, int extraH = 0)
        : title_ (title), sub_ (sub), c1 (a1c), c2 (a2c), extra (extraH), specs (ks),
          pos (p.apvts, posId, 2, 13.f), type (p.apvts, typeId, typeCols, 13.f),
          onAtt (p.apvts, onId, onBtn)
    {
        setAccent (onBtn, c1, c2);
        onBtn.setButtonText ("On");
        addAndMakeVisible (onBtn);
        pos.setAccent (c1, c2); type.setAccent (c1, c2);
        addAndMakeVisible (pos); addAndMakeVisible (type);
        for (auto& s : specs)
        {
            knobs.push_back (std::make_unique<Knob> (p.apvts, s.id, s.label, s.suffix, s.dec, s.scale));
            knobs.back()->setAccent (c1, c2);
            addAndMakeVisible (*knobs.back());
        }
        type.onChange = [this] (int) { applyType(); };
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawGlass (g, r, 22.f);
        g.setGradientFill (juce::ColourGradient (c1, 16.f, 0.f, c2, r.getRight() - 16.f, 0.f, false));
        g.fillRoundedRectangle (16.f, 0.f, r.getWidth() - 32.f, 3.f, 1.5f);
        g.setColour (pal::ink());
        g.setFont (uiFont (22.f, juce::Font::bold));
        g.drawText (title_, 20, 12, getWidth() - 140, 28, juce::Justification::centredLeft);
        g.setColour (pal::dim());
        g.setFont (uiFont (12.5f));
        g.drawText (sub_, 20, 38, getWidth() - 140, 18, juce::Justification::centredLeft);
    }

    void resized() override
    {
        const int w = getWidth(), pad = 18;
        onBtn.setBounds (w - 84, 14, 70, 26);
        pos.setBounds (pad, 64, w - 2 * pad, 28);
        const int rows = type.rowsNeeded();
        const int th = rows * 30 + (rows - 1) * 6;
        type.setBounds (pad, 102, w - 2 * pad, th);

        const int ky = 102 + th + 10;
        int visible = 0;
        for (auto& k : knobs) if (k->isVisible()) ++visible;
        const int cols = 3, cw = (w - 2 * pad) / cols, ch = 116;
        int idx = 0;
        for (auto& k : knobs)
        {
            if (! k->isVisible()) continue;
            const int row = idx / cols, inRow = juce::jmin (cols, visible - row * cols);
            const int x0 = pad + (cols - inRow) * cw / 2;
            k->setBounds (x0 + (idx % cols) * cw, ky + row * ch, cw, ch);
            ++idx;
        }
        extraArea = { pad, getHeight() - extra - 12, w - 2 * pad, extra };
    }

protected:
    virtual void typeChanged (int) {}
    juce::Rectangle<int> extraArea;

private:
    void applyType()
    {
        const int t = type.getIndex();
        for (size_t i = 0; i < specs.size(); ++i)
        {
            const bool vis = specs[i].typeMask == 0 || ((specs[i].typeMask >> t) & 1);
            knobs[i]->setVisible (vis);
            if (specs[i].altLabel != nullptr)
                knobs[i]->setName (t == specs[i].altType ? specs[i].altLabel : specs[i].label);
        }
        resized();
        typeChanged (t);
    }

    juce::String title_, sub_;
    juce::Colour c1, c2;
    int extra;
    std::vector<FxKnob> specs;
    ChipSelector pos, type;
    juce::ToggleButton onBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment onAtt;
    std::vector<std::unique_ptr<Knob>> knobs;

public:
    void init() { applyType(); }
};

class DelayCard : public FxCard
{
public:
    DelayCard (AuroraProcessor& p)
        : FxCard (p, "Delay", "Digital, analog, tape, ping-pong and dual", "dlyOn", "dlyPos", "dlyType", 3, pal::sky, pal::teal,
                  { { "dlyTime", "Time", " ms", 0, 1.0, 0 }, { "dlyFb", "Feedback", "%", 0, 100.0, 0 },
                    { "dlyMix", "Mix", "%", 0, 100.0, 0 }, { "dlyTone", "Tone", "%", 0, 100.0, 0 },
                    { "dlyMod", "Wow", "%", 0, 100.0, 6, "Mod", 1 } }, 84),
          ratio (p.apvts, "dlyRatio", 4, 13.f),
          apvts (p.apvts),
          timeAtt (*p.apvts.getParameter ("dlyTime"), [this] (float ms) { bpmText = juce::String ((int) std::lround (ms)) + " ms  /  " + juce::String ((int) std::lround (60000.f / ms)) + " bpm"; repaint(); })
    {
        ratio.setAccent (pal::sky, pal::teal);
        tap.setButtonText ("Tap tempo");
        setAccent (tap, pal::sky, pal::teal);
        tap.onClick = [this] { doTap(); };
        addAndMakeVisible (tap); addAndMakeVisible (ratio);
        timeAtt.sendInitialUpdate();
        init();
    }

    void resized() override
    {
        FxCard::resized();
        auto a = extraArea;
        tap.setBounds (a.removeFromTop (34).removeFromLeft (120));
        ratio.setBounds (a.withTrimmedTop (8).withHeight (28));
    }

    void paint (juce::Graphics& g) override
    {
        FxCard::paint (g);
        g.setColour (pal::ink());
        g.setFont (uiFont (14.f, juce::Font::bold));
        g.drawText (bpmText, extraArea.getX() + 134, extraArea.getY(), 220, 34, juce::Justification::centredLeft);
        if (ratio.isVisible())
        {
            g.setColour (pal::dim());
            g.setFont (uiFont (12.f));
            g.drawText ("Right-side repeat as a fraction of the time", extraArea.getX(), extraArea.getY() + 38 + 28, extraArea.getWidth(), 16, juce::Justification::centredLeft);
        }
    }

protected:
    void typeChanged (int t) override { ratio.setVisible (t == 4); }

private:
    void doTap()
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (! taps.empty() && now - taps.back() > 2000.0) taps.clear();
        taps.push_back (now);
        if (taps.size() > 5) taps.erase (taps.begin());
        if (taps.size() < 2) return;
        const double ms = juce::jlimit (40.0, 1000.0, (taps.back() - taps.front()) / (double) (taps.size() - 1));
        auto* prm = apvts.getParameter ("dlyTime");
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) ms));
        prm->endChangeGesture();
    }

    ChipSelector ratio;
    juce::AudioProcessorValueTreeState& apvts;
    juce::TextButton tap;
    std::vector<double> taps;
    juce::String bpmText;
    juce::ParameterAttachment timeAtt;
};

class ModCard : public FxCard
{
public:
    ModCard (AuroraProcessor& p)
        : FxCard (p, "Modulation", "Chorus, vibrato and tremolo", "modOn", "modPos", "modType", 3, pal::violet, pal::sky,
                  { { "modRate", "Rate", "%", 0, 100.0, 0 }, { "modDepth", "Depth", "%", 0, 100.0, 0 },
                    { "modMix", "Mix", "%", 0, 100.0, 1 } }, 40),
          shape (p.apvts, "modShape", 3, 13.f)
    {
        shape.setAccent (pal::violet, pal::sky);
        addAndMakeVisible (shape);
        init();
    }

    void resized() override { FxCard::resized(); shape.setBounds (extraArea.withHeight (28)); }

protected:
    void typeChanged (int t) override { shape.setVisible (t != 0); }

private:
    ChipSelector shape;
};

//==============================================================================
// Signal chain strip
//==============================================================================
class ChainStrip : public juce::Component, private juce::Timer
{
public:
    explicit ChainStrip (AuroraProcessor& p) : proc (p) { startTimerHz (12); }

    std::function<void (int)> onNavigate;   // 0 pedals, 1 amp, 2 effects

    void paint (juce::Graphics& g) override
    {
        auto items = build();
        const auto font = uiFont (13.f, juce::Font::bold);
        float total = 0.f;
        for (auto& it : items) { it.w = textW (font, it.label) + 26.f; total += it.w; }
        const float avail = (float) getWidth() - 8.f;
        const float gap = items.size() > 1 ? juce::jlimit (6.f, 20.f, (avail - total) / (float) (items.size() - 1)) : 0.f;
        float x = 4.f;
        const float y = 4.f, h = 30.f;

        float zx0 = -1.f, zx1 = -1.f, xx = x;
        for (auto& it : items) { if (it.kind == 2) { if (zx0 < 0) zx0 = xx - 8.f; zx1 = xx + it.w + 8.f; } xx += it.w + gap; }
        if (zx0 >= 0.f)
        {
            g.setColour (pal::amber.withAlpha (pal::light ? 0.16f : 0.07f));
            g.fillRoundedRectangle (zx0, y - 3.f, zx1 - zx0, h + 6.f, 18.f);
            g.setColour (pal::amber.withAlpha (0.55f));
            g.drawRoundedRectangle (zx0, y - 3.f, zx1 - zx0, h + 6.f, 18.f, 1.f);
        }

        hits.clear();
        for (size_t i = 0; i < items.size(); ++i)
        {
            auto& it = items[i];
            juce::Rectangle<float> r (x, y, it.w, h);
            hits.push_back ({ r, it.kind, it.fxId });

            const juce::Colour acc = it.kind == 3 ? it.accent : it.kind == 2 ? pal::amber : it.kind == 1 ? pal::teal : pal::dim();
            if (it.active)
            {
                g.setColour (pal::light ? juce::Colours::white.withAlpha (0.75f) : acc.withAlpha (0.16f));
                g.fillRoundedRectangle (r, 15.f);
                g.setColour (acc.withAlpha (0.95f));
                g.drawRoundedRectangle (r.reduced (0.5f), 15.f, 1.4f);
                g.setColour (pal::ink());
            }
            else
            {
                g.setColour (pal::overlay (0.05f));
                g.fillRoundedRectangle (r, 15.f);
                g.setColour (pal::overlay (0.14f));
                g.drawRoundedRectangle (r.reduced (0.5f), 15.f, 1.f);
                g.setColour (pal::dim().withAlpha (0.8f));
            }
            g.setFont (font);
            g.drawText (it.label, r.toNearestInt(), juce::Justification::centred);

            if (i + 1 < items.size())
            {
                g.setColour (pal::dim().withAlpha (0.7f));
                const float cx = x + it.w + gap * 0.5f;
                juce::Path ch;
                ch.startNewSubPath (cx - 2.5f, y + h * 0.5f - 4.f);
                ch.lineTo (cx + 2.5f, y + h * 0.5f);
                ch.lineTo (cx - 2.5f, y + h * 0.5f + 4.f);
                g.strokePath (ch, juce::PathStrokeType (1.4f));
            }
            x += it.w + gap;
        }

        g.setColour (pal::dim());
        g.setFont (uiFont (11.5f));
        g.drawText ("Click Mod, Delay or Reverb to move it before or after the amp", 0, 38, getWidth(), 14, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (auto& h : hits)
        {
            if (! h.r.contains (e.position)) continue;
            if (h.kind == 3)
            {
                static const char* posIds[3] { "modPos", "dlyPos", "revPos" };
                auto* prm = proc.apvts.getParameter (posIds[h.fxId]);
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.f : 1.f);
                prm->endChangeGesture();
            }
            else if (onNavigate)
                onNavigate (h.kind == 1 ? 0 : h.kind == 2 ? 1 : -1);
            return;
        }
    }

private:
    struct Item { juce::String label; int kind; int fxId; bool active; juce::Colour accent; float w = 0.f; };
    struct Hit { juce::Rectangle<float> r; int kind, fxId; };

    float val (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    juce::String choiceName (const char* id) const
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id))) return c->getCurrentChoiceName();
        return {};
    }

    std::vector<Item> build() const
    {
        std::vector<Item> v;
        v.push_back ({ "Input", 0, 0, true, pal::dim() });
        v.push_back ({ "Gate", 0, 0, val ("gate") > -89.f, pal::dim() });
        {
            auto gn = choiceName ("guitar");
            if (gn.contains (" (")) gn = gn.upToFirstOccurrenceOf (" (", false, false);
            v.push_back ({ gn, 0, 0, (int) std::lround (val ("guitar")) != 0, pal::dim() });
        }
        int np = 0;
        for (const char* id : { "comp1On", "comp2On", "oct1On", "oct2On", "drv1On", "drv2On" }) if (val (id) > 0.5f) ++np;
        v.push_back ({ np ? "Pedals " + juce::String (np) : juce::String ("Pedals"), 1, 0, np > 0, pal::teal });

        auto fx = [&] (int which, bool pre)
        {
            static const char* posIds[3] { "modPos", "dlyPos", "revPos" };
            static const char* onIds[3] { "modOn", "dlyOn", "revOn" };
            static const char* typeIds[3] { "modType", "dlyType", "revType" };
            static const juce::uint32 cols[3] { 0xff8b7cff, 0xff2fb8ff, 0xff1fc79a };
            if (((int) std::lround (val (posIds[which])) == 0) != pre) return;
            auto name = choiceName (typeIds[which]);
            if (which == 1) name += " delay";
            if (which == 2) name += " reverb";
            v.push_back ({ name, 3, which, val (onIds[which]) > 0.5f, juce::Colour (cols[which]) });
        };
        for (int i = 0; i < 3; ++i) fx (i, true);

        const bool dual = val ("dual") > 0.5f;
        v.push_back ({ dual ? "Amp A " + choiceName ("a_model") : "Amp " + choiceName ("a_model"), 2, 0, true, pal::amber });
        if (dual) v.push_back ({ "Amp B " + choiceName ("b_model"), 2, 0, true, pal::amber });
        v.push_back ({ choiceName ("a_cab"), 2, 0, true, pal::amber });

        for (int i = 0; i < 3; ++i) fx (i, false);
        v.push_back ({ "Output", 0, 0, true, pal::dim() });
        return v;
    }

    void timerCallback() override { repaint(); }

    AuroraProcessor& proc;
    std::vector<Hit> hits;
};

//==============================================================================
// Tuner (YIN pitch detection on the message thread)
//==============================================================================
class TunerView : public juce::Component, private juce::Timer
{
public:
    explicit TunerView (AuroraProcessor& p) : proc (p) { startTimerHz (20); }

    void paint (juce::Graphics& g) override
    {
        const juce::Colour txt (0xffe8edf0), dimc (0xff8fa0aa);
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b1015), 0.f, 0.f, juce::Colour (0xff111a21), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 22.f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (r.reduced (0.5f), 22.f, 1.f);

        const bool has = freq > 0.f;
        const bool inTune = has && std::fabs (cents) < 5.f;
        const auto acc = ! has ? dimc : inTune ? pal::teal : std::fabs (cents) < 20.f ? pal::amber : pal::coral;

        g.setColour (dimc);
        g.setFont (uiFont (14.f, juce::Font::bold));
        g.drawText ("Tuner (output is muted)", getLocalBounds().removeFromTop (60), juce::Justification::centred);

        if (has)
        {
            g.setColour (acc);
            g.setFont (uiFont (150.f, juce::Font::bold));
            g.drawText (note, getLocalBounds().withTrimmedTop (50).withHeight (190), juce::Justification::centred);
        }

        g.setColour (dimc);
        g.setFont (uiFont (18.f));
        g.drawText (has ? juce::String (freq, 1) + " Hz   " + juce::String (cents, 0) + " cents" : "Play a string",
                    getLocalBounds().withTrimmedTop (236).withHeight (30), juce::Justification::centred);

        const float bw = 640.f, bx = r.getCentreX() - bw * 0.5f, by = 310.f;
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (bx, by, bw, 14.f, 7.f);
        for (int k = -5; k <= 5; ++k)
        {
            g.setColour (juce::Colours::white.withAlpha (k == 0 ? 0.6f : 0.25f));
            const float tx = bx + bw * 0.5f + (float) k * bw / 10.f;
            g.drawLine (tx, by + 24.f, tx, by + (k == 0 ? 46.f : 36.f), k == 0 ? 2.f : 1.f);
        }
        if (has)
        {
            const float nx = bx + bw * 0.5f + juce::jlimit (-50.f, 50.f, cents) / 50.f * bw * 0.5f;
            g.setColour (acc.withAlpha (0.3f));
            g.fillEllipse (nx - 16.f, by - 9.f, 32.f, 32.f);
            g.setColour (acc);
            g.fillEllipse (nx - 9.f, by - 2.f, 18.f, 18.f);
        }
        g.setColour (dimc);
        g.setFont (uiFont (13.f));
        g.drawText ("flat", (int) bx, (int) by + 40, 60, 18, juce::Justification::centredLeft);
        g.drawText ("sharp", (int) (bx + bw - 60.f), (int) by + 40, 60, 18, juce::Justification::centredRight);

        static const char* strings[6] { "E", "A", "D", "G", "B", "E" };
        for (int i = 0; i < 6; ++i)
        {
            const auto rr = juce::Rectangle<float> (r.getCentreX() - 190.f + (float) i * 64.f, 400.f, 56.f, 40.f);
            const bool nearS = has && note.startsWithIgnoreCase (strings[i]) && note.length() <= 3;
            g.setColour (nearS ? acc.withAlpha (0.25f) : juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (rr, 12.f);
            g.setColour (nearS ? txt : dimc);
            g.setFont (uiFont (18.f, juce::Font::bold));
            g.drawText (strings[i], rr.toNearestInt(), juce::Justification::centred);
        }
    }

private:
    void timerCallback() override
    {
        if (! isShowing()) return;
        const double fs = proc.sampleRateAtomic.load();
        const int N = AuroraProcessor::tunerSize;
        const int W = 2048;
        const int maxTau = juce::jmin (1100, (int) (fs / 45.0));
        const int minTau = juce::jmax (20, (int) (fs / 1200.0));
        if (W + maxTau >= N) return;

        const int pos = proc.tunerPos.load();
        buf.resize ((size_t) (W + maxTau));
        for (int i = 0; i < W + maxTau; ++i)
            buf[(size_t) i] = proc.tunerBuf[((pos - (W + maxTau) + i) % N + N) % N];

        float energy = 0.f;
        for (int i = 0; i < W; ++i) energy += buf[(size_t) i] * buf[(size_t) i];
        if (std::sqrt (energy / (float) W) < 0.004f) { freq = 0.f; repaint(); return; }

        d.assign ((size_t) maxTau + 1, 0.f);
        for (int tau = 1; tau <= maxTau; ++tau)
        {
            float s = 0.f;
            for (int j = 0; j < W; ++j) { const float df = buf[(size_t) j] - buf[(size_t) (j + tau)]; s += df * df; }
            d[(size_t) tau] = s;
        }
        float run = 0.f;
        d[0] = 1.f;
        for (int tau = 1; tau <= maxTau; ++tau) { run += d[(size_t) tau]; d[(size_t) tau] = run > 0.f ? d[(size_t) tau] * (float) tau / run : 1.f; }

        int best = -1;
        for (int tau = minTau; tau < maxTau; ++tau)
            if (d[(size_t) tau] < 0.12f)
            {
                while (tau + 1 < maxTau && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau;
                best = tau; break;
            }
        if (best < 0) { freq = 0.f; repaint(); return; }

        float t = (float) best;
        if (best > 1 && best < maxTau)
        {
            const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
            const float den = a - 2.f * b + c;
            if (std::fabs (den) > 1e-9f) t += 0.5f * (a - c) / den;
        }
        const float f = (float) fs / t;
        freq = freq > 0.f ? 0.6f * freq + 0.4f * f : f;

        const float midi = 69.f + 12.f * std::log2 (freq / 440.f);
        const int nearest = (int) std::lround (midi);
        cents = (midi - (float) nearest) * 100.f;
        static const char* names[12] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        note = juce::String (names[((nearest % 12) + 12) % 12]) + juce::String (nearest / 12 - 1);
        repaint();
    }

    AuroraProcessor& proc;
    std::vector<float> buf, d;
    float freq = 0.f, cents = 0.f;
    juce::String note;
};
