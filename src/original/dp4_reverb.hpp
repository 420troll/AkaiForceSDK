#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace trollfx {

enum class ReverbKind { Room, Hall, Plate, Modulated };

struct ReverbParams {
    float decaySeconds = 2.0f;
    float preDelayMs = 0.0f;
    float diffusion = 0.65f;
    float damping = 0.35f;
    float earlyLevel1 = 0.25f;
    float earlyLevel2 = 0.15f;
    float earlyTime1Ms = 17.0f;
    float earlyTime2Ms = 31.0f;
    float width = 1.0f;
    float mix = 1.0f;
    float outputDb = 0.0f;
    float modRateHz = 0.25f;
    float modDepthMs = 0.7f;
};

// Original manual-informed DSP. Room uses a sparse early-reflection field and
// four-line decay network; Hall uses a denser eight-line network; Plate uses
// serial input diffusers and a cross-coupled four-line network. These are not
// recovered Ensoniq code. The manual supports these broad topology families,
// but does not disclose factory delay lengths or coefficients.
class DP4InspiredReverb {
public:
    bool prepare(float sampleRate) {
        if (!(sampleRate >= 8000.0f && sampleRate <= 192000.0f)) return false;
        sr_ = sampleRate;
        constexpr float pi=3.14159265358979323846f;
        for(std::size_t i=0;i<lfoTable_.size();++i)
            lfoTable_[i]=std::sin(2.0f*pi*static_cast<float>(i)/static_cast<float>(lfoTable_.size()));
        const std::size_t predelaySize = static_cast<std::size_t>(sr_ * 0.51f) + 4;
        // Pairwise incommensurate delay lengths reduce short periodic loops.
        constexpr std::array<float, 8> seconds{{.0293f,.0311f,.0347f,.0377f,
                                                .0419f,.0433f,.0479f,.0531f}};
        constexpr std::array<float, 4> diffuserSeconds{{.0031f,.0047f,.0067f,.0091f}};
        try {
            predelay_[0].assign(predelaySize, 0.0f);
            predelay_[1].assign(predelaySize, 0.0f);
            for (std::size_t i=0; i<tank_.size(); ++i)
                tank_[i].assign(static_cast<std::size_t>(sr_ * seconds[i]) + 2, 0.0f);
            for (std::size_t i=0; i<diffuser_.size(); ++i)
                diffuser_[i].assign(static_cast<std::size_t>(sr_ * diffuserSeconds[i]) + 2, 0.0f);
        } catch (...) { return false; }
        reset();
        smoothed_ = target_;
        return true;
    }

    void setKind(ReverbKind kind) noexcept { kind_ = kind; }

    void setParams(const ReverbParams& p) noexcept {
        target_.decaySeconds = finiteClamp(p.decaySeconds, .2f, 60.0f, 2.0f);
        target_.preDelayMs = finiteClamp(p.preDelayMs, 0.0f, 500.0f, 0.0f);
        target_.diffusion = finiteClamp(p.diffusion, 0.0f, 1.0f, .65f);
        target_.damping = finiteClamp(p.damping, 0.0f, .98f, .35f);
        target_.earlyLevel1 = finiteClamp(p.earlyLevel1, 0.0f, 1.0f, .25f);
        target_.earlyLevel2 = finiteClamp(p.earlyLevel2, 0.0f, 1.0f, .15f);
        target_.earlyTime1Ms = finiteClamp(p.earlyTime1Ms, 0.0f, 120.0f, 17.0f);
        target_.earlyTime2Ms = finiteClamp(p.earlyTime2Ms, 0.0f, 120.0f, 31.0f);
        target_.width = finiteClamp(p.width, 0.0f, 1.0f, 1.0f);
        target_.mix = finiteClamp(p.mix, 0.0f, 1.0f, 1.0f);
        target_.outputDb = finiteClamp(p.outputDb, -24.0f, 12.0f, 0.0f);
        target_.modRateHz = finiteClamp(p.modRateHz, .01f, 8.0f, .25f);
        target_.modDepthMs = finiteClamp(p.modDepthMs, 0.0f, 2.0f, .7f);
    }

