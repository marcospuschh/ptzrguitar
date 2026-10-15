#pragma once
#include "Panels.h"

//==============================================================================
// Icon navigation (pedals / amp / effects)
//==============================================================================
class IconNav : public juce::Component
{
public:
    IconNav() {}

    std::function<void (int)> onChange;
    int getIndex() const { return index; }
    void setIndex (int i, bool notify = true)
    {
        if (i == index) return;
        index = i; repaint();
        if (notify && onChange) onChange (index);
    }

    void paint (juce::Graphics& g) override
    {
        static const char* names[3] { "PEDALS", "AMP", "EFFECTS" };
        const int w = getWidth() / 3;
        for (int i = 0; i < 3; ++i)
        {
            const bool sel = i == index, hov = i == hover;
            const auto col = sel ? pal::ink() : (hov ? pal::ink().withAlpha (0.8f) : pal::dim());
            const float cx = (float) (i * w + w / 2), y = 4.f;
            g.setColour (col);
            const juce::PathStrokeType st (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            if (i == 0)
            {
                g.drawRoundedRectangle (cx - 11.f, y, 22.f, 30.f, 5.f, 2.f);
                g.drawEllipse (cx - 8.f, y + 5.f, 5.f, 5.f, 1.6f);
                g.drawEllipse (cx + 3.f, y + 5.f, 5.f, 5.f, 1.6f);
                g.fillEllipse (cx - 4.f, y + 18.f, 8.f, 8.f);
            }
            else if (i == 1)
            {
                g.drawRoundedRectangle (cx - 19.f, y + 4.f, 38.f, 24.f, 5.f, 2.f);
                g.drawLine (cx - 14.f, y + 11.f, cx + 14.f, y + 11.f, 1.6f);
                for (int k = 0; k < 5; ++k) g.fillEllipse (cx - 13.f + (float) k * 6.5f, y + 18.f, 3.6f, 3.6f);
            }
            else
            {
                juce::Path w2;
                w2.startNewSubPath (cx - 18.f, y + 16.f);
                for (float x = -18.f; x <= 18.f; x += 2.f) w2.lineTo (cx + x, y + 16.f + 9.f * std::sin (x * 0.38f) * (1.f - std::fabs (x) / 24.f));
                g.strokePath (w2, st);
            }
            g.setFont (uiFont (11.f, juce::Font::bold).withExtraKerningFactor (0.14f));
            g.drawText (names[i], i * w, 40, w, 14, juce::Justification::centred);
            if (sel)
            {
                g.setColour (pal::ink());
                g.fillEllipse (cx - 2.5f, 57.f, 5.f, 5.f);
            }
        }
    }

    void mouseMove (const juce::MouseEvent& e) override { hover = juce::jlimit (0, 2, e.x / juce::jmax (1, getWidth() / 3)); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override { setIndex (juce::jlimit (0, 2, e.x / juce::jmax (1, getWidth() / 3))); }

private:
    int index = 0, hover = -1;
};

//==============================================================================
// Background: themed backdrop with a soft aurora, wordmark and strip dividers
//==============================================================================
class AuroraBackground : public juce::Component, private juce::Timer
{
public:
    AuroraBackground() { setOpaque (true); setInterceptsMouseClicks (false, false); startTimerHz (20); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        if (pal::light)
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9d3f5), 0.f, 0.f, juce::Colour (0xffa6b5e6), 0.f, b.getHeight(), false));
        else
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff07090c), 0.f, 0.f, juce::Colour (0xff0c1218), 0.f, b.getHeight(), false));
        g.fillAll();

        const float k = pal::light ? 0.55f : 1.f;
        curtain (g, b.getWidth(), 62.f, 20.f, 0.0060f, phase * 0.6f, 200.f, pal::teal, 0.20f * k);
        curtain (g, b.getWidth(), 38.f, 26.f, 0.0045f, phase * 0.4f + 2.f, 210.f, pal::violet, 0.17f * k);
        curtain (g, b.getWidth(), 86.f, 14.f, 0.0090f, -phase * 0.5f, 160.f, pal::sky, 0.08f * k);

        g.setColour (pal::ink());
        g.setFont (uiFont (30.f).withExtraKerningFactor (0.06f));
        g.drawText ("Aurora", 26, 6, 240, 36, juce::Justification::centredLeft);
        g.setColour (pal::dim());
        g.setFont (uiFont (11.5f, juce::Font::bold).withExtraKerningFactor (0.14f));
        g.drawText ("GUITAR STUDIO", 28, 40, 240, 16, juce::Justification::centredLeft);

        g.setColour (pal::overlay (0.16f));
        for (float x : { 310.f, 460.f, 900.f }) g.drawVerticalLine ((int) x, 76.f, 150.f);
        g.setColour (pal::overlay (0.07f));
        g.fillRect (0.f, (float) getHeight() - 54.f - 8.f, (float) getWidth(), 1.f);
    }

