#include "PluginProcessor.h"
#include "PluginEditor.h"

using juce::StringArray;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AuroraProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    auto f = [&] (const juce::String& id, const juce::String& name, float lo, float hi, float def)
    { l.add (std::make_unique<F> (juce::ParameterID { id, 1 }, name, R (lo, hi, 0.001f), def)); };
    auto b = [&] (const juce::String& id, const juce::String& name, bool def)
    { l.add (std::make_unique<B> (juce::ParameterID { id, 1 }, name, def)); };
    auto c = [&] (const juce::String& id, const juce::String& name, StringArray s, int def)
    { l.add (std::make_unique<C> (juce::ParameterID { id, 1 }, name, s, def)); };

    // global
    c ("inMode", "Input mode", { "Mono", "Stereo" }, 0);
    c ("guitar", "Guitar", StringArray (guitarNames, 6), 0);
    f ("inGain", "Input", -24.f, 24.f, 0.f);
    f ("gate", "Gate", -90.f, -30.f, -75.f);
    f ("outGain", "Output", -24.f, 12.f, 0.f);
    b ("tunerOn", "Tuner", false);
    b ("dual", "Dual amp", false);

    // pedals
    b ("comp1On", "Opto 1 on", false);  f ("c1Sus", "Opto 1 sustain", 0.f, 1.f, 0.45f);
    f ("c1Lvl", "Opto 1 level", -12.f, 12.f, 0.f);   f ("c1Tone", "Opto 1 tone", 0.f, 1.f, 0.5f);
    b ("comp2On", "Opto 2 on", false);  f ("c2Comp", "Opto 2 compress", 0.f, 1.f, 0.5f);
    f ("c2Lvl", "Opto 2 level", -12.f, 12.f, 0.f);   f ("c2Blend", "Opto 2 blend", 0.f, 1.f, 0.8f);
    b ("oct1On", "Octave 1 on", false); f ("o1Dry", "Octave 1 dry", 0.f, 1.f, 1.f);
    f ("o1Sub", "Octave 1 sub", 0.f, 1.f, 0.5f);     f ("o1Up", "Octave 1 up", 0.f, 1.f, 0.f);
    b ("oct2On", "Octave 2 on", false); f ("o2Dry", "Octave 2 dry", 0.f, 1.f, 1.f);
    f ("o2Oct1", "Octave 2 oct 1", 0.f, 1.f, 0.5f);  f ("o2Oct2", "Octave 2 oct 2", 0.f, 1.f, 0.f);
    b ("drv1On", "Drive 1 on", false);  f ("d1Drive", "Drive 1 drive", 0.f, 10.f, 4.f);
    f ("d1Tone", "Drive 1 tone", 0.f, 10.f, 5.f);    f ("d1Vol", "Drive 1 volume", 0.f, 10.f, 5.f);
    b ("drv2On", "Drive 2 on", false);  f ("d2Gain", "Drive 2 gain", 0.f, 10.f, 3.f);
    f ("d2Treble", "Drive 2 treble", 0.f, 10.f, 5.f); f ("d2Bass", "Drive 2 bass", 0.f, 10.f, 5.f);
    f ("d2Vol", "Drive 2 volume", 0.f, 10.f, 5.f);

    // amps
    for (const char* pre : { "a_", "b_" })
    {
        const bool isB = pre[0] == 'b';
        const juce::String s (pre), nm = isB ? "Amp B " : "Amp A ";
        c (s + "model", nm + "model", { "Silverface", "Class A 30", "Boutique", "British Crunch" }, isB ? 1 : 2);
        f (s + "gain", nm + "gain", 0.f, 10.f, 5.f);
        f (s + "bass", nm + "bass", 0.f, 10.f, 5.f);
        f (s + "mid", nm + "mid", 0.f, 10.f, 5.f);
        f (s + "treble", nm + "treble", 0.f, 10.f, 5.f);
        f (s + "pres", nm + "presence/cut", 0.f, 10.f, 4.f);
        f (s + "master", nm + "master", 0.f, 10.f, 5.f);
        c (s + "cab", nm + "cabinet", StringArray (cabNames, 5), isB ? 1 : 2);
        c (s + "mic", nm + "microphone", StringArray (micNames, 5), isB ? 4 : 0);
        f (s + "micPos", nm + "mic position", 0.f, 1.f, 0.2f);
        f (s + "level", nm + "level", -24.f, 6.f, 0.f);
        f (s + "pan", nm + "pan", -1.f, 1.f, isB ? 0.5f : -0.5f);
    }

    // effects
    const StringArray pos { "Before amp", "After amp" };
    b ("modOn", "Modulation on", false);
    c ("modPos", "Modulation position", pos, 1);
    c ("modType", "Modulation type", { "Chorus", "Vibrato", "Tremolo" }, 0);
    f ("modRate", "Mod rate", 0.f, 1.f, 0.35f);   f ("modDepth", "Mod depth", 0.f, 1.f, 0.5f);
    f ("modMix", "Mod mix", 0.f, 1.f, 0.5f);
    c ("modShape", "Mod shape", { "Sine", "Triangle", "Square" }, 0);

    b ("dlyOn", "Delay on", false);
    c ("dlyPos", "Delay position", pos, 1);
    c ("dlyType", "Delay type", { "Digital", "Analog", "Tape", "Ping-pong", "Dual" }, 2);
    f ("dlyTime", "Delay time", 40.f, 1000.f, 420.f);  f ("dlyFb", "Delay feedback", 0.f, 0.9f, 0.35f);
    f ("dlyMix", "Delay mix", 0.f, 1.f, 0.3f);         f ("dlyTone", "Delay tone", 0.f, 1.f, 0.6f);
    f ("dlyMod", "Delay mod", 0.f, 1.f, 0.3f);
    c ("dlyRatio", "Dual ratio", { "1/2", "2/3", "3/4", "3/2" }, 2);

    b ("revOn", "Reverb on", true);
    c ("revPos", "Reverb position", pos, 1);
    c ("revType", "Reverb type", { "Room", "Hall", "Plate", "Spring", "Shimmer", "Cloud" }, 1);
    f ("revDecay", "Reverb decay", 0.f, 1.f, 0.45f);  f ("revDamp", "Reverb tone", 0.f, 1.f, 0.4f);
    f ("revMix", "Reverb mix", 0.f, 1.f, 0.25f);      f ("revPre", "Reverb pre-delay", 0.f, 1.f, 0.1f);
    f ("revMod", "Reverb mod", 0.f, 1.f, 0.35f);      f ("revFx", "Reverb effect", 0.f, 1.f, 0.5f);

    return l;
}

