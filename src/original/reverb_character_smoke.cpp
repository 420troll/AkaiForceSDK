#include "dp4_reverb.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
constexpr int kRate=44100;
struct Result { std::vector<float> l,r; };
Result render(trollfx::ReverbKind kind,float decay,float damping=.35f,float width=1.0f,
              std::size_t frames=static_cast<std::size_t>(kRate)*5,bool hot=false) {
    trollfx::DP4InspiredReverb rv;assert(rv.prepare(static_cast<float>(kRate)));rv.setKind(kind);
    trollfx::ReverbParams p;p.decaySeconds=decay;p.preDelayMs=20;p.diffusion=.72f;p.damping=damping;
    p.earlyLevel1=.5f;p.earlyLevel2=.36f;p.earlyTime1Ms=18;p.earlyTime2Ms=43;p.width=width;p.mix=1;p.outputDb=0;
    rv.setParams(p);
    std::vector<float> il(frames),ir(frames);if(hot){for(std::size_t i=0;i<kRate/2;++i){const float v=(i&1)?1.5f:-1.5f;il[i]=v;ir[i]=-v;}}
    else {il[0]=ir[0]=.8f;}
    Result out{std::vector<float>(frames),std::vector<float>(frames)};
    for(std::size_t i=0;i<frames;i+=64)rv.process(il.data()+i,ir.data()+i,out.l.data()+i,out.r.data()+i,std::min<std::size_t>(64,frames-i));
    for(std::size_t i=0;i<frames;++i){assert(std::isfinite(out.l[i])&&std::isfinite(out.r[i]));assert(std::abs(out.l[i])<16&&std::abs(out.r[i])<16);}
    return out;
}
double energy(const Result& x,double a,double b) {
    double sum=0;for(std::size_t i=static_cast<std::size_t>(a*kRate);i<static_cast<std::size_t>(b*kRate);++i)sum+=double(x.l[i])*x.l[i]+double(x.r[i])*x.r[i];return sum;
}
double normalizedDifference(const Result&a,const Result&b) {
    double d=0,e=0;for(std::size_t i=0;i<a.l.size();++i){d+=std::pow(double(a.l[i]-b.l[i]),2)+std::pow(double(a.r[i]-b.r[i]),2);e+=std::pow(double(a.l[i]),2)+std::pow(double(a.r[i]),2);}return d/std::max(1e-30,e);
}
}
int main() {
    constexpr std::array<trollfx::ReverbKind,3> kinds{{trollfx::ReverbKind::Room,trollfx::ReverbKind::Hall,trollfx::ReverbKind::Plate}};
    std::array<Result,3> baseline;
    for(std::size_t i=0;i<kinds.size();++i){
        baseline[i]=render(kinds[i],2.5f);
        assert(energy(baseline[i],.1,.2)>1e-8);
        assert(energy(baseline[i],1,1.1)>1e-10);
        const auto shortDecay=render(kinds[i],.5f);
        const auto longDecay=render(kinds[i],8.0f);
        const double es=energy(shortDecay,1,1.1),el=energy(longDecay,1,1.1);
        assert(el>es*1.35);
        assert(normalizedDifference(baseline[i],shortDecay)>1e-4);
        const auto dark=render(kinds[i],2.5f,.9f);
        assert(normalizedDifference(baseline[i],dark)>1e-5);
        const auto narrow=render(kinds[i],2.5f,.35f,.1f);
        assert(normalizedDifference(baseline[i],narrow)>1e-5);
        (void)render(kinds[i],60.0f,.1f,1.0f,static_cast<std::size_t>(kRate)*2,true);
    }
    assert(normalizedDifference(baseline[0],baseline[1])>.05);
    assert(normalizedDifference(baseline[0],baseline[2])>.05);
    assert(normalizedDifference(baseline[1],baseline[2])>.05);
    for(float rate:{44100.0f,48000.0f}){trollfx::DP4InspiredReverb rv;assert(rv.prepare(rate));}
    std::puts("reverb_character_smoke: PASS (distinct IRs; decay, damping, width responsive; 60 s decay and hot input finite)");
}
