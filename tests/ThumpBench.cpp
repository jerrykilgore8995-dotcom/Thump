// THUMP v0.5 offline measurements. Compiles against the SAME DSP processor as the VST3.
// No external audio interface: JUCE AudioProcessor::processBlock() is exercised directly.
#include "../Source/PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <complex>
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <map>
#include <array>
#include <set>
#include <numeric>
#include <iostream>

namespace fs = std::filesystem;
using Edit = std::pair<std::string,float>;
using Edits = std::vector<Edit>;
constexpr double SR=44100.0;
constexpr int BLOCK=512;
constexpr double PI=3.14159265358979323846;

static void set (ThumpAudioProcessor& p, const char* id, float v)
{
    if(auto* par=p.apvts.getParameter(id))
        par->setValueNotifyingHost(par->convertTo0to1(v));
    else throw std::runtime_error(std::string("Missing parameter: ")+id);
}
static void setup (ThumpAudioProcessor& p, const Edits& e={})
{
    const Edits neutral={{"input",0},{"output",0},{"xover",320},{"mono",0},
        {"lowSatOn",0},{"highSatOn",0},{"lowCompOn",0},{"highCompOn",0},
        {"lowMakeup",0},{"highMakeup",0},{"lowMix",1},{"highMix",1},
        {"limiterOn",0},{"limiterMix",1},
        {"low40G",0},{"low120G",0},{"low220G",0},
        {"high1500G",0},{"high5000G",0},
        {"low40Q",1},{"low120Q",1},{"low220Q",1},
        {"high1500Q",1},{"high5000Q",1}};
    for(auto& kv:neutral)set(p,kv.first.c_str(),kv.second);
    for(auto& kv:e)set(p,kv.first.c_str(),kv.second);
    p.prepareToPlay(SR,BLOCK);
}
static std::vector<std::array<float,2>> process(ThumpAudioProcessor& p,
                                     const std::vector<std::array<float,2>>& in)
{
    std::vector<std::array<float,2>> out(in.size());
    juce::AudioBuffer<float> b(2,BLOCK);juce::MidiBuffer midi;
    for(size_t pos=0;pos<in.size();pos+=BLOCK)
    {
        const int n=(int)std::min<size_t>(BLOCK,in.size()-pos);
        b.clear();
        for(int i=0;i<n;++i) { b.setSample(0,i,in[pos+i][0]);b.setSample(1,i,in[pos+i][1]); }
        if(n<BLOCK)
        {
            // Never exceed prepared block size; partial block has n samples.
            juce::AudioBuffer<float> partial(b.getArrayOfWritePointers(),2,0,n);
            p.processBlock(partial,midi);
        }
        else p.processBlock(b,midi);
        for(int i=0;i<n;++i) out[pos+i]={b.getSample(0,i),b.getSample(1,i)};
    }
    return out;
}
struct Spectrum { std::complex<double> a,b; };
static Spectrum fundamental(const std::vector<std::array<float,2>>& samples,int start,int length,double hz,int harmonic=1)
{
    double cr=0,ci=0,dr=0,di=0;
    const double w=2.0*PI*hz*harmonic/SR;
    // the recurrence reduces the repeated trigonometric calls in the inner loop
    const double dw=std::cos(w), ds=-std::sin(w);
    double c=std::cos(w*start),s=-std::sin(w*start);
    for(int n=start;n<start+length;++n)
    {
        cr+=samples[n][0]*c; ci+=samples[n][0]*s;
        dr+=samples[n][1]*c; di+=samples[n][1]*s;
        const double next=c*dw-s*ds; s=s*dw+c*ds; c=next;
    }
    const double mul=2.0/length;
    return {{cr*mul,ci*mul},{dr*mul,di*mul}};
}
static double db(double v){return 20*std::log10(std::max(v,1e-15));}
static double deg(std::complex<double> z){return std::arg(z)*180/PI;}
static std::string commaFloat(double x)
{
    if(!std::isfinite(x))return "";
    std::ostringstream out;out<<std::fixed<<std::setprecision(6)<<x;return out.str();
}
static std::string desc(const Edits& e)
{
    std::ostringstream s;for(size_t i=0;i<e.size();++i)
    {if(i)s<<";";s<<e[i].first<<"="<<e[i].second;}return s.str();
}
static std::string clean(std::string x){for(char& c:x)if(c==','||c=='\n'||c=='\r')c=';';return x;}
static void tone(std::ofstream& csv,const std::string& category,const std::string& name,
                 double hz,double inputDb,const Edits& edits,bool stereoLeftOnly=false)
{
    auto p=std::make_unique<ThumpAudioProcessor>();setup(*p,edits);
    // 0.4s settle plus 0.2s Fourier segment: frequencies are multiples of 5 Hz.
    constexpr int warm=17640, N=8820;
    const double amp=std::pow(10,inputDb/20);
    std::vector<std::array<float,2>> in(warm+N);
    for(size_t i=0;i<in.size();++i)
    {const float x=(float)(amp*std::sin(2*PI*hz*i/SR));in[i]={x,stereoLeftOnly?0.f:x};}
    const auto y=process(*p,in);
    const auto fund=fundamental(y,warm,N,hz);
    const auto source=fundamental(in,warm,N,hz);
    const auto trans=fund.a/source.a;
    const double mag=std::abs(fund.a);
    double sum=0,maxPeak=0,sum2=0;
    std::array<double,19> hdb{};
    for(int k=2;k<=20;++k)
    {
        if(hz*k >= SR/2-1) {hdb[k-2]=std::nan("");continue;}
        const double h=std::abs(fundamental(y,warm,N,hz,k).a)/std::max(mag,1e-15);
        hdb[k-2]=db(h);sum+=h*h;
    }
    for(int i=warm;i<warm+N;++i)
    {maxPeak=std::max(maxPeak,(double)std::abs(y[i][0]));sum2+=y[i][0]*y[i][0];}
    csv<<clean(category)<<","<<clean(name)<<","<<hz<<","<<inputDb<<","
       <<commaFloat(db(std::abs(trans)))<<","<<commaFloat(deg(trans))<<","
       <<commaFloat(100*std::sqrt(sum))<<","<<commaFloat(db(mag))<<","
       <<commaFloat(db(maxPeak))<<","<<commaFloat(db(std::sqrt(sum2/N)))<<","
       <<commaFloat(p->lowReduction.load())<<","<<commaFloat(p->highReduction.load())<<","
       <<commaFloat(p->limitReduction.load())<<",";
    for(int k=0;k<19;++k){if(k)csv<<",";csv<<commaFloat(hdb[k]);}
    csv<<","<<clean(desc(edits))<<"\n";
}

