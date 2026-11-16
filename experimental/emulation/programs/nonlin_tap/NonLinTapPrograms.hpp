#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
namespace trollfx::esp::programs::nonlin_tap {
enum class Id : std::uint8_t { NonLin1, NonLin2, NonLin3 };
struct Definition { Id id; const char* name; const std::uint64_t* words; std::size_t wordCount; unsigned writeLeftPc, writeRightPc; std::array<unsigned,9> readLeftPc, readRightPc; std::array<std::uint32_t,9> tapLeft, tapRight; std::array<float,9> defaultLevels; std::array<std::pair<unsigned,float>,8> coefficients; std::size_t coefficientCount; };
inline constexpr std::array<std::uint64_t,81> kNONLIN1_TRANSIENT = {
#include "nonlin1_transient_words.inc"
};
inline constexpr std::array<std::uint64_t,117> kNONLIN2_BLOOM = {
#include "nonlin2_bloom_words.inc"
};
inline constexpr std::array<std::uint64_t,86> kNONLIN3_DRUM = {
#include "nonlin3_drum_words.inc"
};
inline constexpr std::array<Definition,3> kCatalog = {{
{Id::NonLin1,"nonlin1_transient",kNONLIN1_TRANSIENT.data(),kNONLIN1_TRANSIENT.size(),14,15,{16,20,24,28,32,36,40,44,48},{17,21,25,29,33,37,41,45,49},{173,293,467,719,1031,1453,2027,11519,21101},{211,347,523,797,1151,1597,2237,12101,22051},{0.76,0.66,0.58,0.52,0.47,0.42,0.37,0.27,0.16},{{{128,0.46},{136,0.46}}},2},
{Id::NonLin2,"nonlin2_bloom",kNONLIN2_BLOOM.data(),kNONLIN2_BLOOM.size(),50,51,{52,56,60,64,68,72,76,80,84},{53,57,61,65,69,73,77,81,85},{4999,9001,16001,23003,30011,38011,46021,54001,62003},{7001,12007,19001,26003,34019,42013,50023,58013,66013},{0.7,0.62,0.58,0.53,0.48,0.43,0.39,0.31,0.21},{{{128,0.62},{129,-0.58},{130,0.66},{131,-0.61},{136,0.62},{137,-0.58},{138,0.66},{139,-0.61}}},8},
{Id::NonLin3,"nonlin3_drum",kNONLIN3_DRUM.data(),kNONLIN3_DRUM.size(),19,20,{21,25,29,33,37,41,45,49,53},{22,26,30,34,38,42,46,50,54},{97,163,263,397,557,751,977,1237,1531},{97,163,263,397,557,751,977,1237,1531},{0.8,0.66,0.54,0.45,0.38,0.31,0.25,0.19,0.12},{{{128,0.42},{136,0.42}}},2},
}};
constexpr const Definition* find(Id id) noexcept { for (const auto& p:kCatalog) if(p.id==id) return &p; return nullptr; }
static_assert(kNONLIN1_TRANSIENT.size()<=160 && kNONLIN2_BLOOM.size()<=160 && kNONLIN3_DRUM.size()<=160,"ES5510 program memory limit");
}
