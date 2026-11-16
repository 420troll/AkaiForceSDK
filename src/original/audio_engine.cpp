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
    if (!reverb_.prepare(sr)) return false;
    reset(); return true;
}
void EffectsUnit::reset() noexcept {
    delayL_.clear(); delayR_.clear(); for(auto& d:tank_) d.clear();
    std::fill(damp_,damp_+4,0.0f); std::fill(filter_,filter_+16,0.0f);
    std::fill(envelope_,envelope_+2,0.0f); std::fill(eqBass_,eqBass_+2,0.0f);
    std::fill(eqTreble_,eqTreble_+2,0.0f); gateGain_=1.0f; phase_=heldPhase_=holdPhase_=0.0f;
    std::fill(preEqHpInput_,preEqHpInput_+2,0.0f); std::fill(preEqHpOutput_,preEqHpOutput_+2,0.0f);
    holdCounter_=0; reverb_.reset();
}
void EffectsUnit::setAlgorithm(EffectAlgorithm a) noexcept { algorithm_=a; setParams(params_); }
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
    params_.distortionGainInDb=std::clamp(p.distortionGainInDb,-24.0f,48.0f);
    params_.distortionGainOutDb=std::clamp(p.distortionGainOutDb,-48.0f,24.0f);
    params_.driveDb=std::clamp(p.driveDb,0.0f,48.0f);
    params_.preEqHighPassHz=std::clamp(p.preEqHighPassHz,0.0f,1000.0f);
    params_.preVcfCutoffHz=std::clamp(p.preVcfCutoffHz,20.0f,20000.0f); params_.preVcfQ=std::clamp(p.preVcfQ,0.5f,25.0f); params_.preVcfEnvDepth=std::clamp(p.preVcfEnvDepth,-1.0f,1.0f);
    params_.postVcfCutoffHz=std::clamp(p.postVcfCutoffHz,20.0f,20000.0f); params_.postVcfQ=std::clamp(p.postVcfQ,0.5f,25.0f); params_.postVcfEnvDepth=std::clamp(p.postVcfEnvDepth,-1.0f,1.0f);
    params_.envelopeAttackMs=std::clamp(p.envelopeAttackMs,0.05f,10000.0f); params_.envelopeReleaseMs=std::clamp(p.envelopeReleaseMs,1.0f,10000.0f);
    params_.compGainDb=std::clamp(p.compGainDb,-48.0f,48.0f); params_.gateOffDb=std::clamp(p.gateOffDb,-96.0f,0.0f); params_.gateOnDb=std::clamp(p.gateOnDb,-96.0f,0.0f); if(params_.gateOnDb<params_.gateOffDb) params_.gateOnDb=params_.gateOffDb;
    params_.gateReleaseMs=std::clamp(p.gateReleaseMs,1.0f,10000.0f); params_.sampleHoldHz=std::clamp(p.sampleHoldHz,0.0f,100.0f);
    params_.leftDelayMs=std::clamp(p.leftDelayMs,0.0f,1500.0f); params_.rightDelayMs=std::clamp(p.rightDelayMs,0.0f,1500.0f); params_.leftEchoMs=std::clamp(p.leftEchoMs,0.0f,1500.0f); params_.rightEchoMs=std::clamp(p.rightEchoMs,0.0f,1500.0f);
    params_.echoLevel=std::clamp(p.echoLevel,0.0f,1.0f); params_.bassFcHz=std::clamp(p.bassFcHz,20.0f,1000.0f); params_.bassGainDb=std::clamp(p.bassGainDb,-48.0f,24.0f);
    params_.trebleFcHz=std::clamp(p.trebleFcHz,1000.0f,16000.0f); params_.trebleGainDb=std::clamp(p.trebleGainDb,-48.0f,24.0f); params_.inputTrimDb=std::clamp(p.inputTrimDb,-24.0f,0.0f);
    params_.decaySeconds=std::clamp(p.decaySeconds,0.2f,60.0f); params_.preDelayMs=std::clamp(p.preDelayMs,0.0f,500.0f);
    params_.earlyLevel1=std::clamp(p.earlyLevel1,0.0f,1.0f); params_.earlyLevel2=std::clamp(p.earlyLevel2,0.0f,1.0f);
    params_.earlyTime1Ms=std::clamp(p.earlyTime1Ms,0.0f,120.0f); params_.earlyTime2Ms=std::clamp(p.earlyTime2Ms,0.0f,120.0f); params_.outputDb=std::clamp(p.outputDb,-24.0f,12.0f);
    ReverbParams rp; rp.decaySeconds=params_.decaySeconds; rp.preDelayMs=params_.preDelayMs;
    rp.diffusion=params_.diffusion; rp.damping=params_.damping; rp.width=params_.width; rp.mix=params_.mix;
    rp.earlyLevel1=params_.earlyLevel1; rp.earlyLevel2=params_.earlyLevel2;
    rp.earlyTime1Ms=params_.earlyTime1Ms; rp.earlyTime2Ms=params_.earlyTime2Ms;
    rp.outputDb=params_.outputDb; rp.modRateHz=params_.rateHz; rp.modDepthMs=std::min(params_.depthMs,2.0f);
    reverb_.setParams(rp);
    switch(algorithm_) { case EffectAlgorithm::Room: reverb_.setKind(ReverbKind::Room); break; case EffectAlgorithm::Hall: reverb_.setKind(ReverbKind::Hall); break; case EffectAlgorithm::Plate: reverb_.setKind(ReverbKind::Plate); break; default: reverb_.setKind(ReverbKind::Modulated); break; }
}
float EffectsUnit::clampFeedback(float x) const noexcept { return std::clamp(x,-0.92f,0.92f); }

