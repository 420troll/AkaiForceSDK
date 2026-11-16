#pragma once

#include <cstddef>

namespace trollfx {

// Stable host-facing boundary for each independently routed unit. Implementors
// allocate in prepare(), update parameters outside process(), and must support
// in-place buffers and arbitrary block lengths up to the prepared maximum.
class AudioEngine {
public:
    virtual ~AudioEngine() = default;
    virtual bool prepare(float sampleRate, int maxBlockSize) = 0;
    virtual void reset() noexcept = 0;
    virtual void process(const float* inL, const float* inR,
                         float* outL, float* outR,
                         std::size_t frames) noexcept = 0;
};

} // namespace trollfx
