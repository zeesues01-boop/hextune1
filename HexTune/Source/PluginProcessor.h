#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

class HexTuneProcessor : public juce::AudioProcessor
{
public:
    HexTuneProcessor();
    ~HexTuneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Hex Tune"; }
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

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- data shared with the GUI ----
    static constexpr int histSize = 300;
    std::atomic<float> detectedMidi { 0.f }, correctedMidi { 0.f }, outLevel { 0.f };
    std::array<std::atomic<float>, histSize> histDet, histCor;
    std::atomic<int> histPos { 0 };
    std::array<std::atomic<float>, 12> pcHist;

    static const char* presetNames[6];
    void applyPreset (int index);
    void autoDetectKey();
    unsigned int currentScaleMask() const;

private:
    // ---- pitch shifter: two crossfading delay taps ----
    struct Shifter
    {
        static constexpr int N = 8192;
        std::vector<float> buf;
        int w = 0;
        float phase = 0.f, win = 1200.f;

        void prepare (float winSamples) { buf.assign (N, 0.f); w = 0; phase = 0.f; win = winSamples; }

        float read (float d) const
        {
            float p = (float) w - d;
            while (p < 0.f) p += (float) N;
            int i = (int) p;
            float f = p - (float) i;
            int i0 = i & (N - 1), i1 = (i + 1) & (N - 1);
            return buf[(size_t) i0] * (1.f - f) + buf[(size_t) i1] * f;
        }

        float process (float in, float ratio)
        {
            buf[(size_t) w] = in;
            const float ph2 = std::fmod (phase + 0.5f, 1.f);
            const float s1 = read (phase * win), s2 = read (ph2 * win);
            const float tp = juce::MathConstants<float>::twoPi;
            const float w1 = 0.5f - 0.5f * std::cos (tp * phase);
            const float w2 = 0.5f - 0.5f * std::cos (tp * ph2);
            phase += (1.f - ratio) / win;
            phase -= std::floor (phase);
            w = (w + 1) & (N - 1);
            return s1 * w1 + s2 * w2;
        }
    };

    // ---- YIN pitch detector ----
    struct Yin
    {
        static constexpr int R = 8192, W = 1024, TMAX = 1600;
        std::array<float, R> ring {};
        std::vector<float> frame = std::vector<float> ((size_t) (W + TMAX));
        std::vector<float> cmnd = std::vector<float> ((size_t) TMAX + 2);
        int pos = 0;
        float sr = 44100.f;

        void prepare (float sampleRate) { sr = sampleRate; ring.fill (0.f); pos = 0; }
        void push (float x) { ring[(size_t) pos] = x; pos = (pos + 1) & (R - 1); }

        // returns frequency in Hz, or 0 if unvoiced
        float detect()
        {
            const int need = W + TMAX;
            for (int i = 0; i < need; ++i)
                frame[(size_t) i] = ring[(size_t) ((pos - need + i) & (R - 1))];

            double e = 0.0;
            for (int i = 0; i < W; ++i) e += (double) frame[(size_t) i] * frame[(size_t) i];
            if (std::sqrt (e / W) < 0.008) return 0.f;

            const int tauMax = juce::jmin (TMAX - 1, (int) (sr / 70.f));
            const int tauMin = juce::jmax (2, (int) (sr / 1000.f));
            float run = 0.f;
            cmnd[0] = 1.f;
            const float* a = frame.data();
            for (int tau = 1; tau <= tauMax; ++tau)
            {
                const float* b = a + tau;
                float s = 0.f;
                for (int j = 0; j < W; ++j) { float d = a[j] - b[j]; s += d * d; }
                run += s;
                cmnd[(size_t) tau] = s * (float) tau / juce::jmax (run, 1e-9f);
            }

            int tau = -1;
            for (int t = tauMin; t <= tauMax; ++t)
            {
                if (cmnd[(size_t) t] < 0.15f)
                {
                    while (t + 1 <= tauMax && cmnd[(size_t) t + 1] < cmnd[(size_t) t]) ++t;
                    tau = t;
                    break;
                }
            }
            if (tau < 0) return 0.f;

            float better = (float) tau;
            if (tau > 1 && tau < tauMax)
            {
                float s0 = cmnd[(size_t) tau - 1], s1 = cmnd[(size_t) tau], s2 = cmnd[(size_t) tau + 1];
                float den = 2.f * (2.f * s1 - s2 - s0);
                if (std::abs (den) > 1e-9f) better = (float) tau + (s2 - s0) / den;
            }
            return sr / better;
        }
    };

    void analyse (float amount, float humanize);
    void updateFilters (float lo, float mid, float hi, float formant);
    void setParam (const juce::String& id, float plainValue);

    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using IIR = juce::dsp::IIR::Filter<float>;

    double sr = 44100.0;
    float winSamples = 1100.f;
    Shifter shifters[2], vfxShifters[2];
    Yin yin;
    int hop = 441, hopCount = 0;
    float targetShift = 0.f, shiftSmooth = 0.f;
    float roboPhase = 0.f;
    unsigned int scaleMask = 0xFFF;

    IIR eqLow[2], eqMid[2], eqHigh[2], fmtA[2], fmtB[2], deessHP[2];
    std::array<float, 4> lastEq { 1e9f, 1e9f, 1e9f, 1e9f };
    float deessEnv[2] { 0.f, 0.f };

    juce::dsp::Compressor<float> comp;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 1 << 20 };
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HexTuneProcessor)
};