    void reset() noexcept {
        for (auto& b : predelay_) std::fill(b.begin(), b.end(), 0.0f);
        for (auto& b : tank_) std::fill(b.begin(), b.end(), 0.0f);
        for (auto& b : diffuser_) std::fill(b.begin(), b.end(), 0.0f);
        predelayPos_ = 0;
        tankPos_.fill(0);
        diffuserPos_.fill(0);
        dampingState_.fill(0.0f);
        feedback_.fill(0.0f);
        outputGain_ = std::pow(10.0f, smoothed_.outputDb/20.0f);
        dampingPole_ = std::clamp(.12f + smoothed_.damping*.82f, .12f, .94f);
        lfoPhase_ = 0.0f;
    }

    void process(const float* inL, const float* inR, float* outL, float* outR,
                 std::size_t n) noexcept {
        if (predelay_[0].empty() || !inL || !inR || !outL || !outR) return;
        const float smooth = 1.0f - std::exp(-1.0f / (0.005f * sr_));
        const std::size_t active = kind_ == ReverbKind::Hall ? 8 : 4;
        const float decayLimit = kind_ == ReverbKind::Room ? .9985f : .99965f;
        float targetFeedback[8]{};
        for (std::size_t k=0; k<active; ++k) {
            const float loop = static_cast<float>(tank_[k].size()) / sr_;
            // T60 gain for each loop. Per-line values preserve the requested
            // decay time instead of applying a fixed, near-unity ceiling.
            targetFeedback[k] = std::min(decayLimit,
                std::pow(10.0f, -3.0f * loop / target_.decaySeconds));
        }
        const float targetDampingPole = std::clamp(.12f + target_.damping * .82f, .12f, .94f);
        const float targetOutputGain = std::pow(10.0f, target_.outputDb/20.0f);

        for (std::size_t i=0; i<n; ++i) {
            smoothParams(smooth);
            outputGain_ += smooth * (targetOutputGain-outputGain_);
            dampingPole_ += smooth * (targetDampingPole-dampingPole_);
            for (std::size_t k=0;k<active;++k)
                feedback_[k] += smooth * (targetFeedback[k] - feedback_[k]);
            const float dryL = finiteOrZero(inL[i]), dryR = finiteOrZero(inR[i]);
            const float pre = std::clamp(smoothed_.preDelayMs * .001f * sr_, 1.0f,
                                        static_cast<float>(predelay_[0].size()-2));
            const float predL = read(predelay_[0], predelayPos_, pre);
            const float predR = read(predelay_[1], predelayPos_, pre);
            const float t1 = std::clamp(smoothed_.earlyTime1Ms * .001f * sr_, 1.0f,
                                       static_cast<float>(predelay_[0].size()-2));
            const float t2 = std::clamp(smoothed_.earlyTime2Ms * .001f * sr_, 1.0f,
                                       static_cast<float>(predelay_[0].size()-2));
            const float ref1L = read(predelay_[0], predelayPos_, t1);
            const float ref1R = read(predelay_[1], predelayPos_, t1 * 1.037f);
            const float ref2L = read(predelay_[0], predelayPos_, t2 * .973f);
            const float ref2R = read(predelay_[1], predelayPos_, t2);
            predelay_[0][predelayPos_] = dryL;
            predelay_[1][predelayPos_] = dryR;
            if (++predelayPos_ == predelay_[0].size()) predelayPos_ = 0;

            float xL = predL, xR = predR;
            if (kind_ == ReverbKind::Room) {
                // Compact, audible first-order reflections with separate
                // left/right arrival patterns and moderate input diffusion.
                xL += smoothed_.earlyLevel1 * ref1L + .32f * smoothed_.earlyLevel2 * ref2R;
                xR += smoothed_.earlyLevel2 * ref2R + .32f * smoothed_.earlyLevel1 * ref1L;
                xL = diffuse(0, xL, .30f + .40f * smoothed_.diffusion);
                xR = diffuse(1, xR, .30f + .40f * smoothed_.diffusion);
            } else if (kind_ == ReverbKind::Hall) {
                // A longer reflection cloud feeding an eight-line field.
                // The secondary arrivals make the onset spatially denser.
                const float cloudL = .42f * ref1L + .27f * read(predelay_[1], predelayPos_, t1 * 1.71f)
                                   + .18f * ref2R + .13f * read(predelay_[0], predelayPos_, t2 * 1.43f);
                const float cloudR = .42f * ref2R + .27f * read(predelay_[0], predelayPos_, t1 * 1.71f)
                                   + .18f * ref1L + .13f * read(predelay_[1], predelayPos_, t2 * 1.43f);
                xL += smoothed_.earlyLevel1 * cloudL;
                xR += smoothed_.earlyLevel2 * cloudR;
                xL = diffuse(0, diffuse(2, xL, .48f + .43f * smoothed_.diffusion), .48f + .43f * smoothed_.diffusion);
                xR = diffuse(1, diffuse(3, xR, .48f + .43f * smoothed_.diffusion), .48f + .43f * smoothed_.diffusion);
            } else if (kind_ == ReverbKind::Plate) {
                // Plate topology: two serial allpass stages per side before
                // its cross-coupled decay field; early controls shape the hit.
                xL += smoothed_.earlyLevel1 * ref1L + .25f * smoothed_.earlyLevel2 * ref2L;
                xR += smoothed_.earlyLevel2 * ref2R + .25f * smoothed_.earlyLevel1 * ref1R;
                xL = diffuse(0, diffuse(2, xL, .50f + .45f * smoothed_.diffusion), .50f + .45f * smoothed_.diffusion);
                xR = diffuse(1, diffuse(3, xR, .50f + .45f * smoothed_.diffusion), .50f + .45f * smoothed_.diffusion);
            } else {
                xL = diffuse(0, xL, .35f + .45f * smoothed_.diffusion);
                xR = diffuse(1, xR, .35f + .45f * smoothed_.diffusion);
            }

            const float mod = (kind_ == ReverbKind::Modulated || kind_ == ReverbKind::Hall || kind_ == ReverbKind::Plate)
                            ? lookupLfo() * smoothed_.modDepthMs * .001f * sr_ : 0.0f;
            lfoPhase_ += smoothed_.modRateHz*static_cast<float>(lfoTable_.size())/sr_;
            if (lfoPhase_ >= static_cast<float>(lfoTable_.size())) lfoPhase_ -= static_cast<float>(lfoTable_.size());

            std::array<float,8> line{};
            float mean = 0.0f;
            for (std::size_t k=0;k<active;++k) {
                const float delay = static_cast<float>(tank_[k].size()-1) + ((k & 1u) ? mod : -mod);
                line[k] = read(tank_[k], tankPos_[k], std::max(1.0f, delay));
                dampingState_[k] += (1.0f-dampingPole_) * (line[k]-dampingState_[k]);
                line[k] = dampingState_[k];
                mean += line[k];
            }
            mean /= static_cast<float>(active);
            const float inMid = .5f * (xL + xR);
            const float inSide = .5f * (xL - xR);
            const float inputGain = .30f + .25f * smoothed_.diffusion;
            // A normalized Hadamard scatter is orthogonal (energy preserving)
            // before per-line decay and makes the plate's cross-coupled tank
            // ring instead of losing energy at every pass.
            const std::array<float,4> plateMix{{
                .5f*(line[0]+line[1]+line[2]+line[3]),
                .5f*(line[0]-line[1]+line[2]-line[3]),
                .5f*(line[0]+line[1]-line[2]-line[3]),
                .5f*(line[0]-line[1]-line[2]+line[3])}};
            float wetL=0.0f, wetR=0.0f;
            for (std::size_t k=0;k<active;++k) {
                float injection;
                if (kind_ == ReverbKind::Plate) {
                    // Alternating mid/side excitation gives a bright stereo plate.
                    injection = ((k & 1u) ? inSide : inMid) * inputGain;
                    tank_[k][tankPos_[k]] = injection + plateMix[k] * feedback_[k];
                } else {
                    injection = ((k & 1u) ? inSide : inMid) * inputGain;
                    // Orthogonal Householder scattering; Hall mixes eight
                    // lines while Room intentionally remains a smaller field.
                    tank_[k][tankPos_[k]] = injection + (2.0f*mean - line[k]) * feedback_[k];
                }
                if (++tankPos_[k] == tank_[k].size()) tankPos_[k]=0;
                const bool leftTap = kind_ == ReverbKind::Hall ? (k==0 || k==2 || k==5 || k==7) : (k==0 || k==2);
                (leftTap ? wetL : wetR) += line[k];
            }
            const float norm = kind_ == ReverbKind::Hall ? .5f : 1.0f;
            wetL *= norm; wetR *= norm;
            if (kind_ == ReverbKind::Plate) { const float l=wetL; wetL=.78f*l+.22f*wetR; wetR=.78f*wetR+.22f*l; }
            const float mid=.5f*(wetL+wetR), side=.5f*(wetL-wetR)*smoothed_.width;
            wetL=(mid+side)*outputGain_; wetR=(mid-side)*outputGain_;
            outL[i]=dryL+(wetL-dryL)*smoothed_.mix;
            outR[i]=dryR+(wetR-dryR)*smoothed_.mix;
        }
    }

private:
    static float finiteClamp(float v,float lo,float hi,float fallback) noexcept {
        return std::isfinite(v) ? std::clamp(v,lo,hi) : fallback;
    }
    static float finiteOrZero(float v) noexcept { return std::isfinite(v) ? v : 0.0f; }
    static float read(const std::vector<float>& b,std::size_t pos,float delay) noexcept {
        if (b.size()<2) return 0.0f;
        delay=std::clamp(delay,1.0f,static_cast<float>(b.size()-1));
        float p=static_cast<float>(pos)-delay;
        if(p<0.0f)p+=static_cast<float>(b.size());
        const auto i=static_cast<std::size_t>(p), j=(i+1)%b.size();
        return b[i]+(b[j]-b[i])*(p-static_cast<float>(i));
    }
    float diffuse(std::size_t ix,float x,float g) noexcept {
        auto& b=diffuser_[ix];
        const float delayed=read(b,diffuserPos_[ix],static_cast<float>(b.size()-1));
        const float y=delayed-g*x;
        b[diffuserPos_[ix]]=x+g*y;
        if(++diffuserPos_[ix]==b.size()) diffuserPos_[ix]=0;
        return y;
    }
    float lookupLfo() const noexcept {
        const auto i=static_cast<std::size_t>(lfoPhase_);
        const auto j=(i+1)%lfoTable_.size();
        return lfoTable_[i]+(lfoTable_[j]-lfoTable_[i])*(lfoPhase_-static_cast<float>(i));
    }
    void smoothParams(float a) noexcept {
#define TROLLFX_SMOOTH(field) smoothed_.field += a * (target_.field - smoothed_.field)
        TROLLFX_SMOOTH(decaySeconds); TROLLFX_SMOOTH(preDelayMs); TROLLFX_SMOOTH(diffusion);
        TROLLFX_SMOOTH(damping); TROLLFX_SMOOTH(earlyLevel1); TROLLFX_SMOOTH(earlyLevel2);
        TROLLFX_SMOOTH(earlyTime1Ms); TROLLFX_SMOOTH(earlyTime2Ms); TROLLFX_SMOOTH(width);
        TROLLFX_SMOOTH(mix); TROLLFX_SMOOTH(outputDb); TROLLFX_SMOOTH(modRateHz); TROLLFX_SMOOTH(modDepthMs);
#undef TROLLFX_SMOOTH
    }
    float sr_=44100.0f,lfoPhase_=0.0f,outputGain_=1.0f,dampingPole_=.4f;
    ReverbKind kind_=ReverbKind::Room;
    ReverbParams target_{},smoothed_{};
    std::array<std::vector<float>,2> predelay_{};
    std::array<std::vector<float>,8> tank_{};
    std::array<std::vector<float>,4> diffuser_{};
    std::size_t predelayPos_=0;
    std::array<std::size_t,8> tankPos_{};
    std::array<std::size_t,4> diffuserPos_{};
    std::array<float,8> dampingState_{},feedback_{};
    std::array<float,2048> lfoTable_{};
};

} // namespace trollfx
