#include "../../../mame_es5510_standalone.hpp"
#include "../ReverbPrograms.hpp"
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
constexpr std::uint32_t kRate=44100,kBank=32768,kFrames=kRate*2;
constexpr std::array<std::uint64_t,31> nonlin1_transient={
#include "../nonlin1_transient_words.inc"
};
constexpr std::array<std::uint64_t,61> nonlin2_bloom={
#include "../nonlin2_bloom_words.inc"
};
constexpr std::array<std::uint64_t,36> nonlin3_drum={
#include "../nonlin3_drum_words.inc"
};
constexpr std::array<std::uint64_t,41> small_plate={
#include "../small_plate_words.inc"
};
constexpr std::array<std::uint64_t,51> large_plate={
#include "../large_plate_words.inc"
};
struct Def {const char* name;const std::uint64_t* words;std::size_t count;std::array<std::pair<unsigned,float>,8> coeff;std::size_t coeffCount;std::uint32_t ldelay,rdelay;float feedback;unsigned writeOffset;};
constexpr std::array<Def,5> defs={{
 {"nonlin1_transient",nonlin1_transient.data(),31,{{{64,.48f}}},1,317,431,.24f,20},
 {"nonlin2_bloom",nonlin2_bloom.data(),61,{{{64,.62f},{65,-.58f},{66,.66f},{67,-.61f},{80,.62f},{81,-.58f},{82,.66f},{83,-.61f}}},8,1877,2347,.79f,50},
 {"nonlin3_drum",nonlin3_drum.data(),36,{{{64,.42f},{80,.42f}}},2,509,509,.46f,25},
 {"small_plate",small_plate.data(),41,{{{64,.58f},{65,-.52f},{80,.58f},{81,-.52f}}},4,1301,1499,.68f,30},
 {"large_plate",large_plate.data(),51,{{{64,.63f},{65,-.59f},{66,.67f},{80,.63f},{81,-.59f},{82,.67f}}},6,2801,3163,.82f,40},
}};
std::uint32_t q22(float x){const auto q=std::clamp<long>(std::lround(double(x)*4194304),-0x800000,0x7fffff);return std::uint32_t(q)&0xffffffu;}
void write16(std::ofstream&o,std::uint16_t x){o.put(char(x));o.put(char(x>>8));}
void write32(std::ofstream&o,std::uint32_t x){for(int i=0;i<4;++i)o.put(char(x>>(8*i)));}
void wav(const std::string&path,const std::vector<std::array<std::int16_t,2>>& x){std::ofstream o(path,std::ios::binary);if(!o)throw std::runtime_error("WAV open");auto bytes=std::uint32_t(x.size()*4);o.write("RIFF",4);write32(o,bytes+36);o.write("WAVEfmt ",8);write32(o,16);write16(o,1);write16(o,2);write32(o,kRate);write32(o,kRate*4);write16(o,4);write16(o,16);o.write("data",4);write32(o,bytes);for(auto f:x){write16(o,std::uint16_t(f[0]));write16(o,std::uint16_t(f[1]));}if(!o)throw std::runtime_error("WAV write");}
std::vector<std::array<std::int16_t,2>> render(const Def& d,bool beats){MameES5510 e;if(!e.prepare(kBank*2))throw std::runtime_error("prepare");e.setHostSerial(0x20);e.setRegister(244,0);e.setRegister(245,kBank*2-1);e.setRegister(248,0);e.setRegister(d.writeOffset,0);e.setRegister(d.writeOffset+1,kBank);e.setRegister(0,d.ldelay);e.setRegister(1,kBank+d.rdelay);e.setRegister(160,q22(d.feedback));e.setRegister(161,q22(1));e.setRegister(162,0);
 for(std::size_t i=0;i<d.coeffCount;++i)e.setRegister(d.coeff[i].first,q22(d.coeff[i].second));
 if(!e.loadProgram(d.words,d.count))throw std::runtime_error("program load");
 std::vector<std::array<std::int16_t,2>> out;out.reserve(kFrames);
 for(std::uint32_t n=0;n<kFrames;++n){std::int16_t x=0;if(beats){if(n%3528==0){const float env=(n%14112==0)?0.95f:0.72f;x=std::int16_t(std::lround(env*32767));}}else if(n==0)x=24576;
  std::array<std::int16_t,6> in{0,0,0,0,x,x};std::array<std::int16_t,2> y{};e.processFrame(in,y);out.push_back(y);}
 return out;}
struct Metrics {double early=0,tail=0;int peak=0,clips=0;double corr=0;std::size_t firstL=0,firstR=0;};
Metrics measure(const std::vector<std::array<std::int16_t,2>>& x){Metrics m;long double el=0,ta=0,lr=0,ll=0,rr=0;for(std::size_t i=0;i<x.size();++i){for(int c=0;c<2;++c){int a=std::abs(int(x[i][c]));m.peak=std::max(m.peak,a);m.clips+=a>=32767;if(i<kRate/10)el+=static_cast<long double>(a)*a;else ta+=static_cast<long double>(a)*a;}lr+=static_cast<long double>(x[i][0])*x[i][1];ll+=static_cast<long double>(x[i][0])*x[i][0];rr+=static_cast<long double>(x[i][1])*x[i][1];}m.firstL=x.size();m.firstR=x.size();for(std::size_t i=0;i<x.size();++i){if(m.firstL==x.size()&&x[i][0])m.firstL=i;if(m.firstR==x.size()&&x[i][1])m.firstR=i;}m.early=std::sqrt(double(el)/(std::min<std::size_t>(x.size(),kRate/10)*2));m.tail=std::sqrt(double(ta)/((x.size()-std::min<std::size_t>(x.size(),kRate/10))*2));m.corr=double(lr/std::sqrt(std::max<long double>(1,ll*rr)));return m;}
}
int main(int argc,char**argv){try{std::string dir=argc>1?argv[1]:".";for(const auto&d:defs){auto impulse=render(d,false);auto beats=render(d,true);auto a=measure(impulse),b=measure(beats);if(a.peak==0||b.peak==0)throw std::runtime_error(std::string(d.name)+" silence");if(b.clips>0)throw std::runtime_error(std::string(d.name)+" clips on hot beat fixture");wav(dir+"/"+d.name+"-impulse.wav",impulse);wav(dir+"/"+d.name+"-hot-beats.wav",beats);std::cout<<d.name<<" words="<<d.count<<" impulse_peak="<<a.peak<<" early_rms="<<a.early<<" tail_rms="<<a.tail<<" beat_peak="<<b.peak<<" beat_clips="<<b.clips<<" stereo_corr="<<a.corr<<" first_ms="<<1000.0*a.firstL/kRate<<"/"<<1000.0*a.firstR/kRate<<"\n";}std::cout<<"PASS all five ES5510 reverb prototypes, impulse and hot-beat stability\n";}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}return 0;}
