"""Actual THUMP v0.5 VST3 offline measurements at 44.1kHz, both drives bypassed."""
import csv, math, os, re, sys, json
from pathlib import Path
import numpy as np
from pedalboard import load_plugin

PLUG=Path(sys.argv[1])
OUT=Path("measurements/v0.5")
OUT.mkdir(parents=True,exist_ok=True)
SR=44100
FSAMP=65536
FREQS=[20,30,40,60,80,100,120,160,220,320,500,800,1000,1500,2200,3500,5000,8000,10000,16000,20000]
plugin=load_plugin(str(PLUG))
def key(s): return re.sub(r"[^a-z0-9]","",str(s).lower())
PARAM={key(k):k for k in plugin.parameters}
PARMNAMES={key(getattr(p,"name",k)):k for k,p in plugin.parameters.items()}
print("DISCOVERED VST3 PARAMETERS:",list(plugin.parameters.keys()))
def setv(alias,value):
    k=PARAM.get(key(alias),PARMNAMES.get(key(alias)))
    if k is None: raise RuntimeError("Missing VST3 parameter "+alias)
    p=plugin.parameters[k]
    try: p.raw_value=value
    except (ValueError,TypeError) as e:
        # JUCE choice parameters are represented as normalized 0..1 in some hosts.
        if alias.lower().endswith("mode") or alias.lower().endswith("ratio") or alias.lower().endswith("order"):
            p.value=float(value)
        else: raise RuntimeError(f"Cannot set {alias} ({k})={value}: {e}")
INITIAL_RAW={k:p.raw_value for k,p in plugin.parameters.items()}
def apply(overrides):
    # Restore EVERY parameter, not only the parameters mentioned by the case.
    # Otherwise EQ/threshold/limiter state leaks from one sweep into the next.
    for key_name,raw in INITIAL_RAW.items():
        plugin.parameters[key_name].raw_value=raw
    for name in ("Low Saturation","High Tube Style Saturation","Low Compressor",
                 "High Compressor","Final Limiter","MONOIZE"):
        setv(name,0)
    for name,value in (("Low Makeup",0),("High Makeup",0),
                       ("Low Blend",1),("High Blend",1)):
        setv(name,value)
    for name,value in overrides.items():setv(name,value)

def process(x,params):
    apply(params)
    return np.asarray(plugin.process(x.astype("float32"),sample_rate=SR,reset=True),dtype=np.float64)
def complex_bin(x,f):
    n=len(x);time=np.arange(n)/SR
    return 2.0*np.dot(x,np.exp(-2j*np.pi*f*time))/n
def tone(cat,name,f,db,overrides):
    n0=17640;n=8820; a=10**(db/20)
    s=a*np.sin(2*np.pi*f*np.arange(n0+n)/SR)
    x=np.stack([s,s])
    y=process(x,overrides)[0,n0:n0+n]
    z=complex_bin(s[n0:],f);out=complex_bin(y,f); mag=abs(out)
    harmonics=[]
    for k in range(2,21):
        if f*k>=SR/2: harmonics.append(None)
        else: harmonics.append(20*np.log10(max(abs(complex_bin(y,k*f)),1e-15)/max(mag,1e-15)))
    thd=100*np.sqrt(sum(10**(v/10) for v in harmonics if v is not None))
    peak=20*np.log10(max(np.max(np.abs(y)),1e-15))
    phase=np.angle(out/z,deg=True)
    return [cat,name,f,db,20*np.log10(max(abs(out/z),1e-15)),phase,thd,peak,
            float(np.sqrt(np.mean(y*y))),*[v if v is not None else "" for v in harmonics],json.dumps(overrides,sort_keys=True)]
def imp(cfg,left_only=False):
    x=np.zeros((2,FSAMP),dtype="float32");x[0,0]=.1
    if not left_only:x[1,0]=.1
    return np.fft.rfft(process(x,cfg),axis=1)/.1
def freq_rows(cat,name,cfg,ref):
    v=imp(cfg)
    for f in FREQS:
        k=round(f*FSAMP/SR)
        a=v[0,k]; b=v[1,k];r=ref[0,k]
        yield [cat,name,f,20*np.log10(max(abs(a),1e-15)),np.angle(a,deg=True),
              20*np.log10(max(abs(a/r),1e-15)),np.angle(a/r,deg=True),
              20*np.log10(max(abs(b),1e-15)),np.angle(a*np.conj(b),deg=True),
              json.dumps(cfg,sort_keys=True)]
with open(OUT/"parameter_names.json","w") as f:json.dump(
    {k:{"name":getattr(v,"name",k),"value":str(v),"range":str(getattr(v,"range",""))}
      for k,v in plugin.parameters.items()},f,indent=2)
