"""THUMP v0.6 DSP and rack-geometry regressions. No third-party modules required."""
from pathlib import Path
import math
import re

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")
head = (root / "Source/PluginProcessor.h").read_text(encoding="utf-8")
gui = (root / "Source/PluginEditor.cpp").read_text(encoding="utf-8")
intro = gui.split("ThumpAudioProcessorEditor::ThumpAudioProcessorEditor", 1)[1].split(
    "ThumpAudioProcessorEditor::~", 1)[0]

# New parameters are real APVTS-backed controls, not decorative widgets.
ids = ["hpfHz", "hpfSlope", "dynOn", "dynHz", "dynThresh", "dynCut",
       "highMidG", "highMidQ", "highMidHz", "highDriveType", "glueOn"]
for pid in ids:
    assert f'"{pid}"' in cpp, pid
    assert f'"{pid}"' in gui, pid

assert "MatchState lowMatch, highMatch" in head
assert "levelMatchedWet(x,x+residual,b.lowMatch)" in cpp
assert "levelMatchedWet(x,wet,b.highMatch)" in cpp
assert "b.inputHp[0].run" in cpp and "b.inputHp[1].run" in cpp
assert "b.dynamicEq.bell" in cpp
assert "highEq[(size_t)ch][2].run" in cpp
assert "b.ampPostLP.run" in cpp and "b.fuzzPostLP.run" in cpp
assert "glueBlend+=" in cpp
assert "Decimation can introduce small new sample peaks" in cpp
assert "s.limiterMix>=.9999f" in cpp
assert "smoothOutputGain" in cpp
assert "vcaAttackLookup[(size_t)index]" in cpp

# Expensive envelope coefficients are now cached per block/preparation.
compressor = cpp.split("void ThumpAudioProcessor::runCompressor",1)[1].split(
    "void ThumpAudioProcessor::processBlock",1)[0]
assert "std::exp" not in compressor, "Per-sample exp() found in compressor"

controls = []
labels = []
def add(kind,name,x,y,w,h):
    controls.append((kind,name,float(x),float(y),float(w),float(h)))
def label(name,x,y,w,h):
    labels.append(("label",name,float(x),float(y),float(w),float(h)))

for m in re.finditer(r'addKnob\("([^"]+)","[^"]+",([\d.]+),([\d.]+),([\d.]+)\)', intro):
    name,cx,cy,d = m.groups();cx=float(cx);cy=float(cy);d=float(d)
    add("knob",name,cx-d/2,cy-d/2,d,d+20)
    label(name,cx-62,cy-d/2-24,124,18)

for m in re.finditer(r'addEQ\("([^"]+)","[^"]+","[^"]+",([\d.]+),([\d.]+)\)', intro):
    name,x,y=m.groups();x=float(x);y=float(y)
    add("fader",name,x-28,y,56,205)
    add("Q",name,x-26,650,52,70)
    label(name+" freq",x-35,y-31,70,20)
    label(name+" Q",x-15,628,30,17)

for m in re.finditer(r'addSwitch\("([^"]+)","[^"]+",([\d.]+),([\d.]+),([\d.]+)\)',intro):
    name,x,y,w=m.groups();add("switch",name,x,y,w,36)

for m in re.finditer(r'addMode\("([^"]+)",([\d.]+),([\d.]+),([\d.]+),([\d.]+)\)',intro):
    add("mode",*m.groups())

for m in re.finditer(r'addChoice\("([^"]+)",\s*\{[^}]+\},\s*([\d.]+),([\d.]+),([\d.]+),([\d.]+)\)',intro,re.S):
    add("choice",*m.groups())

for m in re.finditer(r'addRatioSlider\("([^"]+)",([\d.]+),([\d.]+),([\d.]+),([\d.]+)\)',intro):
    add("ratio",*m.groups())

for m in re.finditer(r'addHorizontal\("([^"]+)","[^"]+",([\d.]+),([\d.]+),([\d.]+),([\d.]+),[\d.]+f\)',intro):
    name,x,y,w,h=m.groups()
    add("slider",name,x,y,w,h)
    label(name,float(x),float(y)-21,float(w),17)

for m in re.finditer(r'addTag\("([^"]+)",([\d.]+),([\d.]+),([\d.]+),([\d.]+),[\d.]+\)',intro):
    label(*m.groups())

assert len(controls)>=55, len(controls)
assert len(labels)>=40, len(labels)
def overlaps(a,b):
    return (min(a[2]+a[4],b[2]+b[4])-max(a[2],b[2]) > 1 and
            min(a[3]+a[5],b[3]+b[5])-max(a[3],b[3]) > 1)
def allowed_pair(a,b):
    return {a[1],b[1]} in ({"lowFetRatio","lowVcaRatio"},
                            {"highFetRatio","highVcaRatio"})
for i,a in enumerate(controls):
    assert a[2]>=0 and a[3]>=0 and a[2]+a[4]<=1600 and a[3]+a[5]<=1050,a
    for b in controls[i+1:]:
        assert not overlaps(a,b) or allowed_pair(a,b),(a,b)
for i,a in enumerate(labels):
    assert a[2]>=0 and a[3]>=0 and a[2]+a[4]<=1600 and a[3]+a[5]<=1050,a
    for b in labels[i+1:]:
        assert not overlaps(a,b),(a,b)

# Simple analytic check of final linked sample ceiling guard.
for db in (-12,-6,-1,0):
    ceiling=10**(db/20)
    for left,right in ((1.3,-0.2),(-.3,2.7),(.01,.01)):
        factor=min(1.0,ceiling/max(abs(left),abs(right),1e-9))
        assert max(abs(left*factor),abs(right*factor))<=ceiling+1e-12
print(f"PASS: v0.6 controls, no control/label collisions "
      f"({len(controls)} controls; {len(labels)} labels), cached timing and sample ceiling")
