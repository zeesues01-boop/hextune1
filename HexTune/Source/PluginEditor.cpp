#include "PluginEditor.h"
using namespace HexColours;

// ============================ Look & Feel ============================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF1A0F2E));
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, text);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF1A0F2E));
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, neon.darker (0.3f));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF22123D));
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF0D0818));
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, edge);
    setColour (juce::TextEditor::focusedOutlineColourId, neon);
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, juce::Slider& s)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float arcR = r - 2.f, bodyR = r * 0.72f;
    const float ang = a0 + pos * (a1 - a0);

    juce::Path track, val;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, a0, a1, true);
    val.addCentredArc   (c.x, c.y, arcR, arcR, 0.f, a0, ang, true);

    g.setColour (juce::Colour (0xFF241440));
    g.strokePath (track, juce::PathStrokeType (r * 0.09f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (neon.withAlpha (0.25f));
    g.strokePath (val, juce::PathStrokeType (r * 0.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setGradientFill (juce::ColourGradient (neon, c.x - r, c.y + r, magenta, c.x + r, c.y - r, false));
    g.strokePath (val, juce::PathStrokeType (r * 0.09f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF241838), c.x, c.y - bodyR,
                                             juce::Colour (0xFF07030F), c.x, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f);
    g.setColour (edge);
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f, 1.5f);

    juce::Path p;
    p.addRoundedRectangle (-r * 0.02f, -bodyR + 3.f, r * 0.04f, bodyR * 0.2f, 1.5f);
    g.setColour (magenta);
    g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));

    g.setColour (text);
    g.setFont (juce::Font (juce::FontOptions (r * (s.getProperties().contains ("small") ? 0.34f : 0.3f), juce::Font::bold)));
    g.drawText (s.getTextFromValue (s.getValue()), juce::Rectangle<float> (c.x - bodyR, c.y - bodyR * 0.3f, bodyR * 2.f, bodyR * 0.6f),
                juce::Justification::centred);
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    const auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const bool on = b.getToggleState();
    if (b.getButtonText() == "power")
    {
        const auto c = r.getCentre();
        const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.38f;
        juce::Path p;
        p.addCentredArc (c.x, c.y, rad, rad, 0.f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
        p.startNewSubPath (c.x, c.y - rad * 1.15f);
        p.lineTo (c.x, c.y - rad * 0.1f);
        g.setColour (on ? magenta : dim);
        g.strokePath (p, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }
    const float h = r.getHeight();
    g.setColour (on ? neon : juce::Colour (0xFF3A3150));
    g.fillRoundedRectangle (r, h * 0.5f);
    g.setColour (juce::Colours::white);
    const float kx = on ? r.getRight() - h + 3.f : r.getX() + 3.f;
    g.fillEllipse (kx, r.getY() + 3.f, h - 6.f, h - 6.f);
}

void NeonLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.f);
    g.setColour (juce::Colour (0xFF1A0F2E));
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (edge);
    g.drawRoundedRectangle (r, 8.f, 1.5f);
    juce::Path a;
    const float cx = (float) w - 20.f, cy = (float) h * 0.5f;
    a.startNewSubPath (cx - 5.f, cy - 3.f); a.lineTo (cx, cy + 3.f); a.lineTo (cx + 5.f, cy - 3.f);
    g.setColour (text);
    g.strokePath (a, juce::PathStrokeType (2.f));
}

// ============================ Editor ============================
static juce::String fmtPct (double v)   { return juce::String ((int) std::round (v)) + "%"; }
static juce::String fmtDb  (double v)   { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }

static void fillChoices (juce::ComboBox& box, juce::AudioProcessorValueTreeState& s, const char* id)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (c->choices, 1);
}

