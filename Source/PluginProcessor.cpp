#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <utility>

namespace thump
{
static void setRBJ (Biquad& f, float hz, float q, double fs, int kind, float gainDb = 0) noexcept
{
    const float w = 2.0f * pi * juce::jlimit (5.0f, (float) fs * 0.475f, hz) / (float) fs;
    const float c = std::cos (w), s = std::sin (w);
    const float a = s / (2.0f * juce::jmax (0.1f, q));
    float A0 = 1 + a, A1 = -2*c, A2 = 1-a;
    float B0 = 1, B1 = -2*c, B2 = 1;
    if (kind == 0) { B0 = (1-c)*0.5f; B1 = 1-c; B2 = B0; }
    if (kind == 1) { B0 = (1+c)*0.5f; B1 = -(1+c); B2 = B0; }
    if (kind == 2)
    {
        const float g = std::pow (10.0f, gainDb / 40.0f);
        B0 = 1 + a*g; B1 = -2*c; B2 = 1-a*g;
        A0 = 1 + a/g; A1 = -2*c; A2 = 1-a/g;
    }
    f.b0 = B0/A0; f.b1 = B1/A0; f.b2 = B2/A0;
    f.a1 = A1/A0; f.a2 = A2/A0;
}
// Neve 1073 / 1084 LOW shelf family: the primary response uses an RBJ shelf
// with a deliberately broad transition. The separate mild inverse bump in
// Band::lowDivot reconstructs the known small opposite-sign response above the
// shelf corner (UAD / GroupDIY 1073 circuit measurements). This is a documented
// shape approximation, NOT a calibration to a particular physical unit.
// Panel frequencies 40 / 120 / 220 Hz are user-selected adaptations of the
// originals 35 / 60 / 110 / 220 Hz; stock Q trim remains 1.00.
void Biquad::lowShelf (float hz, float qControl, float gainDb, double fs) noexcept
{
    if (std::abs (gainDb) < 0.00001f)
    {
        b0 = 1.f; b1 = b2 = a1 = a2 = 0.f;
        return;
    }
    const float w = 2.f * pi * juce::jlimit (5.f, (float)fs * .475f, hz) / (float)fs;
    const float c = std::cos (w), sine = std::sin (w);
    const float A = std::pow (10.f, gainDb / 40.f);
    // qControl is relative to the panel's stock (1.00) position.
    const float shelfSlope = juce::jlimit (.26f, 1.45f,
                                           .78f * std::pow (qControl, .62f));
    const float alpha = .5f * sine * std::sqrt (juce::jmax (0.0001f,
                        (A + 1.f/A)*(1.f/shelfSlope - 1.f) + 2.f));
    const float beta = 2.f * std::sqrt (A) * alpha;
    const float a0 = (A+1.f) + (A-1.f)*c + beta;
    const float ab1 = -2.f*((A-1.f) + (A+1.f)*c);
    const float ab2 = (A+1.f) + (A-1.f)*c - beta;
    const float bb0 = A*((A+1.f) - (A-1.f)*c + beta);
    const float bb1 = 2.f*A*((A-1.f) - (A+1.f)*c);
    const float bb2 = A*((A+1.f) - (A-1.f)*c - beta);
    b0=bb0/a0; b1=bb1/a0; b2=bb2/a0;
    a1=ab1/a0; a2=ab2/a0;
}

// API 550/550A/550B proportional-Q family. The real 550 (not the 550A-1)
// gets appreciably narrower when gain grows. Interpolate at the original
// stepped amounts for a smooth +-12dB slider; 1.00 = stock model Q multiplier.
// Approximate response-matched control points, not digitized factory measurements.
float proportionalQ (float gainDb, float qControl) noexcept
{
    static constexpr float g[] = { 0.f, 2.f, 4.f, 6.f, 9.f, 12.f };
    static constexpr float q[] = { .39f, .48f, .66f, .88f, 1.20f, 1.55f };
    const float absGain = juce::jlimit (0.f, 12.f, std::abs (gainDb));
    float stockQ = q[5];
    for (int i=1; i<6; ++i)
        if (absGain <= g[i])
        {
            stockQ = juce::jmap (absGain, g[i-1], g[i], q[i-1], q[i]);
            break;
        }
    return juce::jlimit (.15f, 4.8f, stockQ * qControl);
}

void Biquad::lowPass (float f, float q, double sr) noexcept { setRBJ (*this, f, q, sr, 0); }
void Biquad::highPass (float f, float q, double sr) noexcept { setRBJ (*this, f, q, sr, 1); }
void Biquad::bell (float f, float q, float g, double sr) noexcept { setRBJ (*this, f, q, sr, 2, g); }
void Band::reset() noexcept
{
    for (auto& f : lp) f.reset();
    for (auto& f : hp) f.reset();
    for (auto& f : eq) f.reset();
    for (auto& f : lowDivot) f.reset();
    for (auto& f : inputHp) f.reset();
    dynamicEq.reset();
    tubePreHP.reset(); tubePostLP.reset();
    ampPreHP.reset(); ampPostLP.reset(); fuzzPreHP.reset(); fuzzPostLP.reset();
    lowSatDC.reset();
    lowMatch.reset(); highMatch.reset();
    hpf12Blend = hpf24Blend = ampBlend = fuzzBlend = 0.f;
    lowSatBlend = highSatBlend = 1.f;

}

const std::array<const char*, 39>& presetParameterIds()
{
    static const std::array<const char*, 39> ids = {
        "input", "output", "xover", "mono", "lowSatOn", "lowDrive",
        "lowSatMix", "low40G", "low40Q", "low120G", "low120Q", "low220G",
        "low220Q", "lowCompOn", "lowMode", "lowThresh", "lowAttack", "lowRelease",
        "lowMakeup", "lowMix", "highSatOn", "highDrive", "highSatMix", "high1500G",
        "high1500Q", "high5000G", "high5000Q", "highCompOn", "highMode", "highThresh",
        "highAttack", "highRelease", "highMakeup", "highMix", "limiterOn", "limiterInput",
        "limiterCeiling", "limiterRelease", "limiterMix" };
    return ids;
}

// Factory presets are original bass-mixing STARTING POINTS, never exact artist/engineer settings.
const std::array<Preset, 8>& factoryPresets()
{
    static const std::array<Preset, 8> presets = {{
        { "Modern Metal Lock", "Tight, steady subs and enough mid grit to survive a guitar wall.",
          { 0.0f, 0.0f, 290.0f, 1.0f, 1.0f, 3.2f, 0.17f, 0.0f, 0.7f, 2.0f, 0.85f, -3.0f, 1.0f, 1.0f, 1.0f, -25.0f, 0.2f, 220.0f, 2.5f, 0.95f, 1.0f, 4.5f, 0.46f, 3.0f, 0.95f, 1.0f, 1.0f, 1.0f, 0.0f, -22.0f, 0.1f, 85.0f, 1.0f, 0.78f, 1.0f, 3.5f, -1.0f, 75.0f, 1.0f } },
        { "Classic Rock Body", "Broad 120 Hz warmth with articulate but restrained 1.5 kHz.",
          { 0.0f, 0.0f, 370.0f, 1.0f, 1.0f, 2.3f, 0.12f, 1.0f, 0.55f, 2.5f, 0.7f, -1.5f, 0.8f, 1.0f, 2.0f, -22.0f, 0.45f, 320.0f, 2.0f, 0.95f, 1.0f, 2.6f, 0.22f, 1.5f, 0.8f, -0.5f, 0.9f, 1.0f, 1.0f, -18.0f, 0.28f, 155.0f, 1.0f, 0.67f, 1.0f, 4.0f, -0.8f, 110.0f, 0.9f } },
        { "Punk Pick Attack", "Prominent string click and 1.5 kHz edge, with tight lows.",
          { 0.0f, 0.0f, 280.0f, 1.0f, 1.0f, 2.8f, 0.13f, -1.0f, 0.8f, 1.5f, 0.9f, -2.5f, 1.1f, 1.0f, 0.0f, -24.0f, 0.1f, 180.0f, 2.0f, 0.9f, 1.0f, 5.8f, 0.58f, 5.0f, 0.95f, 3.0f, 1.1f, 1.0f, 0.0f, -20.0f, 0.08f, 65.0f, 1.0f, 0.9f, 1.0f, 6.0f, -1.0f, 55.0f, 1.0f } },
        { "Drop Tuned Grind", "Preserve sub notes while moving distortion into the upper harmonics.",
          { 0.0f, 0.0f, 210.0f, 1.0f, 1.0f, 3.3f, 0.18f, 1.0f, 0.75f, 2.0f, 0.8f, -4.0f, 1.1f, 1.0f, 1.0f, -27.0f, 0.25f, 380.0f, 3.0f, 0.98f, 1.0f, 6.5f, 0.55f, 4.0f, 1.1f, 1.5f, 1.3f, 1.0f, 1.0f, -25.0f, 0.2f, 130.0f, 1.5f, 0.78f, 1.0f, 6.0f, -1.2f, 90.0f, 1.0f } },
        { "CLA-Inspired Bite", "Inspired by CLA's 1176-controlled bass, top-end EQ and distortion choices.",
          { 0.0f, 0.0f, 330.0f, 1.0f, 1.0f, 2.5f, 0.18f, 0.0f, 0.7f, 1.5f, 0.8f, -2.0f, 0.95f, 1.0f, 0.0f, -28.0f, 0.08f, 175.0f, 3.0f, 0.95f, 1.0f, 5.4f, 0.48f, 4.5f, 0.9f, 3.0f, 1.2f, 1.0f, 0.0f, -21.0f, 0.07f, 90.0f, 1.5f, 0.88f, 1.0f, 3.5f, -1.0f, 80.0f, 1.0f } },
        { "Scheps-Inspired Parallel", "Retain natural fundamentals; blend in grit and limited compression.",
          { 0.0f, 0.0f, 340.0f, 1.0f, 1.0f, 2.4f, 0.09f, 0.0f, 0.65f, 1.0f, 0.75f, -1.0f, 0.9f, 0.0f, 2.0f, -20.0f, 0.45f, 280.0f, 0.0f, 0.98f, 1.0f, 4.5f, 0.39f, 5.0f, 0.9f, 0.0f, 1.1f, 0.0f, 1.0f, -20.0f, 0.32f, 145.0f, 0.0f, 0.82f, 1.0f, 4.0f, -1.2f, 110.0f, 0.48f } },
        { "TLA-Inspired Crush", "Aggressive dense compression informed by TLA's heavy bass-control philosophy.",
          { 0.0f, 0.0f, 310.0f, 1.0f, 1.0f, 3.2f, 0.22f, 0.0f, 0.8f, 1.5f, 0.75f, -3.0f, 1.1f, 1.0f, 0.0f, -34.0f, 0.05f, 190.0f, 5.0f, 0.98f, 1.0f, 5.0f, 0.44f, 3.5f, 1.1f, 2.5f, 1.2f, 1.0f, 0.0f, -30.0f, 0.06f, 65.0f, 3.0f, 0.85f, 1.0f, 6.0f, -1.2f, 65.0f, 1.0f } },
        { "Staub-Inspired Edge", "A dense modern-rock balance: firm lows, bright articulation and restrained limiting.",
          { 0.0f, 0.0f, 320.0f, 1.0f, 1.0f, 3.0f, 0.16f, -0.5f, 0.8f, 2.0f, 0.75f, -2.5f, 1.1f, 1.0f, 1.0f, -27.0f, 0.22f, 240.0f, 3.0f, 0.96f, 1.0f, 5.0f, 0.44f, 3.5f, 1.1f, 2.0f, 1.15f, 1.0f, 0.0f, -24.0f, 0.13f, 105.0f, 2.0f, 0.83f, 1.0f, 5.0f, -1.0f, 85.0f, 0.95f } }
    }};
    return presets;
}
}
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto fl = [&](const char* id, const char* name, float mn, float mx, float step, float value)
    {
        p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {id,1}, name,
            juce::NormalisableRange<float> {mn,mx,step}, value));
    };
    auto bl = [&](const char* id, const char* name, bool value)
    { p.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID {id,1}, name,value)); };
    fl ("input", "Input", -24, 24, .1f, 0); fl ("output", "Output", -24, 24, .1f, 0);
    fl ("xover", "Crossover Hz", 120, 800, 1, 320); bl ("mono", "MONOIZE", true);
    fl ("hpfHz", "Pre-split HPF Hz", 20, 45, .1f, 30);
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"hpfSlope",1},
        "Pre-split HPF Slope", juce::StringArray {"Off", "12 dB/oct", "24 dB/oct"}, 0));
    bl ("dynOn", "Dynamic Low EQ", false);
    fl ("dynHz", "Dynamic EQ Frequency Hz", 70, 140, .5f, 100);
    fl ("dynThresh", "Dynamic EQ Threshold dBFS", -45, -6, .1f, -24);
    fl ("dynCut", "Dynamic EQ Max Cut dB", 0, 8, .1f, 3);
    bl ("lowSatOn", "Low Saturation", true); fl ("lowDrive", "Low Drive", 1, 10, .01f, 2.8f);
    fl ("lowSatMix", "Low Saturation Mix", 0, 1, .001f, .18f);
    for (const char* id : {"low40G", "low120G", "low220G"}) fl (id, "Low EQ Gain", -12, 12, .1f, 0);
    for (const char* id : {"low40Q", "low120Q", "low220Q"}) fl (id, "Low Shelf Width (1 = Stock)", .3f, 3, .01f, 1.0f);
    bl ("lowCompOn", "Low Compressor", true);
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"lowMode",1},
        "Low Compressor Model", juce::StringArray {"FET 1176", "VCA 160", "Opto Dual Slope"}, 1));
    fl ("lowThresh", "Low Compressor Threshold", -45, 0, .1f, -24);
    fl ("lowAttack", "Low FET Attack ms", .02f, .8f, .001f, .2f);
    fl ("lowRelease", "Low FET Release ms", 50, 1100, 1, 250);
    fl ("lowMakeup", "Low Makeup", -12, 18, .1f, 2);
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"lowRoute",1},
        "Low Processing Order", juce::StringArray {"EQ > Drive > Comp", "EQ > Comp > Drive",
        "Drive > EQ > Comp", "Drive > Comp > EQ", "Comp > EQ > Drive", "Comp > Drive > EQ"}, 0));
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"lowFetRatio",1},
        "Low FET Ratio", juce::StringArray {"4:1", "8:1", "12:1", "20:1"}, 0));
    fl ("lowVcaRatio", "Low VCA Ratio", 1, 120, .1f, 4);
    fl ("lowMix", "Low Blend", 0, 1, .001f, 1);
    bl ("highSatOn", "High Tube Style Saturation", true);
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"highDriveType",1},
        "High Drive Character", juce::StringArray {"TS Grind", "Amp Grind", "Fuzz Clank"}, 0));
    fl ("highDrive", "High Drive", 1, 10, .01f, 3.8f);
    fl ("highSatMix", "High Saturation Mix", 0, 1, .001f, .38f);
    for (const char* id : {"highMidG", "high1500G", "high5000G"})
        fl (id, "High EQ Gain", -12, 12, .1f, 0);
    for (const char* id : {"highMidQ", "high1500Q", "high5000Q"})
        fl (id, "High Proportional Q Trim (1 = Stock)", .3f, 3, .01f, 1.0f);
    fl ("highMidHz", "High Sweep EQ Frequency", 350, 1500, 1, 800);
    bl ("highCompOn", "High Compressor", true);
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"highMode",1},
        "High Compressor Model", juce::StringArray {"FET 1176", "VCA 160", "Opto Dual Slope"}, 1));
    fl ("highThresh", "High Compressor Threshold", -45, 0, .1f, -22);
    fl ("highAttack", "High FET Attack ms", .02f, .8f, .001f, .2f);
    fl ("highRelease", "High FET Release ms", 50, 1100, 1, 180);
    fl ("highMakeup", "High Makeup", -12, 18, .1f, 1);
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"highRoute",1},
        "High Processing Order", juce::StringArray {"EQ > Drive > Comp", "EQ > Comp > Drive",
        "Drive > EQ > Comp", "Drive > Comp > EQ", "Comp > EQ > Drive", "Comp > Drive > EQ"}, 0));
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"highFetRatio",1},
        "High FET Ratio", juce::StringArray {"4:1", "8:1", "12:1", "20:1"}, 0));
    fl ("highVcaRatio", "High VCA Ratio", 1, 120, .1f, 4);
    fl ("highMix", "High Blend", 0, 1, .001f, .72f);
    bl ("limiterOn", "Final Limiter", true);
    bl ("glueOn", "Limiter Glue Compressor", true);
    fl ("limiterInput", "Limiter Drive", -12, 18, .1f, 4.0f);
    fl ("limiterCeiling", "Limiter Ceiling", -12, 0, .1f, -1);
    fl ("limiterRelease", "Limiter Release ms", 40, 300, 1, 90);
    fl ("limiterMix", "Limiter Mix", 0, 1, .001f, 1);
    return p;
}

