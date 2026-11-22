#include "PluginEditor.h"
using namespace HexColours;

static juce::Font F (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

static const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

static juce::String noteName (float midi)
{
    if (midi <= 0.f) return "--";
    const int n = (int) std::round (midi);
    return juce::String (noteNames[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1);
}

void HexTuneEditor::panel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setColour (HexColours::panel);
    g.fillRoundedRectangle (r, 14.f);
    g.setColour (edge);
    g.drawRoundedRectangle (r, 14.f, 1.5f);
    if (title.isNotEmpty())
    {
        g.setColour (neon);
        g.fillRoundedRectangle (r.getX() + 16.f, r.getY() + 14.f, 5.f, 22.f, 2.f);
        g.setColour (text);
        g.setFont (F (21.f, true));
        g.drawText (title, r.withTrimmedLeft (32.f).withHeight (50.f), juce::Justification::centredLeft);
    }
}

void HexTuneEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF1E0B38), 0.f, 0.f, bg, (float) getWidth(), (float) getHeight(), true));
    g.fillRect (getLocalBounds());

    g.addTransform (juce::AffineTransform::scale (sc));
    paintHeader (g);
    paintLeft (g);
    paintEffects (g);
    paintPresets (g);
    paintKeys (g);

    if (tab == 0) paintGraph (g);
    else if (tab == 1) paintFxPage (g);
    else paintPresetPage (g);
}

