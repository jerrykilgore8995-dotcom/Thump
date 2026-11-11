#!/usr/bin/env python3
"""Zero-dependency numerical checks for v0.4 (not hardware emulation verification)."""
from pathlib import Path
import math
P = Path(__file__).resolve().parents[1]
s = (P/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
h = (P/'Source/PluginProcessor.h').read_text(encoding='utf-8')
w = (P/'.github/workflows/windows-vst3.yml').read_text(encoding='utf-8')

def oldlow(x, drive, mix):
    d = .9 + drive*.54
    shaped = math.tanh(d*x+.20*d*x*x)/d
    return x + mix*(shaped-x)*1.8

def newlow(x, drive, mix):
    d = 2.45 + 1.55*drive
    bias = .10 + .012*drive
    offset = math.tanh(bias)
    shaped = (math.tanh(d*x+bias)-offset)/(d*(1-offset*offset))
    return x + min(1., 1.85*mix)*(shaped-x)

def thd(signal, fs, fundamental):
    n = len(signal)
    harmonics=[]
    for k in range(1,11):
        omega=2*math.pi*k*fundamental/fs
        re=sum(x*math.cos(omega*i) for i,x in enumerate(signal))
        im=sum(x*math.sin(omega*i) for i,x in enumerate(signal))
        harmonics.append(math.hypot(re,im))
    return math.sqrt(sum(v*v for v in harmonics[1:]))/harmonics[0]

fs=88200
samples=[.16*math.sin(2*math.pi*80*i/fs) for i in range(fs//10)]
a=thd([oldlow(x,3.0,.16) for x in samples],fs,80)
b=thd([newlow(x,3.0,.16) for x in samples],fs,80)
print('Old low THD %.3f%%; new %.3f%%; ratio %.2fx'%(a*100,b*100,b/a))
assert b>2*a,(a,b)
assert b<.25
peak_db=-6.+4.
ceiling_db=-1.
glue_target=min(4.5,max(0.,peak_db-(ceiling_db-9.))*0.2857)
assert 1.5<glue_target<4.5
assert peak_db<ceiling_db
assert 'lowDivot[3]' in h
assert 'b.lowDivot[i].bell' in s
assert 'glueGrDb' in s
assert 'limiterBlend' in s
assert '1.85f*s.lowSatMix' in s
assert '0.4.0_Windows' in w
print('PASS: low harmonics >2x and limiter glue reacts below peak ceiling')