#include "mpc_plugin.h"
#include "../../../experimental/emulation/mame_es5510_standalone.cpp"
#include "tempo_math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace {
using trollfx::esp::MameES5510;

constexpr std::array<std::uint64_t, 11> kDelayProgram = {
    0xffffef0a9000ULL, 0xffffee0b9000ULL, 0x280e000c9c30ULL,
    0x280f000d9c30ULL, 0xffff0e0a0030ULL, 0xffff0f0b0030ULL,
    0xffff0a009508ULL, 0xffff0b009508ULL, 0xffff0af19030ULL,
    0xffff0bf09030ULL, 0xfffffffff000ULL
};
constexpr std::array<std::uint64_t, 11> kChorusProgram = {
    0xffffef0a9000ULL, 0xffffee0b9000ULL, 0x280e000c9c30ULL,
    0x280f000d9c30ULL, 0xffff0a009508ULL, 0xffff0b009508ULL,
    0xffff0e0a0030ULL, 0xffff0f0b0030ULL, 0xffff0af19030ULL,
    0xffff0bf09030ULL, 0xfffffffff000ULL
};
constexpr float kQ22Scale = static_cast<float>(1u << 22);
constexpr std::uint32_t kMaxDelaySamples = 441000;
constexpr std::uint32_t kRightBank = 441000;
constexpr std::size_t kLfoTableSize = 2048;
enum Param { kAlgorithm=0, kTime, kFeedback, kRate, kDepth, kMix, kBypass,
              kTempoSync, kDivision, kFallbackBpm, kFineOffset, kStereoSpread, kSyncStatus };
std::vector<std::string> divisionChoices() {
    std::vector<std::string> result;
    result.reserve(trollfx5510::kDivisions.size());
    for (const auto& division : trollfx5510::kDivisions) result.emplace_back(division.label);
    return result;
}

std::int16_t toS16(float x) noexcept {
    if (!std::isfinite(x)) return 0;
    const float scaled = std::clamp(x, -1.0f, 0.9999695f) * 32768.0f;
    return static_cast<std::int16_t>(std::lrint(scaled));
}
float fromS16(std::int16_t x) noexcept { return static_cast<float>(x) * (1.0f / 32768.0f); }
std::uint32_t q22(float x) noexcept {
    const auto v = std::clamp(x, -1.999999f, 1.999999f) * kQ22Scale;
    return static_cast<std::uint32_t>(std::lrint(v)) & 0xffffffu;
}
}