juce::AudioProcessorValueTreeState::ParameterLayout ThumpAudioProcessor::makeLayout()
{ return createParameterLayout(); }

ThumpAudioProcessor::ThumpAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "THUMP_PARAMS", makeLayout())
{}

bool ThumpAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
        && (l.getMainInputChannelSet() == juce::AudioChannelSet::mono()
            || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}

void ThumpAudioProcessor::prepareToPlay (double fs, int maxBlock)
{
    processingSampleRate = fs * 4.0;
    wetMatchCoeff = 1.f - std::exp (-1.f / (.035f * (float)processingSampleRate));
    colorPoleCoeff = {{250.f,2200.f,7500.f}};
    for (auto& c : colorPoleCoeff)
        c = 1.f - std::exp (-2.f*thump::pi*c/(float)processingSampleRate);
    oversampling.initProcessing ((size_t) maxBlock);
    oversampling.reset();
    for (auto& b : bands) b.reset();
    lowDetector.reset(); highDetector.reset();
    filterInitialized = false;
    dynamicRms2 = dynamicCutDb = 0.f;
    dynamicCounter = 0;
    const auto initial = readParams();
    smoothInputGain = thump::dbToGain (initial.inputDb);
    smoothOutputGain = thump::dbToGain (initial.outputDb);
    smoothLowDrive=initial.lowDrive; smoothHighDrive=initial.highDrive;
    smoothLowSatMix=initial.lowSatMix; smoothHighSatMix=initial.highSatMix;
    hostGainSmoothCoeff = 1.f - std::exp (-1.f/(.008f*(float)fs));
    parameterSmoothCoeff = 1.f - std::exp (-1.f/(.010f*(float)processingSampleRate));
    bypassCoeff = std::exp (-1.f/(.008f*(float)processingSampleRate));
    optoAttackCoeff = std::exp (-1000.f/(20.f*(float)processingSampleRate));
    rmsVcaCoeff = std::exp(-1.f/(.003f*(float)processingSampleRate));
    rmsOptoCoeff = std::exp(-1.f/(.007f*(float)processingSampleRate));
    colorDcCoeff = std::exp (-2.f*thump::pi*14.f/(float)processingSampleRate);
    for (int idx=0; idx<=200; ++idx)
    {
        const float above = 10.f + idx*.1f;
        const float ms = above<=20.f ? juce::jmap (above,10.f,20.f,15.f,5.f)
                                     : juce::jmap (above,20.f,30.f,5.f,3.f);
        vcaAttackLookup[(size_t)idx] = std::exp (-1000.f/(ms*(float)processingSampleRate));
    }
    stereoWork.setSize (2, maxBlock, false, false, true);
    stereoWork.clear();
    for (auto& pair : highEq) for (auto& eq : pair) eq.reset();
    lookaheadOS = juce::jmax (1, (int) std::ceil (fs * .001)) * 4;
    delayL.assign ((size_t) lookaheadOS, 0.0f);
    delayR.assign ((size_t) lookaheadOS, 0.0f);
    delayIndex = 0; detectorPeak = 0; glueGrDb = 0; limiterBlend = 1.f; glueBlend = 1.f;
    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()) + lookaheadOS / 4);
    updateFilters (readParams());
}
void ThumpAudioProcessor::releaseResources() {}