private:
    void curtain (juce::Graphics& g, float w, float y0, float amp, float freq, float ph, float height,
                  juce::Colour c, float alpha)
    {
        auto edge = [&] (float x) { return y0 + amp * std::sin (x * freq + ph) + amp * 0.4f * std::sin (x * freq * 2.3f - ph * 1.3f); };
        juce::Path p;
        p.startNewSubPath (0.f, edge (0.f));
        for (float x = 8.f; x <= w; x += 8.f) p.lineTo (x, edge (x));
        p.lineTo (w, edge (w) + height);
        p.lineTo (0.f, edge (0.f) + height);
        p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (c.withAlpha (alpha), 0.f, y0 - amp, c.withAlpha (0.f), 0.f, y0 + height, false));
        g.fillPath (p);
    }

    void timerCallback() override
    {
        phase += 0.04f;
        if (isShowing()) repaint (0, 0, getWidth(), 260);
    }

    float phase = 0.f;
};

//==============================================================================
// Pages
//==============================================================================
class PedalsPage : public juce::Component
{
public:
    explicit PedalsPage (AuroraProcessor& p)
    {
        auto add = [&] (const char* name, int pattern, const char* on, juce::uint32 body, std::vector<KnobSpec> k)
        {
            pedals.push_back (std::make_unique<PedalView> (p, name, pattern, on, juce::Colour (body), k));
            addAndMakeVisible (*pedals.back());
        };
        add ("Opto 1", 0, "comp1On", 0xffede7da,
             { { "c1Sus", "Sustain", "%", 0, 100.0 }, { "c1Lvl", "Level", " dB", 1, 1.0 }, { "c1Tone", "Tone", "%", 0, 100.0 } });
        add ("Opto 2", 1, "comp2On", 0xffdde4d3,
             { { "c2Comp", "Compress", "%", 0, 100.0 }, { "c2Lvl", "Level", " dB", 1, 1.0 }, { "c2Blend", "Blend", "%", 0, 100.0 } });
        add ("Octave 1", 2, "oct1On", 0xffdedaea,
             { { "o1Dry", "Dry", "%", 0, 100.0 }, { "o1Sub", "Sub", "%", 0, 100.0 }, { "o1Up", "Up", "%", 0, 100.0 } });
        add ("Octave 2", 3, "oct2On", 0xffecd7d3,
             { { "o2Dry", "Dry", "%", 0, 100.0 }, { "o2Oct1", "Oct 1", "%", 0, 100.0 }, { "o2Oct2", "Oct 2", "%", 0, 100.0 } });
        add ("Drive 1", 4, "drv1On", 0xffe4cca2,
             { { "d1Drive", "Drive", "", 1, 1.0 }, { "d1Tone", "Tone", "", 1, 1.0 }, { "d1Vol", "Volume", "", 1, 1.0 } });
        add ("Drive 2", 5, "drv2On", 0xffd8c3ae,
             { { "d2Gain", "Gain", "", 1, 1.0 }, { "d2Treble", "Treble", "", 1, 1.0 }, { "d2Bass", "Bass", "", 1, 1.0 }, { "d2Vol", "Volume", "", 1, 1.0 } });
    }

