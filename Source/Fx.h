#pragma once
#include "Dsp.h"

//==============================================================================
// Optical-style compressor (program-dependent release), stereo-linked
//==============================================================================
struct OptoComp
{
    double fs = 48000.0;
    float p = 0.f, gr = 0.f;
    Biquad tilt;

    void prepare (double sr) { fs = sr; p = gr = 0.f; tilt.reset(); }

    // amount 0..1, tone 0..1 (0.5 = flat), blend 0..1 (1 = fully compressed)
    void process (float* L, float* R, int n, float amount, float levelDb, float attMs, float relMs,
                  float ratio, float blend, float tone)
    {
        tilt.highShelf (fs, 3500.f, 0.7f, (tone - 0.5f) * 10.f);
        const float thr = -6.f - amount * 34.f;
        const float knee = 12.f;
        const float slope = 1.f - 1.f / ratio;
        const float makeup = amount * 9.f + levelDb;
        const float kp = 1.f - std::exp (-1.f / (0.012f * (float) fs));
        const float ca = 1.f - std::exp (-1.f / (attMs * 0.001f * (float) fs));

        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            p += ((xl * xl + xr * xr) * 0.5f - p) * kp;
            const float lvl = 10.f * std::log10 (p + 1e-10f);
            const float over = lvl - thr;
            float t;
            if (over <= -knee * 0.5f) t = 0.f;
            else if (over >= knee * 0.5f) t = over * slope;
            else { const float k = over + knee * 0.5f; t = slope * k * k / (2.f * knee); }

            if (t > gr) gr += (t - gr) * ca;
            else
            {
                const float rel = relMs * (1.f + gr / 6.f);   // opto: deeper reduction recovers slower
                gr += (t - gr) * (1.f - std::exp (-1.f / (rel * 0.001f * (float) fs)));
            }

            const float g = dbToGain (makeup - gr);
            L[i] = tilt.process (0, xl * (1.f - blend) + xl * g * blend);
            R[i] = tilt.process (1, xr * (1.f - blend) + xr * g * blend);
        }
    }
};

//==============================================================================
// Pitch shifter (dual crossfaded delay taps). ratio 0.5 = octave down, 2 = octave up.
//==============================================================================
struct PitchShifter
{
    DelayLine dl;
    float win = 1000.f, phase = 0.f;

    void prepare (double fs, float windowMs)
    {
        win = (float) fs * windowMs * 0.001f;
        dl.prepare ((int) win + 16);
        phase = 0.f;
    }

    float process (float x, float ratio)
    {
        dl.write (x);
        float p2 = phase + 0.5f; if (p2 >= 1.f) p2 -= 1.f;
        const float s1 = std::sin (Biquad::pi * phase), s2 = std::sin (Biquad::pi * p2);
        const float y = dl.read (1.f + phase * win) * s1 * s1 + dl.read (1.f + p2 * win) * s2 * s2;
        phase += (1.f - ratio) / win;
        while (phase >= 1.f) phase -= 1.f;
        while (phase < 0.f) phase += 1.f;
        return y;
    }
};

struct OctavePoly
{
    PitchShifter down, up;
    void prepare (double fs) { down.prepare (fs, 90.f); up.prepare (fs, 45.f); }

    void process (float* L, float* R, int n, float dry, float sub, float upv)
    {
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            const float fx = sub * down.process (m, 0.5f) + upv * up.process (m, 2.f);
            L[i] = L[i] * dry + fx;
            R[i] = R[i] * dry + fx;
        }
    }
};

// Analog-style octave down: flip-flop frequency divider with envelope, like classic octave boxes.
struct OctaveAnalog
{
    double fs = 48000.0;
    Biquad detLP, outLP1, outLP2;
    bool pos = false, ff1 = false, ff2 = false;
    float env = 0.f;

    void prepare (double sr)
    {
        fs = sr;
        detLP.lowpass (fs, 650.f, 0.7f);
        outLP1.lowpass (fs, 500.f, 0.7f);
        outLP2.lowpass (fs, 350.f, 0.7f);
        detLP.reset(); outLP1.reset(); outLP2.reset();
        pos = ff1 = ff2 = false; env = 0.f;
    }

    void process (float* L, float* R, int n, float dry, float o1, float o2)
    {
        const float aA = 1.f - std::exp (-1.f / (0.002f * (float) fs));
        const float aR = 1.f - std::exp (-1.f / (0.040f * (float) fs));
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            const float d = detLP.process (0, m);
            const float a = std::fabs (m);
            env += (a - env) * (a > env ? aA : aR);

            if (!pos && d > 0.004f) { pos = true; ff1 = !ff1; if (ff1) ff2 = !ff2; }
            else if (pos && d < -0.004f) pos = false;

            const float s1 = outLP1.process (0, (ff1 ? 1.f : -1.f) * env);
            const float s2 = outLP2.process (0, (ff2 ? 1.f : -1.f) * env);
            const float fx = 1.4f * (o1 * s1 + o2 * s2);
            L[i] = L[i] * dry + fx;
            R[i] = R[i] * dry + fx;
        }
    }
};