thump::State ThumpAudioProcessor::readParams() const noexcept
{
    auto x = [&](const char* id) { return apvts.getRawParameterValue (id)->load(); };
    thump::State p;
    p.inputDb=x("input"); p.outputDb=x("output"); p.crossover=x("xover"); p.mono=x("mono")>0.5f;
    p.hpfHz=x("hpfHz"); p.hpfSlope=(int)x("hpfSlope");
    p.dynamicOn=x("dynOn")>0.5f; p.dynamicHz=x("dynHz");
    p.dynamicThreshold=x("dynThresh"); p.dynamicMaxCut=x("dynCut");
    p.lowSatOn=x("lowSatOn")>0.5f; p.lowDrive=x("lowDrive"); p.lowSatMix=x("lowSatMix");
    p.eqLowGain[0]=x("low40G"); p.eqLowQ[0]=x("low40Q");
    p.eqLowGain[1]=x("low120G"); p.eqLowQ[1]=x("low120Q");
    p.eqLowGain[2]=x("low220G"); p.eqLowQ[2]=x("low220Q");
    p.lowCompOn=x("lowCompOn")>0.5f; p.lowMode=(int)x("lowMode");
    p.lowRoute=(int)x("lowRoute"); p.lowFetRatio=(int)x("lowFetRatio");
    p.lowVcaRatio=x("lowVcaRatio");
    p.lowThresh=x("lowThresh"); p.lowAttackMs=x("lowAttack");
    p.lowReleaseMs=x("lowRelease"); p.lowMakeupDb=x("lowMakeup"); p.lowMix=x("lowMix");
    p.highSatOn=x("highSatOn")>0.5f; p.highDrive=x("highDrive"); p.highSatMix=x("highSatMix");
    p.highDriveType=(int)x("highDriveType");
    p.highMidHz=x("highMidHz");
    p.eqHighGain[2]=x("high5000G"); p.eqHighQ[2]=x("high5000Q");
    p.eqHighGain[0]=x("highMidG"); p.eqHighQ[0]=x("highMidQ");
    p.eqHighGain[1]=x("high1500G"); p.eqHighQ[1]=x("high1500Q");
    p.highCompOn=x("highCompOn")>0.5f; p.highMode=(int)x("highMode");
    p.highRoute=(int)x("highRoute"); p.highFetRatio=(int)x("highFetRatio");
    p.highVcaRatio=x("highVcaRatio");
    p.highThresh=x("highThresh"); p.highAttackMs=x("highAttack");
    p.highReleaseMs=x("highRelease"); p.highMakeupDb=x("highMakeup"); p.highMix=x("highMix");
    p.limiterOn=x("limiterOn")>0.5f; p.glueOn=x("glueOn")>0.5f;
    p.limiterInputDb=x("limiterInput");
    p.limiterCeilingDb=x("limiterCeiling"); p.limiterReleaseMs=x("limiterRelease"); p.limiterMix=x("limiterMix");
    return p;
}