void EffectsUnit::process(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    if(!l||!r||!ol||!or_) return;
    switch(algorithm_) {
    case EffectAlgorithm::Room: case EffectAlgorithm::Hall: case EffectAlgorithm::Plate: case EffectAlgorithm::ModulatedReverb:
        reverb_.process(l,r,ol,or_,n); break;
    case EffectAlgorithm::Chorus: case EffectAlgorithm::Flanger:
        processDelay(l,r,ol,or_,n,true,algorithm_==EffectAlgorithm::Flanger); break;
    case EffectAlgorithm::VCFDistortion: processVCFDistortion(l,r,ol,or_,n); break;
    case EffectAlgorithm::EQPannerDDL: processPannerDDL(l,r,ol,or_,n); break;
    case EffectAlgorithm::Compressor: processCompressor(l,r,ol,or_,n); break;
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

void EffectsUnit::processVCFDistortion(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    struct Coeff { float b0,b1,b2,a1,a2; };
    auto coeff=[this](float cutoff,float q) noexcept {
        const float w=2.0f*pi*std::clamp(cutoff,20.0f,sampleRate_*.45f)/sampleRate_;
        const float c=std::cos(w), alpha=std::sin(w)/(2.0f*std::clamp(q,.5f,25.0f));
        const float a0=1.0f+alpha;
        return Coeff{(1.0f-c)*.5f/a0,(1.0f-c)/a0,(1.0f-c)*.5f/a0,-2.0f*c/a0,(1.0f-alpha)/a0};
    };
    const float att=std::exp(-1.0f/(.001f*params_.envelopeAttackMs*sampleRate_));
    const float rel=std::exp(-1.0f/(.001f*params_.envelopeReleaseMs*sampleRate_));
    const float inGain=std::pow(10.0f,params_.distortionGainInDb/20.0f), drive=std::pow(10.0f,params_.driveDb/20.0f), outGain=std::pow(10.0f,params_.distortionGainOutDb/20.0f);
    const float hpCoeff=params_.preEqHighPassHz>0.0f?std::exp(-2.0f*pi*std::min(params_.preEqHighPassHz,sampleRate_*.45f)/sampleRate_):0.0f;
    for(std::size_t i=0;i<n;++i) {
        float in[2]={l[i]*inGain,r[i]*inGain}, out[2]{};
        for(int ch=0;ch<2;++ch) {
            if(params_.preEqHighPassHz>0.0f) {
                const float hp=hpCoeff*(preEqHpOutput_[ch]+in[ch]-preEqHpInput_[ch]);
                preEqHpInput_[ch]=in[ch]; preEqHpOutput_[ch]=hp; in[ch]=hp;
            } else {
                preEqHpInput_[ch]=in[ch]; preEqHpOutput_[ch]=0.0f;
            }
            const float mag=std::abs(in[ch]); envelope_[ch]=mag>envelope_[ch]?att*envelope_[ch]+(1.0f-att)*mag:rel*envelope_[ch]+(1.0f-rel)*mag;
            float x=in[ch];
            for(int stage=0;stage<2;++stage) {
                const float base=stage==0?params_.preVcfCutoffHz:params_.postVcfCutoffHz;
                const float depth=stage==0?params_.preVcfEnvDepth:params_.postVcfEnvDepth;
                const float q=stage==0?params_.preVcfQ:params_.postVcfQ;
                const float fc=std::clamp(base*std::exp2(depth*envelope_[ch]*4.0f),20.0f,sampleRate_*.45f);
                const Coeff c=coeff(fc,q);
                const int s=(stage*2+ch)*4;
                float y=c.b0*x+c.b1*filter_[s]+c.b2*filter_[s+1]-c.a1*filter_[s+2]-c.a2*filter_[s+3];
                filter_[s+1]=filter_[s]; filter_[s]=x; filter_[s+3]=filter_[s+2]; filter_[s+2]=y;
                x=y;
            }
            out[ch]=(params_.distortionBypass?x:std::tanh(x*drive))*outGain;
        }
        ol[i]=out[0]; or_[i]=out[1];
    }
}

void EffectsUnit::processCompressor(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    const float atk=std::exp(-1.0f/(.001f*std::max(.05f,params_.attackMs)*sampleRate_));
    const float rel=std::exp(-1.0f/(.001f*params_.releaseMs*sampleRate_));
    const float gateRel=std::exp(-1.0f/(.001f*params_.gateReleaseMs*sampleRate_));
    const float makeup=std::pow(10.0f,params_.compGainDb/20.0f);
    for(std::size_t i=0;i<n;++i) {
        const float peak=std::max(std::abs(l[i]),std::abs(r[i]));
        const float c=peak>envelope_[0]?atk:rel; envelope_[0]=c*envelope_[0]+(1.0f-c)*peak;
        const float db=20.0f*std::log10(std::max(envelope_[0],1.0e-6f));
        const float threshold=std::pow(10.0f,params_.thresholdDb/20.0f);
        float gain=1.0f;
        if(envelope_[0]>threshold && params_.ratio>1.0f) gain=std::pow(threshold/envelope_[0],1.0f-1.0f/params_.ratio);
        const bool gateOpen=gateGain_>.5f;
        const bool desiredOpen=gateOpen?db>params_.gateOffDb:db>=params_.gateOnDb;
        const float target=desiredOpen?1.0f:0.0f;
        const float gc=target>gateGain_?atk:gateRel; gateGain_=gc*gateGain_+(1.0f-gc)*target;
        const float g=gain*gateGain_*makeup;
        ol[i]=l[i]*g; or_[i]=r[i]*g;
    }
}

void EffectsUnit::processPannerDDL(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
    const float trim=std::pow(10.0f,params_.inputTrimDb/20.0f);
    const float bGain=std::pow(10.0f,params_.bassGainDb/20.0f)-1.0f;
    const float tGain=std::pow(10.0f,params_.trebleGainDb/20.0f)-1.0f;
    const float bA=1.0f-std::exp(-2.0f*pi*params_.bassFcHz/sampleRate_);
    const float tA=1.0f-std::exp(-2.0f*pi*params_.trebleFcHz/sampleRate_);
    const float phaseInc=2.0f*pi*params_.rateHz/sampleRate_;
    const std::size_t holdPeriod=params_.sampleHoldHz>0.0f?static_cast<std::size_t>(std::max(1.0f,sampleRate_/params_.sampleHoldHz)):0;
    const float dL=std::clamp(params_.leftDelayMs*.001f*sampleRate_,1.0f,float(delayL_.size-1));
    const float dR=std::clamp(params_.rightDelayMs*.001f*sampleRate_,1.0f,float(delayR_.size-1));
    const float eL=std::clamp(params_.leftEchoMs*.001f*sampleRate_,1.0f,float(delayL_.size-1));
    const float eR=std::clamp(params_.rightEchoMs*.001f*sampleRate_,1.0f,float(delayR_.size-1));
    for(std::size_t i=0;i<n;++i) {
        if(holdPeriod && holdCounter_++>=holdPeriod) { holdCounter_=0; heldPhase_=std::sin(phase_); }
        const float wave=holdPeriod?heldPhase_:std::sin(phase_);
        phase_+=phaseInc; if(phase_>=2*pi) phase_-=2*pi;
        float x[2]={l[i]*trim,r[i]*trim}, wet[2]{};
        for(int ch=0;ch<2;++ch) {
            eqBass_[ch]+=bA*(x[ch]-eqBass_[ch]);
            eqTreble_[ch]+=tA*(x[ch]-eqTreble_[ch]);
            const float eq=x[ch]+bGain*eqBass_[ch]+tGain*(x[ch]-eqTreble_[ch]);
            const float panWave=(ch==1&&params_.pannerPhaseOpposed)?-wave:wave;
            const float pan=std::clamp(.5f+.5f*panWave*params_.width,0.0f,1.0f);
            const float a=ch==0?dL:dR, e=ch==0?eL:eR;
            const float delayed=(ch==0?delayL_:delayR_).read(a);
            const float echo=(ch==0?delayL_:delayR_).read(e);
            (ch==0?delayL_:delayR_).write(eq+echo*params_.echoLevel*params_.feedback);
            wet[ch]=delayed+echo*params_.echoLevel;
            if(ch==0) wet[ch]*=1.0f-pan; else wet[ch]*=pan;
        }
        const float dryL=x[0],dryR=x[1];
        ol[i]=dryL+(wet[0]-dryL)*params_.mix; or_[i]=dryR+(wet[1]-dryR)*params_.mix;
    }
}
} // namespace trollfx
