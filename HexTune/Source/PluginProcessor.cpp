#include "PluginProcessor.h"
#include "PluginEditor.h"

HexTuneProcessor::HexTuneProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& a : histDet) a.store (0.f);
    for (auto& a : histCor) a.store (0.f);
    for (auto& a : pcHist)  a.store (0.f);
}

bool HexTuneProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void HexTuneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    winSamples = (float) (sr * 0.025);
    for (auto& s : shifters)    s.prepare (winSamples);
    for (auto& s : vfxShifters) s.prepare (winSamples);
    yin.prepare ((float) sr);
    hop = juce::jmax (64, (int) (sr / 100.0));
    hopCount = 0;
    targetShift = shiftSmooth = 0.f;
    lastEq.fill (1e9f);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };

    for (int c = 0; c < 2; ++c)
    {
        eqLow[c].coefficients  = Coeffs::makeLowShelf  (sr, 120.f, 0.7f, 1.f);
        eqMid[c].coefficients  = Coeffs::makePeakFilter (sr, 1500.f, 0.8f, 1.f);
        eqHigh[c].coefficients = Coeffs::makeHighShelf (sr, 8000.f, 0.7f, 1.f);
        fmtA[c].coefficients   = Coeffs::makeLowShelf  (sr, 700.f, 0.7f, 1.f);
        fmtB[c].coefficients   = Coeffs::makeHighShelf (sr, 2500.f, 0.7f, 1.f);
        deessHP[c].coefficients = Coeffs::makeHighPass (sr, 5500.f, 0.7f);
        eqLow[c].reset(); eqMid[c].reset(); eqHigh[c].reset();
        fmtA[c].reset();  fmtB[c].reset();  deessHP[c].reset();
        deessEnv[c] = 0.f;
    }

    comp.prepare (spec);
    comp.setAttack (3.f);
    comp.setRelease (90.f);
    chorus.prepare (spec);
    chorus.setRate (1.1f);
    chorus.setDepth (0.3f);
    chorus.setCentreDelay (8.f);
    chorus.setFeedback (0.f);
    delay.prepare (spec);
    delay.reset();
    reverb.reset();
    reverb.setSampleRate (sr);
}

void HexTuneProcessor::updateFilters (float lo, float mid, float hi, float formant)
{
    if (lo == lastEq[0] && mid == lastEq[1] && hi == lastEq[2] && formant == lastEq[3])
        return;
    lastEq = { lo, mid, hi, formant };

    const auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };
    for (int c = 0; c < 2; ++c)
    {
        eqLow[c].coefficients  = Coeffs::makeLowShelf  (sr, 120.f, 0.7f, g (lo));
        eqMid[c].coefficients  = Coeffs::makePeakFilter (sr, 1500.f, 0.8f, g (mid));
        eqHigh[c].coefficients = Coeffs::makeHighShelf (sr, 8000.f, 0.7f, g (hi));
        // formant control is a spectral-tilt approximation (not true formant resynthesis)
        fmtA[c].coefficients   = Coeffs::makeLowShelf  (sr, 700.f, 0.7f, g (-formant * 0.6f));
        fmtB[c].coefficients   = Coeffs::makeHighShelf (sr, 2500.f, 0.7f, g (formant * 0.6f));
    }
}

void HexTuneProcessor::analyse (float amount, float humanize)
{
    const float speed = apvts.getRawParameterValue ("speed")->load();
    const float f = yin.detect();

    float det = 0.f, cor = 0.f;
    if (f > 60.f && f < 1100.f)
    {
        det = 69.f + 12.f * std::log2 (f / 440.f);

        // pitch-class histogram for Auto Detect
        for (auto& a : pcHist) a.store (a.load() * 0.998f);
        const int pc = ((int) std::round (det) % 12 + 12) % 12;
        pcHist[(size_t) pc].store (pcHist[(size_t) pc].load() + 1.f);

        // snap to nearest allowed note
        float bestN = std::round (det), bestD = 1e9f;
        for (int o = -7; o <= 7; ++o)
        {
            const int n = (int) std::round (det) + o;
            if (scaleMask & (1u << (((n % 12) + 12) % 12)))
            {
                const float d = std::abs ((float) n - det);
                if (d < bestD) { bestD = d; bestN = (float) n; }
            }
        }

        const float target = (bestN - det) * (amount / 100.f) * (1.f - 0.5f * humanize);
        const float hopMs = 1000.f * (float) hop / (float) sr;
        const float eff = speed * (1.f + humanize * 2.f);
        const float alpha = eff < 0.01f ? 1.f : 1.f - std::exp (-hopMs / eff);
        targetShift += (target - targetShift) * alpha;
        cor = det + targetShift;
    }
    else
    {
        targetShift *= 0.9f; // unvoiced: relax back to no shift
    }

    detectedMidi.store (det);
    correctedMidi.store (cor);
    const int p = histPos.load();
    histDet[(size_t) p].store (det);
    histCor[(size_t) p].store (cor);
    histPos.store ((p + 1) % histSize);
}

void HexTuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float amount   = P ("amount");
    const float humanize = P ("humanize") / 100.f;
    const bool  fxOn     = P ("fxOn") > 0.5f;
    const int   vfxType  = (fxOn && P ("vfxOn") > 0.5f) ? (int) P ("vfxType") : 0;
    const bool  eqOn     = fxOn && P ("eqOn") > 0.5f;
    const bool  satOn    = fxOn && P ("satOn") > 0.5f;
    const bool  deessOn  = fxOn && P ("deessOn") > 0.5f;
    const float satAmt   = P ("satAmt") / 100.f;
    const float satG     = 1.f + satAmt * 8.f;
    const float formant  = P ("formant");

    scaleMask = currentScaleMask();
    updateFilters (P ("eqLow"), P ("eqMid"), P ("eqHigh"), formant);

    // ---- stage 1: detect + shift + tone (per sample) ----
    for (int i = 0; i < n; ++i)
    {
        float mono = 0.f;
        for (int c = 0; c < numCh; ++c) mono += buffer.getReadPointer (c)[i];
        yin.push (mono / (float) numCh);

        if (++hopCount >= hop) { hopCount = 0; analyse (amount, humanize); }

        shiftSmooth += (targetShift - shiftSmooth) * 0.003f;
        const float ratio = std::exp2 (shiftSmooth / 12.f);

        roboPhase += juce::MathConstants<float>::twoPi * 70.f / (float) sr;
        if (roboPhase > juce::MathConstants<float>::twoPi) roboPhase -= juce::MathConstants<float>::twoPi;
        const float ring = std::sin (roboPhase);

        for (int c = 0; c < numCh; ++c)
        {
            float* d = buffer.getWritePointer (c);
            float y = shifters[c].process (d[i], ratio);

            switch (vfxType)
            {
                case 1: y = y * 0.3f + y * ring * 0.7f; break;
                case 2: y = y * 0.6f + 0.7f * vfxShifters[c].process (y, 0.5f); break;
                case 3: y = y * 0.7f + 0.5f * vfxShifters[c].process (y, 2.f); break;
                case 4: y = y * 0.75f + 0.55f * vfxShifters[c].process (y, std::exp2 (4.f / 12.f)); break;
                default: break;
            }

            y = fmtB[c].processSample (fmtA[c].processSample (y));

            if (deessOn)
            {
                const float hp = deessHP[c].processSample (y);
                const float low = y - hp;
                const float rect = std::abs (hp);
                deessEnv[c] += (rect - deessEnv[c]) * (rect > deessEnv[c] ? 0.2f : 0.003f);
                const float gr = deessEnv[c] > 0.04f ? 0.04f / deessEnv[c] : 1.f;
                y = low + hp * gr;
            }

            if (eqOn)
                y = eqHigh[c].processSample (eqMid[c].processSample (eqLow[c].processSample (y)));

            if (satOn)
            {
                const float s = std::tanh (satG * y) / std::tanh (satG);
                y += (s - y) * satAmt;
            }
            d[i] = y;
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);

    // ---- stage 2: compressor ----
    if (fxOn && P ("compOn") > 0.5f)
    {
        const float th = P ("compThresh");
        comp.setThreshold (th);
        comp.setRatio (P ("compRatio"));
        comp.process (ctx);
        buffer.applyGain (juce::Decibels::decibelsToGain (-th * 0.2f)); // rough makeup
    }

    // ---- stage 3: chorus ----
    if (fxOn && P ("chorusOn") > 0.5f)
    {
        chorus.setMix (P ("chorusMix") / 100.f);
        chorus.process (ctx);
    }

    // ---- stage 4: tempo-synced delay ----
    if (fxOn && P ("delayOn") > 0.5f)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm()) bpm = *b;

        static const float beats[] = { 1.f, 0.5f, 0.25f, 2.f, 0.75f };
        const int div = juce::jlimit (0, 4, (int) P ("delayDiv"));
        const float dSamp = juce::jlimit (4.f, 1000000.f, (float) (beats[div] * 60.0 / bpm * sr));
        const float mix = P ("delayMix") / 100.f;
        const float fb = 0.25f + 0.35f * mix;
        delay.setDelay (dSamp);
        for (int c = 0; c < numCh; ++c)
        {
            float* d = buffer.getWritePointer (c);
            for (int i = 0; i < n; ++i)
            {
                const float wet = delay.popSample (c);
                delay.pushSample (c, d[i] + wet * fb);
                d[i] += wet * mix;
            }
        }
    }

    // ---- stage 5: reverb ----
    if (fxOn && P ("reverbOn") > 0.5f)
    {
        const float mix = P ("reverbMix") / 100.f;
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.78f; rp.damping = 0.45f; rp.width = 1.f;
        rp.wetLevel = mix * 0.5f; rp.dryLevel = 1.f - mix * 0.25f; rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        if (numCh == 2) reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
        else            reverb.processMono (buffer.getWritePointer (0), n);
    }

    // ---- output gain + meter ----
    buffer.applyGain (juce::Decibels::decibelsToGain (P ("output")));
    float peak = 0.f;
    for (int c = 0; c < numCh; ++c) peak = juce::jmax (peak, buffer.getMagnitude (c, 0, n));
    outLevel.store (juce::jmax (peak, outLevel.load() * 0.9f));
}

juce::AudioProcessorEditor* HexTuneProcessor::createEditor() { return new HexTuneEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HexTuneProcessor(); }
