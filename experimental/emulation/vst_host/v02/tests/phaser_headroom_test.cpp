#include "../Es5510V02Processor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>

using Processor=trollfx::esp::vst::v02::Processor;
constexpr double kPi=3.14159265358979323846;

void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}

double notchDb(double sampleRate,double toneHz){
  Processor p;require(p.prepare(sampleRate,256),"notch prepare failed");
  Processor::Parameters a;a.mode=Processor::Mode::Phaser;a.phaserCenterHz=static_cast<float>(toneHz);
  a.phaserDepthOctaves=0;a.phaserFeedback=0;a.wet=.5f;a.dry=.5f;
  std::array<float,256> l{},r{},ol{},orr{};double inputEnergy=0,outputEnergy=0;
  const int nframes=static_cast<int>(sampleRate*1.5);
  for(int offset=0;offset<nframes;offset+=256){const int n=std::min(256,nframes-offset);
    for(int i=0;i<n;++i){const auto idx=offset+i;l[i]=r[i]=.1f*std::sin(2*kPi*toneHz*idx/sampleRate);}
    p.processBlock(l.data(),r.data(),ol.data(),orr.data(),n,a);
    if(offset>sampleRate*.5)for(int i=0;i<n;++i){inputEnergy+=double(l[i])*l[i];outputEnergy+=double(ol[i])*ol[i];}
  }
  return 10.0*std::log10(outputEnergy/inputEnergy);
}

void hotBeat(double sampleRate,float level,float feedback){
  Processor p;require(p.prepare(sampleRate,512),"hot prepare failed");
  Processor::Parameters a;a.mode=Processor::Mode::Phaser;a.phaserFeedback=feedback;
  a.phaserCenterHz=1200;a.phaserDepthOctaves=2.5f;a.phaserRateHz=.31f;
  a.wet=.8f;a.dry=.6f;
  std::array<float,512> l{},r{},ol{},orr{};double peak=0;std::uint64_t clipped=0;std::uint64_t frames=0;
  const int nframes=static_cast<int>(sampleRate*2.0);
  for(int offset=0;offset<nframes;offset+=512){const int n=std::min(512,nframes-offset);
    // Deterministic kick/snare-like loop. Parameter updates every block mimic
    // automation, while the maximum feedback alternates polarity.
    a.phaserFeedback=(offset/static_cast<int>(512*sampleRate/8))%2?feedback:-feedback;
    a.phaserCenterHz=(offset/static_cast<int>(512*sampleRate/4))%2?650.0f:1600.0f;
    for(int i=0;i<n;++i){const int t=offset+i;const int quarter=static_cast<int>(sampleRate*.5);
      const int phase=t%quarter;const float kick=phase<static_cast<int>(sampleRate*.08)?std::exp(-phase/(sampleRate*.018))*std::sin(2*kPi*(48.0*phase/sampleRate+0.004*phase)):0.0f;
      const float snare=((t%static_cast<int>(sampleRate*.5))<64)?((t*1103515245u+12345u)&0xffff)/32768.0f-1.0f:0.0f;
      const float x=std::clamp(level*(.82f*kick+.18f*snare),-level,level);l[i]=r[i]=x;
    }
    p.processBlock(l.data(),r.data(),ol.data(),orr.data(),n,a);
    for(int i=0;i<n;++i){require(std::isfinite(ol[i])&&std::isfinite(orr[i]),"non-finite output");peak=std::max({peak,std::abs(double(ol[i])),std::abs(double(orr[i]))});if(std::abs(ol[i])>1||std::abs(orr[i])>1)++clipped;}
    frames+=static_cast<unsigned>(n);
  }
  require(clipped==0,"phaser output exceeded digital full scale");
  require(peak<=.981,"phaser guard did not maintain ceiling");
  std::cout<<"hot fs="<<sampleRate<<" input="<<level<<" fb="<<feedback<<" peak="<<peak<<" clipped="<<clipped<<" frames="<<frames<<"\n";
}

int main(){try{
  for(double sr:{44100.0,48000.0}){
    const double n500=notchDb(sr,500),n2000=notchDb(sr,2000);
    require(n500 < -24.0&&n2000 < -24.0,"headroom change weakened allpass notches");
    std::cout<<"notches fs="<<sr<<" 500Hz="<<n500<<"dB 2kHz="<<n2000<<"dB\n";
    for(float input:{.9885531f,.8912509f})for(float fb:{.9f,-.9f})hotBeat(sr,input,fb);
  }
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