HexTuneEditor::HexTuneEditor (HexTuneProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (1672.0 / 941.0);
    setResizeLimits (836, 470, 1672, 941);
    setSize (1100, 619);

    auto knob = [this] (juce::Slider& s, const char* id, std::unique_ptr<SA>& a, double def)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        a = std::make_unique<SA> (proc.apvts, id, s);
        s.setDoubleClickReturnValue (true, def);
        addAndMakeVisible (s);
    };
    knob (amount,   "amount",   aAmount,   100.0);
    knob (speed,    "speed",    aSpeed,    15.0);
    knob (formant,  "formant",  aFormant,  0.0);
    knob (humanize, "humanize", aHumanize, 25.0);
    knob (output,   "output",   aOutput,   0.0);
    amount.textFromValueFunction   = [] (double v) { return fmtPct (v); };
    speed.textFromValueFunction    = [] (double v) { return juce::String (v, v < 10 ? 1 : 0) + " ms"; };
    formant.textFromValueFunction  = [] (double v) { return juce::String (v, 1); };
    humanize.textFromValueFunction = [] (double v) { return fmtPct (v); };
    output.textFromValueFunction   = [] (double v) { return juce::String (v, 1); };
    output.getProperties().set ("small", true);
    for (auto* s : { &amount, &speed, &formant, &humanize, &output }) s->updateText();

    fillChoices (keyBox, proc.apvts, "key");
    fillChoices (scaleBox, proc.apvts, "scale");
    fillChoices (delayDivBox, proc.apvts, "delayDiv");
    fillChoices (vfxBox, proc.apvts, "vfxType");
    aKey = std::make_unique<CA> (proc.apvts, "key", keyBox);
    aScale = std::make_unique<CA> (proc.apvts, "scale", scaleBox);
    aDelayDiv = std::make_unique<CA> (proc.apvts, "delayDiv", delayDivBox);
    aVfx = std::make_unique<CA> (proc.apvts, "vfxType", vfxBox);
    for (auto* c : { &keyBox, &scaleBox, &delayDivBox, &vfxBox, &presetBox }) addAndMakeVisible (c);

    presetBox.addItem ("Default Preset", 1);
    for (int i = 0; i < 6; ++i) presetBox.addItem (HexTuneProcessor::presetNames[i], i + 2);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 2;
        proc.applyPreset (idx);
        selectedPreset = idx;
    };

    addAndMakeVisible (autoDetect);
    autoDetect.onClick = [this] { proc.autoDetectKey(); };

    addAndMakeVisible (search);
    search.setTextToShowWhenEmpty ("Search presets...", dim);
    search.onTextChange = [this] { repaint(); };

    static const char* ids[numToggles] = { "delayOn", "reverbOn", "eqOn", "compOn", "satOn", "chorusOn", "deessOn", "vfxOn", "fxOn" };
    for (int i = 0; i < numToggles; ++i)
    {
        if (i == numToggles - 1) toggles[i].setButtonText ("power");
        aToggles[i] = std::make_unique<BA> (proc.apvts, ids[i], toggles[i]);
        addAndMakeVisible (toggles[i]);
    }

    struct Def { const char* id; const char* name; std::function<juce::String (double)> fmt; };
    const Def defs[] = {
        { "delayMix",   "Delay Mix",   fmtPct }, { "reverbMix", "Reverb Mix", fmtPct }, { "chorusMix", "Chorus Mix", fmtPct },
        { "eqLow",      "EQ Low",      fmtDb },  { "eqMid",     "EQ Mid",     fmtDb },  { "eqHigh",    "EQ High",    fmtDb },
        { "satAmt",     "Saturation",  fmtPct }, { "compThresh","Comp Threshold", fmtDb },
        { "compRatio",  "Comp Ratio",  [] (double v) { return juce::String (v, 1) + ":1"; } } };
    const int cells[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    for (int i = 0; i < 9; ++i)
    {
        auto k = std::make_unique<FxKnob>();
        k->name = defs[i].name;
        k->s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k->s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        k->s.getProperties().set ("small", true);
        k->a = std::make_unique<SA> (proc.apvts, defs[i].id, k->s);
        k->s.textFromValueFunction = defs[i].fmt;
        k->s.updateText();
        const int c = cells[i];
        k->area = { 330.f + (float) (c % 4) * 235.f, 125.f + (float) (c / 4) * 170.f, 235.f, 170.f };
        addChildComponent (k->s);
        fxKnobs.push_back (std::move (k));
    }
    delayDivArea = { 330.f, 125.f, 235.f, 170.f };
    vfxArea      = { 330.f + 2 * 235.f, 125.f + 2 * 170.f, 235.f, 170.f };

    tabRects[0] = { 560.f, 28.f, 195.f, 54.f };
    tabRects[1] = { 765.f, 28.f, 150.f, 54.f };
    tabRects[2] = { 925.f, 28.f, 200.f, 54.f };

    showPage();
    startTimerHz (30);
}

