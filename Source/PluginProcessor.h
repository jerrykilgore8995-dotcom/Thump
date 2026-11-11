#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace thump
{
constexpr float pi = juce::MathConstants<float>::pi;
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float x) noexcept { return 20.0f * std::log10 (juce::jmax (x, 1.0e-9f)); }

// Transposed DF-II audio biquad. Butterworth high/low sections have Q=1/sqrt(2).
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() noexcept { z1 = z2 = 0; }
    void lowPass (float hz, float q, double fs) noexcept;
    void highPass (float hz, float q, double fs) noexcept;
    void bell (float hz, float q, float gainDb, double fs) noexcept;
    void lowShelf (float hz, float qControl, float gainDb, double fs) noexcept;
    inline float run (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

struct Band
{
    Biquad lp[2], hp[2], eq[3], tubePreHP, tubePostLP, lowSatDC;

    void reset() noexcept;
};

// Shared L/R gain-reduction detector prevents stereo image wandering.
struct Detector
{
    float grDb = 0.0f, rms2 = 0.0f, releaseOrigin = 0.0f;
    int releaseSample = 0, previousMode = -1;
    bool optoReleasing = false;
    float wetBlend = 1.0f;
    void reset() noexcept { *this = {}; }
};

struct State
{
    float inputDb = 0, outputDb = 0, crossover = 320;
    bool mono = true, lowSatOn = true, lowCompOn = true, highSatOn = true,
         highCompOn = true, limiterOn = true;
    float lowDrive = 2.8f, lowSatMix = 0.18f, highDrive = 3.8f, highSatMix = 0.38f;
    float lowMix = 1, highMix = 0.72f;
    float eqLowGain[3] = {0, 0, 0}, eqLowQ[3] = {1.0f, 1.0f, 1.0f};
    float eqHighGain[2] = {0, 0}, eqHighQ[2] = {1.0f, 1.0f};
    int lowMode = 1, highMode = 1; // 0 FET, 1 VCA, 2 OPTO
    float lowThresh = -24, lowAttackMs = 0.2f, lowReleaseMs = 250, lowMakeupDb = 2;
    float highThresh = -22, highAttackMs = 0.2f, highReleaseMs = 180, highMakeupDb = 1;
    float limiterInputDb = 0, limiterCeilingDb = -1.0f, limiterReleaseMs = 90, limiterMix = 1;
};

// Original eight factory presets, not recalls of anyone else's session settings.
struct Preset { const char* name; const char* description; std::array<float, 39> values; };
const std::array<Preset, 8>& factoryPresets();
const std::array<const char*, 39>& presetParameterIds();
}

class ThumpAudioProcessor final : public juce::AudioProcessor
{
public:
    ThumpAudioProcessor();
    ~ThumpAudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "THUMP"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Current Settings"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    juce::AudioProcessorValueTreeState apvts;
    void applyFactoryPreset (int index);
    int getSelectedPreset() const noexcept { return selectedPreset.load(); }
    std::atomic<float> inMeterDb {-60}, outMeterDb {-60}, lowReduction {0}, highReduction {0}, limitReduction {0};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    thump::State readParams() const noexcept;
    void updateFilters (const thump::State&) noexcept;
    void runCompressor (float& left, float& right, thump::Detector& detector,
                        bool enabled, int mode, float threshold, float attackMs,
                        float releaseMs, float makeupDb) noexcept;
    float runLowSat (float x, int channel, const thump::State&) noexcept;
    float runHighSat (float x, int channel, const thump::State&) noexcept;

    // A full-band shared 2x oversampling path keeps the two crossover branches aligned.
    juce::dsp::Oversampling<float> oversampling {2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true};
    std::array<thump::Band, 2> bands;
    thump::Detector lowDetector, highDetector;
    juce::AudioBuffer<float> stereoWork; // also makes mono hosts safe with a stereo oversampler
    std::array<std::array<thump::Biquad, 2>, 2> highEq;
    std::vector<float> delayL, delayR;
    int delayIndex = 0, lookaheadOS = 1;
    float detectorPeak = 0;
    double processingSampleRate = 88200;
    std::atomic<int> selectedPreset {-1};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThumpAudioProcessor)
};
