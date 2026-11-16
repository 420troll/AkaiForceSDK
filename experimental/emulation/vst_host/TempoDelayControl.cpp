#include "TempoDelayControl.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace trollfx::esp::vst {
namespace {

constexpr std::array<DivisionRatio,
    static_cast<std::size_t>(Dp4Division::Count)> kDivisions = {{
    {"1/32",          1,  8},
    {"1/16 Triplet",  1,  6},
    {"1/16",          1,  4},
    {"1/16 Dotted",   3,  8},
    {"1/8 Triplet",   1,  3},
    {"1/8",           1,  2},
    {"1/8 Dotted",    3,  4},
    {"1/4 Triplet",   2,  3},
    {"1/4",           1,  1},
    {"1/4 Dotted",    3,  2},
    {"1/2 Triplet",   4,  3},
    {"1/2",           2,  1}
}};

inline double finiteOr(double value, double fallback) noexcept {
    return std::isfinite(value) ? value : fallback;
}

inline std::uint32_t sampleCount(double milliseconds, double sampleRate,
                                 std::uint32_t maximum) noexcept {
    if (maximum == 0) return 0;
    sampleRate = finiteOr(sampleRate, 44100.0);
    if (sampleRate <= 0.0) sampleRate = 44100.0;
    milliseconds = finiteOr(milliseconds, 1.0);
    const double raw = milliseconds * sampleRate / 1000.0;
    if (raw <= 1.0) return 1;
    if (raw >= static_cast<double>(maximum)) return maximum;
    return static_cast<std::uint32_t>(std::floor(raw + 0.5));
}

inline double clampInternalBpm(double bpm) noexcept {
    bpm = finiteOr(bpm, 120.0);
    return std::clamp(bpm, TempoDelayControl::kMinInternalBpm,
                      TempoDelayControl::kMaxInternalBpm);
}

} // namespace

const DivisionRatio& TempoDelayControl::division(Dp4Division value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return kDivisions[index < kDivisions.size() ? index : 0];
}

const char* TempoDelayControl::divisionName(Dp4Division value) noexcept {
    return division(value).name;
}

TempoDelayResult TempoDelayControl::calculate(
    const TempoDelayRequest& request) noexcept {
    TempoDelayResult result{};
    const double rate = (std::isfinite(request.sampleRate) && request.sampleRate > 0.0)
                            ? request.sampleRate : 44100.0;
    const std::uint32_t maxSamples = request.maxDelaySamples == 0
                                         ? kDefaultMaxDelaySamples
                                         : request.maxDelaySamples;
    if (!request.sync) {
        result.requestedDelayMs = finiteOr(request.freeDelayMs, 250.0);
        result.delaySamples = sampleCount(result.requestedDelayMs, rate, maxSamples);
        result.synchronized = false;
        return result;
    }

    const double internalBpm = clampInternalBpm(request.internalBpm);
    double bpm = internalBpm;
    if (request.source == TempoSource::Host && request.hostTempoValid &&
        std::isfinite(request.hostBpm) && request.hostBpm > 0.0 &&
        request.hostBpm <= 1000.0) {
        bpm = request.hostBpm;
        result.usedHostTempo = true;
    } else {
        result.usedInternalFallback = request.source == TempoSource::Host;
    }

    const auto& ratio = division(request.division);
    const double beats = static_cast<double>(ratio.quarterNumerator) /
                         static_cast<double>(ratio.quarterDenominator);
    result.effectiveBpm = bpm;
    result.requestedDelayMs = (60.0 * 1000.0 * beats) / bpm;
    result.delaySamples = sampleCount(result.requestedDelayMs, rate, maxSamples);
    result.synchronized = true;
    return result;
}

} // namespace trollfx::esp::vst