//==============================================================================
// Drives (run at the oversampled rate)
//==============================================================================
// Transparent overdrive: neutral at minimum gain, keeps lows and dynamics (clean blend).
struct DriveTransparent
{
    double fs = 48000.0;
    Biquad hp, lp;

    void prepare (double sr) { fs = sr; hp.highpass (fs, 70.f, 0.7f); hp.reset(); lp.reset(); }

    void process (float* L, float* R, int n, float drive, float tone, float vol)
    {
        lp.lowpass (fs, 2200.f + tone * 7500.f, 0.7f);
        const float g = 1.f + 26.f * drive * drive;
        const float out = dbToGain (-14.f + vol * 28.f) / (1.f + 1.2f * drive);
        float* ch[2] { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                float y = hp.process (c, x) * g;
                y = y / (1.f + std::fabs (y));
                ch[c][i] = lp.process (c, 0.72f * y + 0.28f * x) * out;
            }
    }
};

// Op-amp style overdrive with bass and treble controls and a gentle mid lift.
struct DriveOpAmp
{
    double fs = 48000.0;
    Biquad hp, tb, tm, tt;

    void prepare (double sr)
    {
        fs = sr;
        hp.highpass (fs, 80.f, 0.7f);
        tm.peak (fs, 750.f, 0.8f, 2.5f);
        for (auto* b : { &hp, &tb, &tm, &tt }) b->reset();
    }

    void process (float* L, float* R, int n, float gain, float treble, float bass, float vol)
    {
        tb.lowShelf (fs, 110.f, 0.7f, (bass - 0.5f) * 10.f);
        tt.highShelf (fs, 3600.f, 0.7f, (treble - 0.5f) * 12.f);
        const float g = 1.f + 36.f * gain * gain;
        const float out = dbToGain (-14.f + vol * 28.f) / (1.f + 0.9f * gain);
        float* ch[2] { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                const float y = fastTanh (hp.process (c, x) * g * 0.9f);
                float z = 0.62f * y + 0.38f * x;
                z = tt.process (c, tb.process (c, tm.process (c, z)));
                ch[c][i] = z * out;
            }
    }
};

//==============================================================================
// Modulation: chorus (CE-1 style), vibrato (analog BBD style), tremolo
//==============================================================================
struct ModFx
{
    double fs = 48000.0;
    DelayLine dl[2];
    double phase = 0.0;
    Biquad lp;

    void prepare (double sr)
    {
        fs = sr;
        for (auto& d : dl) d.prepare ((int) (sr * 0.05) + 16);
        phase = 0.0;
        lp.lowpass (fs, 8500.f, 0.6f);
        lp.reset();
    }

    static float lfo (int shape, float p)   // returns -1..1
    {
        const float s = std::sin (6.2831853f * p);
        if (shape == 1) return 4.f * std::fabs (p - 0.5f) - 1.f;
        if (shape == 2) return fastTanh (5.f * s);
        return s;
    }

    // type: 0 chorus, 1 vibrato, 2 tremolo. shape: 0 sine, 1 triangle, 2 square
    void process (float* L, float* R, int n, int type, float rate, float depth, float mix, int shape)
    {
        const float hz = type == 0 ? 0.15f + rate * rate * 5.f
                       : type == 1 ? 0.5f + rate * rate * 9.f
                                   : 1.f + rate * rate * 14.f;
        const double inc = hz / fs;
        float* ch[2] { L, R };

        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                const float in = ch[c][i];
                float out = in;

                if (type == 2)
                {
                    const float p = (float) phase;
                    out = in * (1.f - depth * (0.5f + 0.5f * lfo (shape, p)));
                }
                else
                {
                    dl[c].write (in);
                    if (type == 0)
                    {
                        float p = (float) phase + (c ? 0.5f : 0.f); if (p >= 1.f) p -= 1.f;
                        const float base = 0.0065f * (float) fs;
                        const float dep = (0.0004f + 0.0028f * depth) * (float) fs;
                        const float wet = dl[c].read (base + dep * lfo (0, p));
                        out = in * (1.f - 0.45f * mix) + wet * 0.8f * mix;
                    }
                    else
                    {
                        float p = (float) phase + (c ? 0.25f : 0.f); if (p >= 1.f) p -= 1.f;
                        const float base = 0.004f * (float) fs;
                        const float dep = (0.0003f + 0.0030f * depth) * (float) fs;
                        out = lp.process (c, dl[c].read (base + dep * lfo (shape, p)));
                    }
                }
                ch[c][i] = out;
            }
            phase += inc;
            if (phase >= 1.0) phase -= 1.0;
        }
    }
};

