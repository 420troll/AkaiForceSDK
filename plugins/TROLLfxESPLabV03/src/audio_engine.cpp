#include "../../../src/original/audio_engine.hpp"
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
    const std::size_t diffN[4]={static_cast<std::size_t>(sr*.0031f)+1,static_cast<std::size_t>(sr*.0047f)+1,
        static_cast<std::size_t>(sr*.0067f)+1,static_cast<std::size_t>(sr*.0091f)+1};
    const std::size_t total=2*longN+tankN[0]+tankN[1]+tankN[2]+tankN[3]+diffN[0]+diffN[1]+diffN[2]+diffN[3];
    try { storage_.assign(total,0.0f); } catch (...) { storage_.clear(); return false; }
    std::size_t off=0;
    delayL_={storage_.data()+off,longN,0}; off+=longN;
    delayR_={storage_.data()+off,longN,0}; off+=longN;
    for(int i=0;i<4;++i) { tank_[i]={storage_.data()+off,tankN[i],0}; off+=tankN[i]; }
    for(int i=0;i<4;++i) { diffuser_[i]={storage_.data()+off,diffN[i],0}; off+=diffN[i]; }
    reset(); return true;
}
void EffectsUnit::reset() noexcept {
    delayL_.clear(); delayR_.clear(); for(auto& d:tank_) d.clear(); for(auto& d:diffuser_) d.clear();
    std::fill(damp_,damp_+4,0.0f); phase_=0.0f;
    std::fill(filter_,filter_+2,0.0f); std::fill(envelope_,envelope_+2,0.0f);
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
    case EffectAlgorithm::VCFDistortion: processVCFDistortion(l,r,ol,or_,n); break;
    case EffectAlgorithm::EQPannerDDL: processPannerDDL(l,r,ol,or_,n); break;
    case EffectAlgorithm::Compressor: processCompressor(l,r,ol,or_,n); break;
    default: processDelay(l,r,ol,or_,n,false,false); break;
    }
}

void EffectsUnit::processVCFDistortion(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    const float a=1.0f-std::exp(-2.0f*pi*std::clamp(params_.cutoffHz,80.0f,10000.0f)/sampleRate_);
    const float gain=1.0f+std::clamp(params_.depthMs,0.0f,30.0f)*.28f;
    for(std::size_t i=0;i<n;++i) {
        float wet[2]; const float x[2]={l[i],r[i]};
        for(int c=0;c<2;++c) {
            filter_[c]+=a*(x[c]-filter_[c]);
            const float hp=x[c]-filter_[c];
            const float env=std::fabs(hp); envelope_[c]+=(env>envelope_[c]?.03f:.001f)*(env-envelope_[c]);
            const float cutoffMod=1.0f+std::clamp(envelope_[c]*8.0f,0.0f,3.0f);
            const float resonant=hp+filter_[c]*(.4f+params_.diffusion*.45f)*cutoffMod;
            wet[c]=std::tanh(resonant*gain);
        }
        ol[i]=l[i]+(wet[0]-l[i])*params_.mix; or_[i]=r[i]+(wet[1]-r[i])*params_.mix;
    }
}

void EffectsUnit::processPannerDDL(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    const float inc=2.0f*pi*params_.rateHz/sampleRate_, fb=clampFeedback(params_.feedback);
    const float d=std::clamp(params_.timeMs*.001f*sampleRate_,1.0f,static_cast<float>(delayL_.size-1));
    for(std::size_t i=0;i<n;++i) {
        const float p=.5f+.5f*std::sin(phase_)*params_.width; phase_+=inc; if(phase_>=2*pi)phase_-=2*pi;
        const float dl=delayL_.read(d),dr=delayR_.read(d);
        delayL_.write(l[i]+dr*fb); delayR_.write(r[i]+dl*fb);
        const float angle=p*pi*.5f;
        const float wetL=std::cos(angle)*(dl+dr),wetR=std::sin(angle)*(dl+dr);
        ol[i]=l[i]+(wetL-l[i])*params_.mix; or_[i]=r[i]+(wetR-r[i])*params_.mix;
    }
}

