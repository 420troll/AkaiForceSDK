#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace trollfx {

// These normalized destinations map to continuous members of EffectParams.
// An algorithm only acts on destinations that it actually consumes.
enum class ModDestination : unsigned char {
    Time, Feedback, Mix, Rate, Depth, Diffusion, Damping, Width,
    Threshold, Ratio, Attack, Release, Cutoff, Count
};
enum class ModSource : unsigned char {
    Off, HostController1, HostController2, HostController3, HostController4,
    HostController5, HostController6, HostController7, HostController8,
    InternalLfo, Envelope
};

struct ModulatorSlot {
    bool enabled = false;
    ModSource source = ModSource::Off;
    ModDestination destination = ModDestination::Mix;
    float minNorm = 0.0f;
    float maxNorm = 1.0f;
    bool invert = false;
};

// Two independently assignable modulators per unit. Controller 1..8 values
// are eight global host-automatable normalized parameters, not MIDI CC.
// Internal LFO/envelope sources are TROLLfx additions. When a route is active
// its endpoint mapping overrides base for the assigned destination. Duplicate
// destinations use slot 2.
class ModulationMatrix final {
public:
    static constexpr std::size_t kParamCount = static_cast<std::size_t>(ModDestination::Count);
    using Values = std::array<float, kParamCount>;

    void setSlot(std::size_t index, ModulatorSlot slot) noexcept {
        if (index >= slots_.size()) return;
        slot.minNorm = clamp01(slot.minNorm);
        slot.maxNorm = clamp01(slot.maxNorm);
        if (static_cast<unsigned>(slot.source) > static_cast<unsigned>(ModSource::Envelope)) slot.source = ModSource::Off;
        if (static_cast<unsigned>(slot.destination) >= static_cast<unsigned>(ModDestination::Count)) slot.destination = ModDestination::Mix;
        slots_[index] = slot;
    }
    const ModulatorSlot& slot(std::size_t i) const noexcept { return slots_[i < 2 ? i : 1]; }
    void beginBlock(float sampleRate, float lfoRateHz) noexcept {
        if (sampleRate > 0.0f) sampleRate_ = sampleRate;
        const float rate = std::clamp(lfoRateHz, 0.01f, 20.0f);
        phaseInc_ = 6.28318530718f * rate / sampleRate_;
        attackCoeff_ = std::exp(-1.0f / (0.010f * sampleRate_));
        releaseCoeff_ = std::exp(-1.0f / (0.120f * sampleRate_));
    }
    void reset() noexcept { phase_ = 0.0f; envelope_ = 0.0f; }

    // Call once per audio sample. input drives the envelope detector. Base and
    // output use normalized 0..1 control values; this routine allocates none.
    void process(float inputL, float inputR, const Values& base, Values& output) noexcept {
        const std::array<float, 8> noControllers{};
        process(inputL, inputR, noControllers, base, output);
    }
    void process(float inputL, float inputR, const std::array<float, 8>& controllers,
                 const Values& base, Values& output) noexcept {
        output = base;
        const float level = clamp01(std::max(std::fabs(inputL), std::fabs(inputR)));
        const float coeff = level > envelope_ ? attackCoeff_ : releaseCoeff_;
        envelope_ = level + coeff * (envelope_ - level);
        const float lfo = 0.5f + 0.5f * std::sin(phase_);
        phase_ += phaseInc_;
        if (phase_ >= 6.28318530718f) phase_ -= 6.28318530718f;
        for (const auto& s : slots_) {
            if (!s.enabled || s.source == ModSource::Off) continue;
            float value = 0.0f;
            const unsigned source = static_cast<unsigned>(s.source);
            if (source >= static_cast<unsigned>(ModSource::HostController1) &&
                source <= static_cast<unsigned>(ModSource::HostController8)) {
                value = clamp01(controllers[source - static_cast<unsigned>(ModSource::HostController1)]);
            } else if (s.source == ModSource::InternalLfo) value = lfo;
            else value = clamp01(envelope_);
            if (s.invert) value = 1.0f - value;
            const float lo = std::min(s.minNorm, s.maxNorm);
            const float hi = std::max(s.minNorm, s.maxNorm);
            output[static_cast<std::size_t>(s.destination)] = lo + (hi - lo) * value;
        }
    }

    static float fromDisplay99(unsigned value) noexcept { return clamp01(static_cast<float>(std::min(value, 99u)) / 99.0f); }
    static unsigned toDisplay99(float value) noexcept { return static_cast<unsigned>(std::lround(clamp01(value) * 99.0f)); }

private:
    static float clamp01(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }
    std::array<ModulatorSlot, 2> slots_{};
    float sampleRate_ = 44100.0f, phase_ = 0.0f, phaseInc_ = 0.0f;
    float envelope_ = 0.0f, attackCoeff_ = 0.0f, releaseCoeff_ = 0.0f;
};
}
