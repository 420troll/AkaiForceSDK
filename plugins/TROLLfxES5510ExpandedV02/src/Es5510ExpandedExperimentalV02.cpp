// TROLLfx - VST - ES5510 Expanded Experimental V0.2
#include "mpc_plugin.h"
#include "../../../experimental/emulation/vst_host/v02/Es5510V02Processor.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>

namespace {
enum Param : int {
  kMode=0,kTime,kFeedback,kWet,kDry,kPingPong,kSync,kTempoSource,kDivision,kInternalBpm,
  kChorusRate,kChorusDepth,kPhaserRate,kPhaserDepth,kPhaserCenter,kPhaserSpread,kPhaserFeedback,
  kPanPosition,kPanWidth,kPanMotion,kPanRate,kPanWaveform,kPanDirection,kPanSync,
  kRotarySpeed,kHornSpeed,kDrumSpeed,kAcceleration,kDeceleration,kRotaryWidth,kDoppler,kDrive,kBypass,
  kParamCount
};
constexpr char kUid[]="TfxV";
const std::vector<std::string> kDivisions={"1/32","1/16 Triplet","1/16","1/16 Dotted","1/8 Triplet","1/8","1/8 Dotted","1/4 Triplet","1/4","1/4 Dotted","1/2 Triplet","1/2"};
}

class TROLLfxES5510ExpandedV02 final:public mpc::Plugin {
public:
  TROLLfxES5510ExpandedV02():mpc::Plugin(makeInfo()){
    addParam(mpc::choiceParam("algorithm","Algorithm",{"DELAY","CHORUS","PHASER","PANNER","ROTARY"},0));
    addParam(mpc::logParam("delay_ms","Time",1,10000,500,"ms",1));
    addParam(mpc::floatParam("feedback","Delay Feedback",0,94,55,"%",0));
    addParam(mpc::floatParam("wet","Wet",0,100,50,"%",0));
    addParam(mpc::floatParam("dry","Dry",0,100,50,"%",0));
    addParam(mpc::boolParam("ping_pong","Ping-Pong",false));
    addParam(mpc::boolParam("delay_sync","Sync",false));
    addParam(mpc::choiceParam("tempo_source","Tempo Source",{"HOST","INTERNAL"},0));
    addParam(mpc::choiceParam("division","Division",kDivisions,8));
    addParam(mpc::floatParam("internal_bpm","Internal BPM",50,250,120,"BPM",0));
    addParam(mpc::logParam("chorus_rate","Chorus Rate",0.05f,10,0.5f,"Hz",2));
    addParam(mpc::floatParam("chorus_depth","Chorus Depth",0,20,5,"ms",1));
    addParam(mpc::logParam("phaser_rate","Phaser Rate",0.01f,10,0.25f,"Hz",2));
    addParam(mpc::floatParam("phaser_depth","Phaser Depth",0,4,2,"oct",1));
    addParam(mpc::logParam("phaser_center","Center",30,16000,900,"Hz",0));
    addParam(mpc::floatParam("phaser_spread","Stereo Spread",0,1,0.25f,"oct",2));
    addParam(mpc::floatParam("phaser_feedback","Phaser Feedback",-90,90,65,"%",0));
    addParam(mpc::floatParam("pan_position","Position",-100,100,0,"%",0));
    addParam(mpc::floatParam("pan_width","Width",0,150,100,"%",0));
    addParam(mpc::floatParam("pan_motion","Motion",0,100,80,"%",0));
    addParam(mpc::logParam("pan_rate","Pan Rate",0.02f,10,0.25f,"Hz",2));
    addParam(mpc::choiceParam("pan_waveform","Waveform",{"SINE","TRIANGLE","SQUARE"},0));
    addParam(mpc::choiceParam("pan_direction","Direction",{"FORWARD","REVERSE"},0));
    addParam(mpc::boolParam("pan_sync","Clock Sync",false));
    addParam(mpc::choiceParam("rotary_speed","Speed",{"STOP","SLOW","FAST"},1));
    addParam(mpc::logParam("horn_speed","Horn Speed",0.1f,12,6,"Hz",2));
    addParam(mpc::logParam("drum_speed","Drum Speed",0.05f,4,0.8f,"Hz",2));
    addParam(mpc::floatParam("acceleration","Acceleration",0.01f,5,1,"s",2));
    addParam(mpc::floatParam("deceleration","Deceleration",0.01f,5,1.5f,"s",2));
    addParam(mpc::floatParam("rotary_width","Stereo Width",0,100,100,"%",0));
    addParam(mpc::floatParam("doppler_depth","Doppler",0,8,2,"ms",1));
    addParam(mpc::floatParam("drive","Input Drive",0,100,0,"%",0));
    addParam(mpc::boolParam("bypass","Bypass",false));

    const std::vector<float> base={0,500,55,50,50,0,0,0,8,120,0.5f,5,0.25f,2,900,0.25f,65,0,100,80,0.25f,0,0,0,1,6,0.8f,1,1.5f,100,2,0,0};
    auto preset=[&](const char* name,int mode){auto v=base;v[kMode]=static_cast<float>(mode);addProgram(name,v);};
    preset("Stereo Delay",0);preset("Chorus",1);preset("12-Stage Phaser",2);preset("Motion Panner",3);preset("Dual-Band Rotary",4);
  }

