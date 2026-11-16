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
constexpr std::array<std::uint64_t, 17> kPanner = {
#include "../panner_stereo_words.inc"
};
constexpr std::array<std::uint64_t, 19> kPingPong = {
#include "../../pingpong/stereo_pingpong_words.inc"
};
using trollfx::esp::MameES5510;

std::uint32_t q22(double x) {
    const auto q=std::clamp<long>(std::lround(x*4194304.0),0,0x600000);
    return static_cast<std::uint32_t>(q);
}
void matrix(MameES5510& cpu,double pan,double width) {
    pan=std::clamp(pan,-1.0,1.0); width=std::clamp(width,0.0,1.5);
    const double gl=pan>0.0?1.0-pan:1.0;
    const double gr=pan<0.0?1.0+pan:1.0;
    cpu.setRegister(40,q22(0.5*gl*(1.0+width)));
    cpu.setRegister(41,q22(0.5*gl*(1.0-width)));
    cpu.setRegister(42,q22(0.5*gr*(1.0-width)));
    cpu.setRegister(43,q22(0.5*gr*(1.0+width)));
}
void setup(MameES5510& cpu,const std::uint64_t* p,std::size_t n) {
    if(!cpu.prepare()||!cpu.loadProgram(p,n)) throw std::runtime_error("prepare/load failed");
    cpu.setHostSerial(0x20); cpu.setRegister(244,0);cpu.setRegister(245,0);cpu.setRegister(248,0);
}
std::array<std::int16_t,2> frame(MameES5510& cpu,int l,int r) {
    std::array<std::int16_t,6> in{0,0,0,0,static_cast<std::int16_t>(l),static_cast<std::int16_t>(r)};
    std::array<std::int16_t,2> out{};cpu.processFrame(in,out);return out;
}
void writeWav(const std::string& path,const std::vector<std::array<std::int16_t,2>>& v) {
    std::ofstream o(path,std::ios::binary);auto u16=[&](std::uint16_t x){o.put(char(x));o.put(char(x>>8));};auto u32=[&](std::uint32_t x){for(int i=0;i<4;++i)o.put(char(x>>(8*i)));};
    const auto bytes=static_cast<std::uint32_t>(v.size()*4);o.write("RIFF",4);u32(36+bytes);o.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(44100);u32(176400);u16(4);u16(16);o.write("data",4);u32(bytes);for(auto x:v){u16(static_cast<std::uint16_t>(x[0]));u16(static_cast<std::uint16_t>(x[1]));}if(!o)throw std::runtime_error("WAV write failed");
}
void testPanner() {
    MameES5510 cpu;setup(cpu,kPanner.data(),kPanner.size());
    matrix(cpu,0,1);auto y=frame(cpu,12000,-8000);if(y[0]!=12000||y[1]!=-8000)throw std::runtime_error("center unity matrix failed");
    matrix(cpu,0,0);y=frame(cpu,12000,-8000);if(std::abs(y[0]-2000)>1||std::abs(y[1]-2000)>1)throw std::runtime_error("mono width failed");
    matrix(cpu,-1,1);y=frame(cpu,12000,0);if(y[0]!=12000||y[1]!=0)throw std::runtime_error("left pan failed");
    matrix(cpu,1,1);y=frame(cpu,0,-8000);if(y[0]!=0||y[1]!=-8000)throw std::runtime_error("right pan failed");
    matrix(cpu,0,1.5);y=frame(cpu,12000,-8000);if(std::abs(y[0]-15000)>2||std::abs(y[1]+10000)>2)throw std::runtime_error("stereo width expansion failed");
}
void testPingPong() {
    MameES5510 cpu;setup(cpu,kPingPong.data(),kPingPong.size());
    constexpr std::uint32_t capacity=2048, delay=100;
    cpu.setRegister(245,capacity*2-1);cpu.setRegister(8,0);cpu.setRegister(9,capacity);
    cpu.setRegister(0,delay);cpu.setRegister(1,capacity+delay);
    cpu.setRegister(40,q22(0.65));cpu.setRegister(41,q22(1));cpu.setRegister(42,0);
    int firstL=-1,secondR=-1;
    for(int i=0;i<500;++i){auto y=frame(cpu,i==0?12000:0,0);if(y[0]!=0&&firstL<0)firstL=i;if(y[1]!=0&&secondR<0)secondR=i;}
    if(firstL!=static_cast<int>(delay)||secondR!=static_cast<int>(2*delay))throw std::runtime_error("alternating ping-pong echoes failed");
}

void makePingPongDemo(const std::string& path) {
    MameES5510 cpu;if(!cpu.prepare()||!cpu.loadProgram(kPingPong.data(),kPingPong.size()))throw std::runtime_error("ping-pong demo setup failed");
    constexpr std::uint32_t bank=44100,delay=11025;cpu.setHostSerial(0x20);cpu.setRegister(244,0);cpu.setRegister(245,2*bank-1);cpu.setRegister(248,0);
    cpu.setRegister(8,0);cpu.setRegister(9,bank);cpu.setRegister(0,delay);cpu.setRegister(1,bank+delay);
    cpu.setRegister(40,q22(0.65));cpu.setRegister(41,q22(0.7));cpu.setRegister(42,q22(0.0));
    std::vector<std::array<std::int16_t,2>> audio;audio.reserve(44100*4);
    for(int n=0;n<44100*4;++n){int pulse=(n==0||n==44100)?18000:0;audio.push_back(frame(cpu,pulse,0));}
    writeWav(path,audio);
}

void makeDemo(const std::string& path) {
    MameES5510 cpu;setup(cpu,kPanner.data(),kPanner.size());std::vector<std::array<std::int16_t,2>> audio;audio.reserve(44100*4);
    constexpr double pi=3.14159265358979323846;
    for(int i=0;i<44100*4;++i){double t=double(i)/44100;double pan=0.8*std::sin(2*pi*0.25*t);matrix(cpu,pan,1.25);int l=int(11000*std::sin(2*pi*110*t));int r=int(9000*std::sin(2*pi*220*t+0.7));audio.push_back(frame(cpu,l,r));}
    writeWav(path,audio);
}
}
int main(int argc,char**argv){try{testPanner();testPingPong();if(argc>1)makeDemo(argv[1]);if(argc>2)makePingPongDemo(argv[2]);std::cout<<"PASS panner 17 words; pingpong 19 words; center/mono/pan/width and alternating echo checks\n";}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}return 0;}
