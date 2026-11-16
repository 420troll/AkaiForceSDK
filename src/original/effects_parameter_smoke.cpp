#include "audio_engine.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace trollfx;
namespace {
float difference(EffectAlgorithm algo, const EffectParams& a, const EffectParams& b,
                 const std::vector<float>& inL, const std::vector<float>& inR) {
    EffectsUnit x, y;
    if (!x.prepare(48000.0f, 256) || !y.prepare(48000.0f, 256)) return -1.0f;
    x.setAlgorithm(algo); y.setAlgorithm(algo); x.setParams(a); y.setParams(b);
    std::vector<float> xl(inL.size()), xr(inR.size()), yl(inL.size()), yr(inR.size());
    x.process(inL.data(), inR.data(), xl.data(), xr.data(), inL.size());
    y.process(inL.data(), inR.data(), yl.data(), yr.data(), inL.size());
    double sum=0.0;
    for(std::size_t i=0;i<inL.size();++i) sum+=std::abs(xl[i]-yl[i])+std::abs(xr[i]-yr[i]);
    return static_cast<float>(sum/(2.0*inL.size()));
}
}
int main() {
    constexpr std::size_t n=96000;
    std::vector<float> l(n),r(n);
    for(std::size_t i=0;i<n;++i) { const float s=0.35f*std::sin(2.0f*3.14159265f*440.0f*float(i)/48000.0f); l[i]=s; r[i]=.8f*s; }
    EffectParams a,b; a.mix=b.mix=1.0f;
    auto check=[&](EffectAlgorithm algo, const char* name) {
        const float d=difference(algo,a,b,l,r);
        if (!(d>1.0e-5f) || !std::isfinite(d)) { std::fprintf(stderr,"%s parameter had no audible effect: %g\n",name,d); return false; }
        return true;
    };
    bool ok=true;
    b=a; b.decaySeconds=8.0f; ok &= check(EffectAlgorithm::Hall,"reverb decay");
    b=a; b.preDelayMs=60.0f; ok &= check(EffectAlgorithm::Hall,"reverb predelay");
    b=a; b.diffusion=0.0f; ok &= check(EffectAlgorithm::Plate,"plate diffusion");
    b=a; b.damping=.95f; ok &= check(EffectAlgorithm::Room,"reverb damping");
    b=a; b.distortionGainInDb=30.0f; ok &= check(EffectAlgorithm::VCFDistortion,"VCF distortion drive");
    b=a; b.driveDb=36.0f; ok &= check(EffectAlgorithm::VCFDistortion,"separate clip drive");
    b=a; b.postVcfCutoffHz=800.0f; ok &= check(EffectAlgorithm::VCFDistortion,"post VCF cutoff");
    b=a; b.distortionBypass=true; ok &= check(EffectAlgorithm::VCFDistortion,"distortion bypass");
    b=a; b.compGainDb=12.0f; ok &= check(EffectAlgorithm::Compressor,"compressor makeup");
    b=a; b.gateOffDb=-24.0f; b.gateOnDb=-18.0f; ok &= check(EffectAlgorithm::Compressor,"compressor gate");
    b=a; b.bassGainDb=12.0f; ok &= check(EffectAlgorithm::EQPannerDDL,"bass EQ");
    b=a; b.leftDelayMs=35.0f; ok &= check(EffectAlgorithm::EQPannerDDL,"left delay");
    b=a; b.pannerPhaseOpposed=true; b.rateHz=2.0f; b.mix=1.0f; ok &= check(EffectAlgorithm::EQPannerDDL,"stereo phase panning");
    if(!ok) return 1;
    std::puts("effects_parameter_smoke: PASS (reverb, VCF distortion, compressor, EQ/panner/DDL controls affect rendered audio)");
}
