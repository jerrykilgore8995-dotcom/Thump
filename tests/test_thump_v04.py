#!/usr/bin/env python3
"""Numerical and source-level checks for v0.4 release (not hardware emulation validation)."""
from pathlib import Path
import numpy as np
import math
P=Path(__file__).resolve().parents[1]
s=(P/'Source/PluginProcessor.cpp').read_text()
h=(P/'Source/PluginProcessor.h').read_text()
w=(P/'.github/workflows/windows-vst3.yml').read_text()


def oldlow(x,drive,mix):
    d=.9+drive*.54
    shaped=np.tanh(d*x+.20*d*x*x)/d
    return x+mix*(shaped-x)*1.8

def newlow(x,drive,mix):
    d=2.45+1.55*drive
    bias=.10+.012*drive
    offset=np.tanh(bias)
    shaped=(np.tanh(d*x+bias)-offset)/(d*(1-offset*offset))
    return x+min(1.,1.85*mix)*(shaped-x)

def thd(signal, fs, fundamental):
    samples=len(signal)
    h=np.fft.rfft(signal)
    fbin=round(fundamental*samples/fs)
    p=np.array([abs(h[k*fbin]) for k in range(1,11)])
    return float(np.sqrt(np.sum(p[1:]**2))/p[0])

fs=88200
n=fs
xx=np.arange(n)
samples=.16*np.sin(2*np.pi*80*xx/fs)
a=thd(oldlow(samples,3.0,.16),fs,80)
b=thd(newlow(samples,3.0,.16),fs,80)
print('Old low THD %.3f%%; new %.3f%%; ratio %.2fx'%(a*100,b*100,b/a))
assert b>2*a, (a,b)
assert b<.25, 'Audibly distorted, not complete destruction of low band'

# Model the modest -6dBFS bass peak, after the new default +4dB drive.
ceiling_db=-1.0
peak_db=-6.0+4.0
glue_target=min(4.5,max(0.,peak_db-(ceiling_db-9.))*0.2857)
assert glue_target>1.5, glue_target
# Even without exceeding the ceiling, glue stage should compress mildly.
assert peak_db<ceiling_db

assert 'lowDivot[3]' in h
assert 'b.lowDivot[i].bell' in s
assert 'glueGrDb' in s
assert 'limiterBlend' in s
assert '1.85f*s.lowSatMix' in s
assert '0.4.0' not in w or '0.4.0_Windows' in w
print('PASS: measurable low harmonics >2x, limiter glue reacts below peak ceiling')
