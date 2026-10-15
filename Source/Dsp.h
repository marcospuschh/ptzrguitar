#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

inline float fastTanh (float x)
{
    x = std::fmax (-3.f, std::fmin (3.f, x));
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

inline float dbToGain (float dB) { return std::pow (10.f, dB * 0.05f); }
inline float lerpf (float a, float b, float t) { return a + (b - a) * t; }

// Stereo biquad (RBJ cookbook). Coefficients are shared by both channels.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] { 0, 0 }, z2[2] { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    void identity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    float process (int ch, float x)
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }

    static constexpr float pi = 3.14159265358979f;

    void set (float nb0, float nb1, float nb2, float a0, float na1, float na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0;
        a1 = na1 / a0; a2 = na2 / a0;
    }

    static float clampF (double fs, float f) { return std::fmin (f, (float) fs * 0.45f); }

    void lowpass (double fs, float f, float q)
    {
        f = clampF (fs, f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void highpass (double fs, float f, float q)
    {
        f = clampF (fs, f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void peak (double fs, float f, float q, float dB)
    {
        f = clampF (fs, f);
        const float A = std::pow (10.f, dB / 40.f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }

    void lowShelf (double fs, float f, float q, float dB)
    {
        f = clampF (fs, f);
        const float A = std::pow (10.f, dB / 40.f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w);
        const float be = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        set (A * ((A + 1) - (A - 1) * c + be), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - be),
             (A + 1) + (A - 1) * c + be, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - be);
    }

    void highShelf (double fs, float f, float q, float dB)
    {
        f = clampF (fs, f);
        const float A = std::pow (10.f, dB / 40.f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w);
        const float be = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        set (A * ((A + 1) + (A - 1) * c + be), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - be),
             (A + 1) - (A - 1) * c + be, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - be);
    }

    // Band descriptor used by the cabinet / microphone / guitar tables.
    // type: 'H' highpass, 'L' lowpass, 'P' peak, 'l' low shelf, 'h' high shelf
    struct Band { char type; float f, q, g; };

    void setBand (double fs, const Band& b)
    {
        switch (b.type)
        {
            case 'H': highpass (fs, b.f, b.q); break;
            case 'L': lowpass (fs, b.f, b.q); break;
            case 'P': peak (fs, b.f, b.q, b.g); break;
            case 'l': lowShelf (fs, b.f, b.q, b.g); break;
            case 'h': highShelf (fs, b.f, b.q, b.g); break;
            default: identity(); break;
        }
    }
};

// Circular delay line with linear-interpolated fractional read.
struct DelayLine
{
    std::vector<float> b;
    int w = 0;

    void prepare (int size) { b.assign ((size_t) std::max (size, 8), 0.f); w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.f); w = 0; }
    int size() const { return (int) b.size(); }

    void write (float x) { b[(size_t) w] = x; if (++w >= size()) w = 0; }

    // delay measured in samples back from the most recently written sample (>= 1)
    float read (float d) const
    {
        const int sz = size();
        float rp = (float) w - d;
        while (rp < 0.f) rp += (float) sz;
        int i0 = (int) rp;
        const float fr = rp - (float) i0;
        if (i0 >= sz) i0 -= sz;
        int i1 = i0 + 1; if (i1 >= sz) i1 = 0;
        return b[(size_t) i0] * (1.f - fr) + b[(size_t) i1] * fr;
    }
};