void ThumpAudioProcessor::updateFilters (const thump::State& s) noexcept
{
    constexpr float qButter = 0.70710678f;
    for (size_t ch=0; ch<bands.size(); ++ch)
    {
        auto& b = bands[ch];
        const float highHz[3] {s.highMidHz, 1500.f, 5000.f};
        for (int k=0;k<3;++k)
            highEq[ch][(size_t)k].bell (highHz[k],
                thump::proportionalQ (s.eqHighGain[k], s.eqHighQ[k]),
                s.eqHighGain[k], processingSampleRate);
        for (auto& f : b.lp) f.lowPass (s.crossover, qButter, processingSampleRate);
        for (auto& f : b.hp) f.highPass (s.crossover, qButter, processingSampleRate);
        for (auto& f : b.inputHp) f.highPass (s.hpfHz, qButter, processingSampleRate);
        b.dynamicEq.bell (s.dynamicHz, .85f, -dynamicCutDb, processingSampleRate);
        for (int i=0; i<3; ++i)
        {
            const float hz = i==0 ? 40.f : i==1 ? 120.f : 220.f;
            b.eq[i].lowShelf (hz, s.eqLowQ[i], s.eqLowGain[i], processingSampleRate);
            // Original 1073/1084 shelves show a shallow inverse-response
            // 'divot' above the bend. The reference material documents the
            // behavior, but does not give an exact gain/frequency grid.
            const float inverseDb = -s.eqLowGain[i] * .080f;
            b.lowDivot[i].bell (hz * 2.05f,
                                juce::jlimit (.38f, 1.9f, .70f * std::sqrt (s.eqLowQ[i])),
                                inverseDb, processingSampleRate);
        }
        b.tubePreHP.highPass (720, qButter, processingSampleRate);
        b.tubePostLP.lowPass (5200, qButter, processingSampleRate);
        b.ampPreHP.highPass (350, qButter, processingSampleRate);
        b.ampPostLP.lowPass (6800, qButter, processingSampleRate);
        b.fuzzPreHP.highPass (250, qButter, processingSampleRate);
        b.fuzzPostLP.lowPass (4500, qButter, processingSampleRate);
        b.lowSatDC.highPass (18, qButter, processingSampleRate);
    }
}

// Level compensator: slow, bounded RMS feedback prevents switching distortion
// on (or moving Drive) from pushing the next EQ/compressor several dB harder.
// 35ms tracks musical level without trying to linearize instantaneous clipping.
float ThumpAudioProcessor::levelMatchedWet (float dry, float wet, thump::MatchState& m) noexcept
{
    const float a = wetMatchCoeff;
    m.dryPower += a*(dry*dry - m.dryPower);
    m.wetPower += a*(wet*wet - m.wetPower);
    const float target = std::sqrt ((m.dryPower+1.e-9f)/(m.wetPower+1.e-9f));
    m.compGain += a * (juce::jlimit (.15f, 6.f, target)-m.compGain);
    return wet * m.compGain;
}

