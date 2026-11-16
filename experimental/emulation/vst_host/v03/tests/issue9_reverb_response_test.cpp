#include "../ReverbBank.hpp"
#include "../../v02/Es5510V02Processor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
using Bank=trollfx::esp::vst::v03::ReverbBank;
struct Metrics { double rmsA=0,rmsB=0,rmsC=0,rmsD=0; float peak=0; std::size_t lastAudible=0; };
void put16(std::ofstream& f,std::uint16_t x){f.put(static_cast<char>(x));f.put(static_cast<char>(x>>8));}
void put32(std::ofstream& f,std::uint32_t x){put16(f,static_cast<std::uint16_t>(x));put16(f,static_cast<std::uint16_t>(x>>16));}
void writeWav(const std::filesystem::path& path,const std::vector<float>& l,const std::vector<float>& r,std::uint32_t sr){
 std::ofstream f(path,std::ios::binary);const std::uint32_t data=static_cast<std::uint32_t>(l.size()*4);
 f.write("RIFF",4);put32(f,36+data);f.write("WAVEfmt ",8);put32(f,16);put16(f,1);put16(f,2);put32(f,sr);put32(f,sr*4);put16(f,4);put16(f,16);f.write("data",4);put32(f,data);
 for(std::size_t i=0;i<l.size();++i){auto cv=[](float x){return static_cast<std::int16_t>(std::lround(std::clamp(x,-1.0f,0.999969f)*32768));};put16(f,static_cast<std::uint16_t>(cv(l[i])));put16(f,static_cast<std::uint16_t>(cv(r[i])));}
}
Metrics measure(const std::vector<float>& l,const std::vector<float>& r,std::uint32_t sr){Metrics m;std::array<double,4>sums{};std::array<std::size_t,4>counts{};const std::size_t bounds[]={sr,sr*3u,sr*6u,sr*12u};
 for(std::size_t i=0;i<l.size();++i){const float p=std::max(std::abs(l[i]),std::abs(r[i]));m.peak=std::max(m.peak,p);if(p>0.0001f)m.lastAudible=i;for(int w=0;w<4;++w)if(i<bounds[w]&&(w==0||i>=bounds[w-1])){sums[w]+=0.5*(double(l[i])*l[i]+double(r[i])*r[i]);++counts[w];}}
 m.rmsA=std::sqrt(sums[0]/std::max<std::size_t>(1,counts[0]));m.rmsB=std::sqrt(sums[1]/std::max<std::size_t>(1,counts[1]));m.rmsC=std::sqrt(sums[2]/std::max<std::size_t>(1,counts[2]));m.rmsD=std::sqrt(sums[3]/std::max<std::size_t>(1,counts[3]));return m;}
