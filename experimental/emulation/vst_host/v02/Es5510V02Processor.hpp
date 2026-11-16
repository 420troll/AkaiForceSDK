#pragma once

#include "../../mame_es5510_standalone.hpp"
#include "../TempoDelayControl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace trollfx::esp::vst::v02 {

class Processor final {
public:
    enum class Mode : std::uint8_t { Delay, Chorus, Phaser, Panner, Rotary };
    enum class Waveform : std::uint8_t { Sine, Triangle, Square };
    enum class RotarySpeed : std::uint8_t { Stop, Slow, Fast };

    struct Parameters {
        Mode mode = Mode::Delay;
        bool pingPong = false;
        double delayMs = 500.0;
        float feedback = 0.55f, wet = 0.5f, dry = 0.5f;
        bool sync = false;
        TempoSource tempoSource = TempoSource::Host;
        Dp4Division division = Dp4Division::Quarter;
        double internalBpm = 120.0, hostBpm = 120.0;
        bool hostTempoValid = false;
        float chorusRateHz = 0.5f, chorusDepthMs = 5.0f;
        float phaserRateHz = 0.25f, phaserDepthOctaves = 2.0f;
        float phaserCenterHz = 900.0f, phaserSpreadOctaves = 0.25f;
        float phaserFeedback = 0.65f;
        float panPosition = 0.0f, panWidth = 1.0f;
        float panRateHz = 0.25f, panMotion = 0.8f;
        Waveform panWaveform = Waveform::Sine;
        bool reverse = false, panSync = false;
        RotarySpeed rotarySpeed = RotarySpeed::Slow;
        float hornSpeedHz = 6.0f, drumSpeedHz = 0.8f;
        float accelerationSeconds = 1.0f, decelerationSeconds = 1.5f;
        float rotaryWidth = 1.0f, dopplerDepthMs = 2.0f, drive = 0.0f;
        bool bypass = false;
    };

    Processor() = default;
    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;

    bool prepare(double sampleRate, std::size_t maxBlockSize) noexcept;
    void reset() noexcept;
    void processBlock(const float* inL, const float* inR, float* outL, float* outR,
                      std::size_t frames, const Parameters& parameters) noexcept;
    double sampleRate() const noexcept { return sampleRate_; }
    std::size_t maxBlockSize() const noexcept { return maxBlockSize_; }
    std::uint64_t processedFrames() const noexcept { return processedFrames_; }
    bool isPrepared() const noexcept { return prepared_; }

private:
    enum class Program : std::uint8_t { Delay, PingPong, Chorus, Phaser, Panner, Rotary, Count };
    struct Smooth {
        float value=0.0f,target=0.0f,step=0.0f;std::uint32_t remain=0;
        void immediate(float x) noexcept;
        void set(float x,std::uint32_t samples) noexcept;
        float next() noexcept;
    };
    static constexpr std::size_t kProgramCount=static_cast<std::size_t>(Program::Count);
    static constexpr double kMaxDelaySeconds=10.0;
    static constexpr std::uint32_t kFadeMs=10;
    bool initEngine(Program p,std::size_t dramWords) noexcept;
    void configure(Program p) noexcept;
    void setTempoDelay(const Parameters& p) noexcept;
    void updateProgramControls(Program p,const Parameters& pms) noexcept;
    std::array<std::int16_t,2> run(Program p,const std::array<std::int16_t,6>& in,
                                   const Parameters& pms) noexcept;
    float nextPanWave(Waveform wave) noexcept;
    float nextBypass() noexcept;
    static float finite(float v,float fallback) noexcept;
    static std::int16_t toPcm(float v) noexcept;
    static float fromPcm(std::int16_t v) noexcept;
    static std::uint32_t q22(float v) noexcept;
    static std::uint32_t q22Signed(float v) noexcept;

    std::array<MameES5510,kProgramCount> engines_{};
    bool prepared_=false,paramsInitialized_=false;
    double sampleRate_=0.0;
    std::uint32_t sampleRateInt_=0,ringCapacity_=0;
    std::size_t maxBlockSize_=0;
    std::uint64_t processedFrames_=0;
    Program active_=Program::Delay,fadeFrom_=Program::Delay,fadeTo_=Program::Delay;
    std::uint32_t fadeRemaining_=0,fadeTotal_=0;
    double chorusPhase_=0.0,phaserPhase_=0.0,panPhase_=0.0,hornPhase_=0.0,drumPhase_=0.0;
    float hornRateState_=0.0f,drumRateState_=0.0f;
    Smooth delaySamples_{},feedback_{},wet_{},dry_{},chorusRate_{},chorusDepthSamples_{};
    Smooth phaserRate_{},phaserCenter_{},phaserDepth_{},phaserSpread_{},phaserFeedback_{};
    Smooth panPosition_{},panWidth_{},panMotion_{},panRate_{};
    Smooth rotaryHorn_{},rotaryDrum_{},rotaryWidth_{},rotaryDoppler_{},rotaryDrive_{};
    Smooth bypass_{};
    std::uint32_t tempoDelayTarget_=0;
    std::uint32_t phaserControlCounter_=0;
};

} // namespace trollfx::esp::vst::v02
