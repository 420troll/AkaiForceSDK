// TROLLfx - VST - ES5510 Experimental
// Audio is processed by the standalone MAME-derived ES5510 instruction core.

#include "mpc_plugin.h"
#include "../../../experimental/emulation/vst_host/Es5510EffectProcessor.hpp"

#include <cmath>
#include <cstdint>

namespace {

enum ParameterIndex : int
{
    kMode = 0,
    kTime,
    kFeedback,
    kWet,
    kDry,
    kRate,
    kDepth,
    kBypass,
    kParameterCount
};

constexpr char kUid[] = "TfxX";

} // namespace

class TROLLfxES5510Experimental final : public mpc::Plugin
{
public:
    TROLLfxES5510Experimental() : mpc::Plugin(makeInfo())
    {
        // Parameter IDs are persisted in host state. Keep IDs and order stable.
        addParam(mpc::choiceParam("mode", "Mode", {"DELAY", "CHORUS"}, 0));
        addParam(mpc::logParam("time_ms", "Time", 1.0f, 10000.0f, 500.0f, "ms", 1));
        addParam(mpc::floatParam("feedback", "Feedback", 0.0f, 94.0f, 55.0f, "%", 0));
        addParam(mpc::floatParam("wet", "Wet", 0.0f, 100.0f, 50.0f, "%", 0));
        addParam(mpc::floatParam("dry", "Dry", 0.0f, 100.0f, 50.0f, "%", 0));
        addParam(mpc::logParam("rate_hz", "Rate", 0.05f, 10.0f, 0.5f, "Hz", 2));
        addParam(mpc::floatParam("depth_ms", "Depth", 0.0f, 20.0f, 5.0f, "ms", 1));
        addParam(mpc::boolParam("bypass", "Bypass", false));

        addProgram("Delay",   {0.0f, 500.0f, 55.0f, 50.0f, 50.0f, 0.5f, 5.0f, 0.0f});
        addProgram("Chorus",  {1.0f, 500.0f, 0.0f, 50.0f, 50.0f, 0.5f, 5.0f, 0.0f});
    }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        (void)processor_.prepare(sampleRate, static_cast<size_t>(maxBlockSize > 0 ? maxBlockSize : 1));
    }

    void reset() override { processor_.reset(); }

    void process(mpc::ProcessContext& ctx) override
    {
        const int frames = ctx.numFrames > 0 ? ctx.numFrames : 0;
        if (frames == 0 || ctx.outputs == nullptr || ctx.numOutputs < 2 ||
            ctx.outputs[0] == nullptr || ctx.outputs[1] == nullptr)
            return;

        // Load the host's atomic parameters once per block. No heap work, locks,
        // filesystem access, or logging occurs on this callback path.
        es5510::Parameters p;
        p.mode = getInt(kMode) == 0 ? es5510::Mode::Delay : es5510::Mode::Chorus;
        p.delayMs = get(kTime);
        p.feedback = get(kFeedback) * 0.01f;
        p.wet = get(kWet) * 0.01f;
        p.dry = get(kDry) * 0.01f;
        p.rateHz = get(kRate);
        p.depthMs = get(kDepth);
        p.bypass = getBool(kBypass);

        const float* inL = (ctx.inputs && ctx.numInputs > 0 && ctx.inputs[0]) ? ctx.inputs[0] : nullptr;
        const float* inR = (ctx.inputs && ctx.numInputs > 1 && ctx.inputs[1]) ? ctx.inputs[1] : nullptr;
        // Missing input channels are represented by silence. The processor
        // accepts nullable inputs and supports in-place input/output buffers.
        processor_.processBlock(inL, inR, ctx.outputs[0], ctx.outputs[1],
                                static_cast<size_t>(frames), p);
    }

private:
    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo info;
        info.name = "ES5510 Experimental";
        info.vendor = "TROLLfx";
        info.uniqueId = MPC_FOURCC(kUid[0], kUid[1], kUid[2], kUid[3]);
        info.version = 100;
        info.isInstrument = false;
        info.numInputs = 2;
        info.numOutputs = 2;
        info.tailSamples = 0; // the emulated feedback tail is variable-length
        return info;
    }

    es5510::Es5510EffectProcessor processor_;
};

MPC_PLUGIN_ENTRY(TROLLfxES5510Experimental)
