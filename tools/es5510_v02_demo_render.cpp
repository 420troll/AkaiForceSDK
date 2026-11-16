#include "../experimental/emulation/vst_host/v02/Es5510V02Processor.hpp"
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
using Processor=trollfx::esp::vst::v02::Processor;
void u16(std::ofstream&o,std::uint16_t x){o.put(char(x));o.put(char(x>>8));}
void u32(std::ofstream&o,std::uint32_t x){for(int i=0;i<4;++i)o.put(char(x>>(8*i)));}
void writeWav(const std::string& path,const std::vector<std::array<float,2>>& audio){std::ofstream o(path,std::ios::binary);if(!o)throw std::runtime_error("cannot open WAV output");const auto bytes=kFrames*4;o.write("RIFF",4);u32(o,bytes+36);o.write("WAVEfmt ",8);u32(o,16);u16(o,1);u16(o,2);u32(o,kRate);u32(o,kRate*4);u16(o,4);u16(o,16);o.write("data",4);u32(o,bytes);for(std::uint32_t i=0;i<kFrames;++i)for(int c=0;c<2;++c){long q=std::lround(std::clamp(audio[i][c],-1.0f,0.999969f)*32768.0f);u16(o,static_cast<std::uint16_t>(static_cast<std::int16_t>(q)));}if(!o)throw std::runtime_error("WAV write failed");}
std::pair<float,float> source(int mode,std::uint32_t n){const double t=double(n)/kRate;if(mode==0||mode==5){const float x=(n==0||n==kRate||n==2*kRate)?0.65f:0.0f;return {x,mode==5?0.0f:x*0.55f};}const float l=0.18f*std::sin(2*kPi*110*t)+0.10f*std::sin(2*kPi*440*t)+0.06f*std::sin(2*kPi*1760*t);const float r=0.16f*std::sin(2*kPi*165*t+0.3)+0.09f*std::sin(2*kPi*660*t+0.6)+0.05f*std::sin(2*kPi*1320*t+0.9);return {l,r};}
void render(const std::string& path,int mode){Processor p;if(!p.prepare(kRate,kBlock))throw std::runtime_error("processor prepare failed");std::vector<std::array<float,2>> audio(kFrames);Processor::Parameters a;a.mode=static_cast<Processor::Mode>(mode);a.delayMs=500;a.feedback=0.55f;a.wet=0.72f;a.dry=0.22f;
 if(mode==0){a.sync=true;a.tempoSource=trollfx::esp::vst::TempoSource::Internal;a.internalBpm=120;a.division=trollfx::esp::vst::Dp4Division::EighthDotted;a.dry=0;}
 if(mode==5){a.mode=Processor::Mode::Delay;a.pingPong=true;a.delayMs=250;a.feedback=0.65f;a.wet=0.7f;a.dry=0;}
 if(mode==1){a.chorusRateHz=0.45f;a.chorusDepthMs=5;a.wet=0.55f;}
 if(mode==2){a.phaserRateHz=0.18f;a.phaserCenterHz=900;a.phaserDepthOctaves=2.5f;a.phaserSpreadOctaves=0.3f;a.phaserFeedback=0.65f;}
 if(mode==3){a.panRateHz=0.22f;a.panMotion=0.9f;a.panWidth=1.1f;a.panWaveform=Processor::Waveform::Sine;}
 if(mode==4){a.rotarySpeed=Processor::RotarySpeed::Slow;a.hornSpeedHz=6.6f;a.drumSpeedHz=1.7f;a.accelerationSeconds=0.4f;a.decelerationSeconds=0.7f;a.dopplerDepthMs=2.0f;a.rotaryWidth=1.0f;a.drive=0.08f;}
 std::array<float,kBlock> inL{},inR{},outL{},outR{};
 for(std::uint32_t offset=0;offset<kFrames;offset+=kBlock){const auto count=std::min<std::uint32_t>(kBlock,kFrames-offset);if(mode==4){const double t=double(offset)/kRate;a.rotarySpeed=t<1.0?Processor::RotarySpeed::Slow:t<3.0?Processor::RotarySpeed::Fast:Processor::RotarySpeed::Slow;}
   for(std::uint32_t i=0;i<count;++i){auto x=source(mode,offset+i);inL[i]=x.first;inR[i]=x.second;}
   p.processBlock(inL.data(),inR.data(),outL.data(),outR.data(),count,a);
   for(std::uint32_t i=0;i<count;++i){audio[offset+i]={outL[i],outR[i]};}
 }
 writeWav(path,audio);std::cout<<path<<" frames="<<kFrames<<" processed="<<p.processedFrames()<<"\n";
}
}
int main(int argc,char**argv){try{const std::string dir=argc>1?argv[1]:"artifacts/es5510-emulation/v02";render(dir+"/tempo-delay-1-8-dotted-120bpm.wav",0);render(dir+"/chorus.wav",1);render(dir+"/stereo-phaser-12-stage.wav",2);render(dir+"/stereo-motion-panner.wav",3);render(dir+"/dual-band-rotary.wav",4);render(dir+"/stereo-pingpong-delay.wav",5);}catch(const std::exception&e){std::cerr<<"render failed: "<<e.what()<<'\n';return 1;}return 0;}