AuroraProcessor::AuroraProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
   #define X(n) P.n.p = apvts.getRawParameterValue (#n);
    AURORA_PARAMS(X)
   #undef X
}

bool AuroraProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet();
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
           && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void AuroraProcessor::prepareToPlay (double sr, int blockSize)
{
    fs = sr;
    sampleRateAtomic = sr;
    maxBlock = std::max (blockSize, 64);

    osDrv.initProcessing ((size_t) maxBlock);
    osDrv.reset();
    comp1.prepare (sr); comp2.prepare (sr);
    oct1.prepare (sr);  oct2.prepare (sr);
    drive1.prepare (sr * 2.0); drive2.prepare (sr * 2.0);
    ampA.prepare (sr, maxBlock);
    ampB.prepare (sr, maxBlock);
    bufA.setSize (2, maxBlock);
    bufB.setSize (2, maxBlock);
    mod.prepare (sr); dly.prepare (sr); rev.prepare (sr);

    for (auto* b : { &vLs, &vPk, &vLp }) b->reset();
    lastGuitar = -1;
    gEnv = 0.f; gGain = 1.f;
    lastOut = dbToGain (P.outGain());

    setLatencySamples ((int) std::lround (ampA.getLatency() + osDrv.getLatencyInSamples()));
}

void AuroraProcessor::loadIR (int slot, const juce::File& file)
{
    (slot == 0 ? ampA : ampB).loadIR (file);
    irPath[slot] = file.getFullPathName();
}