with open(OUT/"frequency_phase.csv","w",newline="") as f:
    w=csv.writer(f);w.writerow(["category","setting","frequency_hz","gain_db","phase_deg",
                  "gain_vs_bypass_db","phase_vs_bypass_deg","right_gain_db","lr_phase_deg","overrides"])
    base=imp({});w.writerows(freq_rows("baseline","neutral",{},base))
    for xo in [120,220,320,500,800]:
        for side in ["low","high","both"]:
            cfg={"Crossover Hz":xo,"Low Blend":0 if side=="high" else 1,
                 "High Blend":0 if side=="low" else 1}
            w.writerows(freq_rows("crossover",f"{side}_{xo}",cfg,base))
    for low in [0,.25,.5,.75,1]:
        for hi in [0,.25,.5,.75,1]:
            w.writerows(freq_rows("blend",f"low{low}_high{hi}",
                       {"Low Blend":low,"High Blend":hi},base))
    for xo in [120,320,800]:
        for mono in [0,1]:
            cfg={"Crossover Hz":xo,"MONOIZE":mono}
            w.writerows(freq_rows("monoize",f"x{xo}_mono{mono}",cfg,base))
    # Generic EQ parameters sometimes repeat labels in plugins; discover exact exposed
    # keys and only sweep those that are uniquely addressable.
    eq_stems=["low40","low120","low220","high1500","high5000"]
    eq_labels={}
    for stem in eq_stems:
        matches=[k for k in plugin.parameters if stem in key(k)]
        gain=next((k for k in matches if "gain" in key(k) or key(k).endswith("g")),None)
        width=next((k for k in matches if "q" in key(k) or "width" in key(k)),None)
        if gain and width:eq_labels[stem]=(gain,width)
    # Fallback when the VST3 host exposes duplicate human-readable EQ names.
    kinds=[("low",["low40","low120","low220"],"Low EQ Gain","Low Shelf Width"),
           ("high",["high1500","high5000"],"High EQ Gain","High Proportional Q")]
    for _,stems,glabel,qlabel in kinds:
        gnames=[k for k,p in plugin.parameters.items() if key(getattr(p,"name",k))==key(glabel)]
        qnames=[k for k,p in plugin.parameters.items() if key(getattr(p,"name",k)).startswith(key(qlabel))]
        if len(gnames)>=len(stems) and len(qnames)>=len(stems):
            for idx,stem in enumerate(stems):eq_labels.setdefault(stem,(gnames[idx],qnames[idx]))
    print("EQ BINDINGS:",eq_labels)
    for stem,(gname,qname) in eq_labels.items():
        for gain in [-12,-9,-6,-3,0,3,6,9,12]:
            for width in [.3,.65,1,1.6,3]:
                if gain==0 and width!=1:continue
                cfg={gname:gain,qname:width}
                w.writerows(freq_rows("eq",f"{stem}_g{gain}_q{width}",cfg,base))
with open(OUT/"harmonics_thd.csv","w",newline="") as f:
    w=csv.writer(f)
    w.writerow(["category","setting","frequency_hz","input_dbfs","gain_db","phase_deg",
                "THD_K2_K20_percent","output_peak_dbfs","output_rms_linear"]+
                [f"K{k}_dBc" for k in range(2,21)]+["overrides"])
    for hz in [40,80,100,160,220,500,1000,1500,5000,10000,16000]:
        w.writerow(tone("reference","all_bypassed",hz,-18,{}))
    for side in ("Low","High"):
        other="High" if side=="Low" else "Low"
        f0=100 if side=="Low" else 1000
        for mode in [0,1,2]:
            for amp in [-42,-36,-30,-24,-18,-12,-6]:
                cfg={f"{side} Compressor":1,f"{side} Compressor Model":mode,
                     f"{side} Compressor Threshold":-30,f"{side} Makeup":0,
                     f"{other} Blend":0}
                w.writerow(tone("compressor_transfer",f"{side}_mode{mode}_level{amp}",f0,amp,cfg))
            for threshold in [-42,-36,-30,-24,-18]:
                freqs=[40,100,160,220] if side=="Low" else [800,1000,1500,5000,10000]
                for hz in freqs:
                    cfg={f"{side} Compressor":1,f"{side} Compressor Model":mode,
                         f"{side} Compressor Threshold":threshold,f"{side} Makeup":0,
                         f"{other} Blend":0}
                    w.writerow(tone("compressor_harmonics",f"{side}_mode{mode}_t{threshold}",hz,-12,cfg))
        for mode,ratios in [("FET",range(4)),("VCA",[1,2,4,6,8,12,20,120])]:
            for ratio in ratios:
                for amp in [-36,-24,-12,-6]:
                    cfg={f"{side} Compressor":1,f"{side} Compressor Model":0 if mode=="FET" else 1,
                         f"{side} {'FET Ratio' if mode=='FET' else 'VCA Ratio'}":ratio,
                         f"{side} Compressor Threshold":-30,f"{side} Makeup":0,
                         f"{other} Blend":0}
                    w.writerow(tone("ratio_"+mode.lower(),f"{side}_{ratio}_{amp}",f0,amp,cfg))
    for drive in [-12,-6,0,3,6,9,12,18]:
        for mix in [0,.25,.5,1]:
            for ceiling in [-12,-6,-1,0]:
                cfg={"Final Limiter":1,"Limiter Drive":drive,"Limiter Ceiling":ceiling,
                     "Limiter Mix":mix}
                for hz in [100,1000]:
                    w.writerow(tone("limiter",f"drive{drive}_mix{mix}_ceil{ceiling}",hz,-12,cfg))
print("COMPLETED: THUMP v0.5 measured actual plugin signal path; both drives bypassed.")