float ThumpAudioProcessor::runLowSat (float x, int channel, const thump::State& s) noexcept
{
    auto& b = bands[(size_t)channel];
    b.lowSatBlend += parameterSmoothCoeff*((s.lowSatOn ? 1.f : 0.f)-b.lowSatBlend);
    if (!s.lowSatOn && b.lowSatBlend < 1.e-5f) return x;
    // Asymmetric biased high-gain nonlinearity: unlike the old 2% residual,
    // at Drive 10 this generates strong K2 and K3 (about 20-30% THD for a
    // -15dBFS bass sine with a full wet path); AC-couple residual only.
    const float t = juce::jlimit (0.f,1.f,(s.lowDrive-1.f)/9.f);
    const float k = .3f + 2.7f*t*t;
    const float bias = .15f + .32f*t;
    const float reference = .18f;
    // Linearized at Drive 1, normalized with a fundamental-level calibration
    // fit rather than simple peak-based makeup gain.
    const float normalizer = juce::jmax (.12f,
        k*(1.f-.16f*bias) / (1.f+.34f*k*k));
    const float z = juce::jlimit(-5.f,5.f,x/reference);
    const float shaped = reference * (std::tanh(k*z + bias)-std::tanh(bias))/normalizer;
    const float residual=b.lowSatDC.run(shaped-x);
    const float wet=levelMatchedWet(x,x+residual,b.lowMatch);
    return x + b.lowSatBlend*s.lowSatMix*(wet-x);
}
float ThumpAudioProcessor::runHighSat (float x, int channel, const thump::State& s) noexcept
{
    auto& b=bands[(size_t)channel];
    b.highSatBlend += parameterSmoothCoeff*((s.highSatOn ? 1.f : 0.f)-b.highSatBlend);
    b.ampBlend += parameterSmoothCoeff*((s.highDriveType==1 ? 1.f : 0.f)-b.ampBlend);
    b.fuzzBlend += parameterSmoothCoeff*((s.highDriveType==2 ? 1.f : 0.f)-b.fuzzBlend);
    if (!s.highSatOn && b.highSatBlend < 1.e-5f) return x;
    const float t=juce::jlimit(0.f,1.f,(s.highDrive-1.f)/9.f);
    const float reference=.18f;
    // All paths keep independent pre/post filter history. Smoothed weights
    // prevent hard jumps when switching types during automation.
    const float tsIn=b.tubePreHP.run(x);
    const float tsK=.35f+4.1f*t*t;
    const float tsZ=juce::jlimit(-5.f,5.f,tsIn/reference);
    const float tsNorm=juce::jmax(.12f,tsK/(1.f+.32f*tsK*tsK));
    const float ts=b.tubePostLP.run(reference*std::tanh(tsK*tsZ)/tsNorm);
    const float ampIn=b.ampPreHP.run(x);
    const float ampZ=juce::jlimit(-5.f,5.f,ampIn/reference);
    const float ampK=.7f+5.f*t*t;
    const float ampBias=.1f+.36f*t;
    const float amp=b.ampPostLP.run(reference*
        (std::tanh(ampK*ampZ+ampBias)-std::tanh(ampBias)));
    const float fuzzIn=b.fuzzPreHP.run(x);
    const float fuzzZ=juce::jlimit(-5.f,5.f,fuzzIn/reference);
    const float fuzz=b.fuzzPostLP.run(reference*
        juce::jlimit(-.75f,.75f,fuzzZ*(1.0f+9.f*t)));
    const float tsBlend=juce::jmax(0.f,1.f-b.ampBlend-b.fuzzBlend);
    const float wet=tsBlend*ts+b.ampBlend*amp+b.fuzzBlend*fuzz;
    const float matched=levelMatchedWet(x,wet,b.highMatch);
    return x + b.highSatBlend*s.highSatMix*(matched-x);
}

// Explicit, frequency-dependent compressor coloration. Four prefiltered source
// bands generate ONLY a small parallel nonlinear residual. The processed audio
// itself remains on its original unfiltered path (no additional all-pass).
float ThumpAudioProcessor::compressorColor (float x, float grDb, int mode,
                                           thump::ColorChannel& state) noexcept
{
    const float fs=(float)processingSampleRate;
    state.lp250 += colorPoleCoeff[0]*(x-state.lp250);
    state.lp2200 += colorPoleCoeff[1]*(x-state.lp2200);
    state.lp7500 += colorPoleCoeff[2]*(x-state.lp7500);
    const float low=state.lp250;
    const float lowMid=state.lp2200;
    const float high=state.lp7500;
    const float segments[4] {low,lowMid-low,high-lowMid,x-high};
    // Gain-reduction-scaled THD. Opto harmonic amplitudes follow the supplied
    // -dBc reference ladders at 100, 1k, 5k, 10k respectively, at 20dB GR.
    // Actual frequencies between anchors and polyphonic signals are approximate.
    static constexpr float optoLinear[4][4] = {
        {0.0001584893f, 0.0000398107f, 0.0000707946f, 0.0000010000f},
        {0.0002818383f, 0.0000630957f, 0.0001258925f, 0.0000251189f},
        {0.0000398107f, 0.0000017783f, 0.0000005623f, 0.0000001778f},
        {0.0000112202f, 0.0000005623f, 0.0000001778f, 0.0000000562f}
    };
    const float fraction=juce::jlimit(0.f,1.f,grDb/20.f);
    if (fraction < 1.e-6f) return x;
    float residual=0.f;
    for(int i=0;i<4;++i)
    {
        const float z=juce::jlimit(-1.2f,1.2f,segments[i]/.18f);
        const float z2=z*z, z3=z2*z, z4=z2*z2, z5=z4*z;
        const float harmonics[4] {2.f*z2,4.f*z3-3.f*z,8.f*z4-8.f*z2,16.f*z5-20.f*z3+5.f*z};
        float coeff[4] {};
        if (mode==2)
            for(int n=0;n<4;++n) coeff[n]=optoLinear[i][n];
        else if (mode==1)
        {
            // THAT 2181 datasheet THD is much lower. This is intentionally
            // enhanced dbx-style VCA coloring, not a claim of 2181 accuracy.
            const float lowWeight = i==0?1.0f:i==1?.60f:i==2?.24f:.10f;
            coeff[0]=.0025f*lowWeight;
            coeff[1]=.0180f*lowWeight;
            coeff[2]=.0012f*lowWeight;
        }
        else
        {
            // FET/transformer-inspired, stronger low-mid K2/K3, weaker top.
            const float lowWeight=i==0?1.0f:i==1?.65f:i==2?.30f:.15f;
            coeff[0]=.0040f*lowWeight;
            coeff[1]=.0045f*lowWeight;
            coeff[2]=.0015f*lowWeight;
        }
        for(int n=0;n<4;++n) residual += .18f * coeff[n] * harmonics[n] * fraction;
    }
    // DC block ONLY on the tiny generated harmonic path.
    const float dcCoeff=colorDcCoeff;
    state.dc=dcCoeff*state.dc+(1.f-dcCoeff)*residual;
    return x+residual-state.dc;
}

