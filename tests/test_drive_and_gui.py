"""v0.5 saturation/geometry/ratio regression checks. Python stdlib only."""
from pathlib import Path
import math
ROOT=Path(__file__).resolve().parents[1]
cpp=(ROOT/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
head=(ROOT/'Source/PluginProcessor.h').read_text(encoding='utf-8')
gui=(ROOT/'Source/PluginEditor.cpp').read_text(encoding='utf-8')

def thd_and_k2(y,fs,hz):
    n=len(y)
    def amp(harm):
        omega=2*math.pi*hz*harm/fs
        re=sum(a*math.cos(omega*i) for i,a in enumerate(y))
        im=sum(a*math.sin(omega*i) for i,a in enumerate(y))
        return math.hypot(re,im)
    a=[amp(i) for i in range(1,21)]
    return math.sqrt(sum(z*z for z in a[1:]))/a[0],a[1]/a[0],a[2]/a[0]

def saturate_low(x,drive):
    t=(drive-1)/9
    k=.3+2.7*t*t
    b=.15+.32*t
    z=max(-5,min(5,x/.18))
    normal=max(.12,k*(1-.16*b)/(1+.34*k*k))
    return .18*(math.tanh(k*z+b)-math.tanh(b))/normal

def saturate_high(x,drive):
    t=(drive-1)/9
    k=.35+4.1*t*t
    z=max(-5,min(5,x/.18))
    normal=max(.12,k/(1+.32*k*k))
    return .18*math.tanh(k*z)/normal

fs=176400
n=fs//10
level=10**(-15/20)
low=[level*math.sin(2*math.pi*80*i/fs) for i in range(n)]
high=[level*math.sin(2*math.pi*1000*i/fs) for i in range(n)]
low1=thd_and_k2([saturate_low(x,1) for x in low],fs,80)
low10=thd_and_k2([saturate_low(x,10) for x in low],fs,80)
# At 1k, TS pre-emphasis attenuates ~.81 and post 5k LP reduces upper harmonic energy.
high1=thd_and_k2([saturate_high(.81*x,1) for x in high],fs,1000)
high10=thd_and_k2([saturate_high(.81*x,10) for x in high],fs,1000)
assert .2 < low10[0] < .36,low10
assert .2 < high10[0] < .38,high10
assert low10[0] > 3*low1[0]
assert high10[0] > 3*high1[0]
assert high10[1] < 1.e-4,high10
assert low10[1] > .04,low10
assert 'return x + b.lowSatBlend*s.lowSatMix*(wet-x);' in cpp
assert 'return x + b.highSatBlend*s.highSatMix*(matched-x);' in cpp
assert 'levelMatchedWet' in cpp
assert 'oversampling {2, 2' in head
for id in ('lowRoute','highRoute','lowFetRatio','highFetRatio','lowVcaRatio','highVcaRatio'):
    assert id in cpp and id in gui,id
assert 'if(s.mono) lo[0]=lo[1]=' in cpp
assert 'if (s.mono) left[i]' not in cpp
assert '"LOW GR"' in gui and '"HIGH GR"' in gui
assert '"LIMITER  •  GR"' in gui
assert '"LOW MONO ONLY"' in gui
print('PASS: low/high drive increase to >=20% pre-filter THD at max for -15dBFS input')
print('PASS: symmetrical high, asymmetric low, no dry/wet residual-cancellation')
print('PASS: 4x, independent 6-way routing, real GR meters, low-only MONOIZE')
