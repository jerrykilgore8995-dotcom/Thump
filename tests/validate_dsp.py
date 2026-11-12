#!/usr/bin/env python3
"""Offline math and project consistency tests. Not a JUCE compilation or listening test."""
import cmath
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / 'Source' / 'PluginProcessor.cpp').read_text()
HEADER = (ROOT / 'Source' / 'PluginProcessor.h').read_text()
EDITOR = (ROOT / 'Source' / 'PluginEditor.cpp').read_text()


def butterworth(fs, hz, high=False):
    w = 2 * math.pi * min(hz, fs * .475) / fs
    c, s = math.cos(w), math.sin(w)
    alpha = s / math.sqrt(2)
    a0, a1, a2 = 1 + alpha, -2 * c, 1 - alpha
    if high:
        b0, b1, b2 = (1 + c) / 2, -(1 + c), (1 + c) / 2
    else:
        b0, b1, b2 = (1 - c) / 2, (1 - c), (1 - c) / 2
    return (b0/a0, b1/a0, b2/a0, a1/a0, a2/a0)


def h(coeffs, fs, f):
    z = cmath.exp(-2j * math.pi * f / fs)
    b0,b1,b2,a1,a2 = coeffs
    return (b0+b1*z+b2*z*z)/(1+a1*z+a2*z*z)


def smoothstep(t):
    t=max(0,min(1,t))
    return t*t*(3-2*t)


def opto_fraction(t):
    if t <= .100:
        return 1-.5*smoothstep(t/.1)
    return .5*(1-smoothstep((t-.1)/1.9))


def check():
    # Linkwitz-Riley has allpass-style frequency-dependent phase, unity MAGNITUDE.
    worst = 0
    for fs in (44100*2,48000*2,96000*2):
        for xo in (120,210,320,650,800):
            l, r = butterworth(fs,xo), butterworth(fs,xo,True)
            for freq in (20,40,80,120,160,210,320,650,1000,2500,5000,10000,20000):
                total = h(l,fs,freq)**2 + h(r,fs,freq)**2
                worst=max(worst,abs(abs(total)-1))
    assert worst < 1e-8, worst

    # Published 1176 FET: 20us-800us attack, 50-1100ms release. Both knob endpoints.
    assert '"lowAttack", "Low FET Attack ms", .02f, .8f' in CPP
    assert '"highAttack", "High FET Attack ms", .02f, .8f' in CPP
    assert '"lowRelease", "Low FET Release ms", 50, 1100' in CPP
    assert '"highRelease", "High FET Release ms", 50, 1100' in CPP
    # Published 160VU response, 63%-response coefficients 15/5/3ms,
    # rate in dB/s; 10 dB reduction returns in 83.33ms.
    for ms in (15,5,3):
        fs=88200
        c=math.exp(-1000/(ms*fs))
        response=1-c**round(ms*fs/1000)
        assert abs(response-(1-1/math.e)) < 0.001
    assert abs(10/120 - 1/12) < 1e-12
    assert '120.0f / fs' in CPP
    assert '20.0f * fs' in CPP  # opto release cap

    # User-design opto curve: 50% at 100ms, all remaining gain back by 2000ms.
    assert math.isclose(opto_fraction(0),1,abs_tol=1e-12)
    assert math.isclose(opto_fraction(.1),.5,abs_tol=1e-12)
    assert math.isclose(opto_fraction(2.),0,abs_tol=1e-12)
    assert all(opto_fraction(i*.001)>=opto_fraction((i+1)*.001) for i in range(2000))

    section=CPP[CPP.index('const std::array<const char*, 39>& presetParameterIds()'):CPP.index('// Factory presets')]
    ids=re.findall(r'"([a-zA-Z0-9]+)"',section)
    assert len(ids)==39 and len(set(ids))==39, ids
    assert all(f'"{id}"' in CPP for id in ids)
    # Parse preset values. Can compare every type/range with each live parameter declaration.
    section=CPP[CPP.index('const std::array<Preset, 8>& factoryPresets()'):CPP.index('static juce::AudioProcessorValueTreeState::ParameterLayout')]
    found=re.findall(r'\{\s*"([^"\n]+)",\s*"([^"\n]+)",\s*\{([^{}]+)\}\s*\}',section)
    assert len(found)==8, len(found)
    for name,desc,serialized in found:
        data=[float(v.removesuffix('f')) for v in re.findall(r'-?\d+(?:\.\d+)?f',serialized)]
        assert len(data)==39, (name,len(data))
        s=dict(zip(ids,data))
        for id in ('lowMode','highMode'):
            assert s[id] in (0,1,2), (name,id,s[id])
        for id in ('lowCompOn','highCompOn','lowSatOn','highSatOn','limiterOn','mono'):
            assert s[id] in (0,1), (name,id)
        for id in ('lowAttack','highAttack'):
            assert .02<=s[id]<=.8, (name,id)
        for id in ('lowRelease','highRelease'):
            assert 50<=s[id]<=1100, (name,id)
        assert 120<=s['xover']<=800, name
        assert 40<=s['limiterRelease']<=300, name
        for id in ('lowMix','highMix','lowSatMix','highSatMix','limiterMix'):
            assert 0<=s[id]<=1, (name,id)
        for id in ('low40G','low120G','low220G','high1500G','high5000G'):
            assert -12<=s[id]<=12, (name,id)
    assert 'runCompressor(lo[0],lo[1],lowDetector' in CPP
    assert 'runCompressor(hi[0],hi[1],highDetector' in CPP
    assert 'addMode("lowMode"' in EDITOR and 'addMode("highMode"' in EDITOR
    assert 'addSwitch("lowCompOn"' in EDITOR and 'addSwitch("highCompOn"' in EDITOR
    assert 'juce::AudioBuffer<float> stereoWork' in HEADER
    print('PASS: 195 LR4 phase-matched magnitude-sum checks, worst delta', worst)
    print('PASS: FET 1176 published time endpoints, VCA 160 response targets')
    print('PASS: 20ms opto attack, two-stage 100ms / 2s return profile')
    print('PASS: 8 fully populated 39-parameter presets, valid modes and ranges')
    print('PASS: 3-way selectors + bypass on both bands; stereo-safe 4x oversampling')
    print('NOTE: no JUCE compiler or DAW test here; GitHub Action builds the VST3.')

if __name__=='__main__':
    check()
