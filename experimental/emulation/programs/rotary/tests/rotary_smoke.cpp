#include "../../../mame_es5510_standalone.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr std::array<std::uint64_t,40> kProgram = {
#include "../rotary_2band_words.inc"
};
constexpr double kPi=3.1415926535897932384626433832795;
constexpr std::uint32_t kRate=44100;
constexpr std::uint32_t kBank=2048;

std::uint32_t q22(float x) {
    long v=std::lround(static_cast<double>(x)*4194304.0);
    v=std::clamp(v,-4194304L,4194303L);
    return static_cast<std::uint32_t>(v)&0xffffffu;
}

struct Renderer {
    trollfx::esp::MameES5510 cpu;
    Renderer() {
        if(!cpu.prepare()||!cpu.loadProgram(kProgram.data(),kProgram.size()))
            throw std::runtime_error("ES5510 rotary program setup failed");
        cpu.setHostSerial(0x20);
        cpu.setRegister(244,0);
        cpu.setRegister(245,2*kBank-1);
        cpu.setRegister(248,0);
        cpu.setRegister(19,0);           // DOL high band at DBASE
        cpu.setRegister(20,kBank);       // DOL low band in second ring bank
        const float g=1.0f-std::exp(-2.0f*static_cast<float>(kPi)*700.0f/kRate);
        cpu.setRegister(54,q22(g));
        setStatic(4,5);
    }
    void setStatic(std::uint32_t hornDelay,std::uint32_t drumDelay) {
        cpu.setRegister(21,std::clamp(hornDelay,2u,kBank-2));
        cpu.setRegister(24,kBank+std::clamp(drumDelay,2u,kBank-2));
        cpu.setRegister(50,q22(0.5f)); cpu.setRegister(51,q22(0.5f));
        cpu.setRegister(52,q22(0.5f)); cpu.setRegister(53,q22(0.5f));
    }
    std::array<std::int16_t,2> frame(float input) {
        const long q=std::lround(std::clamp(input,-1.0f,0.999969f)*32768.0f);
        const auto pcm=static_cast<std::int16_t>(q);
        const std::array<std::int16_t,6> in={0,0,0,0,pcm,pcm};
        std::array<std::int16_t,2> out{};
        cpu.processFrame(in,out);
        return out;
    }
};

float response(double hz,bool horn) {
    Renderer r;
    r.setStatic(3,4);
    r.cpu.setRegister(50,q22(horn?0.5f:0.0f));
    r.cpu.setRegister(51,0);
    r.cpu.setRegister(52,q22(horn?0.0f:0.5f));
    r.cpu.setRegister(53,0);
    constexpr int warm=22050, measure=22050;
    double sum=0.0;
    for(int n=0;n<warm+measure;n++) {
        const float x=0.2f*static_cast<float>(std::sin(2.0*kPi*hz*n/kRate));
        const auto o=r.frame(x);
        if(n>=warm) sum+=static_cast<double>(o[0])*o[0];
    }
    return static_cast<float>(std::sqrt(sum/measure)/32768.0);
}

struct Speed {
    double current=0.0;
    double phase=0.0;
    void advance(double targetHz,double accelerationSeconds,double fs) {
        const double tau=std::max(accelerationSeconds,1.0/fs);
        current += (targetHz-current)*(1.0-std::exp(-1.0/(tau*fs)));
        phase += 2.0*kPi*current/fs;
        if(phase>=2.0*kPi) phase-=2.0*kPi*std::floor(phase/(2.0*kPi));
    }
};

float panGain(double phase,bool left) {
    const double pan=0.5+0.5*std::sin(phase);
    const double g=left?std::sqrt(1.0-pan):std::sqrt(pan);
    return static_cast<float>(g);
}

std::vector<std::array<std::int16_t,2>> renderMotion(double accelSec,
                                                     double hornFastHz,
                                                     double drumFastHz) {
    Renderer r;
    Speed horn,drum;
    constexpr int frames=44100*4;
    std::vector<std::array<std::int16_t,2>> audio;
    audio.reserve(frames);
    for(int n=0;n<frames;n++) {
        const double t=static_cast<double>(n)/kRate;
        const double hornTarget=t<1.0?1.2:(t<3.0?hornFastHz:1.2);
        const double drumTarget=t<1.0?0.65:(t<3.0?drumFastHz:0.65);
        horn.advance(hornTarget,accelSec,kRate);
        drum.advance(drumTarget,accelSec*1.6,kRate);
        const int hornDelay=static_cast<int>(std::lround(132.0+88.0*std::sin(horn.phase)));
        const int drumDelay=static_cast<int>(std::lround(264.0+176.0*std::sin(drum.phase)));
        r.cpu.setRegister(21,static_cast<std::uint32_t>(std::clamp(hornDelay,2,500)));
        r.cpu.setRegister(24,kBank+static_cast<std::uint32_t>(std::clamp(drumDelay,2,500)));
        r.cpu.setRegister(50,q22(0.12f+0.88f*panGain(horn.phase,true)));
        r.cpu.setRegister(51,q22(0.12f+0.88f*panGain(horn.phase,false)));
        r.cpu.setRegister(52,q22(0.12f+0.88f*panGain(drum.phase,true)));
        r.cpu.setRegister(53,q22(0.12f+0.88f*panGain(drum.phase,false)));
        const float dry=0.28f*static_cast<float>(std::sin(2*kPi*110.0*t))+
                        0.14f*static_cast<float>(std::sin(2*kPi*440.0*t))+
                        0.08f*static_cast<float>(std::sin(2*kPi*1760.0*t));
        audio.push_back(r.frame(dry));
    }
    return audio;
}

