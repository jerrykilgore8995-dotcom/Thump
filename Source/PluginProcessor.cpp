#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

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
    tubePreHP.reset(); tubePostLP.reset(); lowSatDC.reset();

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
          { 0f, 0f, 290f, 1f, 1f, 3.2f, 0.17f, 0f, 0.7f, 2f, 0.85f, -3f, 1f, 1f, 1f, -25f, 0.2f, 220f, 2.5f, 0.95f, 1f, 4.5f, 0.46f, 3f, 0.95f, 1f, 1f, 1f, 0f, -22f, 0.1f, 85f, 1f, 0.78f, 1f, 3.5f, -1f, 75f, 1f } },
        { "Classic Rock Body", "Broad 120 Hz warmth with articulate but restrained 1.5 kHz.",
          { 0f, 0f, 370f, 1f, 1f, 2.3f, 0.12f, 1f, 0.55f, 2.5f, 0.7f, -1.5f, 0.8f, 1f, 2f, -22f, 0.45f, 320f, 2f, 0.95f, 1f, 2.6f, 0.22f, 1.5f, 0.8f, -0.5f, 0.9f, 1f, 1f, -18f, 0.28f, 155f, 1f, 0.67f, 1f, 4f, -0.8f, 110f, 0.9f } },
        { "Punk Pick Attack", "Prominent string click and 1.5 kHz edge, with tight lows.",
          { 0f, 0f, 280f, 1f, 1f, 2.8f, 0.13f, -1f, 0.8f, 1.5f, 0.9f, -2.5f, 1.1f, 1f, 0f, -24f, 0.1f, 180f, 2f, 0.9f, 1f, 5.8f, 0.58f, 5f, 0.95f, 3f, 1.1f, 1f, 0f, -20f, 0.08f, 65f, 1f, 0.9f, 1f, 6f, -1f, 55f, 1f } },
        { "Drop Tuned Grind", "Preserve sub notes while moving distortion into the upper harmonics.",
          { 0f, 0f, 210f, 1f, 1f, 3.3f, 0.18f, 1f, 0.75f, 2f, 0.8f, -4f, 1.1f, 1f, 1f, -27f, 0.25f, 380f, 3f, 0.98f, 1f, 6.5f, 0.55f, 4f, 1.1f, 1.5f, 1.3f, 1f, 1f, -25f, 0.2f, 130f, 1.5f, 0.78f, 1f, 6f, -1.2f, 90f, 1f } },
        { "CLA-Inspired Bite", "Inspired by CLA's 1176-controlled bass, top-end EQ and distortion choices.",
          { 0f, 0f, 330f, 1f, 1f, 2.5f, 0.18f, 0f, 0.7f, 1.5f, 0.8f, -2f, 0.95f, 1f, 0f, -28f, 0.08f, 175f, 3f, 0.95f, 1f, 5.4f, 0.48f, 4.5f, 0.9f, 3f, 1.2f, 1f, 0f, -21f, 0.07f, 90f, 1.5f, 0.88f, 1f, 3.5f, -1f, 80f, 1f } },
        { "Scheps-Inspired Parallel", "Retain natural fundamentals; blend in grit and limited compression.",
          { 0f, 0f, 340f, 1f, 1f, 2.4f, 0.09f, 0f, 0.65f, 1f, 0.75f, -1f, 0.9f, 0f, 2f, -20f, 0.45f, 280f, 0f, 0.98f, 1f, 4.5f, 0.39f, 5f, 0.9f, 0f, 1.1f, 0f, 1f, -20f, 0.32f, 145f, 0f, 0.82f, 1f, 4f, -1.2f, 110f, 0.48f } },
        { "TLA-Inspired Crush", "Aggressive dense compression informed by TLA's heavy bass-control philosophy.",
          { 0f, 0f, 310f, 1f, 1f, 3.2f, 0.22f, 0f, 0.8f, 1.5f, 0.75f, -3f, 1.1f, 1f, 0f, -34f, 0.05f, 190f, 5f, 0.98f, 1f, 5f, 0.44f, 3.5f, 1.1f, 2.5f, 1.2f, 1f, 0f, -30f, 0.06f, 65f, 3f, 0.85f, 1f, 6f, -1.2f, 65f, 1f } },
        { "Staub-Inspired Edge", "A dense modern-rock balance: firm lows, bright articulation and restrained limiting.",
          { 0f, 0f, 320f, 1f, 1f, 3f, 0.16f, -0.5f, 0.8f, 2f, 0.75f, -2.5f, 1.1f, 1f, 1f, -27f, 0.22f, 240f, 3f, 0.96f, 1f, 5f, 0.44f, 3.5f, 1.1f, 2f, 1.15f, 1f, 0f, -24f, 0.13f, 105f, 2f, 0.83f, 1f, 5f, -1f, 85f, 0.95f } }
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
    fl ("lowMix", "Low Blend", 0, 1, .001f, 1);
    bl ("highSatOn", "High Tube Style Saturation", true);
    fl ("highDrive", "High Drive", 1, 10, .01f, 3.8f);
    fl ("highSatMix", "High Saturation Mix", 0, 1, .001f, .38f);
    for (const char* id : {"high1500G", "high5000G"}) fl (id, "High EQ Gain", -12, 12, .1f, 0);
    for (const char* id : {"high1500Q", "high5000Q"}) fl (id, "High Proportional Q Trim (1 = Stock)", .3f, 3, .01f, 1.0f);
    bl ("highCompOn", "High Compressor", true);
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"highMode",1},
        "High Compressor Model", juce::StringArray {"FET 1176", "VCA 160", "Opto Dual Slope"}, 1));
    fl ("highThresh", "High Compressor Threshold", -45, 0, .1f, -22);
    fl ("highAttack", "High FET Attack ms", .02f, .8f, .001f, .2f);
    fl ("highRelease", "High FET Release ms", 50, 1100, 1, 180);
    fl ("highMakeup", "High Makeup", -12, 18, .1f, 1);
    fl ("highMix", "High Blend", 0, 1, .001f, .72f);
    bl ("limiterOn", "Final Limiter", true);
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
    processingSampleRate = fs * 2.0;
    oversampling.initProcessing ((size_t) maxBlock);
    oversampling.reset();
    for (auto& b : bands) b.reset();
    lowDetector.reset(); highDetector.reset();
    stereoWork.setSize (2, maxBlock, false, false, true);
    stereoWork.clear();
    for (auto& pair : highEq) for (auto& eq : pair) eq.reset();
    lookaheadOS = juce::jmax (1, (int) std::ceil (fs * .001)) * 2;
    delayL.assign ((size_t) lookaheadOS, 0.0f);
    delayR.assign ((size_t) lookaheadOS, 0.0f);
    delayIndex = 0; detectorPeak = 0; glueGrDb = 0; limiterBlend = 1.f;
    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()) + lookaheadOS / 2);
    updateFilters (readParams());
}
void ThumpAudioProcessor::releaseResources() {}