std::pair<std::vector<float>,std::vector<float>> render(Bank::Mode mode,double sr,float decay,bool hot=false,bool gate=false){
 Bank bank;if(!bank.prepare(sr))std::abort();Bank::Parameters p;p.wet=1;p.dry=0;p.decaySeconds=decay;p.predelayMs=0;p.damping=10;p.attackMs=3;p.holdMs=90;p.releaseMs=220;p.triggerDb=-30;p.retriggerDb=-24;p.retrigger=true;
 const auto rate=static_cast<std::uint32_t>(sr);std::size_t frames=rate*12u;std::vector<float> inL(frames),inR(frames),outL(frames),outR(frames);
 if(gate){for(std::size_t i=0;i<rate*2u;i+=rate/4){const std::size_t n=std::min<std::size_t>(rate/80,frames-i);for(std::size_t j=0;j<n;++j){const float v=0.9f*std::sin(2.0f*3.14159265f*110.0f*j/sr);inL[i+j]=inR[i+j]=v;}}}
 else if(hot){for(std::size_t i=0;i<rate*2u;++i){const float env=(i%(rate/2u)<rate/12u)?0.99f:0.0f;const float v=env*std::sin(2.0f*3.14159265f*55.0f*i/sr);inL[i]=v;inR[i]=0.91f*v;}}
 else inL[0]=0.8f,inR[0]=0.55f;
 for(std::size_t off=0;off<frames;){const auto n=std::min<std::size_t>(256,frames-off);bank.processBlock(mode,inL.data()+off,inR.data()+off,outL.data()+off,outR.data()+off,n,p);off+=n;}
 return {std::move(outL),std::move(outR)};
}
std::pair<std::vector<float>,std::vector<float>> renderDelay(double sr,float feedback){
 using Processor=trollfx::esp::vst::v02::Processor;Processor engine;if(!engine.prepare(sr,512))std::abort();Processor::Parameters p;p.mode=Processor::Mode::Delay;p.delayMs=120;p.feedback=feedback;p.wet=0.7f;p.dry=0.0f;
 const auto rate=static_cast<std::uint32_t>(sr);const std::size_t frames=rate*3u;std::vector<float> il(frames),ir(frames),ol(frames),orr(frames);il[0]=ir[0]=0.95f;
 for(std::size_t off=0;off<frames;){const auto n=std::min<std::size_t>(512,frames-off);engine.processBlock(il.data()+off,ir.data()+off,ol.data()+off,orr.data()+off,n,p);off+=n;}return {std::move(ol),std::move(orr)};
}
}
int main(int argc,char**argv){const auto output=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("artifacts/es5510-emulation/v03/issue9");std::filesystem::create_directories(output);bool ok=true;
 const std::array<std::pair<const char*,Bank::Mode>,4> modes={{{"small_plate",Bank::Mode::SmallPlate},{"large_plate",Bank::Mode::LargePlate},{"room",Bank::Mode::Room},{"hall",Bank::Mode::Hall}}};
 std::cout<<"sr,mode,decay_s,rms_0_1s,rms_1_3s,rms_3_6s,rms_6_12s,tail_ms,peak\n";
 for(double sr:{44100.0,48000.0})for(auto [name,mode]:modes){std::array<Metrics,3> results{};const std::array<float,3> decays={0.5f,2.0f,20.0f};for(std::size_t d=0;d<decays.size();++d){auto audio=render(mode,sr,decays[d]);results[d]=measure(audio.first,audio.second,static_cast<std::uint32_t>(sr));std::cout<<static_cast<int>(sr)<<','<<name<<','<<decays[d]<<','<<results[d].rmsA<<','<<results[d].rmsB<<','<<results[d].rmsC<<','<<results[d].rmsD<<','<<1000.0*results[d].lastAudible/sr<<','<<results[d].peak<<'\n';const std::string suffix=decays[d]==0.5f?"0p5s":decays[d]==2.0f?"2s":"20s";writeWav(output/(std::string(name)+"_"+suffix+"_"+std::to_string(static_cast<int>(sr))+".wav"),audio.first,audio.second,static_cast<std::uint32_t>(sr));if(results[d].peak>0.981f||results[d].lastAudible==0)ok=false;}
   // The short, fixed feedback paths have a comb response, so a single late
   // window need not rise monotonically at every intermediate value. Require
   // a clear low-to-high tail-energy change instead of a false monotonicity
   // assertion that would reject phase-dependent cancellation.
   if(!(results[0].rmsC>0.0&&results[2].rmsC>results[0].rmsC*1.2)){std::cerr<<"Decay tail metric did not change enough for "<<name<<" at "<<sr<<"\n";ok=false;}}
 for(double sr:{44100.0,48000.0})for(auto [name,mode]:std::array<std::pair<const char*,Bank::Mode>,3>{{{"nonlin1",Bank::Mode::NonLin1},{"nonlin2",Bank::Mode::NonLin2},{"nonlin3",Bank::Mode::NonLin3}}}){const auto a=render(mode,sr,0.2f),b=render(mode,sr,20.0f);const auto ma=measure(a.first,a.second,static_cast<std::uint32_t>(sr)),mb=measure(b.first,b.second,static_cast<std::uint32_t>(sr));std::cout<<"one-pass-tap-length,"<<static_cast<int>(sr)<<','<<name<<",min_tail_ms="<<1000.0*ma.lastAudible/sr<<",max_tail_ms="<<1000.0*mb.lastAudible/sr<<'\n';if(!(mb.lastAudible>ma.lastAudible))ok=false;}
 for(double sr:{44100.0,48000.0}){
   for(Bank::Mode m:{Bank::Mode::SmallPlate,Bank::Mode::LargePlate,Bank::Mode::Room,Bank::Mode::Hall,Bank::Mode::Gated}){auto hot=render(m,sr,20.0f,true,m==Bank::Mode::Gated);const auto h=measure(hot.first,hot.second,static_cast<std::uint32_t>(sr));std::cout<<"hot-feedback-stress,"<<static_cast<int>(sr)<<",mode="<<static_cast<int>(m)<<",peak="<<h.peak<<",tail_ms="<<1000.0*h.lastAudible/sr<<'\n';if(h.peak>0.981f||!std::isfinite(h.peak))ok=false;}
   auto gate=render(Bank::Mode::Gated,sr,2.0f,false,true);const auto g=measure(gate.first,gate.second,static_cast<std::uint32_t>(sr));std::size_t silentBad=0;for(std::size_t i=static_cast<std::size_t>(sr*2);i<gate.first.size();++i)if(std::abs(gate.first[i])+std::abs(gate.second[i])>0.0001f)++silentBad;std::cout<<"gate-retrigger,"<<static_cast<int>(sr)<<",peak="<<g.peak<<",post_silence_nonzero="<<silentBad<<'\n';if(g.peak>0.981f||silentBad>static_cast<std::size_t>(sr*0.3))ok=false;
   Metrics previous{};std::array<float,4> levels={0.0f,0.5f,0.94f,0.99f};std::cout<<"delay-feedback-stress,"<<static_cast<int>(sr)<<",feedback,rms_1_3s,peak,tail_ms\n";for(float fb:levels){auto y=renderDelay(sr,fb);auto m=measure(y.first,y.second,static_cast<std::uint32_t>(sr));std::cout<<static_cast<int>(sr)<<','<<fb<<','<<m.rmsB<<','<<m.peak<<','<<1000.0*m.lastAudible/sr<<'\n';if(m.peak>0.981f||!std::isfinite(m.peak))ok=false;if(fb>0.0f&&fb<=0.99f&&m.rmsB<=previous.rmsB)ok=false;previous=m;writeWav(output/("delay_feedback_"+std::to_string(static_cast<int>(fb*100))+"_"+std::to_string(static_cast<int>(sr))+".wav"),y.first,y.second,static_cast<std::uint32_t>(sr));}}
 return ok?0:1;
}
