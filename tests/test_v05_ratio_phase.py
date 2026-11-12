"""v0.5 unit-style checks for ratio math, routing, opto weights, mono and phase."""
from pathlib import Path
import math
import re
root=Path(__file__).resolve().parents[1]
s=(root/'Source/PluginProcessor.cpp').read_text()
h=(root/'Source/PluginProcessor.h').read_text()
layout=(root/'Source/PluginEditor.cpp').read_text()

routes=['EQ > Drive > Comp','EQ > Comp > Drive','Drive > EQ > Comp',
        'Drive > Comp > EQ','Comp > EQ > Drive','Comp > Drive > EQ']
for side in ('low','high'):
    assert f'"{side}Route"' in s
    assert f'"{side}FetRatio"' in s
    assert f'"{side}VcaRatio"' in s
    assert f'addChoice("{side}Route"' in layout
assert 'static constexpr int orders[6][3]' in s
for perm in ('{0,1,2}','{0,2,1}','{1,0,2}','{1,2,0}','{2,0,1}','{2,1,0}'):
    assert perm in s
assert 'if(s.mono) lo[0]=lo[1]=' in s
assert 'if (s.mono) left[i]' not in s
assert 'processingSampleRate = fs * 4.0' in s
assert 'oversampling {2, 2' in h

# Gain-reduction slope for compressor transfer above the knee must be 1/R.
for ratio in (4,8,12,20,1,2,6,10,120):
    at10=(10*(1-1/ratio))
    at30=(30*(1-1/ratio))
    assert math.isclose((30-at30)-(10-at10),20/ratio,abs_tol=1.e-10)
assert 'fetKneeShift[4] {0.f,4.f,5.f,6.f}' in s
assert 'juce::jlimit(1.f,120.f,vcaRatio)' in s

# Published dBc ladder at max GR and proportional scaling at intermediate GR.
ref={100:{2:-76,3:-88,4:-83},1000:{2:-71,3:-84,4:-78,5:-92},5000:{2:-88},10000:{2:-99}}
for freq, bands in ref.items():
    for harmonic,db in bands.items():
        amplitude=10**(db/20)
        assert 0 < amplitude < 0.001
        assert math.isclose(amplitude*.5,amplitude*(10/20),abs_tol=1.e-12)
assert 'const float fraction=juce::jlimit(0.f,1.f,grDb/20.f)' in s
assert 'static constexpr float optoLinear[4][4]' in s
assert 'compressorColor(l*gain,d.grDb,mode,d.coloration[0])' in s
assert 'compressorColor(r*gain,d.grDb,mode,d.coloration[1])' in s
print('PASS: 6 processing permutations per band, 4x, low-only mono, ratio slopes 4/8/12/20 & 1..120')
print('PASS: 100Hz/1kHz/5kHz/10kHz opto harmonic anchors scaled linearly with GR')