  void prepare(double sampleRate,int maxBlockSize)override{
    const bool ok=processor_.prepare(sampleRate,static_cast<std::size_t>(maxBlockSize>0?maxBlockSize:1));prepared_.store(ok,std::memory_order_release);
  }
  void reset()override{processor_.reset();}
  void process(mpc::ProcessContext& ctx)override{
    const int n=std::max(0,ctx.numFrames);if(!n||!ctx.outputs||ctx.numOutputs<2||!ctx.outputs[0]||!ctx.outputs[1])return;
    using Engine=trollfx::esp::vst::v02::Processor;
    Engine::Parameters p;
    p.mode=static_cast<Engine::Mode>(std::clamp(getInt(kMode),0,4));
    p.pingPong=getBool(kPingPong);p.delayMs=get(kTime);p.feedback=get(kFeedback)*0.01f;
    p.wet=get(kWet)*0.01f;p.dry=get(kDry)*0.01f;p.sync=getBool(kSync);
    p.tempoSource=getInt(kTempoSource)==0?trollfx::esp::vst::TempoSource::Host:trollfx::esp::vst::TempoSource::Internal;
    p.division=static_cast<trollfx::esp::vst::Dp4Division>(std::clamp(getInt(kDivision),0,11));
    p.internalBpm=get(kInternalBpm);p.hostTempoValid=ctx.transport.tempoValid&&std::isfinite(ctx.transport.bpm)&&ctx.transport.bpm>0.0;
    p.hostBpm=ctx.transport.bpm;hostTempoSeen_.store(p.hostTempoValid,std::memory_order_relaxed);
    p.chorusRateHz=get(kChorusRate);p.chorusDepthMs=get(kChorusDepth);
    p.phaserRateHz=get(kPhaserRate);p.phaserDepthOctaves=get(kPhaserDepth);p.phaserCenterHz=get(kPhaserCenter);
    p.phaserSpreadOctaves=get(kPhaserSpread);p.phaserFeedback=get(kPhaserFeedback)*0.01f;
    p.panPosition=get(kPanPosition)*0.01f;p.panWidth=get(kPanWidth)*0.01f;p.panMotion=get(kPanMotion)*0.01f;
    p.panRateHz=get(kPanRate);p.panWaveform=static_cast<Engine::Waveform>(std::clamp(getInt(kPanWaveform),0,2));
    p.reverse=getInt(kPanDirection)==1;p.panSync=getBool(kPanSync);
    p.rotarySpeed=static_cast<Engine::RotarySpeed>(std::clamp(getInt(kRotarySpeed),0,2));
    p.hornSpeedHz=get(kHornSpeed);p.drumSpeedHz=get(kDrumSpeed);p.accelerationSeconds=get(kAcceleration);p.decelerationSeconds=get(kDeceleration);
    p.rotaryWidth=get(kRotaryWidth)*0.01f;p.dopplerDepthMs=get(kDoppler);p.drive=get(kDrive)*0.01f;p.bypass=getBool(kBypass);
    const float* left=ctx.inputs&&ctx.numInputs>0?ctx.inputs[0]:nullptr;
    const float* right=ctx.inputs&&ctx.numInputs>1?ctx.inputs[1]:nullptr;
    processor_.processBlock(left,right,ctx.outputs[0],ctx.outputs[1],static_cast<std::size_t>(n),p);
  }

  bool formatValue(int index,float plainValue,std::string& out)override{
    if(index==kTempoSource){
      if(plainValue<0.5f)out=hostTempoSeen_.load(std::memory_order_relaxed)?"HOST":"HOST unavailable; Internal fallback";
      else out="INTERNAL";
      return true;
    }
    return false;
  }

private:
  static mpc::PluginInfo makeInfo(){mpc::PluginInfo i;i.name="ES5510 Expanded V0.2";i.vendor="TROLLfx";i.uniqueId=MPC_FOURCC(kUid[0],kUid[1],kUid[2],kUid[3]);i.version=200;i.isInstrument=false;i.numInputs=2;i.numOutputs=2;i.tailSamples=0;return i;}
  trollfx::esp::vst::v02::Processor processor_;
  std::atomic<bool> hostTempoSeen_{false},prepared_{false};
};

MPC_PLUGIN_ENTRY(TROLLfxES5510ExpandedV02)