void EffectsUnit::processCompressor(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    const float atk=std::exp(-1.0f/(std::max(.05f,params_.attackMs)*.001f*sampleRate_));
    const float rel=std::exp(-1.0f/(std::max(1.0f,params_.releaseMs)*.001f*sampleRate_));
    const float threshold=std::pow(10.0f,params_.thresholdDb/20.0f), ratio=std::clamp(params_.ratio,1.0f,40.0f);
    for(std::size_t i=0;i<n;++i) {
        const float x[2]={l[i],r[i]}; float y[2];
        for(int c=0;c<2;++c) {
            const float level=std::fabs(x[c]); const float coeff=level>envelope_[c]?atk:rel;
            envelope_[c]=coeff*envelope_[c]+(1.0f-coeff)*level;
            float gain=1.0f;
            if(envelope_[c]>threshold && envelope_[c]>1.0e-9f) {
                const float compressed=threshold*std::pow(envelope_[c]/threshold,1.0f/ratio);
                gain=compressed/envelope_[c];
            }
            y[c]=x[c]*gain;
        }
        ol[i]=l[i]+(y[0]-l[i])*params_.mix; or_[i]=r[i]+(y[1]-r[i])*params_.mix;
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
    float decay=0.72f, modDepth=0.0f;
    switch(algorithm_) {
    case EffectAlgorithm::Room: decay=.68f; break;
    case EffectAlgorithm::Hall: decay=.87f; break;
    case EffectAlgorithm::Plate: decay=.84f; break;
    default: decay=.82f; modDepth=static_cast<float>(sampleRate_*.0007f); break;
    }
    decay=std::min(decay,0.35f+params_.feedback*.62f+params_.diffusion*.12f);
    decay=std::clamp(decay,.15f,.94f);
    const float damp=std::clamp(params_.damping,0.0f,.98f);
    const float inc=2*pi*params_.rateHz/sampleRate_;
    for(std::size_t s=0;s<n;++s) {
        const float dry=.5f*(l[s]+r[s]);
        // Room/hall use two short early-reflection taps; plate variants run a
        // two-stage allpass diffuser before the decay field. Both are original
        // structures inferred from the public DP/4 signal-flow descriptions.
        float x=dry;
        if(algorithm_==EffectAlgorithm::Room || algorithm_==EffectAlgorithm::Hall) {
            const float earlyA=delayL_.read(std::clamp(sampleRate_*.017f,1.0f,static_cast<float>(delayL_.size-1)));
            const float earlyB=delayR_.read(std::clamp(sampleRate_*.031f,1.0f,static_cast<float>(delayR_.size-1)));
            delayL_.write(l[s]); delayR_.write(r[s]);
            x += .18f*earlyA+.11f*earlyB;
        } else {
            const int stages=algorithm_==EffectAlgorithm::Plate ? 2 : 1;
            for(int k=0;k<stages;++k) {
                const int ix=2*k;
                const float g=.25f+.55f*params_.diffusion;
                const float delayed=diffuser_[ix].read(static_cast<float>(diffuser_[ix].size)-1.0f);
                const float y=delayed-g*x;
                diffuser_[ix].write(x+g*y); x=y;
                const float delayedR=diffuser_[ix+1].read(static_cast<float>(diffuser_[ix+1].size)-1.0f);
                const float yr=delayedR-g*x;
                diffuser_[ix+1].write(x+g*yr); x=.5f*(y+yr);
            }
        }
        const float phase=std::sin(phase_); phase_+=inc; if(phase_>=2*pi)phase_-=2*pi;
        float d[4],mean=0;
        for(int k=0;k<4;++k) { d[k]=tank_[k].read(static_cast<float>(tank_[k].size)-1.0f+phase*modDepth); mean+=d[k]; }
        mean*=.25f;
        const float diffuser=params_.diffusion;
        const float input=x;
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