HexTuneEditor::~HexTuneEditor() { setLookAndFeel (nullptr); }

juce::Rectangle<int> HexTuneEditor::R (float x, float y, float w, float h) const
{
    return juce::Rectangle<float> (x * sc, y * sc, w * sc, h * sc).toNearestInt();
}

void HexTuneEditor::setTab (int t) { tab = t; showPage(); resized(); repaint(); }

void HexTuneEditor::showPage()
{
    const bool tune = tab == 0, fx = tab == 1;
    for (auto* s : { &amount, &speed, &formant, &humanize }) s->setVisible (tune);
    for (auto& k : fxKnobs) k->s.setVisible (fx);
    delayDivBox.setVisible (fx);
    vfxBox.setVisible (fx);
}

void HexTuneEditor::resized()
{
    sc = (float) getWidth() / 1672.f;
    amount.setBounds   (R (368, 462, 180, 180));
    speed.setBounds    (R (597, 407, 260, 260));
    formant.setBounds  (R (905, 470, 160, 160));
    humanize.setBounds (R (1120, 478, 145, 145));
    output.setBounds   (R (1510, 805, 96, 96));

    keyBox.setBounds   (R (133, 190, 130, 38));
    scaleBox.setBounds (R (133, 245, 130, 38));
    autoDetect.setBounds (R (40, 307, 222, 42));
    search.setBounds   (R (33, 715, 262, 30));
    presetBox.setBounds (R (1295, 28, 195, 38));

    static const float rowY[8] = { 180, 258, 334, 411, 486, 562, 610, 656 };
    for (int i = 0; i < 8; ++i) toggles[i].setBounds (R (1550, rowY[i] - 13, 44, 26));
    toggles[8].setBounds (R (1592, 108, 44, 40));

    for (auto& k : fxKnobs)
        k->s.setBounds (R (k->area.getX() + 55, k->area.getY(), 125, 125));
    delayDivBox.setBounds (R (delayDivArea.getX() + 40, delayDivArea.getY() + 40, 155, 38));
    vfxBox.setBounds      (R (vfxArea.getX() + 40, vfxArea.getY() + 40, 155, 38));
}

void HexTuneEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = toDesign (e.getPosition());
    for (int i = 0; i < 3; ++i)
        if (tabRects[i].contains (p)) { setTab (i); return; }

    if (p.x < 300.f && p.y > 750.f)
    {
        for (size_t i = 0; i < visiblePresets.size(); ++i)
            if (juce::Rectangle<float> (23.f, 756.f + 25.5f * (float) i, 270.f, 24.f).contains (p))
            {
                selectedPreset = visiblePresets[i];
                proc.applyPreset (selectedPreset);
                presetBox.setSelectedId (selectedPreset + 2, juce::dontSendNotification);
                return;
            }
    }

    if (tab == 2)
        for (int i = 0; i < 6; ++i)
        {
            const juce::Rectangle<float> b (340.f + (float) (i % 3) * 315.f, 170.f + (float) (i / 3) * 200.f, 290.f, 170.f);
            if (b.contains (p))
            {
                selectedPreset = i;
                proc.applyPreset (i);
                presetBox.setSelectedId (i + 2, juce::dontSendNotification);
                repaint();
                return;
            }
        }
}
