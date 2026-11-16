#include "audio_engine.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    trollfx::EffectsUnit e;
    if(!e.prepare(44100.0f,512)) return 1;
    constexpr std::size_t n=8192;
    std::vector<float> l(n),r(n),ol(n),orr(n); l[0]=r[0]=.7f;
    trollfx::EffectParams p; p.timeMs=10000; p.feedback=.9f; p.mix=.75f;
    e.setParams(p);
    for(unsigned a=0;a<10;++a) {
        e.setAlgorithm(static_cast<trollfx::EffectAlgorithm>(a)); e.reset();
        e.process(l.data(),r.data(),ol.data(),orr.data(),n);
        for(std::size_t i=0;i<n;++i) if(!std::isfinite(ol[i])||!std::isfinite(orr[i])) return 2;
    }
    p.timeMs=.1f;p.depthMs=30;p.feedback=.92f;e.setParams(p);
    e.setAlgorithm(trollfx::EffectAlgorithm::Flanger);
    for(int k=0;k<16;++k) e.process(l.data(),r.data(),ol.data(),orr.data(),n);
    for(float x:ol) if(!std::isfinite(x)) return 3;
    std::puts("V0.2 engine smoke PASS: 10 algorithms, bounds, finite feedback output");
    return 0;
}
