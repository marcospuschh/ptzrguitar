#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"

//==============================================================================
// Model data
//==============================================================================
struct AmpModel
{
    const char* name;
    const char* sub;
    const char* label[6];      // gain, bass, mid, treble, presence, master
    bool uses[6];
    float g1Min, g1Max, g2Min, g2Max;
    float bias, sag, preHP, trim;
    float adjDb, compDb;   // output calibration: level at default settings, and how much to flatten level vs gain
};

inline const AmpModel& ampModel (int i)
{
    static const AmpModel m[4] = {
        { "Silverface", "American clean",
          { "Volume", "Bass", "Middle", "Treble", "Presence", "Master" }, { true, true, true, true, true, true },
          0.8f, 7.f, 1.f, 3.f, 0.08f, 0.08f, 40.f, 1.0f, -1.7f, 12.f },
        { "Class A 30", "British chime",
          { "Gain", "Bass", "Mid", "Treble", "Cut", "Master" }, { true, true, false, true, true, true },
          1.f, 16.f, 1.5f, 6.f, 0.32f, 0.32f, 60.f, 0.9f, -12.2f, 14.f },
        { "Boutique", "Class A boutique",
          { "Gain", "Bass", "Mid", "Treble", "Cut", "Master" }, { true, true, true, true, true, true },
          1.f, 26.f, 1.5f, 8.f, 0.22f, 0.24f, 55.f, 0.85f, -15.5f, 12.f },
        { "British Crunch", "Orange-style crunch",
          { "Gain", "Bass", "Mid", "Shape", "Presence", "Volume" }, { true, false, false, true, false, true },
          2.f, 40.f, 3.f, 12.f, 0.10f, 0.18f, 100.f, 0.8f, -19.3f, 6.f } };
    return m[std::max (0, std::min (3, i))];
}

inline const char* const cabNames[5] = { "1x12 open", "2x12 Class A", "4x12 closed", "1x12 boutique", "Custom IR" };
inline const char* const micNames[5] = { "Dynamic 57", "Dynamic 421", "Ribbon 121", "Condenser 87", "Tube condenser" };
inline const char* const guitarNames[6] = { "Direct", "Strat (single-coil)", "Tele", "Les Paul (humbucker)", "P-90", "Semi-hollow" };

inline const Biquad::Band* cabBands (int i)
{
    static const Biquad::Band t[4][6] = {
        { { 'H', 85, .7f, 0 }, { 'P', 130, 1.f, 3 }, { 'P', 450, .9f, -4 }, { 'P', 2200, 1.1f, 3 }, { 'L', 5200, .7f, 0 }, { 'L', 7000, .7f, 0 } },
        { { 'H', 95, .7f, 0 }, { 'P', 160, 1.f, 2 }, { 'P', 900, .8f, -2 }, { 'P', 2800, 1.2f, 5 }, { 'P', 5500, 2.f, 2 }, { 'L', 7500, .7f, 0 } },
        { { 'H', 75, .8f, 0 }, { 'P', 110, 1.1f, 4 }, { 'P', 700, .9f, 2 }, { 'P', 2500, 1.3f, 4 }, { 'L', 5200, .8f, 0 }, { 'L', 5800, .7f, 0 } },
        { { 'H', 80, .7f, 0 }, { 'P', 120, 1.f, 3 }, { 'P', 1000, .8f, 2 }, { 'P', 3200, 1.f, 2 }, { 'L', 6200, .7f, 0 }, { 'L', 9000, .7f, 0 } } };
    return t[std::max (0, std::min (3, i))];
}

inline const Biquad::Band* micBands (int i)
{
    static const Biquad::Band t[5][4] = {
        { { 'l', 120, .7f, 1 },   { 'P', 1400, 1.f, 1.5f }, { 'P', 5000, 1.3f, 4 },  { 'L', 9500, .7f, 0 } },
        { { 'l', 100, .7f, 3 },   { 'P', 500, 1.f, -2 },    { 'P', 3500, 1.f, 3 },   { 'L', 10500, .7f, 0 } },
        { { 'l', 150, .7f, 4 },   { 'P', 1000, .8f, 0 },    { 'P', 4000, 1.f, -3 },  { 'L', 7000, .7f, 0 } },
        { { 'l', 80, .7f, 0 },    { 'P', 3000, .8f, 2 },    { 'P', 9000, 1.f, 3 },   { 'L', 14000, .7f, 0 } },
        { { 'l', 120, .7f, 2.5f },{ 'P', 700, .8f, 1.5f },  { 'P', 6500, 1.f, -2 },  { 'L', 11000, .7f, 0 } } };
    return t[std::max (0, std::min (4, i))];
}