    void resized() override
    {
        const int pw = 184;
        const int total = (int) pedals.size() * pw;
        int x = (getWidth() - total) / 2;
        for (auto& p : pedals) { p->setBounds (x, 0, pw, getHeight() - 8); x += pw; }
    }

private:
    std::vector<std::unique_ptr<PedalView>> pedals;
};

class AmpPage : public juce::Component
{
public:
    explicit AmpPage (AuroraProcessor& p)
        : ampA (p, "a_", 0), ampB (p, "b_", 1), tabs ({ "Amp A", "Amp B" }, pal::amber, pal::orange),
          dualAtt (p.apvts, "dual", dualBtn),
          dualWatch (*p.apvts.getParameter ("dual"), [this] (float v) { setDual (v > 0.5f); })
    {
        addAndMakeVisible (ampA);
        addAndMakeVisible (ampB);
        setAccent (dualBtn, pal::amber, pal::orange);
        dualBtn.setButtonText ("Dual amp");
        addAndMakeVisible (dualBtn);
        addAndMakeVisible (tabs);
        tabs.onChange = [this] (int) { showSelected(); };
        dualWatch.sendInitialUpdate();
    }

    void resized() override
    {
        ampA.setBounds (0, 0, AmpPanel::W, AmpPanel::H);
        ampB.setBounds (0, 0, AmpPanel::W, AmpPanel::H);
        dualBtn.setBounds (1020, 2, 118, 28);
        tabs.setBounds (780, 0, 220, 32);
    }

    // Small segmented control for Amp A / Amp B
    class Tabs : public juce::Component
    {
    public:
        Tabs (const juce::StringArray& n, juce::Colour a, juce::Colour b) : names (n), c1 (a), c2 (b) {}
        std::function<void (int)> onChange;
        int getIndex() const { return index; }
        void setIndex (int i, bool notify = true) { if (i == index) return; index = i; repaint(); if (notify && onChange) onChange (index); }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            g.setColour (pal::overlay (0.09f));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            for (int i = 0; i < names.size(); ++i)
            {
                auto cr = juce::Rectangle<float> ((float) i * r.getWidth() / (float) names.size(), 0.f, r.getWidth() / (float) names.size(), r.getHeight()).reduced (3.f);
                if (i == index)
                {
                    g.setGradientFill (juce::ColourGradient (c1, cr.getX(), 0.f, c2, cr.getRight(), 0.f, false));
                    g.fillRoundedRectangle (cr, cr.getHeight() * 0.5f);
                    g.setColour (pal::onAccent());
                }
                else g.setColour (pal::dim());
                g.setFont (uiFont (13.f, juce::Font::bold));
                g.drawText (names[i], cr.toNearestInt(), juce::Justification::centred);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override { setIndex (juce::jlimit (0, names.size() - 1, e.x * names.size() / juce::jmax (1, getWidth()))); }
    private:
        juce::StringArray names; juce::Colour c1, c2; int index = 0;
    };

private:
    void setDual (bool d)
    {
        dual = d;
        ampA.setDual (d); ampB.setDual (d);
        tabs.setVisible (d);
        if (! d) tabs.setIndex (0, false);
        showSelected();
    }
    void showSelected()
    {
        const bool b = dual && tabs.getIndex() == 1;
        ampA.setVisible (! b);
        ampB.setVisible (b);
    }

