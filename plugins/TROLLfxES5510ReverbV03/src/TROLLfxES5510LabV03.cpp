// Consolidated TROLLfx ES5510 V0.3 Force VST2 effect.
#include "mpc_plugin.h"
#include "../../../experimental/emulation/vst_host/v02/Es5510V02Processor.hpp"
#include "../../../experimental/emulation/vst_host/v03/ReverbBank.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace {
enum Param : int {
    kAlgorithm=0,kWet,kDry,kBypass,
    kTime,kFeedback,kPingPong,kSync,kTempoSource,kDivision,kInternalBpm,
    kChorusRate,kChorusDepth,
    kPhaserRate,kPhaserDepth,kPhaserCenter,kPhaserSpread,kPhaserFeedback,
    kPanPosition,kPanWidth,kPanMotion,kPanRate,kPanWaveform,kPanDirection,kPanSync,
    kRotarySpeed,kHornSpeed,kDrumSpeed,kAcceleration,kDeceleration,kRotaryWidth,kDoppler,kDrive,
    kDecay,kPredelay,kDiffusion1,kDiffusion2,kEarlyLevel,
    kGateAttack,kGateHold,kGateRelease,kTriggerThreshold,kRetriggerThreshold,kRetrigger,kDamping,
    kTap1,kTap2,kTap3,kTap4,kTap5,kTap6,kTap7,kTap8,kTap9,
    kParamCount
};
constexpr char kUid[]="TfxR";
const std::vector<std::string> kDivisions={"1/32","1/16 Triplet","1/16","1/16 Dotted","1/8 Triplet","1/8","1/8 Dotted","1/4 Triplet","1/4","1/4 Dotted","1/2 Triplet","1/2"};
const std::vector<std::string> kAlgorithms={"DELAY","CHORUS","PHASER","PANNER","ROTARY","SMALL PLATE","LARGE PLATE","ROOM","HALL","GATED","REVERSE 1","REVERSE 2","NON LIN 1","NON LIN 2","NON LIN 3"};
}

class TROLLfxES5510LabV03 final : public mpc::Plugin {
public:
    TROLLfxES5510LabV03() : mpc::Plugin(makeInfo()) {
        addParam(mpc::choiceParam("algorithm","Algorithm",kAlgorithms,0));
        addParam(mpc::floatParam("wet","Wet",0,100,65,"%",0));
        addParam(mpc::floatParam("dry","Dry",0,100,0,"%",0));
        addParam(mpc::boolParam("bypass","Bypass",false));
        addParam(mpc::logParam("delay_ms","Time",1,10000,500,"ms",1));
        addParam(mpc::floatParam("feedback","Delay Feedback",0,94,55,"%",0));
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
        addParam(mpc::floatParam("phaser_spread","Stereo Spread",-1,1,0.25f,"oct",2));
        addParam(mpc::floatParam("phaser_feedback","Phaser Feedback",-75,75,65,"%",0));
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
        addParam(mpc::floatParam("rotary_width","Rotor Width",0,100,100,"%",0));
        addParam(mpc::floatParam("doppler_depth","Doppler",0,8,2,"ms",1));
        addParam(mpc::floatParam("drive","Rotor Drive",0,100,0,"%",0));
        addParam(mpc::floatParam("decay","Decay",0.2f,20,1.8f,"s",2));
        addParam(mpc::floatParam("predelay","Pre-delay",0,500,13,"ms",1));
        addParam(mpc::floatParam("diffusion1","Diffusion 1",0,99,55,"%",0));
        addParam(mpc::floatParam("diffusion2","Diffusion 2",0,99,50,"%",0));
        addParam(mpc::floatParam("early_level","Early Level",0,99,25,"%",0));
        addParam(mpc::floatParam("gate_attack","Gate Attack",1,10000,45,"ms",1));
        addParam(mpc::floatParam("gate_hold","Gate Hold",1,10000,100,"ms",1));
        addParam(mpc::floatParam("gate_release","Gate Release",1,10000,300,"ms",1));
        addParam(mpc::floatParam("trigger_threshold","Trigger Threshold",-96,0,-30,"dB",0));
        addParam(mpc::floatParam("retrigger_threshold","Retrigger Threshold",-96,0,-28,"dB",0));
        addParam(mpc::boolParam("retrigger","Retrigger",true));
        addParam(mpc::floatParam("damping","HF Damping",0,99,35,"%",0));
        for(int i=0;i<9;++i){const char* ids[]={"tap_1","tap_2","tap_3","tap_4","tap_5","tap_6","tap_7","tap_8","tap_9"};const char* names[]={"Tap Level 1","Tap Level 2","Tap Level 3","Tap Level 4","Tap Level 5","Tap Level 6","Tap Level 7","Tap Level 8","Tap Level 9"};addParam(mpc::floatParam(ids[i],names[i],0,99,i==0?76:i==1?66:i==2?58:i==3?52:i==4?47:i==5?42:i==6?37:i==7?27:16,"%",0));}

        std::vector<float> base(kParamCount,0.0f);
        base[kWet]=65;base[kDry]=0;base[kTime]=500;base[kFeedback]=55;base[kTempoSource]=0;base[kDivision]=8;base[kInternalBpm]=120;
        base[kChorusRate]=0.5f;base[kChorusDepth]=5;base[kPhaserRate]=0.25f;base[kPhaserDepth]=2;base[kPhaserCenter]=900;base[kPhaserSpread]=0.25f;base[kPhaserFeedback]=65;
        base[kPanPosition]=0;base[kPanWidth]=100;base[kPanMotion]=80;base[kPanRate]=0.25f;base[kRotarySpeed]=1;base[kHornSpeed]=6;base[kDrumSpeed]=0.8f;
        base[kAcceleration]=1;base[kDeceleration]=1.5f;base[kRotaryWidth]=100;base[kDoppler]=2;base[kDecay]=1.8f;base[kPredelay]=13;base[kDiffusion1]=55;base[kDiffusion2]=50;base[kEarlyLevel]=25;
        base[kGateAttack]=45;base[kGateHold]=100;base[kGateRelease]=300;base[kTriggerThreshold]=-30;base[kRetriggerThreshold]=-28;base[kRetrigger]=1;
        base[kDamping]=35;base[kTap1]=76;base[kTap2]=66;base[kTap3]=58;base[kTap4]=52;base[kTap5]=47;base[kTap6]=42;base[kTap7]=37;base[kTap8]=27;base[kTap9]=16;
        const auto addPreset=[&](const char* name,int algorithm){auto values=base;values[kAlgorithm]=static_cast<float>(algorithm);addProgram(name,values);};
        addPreset("Stereo Tempo Delay",0);addPreset("Chorus",1);addPreset("Safe 12-Stage Phaser",2);addPreset("Motion Panner",3);addPreset("Dual-Band Rotary",4);
        addPreset("Small Plate",5);addPreset("Large Plate",6);addPreset("Room",7);addPreset("Hall",8);addPreset("Gated Reverb",9);addPreset("Reverse One Shot",10);addPreset("Reverse Retrigger",11);addPreset("NonLin 1 Transient",12);addPreset("NonLin 2 Bloom",13);addPreset("NonLin 3 Drum",14);
    }

