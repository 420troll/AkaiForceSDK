#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace trollfx::esp::programs::reverbs {
enum class Id : std::uint8_t { SmallPlate, LargePlate };
struct Definition {
    Id id;
    const char* name;
    const std::uint64_t* words;
    std::size_t wordCount;
    std::size_t dramWords;
    std::uint32_t defaultLeftTap;
    std::uint32_t defaultRightTap;
    std::uint32_t leftWriteAddressGpr;
    std::uint32_t rightWriteAddressGpr;
    std::uint8_t feedbackGpr;
    std::uint8_t wetGpr;
    std::uint8_t dryGpr;
    std::uint8_t leftCoefficientBase;
    std::uint8_t rightCoefficientBase;
    std::uint8_t coefficientStride;
    std::size_t diffusionStagesPerChannel;
    float defaultFeedback;
};
inline constexpr std::size_t kRecommendedDramWords = 65536; // two 32768-word banks
inline constexpr std::uint8_t kReadLeftGpr = 0, kReadRightGpr = 1;
inline constexpr std::array<std::uint64_t,31> kNonLin1Transient = {
#include "nonlin1_transient_words.inc"
};
inline constexpr std::array<std::uint64_t,61> kNonLin2Bloom = {
#include "nonlin2_bloom_words.inc"
};
inline constexpr std::array<std::uint64_t,36> kNonLin3Drum = {
#include "nonlin3_drum_words.inc"
};
inline constexpr std::array<std::uint64_t,41> kSmallPlate = {
#include "small_plate_words.inc"
};
inline constexpr std::array<std::uint64_t,51> kLargePlate = {
#include "large_plate_words.inc"
};
// The three Non Lin-inspired sketches below remain research fixtures. They
// lack the verified nine-level host-scheduled output envelopes and are not
// exposed through the V0.3-ready descriptor catalog.
inline constexpr std::array<Definition,2> kPlateCatalog = {{
    {Id::SmallPlate,"Small Plate (TROLLfx prototype)",kSmallPlate.data(),kSmallPlate.size(),kRecommendedDramWords,1301,1499,30,31,160,161,162,64,80,16,2,0.68f},
    {Id::LargePlate,"Large Plate (TROLLfx prototype)",kLargePlate.data(),kLargePlate.size(),kRecommendedDramWords,2801,3163,40,41,160,161,162,64,80,16,3,0.82f},
}};
constexpr const Definition* find(Id id) noexcept {
    for (const auto& item : kPlateCatalog) if (item.id == id) return &item;
    return nullptr;
}
static_assert(kNonLin1Transient.size() <= 160 && kNonLin2Bloom.size() <= 160 &&
              kNonLin3Drum.size() <= 160 && kSmallPlate.size() <= 160 &&
              kLargePlate.size() <= 160, "ES5510 instruction RAM is 160 words");
} // namespace trollfx::esp::programs::reverbs