struct GuitarVoicing { Biquad::Band ls, pk, lp; float trimDb; };

inline const GuitarVoicing& guitarVoicing (int i)
{
    static const GuitarVoicing g[6] = {
        { { 'l', 180, .7f, 0 },  { 'P', 3000, 1.f, 0 },   { 'L', 18000, .7f, 0 }, 0.f },
        { { 'l', 180, .7f, -2 }, { 'P', 4200, 1.2f, 3.5f }, { 'L', 12000, .7f, 0 }, -1.f },
        { { 'l', 200, .7f, -3 }, { 'P', 3300, 1.f, 4.5f }, { 'L', 11000, .7f, 0 }, -1.f },
        { { 'l', 150, .7f, 2.5f },{ 'P', 2300, .9f, 2.5f }, { 'L', 7000, .7f, 0 },  3.f },
        { { 'l', 160, .7f, 1 },  { 'P', 2800, 1.f, 3.f },  { 'L', 8500, .7f, 0 },  1.5f },
        { { 'l', 140, .7f, 3 },  { 'P', 1900, .8f, 1.5f }, { 'L', 6000, .7f, 0 },  1.f } };
    return g[std::max (0, std::min (5, i))];
}

struct AmpParams
{
    int model = 0, cab = 0, mic = 0;
    float gain = 5, bass = 5, mid = 5, treble = 5, pres = 5, master = 5, micPos = 0.2f;
};

//==============================================================================
// One amp: preamp -> tone stack -> power amp (4x oversampled) -> cabinet + mic
//==============================================================================
class AmpVoice
{
public:
    AmpVoice() : os (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false) {}

    void prepare (double sr, int maxBlock)
    {
        fs = sr;
        const double osFs = sr * 4.0;
        os.initProcessing ((size_t) maxBlock);
        os.reset();
        dc.highpass (osFs, 18.f, 0.7f);
        miller.lowpass (osFs, 7500.f, 0.6f);
        for (auto* b : { &hpPre, &dc, &miller, &ts[0], &ts[1], &ts[2], &ts[3], &ts[4], &posLP })
            b->reset();
        for (auto& b : cabB) b.reset();
        for (auto& b : micB) b.reset();
        sagEnv[0] = sagEnv[1] = 0.f;
        lastCab = lastMic = -1;
        juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 2 };
        conv.prepare (spec);
        conv.reset();
    }

    float getLatency() const { return os.getLatencyInSamples(); }

    void loadIR (const juce::File& f)
    {
        conv.loadImpulseResponse (f, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes,
                                  0, juce::dsp::Convolution::Normalise::yes);
        irLoaded = true;
    }
    bool hasIR() const { return irLoaded.load(); }

    void process (float* L, float* R, int n, const AmpParams& p)
    {
        const AmpModel& m = ampModel (p.model);
        const double osFs = fs * 4.0;

        const float gn = p.gain * 0.1f;
        const float g1 = lerpf (m.g1Min, m.g1Max, std::pow (gn, 1.6f));
        const float g2 = lerpf (m.g2Min, m.g2Max, gn);
        const float preGain = 0.25f + p.master * 0.2f;
        const float bias = m.bias, biasOut = fastTanh (bias);
        const float envCoef = std::exp (-1.f / (0.06f * (float) osFs));
        const float sag = m.sag;
        const float trim = m.trim * dbToGain (m.adjDb - m.compDb * (gn - 0.5f));

        hpPre.highpass (osFs, m.preHP, 0.7f);
        setupTone (p, osFs);

        float* chans[2] { L, R };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
        auto up = os.processSamplesUp (block);
        const int mm = (int) up.getNumSamples();

        for (int c = 0; c < 2; ++c)
        {
            float* d = up.getChannelPointer ((size_t) c);
            for (int i = 0; i < mm; ++i)
            {
                float x = hpPre.process (c, d[i]);

                // preamp stage 1: asymmetric triode-style clipping
                x = fastTanh (x * g1 + bias) - biasOut;
                x = dc.process (c, x);
                x = miller.process (c, x);

                for (auto& t : ts) x = t.process (c, x);

                // preamp stage 2
                x = fastTanh (x * g2 * 0.6f);

                // power amp with supply sag
                sagEnv[c] = envCoef * sagEnv[c] + (1.f - envCoef) * std::fabs (x);
                x *= preGain / (1.f + sag * 3.f * sagEnv[c]);
                x = fastTanh (x * 1.1f + bias * 0.4f) - fastTanh (bias * 0.4f);

                d[i] = x * trim;
            }
        }
        os.processSamplesDown (block);

        // cabinet + microphone
        if (p.cab == 4)
        {
            if (irLoaded.load())
            {
                juce::dsp::ProcessContextReplacing<float> ctx (block);
                conv.process (ctx);
            }
        }
        else
        {
            if (p.cab != lastCab || p.mic != lastMic || std::fabs (p.micPos - lastPos) > 0.001f)
            {
                const auto* cb = cabBands (p.cab);
                for (int k = 0; k < 6; ++k) cabB[k].setBand (fs, cb[k]);
                const auto* mb = micBands (p.mic);
                for (int k = 0; k < 4; ++k) micB[k].setBand (fs, mb[k]);
                posLP.lowpass (fs, 14000.f * std::pow (3000.f / 14000.f, p.micPos), 0.7f);
                lastCab = p.cab; lastMic = p.mic; lastPos = p.micPos;
            }
            for (int c = 0; c < 2; ++c)
            {
                float* d = chans[c];
                for (int i = 0; i < n; ++i)
                {
                    float x = d[i];
                    for (auto& b : cabB) x = b.process (c, x);
                    for (auto& b : micB) x = b.process (c, x);
                    d[i] = posLP.process (c, x) * 1.3f;
                }
            }
        }
    }

