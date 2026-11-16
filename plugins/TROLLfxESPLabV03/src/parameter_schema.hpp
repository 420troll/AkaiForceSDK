#pragma once

#include <cstdint>

namespace esp_lab {

enum class ParamKind : std::uint8_t { Linear, Logarithmic, Boolean };
enum class Evidence : std::uint8_t { VerifiedBehavior, TROLLfxExtension, Unresolved };

struct ParameterSpec {
    const char* semanticId;
    const char* stableStem;
    const char* displayName;
    const char* unit;
    ParamKind kind;
    float minimum;
    float maximum;
    float defaultValue;
    std::uint16_t algorithmMask;
    Evidence evidence;
    int sourcePage;
    int telebrainTable;
    int telebrainOffset;
};

constexpr std::uint16_t algorithmBit(unsigned index) {
    return static_cast<std::uint16_t>(1u << index);
}

// ESPLabV03 appends these parameters after the original 146 host parameters.
// The first 146 indices/IDs remain stable for existing sessions and presets.
inline constexpr ParameterSpec kAlgorithmSpecificParameters[] = {
    {"distortion_input", "distIn", "Distortion Input", "dB", ParamKind::Linear, -24, 24, 0, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"distortion_output", "distOut", "Distortion Output", "dB", ParamKind::Linear, -24, 24, 0, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"pre_vcf_cutoff", "preVcfCutoff", "Pre VCF Cutoff", "Hz", ParamKind::Logarithmic, 80, 10000, 1800, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"pre_vcf_q", "preVcfQ", "Pre VCF Q", "", ParamKind::Logarithmic, 0.1f, 10, 0.7f, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"pre_vcf_envelope_depth", "preVcfEnvDepth", "Pre VCF Env Depth", "oct", ParamKind::Linear, -4, 4, 0, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"post_vcf_cutoff", "postVcfCutoff", "Post VCF Cutoff", "Hz", ParamKind::Logarithmic, 80, 10000, 1800, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"post_vcf_q", "postVcfQ", "Post VCF Q", "", ParamKind::Logarithmic, 0.1f, 10, 0.7f, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"post_vcf_envelope_depth", "postVcfEnvDepth", "Post VCF Env Depth", "oct", ParamKind::Linear, -4, 4, 0, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"envelope_attack", "envAttack", "Envelope Attack", "ms", ParamKind::Logarithmic, 0.1f, 1000, 8, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"envelope_release", "envRelease", "Envelope Release", "ms", ParamKind::Logarithmic, 1, 5000, 120, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},
    {"distortion_bypass", "distBypass", "Distortion Bypass", "", ParamKind::Boolean, 0, 1, 0, algorithmBit(7), Evidence::TROLLfxExtension, -1, -1, -1},

    {"compressor_gain", "compGain", "Compressor Gain", "dB", ParamKind::Linear, -48, 48, 0, algorithmBit(9), Evidence::VerifiedBehavior, 114, -1, -1},
    {"gate_off_threshold", "gateOff", "Gate Off Threshold", "dB", ParamKind::Linear, -96, 0, -60, algorithmBit(9), Evidence::VerifiedBehavior, 76, -1, -1},
    {"gate_on_threshold", "gateOn", "Gate On Threshold", "dB", ParamKind::Linear, -96, 0, -45, algorithmBit(9), Evidence::VerifiedBehavior, 76, -1, -1},
    {"gate_release", "gateRelease", "Gate Release", "ms", ParamKind::Logarithmic, 1, 10000, 100, algorithmBit(9), Evidence::VerifiedBehavior, 114, -1, -1},

    {"panner_phase_opposed", "pannerPhase", "Panner Phase Opposed", "", ParamKind::Boolean, 0, 1, 0, algorithmBit(8), Evidence::TROLLfxExtension, -1, -1, -1},
    {"sample_hold_rate", "sampleHold", "Sample Hold Rate", "Hz", ParamKind::Logarithmic, 0.1f, 30, 4, algorithmBit(8), Evidence::TROLLfxExtension, -1, -1, -1},
    {"left_delay", "leftDelay", "Left Delay", "ms", ParamKind::Linear, 0, 1500, 250, algorithmBit(8), Evidence::VerifiedBehavior, 97, -1, -1},
    {"right_delay", "rightDelay", "Right Delay", "ms", ParamKind::Linear, 0, 1500, 375, algorithmBit(8), Evidence::VerifiedBehavior, 97, -1, -1},
    {"left_echo", "leftEcho", "Left Echo", "ms", ParamKind::Linear, 0, 1500, 500, algorithmBit(8), Evidence::VerifiedBehavior, 97, -1, -1},
    {"right_echo", "rightEcho", "Right Echo", "ms", ParamKind::Linear, 0, 1500, 750, algorithmBit(8), Evidence::VerifiedBehavior, 97, -1, -1},
    {"echo_level", "echoLevel", "Echo Level", "%", ParamKind::Linear, 0, 100, 50, algorithmBit(8), Evidence::TROLLfxExtension, -1, -1, -1},
    {"bass_frequency", "bassFc", "Bass Frequency", "Hz", ParamKind::Logarithmic, 20, 20000, 250, algorithmBit(8), Evidence::TROLLfxExtension, -1, -1, -1},
    {"bass_gain", "bassGain", "Bass Gain", "dB", ParamKind::Linear, -18, 18, 0, algorithmBit(8), Evidence::TROLLfxExtension, -1, -1, -1},
    {"treble_frequency", "trebleFc", "Treble Frequency", "Hz", ParamKind::Logarithmic, 1000, 16000, 6000, algorithmBit(8), Evidence::VerifiedBehavior, 98, -1, -1},
    {"treble_gain", "trebleGain", "Treble Gain", "dB", ParamKind::Linear, -18, 18, 0, algorithmBit(8), Evidence::TROLLfxExtension, -1, -1, -1},
    {"input_trim", "inputTrim", "Input Trim", "dB", ParamKind::Linear, -24, 24, 0, algorithmBit(8), Evidence::TROLLfxExtension, -1, -1, -1},
};

inline constexpr unsigned kAlgorithmSpecificParameterCount =
    sizeof(kAlgorithmSpecificParameters) / sizeof(kAlgorithmSpecificParameters[0]);
static_assert(kAlgorithmSpecificParameterCount == 27, "appended parameter count changed; update wrapper index accounting and schema");

} // namespace esp_lab
