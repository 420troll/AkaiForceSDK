#include "dp4_reverb.hpp"
#include "modulation.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
float logMap(float x,float lo,float hi){return lo*std::pow(hi/lo,std::clamp(x,0.0f,1.0f));}
double render(bool modulate) {
    constexpr float sr=44100.0f;
    constexpr std::size_t total=sr*4;
    trollfx::DP4InspiredReverb reverb;
    assert(reverb.prepare(sr));
    reverb.setKind(trollfx::ReverbKind::Hall);
    trollfx::ReverbParams rp; rp.decaySeconds=2.0f; rp.preDelayMs=0; rp.mix=1;
    rp.earlyLevel1=rp.earlyLevel2=0; rp.damping=.2f; rp.diffusion=.8f;
    reverb.setParams(rp);
    trollfx::ModulationMatrix matrix;
    matrix.beginBlock(sr,.5f);
    trollfx::ModulatorSlot slot; slot.enabled=modulate; slot.source=trollfx::ModSource::InternalLfo;
    slot.destination=trollfx::ModDestination::Decay; slot.minNorm=.0f; slot.maxNorm=1.0f;
    matrix.setSlot(0,slot);
    trollfx::ModulationMatrix::Values base{},out{};
    base[static_cast<unsigned>(trollfx::ModDestination::Decay)]=.5f;
    std::vector<float> inL(8),inR(8),outL(8),outR(8);
    double energy=0;
    float minDecay=1000,maxDecay=0;
    for(std::size_t offset=0;offset<total;offset+=8) {
        std::fill(inL.begin(),inL.end(),0); std::fill(inR.begin(),inR.end(),0);
        if(offset==0) inL[0]=inR[0]=.5f;
        for(int j=0;j<8;++j) matrix.process(inL[j],inR[j],base,out);
        const float decay=modulate?logMap(out[static_cast<unsigned>(trollfx::ModDestination::Decay)],.2f,60.0f):2.0f;
        minDecay=std::min(minDecay,decay); maxDecay=std::max(maxDecay,decay);
        rp.decaySeconds=decay; reverb.setParams(rp);
        reverb.process(inL.data(),inR.data(),outL.data(),outR.data(),8);
        if(offset>=sr*.5) for(int j=0;j<8;++j) energy+=double(outL[j])*outL[j]+double(outR[j])*outR[j];
    }
    if(modulate) assert(maxDecay-minDecay>1.0f);
    return energy;
}
}
int main(){
    const double fixed=render(false),modulated=render(true);
    assert(std::isfinite(fixed)&&std::isfinite(modulated));
    std::fprintf(stderr,"fixed %.12g modulated %.12g delta %.12g\n",fixed,modulated,std::fabs(fixed-modulated));
    assert(std::fabs(fixed-modulated)>1e-12);
}
