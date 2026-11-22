# Hex Tune (VST3)

Auto-tune / pitch-correction plugin built with JUCE 8. Layout follows the supplied neon UI.

## Get the .vst3 (no compiler needed)
1. Create a free GitHub repo and upload this whole folder (keep the `.github` folder).
2. Open the **Actions** tab -> **Build Hex Tune VST3** -> wait for the run to finish.
3. Download the artifact `HexTune-VST3-windows-latest` and unzip it.
4. Copy `Hex Tune.vst3` to `C:\Program Files\Common Files\VST3\`.
5. In FL Studio: Options -> Manage plugins -> **Find installed plugins**. Add it on a mixer insert (Slot -> Hex Tune).

## Build locally (Windows)
Install Visual Studio 2022 (Desktop C++) and CMake, then:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target HexTune_VST3

Output: `build/HexTune_artefacts/Release/VST3/Hex Tune.vst3`

## What works
- **Retune Speed** 0-200 ms (0 = hard tune), **Pitch Amount**, **Humanize**, **Key / Scale** (+ Auto Detect from what you sing)
- YIN pitch detection -> scale snapping -> crossfaded delay-line pitch shifter
- Effects: tempo-synced Delay, Reverb, 3-band EQ, Compressor, Saturation, Chorus, De-Esser, Voice FX (Robot, Octave Up/Down, Harmony)
- Presets: Hard Tune, Melodic, Rage, R&B, Pop, Robot
- FX tab has the knobs for every effect; the chain switches are on the right.

## Known limits
- Formant is a spectral-tilt approximation, not true formant-preserving resynthesis.
- Mono vocals work best; the pitch shifter is simple, so extreme corrections can sound grainy (which suits hard-tune styles).
- Header folder/save/settings icons from the mockup are not implemented; use FL's own preset menu.
- Not yet compiled or tested in a DAW by the author - first CI run may need small fixes.