thump::State ThumpAudioProcessor::readParams() const noexcept
{
    auto x = [&](const char* id) { return apvts.getRawParameterValue (id)->load(); };
    thump::State p;
    p.inputDb=x("input"); p.outputDb=x("output"); p.crossover=x("xover"); p.mono=x("mono")>0.5f;
    p.lowSatOn=x("lowSatOn")>0.5f; p.lowDrive=x("lowDrive"); p.lowSatMix=x("lowSatMix");
    p.eqLowGain[0]=x("low40G"); p.eqLowQ[0]=x("low40Q");
    p.eqLowGain[1]=x("low120G"); p.eqLowQ[1]=x("low120Q");
    p.eqLowGain[2]=x("low220G"); p.eqLowQ[2]=x("low220Q");
    p.lowCompOn=x("lowCompOn")>0.5f; p.lowMode=(int)x("lowMode");
    p.lowThresh=x("lowThresh"); p.lowAttackMs=x("lowAttack");
    p.lowReleaseMs=x("lowRelease"); p.lowMakeupDb=x("lowMakeup"); p.lowMix=x("lowMix");
    p.highSatOn=x("highSatOn")>0.5f; p.highDrive=x("highDrive"); p.highSatMix=x("highSatMix");
    p.eqHighGain[0]=x("high1500G"); p.eqHighQ[0]=x("high1500Q");
    p.eqHighGain[1]=x("high5000G"); p.eqHighQ[1]=x("high5000Q");
    p.highCompOn=x("highCompOn")>0.5f; p.highMode=(int)x("highMode");
    p.highThresh=x("highThresh"); p.highAttackMs=x("highAttack");
    p.highReleaseMs=x("highRelease"); p.highMakeupDb=x("highMakeup"); p.highMix=x("highMix");
    p.limiterOn=x("limiterOn")>0.5f; p.limiterInputDb=x("limiterInput");
    p.limiterCeilingDb=x("limiterCeiling"); p.limiterReleaseMs=x("limiterRelease"); p.limiterMix=x("limiterMix");
    return p;
}