//==============================================================================
// Delay: digital, analog, tape, ping-pong, dual
//==============================================================================
struct DelayFx
{
    double fs = 48000.0;
    DelayLine dl[2];
    float lpS[2] { 0.f, 0.f };
    float t1c = 0.f, t2c = 0.f;
    double ph1 = 0.0, ph2 = 0.0;

    void prepare (double sr)
    {
        fs = sr;
        for (auto& d : dl) d.prepare ((int) (sr * 2.0) + 16);
        lpS[0] = lpS[1] = 0.f;
        t1c = t2c = (float) (sr * 0.35);
    }

    static float ratioFor (int idx)
    {
        static const float r[] { 0.5f, 2.f / 3.f, 0.75f, 1.5f };
        return r[std::max (0, std::min (3, idx))];
    }

    // type: 0 digital, 1 analog, 2 tape, 3 ping-pong, 4 dual
    void process (float* L, float* R, int n, int type, float timeMs, float fb, float mix,
                  float tone, float modAmt, int ratioIdx)
    {
        const float maxT = (float) dl[0].size() - 8.f;
        const float t1 = std::fmin (std::fmax (timeMs * (float) fs * 0.001f, 32.f), maxT);
        const float t2 = std::fmin (std::fmax (t1 * ratioFor (ratioIdx), 32.f), maxT);
        const float k = 1.f - std::exp (-1.f / (0.06f * (float) fs));
        fb = std::fmin (fb, 0.92f);

        float fc = 800.f + tone * 11000.f;
        if (type == 1) fc = std::fmin (fc, 4200.f);
        if (type == 2) fc = std::fmin (fc, 5800.f);
        const float a = std::exp (-6.2831853f * fc / (float) fs);
        const bool sat = (type == 1 || type == 2);
        const float satG = type == 2 ? 1.8f : 1.5f;

        float* ch[2] { L, R };
        for (int i = 0; i < n; ++i)
        {
            t1c += (t1 - t1c) * k;
            t2c += (t2 - t2c) * k;

            float m = 0.f;
            if (type == 1 || type == 2)
            {
                ph1 += (type == 2 ? 0.55 : 0.8) / fs; if (ph1 >= 1.0) ph1 -= 1.0;
                ph2 += 5.7 / fs;                     if (ph2 >= 1.0) ph2 -= 1.0;
                const float w1 = std::sin (6.2831853f * (float) ph1);
                const float w2 = std::sin (6.2831853f * (float) ph2);
                m = type == 2 ? modAmt * (0.0012f * w1 + 0.00012f * w2) * (float) fs
                              : modAmt * 0.0018f * w1 * (float) fs;
            }

            const float d1 = std::fmax (t1c + m, 2.f);
            const float d2 = std::fmax (t2c + m * 0.9f, 2.f);

            if (type <= 2)
            {
                for (int c = 0; c < 2; ++c)
                {
                    const float in = ch[c][i];
                    const float d = dl[c].read (d1);
                    lpS[c] = d * (1.f - a) + lpS[c] * a;
                    float f = lpS[c];
                    if (sat) f = fastTanh (f * satG) / satG;
                    dl[c].write (in + f * fb);
                    ch[c][i] = in + lpS[c] * mix;
                }
            }
            else
            {
                const float inL = L[i], inR = R[i], in = 0.5f * (inL + inR);
                const float dL = dl[0].read (d1);
                const float dR = dl[1].read (type == 3 ? d1 : d2);
                lpS[0] = dL * (1.f - a) + lpS[0] * a;
                lpS[1] = dR * (1.f - a) + lpS[1] * a;
                if (type == 3)
                {
                    dl[0].write (in + fb * lpS[1]);
                    dl[1].write (fb * lpS[0]);
                }
                else
                {
                    dl[0].write (in + fb * lpS[0]);
                    dl[1].write (in + fb * lpS[1]);
                }
                L[i] = inL + lpS[0] * mix;
                R[i] = inR + lpS[1] * mix;
            }
        }
    }
};

//==============================================================================
// Reverb: 4-line feedback delay network with diffusion.
// Types: 0 Room, 1 Hall, 2 Plate, 3 Spring, 4 Shimmer (octave-up feedback), 5 Cloud (swelling ambient)
//==============================================================================
struct ReverbFx
{
    struct AP
    {
        DelayLine d; int len = 1; float g = 0.6f;
        void prepare (double fs, float ms, float gain) { len = std::max (1, (int) (fs * ms * 0.001)); d.prepare (len + 4); g = gain; }
        float process (float x)
        {
            const float del = d.read ((float) len);
            const float w = x + g * del;
            d.write (w);
            return -g * w + del;
        }
    };

