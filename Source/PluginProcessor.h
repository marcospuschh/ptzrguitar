#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "Dsp.h"
#include "Fx.h"
#include "Amp.h"

#define AURORA_PARAMS(X) \
    X(inMode) X(guitar) X(inGain) X(gate) X(outGain) X(tunerOn) X(dual) \
    X(comp1On) X(c1Sus) X(c1Lvl) X(c1Tone) \
    X(comp2On) X(c2Comp) X(c2Lvl) X(c2Blend) \
    X(oct1On) X(o1Dry) X(o1Sub) X(o1Up) \
    X(oct2On) X(o2Dry) X(o2Oct1) X(o2Oct2) \
    X(drv1On) X(d1Drive) X(d1Tone) X(d1Vol) \
    X(drv2On) X(d2Gain) X(d2Treble) X(d2Bass) X(d2Vol) \
    X(a_model) X(a_gain) X(a_bass) X(a_mid) X(a_treble) X(a_pres) X(a_master) X(a_cab) X(a_mic) X(a_micPos) X(a_level) X(a_pan) \
    X(b_model) X(b_gain) X(b_bass) X(b_mid) X(b_treble) X(b_pres) X(b_master) X(b_cab) X(b_mic) X(b_micPos) X(b_level) X(b_pan) \
    X(modOn) X(modPos) X(modType) X(modRate) X(modDepth) X(modMix) X(modShape) \
    X(dlyOn) X(dlyPos) X(dlyType) X(dlyTime) X(dlyFb) X(dlyMix) X(dlyTone) X(dlyMod) X(dlyRatio) \
    X(revOn) X(revPos) X(revType) X(revDecay) X(revDamp) X(revMix) X(revPre) X(revMod) X(revFx)

struct PF
{
    std::atomic<float>* p = nullptr;
    float operator()() const { return p->load(); }
    int i() const { return (int) std::lround (p->load()); }
    bool on() const { return p->load() > 0.5f; }
};

class AuroraProcessor : public juce::AudioProcessor
{
public:
    AuroraProcessor();
    ~AuroraProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Aurora Guitar Studio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // slot 0 = amp A, 1 = amp B. Message thread only.
    void loadIR (int slot, const juce::File& file);
    bool hasIR (int slot) const { return slot == 0 ? ampA.hasIR() : ampB.hasIR(); }
    juce::String getIRName (int slot) const
    {
        const auto& s = irPath[slot];
        return s.isEmpty() ? juce::String() : juce::File (s).getFileNameWithoutExtension();
    }

    juce::AudioProcessorValueTreeState apvts;

    // Read by the GUI
    std::atomic<float> meterPeak[2] { 0.f, 0.f };
    std::atomic<float> gateOpen { 1.f };
    static constexpr int tunerSize = 4096;
    float tunerBuf[tunerSize] {};
    std::atomic<int> tunerPos { 0 };
    std::atomic<double> sampleRateAtomic { 44100.0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    AmpParams readAmp (bool b) const;
    void runFx (int which, float* L, float* R, int n);
    void processChunk (float* L, float* R, int n);

    struct Params
    {
       #define X(n) PF n;
        AURORA_PARAMS(X)
       #undef X
    } P;

    double fs = 44100.0;
    int maxBlock = 512;

    // gate / input
    float gEnv = 0.f, gGain = 1.f, lastOut = 1.f;
    Biquad vLs, vPk, vLp;
    int lastGuitar = -1;

    // pedals
    OptoComp comp1, comp2;
    OctavePoly oct1;
    OctaveAnalog oct2;
    DriveTransparent drive1;
    DriveOpAmp drive2;
    juce::dsp::Oversampling<float> osDrv { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };

    // amps
    AmpVoice ampA, ampB;
    juce::AudioBuffer<float> bufA, bufB;
    juce::String irPath[2];

    // effects
    ModFx mod;
    DelayFx dly;
    ReverbFx rev;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraProcessor)
};