class Plugin5510 final : public mpc::Plugin {
    struct UnitState {
        MameES5510 cpu;
        int loadedMode = -1;
        double phase = 0.0; // cycles, [0,1)
        double delaySamples = 1.0;
        std::uint32_t targetDelaySamples = 1;
        float wet = 0.35f;
        float feedback = 0.35f, targetFeedback = 0.35f;
        float rate = 0.8f, targetRate = 0.8f;
        float depthMs = 4.0f, targetDepthMs = 4.0f;
        float spread = 100.0f, targetSpread = 100.0f;
        bool syncOutOfRange = false;
        bool ready = false;
    };
    static mpc::PluginInfo makeInfo() {
        mpc::PluginInfo i;
        i.name = "5510 Single";
        i.vendor = "TROLLfx";
        i.uniqueId = MPC_FOURCC('T','5','5','S');
        i.version = 2000;
        i.isInstrument = false;
        i.numInputs = 2;
        i.numOutputs = 2;
        return i;
    }
    void configureUnit(int u, double sr) noexcept {
        auto& state = units_[static_cast<std::size_t>(u)];
        const int mode = std::clamp(getInt(kAlgorithm), 0, 1);
        if (!state.ready || mode != state.loadedMode) {
            state.cpu.resetExecutionState();
            const bool loaded = mode == 0
                ? state.cpu.loadProgram(kDelayProgram.data(), kDelayProgram.size())
                : state.cpu.loadProgram(kChorusProgram.data(), kChorusProgram.size());
            state.ready = state.ready && loaded;
            state.loadedMode = mode;
            state.phase = 0.0;
            state.delaySamples = 1.0;
            state.wet = 0.35f;
            state.feedback = state.targetFeedback = mode == 0 ? std::clamp(get(kFeedback) * 0.01f, -0.99f, 0.99f) : 1.0f;
            state.rate = state.targetRate = get(kRate);
            state.depthMs = state.targetDepthMs = get(kDepth);
            state.spread = state.targetSpread = get(kStereoSpread);
            state.cpu.setHostSerial(0x20);
            state.cpu.setRegister(244, 0); // MEMSIZ: sample-addressed DRAM.
            state.cpu.setRegister(245, 2u * kMaxDelaySamples - 1u);
            state.cpu.setRegister(248, 0);
            state.cpu.setRegister(6, 0);
            state.cpu.setRegister(7, kRightBank);
            state.cpu.setRegister(4, 0);
            state.cpu.setRegister(5, kRightBank);
            if (mode == 0) state.cpu.setRegister(40, q22(0.55f));
            else state.cpu.setRegister(40, static_cast<std::uint32_t>(1u << 22));
        }

        const float timeMs = get(kTime);
        const auto delaySamples = static_cast<std::uint32_t>(std::clamp(
            std::lround(static_cast<double>(timeMs) * sr * 0.001), 1L,
            static_cast<long>(kMaxDelaySamples)));
        state.targetDelaySamples = delaySamples;
        state.targetFeedback = mode == 0 ? std::clamp(get(kFeedback) * 0.01f, -0.99f, 0.99f) : 1.0f;
        state.targetRate = get(kRate);
        state.targetDepthMs = get(kDepth);
        state.targetSpread = get(kStereoSpread);
    }
    void updateDelayTarget(const mpc::ProcessContext& ctx) noexcept {
        auto& state = units_[0];
        if (state.loadedMode != 0) { state.syncOutOfRange = false; return; }
        if (!getBool(kTempoSync)) {
            const auto samples = static_cast<std::uint32_t>(std::clamp(
                std::lround(static_cast<double>(get(kTime)) * sampleRate_ * 0.001), 1L,
                static_cast<long>(kMaxDelaySamples)));
            state.targetDelaySamples = samples;
            state.syncOutOfRange = false;
        } else {
            const double bpm = ctx.transport.tempoValid && std::isfinite(ctx.transport.bpm) && ctx.transport.bpm > 0.0
                ? ctx.transport.bpm : static_cast<double>(get(kFallbackBpm));
            std::uint32_t samples = 0;
            const bool valid = trollfx5510::delaySamples(bpm,
                static_cast<std::size_t>(std::clamp(getInt(kDivision), 0, static_cast<int>(trollfx5510::kDivisions.size()-1))),
                ctx.transport.timeSigNumerator, ctx.transport.timeSigDenominator,
                static_cast<double>(get(kFineOffset)), sampleRate_, kMaxDelaySamples, samples);
            state.syncOutOfRange = !valid;
            if (valid) state.targetDelaySamples = samples;
        }
        const int status = state.syncOutOfRange ? 1 : 0;
        if (getInt(kSyncStatus) != status) set(kSyncStatus, static_cast<float>(status));
    }
public:
    Plugin5510() : mpc::Plugin(makeInfo()) {
        using namespace mpc;
        for (int u=0; u<1; ++u) {
            const char unit = static_cast<char>('A' + u);
            const std::string suffix(1, unit);
            addParam(choiceParam(("algorithm"+suffix).c_str(), ("Unit "+suffix+" Algorithm").c_str(), {"TROLLfx Delay","TROLLfx Chorus"}, 0));
            addParam(logParam(("time"+suffix).c_str(), ("Unit "+suffix+" Time").c_str(), 1.0f, 10000.0f, 250.0f, "ms", 0));
            addParam(floatParam(("feedback"+suffix).c_str(), ("Unit "+suffix+" Feedback").c_str(), -99.0f, 99.0f, 35.0f, "%", 0));
            addParam(logParam(("rate"+suffix).c_str(), ("Unit "+suffix+" Rate").c_str(), 0.05f, 8.0f, 0.8f, "Hz", 2));
            addParam(floatParam(("depth"+suffix).c_str(), ("Unit "+suffix+" Depth").c_str(), 0.0f, 20.0f, 4.0f, "ms", 1));
            addParam(floatParam(("mix"+suffix).c_str(), ("Unit "+suffix+" Wet").c_str(), 0.0f, 100.0f, 35.0f, "%", 0));
            addParam(boolParam(("bypass"+suffix).c_str(), ("Unit "+suffix+" Bypass").c_str(), false));
        }
        addParam(boolParam("tempoSyncA", "Tempo Sync", false));
        addParam(choiceParam("divisionA", "Beat Division", divisionChoices(), 9));
        addParam(floatParam("fallbackBpmA", "Fallback BPM", 40.0f, 240.0f, 120.0f, "BPM", 0));
        addParam(floatParam("fineOffsetA", "Fine Offset", -250.0f, 250.0f, 0.0f, "ms", 0));
        addParam(floatParam("stereoSpreadA", "Stereo Spread", 0.0f, 100.0f, 100.0f, "%", 0));
        addParam(choiceParam("syncStatusA", "Sync Range", {"Ready", "Out of Range"}, 0));
    }
    void prepare(double sampleRate, int) override {
        sampleRate_ = (sampleRate >= 32000.0 && sampleRate <= 96000.0) ? sampleRate : 44100.0;
        delaySmoothing_ = 1.0 - std::exp(-1.0 / (0.020 * sampleRate_));
        wetSmoothing_ = static_cast<float>(1.0 - std::exp(-1.0 / (0.010 * sampleRate_)));
        controlSmoothing_ = static_cast<float>(1.0 - std::exp(-1.0 / (0.015 * sampleRate_)));
        for (std::size_t i=0; i<kLfoTableSize; ++i)
            lfoTable_[i] = std::sin(6.2831853071795864769f * static_cast<float>(i) / static_cast<float>(kLfoTableSize));
        ready_ = true;
        for (auto& unit : units_) {
            unit.ready = unit.cpu.prepare();
            ready_ = ready_ && unit.ready;
            unit.loadedMode = -1;
        }
        if (ready_) for (int u=0; u<1; ++u) configureUnit(u, sampleRate_);
    }
    void reset() override {
        for (auto& unit : units_) {
            if (unit.ready) unit.cpu.reset();
            unit.phase = 0.0;
            unit.delaySamples = 1.0;
            unit.loadedMode = -1;
        }
    }
    bool formatValue(int param, float value, std::string& out) override {
        char buffer[48];
        if (param == kFeedback) {
            if (value == 0.0f) std::snprintf(buffer, sizeof(buffer), "0%%");
            else std::snprintf(buffer, sizeof(buffer), "%+.1f%%", static_cast<double>(value));
            out = buffer; return true;
        }
        if (param == kTime) { std::snprintf(buffer, sizeof(buffer), "%.1f ms", static_cast<double>(value)); out = buffer; return true; }
        if (param == kFallbackBpm) { std::snprintf(buffer, sizeof(buffer), "%.1f BPM", static_cast<double>(value)); out = buffer; return true; }
        if (param == kFineOffset) { std::snprintf(buffer, sizeof(buffer), "%+.1f ms", static_cast<double>(value)); out = buffer; return true; }
        if (param == kRate) { std::snprintf(buffer, sizeof(buffer), "%.2f Hz", static_cast<double>(value)); out = buffer; return true; }
        if (param == kDepth) { std::snprintf(buffer, sizeof(buffer), "%.1f ms", static_cast<double>(value)); out = buffer; return true; }
        return false;
    }
    void process(mpc::ProcessContext& ctx) override {
        const float* inL = ctx.inputs[0];
        const float* inR = ctx.numInputs > 1 ? ctx.inputs[1] : ctx.inputs[0];
        float* outL = ctx.outputs[0];
        float* outR = ctx.numOutputs > 1 ? ctx.outputs[1] : ctx.outputs[0];
        if (!ready_) {
            for (int i=0; i<ctx.numFrames; ++i) { outL[i]=inL[i]; outR[i]=inR[i]; }
            return;
        }
        for (int u=0; u<1; ++u) configureUnit(u, sampleRate_);
        updateDelayTarget(ctx);
        for (int frame=0; frame<ctx.numFrames; ++frame) {
            float left=inL[frame], right=inR[frame];
            for (int u=0; u<1; ++u) {
                auto& state=units_[static_cast<std::size_t>(u)];
                if (state.syncOutOfRange) continue;
                const int mode=state.loadedMode;
                state.feedback += (state.targetFeedback-state.feedback) * controlSmoothing_;
                state.rate += (state.targetRate-state.rate) * controlSmoothing_;
                state.depthMs += (state.targetDepthMs-state.depthMs) * controlSmoothing_;
                state.spread += (state.targetSpread-state.spread) * controlSmoothing_;
                state.cpu.setRegister(40, q22(mode == 0 ? state.feedback : 1.0f));
                const float wetTarget=getBool(kBypass) ? 0.0f : std::clamp(get(kMix)*0.01f,0.0f,1.0f);
                state.wet += (wetTarget-state.wet) * wetSmoothing_;
                const float wet=state.wet;
                const float dryL=left, dryR=right;
                const double delayTarget = static_cast<double>(state.targetDelaySamples);
                state.delaySamples += (delayTarget-state.delaySamples) * delaySmoothing_;
                if (mode == 1) {
                    const double depth=static_cast<double>(state.depthMs*sampleRate_*0.001f);
                    const double center=std::clamp(state.delaySamples, depth+2.0, static_cast<double>(kMaxDelaySamples)-depth-2.0);
                    const double tablePosition=state.phase*static_cast<double>(kLfoTableSize);
                    const auto tableIndex=static_cast<std::size_t>(tablePosition) & (kLfoTableSize-1);
                    const auto nextIndex=(tableIndex+1) & (kLfoTableSize-1);
                    const float frac=static_cast<float>(tablePosition-static_cast<double>(static_cast<std::size_t>(tablePosition)));
                    const double lfo=static_cast<double>(lfoTable_[tableIndex]+(lfoTable_[nextIndex]-lfoTable_[tableIndex])*frac);
                    const double offset=lfo*depth;
                    const double spreadCycles=static_cast<double>(state.spread)*0.005;
                    const double rightPosition=tablePosition+spreadCycles*static_cast<double>(kLfoTableSize);
                    const auto rightIndex=static_cast<std::size_t>(rightPosition) & (kLfoTableSize-1);
                    const auto rightNext=(rightIndex+1) & (kLfoTableSize-1);
                    const float rightFrac=static_cast<float>(rightPosition-static_cast<double>(static_cast<std::size_t>(rightPosition)));
                    const double rightLfo=static_cast<double>(lfoTable_[rightIndex]+(lfoTable_[rightNext]-lfoTable_[rightIndex])*rightFrac);
                    const auto l=static_cast<std::uint32_t>(std::clamp(std::lround(center+offset),1L,static_cast<long>(kMaxDelaySamples)));
                    const auto r=static_cast<std::uint32_t>(std::clamp(std::lround(center+rightLfo*depth),1L,static_cast<long>(kMaxDelaySamples)));
                    state.cpu.setRegister(0,l); state.cpu.setRegister(1,kRightBank+r);
                    state.phase += static_cast<double>(state.rate) / sampleRate_;
                    if(state.phase >= 1.0) state.phase -= 1.0;
                } else {
                    const auto delay=static_cast<std::uint32_t>(std::clamp(std::lround(state.delaySamples),1L,static_cast<long>(kMaxDelaySamples)));
                    state.cpu.setRegister(0,delay); state.cpu.setRegister(1,kRightBank+delay);
                }
                std::array<std::int16_t,6> input{};
                input[4]=toS16(left); input[5]=toS16(right);
                std::array<std::int16_t,2> output{};
                state.cpu.processFrame(input,output);
                left=dryL*(1.0f-wet)+fromS16(output[0])*wet;
                right=dryR*(1.0f-wet)+fromS16(output[1])*wet;
            }
            outL[frame]=left; outR[frame]=right;
        }
    }
private:
    std::array<UnitState,1> units_{};
    std::array<float,kLfoTableSize> lfoTable_{};
    double sampleRate_=44100.0;
    double delaySmoothing_=0.0;
    float wetSmoothing_=0.0f;
    float controlSmoothing_=0.0f;
    bool ready_=false;
};

MPC_PLUGIN_ENTRY(Plugin5510)
