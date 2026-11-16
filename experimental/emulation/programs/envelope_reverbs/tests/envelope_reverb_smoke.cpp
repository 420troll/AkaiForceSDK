#include "../../../mame_es5510_standalone.hpp"
#include "../program_metadata.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
using trollfx::esp::MameES5510;
using namespace trollfx::esp::programs::envelope_reverbs;
constexpr std::uint32_t kRate=44100,kFrames=kRate*2,kDelay=1103,kBank=kRate;
constexpr std::array<std::uint64_t,kGatedPlateEnvelopeWords> gated={
#include "../gated_plate_envelope_words.inc"
};
constexpr std::array<std::uint64_t,kReverseOneShotWords> reverse1={
#include "../reverse_one_shot_words.inc"
};
constexpr std::array<std::uint64_t,kReverseRetriggerWords> reverse2={
#include "../reverse_retrigger_words.inc"
};
std::uint32_t q22(float v){return static_cast<std::uint32_t>(std::lround(v*4194304.0f))&0xffffffu;}
void u16(std::ofstream&o,std::uint16_t x){o.put(char(x));o.put(char(x>>8));}
void u32(std::ofstream&o,std::uint32_t x){for(int i=0;i<4;++i)o.put(char(x>>(8*i)));}
struct Result{std::vector<std::array<float,2>> y;std::uint64_t executed=0;};
float envelope(unsigned kind,std::uint32_t n){
 const float t=float(n)/kRate;
 if(kind==0){if(t<0.03f)return t/0.03f;if(t<0.11f)return 1.0f;if(t<0.31f)return 1.0f-(t-0.11f)/0.20f;return 0.0f;}
 static constexpr float triggers[]={0.0f,0.50f,1.05f};
 const unsigned count=kind==1?1:3;float result=0;
 for(unsigned i=0;i<count;++i){const float age=t-triggers[i];if(age<0)continue;const float v=age<0.42f?age/0.42f:age<0.52f?1.0f:std::max(0.0f,1.0f-(age-0.52f)/0.08f);result=std::max(result,v);}
 return result;
}
template<std::size_t N>
Result render(const std::array<std::uint64_t,N>&program,unsigned writeL,unsigned writeR,unsigned kind,const char*path){
 MameES5510 cpu;if(!cpu.prepare(2*kBank)||!cpu.loadProgram(program.data(),program.size()))throw 1;cpu.setHostSerial(0x20);cpu.setRegister(244,0);cpu.setRegister(245,2*kBank-1);cpu.setRegister(248,0);cpu.setRegister(0,kDelay);cpu.setRegister(1,kBank+kDelay);cpu.setRegister(writeL,0);cpu.setRegister(writeR,kBank);cpu.setRegister(40,q22(0.62f));cpu.setRegister(41,q22(0.95f));cpu.setRegister(42,0);cpu.setRegister(50,q22(0.72f));cpu.setRegister(51,q22(0.42f));cpu.setRegister(43,0);
 Result r;r.y.resize(kFrames);std::array<std::int16_t,6> in{};std::array<std::int16_t,2> out{};
 for(std::uint32_t n=0;n<kFrames;++n){cpu.setRegister(43,q22(envelope(kind,n)));in.fill(0);if(n==0||n==kRate/2||n==kRate){in[4]=26000;in[5]=12000;}cpu.processFrame(in,out);r.y[n]={float(out[0])/32768.0f,float(out[1])/32768.0f};}
 r.executed=cpu.instructionsExecuted();std::ofstream o(path,std::ios::binary);if(o){const auto bytes=kFrames*4;o.write("RIFF",4);u32(o,bytes+36);o.write("WAVEfmt ",8);u32(o,16);u16(o,1);u16(o,2);u32(o,kRate);u32(o,kRate*4);u16(o,4);u16(o,16);o.write("data",4);u32(o,bytes);for(auto f:r.y)for(float x:f)u16(o,static_cast<std::uint16_t>(static_cast<std::int16_t>(std::clamp(std::lround(x*32768),-32768l,32767l))));}
 return r;
}
void report(const char*name,const Result&r){double e[4]={},peak=0;unsigned clips=0;for(std::uint32_t n=0;n<r.y.size();++n){for(float x:r.y[n]){peak=std::max(peak,double(std::abs(x)));if(std::abs(x)>=1)++clips;}const double energy=r.y[n][0]*r.y[n][0]+r.y[n][1]*r.y[n][1];e[std::min(3u,n/(kRate/2))]+=energy;}std::cout<<name<<" words="<<(r.executed/kFrames)<<" peak="<<peak<<" clips="<<clips<<" halfsec_energy="<<e[0]<<","<<e[1]<<","<<e[2]<<","<<e[3]<<"\n";}
}
int main(int argc,char**argv){const std::string dir=argc>1?argv[1]:"/tmp";try{auto g=render(gated,kGatedPlateEnvelopeWriteL,kGatedPlateEnvelopeWriteR,0,(dir+"/gated.wav").c_str());auto r1=render(reverse1,kReverseOneShotWriteL,kReverseOneShotWriteR,1,(dir+"/reverse-one-shot.wav").c_str());auto r2=render(reverse2,kReverseRetriggerWriteL,kReverseRetriggerWriteR,2,(dir+"/reverse-retrigger.wav").c_str());report("gated",g);report("reverse1",r1);report("reverse2",r2);double delta=0;for(std::size_t i=0;i<r1.y.size();++i)delta+=std::abs(r1.y[i][0]-r2.y[i][0])+std::abs(r1.y[i][1]-r2.y[i][1]);if(g.executed/kFrames!=kGatedPlateEnvelopeWords||r1.executed/kFrames!=kReverseOneShotWords||r2.executed/kFrames!=kReverseRetriggerWords||delta<0.1)return 1;std::cout<<"PASS ES5510 gated/reverse envelope renders; one-shot/retrigger delta="<<delta<<"\n";}catch(...){std::cerr<<"processor setup failed\n";return 2;}}
