#pragma once
#include <cstddef>
#include <vector>

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
    VCFDistortion, EQPannerDDL, Compressor
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
    float sampleRate_ = 44100.0f, phase_ = 0.0f;
    EffectAlgorithm algorithm_ = EffectAlgorithm::Delay;
    EffectParams params_{};
    DelayLine delayL_{},delayR_{};
    DelayLine tank_[4]{};
    DelayLine diffuser_[4]{};
    float damp_[4]{};
    float filter_[2]{}, envelope_[2]{};
    // Storage is owned here to guarantee stable delay-line pointers.
    std::vector<float> storage_;
};

} // namespace trollfx