    double fs = 48000.0;
    DelayLine line[4], pre;
    AP diff[4], disp[6];
    PitchShifter shim;
    float lpS[4] { 0, 0, 0, 0 };
    double ph[4] { 0, 0.25, 0.5, 0.75 };
    float swell = 0.f, lastWet = 0.f;

    void prepare (double sr)
    {
        fs = sr;
        pre.prepare ((int) (sr * 0.12) + 8);
        for (auto& l : line) l.prepare ((int) (sr * 0.2) + 16);
        const float dm[4] { 4.77f, 3.6f, 12.7f, 9.3f };
        for (int i = 0; i < 4; ++i) diff[i].prepare (sr, dm[i], 0.65f);
        const float sm[6] { 2.9f, 3.7f, 4.3f, 5.1f, 6.1f, 7.3f };
        for (int i = 0; i < 6; ++i) disp[i].prepare (sr, sm[i], 0.72f);
        shim.prepare (sr, 55.f);
        for (auto& s : lpS) s = 0.f;
        swell = lastWet = 0.f;
    }

    // decay, damp (tone), mix, preDelay, mod, fx all 0..1.
    // fx: Shimmer = octave amount, Cloud = swell time. Other types ignore it.
    void process (float* L, float* R, int n, int type, float decay, float damp, float mix,
                  float preDelay, float modAmt, float fx)
    {
        struct T { float scale, rtBase, rtSpan, damp, diffG; };
        static const T tt[] { { 0.55f, 0.25f, 1.6f, 0.35f, 0.60f },
                              { 1.45f, 0.90f, 6.0f, 0.40f, 0.65f },
                              { 0.80f, 0.50f, 4.0f, 0.15f, 0.72f },
                              { 0.42f, 0.50f, 2.2f, 0.60f, 0.70f },
                              { 1.35f, 1.20f, 7.0f, 0.30f, 0.68f },
                              { 1.70f, 1.50f, 9.0f, 0.45f, 0.75f } };
        type = std::max (0, std::min (5, type));
        const T& t = tt[type];
        const bool spring = type == 3, shimmer = type == 4, cloud = type == 5;

        static const float baseMs[4] { 29.7f, 37.1f, 41.1f, 43.7f };
        static const float rates[4] { 0.31f, 0.43f, 0.57f, 0.71f };
        const float rt60 = t.rtBase + t.rtSpan * decay * decay;
        float len[4], g[4];
        for (int i = 0; i < 4; ++i)
        {
            len[i] = baseMs[i] * t.scale * (float) fs * 0.001f;
            g[i] = std::pow (10.f, -3.f * len[i] / ((float) fs * rt60));
        }
        const float dc = std::fmin (0.95f, t.damp * 0.5f + damp * 0.7f);
        const float depth = (spring ? 0.0001f : cloud ? 0.0012f : 0.0005f) * (0.3f + modAmt * 1.7f) * (float) fs;
        const float preS = std::fmax (1.f, preDelay * 0.1f * (float) fs);
        const float swK = cloud ? 1.f - std::exp (-1.f / ((float) fs * (0.002f + fx * 0.35f))) : 1.f;
        float gAvg = 0.f;
        for (int i = 0; i < 4; ++i) gAvg += 0.25f * g[i];
        // Loop gain through the network peaks near 1/(1-g), so scale the feedback by (1-g): always < 1, decays cleanly.
        const float shimG = shimmer ? 0.6f * fx * (1.f - gAvg) : 0.f;
        for (auto& a : diff) a.g = t.diffG;
        for (auto& a : disp) a.g = spring ? 0.72f : 0.55f;

        for (int i = 0; i < n; ++i)
        {
            float x = 0.5f * (L[i] + R[i]);
            pre.write (x);
            x = pre.read (preS);
            if (cloud) { swell += (x - swell) * swK; x = swell; }
            x += lastWet * shimG;
            for (auto& a : diff) x = a.process (x);
            if (spring || cloud) for (auto& a : disp) x = a.process (x);

            float y[4];
            for (int k = 0; k < 4; ++k)
            {
                ph[k] += rates[k] / fs; if (ph[k] >= 1.0) ph[k] -= 1.0;
                y[k] = line[k].read (len[k] + depth * std::sin (6.2831853f * (float) ph[k]));
                lpS[k] = (1.f - dc) * y[k] + dc * lpS[k];
            }
            const float sum = 0.5f * (lpS[0] + lpS[1] + lpS[2] + lpS[3]);
            for (int k = 0; k < 4; ++k)
                line[k].write (0.5f * x + g[k] * (lpS[k] - sum));

            const float wl = 0.5f * (lpS[0] + lpS[2]);
            const float wr = 0.5f * (lpS[1] + lpS[3]);
            lastWet = shimmer ? shim.process (0.5f * (wl + wr), 2.f) : 0.f;
            L[i] += wl * mix;
            R[i] += wr * mix;
        }
    }
};