AmpParams AuroraProcessor::readAmp (bool b) const
{
    AmpParams a;
    if (! b)
    {
        a.model = P.a_model.i(); a.cab = P.a_cab.i(); a.mic = P.a_mic.i();
        a.gain = P.a_gain(); a.bass = P.a_bass(); a.mid = P.a_mid(); a.treble = P.a_treble();
        a.pres = P.a_pres(); a.master = P.a_master(); a.micPos = P.a_micPos();
    }
    else
    {
        a.model = P.b_model.i(); a.cab = P.b_cab.i(); a.mic = P.b_mic.i();
        a.gain = P.b_gain(); a.bass = P.b_bass(); a.mid = P.b_mid(); a.treble = P.b_treble();
        a.pres = P.b_pres(); a.master = P.b_master(); a.micPos = P.b_micPos();
    }
    return a;
}

void AuroraProcessor::runFx (int which, float* L, float* R, int n)
{
    if (which == 0 && P.modOn.on())
        mod.process (L, R, n, P.modType.i(), P.modRate(), P.modDepth(), P.modMix(), P.modShape.i());
    else if (which == 1 && P.dlyOn.on())
        dly.process (L, R, n, P.dlyType.i(), P.dlyTime(), P.dlyFb(), P.dlyMix(), P.dlyTone(), P.dlyMod(), P.dlyRatio.i());
    else if (which == 2 && P.revOn.on())
        rev.process (L, R, n, P.revType.i(), P.revDecay(), P.revDamp(), P.revMix(), P.revPre(), P.revMod(), P.revFx());
}

//==============================================================================
void AuroraProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0) return;

    if (getTotalNumInputChannels() < 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);   // mono guitar into the stereo chain

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    for (int start = 0; start < n; start += maxBlock)
        processChunk (L + start, R + start, std::min (maxBlock, n - start));

    meterPeak[0].store (buffer.getMagnitude (0, 0, n));
    meterPeak[1].store (buffer.getMagnitude (1, 0, n));
    gateOpen.store (gGain);
}