    void prepare(double sampleRate,int maxBlockSize) override {
        const auto n=static_cast<std::size_t>(std::max(1,maxBlockSize));
        bool ok=base_.prepare(sampleRate,n)&&reverbs_.prepare(sampleRate);
        try{scratchL_.assign(n,0.0f);scratchR_.assign(n,0.0f);}catch(...){ok=false;}
        sampleRate_=sampleRate;fadeSamples_=fadeRemaining_=0;lastAlgorithm_=-1;prepared_.store(ok,std::memory_order_release);
    }
    void reset() override {base_.reset();fadeRemaining_=0;lastAlgorithm_=-1;}
    void process(mpc::ProcessContext& ctx) override {
        const int n=std::max(0,ctx.numFrames);if(!n||!ctx.outputs||ctx.numOutputs<2||!ctx.outputs[0]||!ctx.outputs[1])return;
        const float* left=ctx.inputs&&ctx.numInputs>0?ctx.inputs[0]:nullptr;const float* right=ctx.inputs&&ctx.numInputs>1?ctx.inputs[1]:nullptr;
        if(!prepared_.load(std::memory_order_acquire)){for(int i=0;i<n;++i){ctx.outputs[0][i]=left?left[i]:0;ctx.outputs[1][i]=right?right[i]:0;}return;}
        const int algorithm=std::clamp(getInt(kAlgorithm),0,static_cast<int>(kAlgorithms.size()-1));
        if(algorithm!=lastAlgorithm_){lastAlgorithm_=algorithm;fadeSamples_=fadeRemaining_=static_cast<std::uint32_t>(std::max(1.0,sampleRate_*0.01));}
        const std::size_t block=static_cast<std::size_t>(n);std::size_t offset=0;
        while(offset<block){const auto count=std::min(scratchL_.size(),block-offset);const float* inL=left?left+offset:nullptr;const float* inR=right?right+offset:nullptr;
            if(algorithm<5){
                using Core=trollfx::esp::vst::v02::Processor;Core::Parameters p;
                p.mode=static_cast<Core::Mode>(algorithm);p.pingPong=getBool(kPingPong);p.delayMs=get(kTime);p.feedback=get(kFeedback)*0.01f;
                p.wet=get(kWet)*0.01f;p.dry=get(kDry)*0.01f;p.sync=getBool(kSync);p.tempoSource=getInt(kTempoSource)==0?trollfx::esp::vst::TempoSource::Host:trollfx::esp::vst::TempoSource::Internal;
                p.division=static_cast<trollfx::esp::vst::Dp4Division>(std::clamp(getInt(kDivision),0,11));p.internalBpm=get(kInternalBpm);
                p.hostTempoValid=ctx.transport.tempoValid&&std::isfinite(ctx.transport.bpm)&&ctx.transport.bpm>0;p.hostBpm=ctx.transport.bpm;
                hostTempoSeen_.store(p.hostTempoValid,std::memory_order_relaxed);
                p.chorusRateHz=get(kChorusRate);p.chorusDepthMs=get(kChorusDepth);p.phaserRateHz=get(kPhaserRate);p.phaserDepthOctaves=get(kPhaserDepth);p.phaserCenterHz=get(kPhaserCenter);
                p.phaserSpreadOctaves=get(kPhaserSpread);p.phaserFeedback=get(kPhaserFeedback)*0.01f;p.panPosition=get(kPanPosition)*0.01f;p.panWidth=get(kPanWidth)*0.01f;p.panMotion=get(kPanMotion)*0.01f;
                p.panRateHz=get(kPanRate);p.panWaveform=static_cast<Core::Waveform>(std::clamp(getInt(kPanWaveform),0,2));p.reverse=getInt(kPanDirection)==1;p.panSync=getBool(kPanSync);
                p.rotarySpeed=static_cast<Core::RotarySpeed>(std::clamp(getInt(kRotarySpeed),0,2));p.hornSpeedHz=get(kHornSpeed);p.drumSpeedHz=get(kDrumSpeed);p.accelerationSeconds=get(kAcceleration);p.decelerationSeconds=get(kDeceleration);
                p.rotaryWidth=get(kRotaryWidth)*0.01f;p.dopplerDepthMs=get(kDoppler);p.drive=get(kDrive)*0.01f;p.bypass=getBool(kBypass);
                base_.processBlock(inL,inR,scratchL_.data(),scratchR_.data(),count,p);
            }else{
                using Bank=trollfx::esp::vst::v03::ReverbBank;Bank::Parameters p;
                p.wet=getBool(kBypass)?0.0f:get(kWet)*0.01f;p.dry=getBool(kBypass)?1.0f:get(kDry)*0.01f;
                p.decaySeconds=get(kDecay);p.predelayMs=get(kPredelay);p.diffusion1=get(kDiffusion1);p.diffusion2=get(kDiffusion2);p.earlyLevel=get(kEarlyLevel);
                p.attackMs=get(kGateAttack);p.holdMs=get(kGateHold);p.releaseMs=get(kGateRelease);p.triggerDb=get(kTriggerThreshold);p.retriggerDb=get(kRetriggerThreshold);p.retrigger=getBool(kRetrigger);
                p.damping=get(kDamping);p.tapLevels={get(kTap1),get(kTap2),get(kTap3),get(kTap4),get(kTap5),get(kTap6),get(kTap7),get(kTap8),get(kTap9)};
                const std::array<Bank::Mode,10> modes={Bank::Mode::SmallPlate,Bank::Mode::LargePlate,Bank::Mode::Room,Bank::Mode::Hall,Bank::Mode::Gated,Bank::Mode::ReverseOneShot,Bank::Mode::ReverseRetrigger,Bank::Mode::NonLin1,Bank::Mode::NonLin2,Bank::Mode::NonLin3};
                const auto revIndex=static_cast<std::size_t>(algorithm-5);reverbs_.processBlock(modes[std::min(revIndex,modes.size()-1)],inL,inR,scratchL_.data(),scratchR_.data(),count,p);
            }
            for(std::size_t i=0;i<count;++i){const float l=inL&&std::isfinite(inL[i])?inL[i]:0,r=inR&&std::isfinite(inR[i])?inR[i]:0;
                const float fade=fadeRemaining_?1.0f-static_cast<float>(fadeRemaining_)/static_cast<float>(fadeSamples_):1.0f;
                ctx.outputs[0][offset+i]=l*(1.0f-fade)+scratchL_[i]*fade;ctx.outputs[1][offset+i]=r*(1.0f-fade)+scratchR_[i]*fade;
                if(fadeRemaining_)--fadeRemaining_;
            }
            offset+=count;
        }
    }
    bool formatValue(int index,float plain,std::string& out) override {
        if(index==kTempoSource){out=plain<0.5f?(hostTempoSeen_.load(std::memory_order_relaxed)?"HOST":"HOST unavailable; Internal fallback"):"INTERNAL";return true;}
        return false;
    }
private:
    static mpc::PluginInfo makeInfo(){mpc::PluginInfo i;i.name="ES5510 NonLin Reverb V0.3";i.vendor="TROLLfx";i.uniqueId=MPC_FOURCC(kUid[0],kUid[1],kUid[2],kUid[3]);i.version=300;i.isInstrument=false;i.numInputs=2;i.numOutputs=2;i.tailSamples=0;return i;}
    trollfx::esp::vst::v02::Processor base_;
    trollfx::esp::vst::v03::ReverbBank reverbs_;
    std::vector<float> scratchL_,scratchR_;
    std::atomic<bool> prepared_{false},hostTempoSeen_{false};
    double sampleRate_=44100.0;
    int lastAlgorithm_=-1;
    std::uint32_t fadeSamples_=0,fadeRemaining_=0;
};

MPC_PLUGIN_ENTRY(TROLLfxES5510LabV03)
