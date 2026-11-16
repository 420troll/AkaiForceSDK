#include "../../../src/original/audio_engine.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <vector>

int main(){
    constexpr std::size_t n=512;
    std::array<trollfx::EffectsUnit,4> units;
    for(auto& u:units)assert(u.prepare(44100.0f,static_cast<int>(n)));
    std::vector<float> inL(n),inR(n),aL(n),aR(n),bL(n),bR(n);
    inL[0]=0.7f;inR[0]=-0.4f;
    constexpr std::array<trollfx::EffectAlgorithm,4> chain={trollfx::EffectAlgorithm::Delay,trollfx::EffectAlgorithm::Chorus,trollfx::EffectAlgorithm::Hall,trollfx::EffectAlgorithm::Compressor};
    trollfx::EffectParams p;p.mix=0.4f;p.feedback=0.35f;p.timeMs=80;p.decaySeconds=1.4f;p.preDelayMs=5;p.thresholdDb=-30;p.ratio=5;
    for(std::size_t u=0;u<4;++u){units[u].setAlgorithm(chain[u]);units[u].setParams(p);}
    units[0].process(inL.data(),inR.data(),aL.data(),aR.data(),n);
    units[1].process(aL.data(),aR.data(),bL.data(),bR.data(),n);
    units[2].process(bL.data(),bR.data(),aL.data(),aR.data(),n);
    units[3].process(aL.data(),aR.data(),bL.data(),bR.data(),n);
    for(std::size_t i=0;i<n;++i)assert(std::isfinite(bL[i])&&std::isfinite(bR[i]));
    for(int mode=0;mode<18;++mode){
        trollfx::EffectsUnit unit;assert(unit.prepare(48000.0f,static_cast<int>(n)));
        unit.setAlgorithm(static_cast<trollfx::EffectAlgorithm>(mode));unit.setParams(p);
        unit.process(inL.data(),inR.data(),aL.data(),aR.data(),n);
        for(std::size_t i=0;i<n;++i)assert(std::isfinite(aL[i])&&std::isfinite(aR[i]));
    }
    // Every exposed algorithm must respond to a non-zero setting, including
    // algorithms added from the documented DP/4 function families.
    for(int mode=10;mode<18;++mode){
        trollfx::EffectsUnit unit;assert(unit.prepare(44100.0f,static_cast<int>(n)));
        auto q=p;q.mix=1.0f;q.feedback=.55f;q.timeMs=5.0f;q.depthMs=7.0f;q.cutoffHz=3600.0f;
        unit.setAlgorithm(static_cast<trollfx::EffectAlgorithm>(mode));unit.setParams(q);
        unit.process(inL.data(),inR.data(),aL.data(),aR.data(),n);
        float energy=0;for(std::size_t i=0;i<n;++i){assert(std::isfinite(aL[i])&&std::isfinite(aR[i]));energy+=std::abs(aL[i])+std::abs(aR[i]);}
        assert(energy>0.01f);
    }
}
