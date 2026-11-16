#include "../../../mame_es5510_standalone.hpp"
#include "../NonLinTapPrograms.hpp"
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
using trollfx::esp::MameES5510;
using namespace trollfx::esp::programs::nonlin_tap;
constexpr unsigned kBaseRate=44100,kBank=96000,kFrames=kBaseRate*2;
std::uint32_t q22(float x){long q=std::lround(double(x)*4194304);q=std::clamp(q,-0x800000L,0x7fffffL);return std::uint32_t(q)&0xffffffu;}
void u16(std::ofstream&o,std::uint16_t x){o.put(char(x));o.put(char(x>>8));}
void u32(std::ofstream&o,std::uint32_t x){for(int i=0;i<4;++i)o.put(char(x>>(8*i)));}
struct Report {std::uint64_t words=0;std::vector<std::array<std::int16_t,2>> frames;unsigned clips=0;std::size_t first=0,last=0;double corr=0;unsigned peak=0;};
Report render(const Definition& d,unsigned sampleRate,bool beats,const std::string& path,float levelScale=1.0f){
  MameES5510 cpu;const std::size_t dram=2*kBank;
  if(!cpu.prepare(dram))throw std::runtime_error("DRAM prepare failed");
  cpu.setHostSerial(0x20);cpu.setRegister(244,0);cpu.setRegister(245,dram-1);cpu.setRegister(248,0);
  cpu.setRegister(d.writeLeftPc,0);cpu.setRegister(d.writeRightPc,kBank);
  const double scale=double(sampleRate)/kBaseRate;
  for(unsigned i=0;i<9;++i){cpu.setRegister(d.readLeftPc[i],std::uint32_t(std::lround(d.tapLeft[i]*scale)));cpu.setRegister(d.readRightPc[i],kBank+std::uint32_t(std::lround(d.tapRight[i]*scale)));cpu.setRegister(150+i,q22(d.defaultLevels[i]*0.10f*levelScale));}
  for(std::size_t i=0;i<d.coefficientCount;++i)cpu.setRegister(d.coefficients[i].first,q22(d.coefficients[i].second));
  cpu.setRegister(180,q22(0.95f));cpu.setRegister(181,0);
  if(!cpu.loadProgram(d.words,d.wordCount))throw std::runtime_error("invalid microprogram");
  const unsigned frames=sampleRate*2;Report out;out.frames.resize(frames);out.first=frames;out.last=0;
  std::array<std::int16_t,6> in{};std::array<std::int16_t,2> y{};
  long double lr=0,ll=0,rr=0;
  for(unsigned n=0;n<frames;++n){
    in.fill(0);
    if(beats){if(n%(sampleRate/8)==0){const auto pulse=static_cast<std::int16_t>((n%(sampleRate/2)==0)?30000:22000);in[4]=pulse;in[5]=static_cast<std::int16_t>(pulse*0.7);}}
    else if(n==0){in[4]=30000;in[5]=18000;}
    cpu.processFrame(in,y);out.frames[n]=y;
    out.peak=std::max(out.peak,unsigned(std::max(std::abs(int(y[0])),std::abs(int(y[1])))));
    if(y[0]==32767||y[0]==-32768||y[1]==32767||y[1]==-32768)++out.clips;
    if((y[0]||y[1])&&out.first==frames)out.first=n;
    if(y[0]||y[1])out.last=n;
    lr+=static_cast<long double>(y[0])*y[1];ll+=static_cast<long double>(y[0])*y[0];rr+=static_cast<long double>(y[1])*y[1];
  }
  out.words=cpu.instructionsExecuted()/frames;out.corr=double(lr/std::sqrt(std::max<long double>(1,ll*rr)));
  if(!path.empty()){
    std::ofstream o(path,std::ios::binary);if(!o)throw std::runtime_error("WAV open failed");const auto bytes=frames*4;
    o.write("RIFF",4);u32(o,bytes+36);o.write("WAVEfmt ",8);u32(o,16);u16(o,1);u16(o,2);u32(o,sampleRate);u32(o,sampleRate*4);u16(o,4);u16(o,16);o.write("data",4);u32(o,bytes);
    for(auto f:out.frames){u16(o,std::uint16_t(f[0]));u16(o,std::uint16_t(f[1]));}if(!o)throw std::runtime_error("WAV write failed");
  }
  return out;
}
}

int main(int argc,char**argv){try{
  const std::string dir=argc>1?argv[1]:"/tmp";double diffs[3]{};
  for(unsigned fs:{44100u,48000u})for(const auto&d:kCatalog){
    const auto suffix=fs==44100?"44k1":"48k";
    auto impulse=render(d,fs,false,dir+"/"+d.name+"-"+suffix+"-impulse.wav");
    auto beats=render(d,fs,true,dir+"/"+d.name+"-"+suffix+"-beats.wav");
    if(impulse.words!=d.wordCount||beats.clips||impulse.first==impulse.frames.size()||impulse.last==0||beats.last==0)throw std::runtime_error(std::string(d.name)+" failed output/clip check");
    if(fs==44100)diffs[static_cast<unsigned>(d.id)]=double(impulse.last);
    if(fs==44100){
      const auto changed=render(d,fs,false,"",0.45f);
      if(changed.frames==impulse.frames)throw std::runtime_error(std::string(d.name)+" tap-level control has no audio effect");
    }
    std::cout<<suffix<<" "<<d.name<<" words="<<impulse.words<<" first="<<impulse.first<<" last="<<impulse.last<<" impulse_peak="<<impulse.peak<<" beat_peak="<<beats.peak<<" beat_clips="<<beats.clips<<" stereo_corr="<<impulse.corr<<"\n";
  }
  if(diffs[0]==diffs[1]||diffs[0]==diffs[2]||diffs[1]==diffs[2])throw std::runtime_error("nonlinear families share identical impulse end times");
  std::cout<<"PASS one-pass ES5510 tap-envelope networks at 44.1/48k; three decay/density timings differ\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}return 0;}
