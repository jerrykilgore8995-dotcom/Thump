# THUMP v0.5.0 — JUCE 8 analog-style dual-band bass processor

**Source prototype:** JUCE/C++17 VST3 and standalone app, black + satin gold rack GUI, eight original rock/metal factory presets. Windows x64 build supplied through GitHub Actions. For Studio One / REAPER at 44.1 or 48 kHz.

## What's new in 0.5

- **Real overdrive range:** Both Drive controls now give moderate to heavy audible distortion. At **Drive 10, 100% wet**, a -15 dBFS reference sine produces approximately **28% total harmonic distortion at 80 Hz** on the LOW path and **27% at 1 kHz** on the HIGH path in the offline reference calculations. Actual THD depends on signal amplitude, EQ, crossover, spectrum, drive/mix, and compressor order. High is *symmetrical*, K3-dominant clipping inspired by anti-parallel Tube Screamer diodes and ~720 Hz pre-emphasis. Low uses a distinct *asymmetrically biased* nonlinearity rich in K2 and K3. Each has a genuine 0–100% wet/dry Mix.
- **Level matching:** Per-band 35 ms RMS-tracking, bounded compensation on the wet drive branch. The output of stronger distortion is normally near the dry level rather than acting like an additional input-gain knob. Crest factor and spectral balance still change; momentary transients and changes in downstream compression **cannot be perfectly identical**.
- **Six independent orders:** Each band has a drop-down with **EQ→Drive→Comp, EQ→Comp→Drive, Drive→EQ→Comp, Drive→Comp→EQ, Comp→EQ→Drive, Comp→Drive→EQ**. Default remains EQ→Drive→Comp. The visible rack face, all gain-reduction meters, and existing parameter automation IDs remain intact.
- **Compressor ratio controls:** FET: switch 4:1/8:1/12:1/20:1, with 1176-style attack/release ranges and ratio-dependent threshold offsets. VCA: continuously adjustable 1:1–120:1 (dbx 160-style level-dependent attack and 120 dB/s release). Opto: 3:1, 20 ms attack, 50% recovery at 100 ms and rest by 2 seconds.
- **Compressor coloration:** Additive, independent, gain-reduction-scaled K2–K5 residuals. Opto uses user-supplied dBc targets listed below; FET mildly colors low frequencies with both even and odd harmonics; VCA offers deliberately *enhanced dbx-style* color (~1–2% K3 source coefficient at 20 dB reduction in the low band), **not** the much cleaner THAT 2181A/B/C chip specification. Each model generates no color when its gain reduction is zero; bypassed compressors do not add color after their bypass crossfade.
- **4× shared oversampling:** Entire signal chain runs at four times host sample rate with JUCE's polyphase anti-aliasing filters; this reduces aliasing, but **4× cannot eliminate foldback at arbitrarily high harmonics**. The high drive also uses a 5.2 kHz post-clip low-pass filter. Real input/headroom tests remain essential.
- **Low-only MONOIZE:** Averages the low branch L/R after the common Linkwitz–Riley split, before processing; high frequencies retain their stereo positions. The post-split low/high blend remains a sample-aligned linear sum. No separately delayed parallel crossovers or mix-mode comb filters.
- **Final limiter:** Keeps stereo-linked 1 ms lookahead, gentle glue compression, selectable ceiling and wet/dry. Its ceiling applies to the wet limiting path; when Limiter Mix is below 100%, or downstream Output gain is added, peaks can still exceed the indicated ceiling. This is not yet certified true-peak limiting.

## Channel layout

```
Input -> shared stereo 4x oversampling -> LR4 crossover (120–800 Hz)
                                     |                |
                         LOW (optional mono)       HIGH (stereo remains)
                         Order dropdown:           Order dropdown:
                         [EQ | Drive | Comp]       [EQ | Drive | Comp]
                         all 6 permutations        all 6 permutations
                         40/120/220 Hz EQ          1.5k/5k Hz EQ
                         asymmetric drive          symmetrical 720 Hz / TS-like drive
                         FET/VCA/OPTO + ratio      FET/VCA/OPTO + ratio
                                     |                |
                                  LOW BLEND      HIGH BLEND
                                     +----------------+
                                              |
                                stereo-linked glue + lookahead limiter
                                              |
                                       downsample -> OUTPUT
```

**EQ note:** Low EQ is a documented *1073/1084 family approximation* (broad shelves and small response divots); high EQ uses *550 family proportional-Q behavior*. No physical-unit frequency-response grid is embedded; do not claim sample-identical hardware EQ curves.

## Opto color targets (20 dB gain reduction, individual harmonic ratios in dBc)

| Signal frequency | K2 | K3 | K4 | K5 |
| --- | ---: | ---: | ---: | ---: |
| 100 Hz | -76 | -88 | -83 | — |
| 1 kHz | -71 | -84 | -78 | -92 |
| 5 kHz | -88 | — | — | — |
| 10 kHz | -99 | — | — | — |

The signal is split into four very gentle analysis bands **for harmonic residual generation only**; main audio remains unfiltered in the compressor color stage. Coefficients are scaled by `min(GR/20 dB, 1)`. The dBc targets are *design anchors*: response at intermediate frequencies, between source bands and with multi-tone program material is approximate and requires real plugin/Plugin Doctor measurements. Never call this a calibrated optical cell.

## References / calibration caveats

- Ibanez TS808 feedback-network values (4.7 kΩ / 47 nF, ~720 Hz pre-emphasis, diode clipping): https://bench.swellamps.com/overdrive-opamp-1979/
- UREI 1176LN manual (4/8/12/20 ratios; 20–800 μs / 50–1100 ms, <0.5% THD under its published test conditions): https://thehistoryofrecording.com/Manuals/UREI/Urei%201176LN%2001.pdf
- Original dbx 160/161 manual (15/5/3 ms step-dependent attack, 120 dB/s release, 1:1 to 120:1): https://device.report/m/14f6be6867c8f4c7ffc98e55d049eea98f3b8157ca09eb1f8f792003136199e6
- THAT 2181 datasheet (at 1 kHz, very low measured THD depending on signal/gain): https://www.thatcorp.com/datashts/THAT_2181-Series_Datasheet.pdf

## Build from source

Configure with `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`, then `cmake --build build --config Release --target Thump_VST3`. CMake FetchContent downloads JUCE 8; offline development can set `-DJUCE_LOCAL_PATH=...`.

### Checks

```
python tests/validate_dsp.py
python tests/test_eq_curves.py
python tests/test_drive_and_gui.py
python tests/test_thump_v04.py
python tests/test_v05_ratio_phase.py
```

These are mathematical regressions, not a compiled-plugin listening test. **A passing CI build only establishes successful compilation and the supplied tests, not hardware emulation fidelity**. Test THD sweeps, stereo phase, high-frequency aliasing, compressor ratios at settled levels, and peak ceiling in REAPER/Plugin Doctor after installing.
