// THUMP v0.6: processor-level signal-flow smoke tests, 44.1kHz, 256 samples.
// Runs the real JUCE processor, not an offline approximation.
#include "../Source/PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <initializer_list>
#include <string>
#include <algorithm>

using Edit = std::pair<const char*, float>;
constexpr double fs = 44100.;
constexpr int block = 256;
constexpr int samples = 44100;
constexpr double pi = 3.14159265358979323846;

static void set (ThumpAudioProcessor& p, const char* id, float value)
{
    auto* parameter = p.apvts.getParameter (id);
    if (!parameter) throw std::runtime_error(std::string("Missing parameter ")+id);
    parameter->setValueNotifyingHost (parameter->convertTo0to1(value));
}

static std::vector<float> run (double hz, float amp, std::initializer_list<Edit> edits)
{
    ThumpAudioProcessor p;
    for (const auto& e : std::initializer_list<Edit> {
            {"input",0}, {"output",0}, {"mono",0}, {"xover",320},
            {"lowSatOn",0}, {"highSatOn",0},
            {"lowCompOn",0}, {"highCompOn",0},
            {"lowMakeup",0}, {"highMakeup",0},
            {"lowMix",1}, {"highMix",1}, {"limiterOn",0},
            {"hpfSlope",0}, {"dynOn",0}, {"highMidG",0},
            {"highDriveType",0}, {"glueOn",1} })
        set(p,e.first,e.second);
    for (const auto& e: edits) set(p,e.first,e.second);
    p.prepareToPlay(fs,block);
    juce::AudioBuffer<float> buffer(2,block);
    juce::MidiBuffer midi;
    std::vector<float> output;
    output.reserve(samples);
    for(int pos=0;pos<samples;pos+=block)
    {
        const int n=std::min(block,samples-pos);
        for(int i=0;i<n;++i)
        {
            const float x=amp*(float)std::sin(2.0*pi*hz*(pos+i)/fs);
            buffer.setSample(0,i,x);
            buffer.setSample(1,i,x);
        }
        if(n<block)
        {
            juce::AudioBuffer<float> part(buffer.getArrayOfWritePointers(),2,0,n);
            p.processBlock(part,midi);
        }
        else p.processBlock(buffer,midi);
        for(int i=0;i<n;++i)
        {
            const float l=buffer.getSample(0,i), r=buffer.getSample(1,i);
            if(!std::isfinite(l) || !std::isfinite(r))
                throw std::runtime_error("Nonfinite processed audio");
            if(std::abs(l-r)>1.e-5f)
                throw std::runtime_error("Identical stereo input changed L/R balance");
            output.push_back(l);
        }
    }
    return output;
}

static double rms(const std::vector<float>& values)
{
    double sum=0;
    const size_t start=values.size()*3/4; // Test settled signal after 750ms.
    for(size_t n=start;n<values.size();++n) sum+=(double)values[n]*values[n];
    return std::sqrt(sum/(values.size()-start));
}
static double maximum(const std::vector<float>& values)
{
    double peak=0;
    for(float v:values)peak=std::max(peak,(double)std::abs(v));
    return peak;
}
static void require(bool condition,const char* message)
{
    if(!condition)throw std::runtime_error(message);
}

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI juceContext;
        const auto bypass=run(30,0.25f,{});
        const auto hpf12=run(30,0.25f,{{"hpfHz",35},{"hpfSlope",1}});
        const auto hpf24=run(30,0.25f,{{"hpfHz",35},{"hpfSlope",2}});
        require(rms(hpf12)<rms(bypass)*.90,"HPF 12dB attenuation ineffective");
        require(rms(hpf24)<rms(hpf12)*.90,"HPF 24dB not steeper than 12dB");

        const auto bassPlain=run(100,0.25f,{{"lowMix",1},{"highMix",0}});
        const auto bassDyn=run(100,0.25f,{{"lowMix",1},{"highMix",0},
             {"dynOn",1},{"dynHz",100},{"dynThresh",-35},{"dynCut",8}});
        require(rms(bassDyn)<rms(bassPlain)*.90,"Dynamic bass EQ did not reduce loud note");

        const auto midPlain=run(800,0.1f,{{"lowMix",0},{"highMix",1}});
        const auto midBoost=run(800,0.1f,{{"lowMix",0},{"highMix",1},
             {"highMidG",8},{"highMidHz",800}});
        require(rms(midBoost)>rms(midPlain)*1.4,"Sweepable mid bell has no boost");

        const auto ts=run(1000,0.2f,{{"lowMix",0},{"highMix",1},
             {"highSatOn",1},{"highSatMix",1},{"highDrive",10},{"highDriveType",0}});
        const auto amp=run(1000,0.2f,{{"lowMix",0},{"highMix",1},
             {"highSatOn",1},{"highSatMix",1},{"highDrive",10},{"highDriveType",1}});
        const auto fuzz=run(1000,0.2f,{{"lowMix",0},{"highMix",1},
             {"highSatOn",1},{"highSatMix",1},{"highDrive",10},{"highDriveType",2}});
        auto difference=[](const std::vector<float>& a,const std::vector<float>& b)
        {
            double v=0;
            for(size_t i=a.size()*3/4;i<a.size();++i)
                v+=std::abs(a[i]-b[i]);
            return v/(a.size()/4);
        };
        require(difference(ts,amp)>0.005,"Amp and TS modes too similar");
        require(difference(ts,fuzz)>0.005,"Fuzz and TS modes too similar");

        const auto limited=run(100,0.8f,{{"limiterOn",1},{"limiterInput",12},
             {"limiterCeiling",-6},{"limiterMix",1},{"glueOn",0}});
        require(maximum(limited)<=std::pow(10.,-6./20.)+0.0001,
            "100% wet limiter exceeded sample peak ceiling");

        std::cout<<"PASS: THUMP v0.6 real processor: stereo, 12/24dB HPF, "
                   "dynamic low EQ, swept high-mid, 3 distinct drives, sample ceiling\n";
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr<<"FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
