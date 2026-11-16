#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace trollfx {

class FractionalDelay {
public:
    bool prepare(float sampleRate, float maxDelaySeconds) {
        if (!(sampleRate > 0.0f) || !(maxDelaySeconds > 0.0f)) return false;
        const auto n = static_cast<std::size_t>(std::ceil(sampleRate * maxDelaySeconds)) + 4;
        if (n > (1u << 24)) return false;
        left_.assign(n, 0.0f); right_.assign(n, 0.0f); write_ = 0;
        maxDelay_ = static_cast<float>(n - 3); return true;
    }
    void reset() noexcept { std::fill(left_.begin(),left_.end(),0); std::fill(right_.begin(),right_.end(),0); write_=0; }
    float maxDelaySamples() const noexcept { return maxDelay_; }
    void process(float inL, float inR, float delaySamples, float& outL, float& outR) noexcept {
        if (left_.empty()) { outL=inL; outR=inR; return; }
        delaySamples=std::clamp(delaySamples,1.0f,maxDelay_);
        const float pos=static_cast<float>(write_)-delaySamples;
        const auto i0=wrap(static_cast<long long>(std::floor(pos)));
        const auto i1=(i0+1)%left_.size(); const float f=pos-std::floor(pos);
        outL=left_[i0]+(left_[i1]-left_[i0])*f;
        outR=right_[i0]+(right_[i1]-right_[i0])*f;
        left_[write_]=inL; right_[write_]=inR; write_=(write_+1)%left_.size();
    }
private:
    std::size_t wrap(long long i) const noexcept { const auto n=static_cast<long long>(left_.size()); i%=n; if(i<0)i+=n; return static_cast<std::size_t>(i); }
    std::vector<float> left_,right_; std::size_t write_=0; float maxDelay_=1;
};

struct ModDelayParams { float baseMs=18, depthMs=4, rateHz=0.35f, feedback=0.2f, mix=0.5f; };

class ModDelay {
public:
    bool prepare(float sampleRate, float maxDelaySeconds=1.0f) { sr_=sampleRate; phase_=0; fbL_=fbR_=0; return delay_.prepare(sampleRate,maxDelaySeconds); }
    void reset() noexcept { delay_.reset(); phase_=fbL_=fbR_=0; }
    void setParams(ModDelayParams p) noexcept { p.baseMs=std::clamp(p.baseMs,1.0f,900.0f); p.depthMs=std::clamp(p.depthMs,0.0f,100.0f); p.rateHz=std::clamp(p.rateHz,0.01f,8.0f); p.feedback=std::clamp(p.feedback,-0.92f,0.92f); p.mix=std::clamp(p.mix,0.0f,1.0f); p_=p; }
    void process(const float* inL,const float* inR,float* outL,float* outR,std::size_t n,bool crossFeedback=false,bool flanger=false) noexcept {
        const float inc=6.28318530718f*p_.rateHz/sr_;
        for(std::size_t i=0;i<n;++i) {
            const float lfo=std::sin(phase_); phase_+=inc; if(phase_>=6.28318530718f)phase_-=6.28318530718f;
            float dl,dr; float ms=p_.baseMs+p_.depthMs*lfo;
            delay_.process(inL[i]+fbL_,inR[i]+fbR_,std::max(1.0f,ms*0.001f*sr_),dl,dr);
            fbL_=crossFeedback?dr*p_.feedback:dl*p_.feedback; fbR_=crossFeedback?dl*p_.feedback:dr*p_.feedback;
            const float wetL=flanger?0.5f*(inL[i]+dl):dl, wetR=flanger?0.5f*(inR[i]+dr):dr;
            outL[i]=inL[i]*(1-p_.mix)+wetL*p_.mix; outR[i]=inR[i]*(1-p_.mix)+wetR*p_.mix;
        }
    }
private: FractionalDelay delay_; ModDelayParams p_{}; float sr_=44100,phase_=0,fbL_=0,fbR_=0;
};
using Chorus = ModDelay;
using Flanger = ModDelay;
}
