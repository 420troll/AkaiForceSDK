#include "mpc_plugin.h"
#include "../../../experimental/emulation/mame_es5510_standalone.cpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

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
constexpr int kUnitStride = 7;
constexpr int kUnitBase = 0;
enum Param { kAlgorithm=0, kTime, kFeedback, kRate, kDepth, kMix, kBypass };

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
        bool ready = false;
    };
    static mpc::PluginInfo makeInfo() {
        mpc::PluginInfo i;
        i.name = "5510";
        i.vendor = "TROLLfx";
        i.uniqueId = MPC_FOURCC('T','5','5','1');
        i.version = 1000;
        i.isInstrument = false;
        i.numInputs = 2;
        i.numOutputs = 2;
        return i;
    }
    static int index(int unit, Param param) noexcept { return kUnitBase + unit * kUnitStride + static_cast<int>(param); }
    void configureUnit(int u, double sr) noexcept {
        auto& state = units_[static_cast<std::size_t>(u)];
        const int mode = std::clamp(getInt(index(u,kAlgorithm)), 0, 1);
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

        const float timeMs = get(index(u,kTime));
        const auto delaySamples = static_cast<std::uint32_t>(std::clamp(
            std::lround(static_cast<double>(timeMs) * sr * 0.001), 1L,
            static_cast<long>(kMaxDelaySamples)));
        state.targetDelaySamples = delaySamples;
        if (mode == 0) {
            state.cpu.setRegister(40, q22(get(index(u,kFeedback)) * 0.01f));
        } else {
            state.cpu.setRegister(40, static_cast<std::uint32_t>(1u << 22));
        }
    }
public:
    Plugin5510() : mpc::Plugin(makeInfo()) {
        using namespace mpc;
        for (int u=0; u<4; ++u) {
            const char unit = static_cast<char>('A' + u);
            const std::string suffix(1, unit);
            addParam(choiceParam(("algorithm"+suffix).c_str(), ("Unit "+suffix+" Algorithm").c_str(), {"Delay","Chorus"}, 0));
            addParam(logParam(("time"+suffix).c_str(), ("Unit "+suffix+" Time").c_str(), 1.0f, 10000.0f, 250.0f, "ms", 0));
            addParam(floatParam(("feedback"+suffix).c_str(), ("Unit "+suffix+" Feedback").c_str(), -99.0f, 99.0f, 35.0f, "%", 0));
            addParam(logParam(("rate"+suffix).c_str(), ("Unit "+suffix+" Rate").c_str(), 0.05f, 8.0f, 0.8f, "Hz", 2));
            addParam(floatParam(("depth"+suffix).c_str(), ("Unit "+suffix+" Depth").c_str(), 0.0f, 20.0f, 4.0f, "ms", 1));
            addParam(floatParam(("mix"+suffix).c_str(), ("Unit "+suffix+" Wet").c_str(), 0.0f, 100.0f, 35.0f, "%", 0));
            addParam(boolParam(("bypass"+suffix).c_str(), ("Unit "+suffix+" Bypass").c_str(), false));
        }
    }
    void prepare(double sampleRate, int) override {
        sampleRate_ = (sampleRate >= 32000.0 && sampleRate <= 96000.0) ? sampleRate : 44100.0;
        delaySmoothing_ = 1.0 - std::exp(-1.0 / (0.020 * sampleRate_));
        wetSmoothing_ = static_cast<float>(1.0 - std::exp(-1.0 / (0.010 * sampleRate_)));
        for (std::size_t i=0; i<kLfoTableSize; ++i)
            lfoTable_[i] = std::sin(6.2831853071795864769f * static_cast<float>(i) / static_cast<float>(kLfoTableSize));
        ready_ = true;
        for (auto& unit : units_) {
            unit.ready = unit.cpu.prepare();
            ready_ = ready_ && unit.ready;
            unit.loadedMode = -1;
        }
        if (ready_) for (int u=0; u<4; ++u) configureUnit(u, sampleRate_);
    }
    void reset() override {
        for (auto& unit : units_) {
            if (unit.ready) unit.cpu.reset();
            unit.phase = 0.0;
            unit.delaySamples = 1.0;
            unit.loadedMode = -1;
        }
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
        for (int u=0; u<4; ++u) configureUnit(u, sampleRate_);
        for (int frame=0; frame<ctx.numFrames; ++frame) {
            float left=inL[frame], right=inR[frame];
            for (int u=0; u<4; ++u) {
                if (getBool(index(u,kBypass))) continue;
                auto& state=units_[static_cast<std::size_t>(u)];
                const int mode=state.loadedMode;
                const float wetTarget=std::clamp(get(index(u,kMix))*0.01f,0.0f,1.0f);
                state.wet += (wetTarget-state.wet) * wetSmoothing_;
                const float wet=state.wet;
                const float dryL=left, dryR=right;
                const double delayTarget = static_cast<double>(state.targetDelaySamples);
                state.delaySamples += (delayTarget-state.delaySamples) * delaySmoothing_;
                if (mode == 1) {
                    const double depth=static_cast<double>(get(index(u,kDepth))*sampleRate_*0.001f);
                    const double center=std::clamp(state.delaySamples, depth+2.0, static_cast<double>(kMaxDelaySamples)-depth-2.0);
                    const double tablePosition=state.phase*static_cast<double>(kLfoTableSize);
                    const auto tableIndex=static_cast<std::size_t>(tablePosition) & (kLfoTableSize-1);
                    const auto nextIndex=(tableIndex+1) & (kLfoTableSize-1);
                    const float frac=static_cast<float>(tablePosition-static_cast<double>(static_cast<std::size_t>(tablePosition)));
                    const double lfo=static_cast<double>(lfoTable_[tableIndex]+(lfoTable_[nextIndex]-lfoTable_[tableIndex])*frac);
                    const double offset=lfo*depth;
                    const auto l=static_cast<std::uint32_t>(std::clamp(std::lround(center+offset),1L,static_cast<long>(kMaxDelaySamples)));
                    const auto r=static_cast<std::uint32_t>(std::clamp(std::lround(center-offset),1L,static_cast<long>(kMaxDelaySamples)));
                    state.cpu.setRegister(0,l); state.cpu.setRegister(1,kRightBank+r);
                    state.phase += static_cast<double>(get(index(u,kRate))) / sampleRate_;
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
    std::array<UnitState,4> units_{};
    std::array<float,kLfoTableSize> lfoTable_{};
    double sampleRate_=44100.0;
    double delaySmoothing_=0.0;
    float wetSmoothing_=0.0f;
    bool ready_=false;
};

MPC_PLUGIN_ENTRY(Plugin5510)