    AmpPanel ampA, ampB;
    Tabs tabs;
    juce::ToggleButton dualBtn;
    juce::AudioProcessorValueTreeState::ButtonAttachment dualAtt;
    juce::ParameterAttachment dualWatch;
    bool dual = false;
};

class ReverbCard : public FxCard
{
public:
    explicit ReverbCard (AuroraProcessor& p)
        : FxCard (p, "Reverb", "Six BigSky-style spaces", "revOn", "revPos", "revType", 3, pal::teal, pal::violet,
                  { { "revDecay", "Decay", "%", 0, 100.0, 0 }, { "revDamp", "Tone", "%", 0, 100.0, 0 },
                    { "revPre", "Pre-delay", " ms", 0, 100.0, 0 }, { "revMod", "Mod", "%", 0, 100.0, 0 },
                    { "revMix", "Mix", "%", 0, 100.0, 0 }, { "revFx", "Amount", "%", 0, 100.0, 48, "Shimmer", 4 } })
    { init(); }
};

class FxPage : public juce::Component
{
public:
    explicit FxPage (AuroraProcessor& p) : mod (p), delay (p), reverb (p)
    {
        addAndMakeVisible (mod); addAndMakeVisible (delay); addAndMakeVisible (reverb);
    }
    void resized() override
    {
        const int gap = 20, h = getHeight();
        mod.setBounds (0, 0, 340, h);
        delay.setBounds (340 + gap, 0, 400, h);
        reverb.setBounds (740 + 2 * gap, 0, getWidth() - 740 - 2 * gap, h);
    }
private:
    ModCard mod;
    DelayCard delay;
    ReverbCard reverb;
};

//==============================================================================
// Bottom bar: tuner, tap tempo, theme
//==============================================================================
class BottomBar : public juce::Component
{
public:
    explicit BottomBar (AuroraProcessor& p)
        : apvts (p.apvts),
          tunerWatch (*p.apvts.getParameter ("tunerOn"), [this] (float v) { tunerOn = v > 0.5f; repaint(); }),
          timeWatch (*p.apvts.getParameter ("dlyTime"), [this] (float ms) { bpm = 60000.f / ms; repaint(); })
    {
        tunerWatch.sendInitialUpdate();
        timeWatch.sendInitialUpdate();
    }

    std::function<void()> onTheme;

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff101216));
        g.fillRect (getLocalBounds());
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawHorizontalLine (0, 0.f, (float) getWidth());

        const auto font = uiFont (12.f, juce::Font::bold).withExtraKerningFactor (0.14f);
        g.setFont (font);
        hits.clear();
        float x = 30.f;
        auto item = [&] (const juce::String& txt, int id, bool active, juce::Colour col)
        {
            const float w = textW (font, txt) + 4.f;
            juce::Rectangle<float> r (x - 8.f, 0.f, w + 16.f, (float) getHeight());
            hits.push_back ({ r, id });
            g.setColour (active ? pal::teal : col);
            g.drawText (txt, (int) x, 0, (int) w + 2, getHeight(), juce::Justification::centredLeft);
            if (active) g.fillRect (x, (float) getHeight() - 14.f, w, 2.f);
            x += w + 30.f;
        };
        const juce::Colour base (0xffc4cad2);
        item ("TUNER", 0, tunerOn, base);
        item ("TAP", 1, false, base);
        item (juce::String (bpm, 1) + " BPM", -1, false, base.withAlpha (0.65f));
        item (pal::light ? "THEME: LIGHT" : "THEME: DARK", 2, false, base);

        g.setColour (base.withAlpha (0.55f));
        g.drawText ("AURORA GUITAR STUDIO", getWidth() - 320, 0, 290, getHeight(), juce::Justification::centredRight);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (auto& h : hits)
            if (h.r.contains (e.position))
            {
                if (h.id == 0)
                {
                    auto* prm = apvts.getParameter ("tunerOn");
                    prm->beginChangeGesture(); prm->setValueNotifyingHost (tunerOn ? 0.f : 1.f); prm->endChangeGesture();
                }
                else if (h.id == 1) doTap();
                else if (h.id == 2 && onTheme) onTheme();
                return;
            }
    }

private:
    struct Hit { juce::Rectangle<float> r; int id; };

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

    juce::AudioProcessorValueTreeState& apvts;
    juce::ParameterAttachment tunerWatch, timeWatch;
    bool tunerOn = false;
    float bpm = 143.f;
    std::vector<double> taps;
    std::vector<Hit> hits;
};

//==============================================================================
// Content (fixed 1180x800 layout, scaled by the editor)
//==============================================================================
class AuroraContent : public juce::Component
{
public:
    static constexpr int W = 1180, H = 800;