private:
    void setupTone (const AmpParams& p, double osFs)
    {
        for (auto& t : ts) t.identity();
        switch (p.model)
        {
            case 0:  // Silverface: scooped mids, bright top
                ts[0].lowShelf  (osFs, 110.f, .7f, (p.bass - 5.f) * 2.4f);
                ts[1].peak      (osFs, 450.f, .8f, -5.f + (p.mid - 5.f) * 2.f);
                ts[2].highShelf (osFs, 2800.f, .7f, (p.treble - 5.f) * 2.8f + 2.f);
                ts[3].highShelf (osFs, 5500.f, .7f, (p.pres - 5.f) * 2.f);
                break;
            case 1:  // Class A 30: fixed upper-mid chime, Top Boost treble, Cut control
                ts[0].lowShelf  (osFs, 120.f, .7f, (p.bass - 5.f) * 2.f);
                ts[1].peak      (osFs, 1800.f, .9f, 4.5f);
                ts[2].highShelf (osFs, 3200.f, .7f, (p.treble - 5.f) * 2.6f);
                ts[3].lowpass   (osFs, 9500.f - p.pres * 650.f, .6f);
                ts[4].peak      (osFs, 300.f, .7f, -2.f);
                break;
            case 2:  // Boutique: forward, smooth mids
                ts[0].lowShelf  (osFs, 100.f, .7f, (p.bass - 5.f) * 2.2f);
                ts[1].peak      (osFs, 800.f, .8f, (p.mid - 5.f) * 2.2f + 2.5f);
                ts[2].highShelf (osFs, 3000.f, .7f, (p.treble - 5.f) * 2.4f);
                ts[3].lowpass   (osFs, 10000.f - p.pres * 620.f, .6f);
                break;
            default: // British crunch: one "Shape" control (tilt) plus fixed mid hump
                ts[0].lowShelf  (osFs, 100.f, .7f, -(p.treble - 5.f) * 2.2f + 1.5f);
                ts[1].peak      (osFs, 650.f, .9f, 5.f);
                ts[2].highShelf (osFs, 3200.f, .7f, (p.treble - 5.f) * 2.6f);
                ts[3].lowpass   (osFs, 6500.f, .7f);
                break;
        }
    }

    double fs = 48000.0;
    juce::dsp::Oversampling<float> os;
    Biquad hpPre, dc, miller, ts[5], posLP, cabB[6], micB[4];
    float sagEnv[2] { 0.f, 0.f };
    int lastCab = -1, lastMic = -1;
    float lastPos = -1.f;
    juce::dsp::Convolution conv;
    std::atomic<bool> irLoaded { false };
};
