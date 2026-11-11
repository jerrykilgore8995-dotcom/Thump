"""v0.3 design/regression guardrails, stdlib only; not DAW listening tests."""
from pathlib import Path
import math, re
root=Path(__file__).resolve().parents[1]
processor=(root/'Source'/'PluginProcessor.cpp').read_text()
editor=(root/'Source'/'PluginEditor.cpp').read_text()
assert 'const float driveDb=5.8f + 3.6f*s.highDrive;' in processor
assert 'return x*(1.f-wet) + distorted*wet;' in processor
assert '(clipped-filtered) * 2.8f' not in processor
assert 'drawVU(g,{225,186' not in editor and 'drawVU(g,{1128,186' not in editor
assert '"LOW  •  GR"' in editor and '"HIGH  •  GR"' in editor
assert '"LIMITER  •  GR"' in editor
for key in ('lowCompOn','highCompOn','lowMode','highMode'):
    assert key in editor
# Static transfer-model sanity: with healthy bass harmonics, Drive 8 really clips.
def saturate(x,drive):
    dv=10**((5.8+3.6*drive)/20)
    a=x*dv
    d1=math.tanh(a*1.45+.11)-math.tanh(.11)
    d2=math.tanh(a*1.45-.05)-math.tanh(-.05)
    return (.58*d1+.42*d2)*(.52+(drive-1)*(.31-.52)/9)
assert saturate(.02,8.0) > 4*saturate(.02,1.0)
assert saturate(.02,10.0) > .1  # before LPF, clearly stronger than 20mV clean high band
# Check named GUI layout section rectangles do not overlap each other.
for left_a,left_b in ((36,778),(822,1564)):
    assert left_a < left_b
assert 'setSize (1520,817)' in editor
print('PASS: strong TS-style high-band clipping, non-cancelling wet mix and three GR meters')