    explicit AuroraContent (AuroraProcessor& p)
        : guitar (p.apvts, "guitar", "Guitar"),
          inMode (p.apvts, "inMode", 1, 12.5f),
          tunerWatch (*p.apvts.getParameter ("tunerOn"), [this] (float v) { tunerView.setVisible (v > 0.5f); if (v > 0.5f) tunerView.toFront (false); }),
          chain (p), pedalsPage (p), ampPage (p), fxPage (p), tunerView (p), bar (p),
          kIn (p.apvts, "inGain", "Input", " dB", 1),
          kGate (p.apvts, "gate", "Gate", " dB", 0),
          kOut (p.apvts, "outGain", "Output", " dB", 1),
          meterL (p), meterR (p),
          gateLed ([&p] { return p.gateOpen.load() > 0.5f; })
    {
        addAndMakeVisible (bg);
        addAndMakeVisible (nav);
        addAndMakeVisible (guitar);
        inMode.setAccent (pal::teal, pal::violet);
        addAndMakeVisible (inMode);
        addAndMakeVisible (chain);
        addAndMakeVisible (pedalsPage); addAndMakeVisible (ampPage); addAndMakeVisible (fxPage);
        addChildComponent (tunerView);
        for (auto* k : { &kIn, &kGate, &kOut }) addAndMakeVisible (*k);
        addAndMakeVisible (meterL); addAndMakeVisible (meterR);
        addAndMakeVisible (gateLed);
        addAndMakeVisible (bar);

        nav.onChange = [this] (int) { showPage(); };
        chain.onNavigate = [this] (int t) { if (t >= 0) nav.setIndex (t); };
        bar.onTheme = [this] { pal::light = ! pal::light; repaintAll (*this); };
        nav.setIndex (1, false);
        showPage();
        tunerWatch.sendInitialUpdate();
        setSize (W, H);
    }

    void resized() override
    {
        bg.setBounds (getLocalBounds());
        nav.setBounds (450, 4, 300, 66);
        meterL.setBounds (22, 78, 12, 72);
        meterR.setBounds (1146, 78, 12, 72);
        kIn.setBounds (54, 62, 104, 96);
        kGate.setBounds (172, 62, 104, 96);
        gateLed.setBounds (262, 66, 18, 18);
        inMode.setBounds (334, 86, 104, 54);
        guitar.setBounds (484, 86, 390, 54);
        kOut.setBounds (1010, 62, 104, 96);

        chain.setBounds (20, 166, 1140, 52);

        const juce::Rectangle<int> page (20, 222, 1140, 518);
        pedalsPage.setBounds (page); ampPage.setBounds (page); fxPage.setBounds (page);
        tunerView.setBounds (page);
        bar.setBounds (0, H - 54, W, 54);
    }

private:
    void showPage()
    {
        const int t = nav.getIndex();
        pedalsPage.setVisible (t == 0);
        ampPage.setVisible (t == 1);
        fxPage.setVisible (t == 2);
    }

    AuroraBackground bg;
    IconNav nav;
    ChoiceButton guitar;
    ChipSelector inMode;
    juce::ParameterAttachment tunerWatch;
    ChainStrip chain;
    PedalsPage pedalsPage;
    AmpPage ampPage;
    FxPage fxPage;
    TunerView tunerView;
    BottomBar bar;
    Knob kIn, kGate, kOut;
    LevelMeter meterL, meterR;
    Led gateLed;
};

//==============================================================================
class AuroraEditor : public juce::AudioProcessorEditor
{
public:
    explicit AuroraEditor (AuroraProcessor& p) : AudioProcessorEditor (p), content (p)
    {
        setLookAndFeel (&laf);
        addAndMakeVisible (content);
        setResizable (true, true);
        setResizeLimits (708, 480, 1770, 1200);
        getConstrainer()->setFixedAspectRatio ((double) AuroraContent::W / AuroraContent::H);
        setSize (AuroraContent::W, AuroraContent::H);
    }

    ~AuroraEditor() override { setLookAndFeel (nullptr); }

    void resized() override
    {
        content.setBounds (0, 0, AuroraContent::W, AuroraContent::H);
        content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) AuroraContent::W));
    }

private:
    AuroraLookAndFeel laf;   // must outlive `content`
    AuroraContent content;
};