void writeWav(const std::string& path,const std::vector<std::array<std::int16_t,2>>& frames) {
    std::ofstream out(path,std::ios::binary);
    const auto u16=[&out](std::uint16_t x){out.put(static_cast<char>(x));out.put(static_cast<char>(x>>8));};
    const auto u32=[&out](std::uint32_t x){for(int i=0;i<4;i++)out.put(static_cast<char>(x>>(8*i)));};
    const std::uint32_t bytes=static_cast<std::uint32_t>(frames.size()*4);
    out.write("RIFF",4);u32(bytes+36);out.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(kRate);u32(kRate*4);u16(4);u16(16);out.write("data",4);u32(bytes);
    for(auto f:frames){u16(static_cast<std::uint16_t>(f[0]));u16(static_cast<std::uint16_t>(f[1]));}
    if(!out)throw std::runtime_error("WAV write failed");
}

} // namespace

int main(int argc,char** argv) {
    try {
        const std::string outDir=argc>1?argv[1]:".";
        const float lowLow=response(100.0,false), lowHigh=response(5000.0,false);
        const float hornLow=response(100.0,true), hornHigh=response(5000.0,true);
        const float lowReject=20*std::log10(std::max(lowHigh,1.0e-9f)/std::max(lowLow,1.0e-9f));
        const float hornReject=20*std::log10(std::max(hornHigh,1.0e-9f)/std::max(hornLow,1.0e-9f));
        if(lowLow<lowHigh*2.0f || hornHigh<hornLow*2.0f)
            throw std::runtime_error("low/high band responses are not differentiated");

        const auto slow=renderMotion(0.2,1.4,0.8);
        const auto fast=renderMotion(0.2,6.6,1.7);
        const auto hornRateOnly=renderMotion(0.2,1.4,1.7);
        const auto drumRateOnly=renderMotion(0.2,6.6,0.8);
        const auto slowAccel=renderMotion(1.5,6.6,1.7);
        auto rmsDiff=[](const auto& a,const auto& b){double s=0;for(std::size_t i=0;i<a.size();i++){double d=double(a[i][0])-b[i][0];s+=d*d;}return std::sqrt(s/a.size())/32768.0;};
        const double rateDifference=rmsDiff(slow,fast);
        const double hornRateDifference=rmsDiff(fast,hornRateOnly);
        const double drumRateDifference=rmsDiff(fast,drumRateOnly);
        const double accelerationDifference=rmsDiff(fast,slowAccel);
        if(rateDifference<0.001 || hornRateDifference<0.001 || drumRateDifference<0.001 || accelerationDifference<0.0001)
            throw std::runtime_error("horn/drum speed or acceleration schedule did not affect output");

        double corr=0,ll=0,rr=0; unsigned clips=0;
        for(const auto& frame:fast){const double l=frame[0],r=frame[1];corr+=l*r;ll+=l*l;rr+=r*r;if(frame[0]==32767||frame[0]==-32768||frame[1]==32767||frame[1]==-32768)clips++;}
        const double stereoCorrelation=corr/std::sqrt(ll*rr);
        if(clips!=0)throw std::runtime_error("rotary demo clipped");
        if(std::abs(stereoCorrelation)>0.9999)throw std::runtime_error("independent rotor movement collapsed stereo");
        writeWav(outDir+"/rotary_dual_band_demo.wav",fast);
        std::ofstream csv(outDir+"/rotary_band_response.csv");
        csv<<"path,100_hz_rms,5000_hz_rms,relative_change_db\n"
           <<"drum_low,"<<lowLow<<','<<lowHigh<<','<<lowReject<<'\n'
           <<"horn_high,"<<hornLow<<','<<hornHigh<<','<<hornReject<<'\n';
        if(!csv)throw std::runtime_error("CSV write failed");
        std::cout<<"program_words="<<kProgram.size()<<" crossover_two_one_poles_hz=700"
                 <<" drum_low_100hz_rms="<<lowLow<<" drum_low_5khz_rms="<<lowHigh
                 <<" horn_high_100hz_rms="<<hornLow<<" horn_high_5khz_rms="<<hornHigh
                 <<" low_band_change_db="<<lowReject<<" high_band_change_db="<<hornReject
                 <<" speed_schedule_rms_delta="<<rateDifference
                 <<" horn_rate_rms_delta="<<hornRateDifference
                 <<" drum_rate_rms_delta="<<drumRateDifference
                 <<" acceleration_rms_delta="<<accelerationDifference
                 <<" stereo_corr="<<stereoCorrelation<<" clips="<<clips<<'\n';
    } catch(const std::exception& e) { std::cerr<<"rotary test failed: "<<e.what()<<'\n';return 1; }
    return 0;
}
