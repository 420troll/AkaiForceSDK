#include "audio_engine.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    trollfx::EffectsUnit unit;
    if(!unit.prepare(44100.0f,512)) return 1;
    constexpr std::size_t n=4096;
    std::vector<float> l(n),r(n),ol(n),orr(n);
    l[0]=r[0]=0.5f;
    trollfx::EffectParams p; p.mix=.7f; p.timeMs=10000; p.feedback=.8f;
    unit.setParams(p);
    const trollfx::EffectAlgorithm modes[]={trollfx::EffectAlgorithm::Delay,trollfx::EffectAlgorithm::Chorus,
        trollfx::EffectAlgorithm::Flanger,trollfx::EffectAlgorithm::Room,trollfx::EffectAlgorithm::Hall,
        trollfx::EffectAlgorithm::Plate,trollfx::EffectAlgorithm::ModulatedReverb};
    for(auto a:modes) {
        unit.setAlgorithm(a); unit.reset(); unit.process(l.data(),r.data(),ol.data(),orr.data(),n);
        for(std::size_t i=0;i<n;++i) if(!std::isfinite(ol[i])||!std::isfinite(orr[i])) return 2;
    }
    unit.setAlgorithm(trollfx::EffectAlgorithm::Flanger); p.timeMs=.1f;p.depthMs=15;p.feedback=.9f;unit.setParams(p);
    unit.reset(); for(int i=0;i<100;++i) unit.process(l.data(),r.data(),ol.data(),orr.data(),n);
    for(float x:ol) if(!std::isfinite(x)) return 3;
    std::puts("audio_engine_smoke: PASS (7 algorithms, 10 s delay bound, 0.1-15 ms flanger, finite output)");
    return 0;
}
