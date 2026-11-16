#include "../experimental/emulation/vst_host/v02/Es5510V02Processor.hpp"
#include "../experimental/emulation/vst_host/v03/ReverbBank.hpp"

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
constexpr std::uint32_t kRate=44100,kFrames=kRate*4,kBlock=256;
constexpr double kPi=3.14159265358979323846;
using Core=trollfx::esp::vst::v02::Processor;
using Reverbs=trollfx::esp::vst::v03::ReverbBank;
void u16(std::ofstream&o,std::uint16_t v){o.put(char(v));o.put(char(v>>8));}
void u32(std::ofstream&o,std::uint32_t v){for(int i=0;i<4;++i)o.put(char(v>>(8*i)));}
void saveWav(const std::string& path,const std::vector<std::array<float,2>>& audio){
    std::ofstream o(path,std::ios::binary);if(!o)throw std::runtime_error("open WAV");const auto bytes=kFrames*4;
    o.write("RIFF",4);u32(o,bytes+36);o.write("WAVEfmt ",8);u32(o,16);u16(o,1);u16(o,2);u32(o,kRate);u32(o,kRate*4);u16(o,4);u16(o,16);o.write("data",4);u32(o,bytes);
    for(auto f:audio)for(float x:f){const auto s=static_cast<std::int16_t>(std::lround(std::clamp(x,-1.0f,0.999969f)*32768.0f));u16(o,static_cast<std::uint16_t>(s));}
    if(!o)throw std::runtime_error("write WAV");
}
std::pair<float,float> beat(std::uint32_t n){
    const auto phase=n%11025;const float pulse=phase<48?0.82f*std::exp(-static_cast<float>(phase)/13.0f):0.0f;
    const auto bar=n%44100;const float snare=(bar==11025||bar==33075)&&phase<80?0.48f*std::exp(-static_cast<float>(phase)/22.0f):0.0f;
    const float bass=0.12f*std::sin(2*kPi*55.0*n/kRate)*std::exp(-static_cast<float>(phase)/1200.0f);
    const float l=pulse+snare+bass,r=pulse*0.83f-snare*0.7f+bass*0.92f;
    return {std::clamp(l,-0.98f,0.98f),std::clamp(r,-0.98f,0.98f)};
}
template<class Process>
void render(const std::string& path,Process process){
    std::vector<std::array<float,2>> audio(kFrames);std::array<float,kBlock> inL{},inR{},outL{},outR{};
    for(std::uint32_t pos=0;pos<kFrames;pos+=kBlock){const auto count=std::min(kBlock,kFrames-pos);
        for(std::uint32_t i=0;i<count;++i){const auto x=beat(pos+i);inL[i]=x.first;inR[i]=x.second;}
        process(inL.data(),inR.data(),outL.data(),outR.data(),count);
        for(std::uint32_t i=0;i<count;++i)audio[pos+i]={outL[i],outR[i]};
    }
    double energyL=0,energyR=0,cross=0;float peak=0;std::uint32_t clips=0;
    for(auto f:audio){energyL+=double(f[0])*f[0];energyR+=double(f[1])*f[1];cross+=double(f[0])*f[1];const float framePeak=std::max(std::abs(f[0]),std::abs(f[1]));peak=std::max(peak,framePeak);clips+=framePeak>=1.0f;}
    saveWav(path,audio);const double corr=cross/std::sqrt(std::max(1e-30,energyL*energyR));
    std::cout<<path<<" peak="<<peak<<" corr="<<corr<<" clips="<<clips<<"\n";
    if(peak==0||clips)throw std::runtime_error("silent or clipped demo: "+path);
}
void renderCore(const std::string& path,Core::Mode mode,bool ping=false){Core p;if(!p.prepare(kRate,kBlock))throw std::runtime_error("base prepare");Core::Parameters a;a.mode=mode;a.pingPong=ping;a.wet=0.66f;a.dry=0.34f;a.delayMs=355;a.feedback=0.53f;a.chorusRateHz=0.48f;a.chorusDepthMs=4.8f;a.phaserRateHz=0.21f;a.phaserDepthOctaves=2.2f;a.phaserCenterHz=850;a.phaserFeedback=0.66f;a.panMotion=0.82f;a.panRateHz=0.23f;a.rotarySpeed=Core::RotarySpeed::Slow;
    render(path,[&](auto*l,auto*r,auto*ol,auto*orr,std::size_t n){p.processBlock(l,r,ol,orr,n,a);});}
void renderReverb(const std::string& path,Reverbs::Mode mode){Reverbs p;if(!p.prepare(kRate))throw std::runtime_error("reverb prepare");Reverbs::Parameters a;a.wet=0.72f;a.dry=0.18f;a.decaySeconds=mode==Reverbs::Mode::SmallPlate?1.4f:2.2f;a.predelayMs=17;a.diffusion1=59;a.diffusion2=46;a.earlyLevel=28;a.damping=40;a.attackMs=54;a.holdMs=92;a.releaseMs=280;a.triggerDb=-28;a.retriggerDb=-25;
    render(path,[&](auto*l,auto*r,auto*ol,auto*orr,std::size_t n){p.processBlock(mode,l,r,ol,orr,n,a);});}
}
int main(int argc,char**argv){try{
    const std::string dir=argc>1?argv[1]:"artifacts/es5510-emulation/v03/demos";
    renderCore(dir+"/stereo-delay.wav",Core::Mode::Delay);renderCore(dir+"/ping-pong-delay.wav",Core::Mode::Delay,true);
    renderCore(dir+"/chorus.wav",Core::Mode::Chorus);renderCore(dir+"/phaser-12-stage.wav",Core::Mode::Phaser);
    renderCore(dir+"/motion-panner.wav",Core::Mode::Panner);renderCore(dir+"/dual-band-rotary.wav",Core::Mode::Rotary);
    renderReverb(dir+"/small-plate.wav",Reverbs::Mode::SmallPlate);renderReverb(dir+"/large-plate.wav",Reverbs::Mode::LargePlate);
    renderReverb(dir+"/room.wav",Reverbs::Mode::Room);renderReverb(dir+"/hall.wav",Reverbs::Mode::Hall);
    renderReverb(dir+"/gated-reverb.wav",Reverbs::Mode::Gated);renderReverb(dir+"/reverse-one-shot.wav",Reverbs::Mode::ReverseOneShot);
    renderReverb(dir+"/reverse-retrigger.wav",Reverbs::Mode::ReverseRetrigger);
    renderReverb(dir+"/nonlin1-transient.wav",Reverbs::Mode::NonLin1);renderReverb(dir+"/nonlin2-bloom.wav",Reverbs::Mode::NonLin2);
    renderReverb(dir+"/nonlin3-drum.wav",Reverbs::Mode::NonLin3);
}catch(const std::exception&e){std::cerr<<"V0.3 demo render failed: "<<e.what()<<'\n';return 1;}return 0;}