void AuroraProcessor::processChunk (float* L, float* R, int n)
{
    // ---- input: mono/stereo, trim, tuner tap, gate -------------------------------------
    const bool mono = P.inMode.i() == 0;
    const float inLin = dbToGain (P.inGain());
    const bool tuner = P.tunerOn.on();
    const float thr = dbToGain (P.gate());
    const float aEnv = std::exp (-1.f / (0.001f * (float) fs));
    const float rEnv = std::exp (-1.f / (0.080f * (float) fs));
    const float aG = std::exp (-1.f / (0.003f * (float) fs));
    const float rG = std::exp (-1.f / (0.120f * (float) fs));
    int tp = tunerPos.load();

    const int gi = P.guitar.i();
    if (gi != lastGuitar)
    {
        const auto& v = guitarVoicing (gi);
        vLs.setBand (fs, v.ls); vPk.setBand (fs, v.pk); vLp.setBand (fs, v.lp);
        lastGuitar = gi;
    }
    const float vTrim = gi == 0 ? 1.f : dbToGain (guitarVoicing (gi).trimDb);

    for (int i = 0; i < n; ++i)
    {
        float l = L[i] * inLin, r = R[i] * inLin;
        if (mono) l = r = 0.5f * (l + r);

        if (tuner)
        {
            tunerBuf[tp] = 0.5f * (l + r);
            tp = (tp + 1) % tunerSize;
        }

        const float det = std::fmax (std::fabs (l), std::fabs (r));
        gEnv = det > gEnv ? aEnv * gEnv + (1 - aEnv) * det : rEnv * gEnv + (1 - rEnv) * det;
        const float target = gEnv < thr ? 0.f : 1.f;
        gGain = target > gGain ? aG * gGain + (1 - aG) * target : rG * gGain + (1 - rG) * target;
        l *= gGain; r *= gGain;

        if (gi != 0)
        {
            l = vLp.process (0, vPk.process (0, vLs.process (0, l))) * vTrim;
            r = vLp.process (1, vPk.process (1, vLs.process (1, r))) * vTrim;
        }
        L[i] = l; R[i] = r;
    }
    if (tuner) tunerPos.store (tp);

    // ---- pedals: opto, octave, octave, drive, drive -------------------------------------
    if (P.comp1On.on())
        comp1.process (L, R, n, P.c1Sus(), P.c1Lvl(), 15.f, 400.f, 3.f, 1.f, P.c1Tone());
    if (P.comp2On.on())
        comp2.process (L, R, n, P.c2Comp(), P.c2Lvl(), 6.f, 220.f, 4.5f, P.c2Blend(), 0.5f);
    if (P.oct1On.on())
        oct1.process (L, R, n, P.o1Dry(), P.o1Sub(), P.o1Up());
    if (P.oct2On.on())
        oct2.process (L, R, n, P.o2Dry(), P.o2Oct1(), P.o2Oct2());

    if (P.drv1On.on() || P.drv2On.on())
    {
        float* ch[2] { L, R };
        juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
        auto up = osDrv.processSamplesUp (blk);
        float* uL = up.getChannelPointer (0);
        float* uR = up.getChannelPointer (1);
        const int m = (int) up.getNumSamples();
        if (P.drv1On.on()) drive1.process (uL, uR, m, P.d1Drive() * 0.1f, P.d1Tone() * 0.1f, P.d1Vol() * 0.1f);
        if (P.drv2On.on()) drive2.process (uL, uR, m, P.d2Gain() * 0.1f, P.d2Treble() * 0.1f, P.d2Bass() * 0.1f, P.d2Vol() * 0.1f);
        osDrv.processSamplesDown (blk);
    }

    // ---- effects placed before the amp ------------------------------------------------------
    if (P.modPos.i() == 0) runFx (0, L, R, n);
    if (P.dlyPos.i() == 0) runFx (1, L, R, n);
    if (P.revPos.i() == 0) runFx (2, L, R, n);

    // ---- amp A (+ amp B when dual) ---------------------------------------------------------------
    {
        const bool dual = P.dual.on();
        float* aL = bufA.getWritePointer (0); float* aR = bufA.getWritePointer (1);
        std::memcpy (aL, L, sizeof (float) * (size_t) n);
        std::memcpy (aR, R, sizeof (float) * (size_t) n);
        ampA.process (aL, aR, n, readAmp (false));
        const float lvA = dbToGain (P.a_level());

        if (! dual)
        {
            for (int i = 0; i < n; ++i) { L[i] = aL[i] * lvA; R[i] = aR[i] * lvA; }
        }
        else
        {
            float* bL = bufB.getWritePointer (0); float* bR = bufB.getWritePointer (1);
            std::memcpy (bL, L, sizeof (float) * (size_t) n);
            std::memcpy (bR, R, sizeof (float) * (size_t) n);
            ampB.process (bL, bR, n, readAmp (true));
            const float lvB = dbToGain (P.b_level());
            const float pa = (P.a_pan() + 1.f) * 0.7853982f, pb = (P.b_pan() + 1.f) * 0.7853982f;
            const float aLg = std::cos (pa) * 1.4142f * lvA, aRg = std::sin (pa) * 1.4142f * lvA;
            const float bLg = std::cos (pb) * 1.4142f * lvB, bRg = std::sin (pb) * 1.4142f * lvB;
            for (int i = 0; i < n; ++i)
            {
                const float ma = 0.5f * (aL[i] + aR[i]), mb = 0.5f * (bL[i] + bR[i]);
                L[i] = ma * aLg + mb * bLg;
                R[i] = ma * aRg + mb * bRg;
            }
        }
    }

    // ---- effects placed after the amp ------------------------------------------------------------
    if (P.modPos.i() == 1) runFx (0, L, R, n);
    if (P.dlyPos.i() == 1) runFx (1, L, R, n);
    if (P.revPos.i() == 1) runFx (2, L, R, n);

    // ---- output -----------------------------------------------------------------------------------
    const float outLin = dbToGain (P.outGain());
    const float step = (outLin - lastOut) / (float) n;
    float g = lastOut;
    for (int i = 0; i < n; ++i)
    {
        g += step;
        L[i] *= g; R[i] *= g;
    }
    lastOut = outLin;

    if (tuner)
    {
        std::memset (L, 0, sizeof (float) * (size_t) n);
        std::memset (R, 0, sizeof (float) * (size_t) n);
    }
}

//==============================================================================
juce::AudioProcessorEditor* AuroraProcessor::createEditor() { return new AuroraEditor (*this); }

void AuroraProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("irA", irPath[0], nullptr);
    state.setProperty ("irB", irPath[1], nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void AuroraProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid()) return;
        apvts.replaceState (state);
        for (int s = 0; s < 2; ++s)
        {
            const juce::File f (state.getProperty (s == 0 ? "irA" : "irB").toString());
            if (f.existsAsFile()) loadIR (s, f);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AuroraProcessor(); }
