#include "dp4_reverb.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
constexpr int kRate=44100;
constexpr std::size_t kFrames=static_cast<std::size_t>(kRate)*6;
struct Render { std::vector<float> l,r; };

void put16(std::ofstream& f,std::uint16_t x) { f.put(static_cast<char>(x)); f.put(static_cast<char>(x>>8)); }
void put32(std::ofstream& f,std::uint32_t x) { for(int i=0;i<4;++i) f.put(static_cast<char>(x>>(8*i))); }
bool writeWav(const std::string& path,const Render& x) {
    std::ofstream f(path,std::ios::binary); if(!f) return false;
    const auto bytes=static_cast<std::uint32_t>(x.l.size()*4);
    f.write("RIFF",4);put32(f,36+bytes);f.write("WAVEfmt ",8);put32(f,16);put16(f,1);put16(f,2);
    put32(f,kRate);put32(f,kRate*4);put16(f,4);put16(f,16);f.write("data",4);put32(f,bytes);
    for(std::size_t i=0;i<x.l.size();++i) for(float v:{x.l[i],x.r[i]}) {
        const auto s=static_cast<std::int16_t>(std::lrint(std::clamp(v,-1.0f,1.0f)*32767.0f));put16(f,static_cast<std::uint16_t>(s));
    }
    return bool(f);
}
Render render(trollfx::ReverbKind kind,float decay,const std::vector<float>& inputL,const std::vector<float>& inputR) {
    trollfx::DP4InspiredReverb rv;
    if(!rv.prepare(static_cast<float>(kRate))) throw 1;
    rv.setKind(kind);
    trollfx::ReverbParams p; p.decaySeconds=decay;p.preDelayMs=20;p.diffusion=.72f;p.damping=.34f;
    p.earlyLevel1=.50f;p.earlyLevel2=.36f;p.earlyTime1Ms=18;p.earlyTime2Ms=43;
    p.width=1;p.mix=1;p.outputDb=0;p.modRateHz=.23f;p.modDepthMs=.8f;rv.setParams(p);
    Render out{std::vector<float>(inputL.size()),std::vector<float>(inputL.size())};
    for(std::size_t i=0;i<inputL.size();i+=128) {
        const auto n=std::min<std::size_t>(128,inputL.size()-i);
        rv.process(inputL.data()+i,inputR.data()+i,out.l.data()+i,out.r.data()+i,n);
    }
    return out;
}
double energy(const Render& x,double a,double b) {
    const auto lo=static_cast<std::size_t>(a*kRate),hi=std::min(x.l.size(),static_cast<std::size_t>(b*kRate));
    double e=0;for(auto i=lo;i<hi;++i)e+=double(x.l[i])*x.l[i]+double(x.r[i])*x.r[i];return e;
}
double correlation(const Render& x,double a,double b) {
    const auto lo=static_cast<std::size_t>(a*kRate),hi=std::min(x.l.size(),static_cast<std::size_t>(b*kRate));
    double lr=0,ll=0,rr=0;for(auto i=lo;i<hi;++i){lr+=double(x.l[i])*x.r[i];ll+=double(x.l[i])*x.l[i];rr+=double(x.r[i])*x.r[i];}
    return lr/std::sqrt(std::max(1e-30,ll*rr));
}
std::pair<double,std::size_t> peak(const Render& x,double a,double b) {
    const auto lo=static_cast<std::size_t>(a*kRate),hi=std::min(x.l.size(),static_cast<std::size_t>(b*kRate));
    double m=0;std::size_t at=lo;for(auto i=lo;i<hi;++i){const double v=std::max(std::abs(x.l[i]),std::abs(x.r[i]));if(v>m){m=v;at=i;}}
    return {m,at};
}
std::size_t density(const Render& x,double a,double b,double threshold) {
    const auto lo=static_cast<std::size_t>(a*kRate),hi=std::min(x.l.size(),static_cast<std::size_t>(b*kRate));
    std::size_t n=0;for(auto i=lo;i<hi;++i)if(std::abs(x.l[i])>threshold||std::abs(x.r[i])>threshold)++n;return n;
}
void add(Render& dst,const Render& src,std::size_t offset) {
    for(std::size_t i=0;i<src.l.size()&&offset+i<dst.l.size();++i){dst.l[offset+i]+=src.l[i];dst.r[offset+i]+=src.r[i];}
}
}

