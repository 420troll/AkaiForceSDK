#pragma once

#include <cstdint>

namespace trollfx::esp::vst {

enum class TempoSource : std::uint8_t { Host = 0, Internal = 1 };
enum class Dp4Division : std::uint8_t {
    ThirtySecond = 0,
    SixteenthTriplet,
    Sixteenth,
    SixteenthDotted,
    EighthTriplet,
    Eighth,
    EighthDotted,
    QuarterTriplet,
    Quarter,
    QuarterDotted,
    HalfTriplet,
    Half,
    Count
};

struct DivisionRatio {
    const char* name;
    std::uint8_t quarterNumerator;
    std::uint8_t quarterDenominator;
};

struct TempoDelayRequest {
    bool sync = false;
    TempoSource source = TempoSource::Host;
    Dp4Division division = Dp4Division::Quarter;
    double internalBpm = 120.0;
    double hostBpm = 120.0;
    bool hostTempoValid = false;
    double freeDelayMs = 250.0;
    double sampleRate = 44100.0;
    std::uint32_t maxDelaySamples = 441000;
};

struct TempoDelayResult {
    // For unsynced requests this is exactly freeDelayMs. For synced requests
    // this is the rational division time before integer-sample quantization.
    double requestedDelayMs = 250.0;
    // Sample-rounded, clamped value suitable for conversion to ES5510 GPRs.
    std::uint32_t delaySamples = 11025;
    double effectiveBpm = 120.0;
    bool usedHostTempo = false;
    bool usedInternalFallback = false;
    bool synchronized = false;
};

class TempoDelayControl final {
public:
    static constexpr double kMinInternalBpm = 50.0;
    static constexpr double kMaxInternalBpm = 250.0;
    static constexpr std::uint32_t kDefaultMaxDelaySamples = 441000;

    static const DivisionRatio& division(Dp4Division value) noexcept;
    static const char* divisionName(Dp4Division value) noexcept;

    // No state, allocation, logging or host calls. Invalid/missing host BPM
    // falls back to the clamped internal BPM rather than using stale tempo.
    static TempoDelayResult calculate(const TempoDelayRequest& request) noexcept;
};

} // namespace trollfx::esp::vst
