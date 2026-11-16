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
using namespace trollfx::esp::programs::room_hall;
constexpr std::uint32_t kRate=44100,kFrames=kRate*2,kBank=kRate;
constexpr double kPi=3.14159265358979323846;
constexpr std::array<std::uint64_t,kRoomWords> room={
#include "../room_feedback_diffusion_words.inc"
};
constexpr std::array<std::uint64_t,kHallWords> hall={
#include "../hall_feedback_diffusion_words.inc"
};
std::uint32_t q22(float v){return static_cast<std::uint32_t>(std::lround(v*4194304.0f))&0xffffffu;}
void u16(std::ofstream&o,std::uint16_t x){o.put(char(x));o.put(char(x>>8));}
void u32(std::ofstream&o,std::uint32_t x){for(int i=0;i<4;++i)o.put(char(x>>(8*i)));}
struct Result{std::vector<std::array<float,2>> y;std::uint64_t executed=0;};
Result render(const std::uint64_t*program,std::size_t words,unsigned writeL,unsigned writeR,bool isHall,const char*wav){
 MameES5510 cpu;if(!cpu.prepare(2*kBank)||!cpu.loadProgram(program,words))throw 1;
 cpu.setHostSerial(0x20);cpu.setRegister(244,0);cpu.setRegister(245,2*kBank-1);cpu.setRegister(248,0);
 const std::uint32_t decayL=isHall?4200u:1050u,decayR=isHall?5100u:1277u;
 const std::uint32_t earlyL=isHall?730u:310u,earlyR=isHall?943u:437u;
 cpu.setRegister(0,decayL);cpu.setRegister(1,kBank+decayR);cpu.setRegister(4,earlyL);cpu.setRegister(5,kBank+earlyR);
 cpu.setRegister(writeL,0);cpu.setRegister(writeR,kBank);
 cpu.setRegister(40,q22(0.66f));cpu.setRegister(41,q22(0.24f));cpu.setRegister(42,q22(0.58f));cpu.setRegister(43,0);
 cpu.setRegister(50,q22(isHall?0.58f:0.78f));cpu.setRegister(51,q22(isHall?0.30f:0.0f));cpu.setRegister(52,q22(0.55f));cpu.setRegister(53,q22(0.55f));
 Result result;result.y.resize(kFrames);
 std::array<std::int16_t,6> in{};std::array<std::int16_t,2> out{};
 for(std::uint32_t n=0;n<kFrames;++n){in.fill(0);if(n==0){in[4]=28000;in[5]=11000;}cpu.processFrame(in,out);result.y[n]={float(out[0])/32768.0f,float(out[1])/32768.0f};}
 result.executed=cpu.instructionsExecuted();
 if(wav&&*wav){std::ofstream o(wav,std::ios::binary);const auto bytes=kFrames*4;o.write("RIFF",4);u32(o,bytes+36);o.write("WAVEfmt ",8);u32(o,16);u16(o,1);u16(o,2);u32(o,kRate);u32(o,kRate*4);u16(o,4);u16(o,16);o.write("data",4);u32(o,bytes);for(auto f:result.y)for(float x:f)u16(o,static_cast<std::uint16_t>(static_cast<std::int16_t>(std::clamp(std::lround(x*32768),-32768l,32767l))));}
 return result;
}
void metrics(const char*name,const Result&r){double a=0,b=0,cross=0;float peak=0;unsigned clips=0;std::uint32_t first=0;for(std::uint32_t i=0;i<r.y.size();++i){const auto l=r.y[i][0],rr=r.y[i][1];a+=l*l;b+=rr*rr;cross+=l*rr;peak=std::max(peak,std::max(std::abs(l),std::abs(rr)));if(std::abs(l)>=1||std::abs(rr)>=1)++clips;if(!first&&(l!=0||rr!=0))first=i;}const double corr=cross/std::sqrt(std::max(1e-30,a*b));double early=0,late=0;for(std::uint32_t i=0;i<r.y.size();++i){const double e=r.y[i][0]*r.y[i][0]+r.y[i][1]*r.y[i][1];if(i<kRate/10)early+=e;else late+=e;}std::cout<<name<<" words="<<(r.executed/kFrames)<<" first="<<first<<" peak="<<peak<<" clips="<<clips<<" energy_0_100ms="<<early<<" energy_100_2000ms="<<late<<" stereo_corr="<<corr<<"\n";}
}
int main(int argc,char**argv){try{auto r=render(room.data(),room.size(),kRoomWriteL,kRoomWriteR,false,argc>1?argv[1]:"");auto h=render(hall.data(),hall.size(),kHallWriteL,kHallWriteR,true,argc>2?argv[2]:"");metrics("room",r);metrics("hall",h);if(r.executed/kFrames!=kRoomWords||h.executed/kFrames!=kHallWords)return 1;auto diff=0.0;for(std::size_t i=0;i<r.y.size();++i)diff+=std::abs(r.y[i][0]-h.y[i][0])+std::abs(r.y[i][1]-h.y[i][1]);if(diff<1.0){std::cerr<<"room/hall render difference too small\n";return 2;}std::cout<<"PASS distinct room/hall ES5510 instruction renders; difference="<<diff<<"\n";}catch(...){std::cerr<<"processor setup failed\n";return 3;}}
