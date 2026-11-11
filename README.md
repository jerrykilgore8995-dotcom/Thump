# THUMP v0.2.1 — Analog-style dual-band bass processor

**JUCE 8 / VST3 & Standalone / C++17**. Gold-and-black, scalable, real JUCE vector faceplate — not a static PNG. Includes eight original bass preset starting points and one-click Windows x64 Actions packaging.

## Status

**Source-code prototype.** The mathematical DSP/preset tests pass, but the JUCE VST3 binary has *not* been compiled, hosted in Reaper/Studio One, measured on audio, or auditioned in this environment. Do not treat the analog character as a verified hardware emulation.

## Signal flow

```
IN (+/-24 dB) → stereo 2x oversample → Linkwitz–Riley 4th-order crossover (120–800 Hz)
                              |                                         |
                  LOW: 40 / 120 / 220 Hz broad shelves       HIGH: 1.5 / 5 kHz proportional-Q bells
                  asymmetric low saturation                 Tube Screamer-like HP→soft-clip→LP
                  selectable FET/VCA/OPTO compressor        selectable FET/VCA/OPTO compressor
                              |                                         |
                         LOW MIX                                  HIGH MIX
                              +------------- SUM ----------------------+
                               → stereo-linked 1ms lookahead limiter
                               → MONOIZE (on/off) → OUTPUT (+/-24 dB)
```

### Two independent compressors: physically styled gold three-position selector on **each** band

| Mode | Ratio | Attack | Release | Notes |
|---|---|---|---|---|
| **FET (1176-inspired)** | Fixed 4:1 | FET ATTACK knob: 0.020–0.800 ms | FET RELEASE knob: 50–1100 ms | Peak detection; small added gain-cell coloration with GR |
| **VCA (dbx 160VU-inspired)** | Fixed 4:1 | Program-dependent nominal 15ms at 10 dB, 5ms at 20 dB, 3ms at 30 dB | Auto, 120 dB/s recovery | RMS-like detector, gentle 6dB knee |
| **OPTO (custom)** | Fixed 3:1 | Fixed **20 ms** | Dual-slope: first 50% of gain reduction recovers over **100ms**, remaining 50% over following **1.9s** | Custom smooth two-stage release designed for bass |

- Independent **COMP ON** buttons bypass the low or high compressor with an 8 ms gain crossfade; stereo sidechain detection remains linked within each band. 
- The **FET ATTACK** and **FET RELEASE** knobs are enabled only in FET mode. VCA and OPTO ignore them.
- Original dbx manual defines attack measurement as *63% of a level step* and specifies release in *dB/second*. This is a timing/specification approximation, **not an exact analog dbx 160 circuit simulation** or conformance claim. A full exact emulation would require transient-response measurements and hardware calibration.
- The OPTO **20ms** attack is intentional; original request said “20 s,” interpreted as **20 ms** because a 20-second bass attack would be impractical. If 20 seconds was intentional, change it in `runCompressor` in `Source/PluginProcessor.cpp`.

### EQ curve families — v0.2.1

- **Low:** Three broad, smoothly transitioning low-frequency shelves at 40, 120 and 220 Hz with independent gain ±12dB. This is **1084-family low-shelf topology**, adapted to the user's three fixed bands. The original hardware has *one* selectable low-frequency shelf at **35/60/110/220 Hz** (not three independent bell filters), and its published manual does not give the measured response tables required to guarantee an exact transfer-function match. Multiple THUMP shelf gains can stack below their corner frequencies by design.
- **High:** Two bell bands at **1.5 kHz and 5 kHz**, gain ±12dB. **550-family proportional Q** automatically makes small gain changes broad and larger changes narrower. THUMP implements a continuously variable Q law (stock baseline **Q = 0.46 + 1.14 × (abs(gainDb)/12)^0.78**), not a claim of measured-exact API curve matching.
- **Q / WIDTH rotary on each band:** **1.00 = unmodified stock-model curve;** values <1 broaden and >1 narrow/steepen. On low shelving filters, this controls shelf slope rather than true bell Q. On high peaking filters, it scales the proportional-Q curve. All five knobs double-click reset to 1.00. The preset's stored Q trims may intentionally differ.
- All EQ bands use minimum-phase stable biquads in each existing branch, with equal processing latency between branches. EQ, distortion and compression can of course alter the phase relation on recombination.