void ThumpAudioProcessor::runCompressor (float& l, float& r, thump::Detector& d,
                                        bool enabled, int mode, float threshold,
                                        float attackMs, float releaseMs, float makeupDb, int fetRatio, float vcaRatio) noexcept
{
    const float fs = (float) processingSampleRate;
    mode = juce::jlimit (0, 2, mode);
    if (d.previousMode != mode)
    {
        // Reset only the envelope control on switching models; no delayed audio is touched.
        d.grDb = 0.0f; d.rms2 = 0.0f;
        d.optoReleasing = false; d.releaseSample = 0;
        d.previousMode = mode;
    }
    const float peak = juce::jmax (std::abs (l), std::abs (r));
    float level = peak;
    if (mode != 0)
    {
        // RMS-like detection on VCA / OPTO (unlike the FET peak sidechain).
        const float c = mode == 1 ? rmsVcaCoeff : rmsOptoCoeff;
        d.rms2 = c * d.rms2 + (1.0f - c) * peak * peak;
        level = std::sqrt (juce::jmax (d.rms2, 0.0f));
    }
    // UREI 1176 threshold changes with ratio selection in the original unit.
    // Relative offsets model the published -30/-26/-25/-24 dB knee positions.
    static constexpr float fetKneeShift[4] {0.f,4.f,5.f,6.f};
    const float adjustedThreshold = mode==0
        ? threshold+fetKneeShift[juce::jlimit(0,3,fetRatio)] : threshold;
    const float over = thump::gainToDb (level) - adjustedThreshold;
    float above = juce::jmax (0.0f, over);
    if (mode == 1) // dbx-style soft knee, 6dB span
        above = over <= -3.0f ? 0.0f : (over >= 3.0f ? over
                                  : (over + 3.0f) * (over + 3.0f) / 12.0f);
    static constexpr float ratios[4] {4.f,8.f,12.f,20.f};
    const float ratio = mode==0 ? ratios[juce::jlimit(0,3,fetRatio)]
                                   : (mode==1 ? juce::jlimit(1.f,120.f,vcaRatio) : 3.f);
    const float goal = above * (1.0f - 1.0f / ratio);
    if (mode == 0) // FET: measured 1176 A/R limits, knob sets time; fastest clockwise.
    {
        const float coeff = goal > d.grDb ? d.fetAttackCoeff : d.fetReleaseCoeff;
        d.grDb = coeff * d.grDb + (1.0f - coeff) * goal;
    }
    else if (mode == 1) // original dbx 160 spec: 15 / 5 / 3 ms at 10 / 20 / 30 dB
    {
        if (goal > d.grDb)
        {
            // Attack depends on the instantaneous level excursion, not knob position.
            const int index=juce::jlimit (0,200,juce::roundToInt((above-10.f)*10.f));
            const float c = vcaAttackLookup[(size_t)index];
            d.grDb = c * d.grDb + (1.0f-c) * goal;
        }
        else d.grDb = juce::jmax (goal, d.grDb - 120.0f / fs); // dB per second
    }
    else // OPTO: fixed 20ms attack, smooth two-stage 0..100ms and 100..2000ms release
    {
        if (goal >= d.grDb)
        {
            const float c = optoAttackCoeff;
            d.grDb = c * d.grDb + (1.0f-c) * goal;
            d.optoReleasing = false;
        }
        else
        {
            if (!d.optoReleasing)
            {
                d.releaseOrigin = d.grDb;
                d.releaseSample = 0;
                d.optoReleasing = true;
            }
            const float elapsed = (float)d.releaseSample / fs;
            auto smoothStep = [] (float t) noexcept
            { t = juce::jlimit (0.0f, 1.0f, t); return t*t*(3.0f-2.0f*t); };
            const float remaining = elapsed <= .100f
                ? 1.0f - .5f * smoothStep (elapsed / .100f)
                : .5f * (1.0f - smoothStep ((elapsed - .100f) / 1.900f));
            d.grDb = juce::jmax (goal, d.releaseOrigin * remaining);
            d.releaseSample = juce::jmin (d.releaseSample + 1, (int) (fs * 2.0f) + 1);
            if (d.grDb <= goal) d.optoReleasing = false;
        }
    }
    // 8ms bypass ramp prevents clicks; processing state continues while bypassed.
    d.wetBlend = bypassCoeff * d.wetBlend + (1.0f - bypassCoeff) * (enabled ? 1.0f : 0.0f);
    const float gain = thump::dbToGain (-d.grDb + makeupDb);
    const float processedL = compressorColor(l*gain,d.grDb,mode,d.coloration[0]);
    const float processedR = compressorColor(r*gain,d.grDb,mode,d.coloration[1]);
    l += d.wetBlend * (processedL - l);
    r += d.wetBlend * (processedR - r);
}

void ThumpAudioProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;
    const int n = audio.getNumSamples();
    if (n < 1) return;
    const int nc = juce::jmin (getTotalNumInputChannels(), 2);
    if (nc < 1 || n > stereoWork.getNumSamples() || delayL.empty())
    {
        audio.clear(); return; // the host should never exceed the prepared block size
    }
    auto s = readParams();
    // Smooth response edits between host blocks; avoid zipper noise on EQ and crossover.
    if (!filterInitialized) { smoothedFilters=s; filterInitialized=true; }
    else
    {
        const float alpha=1.f-std::exp(-(float)n/(.020f*(float)getSampleRate()));
        auto chase=[alpha](float& v,float target) { v+=alpha*(target-v); };
        chase(smoothedFilters.crossover,s.crossover);
        chase(smoothedFilters.hpfHz,s.hpfHz);
        chase(smoothedFilters.dynamicHz,s.dynamicHz);
        chase(smoothedFilters.highMidHz,s.highMidHz);
        for(int k=0;k<3;++k)
        {
            chase(smoothedFilters.eqLowGain[k],s.eqLowGain[k]);
            chase(smoothedFilters.eqLowQ[k],s.eqLowQ[k]);
            chase(smoothedFilters.eqHighGain[k],s.eqHighGain[k]);
            chase(smoothedFilters.eqHighQ[k],s.eqHighQ[k]);
        }
    }
    updateFilters(smoothedFilters);
    const float preTarget=thump::dbToGain(s.inputDb);
    const float postTarget=thump::dbToGain(s.outputDb);
    const float osGainSmooth=1.f-std::exp(-1.f/(.008f*(float)processingSampleRate));
    for (int i=0; i<n; ++i)
    {
        smoothInputGain+=hostGainSmoothCoeff*(preTarget-smoothInputGain);
        const float l = audio.getSample (0,i);
        const float r = nc>1 ? audio.getSample (1,i) : l;
        stereoWork.setSample (0,i,l*smoothInputGain);
        stereoWork.setSample (1,i,r*smoothInputGain);
    }
    auto cacheFetTimes=[this](thump::Detector& d,float attack,float releaseTime)
    {
        const float fs=(float)processingSampleRate;
        d.fetAttackCoeff=std::exp(-1000.f/(juce::jmax(.02f,attack)*fs));
        d.fetReleaseCoeff=std::exp(-1000.f/(juce::jmax(1.f,releaseTime)*fs));
    };
    cacheFetTimes(lowDetector,s.lowAttackMs,s.lowReleaseMs);
    cacheFetTimes(highDetector,s.highAttackMs,s.highReleaseMs);
    const float lowDriveTarget=s.lowDrive, highDriveTarget=s.highDrive;
    const float lowWetTarget=s.lowSatMix, highWetTarget=s.highSatMix;
    const float dynRmsA=1.f-std::exp(-1.f/(.020f*(float)processingSampleRate));
    const float dynAttackA=1.f-std::exp(-1.f/(.030f*(float)processingSampleRate));
    const float dynReleaseA=1.f-std::exp(-1.f/(.150f*(float)processingSampleRate));
    juce::dsp::AudioBlock<float> source (stereoWork);
    auto work = source.getSubBlock (0, (size_t)n);
    auto up = oversampling.processSamplesUp (juce::dsp::AudioBlock<const float>(work));
    const int count = (int) up.getNumSamples();
    float* left = up.getChannelPointer (0);
    float* right = up.getChannelPointer (1);
    // All six orders can be selected independently, per band, using automatable
    // parameter choices. No added path delays; branch summation is sample-aligned.
    static constexpr int orders[6][3] {
        {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0}
    }; // 0 EQ, 1 DRIVE, 2 COMP
    for (int i=0; i<count; ++i)
    {
        smoothLowDrive+=parameterSmoothCoeff*(lowDriveTarget-smoothLowDrive);
        smoothHighDrive+=parameterSmoothCoeff*(highDriveTarget-smoothHighDrive);
        smoothLowSatMix+=parameterSmoothCoeff*(lowWetTarget-smoothLowSatMix);
        smoothHighSatMix+=parameterSmoothCoeff*(highWetTarget-smoothHighSatMix);
        s.lowDrive=smoothLowDrive; s.highDrive=smoothHighDrive;
        s.lowSatMix=smoothLowSatMix; s.highSatMix=smoothHighSatMix;
        smoothOutputGain+=osGainSmooth*(postTarget-smoothOutputGain);
        float lo[2], hi[2];
        const float in[2] {left[i],right[i]};
        for (int ch=0; ch<2; ++ch)
        {
            auto& b=bands[(size_t)ch];
            b.hpf12Blend+=parameterSmoothCoeff*((s.hpfSlope!=0?1.f:0.f)-b.hpf12Blend);
            b.hpf24Blend+=parameterSmoothCoeff*((s.hpfSlope==2?1.f:0.f)-b.hpf24Blend);
            const float hp12=b.inputHp[0].run(in[ch]);
            const float hp24=b.inputHp[1].run(hp12);
            const float hp=hp12+b.hpf24Blend*(hp24-hp12);
            const float clean=in[ch]+b.hpf12Blend*(hp-in[ch]);
            lo[ch]=b.lp[1].run(b.lp[0].run(clean));
            hi[ch]=b.hp[1].run(b.hp[0].run(clean));
        }
        if(s.mono) lo[0]=lo[1]=.5f*(lo[0]+lo[1]);
        // Linked dynamic EQ sidechain, with 30ms attack and 150ms release.
        const float bassPeak=juce::jmax(std::abs(lo[0]),std::abs(lo[1]));
        dynamicRms2+=dynRmsA*(bassPeak*bassPeak-dynamicRms2);
        const float dynOver=thump::gainToDb(std::sqrt(juce::jmax(0.f,dynamicRms2)))
                             -s.dynamicThreshold;
        const float cutTarget=s.dynamicOn
            ? s.dynamicMaxCut*juce::jlimit(0.f,1.f,dynOver/9.f) : 0.f;
        const float dynAlpha=cutTarget>dynamicCutDb?dynAttackA:dynReleaseA;
        dynamicCutDb+=dynAlpha*(cutTarget-dynamicCutDb);
        if(++dynamicCounter>=64)
        {
            dynamicCounter=0;
            for(auto& b:bands)
                b.dynamicEq.bell(smoothedFilters.dynamicHz,.85f,-dynamicCutDb,processingSampleRate);
        }
        for(int ch=0;ch<2;++ch)
            lo[ch]=bands[(size_t)ch].dynamicEq.run(lo[ch]);
        const auto routeLow=orders[juce::jlimit(0,5,s.lowRoute)];
        const auto routeHigh=orders[juce::jlimit(0,5,s.highRoute)];
        for(int step=0;step<3;++step)
        {
            const int lowStage=routeLow[step], highStage=routeHigh[step];
            if(lowStage==0)
                for(int ch=0;ch<2;++ch)
                    for(int k=0;k<3;++k)
                        lo[ch]=bands[(size_t)ch].lowDivot[k].run(bands[(size_t)ch].eq[k].run(lo[ch]));
            else if(lowStage==1)
                for(int ch=0;ch<2;++ch) lo[ch]=runLowSat(lo[ch],ch,s);
            else
                runCompressor(lo[0],lo[1],lowDetector,s.lowCompOn,s.lowMode,
                              s.lowThresh,s.lowAttackMs,s.lowReleaseMs,s.lowMakeupDb,
                              s.lowFetRatio,s.lowVcaRatio);
            if(highStage==0)
                for(int ch=0;ch<2;++ch)
                    hi[ch]=highEq[(size_t)ch][2].run(highEq[(size_t)ch][1].run(
                        highEq[(size_t)ch][0].run(hi[ch])));
            else if(highStage==1)
                for(int ch=0;ch<2;++ch) hi[ch]=runHighSat(hi[ch],ch,s);
            else
                runCompressor(hi[0],hi[1],highDetector,s.highCompOn,s.highMode,
                              s.highThresh,s.highAttackMs,s.highReleaseMs,s.highMakeupDb,
                              s.highFetRatio,s.highVcaRatio);
        }
        left[i] = (lo[0]*s.lowMix + hi[0]*s.highMix) * smoothOutputGain;
        right[i] = (lo[1]*s.lowMix + hi[1]*s.highMix) * smoothOutputGain;
    }
    const float ceiling = thump::dbToGain (s.limiterCeilingDb);
    const float limiterIn = thump::dbToGain (s.limiterInputDb);
    const float fsOS = (float) processingSampleRate;
    const float release = std::exp (-1000.f/(s.limiterReleaseMs*fsOS));
    const float glueAttack = std::exp (-1000.f/(2.0f*fsOS));
    const float glueRelease = std::exp (-1000.f/(juce::jmax (s.limiterReleaseMs,65.f)*fsOS));
    const float bypassSlew = 1.f - std::exp (-1000.f/(8.f*fsOS));
    float maxLimiterReduction = 0.f;
    for (int i=0; i<count; ++i)
    {
        const float a = left[i] * (s.limiterOn ? limiterIn : 1.f);
        const float b = right[i] * (s.limiterOn ? limiterIn : 1.f);
        const float p = juce::jmax (std::abs(a), std::abs(b));
        // Audible but restrained 1.4:1 program-dependent leveling BEFORE the
        // final peak catcher. Above a ceiling-relative -9 dB threshold, it
        // progressively 'locks' the combined bass while retaining transients.
        const float overGlueDb = thump::gainToDb (p) - (s.limiterCeilingDb-9.f);
        const float targetGlueDb = (s.limiterOn && s.glueOn)
            ? juce::jlimit (0.f, 4.5f, juce::jmax (0.f, overGlueDb)*.2857f)
            : 0.f;
        const float c = targetGlueDb > glueGrDb ? glueAttack : glueRelease;
        glueGrDb = c * glueGrDb + (1.f-c) * targetGlueDb;
        glueBlend+=bypassSlew*((s.glueOn ? 1.f : 0.f)-glueBlend);
        const float glueGain = thump::dbToGain (-glueGrDb*glueBlend);
        // 1ms lookahead: detector examines the undelayed stream, while gain
        // is applied to the appropriately delayed stream. Shared stereo
        // envelope preserves center image. Peak catcher is fast/clean.
        detectorPeak = juce::jmax (p*glueGain, detectorPeak*release);
        const float limiterGain = s.limiterOn
            ? juce::jmin (1.f, ceiling / juce::jmax (detectorPeak, 1.e-9f))
            : 1.f;
        const float totalGain = glueGain * limiterGain;
        const float targetBlend = s.limiterOn ? s.limiterMix : 0.f;
        limiterBlend += bypassSlew * (targetBlend - limiterBlend);
        float appliedGain = 1.f - limiterBlend * (1.f-totalGain);
        const size_t index = (size_t) delayIndex;
        const float delayedA = delayL[index], delayedB = delayR[index];
        // Host sample ceiling is guaranteed for full wet, not partial dry blend.
        if(s.limiterOn && s.limiterMix>=.9999f)
            appliedGain=juce::jmin(appliedGain,
                ceiling/juce::jmax(juce::jmax(std::abs(delayedA),std::abs(delayedB)),1.e-9f));
        delayL[index] = a; delayR[index] = b;
        delayIndex = (delayIndex+1) % (int)delayL.size();
        left[i] = delayedA * appliedGain;
        right[i] = delayedB * appliedGain;
        maxLimiterReduction = juce::jmax (maxLimiterReduction,
            -thump::gainToDb (juce::jmax (appliedGain,1.0e-7f)));

    }
    oversampling.processSamplesDown (work);
    // Decimation can introduce small new sample peaks. Guard at host rate.
    // This guarantees sample peaks at 100% wet, NOT intersample true peaks.
    if(s.limiterOn && s.limiterMix>=.9999f)
        for(int i=0;i<n;++i)
        {
            const float l=stereoWork.getSample(0,i), r=stereoWork.getSample(1,i);
            const float maximum=juce::jmax(std::abs(l),std::abs(r));
            if(maximum>ceiling)
            {
                const float g=ceiling/maximum;
                stereoWork.setSample(0,i,l*g);
                stereoWork.setSample(1,i,r*g);
            }
        }
    for (int ch=0; ch<nc; ++ch)
    {
        auto* d = audio.getWritePointer(ch);
        const auto* src = stereoWork.getReadPointer(ch);
        for (int i=0; i<n; ++i)
        { d[i] = src[i]; }
    }
    for (int ch=nc; ch<audio.getNumChannels(); ++ch) audio.clear(ch,0,n);
    lowReduction.store (s.lowCompOn ? lowDetector.grDb*lowDetector.wetBlend : 0.f);
    highReduction.store (s.highCompOn ? highDetector.grDb*highDetector.wetBlend : 0.f);
    limitReduction.store (maxLimiterReduction);
}

