#include "PluginProcessor.h"

const char* HexTuneProcessor::presetNames[6] = { "Hard Tune", "Melodic", "Rage", "R&B", "Pop", "Robot" };

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

static juce::ParameterID pid (const char* s) { return { s, 1 }; }

juce::AudioProcessorValueTreeState::ParameterLayout HexTuneProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>;

    Layout l;
    l.add (std::make_unique<C> (pid ("key"), "Key",
           juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    l.add (std::make_unique<C> (pid ("scale"), "Scale",
           juce::StringArray { "Major", "Minor", "Chromatic" }, 0));

    l.add (std::make_unique<F> (pid ("speed"),    "Retune Speed", R (0.f, 200.f, 0.1f), 15.f));
    l.add (std::make_unique<F> (pid ("amount"),   "Pitch Amount", R (0.f, 200.f, 1.f), 100.f));
    l.add (std::make_unique<F> (pid ("formant"),  "Formant",      R (-12.f, 12.f, 0.1f), 0.f));
    l.add (std::make_unique<F> (pid ("humanize"), "Humanize",     R (0.f, 100.f, 1.f), 25.f));

    l.add (std::make_unique<B> (pid ("delayOn"), "Delay On", true));
    l.add (std::make_unique<C> (pid ("delayDiv"), "Delay Time",
           juce::StringArray { "1/4", "1/8", "1/16", "1/2", "1/8 dotted" }, 0));
    l.add (std::make_unique<F> (pid ("delayMix"), "Delay Mix", R (0.f, 100.f, 1.f), 35.f));

    l.add (std::make_unique<B> (pid ("reverbOn"), "Reverb On", true));
    l.add (std::make_unique<F> (pid ("reverbMix"), "Reverb Mix", R (0.f, 100.f, 1.f), 40.f));

    l.add (std::make_unique<B> (pid ("eqOn"), "EQ On", true));
    l.add (std::make_unique<F> (pid ("eqLow"),  "EQ Low",  R (-12.f, 12.f, 0.1f), 0.f));
    l.add (std::make_unique<F> (pid ("eqMid"),  "EQ Mid",  R (-12.f, 12.f, 0.1f), 0.f));
    l.add (std::make_unique<F> (pid ("eqHigh"), "EQ High", R (-12.f, 12.f, 0.1f), 2.f));

    l.add (std::make_unique<B> (pid ("compOn"), "Comp On", true));
    l.add (std::make_unique<F> (pid ("compThresh"), "Comp Threshold", R (-40.f, 0.f, 0.1f), -18.f));
    l.add (std::make_unique<F> (pid ("compRatio"),  "Comp Ratio",     R (1.f, 20.f, 0.1f), 3.f));

    l.add (std::make_unique<B> (pid ("satOn"), "Saturation On", true));
    l.add (std::make_unique<F> (pid ("satAmt"), "Saturation", R (0.f, 100.f, 1.f), 50.f));

    l.add (std::make_unique<B> (pid ("chorusOn"), "Chorus On", false));
    l.add (std::make_unique<F> (pid ("chorusMix"), "Chorus Mix", R (0.f, 100.f, 1.f), 40.f));

    l.add (std::make_unique<B> (pid ("deessOn"), "De-Esser On", true));

    l.add (std::make_unique<B> (pid ("vfxOn"), "Voice FX On", true));
    l.add (std::make_unique<C> (pid ("vfxType"), "Voice FX",
           juce::StringArray { "None", "Robot", "Octave Down", "Octave Up", "Harmony" }, 0));

    l.add (std::make_unique<F> (pid ("output"), "Output", R (-24.f, 12.f, 0.1f), 0.f));
    l.add (std::make_unique<B> (pid ("fxOn"), "Effects Chain On", true));
    return l;
}

void HexTuneProcessor::setParam (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void HexTuneProcessor::applyPreset (int index)
{
    for (auto* p : getParameters())
        if (auto* q = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            if (q->paramID != "key" && q->paramID != "scale" && q->paramID != "output")
                q->setValueNotifyingHost (q->getDefaultValue());

    switch (index)
    {
        case 0: // Hard Tune
            setParam ("speed", 0.f); setParam ("humanize", 0.f);
            setParam ("delayMix", 20.f); setParam ("reverbMix", 25.f);
            break;
        case 1: // Melodic
            setParam ("speed", 25.f); setParam ("humanize", 35.f);
            setParam ("delayMix", 30.f); setParam ("reverbMix", 45.f);
            setParam ("chorusOn", 1.f); setParam ("chorusMix", 25.f);
            break;
        case 2: // Rage
            setParam ("speed", 3.f); setParam ("humanize", 0.f); setParam ("formant", 2.f);
            setParam ("satAmt", 75.f); setParam ("eqHigh", 4.f);
            setParam ("delayDiv", 1.f); setParam ("delayMix", 40.f); setParam ("reverbMix", 30.f);
            break;
        case 3: // R&B
            setParam ("speed", 40.f); setParam ("humanize", 55.f);
            setParam ("reverbMix", 50.f); setParam ("delayMix", 25.f);
            setParam ("chorusOn", 1.f); setParam ("chorusMix", 35.f);
            setParam ("satAmt", 30.f);
            break;
        case 4: // Pop
            setParam ("speed", 10.f); setParam ("humanize", 15.f);
            setParam ("eqHigh", 3.f); setParam ("compRatio", 4.f);
            setParam ("delayMix", 20.f); setParam ("reverbMix", 30.f);
            break;
        case 5: // Robot
            setParam ("speed", 0.f); setParam ("humanize", 0.f);
            setParam ("vfxType", 1.f); setParam ("delayMix", 15.f); setParam ("reverbMix", 20.f);
            break;
        default: break;
    }
}

unsigned int HexTuneProcessor::currentScaleMask() const
{
    static const int major[] = { 0, 2, 4, 5, 7, 9, 11 };
    static const int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
    const int key   = (int) apvts.getRawParameterValue ("key")->load();
    const int scale = (int) apvts.getRawParameterValue ("scale")->load();
    if (scale >= 2) return 0xFFF;

    unsigned int m = 0;
    for (int d : (scale == 0 ? major : minor))
        m |= 1u << ((key + d) % 12);
    return m;
}

void HexTuneProcessor::autoDetectKey()
{
    static const float maj[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static const float min[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

    float h[12], total = 0.f;
    for (int i = 0; i < 12; ++i) { h[i] = pcHist[(size_t) i].load(); total += h[i]; }
    if (total < 1.f) return; // nothing heard yet

    auto corr = [&] (const float* prof, int tonic)
    {
        float mh = total / 12.f, mp = 0.f;
        for (int i = 0; i < 12; ++i) mp += prof[i];
        mp /= 12.f;
        float num = 0.f, dh = 0.f, dp = 0.f;
        for (int i = 0; i < 12; ++i)
        {
            const float a = h[(tonic + i) % 12] - mh, b = prof[i] - mp;
            num += a * b; dh += a * a; dp += b * b;
        }
        return num / (std::sqrt (dh * dp) + 1e-9f);
    };

    float best = -2.f; int bestKey = 0, bestScale = 0;
    for (int k = 0; k < 12; ++k)
    {
        const float a = corr (maj, k), b = corr (min, k);
        if (a > best) { best = a; bestKey = k; bestScale = 0; }
        if (b > best) { best = b; bestKey = k; bestScale = 1; }
    }
    setParam ("key", (float) bestKey);
    setParam ("scale", (float) bestScale);
}

void HexTuneProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void HexTuneProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}
