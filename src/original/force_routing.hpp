#pragma once
#include <algorithm>

namespace trollfx {

// Stereo 1-source approximations of the DP/4 AB Feedback 1/2 concepts.
// The VST host gets a one-block feedback delay; these do not reproduce the
// DP/4's physical ESP routing or exact dry-path circuit.
inline float routeFeedbackInput(float dry, float previousProcessed, float send) noexcept {
    return dry + previousProcessed * std::clamp(send, -0.92f, 0.92f);
}
inline void routeFeedbackOutput(int mode, float dryL, float dryR, float procL,
                               float procR, float& outL, float& outR) noexcept {
    if (mode == 0) { outL = procL; outR = procR; }
    else { outL = 0.5f * (dryL + procL); outR = 0.5f * (dryR + procR); }
}

} // namespace trollfx
