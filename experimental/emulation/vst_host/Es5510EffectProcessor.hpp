#pragma once

/*
 * Real-time host adapter for the original TROLLfx ES5510 delay and chorus.
 * All effect audio runs through MameES5510 instructions. The chorus LFO only
 * updates its two delay-address GPRs; it does not generate effect audio.
 *
 * The embedded processor is adapted from MAME and licensed under BSD-3-Clause.
 * See ../THIRD_PARTY_NOTICES.md.
 */

#include "../mame_es5510_standalone.hpp"

#include <cstddef>
#include <cstdint>

namespace trollfx::esp::vst {

class Es5510EffectProcessor {
public:
    enum class Mode : std::uint8_t { Delay = 0, Chorus = 1 };

    // The wrapper snapshots its host parameters once at the start of each
    // process block and passes that immutable value into processBlock().
    struct Parameters {
        Mode mode = Mode::Delay;
        float delayMs = 250.0f;
        float feedback = 0.55f;
        float wet = 0.5f;
        float dry = 0.5f;
        float rateHz = 0.5f;
        float depthMs = 5.0f;
        bool bypass = false;
    };

    Es5510EffectProcessor() = default;
    Es5510EffectProcessor(const Es5510EffectProcessor&) = delete;
    Es5510EffectProcessor& operator=(const Es5510EffectProcessor&) = delete;

    // May allocate the two fixed 2 MiB delay RAM images. Call only during
    // plugin prepare/activation, never from the audio callback.
    bool prepare(double sampleRate, std::size_t maximumBlockSize) noexcept;

    // Clears both delay histories and resets the LFO/crossfade state. This is
    // a control-thread operation because it visits the prepared delay RAM.
    void reset() noexcept;

    // Non-interleaved float stereo process. Null inputs are silence; null
    // outputs are ignored. Same-channel in-place processing is supported.
    // `frames` is not restricted to maximumBlockSize because processing uses
    // no temporary block buffers.
    void processBlock(const float* inputLeft, const float* inputRight,
                      float* outputLeft, float* outputRight,
                      std::size_t frames, const Parameters& parameters) noexcept;

    bool isPrepared() const noexcept { return prepared_; }
    double sampleRate() const noexcept { return sampleRate_; }
    std::size_t maximumBlockSize() const noexcept { return maximumBlockSize_; }
    std::uint64_t processedFrames() const noexcept { return processedFrames_; }

private:
    struct SmoothedValue {
        float value = 0.0f;
        float target = 0.0f;
        float step = 0.0f;
        std::uint32_t remaining = 0;

        void setImmediate(float v) noexcept;
        void setTarget(float v, std::uint32_t rampFrames) noexcept;
        float next() noexcept;
    };

    static constexpr std::size_t kProgramDelayWords = 19;
    static constexpr std::size_t kProgramChorusWords = 11;
    static constexpr double kMaxDelaySeconds = 10.0;
    static constexpr float kSmoothingSeconds = 0.010f;

    static bool supportedRate(double rate, std::uint32_t& integerRate) noexcept;
    static float sanitize(float value, float fallback, float minimum,
                          float maximum) noexcept;
    static std::uint32_t q22(float value) noexcept;
    static std::int16_t toPcm16(float sample) noexcept;
    static float fromPcm16(std::int16_t sample) noexcept;
    static void configureEngine(MameES5510& engine,
                                const std::uint64_t* program,
                                std::size_t programWords,
                                std::uint32_t capacitySamples,
                                std::uint32_t sampleRate,
                                bool delayProgram,
                                const Parameters& parameters) noexcept;
    void initializeEngines(const Parameters& parameters) noexcept;
    void applyBlockParameters(const Parameters& parameters) noexcept;
    float nextModeMix() noexcept;
    float nextBypassMix() noexcept;

    MameES5510 delayEngine_;
    MameES5510 chorusEngine_;
    bool prepared_ = false;
    bool parametersInitialized_ = false;
    std::uint32_t sampleRateInteger_ = 0;
    double sampleRate_ = 0.0;
    std::size_t maximumBlockSize_ = 0;
    std::uint32_t capacitySamples_ = 0;
    std::uint64_t processedFrames_ = 0;
    double chorusPhaseCycles_ = 0.0;

    SmoothedValue delaySamples_{};
    SmoothedValue feedback_{};
    SmoothedValue delayWet_{};
    SmoothedValue delayDry_{};
    SmoothedValue chorusRate_{};
    SmoothedValue chorusDepthSamples_{};
    SmoothedValue chorusWet_{};
    float modeMix_ = 0.0f;       // 0 = delay; 1 = chorus
    float modeTarget_ = 0.0f;
    float modeStep_ = 0.0f;
    std::uint32_t modeRampRemaining_ = 0;
    float bypassMix_ = 0.0f;     // 0 = effect; 1 = dry input
    float bypassTarget_ = 0.0f;
    float bypassStep_ = 0.0f;
    std::uint32_t bypassRampRemaining_ = 0;
};

} // namespace trollfx::esp::vst