using Complex=std::complex<float>;
struct Transfer
{
  std::vector<Complex> L,R;
};
constexpr int FFTSIZE=65536;
static Transfer impulse(const Edits& edits,bool leftOnly)
{
    auto p=std::make_unique<ThumpAudioProcessor>();setup(*p,edits);
    std::vector<std::array<float,2>> x(FFTSIZE);
    x[0]={0.1f,leftOnly?0.f:0.1f};
    const auto y=process(*p,x);
    std::vector<Complex> yin(FFTSIZE),yout(FFTSIZE);
    juce::dsp::FFT fft(16);
    Transfer t;t.L.resize(FFTSIZE);t.R.resize(FFTSIZE);
    for(int ch=0;ch<2;++ch)
    {
        for(int i=0;i<FFTSIZE;++i)yin[i]=Complex(y[i][ch],0);
        fft.perform(yin.data(),yout.data(),false);
        auto& dest=ch==0?t.L:t.R;
        for(int i=0;i<FFTSIZE;++i)dest[i]=yout[i]*10.f;
    }
    return t;
}
static Complex at(const std::vector<Complex>& a,double f)
{
    const double k=f*FFTSIZE/SR;const int i=(int)std::floor(k);
    const float alpha=(float)(k-i);
    // Complex interpolation is adequate below Nyquist at FFT size 65536.
    return (1-alpha)*a[i]+alpha*a[i+1];
}
static void freqrow(std::ofstream& csv,const std::string& category,const std::string& name,
                    double f,const Transfer& t,const Transfer& ref,const Edits& e)
{
    const auto l=at(t.L,f),r=at(t.R,f),rr=at(ref.L,f),tr=l/rr;
    const double lr=std::abs(l)>1.e-12 ? 20*std::log10(std::max((double)std::abs(r),1e-13)/std::abs(l)) : 0;
    const double stereoDelta=std::arg(l*std::conj(r))*180/PI;
    csv<<clean(category)<<","<<clean(name)<<","<<f<<","
       <<commaFloat(db(std::abs(l)))<<","<<commaFloat(deg(l))<<","
       <<commaFloat(db(std::abs(tr)))<<","<<commaFloat(deg(tr))<<","
       <<commaFloat(db(std::abs(r)))<<","<<commaFloat(lr)<<","
       <<commaFloat(stereoDelta)<<","<<clean(desc(e))<<"\n";
}
static void writeFreq(std::ofstream& csv,const std::string& category,const std::string& name,
                      const Edits& edits,const Transfer& ref,const std::vector<double>& frequencies,
                      bool leftOnly=false)
{
   const auto t=impulse(edits,leftOnly);
   for(double f:frequencies)freqrow(csv,category,name,f,t,ref,edits);
}
static void steps(std::ofstream& csv,const std::string& category,const std::string& name,
                  const Edits& edits, double hz, double dbBefore,double dbAfter,
                  const std::vector<double>& timeMs)
{
    auto p=std::make_unique<ThumpAudioProcessor>();setup(*p,edits);
    const int first=(int)(.25*SR),second=(int)(2.55*SR);
    const int count=first+second;
    std::vector<std::array<float,2>> x(count);
    for(int i=0;i<count;++i)
    {
        const double a=std::pow(10.0,(i<first?dbBefore:dbAfter)/20.0);
        const float s=(float)(a*std::sin(2*PI*hz*i/SR));x[i]={s,s};
    }
    // Keep only gain-reduction histories; continuous running processor, no reset.
    juce::AudioBuffer<float> b(2,BLOCK);juce::MidiBuffer midi;
    size_t next=0;
    for(int pos=0;pos<count;pos+=BLOCK)
    {
        const int n=std::min(BLOCK,count-pos);b.clear();
        for(int i=0;i<n;++i){b.setSample(0,i,x[pos+i][0]);b.setSample(1,i,x[pos+i][1]);}
        if(n<BLOCK){juce::AudioBuffer<float> partial(b.getArrayOfWritePointers(),2,0,n);p->processBlock(partial,midi);}
        else p->processBlock(b,midi);
        const double elapsed=1000.0*(pos+n-first)/SR;
        while(next<timeMs.size() && elapsed >= timeMs[next])
        {
            csv<<clean(category)<<","<<clean(name)<<","<<hz<<","<<dbBefore<<","<<dbAfter<<","<<timeMs[next]<<","
               <<commaFloat(p->lowReduction.load())<<","<<commaFloat(p->highReduction.load())<<","
               <<commaFloat(p->limitReduction.load())<<","<<clean(desc(edits))<<"\n";
            ++next;
        }
    }
}
int main(int argc,char**argv)
{
    juce::ScopedJuceInitialiser_GUI juceContext;
    const fs::path path=argc>1 ? fs::path(argv[1]) : fs::path("measurements/v0.5");
    fs::create_directories(path);
    std::ofstream resp(path/"frequency_phase.csv"), harm(path/"harmonics_thd.csv"), trans(path/"compressor_transients.csv");
    resp<<"category,setting,frequency_hz,gain_db,phase_deg,gain_vs_bypass_db,phase_vs_bypass_deg,right_gain_db,right_vs_left_db,left_minus_right_phase_deg,parameter_overrides\n";
    harm<<"category,setting,frequency_hz,input_peak_dbfs,fundamental_gain_db,fundamental_phase_deg,thd_k2_k20_percent,fundamental_peak_dbfs,output_peak_dbfs,output_rms_dbfs,low_gr_db,high_gr_db,limiter_gr_db";
    for(int k=2;k<=20;++k)harm<<",K"<<k<<"_dbc";
    harm<<",parameter_overrides\n";
    trans<<"category,setting,frequency_hz,level_before_dbfs,level_after_dbfs,elapsed_ms,low_gr_db,high_gr_db,limiter_gr_db,parameter_overrides\n";
    const std::vector<double> frequencies={20,30,40,60,80,100,120,160,220,320,500,800,1000,1500,2200,3500,5000,8000,10000,16000,20000};
    const auto reference=impulse({},false);
    const auto leftReference=impulse({},true);
    writeFreq(resp,"reference","all_bypassed_mono_off",{},reference,frequencies);
    for(float xover:{120.f,220.f,320.f,500.f,800.f})
    {
        for(auto side:{"low","high","both"})
        {
            Edits e={{"xover",xover},{"lowMix",std::string(side)=="high"?0.f:1.f},
                                         {"highMix",std::string(side)=="low"?0.f:1.f}};
            writeFreq(resp,"crossover",std::string(side)+"_"+std::to_string((int)xover),e,reference,frequencies);
        }
    }
    // EQ frequency, slider and Q sweeps (drives/comp/limiter bypassed).
    for(const auto& p:std::array<std::string,5>{"low40","low120","low220","high1500","high5000"})
    {
        for(float gain:{-12.f,-9.f,-6.f,-3.f,0.f,3.f,6.f,9.f,12.f})
        for(float q:{.3f,.65f,1.f,1.6f,3.f})
        {
            if(gain==0 && q!=1)continue;
            const Edits e={{p+"G",gain},{p+"Q",q}};
            writeFreq(resp,"eq",p+"_g"+std::to_string((int)gain)+"_q"+std::to_string(q),e,reference,frequencies);
        }
    }
    for(float lo:{0.f,.25f,.5f,.75f,1.f})
    for(float hi:{0.f,.25f,.5f,.75f,1.f})
    {
        Edits e={{"lowMix",lo},{"highMix",hi}};
        writeFreq(resp,"blend","low"+std::to_string(lo)+"_high"+std::to_string(hi),e,reference,frequencies);
    }
    for(float xover:{120.f,320.f,800.f})for(float mono:{0.f,1.f})
    {
        Edits e={{"mono",mono},{"xover",xover}};
        writeFreq(resp,"stereo","left_impulse_x"+std::to_string((int)xover)+"_mono"+std::to_string((int)mono),e,leftReference,frequencies,true);
    }
    // With saturation bypassed, processing order should not change with both
    // compressors bypassed. EQ on, all 6 permutations of each band.
    for(const auto& side:std::array<std::string,2>{"low","high"})
    for(int route=0;route<6;++route)
    {
        Edits e={{side+"Route",(float)route},{side=="low"?"low120G":"high1500G",6.f}};
        writeFreq(resp,"routing_linear",side+"_route"+std::to_string(route),e,reference,frequencies);
    }
    const auto testTone=[&](const std::string& category,const std::string& name,
                      double hz,double db,const Edits& e){tone(harm,category,name,hz,db,e);};
    for(double f:{40.,80.,100.,160.,220.,500.,1000.,1500.,5000.,10000.,16000.})
        testTone("reference","all_bypassed",f,-18.,{});
    // Measurements for each EQ at extremes and reference Q; harmonic generation must be tiny.
    for(const auto& p:std::array<std::string,5>{"low40","low120","low220","high1500","high5000"})
    for(float g:{-12.f,-6.f,0.f,6.f,12.f})
        testTone("eq_thd",p+"_"+std::to_string((int)g),p[0]=='l'?100.:1500.,-18.,{{p+"G",g}});
    for(const auto& side:std::array<std::string,2>{"low","high"})
    {
        const bool low=(side=="low");
        const std::string other=low?"high":"low";
        const float f=(float)(low?100.:1000.);
        for(int mode=0;mode<3;++mode)
        {
            for(float input:{-42.f,-36.f,-30.f,-24.f,-18.f,-12.f,-6.f})
            {
                Edits e={{side+"CompOn",1.f},{side+"Mode",(float)mode},
                         {side+"Thresh",-30.f},{side+"Makeup",0.f},
                         {other+"Mix",0.f},{side+"Mix",1.f}};
                testTone("compressor_transfer",side+"_mode"+std::to_string(mode)+"_level"+std::to_string((int)input),f,input,e);
            }
            for(float threshold:{-42.f,-36.f,-30.f,-24.f,-18.f})
            for(double hz:low?std::vector<double>{40,100,160,220}:std::vector<double>{800,1000,1500,5000,10000})
            {
                Edits e={{side+"CompOn",1.f},{side+"Mode",(float)mode},
                         {side+"Thresh",threshold},{side+"Makeup",0.f},
                         {other+"Mix",0.f}};
                testTone("compressor_harmonics",side+"_m"+std::to_string(mode)+"_t"+std::to_string((int)threshold),hz,-12.,e);
            }
        }
        for(int r=0;r<4;++r) for(float input:{-36.f,-24.f,-12.f,-6.f})
        {
            Edits e={{side+"CompOn",1},{side+"Mode",0},{side+"FetRatio",(float)r},
                     {side+"Thresh",-30.f},{other+"Mix",0.f}};
            testTone("ratio_fet",side+"_r"+std::to_string(r)+"_level"+std::to_string((int)input),f,input,e);
        }
        for(float r:{1.f,2.f,4.f,6.f,8.f,12.f,20.f,120.f})
        for(float input:{-36.f,-24.f,-12.f,-6.f})
        {
            Edits e={{side+"CompOn",1},{side+"Mode",1},{side+"VcaRatio",r},
                     {side+"Thresh",-30.f},{other+"Mix",0.f}};
            testTone("ratio_vca",side+"_ratio"+std::to_string((int)r)+"_level"+std::to_string((int)input),f,input,e);
        }
        for(int route=0;route<6;++route)
        {
            Edits e={{side+"CompOn",1},{side+"Mode",(float)(route%3)},
                     {side+"Thresh",-30.f},{side+"Route",(float)route},
                     {side=="low"?"low120G":"high1500G",6.f},{other+"Mix",0}};
            testTone("route_nonlinear",side+"_route"+std::to_string(route),f,-12.,e);
        }
        for(int mode=0;mode<3;++mode)
        {
            Edits e={{side+"CompOn",1.f},{side+"Mode",(float)mode},
                     {side+"Thresh",-30.f},{other+"Mix",0.f}};
            steps(trans,"compressor_attack",side+"_mode"+std::to_string(mode),e,f,-48,-10,
                  {0,2,5,10,15,20,40,80,120,250,500,1000,2000});
            steps(trans,"compressor_release",side+"_mode"+std::to_string(mode),e,f,-10,-48,
                  {0,2,5,10,20,40,80,100,150,250,500,800,1000,1500,2000,2300});
        }
    }
    for(float drive:{-12.f,-6.f,0.f,3.f,6.f,9.f,12.f,18.f})
    for(float mix:{0.f,.25f,.5f,1.f})
    for(float ceiling:{-12.f,-6.f,-1.f,0.f})
    for(double freq:{100.,1000.})
    {
        Edits e={{"limiterOn",1},{"limiterInput",drive},{"limiterCeiling",ceiling},{"limiterMix",mix}};
        testTone("limiter","lim_drive"+std::to_string((int)drive)+"_mix"+std::to_string(mix)+"_ceil"+std::to_string((int)ceiling),freq,-12.,e);
    }
    std::cout<<"THUMP v0.5 actual JUCE processBlock analysis written to "<<path.string()<<"\n";
    std::cout<<"All saturation modules explicitly OFF, 44.1kHz host, 4x internal DSP, 512 sample blocks.\n";
    return 0;
}