void ThumpAudioProcessor::applyFactoryPreset (int index)
{
    if (!juce::isPositiveAndBelow (index, 8)) return;
    const auto& p=thump::factoryPresets()[(size_t)index];
    const auto& ids=thump::presetParameterIds();
    for (size_t i=0; i<ids.size(); ++i)
        if (auto* param=apvts.getParameter(ids[i]))
        {
            const auto value=param->convertTo0to1(p.values[i]);
            param->beginChangeGesture(); param->setValueNotifyingHost(value); param->endChangeGesture();
        }
    // Legacy presets predate routing and ratio choices. Reset these to calibrated
    // defaults when a FACTORY preset is selected, while keeping old session recall.
    for (const auto& item : std::array<std::pair<const char*, float>,6> {{
        {"lowRoute",0.f},{"highRoute",0.f},{"lowFetRatio",0.f},
        {"highFetRatio",0.f},{"lowVcaRatio",4.f},{"highVcaRatio",4.f}}})
        if (auto* param=apvts.getParameter(item.first))
            param->setValueNotifyingHost(param->convertTo0to1(item.second));
    // New parameters default neutral when recalling a legacy factory preset.
    for (const auto& item : std::array<std::pair<const char*, float>,10> {{
        {"hpfHz",30.f},{"hpfSlope",0.f},{"dynOn",0.f},
        {"dynHz",100.f},{"dynThresh",-24.f},{"dynCut",3.f},
        {"highDriveType",0.f},{"highMidG",0.f},{"highMidQ",1.f},
        {"highMidHz",800.f}}})
        if (auto* param=apvts.getParameter(item.first))
            param->setValueNotifyingHost(param->convertTo0to1(item.second));
    if(auto* glue=apvts.getParameter("glueOn"))
        glue->setValueNotifyingHost(glue->convertTo0to1(1.f));
    selectedPreset.store(index);
}
void ThumpAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto tree=apvts.copyState(); tree.setProperty ("presetIndex",selectedPreset.load(),nullptr);
    std::unique_ptr<juce::XmlElement> xml (tree.createXml());
    copyXmlToBinary (*xml,dest);
}
void ThumpAudioProcessor::setStateInformation (const void* data, int bytes)
{
    if (auto xml=getXmlFromBinary(data,bytes))
        if (xml->hasTagName (apvts.state.getType().toString()))
        {
            auto tree=juce::ValueTree::fromXml (*xml);
            selectedPreset.store ((int) tree.getProperty ("presetIndex",-1));
            apvts.replaceState(tree);
        }
}
juce::AudioProcessorEditor* ThumpAudioProcessor::createEditor() { return new ThumpAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ThumpAudioProcessor(); }
