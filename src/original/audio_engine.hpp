#pragma once
#include <cstddef>
#include <array>
#include <vector>
#include "dp4_reverb.hpp"

namespace trollfx {

class AudioEngine {
public:
    virtual ~AudioEngine() = default;
    virtual bool prepare(float sampleRate, int maxBlock) = 0;
    virtual void reset() noexcept = 0;
    virtual void process(const float* inL, const float* inR, float* outL,
                         float* outR, std::size_t frames) noexcept = 0;
};

enum class EffectAlgorithm : unsigned char {
    Delay, Chorus, Flanger, Room, Hall, Plate, ModulatedReverb,
    VCFDistortion, EQPannerDDL, Compressor,
    MultiTapDelay, DualDelay, EightVoiceChorus, ParametricEQ,
    DeEsser, Expander, Gate, RumbleFilter
};

struct EffectParams {
    float timeMs = 420.0f;
    float feedback = 0.32f;
    float mix = 0.35f;
    float rateHz = 0.27f;
    float depthMs = 2.0f;
    float diffusion = 0.65f;
    float damping = 0.35f;
    float width = 1.0f;
    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float attackMs = 8.0f;
    float releaseMs = 120.0f;
    float cutoffHz = 1800.0f;
    float distortionGainInDb = 12.0f, distortionGainOutDb = -6.0f;
    float driveDb = 12.0f;
    float preVcfCutoffHz = 5000.0f, preVcfQ = 0.707f, preVcfEnvDepth = 0.0f;
    float postVcfCutoffHz = 5000.0f, postVcfQ = 0.707f, postVcfEnvDepth = 0.0f;
    float envelopeAttackMs = 10.0f, envelopeReleaseMs = 100.0f;
    bool distortionBypass = false;
    float compGainDb = 0.0f, gateOffDb = -96.0f, gateOnDb = -90.0f, gateReleaseMs = 100.0f;
    bool pannerPhaseOpposed = false;
    float sampleHoldHz = 0.0f;
    float leftDelayMs = 0.0f, rightDelayMs = 0.0f, leftEchoMs = 0.0f, rightEchoMs = 0.0f;
    float echoLevel = 0.0f, bassFcHz = 180.0f, bassGainDb = 0.0f;
    float trebleFcHz = 6000.0f, trebleGainDb = 0.0f, inputTrimDb = 0.0f;
    float mid1FcHz = 600.0f, mid1GainDb = 0.0f, mid1Q = 1.0f;
    float mid2FcHz = 2400.0f, mid2GainDb = 0.0f, mid2Q = 1.0f;
    float decaySeconds = 2.0f, preDelayMs = 0.0f;
    float earlyLevel1 = 0.25f, earlyLevel2 = 0.15f;
    float earlyTime1Ms = 17.0f, earlyTime2Ms = 31.0f, outputDb = 0.0f;
};

// One prepared stereo engine. Parameter setters are control-thread operations;
// process() performs no allocation, locking or I/O.
class EffectsUnit final : public AudioEngine {
public:
    bool prepare(float sampleRate, int maxBlock) override;
    void reset() noexcept override;
    void process(const float* inL, const float* inR, float* outL, float* outR,
                 std::size_t frames) noexcept override;
    void setAlgorithm(EffectAlgorithm algorithm) noexcept;
    void setParams(const EffectParams& params) noexcept;

private:
    struct DelayLine {
        float* data = nullptr; std::size_t size = 0, pos = 0;
        void clear() noexcept;
        float read(float delaySamples) const noexcept;
        void write(float value) noexcept;
    };
    float clampFeedback(float x) const noexcept;
    void processDelay(const float* l,const float* r,float* ol,float* or_,std::size_t n,bool mod,bool flange) noexcept;
    void processReverb(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    void processVCFDistortion(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    void processPannerDDL(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    void processCompressor(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    void processEightVoiceChorus(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    void processParametricEQ(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    void processDeEsser(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    void processExpander(const float* l,const float* r,float* ol,float* or_,std::size_t n,bool gate) noexcept;
    void processRumbleFilter(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept;
    float sampleRate_ = 44100.0f, phase_ = 0.0f;
    EffectAlgorithm algorithm_ = EffectAlgorithm::Delay;
    EffectParams params_{};
    DelayLine delayL_{},delayR_{};
    DelayLine tank_[4]{};
    DelayLine diffuser_[4]{};
    float damp_[4]{};
    float filter_[32]{}, envelope_[2]{}, gateGain_ = 1.0f;
    float eqBass_[2]{}, eqTreble_[2]{}, heldPhase_ = 0.0f, holdPhase_ = 0.0f;
    std::size_t holdCounter_ = 0;
    DP4InspiredReverb reverb_{};
    // Storage is owned here to guarantee stable delay-line pointers.
    std::vector<float> storage_;
};

} // namespace trollfx
