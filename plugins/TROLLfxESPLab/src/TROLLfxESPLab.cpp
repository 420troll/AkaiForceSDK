// StereoDelay.cpp - tempo-synced / free stereo delay with filtered feedback and ping-pong.
//
// SPDX-License-Identifier: MIT
//
// What this example demonstrates:
//   [1] an EFFECT (kind "effect", 2 in / 2 out, no MIDI)
//   [2] the Sync On/Off + Division + free Time pattern, using ctx.transport.bpm
//   [3] allocating delay memory ONLY in prepare() (DelayLine::prepare)
//   [4] smoothing the delay time so changes glide instead of click (tape-style pitch wobble)
//   [5] filters and a soft clipper inside the feedback loop so it can never blow up
//   [6] ping-pong routing (L feeds R, R feeds L)
//
// Signal path:
//   in -> [+ feedback] -> delay -> lowcut -> highcut -> softclip -> (wet, and back to feedback)
//   out = dry * (1 - mix) + wet * mix

#include "mpc_plugin.h"
#include "../../../src/original/fractional_delay.hpp"

class StereoDelay : public mpc::Plugin
{
public:
    enum Params { kTime, kDepth, kRate, kFeedback, kCross, kMode, kMix, kNumParams };

    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo i;
        i.name = "ESP Lab";
        i.vendor = "TROLLfx";
        i.uniqueId = MPC_FOURCC('T', 'f', 'x', 'E');
        i.version = 1000;
        i.isInstrument = false;   // [1] effect
        i.numInputs = 2;
        i.numOutputs = 2;
        i.tailSamples = 0;        // unknown (depends on feedback)
        return i;
    }

    StereoDelay() : mpc::Plugin(makeInfo())
    {
        using namespace mpc;
        addParam(logParam("time", "Time", 1.0f, 900.0f, 18.0f, "ms", 0));
        addParam(floatParam("depth", "Depth", 0.0f, 100.0f, 4.0f, "ms", 1));
        addParam(logParam("rate", "Rate", 0.01f, 8.0f, 0.35f, "Hz", 2));
        addParam(floatParam("feedback", "Feedback", -92.0f, 92.0f, 20.0f, "%", 0));
        addParam(boolParam("cross", "Cross Feedback", false));
        addParam(choiceParam("mode", "Mode", {"Delay", "Chorus", "Flanger"}, 1));
        addParam(floatParam("mix", "Mix", 0.0f, 100.0f, 50.0f, "%", 0));
    }

    void prepare(double sampleRate, int /*maxBlockSize*/) override
    {
        // [3] The only allocation in the plugin: room for kMaxDelaySeconds at this sample rate.
        dsp_.prepare(static_cast<float>(sampleRate), 1.0f);
        sampleRate_ = static_cast<float>(sampleRate);
    }

    void reset() override
    {
        dsp_.reset();
    }

    void process(mpc::ProcessContext& ctx) override
    {
        trollfx::ModDelayParams p;
        p.baseMs=get(kTime); p.depthMs=get(kDepth); p.rateHz=get(kRate);
        p.feedback=get(kFeedback)*0.01f; p.mix=get(kMix)*0.01f;
        dsp_.setParams(p);
        dsp_.process(ctx.inputs[0], ctx.numInputs > 1 ? ctx.inputs[1] : ctx.inputs[0],
                     ctx.outputs[0], ctx.outputs[1], static_cast<std::size_t>(ctx.numFrames),
                     getBool(kCross), getInt(kMode)==2);
    }

private:
    trollfx::ModDelay dsp_;
    float sampleRate_ = 44100.0f;
};

MPC_PLUGIN_ENTRY(StereoDelay)
