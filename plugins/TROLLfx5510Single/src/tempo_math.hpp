#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace trollfx5510 {
struct Division { const char* label; double quarterNotes; int denominator; int feel; };
// Straight, dotted, then triplet values for each note length; last entry is two bars.
inline constexpr std::array<Division, 19> kDivisions = {{
    {"1/32", 0.125, 32, 0}, {"1/32 dotted", 0.1875, 32, 1}, {"1/32 triplet", 1.0/12.0, 32, 2},
    {"1/16", 0.25, 16, 0}, {"1/16 dotted", 0.375, 16, 1}, {"1/16 triplet", 1.0/6.0, 16, 2},
    {"1/8", 0.5, 8, 0}, {"1/8 dotted", 0.75, 8, 1}, {"1/8 triplet", 1.0/3.0, 8, 2},
    {"1/4", 1.0, 4, 0}, {"1/4 dotted", 1.5, 4, 1}, {"1/4 triplet", 2.0/3.0, 4, 2},
    {"1/2", 2.0, 2, 0}, {"1/2 dotted", 3.0, 2, 1}, {"1/2 triplet", 4.0/3.0, 2, 2},
    {"1/1", 4.0, 1, 0}, {"1/1 dotted", 6.0, 1, 1}, {"1/1 triplet", 8.0/3.0, 1, 2},
    {"2 bars", 8.0, 0, 0}
}};

inline bool delaySamples(double bpm, std::size_t division, int meterNumerator,
                         int meterDenominator, double fineMilliseconds,
                         double sampleRate, std::uint32_t maximum,
                         std::uint32_t& result) noexcept {
    if (!std::isfinite(bpm) || !std::isfinite(fineMilliseconds) ||
        !std::isfinite(sampleRate) || bpm <= 0.0 || sampleRate <= 0.0 ||
        division >= kDivisions.size() || maximum == 0) return false;
    double beats = kDivisions[division].quarterNotes;
    if (division == kDivisions.size() - 1) {
        if (meterNumerator <= 0 || meterDenominator <= 0) return false;
        beats = 2.0 * static_cast<double>(meterNumerator) * 4.0 /
                static_cast<double>(meterDenominator);
    }
    const double samples = sampleRate * 60.0 * beats / bpm +
                           sampleRate * fineMilliseconds * 0.001;
    if (!std::isfinite(samples) || samples < 1.0 || samples > static_cast<double>(maximum)) return false;
    const auto rounded = static_cast<std::uint64_t>(std::llround(samples));
    if (rounded == 0 || rounded > maximum) return false;
    result = static_cast<std::uint32_t>(rounded);
    return true;
}
} // namespace trollfx5510
