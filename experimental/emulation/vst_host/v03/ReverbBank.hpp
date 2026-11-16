#pragma once

#include "../../mame_es5510_standalone.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace trollfx::esp::vst::v03 {

class ReverbBank final {
public:
    enum class Mode : std::uint8_t { SmallPlate, LargePlate, Room, Hall, Gated, ReverseOneShot, ReverseRetrigger, NonLin1, NonLin2, NonLin3, Count };
    struct Parameters {
        float wet=0.65f, dry=0.0f, decaySeconds=1.8f, predelayMs=0.0f;
        float diffusion1=55.0f, diffusion2=50.0f, earlyLevel=25.0f, damping=35.0f;
        std::array<float,9> tapLevels{{76,66,58,52,47,42,37,27,16}};
        float attackMs=35.0f, holdMs=80.0f, releaseMs=240.0f;
        float triggerDb=-32.0f, retriggerDb=-28.0f;
        bool retrigger=true;
    };

    bool prepare(double sampleRate) noexcept;
    void processBlock(Mode mode,const float* inL,const float* inR,float* outL,float* outR,
                      std::size_t frames,const Parameters& parameters) noexcept;
    std::uint64_t processedFrames() const noexcept { return processedFrames_; }
    std::uint32_t maxInstructionCount() const noexcept { return maxInstructionCount_; }

private:
    struct Engine {
        MameES5510 cpu;
        std::uint32_t leftTap=0,rightTap=0,bankSamples=0;
        std::uint8_t writeLeft=0,writeRight=0;
        std::uint8_t feedbackGpr=40,wetGpr=41,dryGpr=42;
        std::array<float,8> coefficient{};
        std::array<std::uint8_t,8> coefficientGpr{};
        std::size_t coefficientCount=0;
        std::uint8_t envGpr=0;
        std::uint8_t dampingGpr=0;
        std::array<std::uint8_t,9> tapLevelGpr{};
        std::array<std::uint8_t,9> tapLeftGpr{},tapRightGpr{};
        std::array<std::uint32_t,9> tapLeft{},tapRight{};
        std::array<float,9> defaultTapLevels{};
        std::size_t tapCount=0;
        float feedbackBase=0.65f;
        std::uint64_t holdRemaining=0;
        float envelope=0.0f;
        bool triggered=false,previousAbove=false;
        std::uint64_t frames=0;
    };

    static constexpr std::size_t kCount=static_cast<std::size_t>(Mode::Count);
    static std::int16_t toPcm(float x) noexcept;
    static float fromPcm(std::int16_t x) noexcept;
    static std::uint32_t q22(float x) noexcept;
    static std::uint32_t q22Signed(float x) noexcept;
    bool setup(Mode mode) noexcept;
    void setControls(Mode mode,Engine& e,const Parameters& p) noexcept;
    float nextEnvelope(Mode mode,Engine& e,float inL,float inR,const Parameters& p) noexcept;
    std::array<std::int16_t,2> run(Mode mode,const std::array<std::int16_t,6>& input,
                                   const Parameters& p) noexcept;

    std::array<Engine,kCount> engines_{};
    bool prepared_=false;
    double sampleRate_=44100.0;
    std::uint32_t sampleRateInt_=44100;
    Mode active_=Mode::SmallPlate,fadeFrom_=Mode::SmallPlate,fadeTo_=Mode::SmallPlate;
    std::uint32_t fadeRemaining_=0,fadeTotal_=0;
    std::uint64_t processedFrames_=0;
    std::uint32_t maxInstructionCount_=0;
};

} // namespace trollfx::esp::vst::v03
