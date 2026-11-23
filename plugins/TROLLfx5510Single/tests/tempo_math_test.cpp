#include "../src/tempo_math.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using trollfx5510::delaySamples;
    constexpr double sr = 44100.0;
    constexpr std::uint32_t max = 441000;
    std::uint32_t n = 0;
    assert(delaySamples(90.0, 9, 4, 4, 0.0, sr, max, n) && n == 29400); // quarter
    assert(delaySamples(120.0, 9, 4, 4, 0.0, sr, max, n) && n == 22050);
    assert(delaySamples(140.0, 9, 4, 4, 0.0, sr, max, n) && n == 18900);
    assert(delaySamples(120.0, 10, 4, 4, 0.0, sr, max, n) && n == 33075); // dotted quarter
    assert(delaySamples(120.0, 11, 4, 4, 0.0, sr, max, n) && n == 14700); // quarter triplet
    assert(delaySamples(90.0, 18, 4, 4, 0.0, sr, max, n) && n == 235200); // 2 bars
    assert(delaySamples(120.0, 18, 3, 4, 0.0, sr, max, n) && n == 132300); // 2 bars 3/4
    assert(delaySamples(120.0, 9, 4, 4, 12.0, sr, max, n) && n == 22579); // fine
    assert(!delaySamples(30.0, 18, 4, 4, 0.0, sr, max, n)); // exceeds 10 s
    assert(!delaySamples(120.0, 9, 4, 4, -600.0, sr, max, n)); // invalid underflow
    assert(!delaySamples(0.0, 9, 4, 4, 0.0, sr, max, n));
    std::cout << "tempo math: BPM 90/120/140, straight/dotted/triplet, meter, fine offset and capacity PASS\n";
}