**Primary references:** [AMS Neve 1084 manufacturer specifications](https://www.ams-neve.com/outboard/classic-range/1084-2/), [API 550 version and proportional-Q circuit history (Paul Wolff / John Klett)](https://www.technicalaudio.com/pdf/API_Automated_Processes/API_550_EQ/API_550_Equalizer_Versions.pdf), and [TDR published API 550A hardware measurement-based design notes](https://docs.tokyodawn.net/slickeq-custom-models-guide/). An *exact* match remains an explicit calibration task requiring full gain-vs-frequency curves measured from representative units.

### DSP/phase notes

- The fourth-order Linkwitz–Riley filters use **identical sample rate, Q and crossover** for their low/high 2-pole cascades. They have flat *magnitude* after equal-level summing, but the sum has an **all-pass phase rotation**, not zero phase or linear phase. Independent band EQ/saturation/dynamics intentionally alter their relative transfer functions.
- One shared 2x polyphase oversampling stage processes both bands (no mismatched per-band oversampling delay). Both channel inputs work; mono inputs duplicate into a stereo scratch buffer so oversampler channels match.
- Residual-only AC coupling at 18Hz removes DC generated by the **low asymmetric saturator** while leaving the unprocessed low fundamental untouched.
- Limiter has stereo-linked peak control and fixed 1ms lookahead plus oversampler delay. The limiter's `MIX` under 100% is **parallel limiting** and does **not** guarantee a hard peak ceiling. Positive OUTPUT gain can exceed the ceiling even at 100% limiter mix.
- UI uses machined-gold knobs, gold EQ slider caps, black faceplate and controls, amber VU meters, custom gold-mode detent, detachable scalable layout, preset ComboBox, param automation, and XML state persistence.

## Factory presets (original starting points, not actual mixer presets)

1. **Modern Metal Lock:** VCA low / FET high; solid controlled bass and articulate grind.
2. **Classic Rock Body:** OPTO low / VCA high; fundamental and relaxed articulation.
3. **Punk Pick Attack:** FET low / FET high; fast string attack and presence.
4. **Drop Tuned Grind:** VCA low / VCA high; stable lower-register support.
5. **CLA-Inspired Bite:** FET low / FET high; 1176-like compression and EQ aggression.
6. **Scheps-Inspired Parallel:** OPTO low / VCA high (both compressors bypassed), high grit blend and limiter parallel mix.
7. **TLA-Inspired Crush:** FET low / FET high; denser bass and more compression.
8. **Staub-Inspired Edge:** VCA low / FET high; forward high-mid presence and solid low end.

Engineer-name presets are **stylistic references, not exact settings** or endorsements. Every preset stores all 39 plugin parameters including two mode selectors, on/off states, crossover, EQ gain and Q, saturation and blend, input/output, and limiter.

## Windows VST3 build via GitHub Actions (no local compiler needed)

1. Use the existing **Thump** GitHub repository, not TG-Zener-Limiter.
2. Upload the *contents* of this project (the `Source/`, `tests/`, `.github/workflows/` folders and `CMakeLists.txt`) to the new repository's **main** branch. Keep the folder structure, including the hidden `.github` folder.
3. Open the repository's **Actions** tab. Open **THUMP Windows VST3** and select **Run workflow**. Pushing to `main` also runs it automatically.
4. Wait for the **build** job to finish; under the successful run, download the artifact named `THUMP-Windows-x64-VST3`. Extract the `.zip` inside it to get **THUMP.vst3**.
5. Move **THUMP.vst3** to `C:\Program Files\Common Files\VST3\`, then rescan VST3 in Reaper or Studio One.

The CMake build automatically FetchContent-downloads **JUCE 8.0.17-security**. Alternatively provide `-DJUCE_LOCAL_PATH=/path/to/JUCE` if building offline. The CI VM has network access; this development runtime did not have JUCE installed.

### Local builds (if needed)

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Thump_VST3 --parallel 4
python tests/validate_dsp.py
```

## Documentation sources

- Universal Audio 1176 Classic Limiter Collection: attack **20–800µs**, release **50–1100ms**: https://help.uaudio.com/hc/en-us/articles/4419447352980-UA-1176-Classic-Limiter-Collection-Manual
- dbx 160/161 original manual: attack **15/5/3ms** for **10/20/30dB** steps; release **120dB/s**: https://www.thehistoryofrecording.com/Manuals/DBX/dbx_160.pdf

## Validation and future work

`python tests/validate_dsp.py` checks LR4 magnitude summation across 3 sample rates and 5 crossover points, timing design constants, opto release profile, 8 full preset arrays with 39 valid params, and GUI controls. These checks **cannot confirm** successful VST3 compilation, actual measured A/R conformity, transient behavior, latency, peak ceiling, phase after nonlinear effects, or real-time CPU performance. Next step is compiling in the GitHub workflow and running plug-in validation plus 44.1k/48k listening & analyzer tests.