void ThumpAudioProcessor::updateFilters (const thump::State& s) noexcept
{
    constexpr float qButter = 0.70710678f;
    for (size_t ch=0; ch<bands.size(); ++ch)
    {
        auto& b = bands[ch];
        highEq[ch][0].bell (1500, thump::proportionalQ (s.eqHighGain[0], s.eqHighQ[0]),
                             s.eqHighGain[0], processingSampleRate);
        highEq[ch][1].bell (5000, thump::proportionalQ (s.eqHighGain[1], s.eqHighQ[1]),
                             s.eqHighGain[1], processingSampleRate);
        for (auto& f : b.lp) f.lowPass (s.crossover, qButter, processingSampleRate);
        for (auto& f : b.hp) f.highPass (s.crossover, qButter, processingSampleRate);
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
        b.lowSatDC.highPass (18, qButter, processingSampleRate);
    }
}

float ThumpAudioProcessor::runLowSat (float x, int channel, const thump::State& s) noexcept
{
    // This parallel asymmetric triode/tape-like shaper retains the dry bass
    // fundamental while generating considerably stronger K2/K3 harmonics.
    // Previous implementation was too weak at normal -18dBFS bass levels.
    const float d = 2.45f + 1.55f*s.lowDrive; // 4..18: substantive nonlinearity
    const float bias = .10f + .012f*s.lowDrive;
    const float offset = std::tanh (bias);
    const float linearGain = d*(1.f-offset*offset);
    const float shaped = (std::tanh (d*x + bias)-offset)/linearGain;
    // AC-couple only the difference; wet/dry timing of the fundamental stays
    // aligned and the 18 Hz blocker prevents an asymmetry-driven DC offset.
    const float residual = bands[(size_t)channel].lowSatDC.run (shaped-x);
    const float wet = s.lowSatOn ? juce::jlimit (0.f, 1.f, 1.85f*s.lowSatMix) : 0.f;
    return x + wet * residual;
}
float ThumpAudioProcessor::runHighSat (float x, int channel, const thump::State& s) noexcept
{
    auto& band=bands[(size_t)channel];
    // Tube Screamer-inspired mid-forward drive path, NOT a circuit-exact TS808:
    // 1) Cut excessive lows before clipping (bass fundamental remains on LOW path).
    // 2) Gain into a soft anti-parallel-diode-like clipping knee.
    // 3) Tone LPF around 5.2 kHz; wet blend of FULL clipped signal, NOT a small
    //    waveshaping residual that cancels against the original dry channel.
    // Range ~10 to 42 dB pre-drive: audible even on modest 10-40mV high-band inputs.
    const float preEmphasis=band.tubePreHP.run(x);
    const float driveDb=5.8f + 3.6f*s.highDrive;
    const float drive=thump::dbToGain(driveDb);
    const float amp=preEmphasis * drive;
    // Twin silicon diode soft clip: rounded knee, asymmetric small even component.
    const float d1=std::tanh(amp*1.45f + .11f)-std::tanh(.11f);
    const float d2=std::tanh(amp*1.45f - .05f)-std::tanh(-.05f);
    const float clipped=.58f*d1 + .42f*d2;
    const float bandLimited=band.tubePostLP.run(clipped);
    // Moderate compensation stops higher drive from simply becoming louder;
    // the nonlinear harmonic density nonetheless rises sharply with drive.
    const float outputTrim=juce::jmap(s.highDrive,1.f,10.f,.52f,.31f);
    const float distorted=bandLimited*outputTrim;
    const float wet=s.highSatOn ? s.highSatMix : 0.f;
    return x*(1.f-wet) + distorted*wet;
}
void ThumpAudioProcessor::runCompressor (float& l, float& r, thump::Detector& d,
                                        bool enabled, int mode, float threshold,
                                        float attackMs, float releaseMs, float makeupDb) noexcept
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
        const float c = std::exp (-1.0f / ((mode == 1 ? .003f : .007f) * fs));
        d.rms2 = c * d.rms2 + (1.0f - c) * peak * peak;
        level = std::sqrt (juce::jmax (d.rms2, 0.0f));
    }
    const float over = thump::gainToDb (level) - threshold;
    float above = juce::jmax (0.0f, over);
    if (mode == 1) // dbx-style soft knee, 6dB span
        above = over <= -3.0f ? 0.0f : (over >= 3.0f ? over
                                  : (over + 3.0f) * (over + 3.0f) / 12.0f);
    const float ratio = (mode == 2 ? 3.0f : 4.0f); // FET 4:1, VCA 4:1, OPTO 3:1
    const float goal = above * (1.0f - 1.0f / ratio);
    if (mode == 0) // FET: measured 1176 A/R limits, knob sets time; fastest clockwise.
    {
        const float ms = goal > d.grDb ? attackMs : releaseMs;
        const float coeff = std::exp (-1000.0f / (juce::jmax (.02f,ms) * fs));
        d.grDb = coeff * d.grDb + (1.0f - coeff) * goal;
    }
    else if (mode == 1) // original dbx 160 spec: 15 / 5 / 3 ms at 10 / 20 / 30 dB
    {
        if (goal > d.grDb)
        {
            // Attack depends on the instantaneous level excursion, not knob position.
            const float x = juce::jlimit (10.0f, 30.0f, above);
            const float ms = x <= 20.0f ? juce::jmap (x,10.0f,20.0f,15.0f,5.0f)
                                         : juce::jmap (x,20.0f,30.0f,5.0f,3.0f);
            const float c = std::exp (-1000.0f / (ms * fs));
            d.grDb = c * d.grDb + (1.0f-c) * goal;
        }
        else d.grDb = juce::jmax (goal, d.grDb - 120.0f / fs); // dB per second
    }
    else // OPTO: fixed 20ms attack, smooth two-stage 0..100ms and 100..2000ms release
    {
        if (goal >= d.grDb)
        {
            const float c = std::exp (-1000.0f / (20.0f * fs));
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
    const float bypassCoeff = std::exp (-1.0f / (.008f * fs));
    d.wetBlend = bypassCoeff * d.wetBlend + (1.0f - bypassCoeff) * (enabled ? 1.0f : 0.0f);
    float gain = thump::dbToGain (-d.grDb + makeupDb);
    if (mode == 0)
    {
        // Restrained FET color: a gentle odd-harmonic contribution in the wet path.
        const float color = .04f * juce::jlimit (0.0f, 1.0f, d.grDb / 10.0f);
        const float processedL = (l * gain) * (1.0f-color) + std::tanh (l*gain*1.7f) / 1.7f * color;
        const float processedR = (r * gain) * (1.0f-color) + std::tanh (r*gain*1.7f) / 1.7f * color;
        l += d.wetBlend * (processedL - l);
        r += d.wetBlend * (processedR - r);
    }
    else
    {
        const float effectiveGain = 1.0f + d.wetBlend * (gain - 1.0f);
        l *= effectiveGain; r *= effectiveGain;
    }
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
    const auto s = readParams();
    updateFilters (s);
    const float pre = thump::dbToGain (s.inputDb);
    for (int i=0; i<n; ++i)
    {
        const float l = audio.getSample (0,i);
        const float r = nc>1 ? audio.getSample (1,i) : l;
        stereoWork.setSample (0,i,l*pre);
        stereoWork.setSample (1,i,r*pre);
    }
    juce::dsp::AudioBlock<float> source (stereoWork);
    auto work = source.getSubBlock (0, (size_t)n);
    auto up = oversampling.processSamplesUp (juce::dsp::AudioBlock<const float>(work));
    const int count = (int) up.getNumSamples();
    float* left = up.getChannelPointer (0);
    float* right = up.getChannelPointer (1);
    for (int i=0; i<count; ++i)
    {
        float lo[2], hi[2];
        const float in[2] {left[i],right[i]};
        for (int ch=0; ch<2; ++ch)
        {
            auto& b = bands[(size_t)ch];
            lo[ch] = b.lp[1].run (b.lp[0].run (in[ch]));
            hi[ch] = b.hp[1].run (b.hp[0].run (in[ch]));
            // LR4 low and high have matching phase at crossover and sum to unity MAGNITUDE.
            for (int k=0; k<3; ++k)
                lo[ch] = b.lowDivot[k].run (b.eq[k].run (lo[ch]));
            hi[ch] = highEq[(size_t)ch][1].run (highEq[(size_t)ch][0].run (hi[ch]));
            lo[ch] = runLowSat (lo[ch],ch,s);
            hi[ch] = runHighSat (hi[ch],ch,s);
        }
        runCompressor (lo[0],lo[1],lowDetector,s.lowCompOn,s.lowMode,
                       s.lowThresh,s.lowAttackMs,s.lowReleaseMs,s.lowMakeupDb);
        runCompressor (hi[0],hi[1],highDetector,s.highCompOn,s.highMode,
                       s.highThresh,s.highAttackMs,s.highReleaseMs,s.highMakeupDb);
        left[i] = lo[0]*s.lowMix + hi[0]*s.highMix;
        right[i] = lo[1]*s.lowMix + hi[1]*s.highMix;
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
        const float targetGlueDb = s.limiterOn
            ? juce::jlimit (0.f, 4.5f, juce::jmax (0.f, overGlueDb)*.2857f)
            : 0.f;
        const float c = targetGlueDb > glueGrDb ? glueAttack : glueRelease;
        glueGrDb = c * glueGrDb + (1.f-c) * targetGlueDb;
        const float glueGain = thump::dbToGain (-glueGrDb);
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
        const float appliedGain = 1.f - limiterBlend * (1.f-totalGain);
        const size_t index = (size_t) delayIndex;
        const float delayedA = delayL[index], delayedB = delayR[index];
        delayL[index] = a; delayR[index] = b;
        delayIndex = (delayIndex+1) % (int)delayL.size();
        left[i] = delayedA * appliedGain;
        right[i] = delayedB * appliedGain;
        maxLimiterReduction = juce::jmax (maxLimiterReduction,
            -thump::gainToDb (juce::jmax (appliedGain,1.0e-7f)));
        if (s.mono) left[i] = right[i] = .5f*(left[i]+right[i]);
    }
    oversampling.processSamplesDown (work);
    const float post = thump::dbToGain (s.outputDb);
    for (int ch=0; ch<nc; ++ch)
    {
        auto* d = audio.getWritePointer(ch);
        const auto* src = stereoWork.getReadPointer(ch);
        for (int i=0; i<n; ++i)
        { d[i] = src[i] * post; }
    }
    for (int ch=nc; ch<audio.getNumChannels(); ++ch) audio.clear(ch,0,n);
    lowReduction.store (lowDetector.grDb);
    highReduction.store (highDetector.grDb);
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