int main(int argc,char**argv) {
    const std::string dir=argc>1?argv[1]:"artifacts/esp-lab-v03-reverb-comparison";
    std::filesystem::create_directories(dir);
    std::vector<float> il(kFrames,0),ir(kFrames,0);il[0]=ir[0]=.8f;
    constexpr std::array<trollfx::ReverbKind,3> kinds{{trollfx::ReverbKind::Room,trollfx::ReverbKind::Hall,trollfx::ReverbKind::Plate}};
    constexpr std::array<const char*,3> names{{"Room","Hall","Plate"}};
    std::array<Render,3> responses;
    std::ofstream report(dir+"/MEASUREMENTS.txt");
    if(!report)return 2;
    report<<"Matched wet-only impulse responses, 44.1 kHz stereo, 16-bit PCM\n"
          <<"Input: simultaneous 0.8 impulse; 20 ms predelay; 2.5 s decay; diffusion .72; damping .34; width 1.0; identical early-reflection controls.\n"
          <<"Implementations are original manual-informed structures, not factory algorithm captures.\n\n";
    for(std::size_t a=0;a<3;++a) {
        responses[a]=render(kinds[a],2.5f,il,ir);
        if(!writeWav(dir+"/"+std::string(names[a])+"-IR-44100.wav",responses[a]))return 3;
        const auto first=peak(responses[a],.0,.25);
        const double e01=energy(responses[a],.1,.2),e5=energy(responses[a],.5,.6);
        const double e1=energy(responses[a],1,1.1),e2=energy(responses[a],2,2.1),e4=energy(responses[a],4,4.1);
        report<<names[a]<<"\n  first 0-250 ms peak="<<first.first<<" at "<<1000.0*first.second/kRate<<" ms"
              <<"\n  early active samples (>10% peak, 30-250 ms)="<<density(responses[a],.03,.25,first.first*.1)
              <<"\n  energy [0.1-0.2, 0.5-0.6, 1.0-1.1, 2.0-2.1, 4.0-4.1] s="
              <<e01<<", "<<e5<<", "<<e1<<", "<<e2<<", "<<e4
              <<"\n  stereo correlation 0.25-2.5 s="<<correlation(responses[a],.25,2.5)<<"\n\n";
    }
    Render seq{std::vector<float>(static_cast<std::size_t>(kRate)*15),std::vector<float>(static_cast<std::size_t>(kRate)*15)};
    // A deterministic transient pattern, rendered once through each algorithm
    // with identical timing and settings for an easy listening comparison.
    std::vector<float> beatL(static_cast<std::size_t>(kRate)*4),beatR(beatL.size());
    for(int hit=0;hit<8;++hit) {
        const auto at=static_cast<std::size_t>(hit)*kRate/2;
        for(std::size_t j=0;j<1200&&at+j<beatL.size();++j) {
            const float env=std::exp(-static_cast<float>(j)/260.0f);
            const float low=std::sin(2.0f*3.14159265f*(52.0f+18.0f*std::exp(-float(j)/180.0f))*float(j)/kRate);
            const float noise=std::sin(float((j+at)*7919%65521)*.000191f);
            beatL[at+j]+=.48f*env*(.75f*low+.25f*noise);
            beatR[at+j]+=.48f*env*(.75f*low+.25f*noise);
        }
        const auto snare=at+static_cast<std::size_t>(kRate/4);
        for(std::size_t j=0;j<1800&&snare+j<beatL.size();++j){const float env=std::exp(-float(j)/420.0f);const float n=std::sin(float((j+snare)*1543%65521)*.000733f);beatL[snare+j]+=.24f*env*n;beatR[snare+j]+=.24f*env*std::sin(float((j+snare+73)*1543%65521)*.000733f);}
    }
    for(std::size_t a=0;a<3;++a){
        const std::size_t offset=static_cast<std::size_t>(a)*kRate*5;
        auto musical=render(kinds[a],2.5f,beatL,beatR);add(seq,musical,offset);
    }
    if(!writeWav(dir+"/Room-Hall-Plate-sequence-44100.wav",seq))return 4;
    const double roomHall=energy(responses[0],.1,4.0)/std::max(1e-30,energy(responses[1],.1,4.0));
    const double plateHall=energy(responses[2],.1,4.0)/std::max(1e-30,energy(responses[1],.1,4.0));
    report<<"Integrated-energy ratios (0.1-4 s): Room/Hall="<<roomHall<<", Plate/Hall="<<plateHall<<"\n"
          <<"All responses are finite by construction checks in the companion smoke test.\n";
    report.close();
    std::puts(("Wrote matched IRs, sequence demo, and measurements to "+dir).c_str());
    return 0;
}
