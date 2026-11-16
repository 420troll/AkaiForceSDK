#include "audio_engine.hpp"
#include <algorithm>
#include <cmath>

namespace trollfx {
namespace { constexpr float pi = 3.14159265358979323846f; }

void EffectsUnit::DelayLine::clear() noexcept { if (data) std::fill(data,data+size,0.0f); pos=0; }
float EffectsUnit::DelayLine::read(float d) const noexcept {
    if (!data || size < 2) return 0.0f;
    d=std::clamp(d,1.0f,static_cast<float>(size-1));
    float p=static_cast<float>(pos)-d;
    while(p<0.0f) p+=static_cast<float>(size);
    const auto i=static_cast<std::size_t>(p);
    const auto j=(i+1)%size;
    const float f=p-static_cast<float>(i);
    return data[i]+(data[j]-data[i])*f;
}
void EffectsUnit::DelayLine::write(float x) noexcept { data[pos]=x; if(++pos==size) pos=0; }

bool EffectsUnit::prepare(float sr,int maxBlock) {
    if (!(sr>=8000.0f) || !(sr<=192000.0f) || maxBlock<1) return false;
    sampleRate_=sr;
    // Each stereo delay line supports a full 10 seconds. Four compact FDN lines
    // share the remaining storage; all memory is allocated before audio starts.
    const std::size_t longN=static_cast<std::size_t>(sr*10.0f)+4;
    const std::size_t tankN[4]={static_cast<std::size_t>(sr*.0311f)+1,static_cast<std::size_t>(sr*.0377f)+1,
        static_cast<std::size_t>(sr*.0419f)+1,static_cast<std::size_t>(sr*.0487f)+1};
    const std::size_t total=2*longN+tankN[0]+tankN[1]+tankN[2]+tankN[3];
    try { storage_.assign(total,0.0f); } catch (...) { storage_.clear(); return false; }
    std::size_t off=0;
    delayL_={storage_.data()+off,longN,0}; off+=longN;
    delayR_={storage_.data()+off,longN,0}; off+=longN;
    for(int i=0;i<4;++i) { tank_[i]={storage_.data()+off,tankN[i],0}; off+=tankN[i]; }
    reset(); return true;
}
void EffectsUnit::reset() noexcept {
    delayL_.clear(); delayR_.clear(); for(auto& d:tank_) d.clear();
    std::fill(damp_,damp_+4,0.0f); phase_=0.0f;
}
void EffectsUnit::setAlgorithm(EffectAlgorithm a) noexcept { algorithm_=a; }
void EffectsUnit::setParams(const EffectParams& p) noexcept {
    params_=p;
    params_.timeMs=std::clamp(p.timeMs,0.1f,10000.0f);
    params_.feedback=std::clamp(p.feedback,-0.92f,0.92f);
    params_.mix=std::clamp(p.mix,0.0f,1.0f);
    params_.rateHz=std::clamp(p.rateHz,0.01f,8.0f);
    params_.depthMs=std::clamp(p.depthMs,0.0f,30.0f);
    params_.diffusion=std::clamp(p.diffusion,0.0f,1.0f);
    params_.damping=std::clamp(p.damping,0.0f,0.98f);
    params_.width=std::clamp(p.width,0.0f,1.0f);
}
float EffectsUnit::clampFeedback(float x) const noexcept { return std::clamp(x,-0.92f,0.92f); }

void EffectsUnit::process(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    if(!l||!r||!ol||!or_) return;
    switch(algorithm_) {
    case EffectAlgorithm::Room: case EffectAlgorithm::Hall: case EffectAlgorithm::Plate: case EffectAlgorithm::ModulatedReverb:
        processReverb(l,r,ol,or_,n); break;
    case EffectAlgorithm::Chorus: case EffectAlgorithm::Flanger:
        processDelay(l,r,ol,or_,n,true,algorithm_==EffectAlgorithm::Flanger); break;
    default: processDelay(l,r,ol,or_,n,false,false); break;
    }
}

void EffectsUnit::processDelay(const float* l,const float* r,float* ol,float* or_,std::size_t n,bool mod,bool flange) noexcept {
    const float inc=2.0f*pi*params_.rateHz/sampleRate_;
    for(std::size_t i=0;i<n;++i) {
        const float dryL=l[i],dryR=r[i];
        const float wave=std::sin(phase_); phase_+=inc; if(phase_>=2*pi) phase_-=2*pi;
        float ms=params_.timeMs;
        if(flange) ms=std::clamp(7.55f+7.45f*wave*params_.depthMs/30.0f,0.1f,15.0f);
        else if(mod) ms=std::max(1.0f,ms+params_.depthMs*wave);
        const float delay=ms*.001f*sampleRate_;
        const float dl=delayL_.read(delay), dr=delayR_.read(delay);
        const float fb=clampFeedback(params_.feedback);
        delayL_.write(dryL+dr*fb); delayR_.write(dryR+dl*fb);
        const float wetL=flange?0.5f*(dryL+dl):dl;
        const float wetR=flange?0.5f*(dryR+dr):dr;
        ol[i]=dryL+(wetL-dryL)*params_.mix; or_[i]=dryR+(wetR-dryR)*params_.mix;
    }
}

void EffectsUnit::processReverb(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    float decay=0.72f, pre=0.0f, modDepth=0.0f;
    switch(algorithm_) {
    case EffectAlgorithm::Room: decay=.68f; break;
    case EffectAlgorithm::Hall: decay=.87f; pre=static_cast<float>(sampleRate_*.012f); break;
    case EffectAlgorithm::Plate: decay=.84f; pre=static_cast<float>(sampleRate_*.004f); break;
    default: decay=.82f; pre=static_cast<float>(sampleRate_*.008f); modDepth=static_cast<float>(sampleRate_*.0007f); break;
    }
    decay=std::min(decay,0.35f+params_.feedback*.62f+params_.diffusion*.12f);
    decay=std::clamp(decay,.15f,.94f);
    const float damp=std::clamp(params_.damping,0.0f,.98f);
    const float inc=2*pi*params_.rateHz/sampleRate_;
    for(std::size_t s=0;s<n;++s) {
        const float x=.5f*(l[s]+r[s]);
        const float phase=std::sin(phase_); phase_+=inc; if(phase_>=2*pi)phase_-=2*pi;
        float d[4],mean=0;
        for(int k=0;k<4;++k) { d[k]=tank_[k].read(static_cast<float>(tank_[k].size)-1.0f+phase*modDepth); mean+=d[k]; }
        mean*=.25f;
        const float diffuser=params_.diffusion;
        const float input=x + pre*0.0f;
        float wetL=0,wetR=0;
        for(int k=0;k<4;++k) {
            const float v=damp_[k]+(d[k]-damp_[k])*(1.0f-damp);
            damp_[k]=v;
            const float fb=(2.0f*mean-d[k])*decay;
            const float sign=(k&1)?-1.0f:1.0f;
            const float inj=input*sign*(.22f+.18f*diffuser);
            tank_[k].write(inj+fb);
            if(k==0||k==2) wetL+=d[k]; else wetR+=d[k];
        }
        wetL*=.5f; wetR*=.5f;
        const float mid=.5f*(wetL+wetR),side=.5f*(wetL-wetR)*params_.width;
        wetL=mid+side; wetR=mid-side;
        ol[s]=l[s]+(wetL-l[s])*params_.mix;
        or_[s]=r[s]+(wetR-r[s])*params_.mix;
    }
}
} // namespace trollfx
