#include "fractional_delay.hpp"
#include <array>
#include <cassert>
#include <cmath>

int main() {
    trollfx::FractionalDelay d; assert(d.prepare(44100,0.1f));
    float l=0,r=0; d.process(1,0,1,l,r); assert(l==0);
    d.process(0,0,1,l,r); assert(std::abs(l-1)<1e-6f);
    trollfx::Chorus fx; assert(fx.prepare(44100)); trollfx::ModDelayParams p; p.feedback=.9f; p.mix=.5f; fx.setParams(p);
    std::array<float,512> il{},ir{},ol{},orr{}; il[0]=1;
    for(int b=0;b<20;++b) { fx.process(il.data(),ir.data(),ol.data(),orr.data(),il.size(),true,false); for(int i=0;i<512;++i) assert(std::isfinite(ol[i])&&std::isfinite(orr[i])); il.fill(0); }
}
