#pragma once
#include "PluginProcessor.h"

namespace HexColours
{
    const juce::Colour bg      { 0xFF0A0613 };
    const juce::Colour panel   { 0xFF130B21 };
    const juce::Colour edge    { 0xFF3A1A66 };
    const juce::Colour neon    { 0xFFB43CFF };
    const juce::Colour magenta { 0xFFFF3FE0 };
    const juce::Colour text    { 0xFFEDE3FF };
    const juce::Colour dim     { 0xFF8E7AB5 };
}

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (15.f)); }
};

class HexTuneEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HexTuneEditor (HexTuneProcessor&);
    ~HexTuneEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override { repaint(); }
    juce::Rectangle<int> R (float x, float y, float w, float h) const;
    juce::Point<float> toDesign (juce::Point<int> p) const { return p.toFloat() / sc; }
    void setTab (int t);
    void showPage();

    void paintHeader (juce::Graphics&);
    void paintGraph (juce::Graphics&);
    void paintLeft (juce::Graphics&);
    void paintEffects (juce::Graphics&);
    void paintPresets (juce::Graphics&);
    void paintKeys (juce::Graphics&);
    void paintFxPage (juce::Graphics&);
    void paintPresetPage (juce::Graphics&);
    void panel (juce::Graphics&, juce::Rectangle<float>, const juce::String& title = {});

    HexTuneProcessor& proc;
    NeonLookAndFeel laf;
    float sc = 1.f;
    int tab = 0, selectedPreset = 0;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::Slider amount, speed, formant, humanize, output;
    std::unique_ptr<SA> aAmount, aSpeed, aFormant, aHumanize, aOutput;

    juce::ComboBox keyBox, scaleBox, presetBox, delayDivBox, vfxBox;
    std::unique_ptr<CA> aKey, aScale, aDelayDiv, aVfx;
    juce::TextButton autoDetect { "Auto Detect" };
    juce::TextEditor search;

    static constexpr int numToggles = 9; // 8 effects + master power
    juce::ToggleButton toggles[numToggles];
    std::unique_ptr<BA> aToggles[numToggles];

    struct FxKnob { juce::Slider s; std::unique_ptr<SA> a; juce::String name; juce::Rectangle<float> area; };
    std::vector<std::unique_ptr<FxKnob>> fxKnobs;
    juce::Rectangle<float> delayDivArea, vfxArea;

    juce::Rectangle<float> tabRects[3];
    std::vector<int> visiblePresets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HexTuneEditor)
};