void HexTuneEditor::paintHeader (juce::Graphics& g)
{
    juce::Path star;
    star.addStar ({ 85.f, 58.f }, 5, 22.f, 50.f, -0.3f);
    g.setGradientFill (juce::ColourGradient (neon, 40.f, 20.f, magenta, 130.f, 100.f, false));
    g.fillPath (star);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white, 160.f, 20.f, magenta, 430.f, 80.f, false));
    g.setFont (F (64.f, true).withExtraKerningFactor (0.02f));
    g.drawText ("HEX TUNE", juce::Rectangle<float> (160.f, 14.f, 340.f, 80.f), juce::Justification::centredLeft);

    const char* names[3] = { "TUNE", "FX", "PRESETS" };
    for (int i = 0; i < 3; ++i)
    {
        const auto r = tabRects[i];
        if (i == tab)
        {
            g.setGradientFill (juce::ColourGradient (neon.darker (0.2f), r.getX(), r.getY(), magenta.darker (0.4f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 12.f);
            g.setColour (magenta);
            g.drawRoundedRectangle (r, 12.f, 2.f);
        }
        g.setColour (i == tab ? juce::Colours::white : dim);
        g.setFont (F (28.f, true));
        g.drawText (names[i], r, juce::Justification::centred);
    }
}

void HexTuneEditor::paintLeft (juce::Graphics& g)
{
    panel (g, { 23.f, 122.f, 257.f, 243.f }, "KEY & SCALE");
    g.setColour (text);
    g.setFont (F (20.f));
    g.drawText ("Key", juce::Rectangle<float> (45.f, 190.f, 80.f, 38.f), juce::Justification::centredLeft);
    g.drawText ("Scale", juce::Rectangle<float> (45.f, 245.f, 80.f, 38.f), juce::Justification::centredLeft);

    panel (g, { 23.f, 383.f, 257.f, 262.f }, "PITCH GRAPH");
    g.setColour (dim);
    g.setFont (F (21.f));
    g.drawText ("Detected Pitch", juce::Rectangle<float> (112.f, 462.f, 170.f, 30.f), juce::Justification::centredLeft);
    g.setColour (magenta);
    g.setFont (F (52.f, true));
    g.drawText (noteName (proc.detectedMidi.load()), juce::Rectangle<float> (112.f, 495.f, 170.f, 60.f), juce::Justification::centredLeft);

    // level bars (output peak)
    const float lvl = juce::jlimit (0.f, 1.f, (juce::Decibels::gainToDecibels (proc.outLevel.load(), -60.f) + 60.f) / 60.f);
    for (int bar = 0; bar < 2; ++bar)
        for (int i = 0; i < 20; ++i)
        {
            const float y = 620.f - (float) i * 8.8f;
            const bool lit = (float) i / 20.f < lvl;
            g.setColour (lit ? neon.interpolatedWith (magenta, (float) i / 20.f) : juce::Colour (0xFF2A1A44));
            g.fillRect (42.f + (float) bar * 22.f, y, 18.f, 6.f);
        }
}

void HexTuneEditor::paintGraph (juce::Graphics& g)
{
    panel (g, { 305.f, 108.f, 980.f, 262.f });
    const juce::Rectangle<float> plot (355.f, 112.f, 928.f, 254.f);
    auto yOf = [] (float m) { return 146.f + (84.f - m) * 4.89f; };

    g.setFont (F (17.f));
    for (int oct = 3; oct <= 6; ++oct)
    {
        const float y = yOf ((float) (oct + 1) * 12.f);
        g.setColour (juce::Colour (0xFF2B1A4A));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (dim);
        g.drawText ("C" + juce::String (oct), juce::Rectangle<float> (310.f, y - 10.f, 42.f, 20.f), juce::Justification::centred);
    }
    for (int i = 0; i <= 12; ++i)
    {
        g.setColour (juce::Colour (0xFF1E1236));
        g.drawVerticalLine ((int) (plot.getX() + plot.getWidth() * (float) i / 12.f), plot.getY(), plot.getBottom());
    }

    auto trace = [&] (auto& hist, juce::Colour col, float thick, float alpha)
    {
        juce::Path p;
        bool open = false;
        const int start = proc.histPos.load();
        for (int i = 0; i < HexTuneProcessor::histSize; ++i)
        {
            const float m = hist[(size_t) ((start + i) % HexTuneProcessor::histSize)].load();
            if (m <= 0.f) { open = false; continue; }
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (HexTuneProcessor::histSize - 1);
            const float y = juce::jlimit (plot.getY(), plot.getBottom(), yOf (m));
            if (! open) { p.startNewSubPath (x, y); open = true; } else p.lineTo (x, y);
        }
        g.setColour (col.withAlpha (alpha * 0.3f));
        g.strokePath (p, juce::PathStrokeType (thick * 3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col.withAlpha (alpha));
        g.strokePath (p, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    trace (proc.histDet, neon.darker (0.3f), 2.f, 0.6f);
    trace (proc.histCor, magenta, 3.f, 1.f);

    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawVerticalLine ((int) plot.getRight() - 1, plot.getY(), plot.getBottom());

    // knob captions
    g.setColour (text);
    g.setFont (F (23.f, true));
    g.drawText ("PITCH AMOUNT", juce::Rectangle<float> (360.f, 418.f, 200.f, 30.f), juce::Justification::centred);
    g.drawText ("RETUNE SPEED", juce::Rectangle<float> (607.f, 362.f, 240.f, 30.f), juce::Justification::centred);
    g.drawText ("FORMANT", juce::Rectangle<float> (900.f, 415.f, 170.f, 30.f), juce::Justification::centred);
    g.drawText ("HUMANIZE", juce::Rectangle<float> (1100.f, 420.f, 180.f, 30.f), juce::Justification::centred);

    g.setColour (dim);
    g.setFont (F (17.f));
    auto cap = [&] (const char* s, float cx, float y) { g.drawText (s, juce::Rectangle<float> (cx - 40.f, y, 80.f, 22.f), juce::Justification::centred); };
    cap ("0%", 396.f, 622.f);   cap ("200%", 515.f, 622.f);
    cap ("0 ms", 625.f, 640.f); cap ("200 ms", 833.f, 640.f);
    cap ("-12", 931.f, 622.f);  cap ("+12", 1031.f, 622.f);
    cap ("0%", 1139.f, 622.f);  cap ("100%", 1245.f, 622.f);
}

void HexTuneEditor::paintFxPage (juce::Graphics& g)
{
    panel (g, { 310.f, 100.f, 985.f, 565.f });
    g.setColour (text);
    g.setFont (F (21.f, true));
    for (auto& k : fxKnobs)
        g.drawText (k->name, juce::Rectangle<float> (k->area.getX(), k->area.getY() + 128.f, k->area.getWidth(), 28.f), juce::Justification::centred);
    g.drawText ("Delay Time", juce::Rectangle<float> (delayDivArea.getX(), delayDivArea.getY() + 90.f, delayDivArea.getWidth(), 28.f), juce::Justification::centred);
    g.drawText ("Voice FX", juce::Rectangle<float> (vfxArea.getX(), vfxArea.getY() + 90.f, vfxArea.getWidth(), 28.f), juce::Justification::centred);
}

void HexTuneEditor::paintPresetPage (juce::Graphics& g)
{
    panel (g, { 310.f, 100.f, 985.f, 565.f });
    for (int i = 0; i < 6; ++i)
    {
        const juce::Rectangle<float> b (340.f + (float) (i % 3) * 315.f, 170.f + (float) (i / 3) * 200.f, 290.f, 170.f);
        const bool sel = i == selectedPreset;
        g.setGradientFill (sel ? juce::ColourGradient (neon, b.getX(), b.getY(), magenta.darker (0.4f), b.getRight(), b.getBottom(), false)
                               : juce::ColourGradient (juce::Colour (0xFF22123D), b.getX(), b.getY(), juce::Colour (0xFF150C26), b.getRight(), b.getBottom(), false));
        g.fillRoundedRectangle (b, 16.f);
        g.setColour (sel ? magenta : edge);
        g.drawRoundedRectangle (b, 16.f, 2.f);
        g.setColour (juce::Colours::white);
        g.setFont (F (34.f, true));
        g.drawText (HexTuneProcessor::presetNames[i], b, juce::Justification::centred);
    }
}

void HexTuneEditor::paintEffects (juce::Graphics& g)
{
    panel (g, { 1312.f, 102.f, 337.f, 610.f }, "EFFECTS CHAIN");

    auto P = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    auto pct = [&] (const char* id) { return juce::String ((int) std::round (P (id))) + "%"; };
    const bool eqFlat = std::abs (P ("eqLow")) < 0.05f && std::abs (P ("eqMid")) < 0.05f && std::abs (P ("eqHigh")) < 0.05f;

    struct Row { const char* name; float y; juce::String sub; };
    const Row rows[] = {
        { "Delay",      180.f, delayDivBox.getText() + "   " + pct ("delayMix") },
        { "Reverb",     258.f, "Hall   " + pct ("reverbMix") },
        { "EQ",         334.f, eqFlat ? "Flat" : "Custom" },
        { "Compressor", 411.f, juce::String (P ("compThresh"), 1) + " dB   " + juce::String (P ("compRatio"), 1) + ":1" },
        { "Saturation", 486.f, "Tube   " + pct ("satAmt") },
        { "Chorus",     562.f, {} },
        { "De-Esser",   610.f, {} },
        { "Voice FX",   656.f, vfxBox.getText() } };

    for (const auto& r : rows)
    {
        g.setColour (neon);
        g.fillRoundedRectangle (1338.f, r.y - 14.f, 5.f, 28.f, 2.f);
        g.setColour (text);
        g.setFont (F (22.f, true));
        g.drawText (r.name, juce::Rectangle<float> (1402.f, r.y - 24.f, 200.f, 30.f), juce::Justification::centredLeft);
        g.setColour (dim);
        g.setFont (F (17.f));
        g.drawText (r.sub, juce::Rectangle<float> (1402.f, r.y + 2.f, 200.f, 24.f), juce::Justification::centredLeft);
    }
}

void HexTuneEditor::paintPresets (juce::Graphics& g)
{
    panel (g, { 23.f, 680.f, 283.f, 240.f }, "PRESETS");

    visiblePresets.clear();
    const auto q = search.getText().trim();
    for (int i = 0; i < 6; ++i)
        if (q.isEmpty() || juce::String (HexTuneProcessor::presetNames[i]).containsIgnoreCase (q))
            visiblePresets.push_back (i);

    for (size_t i = 0; i < visiblePresets.size(); ++i)
    {
        const int idx = visiblePresets[i];
        const juce::Rectangle<float> r (23.f, 756.f + 25.5f * (float) i, 270.f, 24.f);
        if (idx == selectedPreset)
        {
            g.setGradientFill (juce::ColourGradient (neon, r.getX(), r.getY(), magenta.darker (0.3f), r.getRight(), r.getY(), false));
            g.fillRect (r);
        }
        juce::Path star;
        star.addStar ({ r.getX() + 22.f, r.getCentreY() }, 5, 4.5f, 9.f, 0.f);
        g.setColour (idx == selectedPreset ? juce::Colours::white : dim);
        if (idx == selectedPreset) g.fillPath (star); else g.strokePath (star, juce::PathStrokeType (1.2f));
        g.setFont (F (19.f));
        g.drawText (HexTuneProcessor::presetNames[idx], r.withTrimmedLeft (44.f), juce::Justification::centredLeft);
    }
}

void HexTuneEditor::paintKeys (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xFF0D0818));
    g.fillRoundedRectangle (320.f, 735.f, 1125.f, 185.f, 10.f);

    const unsigned int mask = proc.currentScaleMask();
    const int active = proc.correctedMidi.load() > 0.f ? (int) std::round (proc.correctedMidi.load()) : -1;
    static const int off[7] = { 0, 2, 4, 5, 7, 9, 11 };
    const int whites = 36;
    const float x0 = 335.f, ww = 1100.f / (float) whites, top = 745.f, hh = 165.f;

    auto midiOf = [&] (int i) { return 36 + 12 * (i / 7) + off[i % 7]; };
    auto colourFor = [&] (int midi, juce::Colour base, juce::Colour inScale)
    {
        if (midi == active) return magenta;
        return (mask & (1u << (midi % 12))) ? inScale : base;
    };

    for (int i = 0; i < whites; ++i)
    {
        const int m = midiOf (i);
        g.setColour (colourFor (m, juce::Colour (0xFFC9B8E6), juce::Colour (0xFFE08BEA)));
        g.fillRoundedRectangle (x0 + (float) i * ww + 1.f, top, ww - 2.f, hh, 3.f);
    }
    for (int i = 0; i < whites - 1; ++i)
    {
        const int idx = i % 7;
        if (idx == 2 || idx == 6) continue;
        const int m = midiOf (i) + 1;
        const float bw = ww * 0.62f;
        g.setColour (colourFor (m, juce::Colour (0xFF0A0512), juce::Colour (0xFF5B1E8F)));
        g.fillRoundedRectangle (x0 + (float) (i + 1) * ww - bw * 0.5f, top, bw, hh * 0.62f, 3.f);
    }

    // output panel
    panel (g, { 1455.f, 735.f, 195.f, 185.f }, {});
    g.setColour (text);
    g.setFont (F (21.f, true));
    g.drawText ("OUTPUT", juce::Rectangle<float> (1470.f, 742.f, 150.f, 28.f), juce::Justification::centredLeft);
    g.setColour (dim);
    g.setFont (F (17.f));
    g.drawText (juce::String (proc.apvts.getRawParameterValue ("output")->load(), 1) + " dB",
                juce::Rectangle<float> (1490.f, 892.f, 130.f, 22.f), juce::Justification::centred);
}
