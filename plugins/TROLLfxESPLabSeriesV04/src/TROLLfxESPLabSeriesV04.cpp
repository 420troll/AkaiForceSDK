#include "mpc_plugin.h"
#include "../../../src/original/audio_engine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

namespace {
enum LocalParam : int {
    kAlgorithm=0,kMix,kBypass,kTime,kFeedback,kRate,kDepth,kDecay,kPreDelay,
    kDiffusion,kDamping,kWidth,kThreshold,kRatio,kAttack,kRelease,kUnitParams
};
constexpr int kUnits=4;
constexpr int kTotalParams=kUnitParams*kUnits;
const std::vector<std::string> kAlgorithms={"DELAY","CHORUS","FLANGER","ROOM","HALL","PLATE","MODULATED REVERB","VCF DISTORTION","EQ-PANNER-DDL","COMPRESSOR","MULTITAP DELAY","DUAL DELAY","8-VOICE CHORUS","PARAMETRIC EQ","DE-ESSER","EXPANDER","GATE","RUMBLE FILTER"};
const std::array<const char*,kUnits> kUnitNames={"A","B","C","D"};

int paramIndex(int unit,LocalParam p) noexcept { return unit*kUnitParams+static_cast<int>(p); }
}

class TROLLfxESPLabSeriesV04 final : public mpc::Plugin {
public:
    TROLLfxESPLabSeriesV04() : mpc::Plugin(makeInfo()) {
        for(int u=0;u<kUnits;++u)addUnitParameters(u);
        std::vector<float> defaults(kTotalParams,0.0f);
        for(int u=0;u<kUnits;++u){
            defaults[paramIndex(u,kMix)]=35.0f;
            defaults[paramIndex(u,kTime)]=420.0f;
            defaults[paramIndex(u,kFeedback)]=28.0f;
            defaults[paramIndex(u,kRate)]=0.27f;
            defaults[paramIndex(u,kDepth)]=2.0f;
            defaults[paramIndex(u,kDecay)]=2.0f;
            defaults[paramIndex(u,kPreDelay)]=13.0f;
            defaults[paramIndex(u,kDiffusion)]=65.0f;
            defaults[paramIndex(u,kDamping)]=35.0f;
            defaults[paramIndex(u,kWidth)]=100.0f;
            defaults[paramIndex(u,kThreshold)]=-18.0f;
            defaults[paramIndex(u,kRatio)]=4.0f;
            defaults[paramIndex(u,kAttack)]=8.0f;
            defaults[paramIndex(u,kRelease)]=120.0f;
            defaults[paramIndex(u,kBypass)]=u==0?0.0f:1.0f;
        }
        auto addAOnly=[&](const char* title,int algorithm){auto values=defaults;values[paramIndex(0,kAlgorithm)]=static_cast<float>(algorithm);addProgram(title,values);};
        addAOnly("A Only - Stereo Delay",0);addAOnly("A Only - Chorus",1);addAOnly("A Only - Flanger",2);
        addAOnly("A Only - Room",3);addAOnly("A Only - Hall",4);addAOnly("A Only - Plate",5);
        addAOnly("A Only - Modulated Reverb",6);addAOnly("A Only - VCF Distortion",7);
        addAOnly("A Only - EQ Panner DDL",8);addAOnly("A Only - Compressor",9);
        auto series=defaults;
        series[paramIndex(0,kAlgorithm)]=0;series[paramIndex(0,kMix)]=70;series[paramIndex(0,kBypass)]=0;
        series[paramIndex(1,kAlgorithm)]=1;series[paramIndex(1,kMix)]=35;series[paramIndex(1,kBypass)]=0;
        series[paramIndex(2,kAlgorithm)]=4;series[paramIndex(2,kMix)]=30;series[paramIndex(2,kBypass)]=0;
        series[paramIndex(3,kAlgorithm)]=9;series[paramIndex(3,kMix)]=45;series[paramIndex(3,kBypass)]=0;
        addProgram("ABCD Series - Delay / Chorus / Hall / Compressor",series);
    }

    void prepare(double sampleRate,int maxBlockSize) override {
        prepared_=false;const auto n=static_cast<std::size_t>(std::max(1,maxBlockSize));
        bool ok=true;for(auto& unit:units_)ok=unit.prepare(static_cast<float>(sampleRate),maxBlockSize)&&ok;
        try{scratchA_L_.assign(n,0.0f);scratchA_R_.assign(n,0.0f);scratchB_L_.assign(n,0.0f);scratchB_R_.assign(n,0.0f);}catch(...){ok=false;}
        sampleRate_=sampleRate;maxBlock_=n;prepared_=ok;
    }
    void reset() override {for(auto& u:units_)u.reset();}

    void process(mpc::ProcessContext& ctx) override {
        const int frames=std::max(0,ctx.numFrames);
        if(!frames||!ctx.outputs||ctx.numOutputs<2||!ctx.outputs[0]||!ctx.outputs[1])return;
        const float* inputL=ctx.inputs&&ctx.numInputs>0?ctx.inputs[0]:nullptr;
        const float* inputR=ctx.inputs&&ctx.numInputs>1?ctx.inputs[1]:inputL;
        if(!prepared_||maxBlock_==0){for(int i=0;i<frames;++i){ctx.outputs[0][i]=inputL?inputL[i]:0;ctx.outputs[1][i]=inputR?inputR[i]:0;}return;}
        int offset=0;
        while(offset<frames){const auto count=std::min<std::size_t>(maxBlock_,static_cast<std::size_t>(frames-offset));
            const float* sourceL=inputL?inputL+offset:scratchB_L_.data();const float* sourceR=inputR?inputR+offset:scratchB_R_.data();
            if(!inputL)std::fill(scratchB_L_.begin(),scratchB_L_.begin()+static_cast<std::ptrdiff_t>(count),0.0f);
            if(!inputR)std::fill(scratchB_R_.begin(),scratchB_R_.begin()+static_cast<std::ptrdiff_t>(count),0.0f);
            float* destinationL=scratchA_L_.data();float* destinationR=scratchA_R_.data();
            for(int u=0;u<kUnits;++u){
                float* outL=(u&1)?scratchB_L_.data():scratchA_L_.data();
                float* outR=(u&1)?scratchB_R_.data():scratchA_R_.data();
                if(u>0){sourceL=(u&1)?scratchA_L_.data():scratchB_L_.data();sourceR=(u&1)?scratchA_R_.data():scratchB_R_.data();}
                if(getBool(paramIndex(u,kBypass))){
                    for(std::size_t i=0;i<count;++i){outL[i]=sourceL?sourceL[i]:0.0f;outR[i]=sourceR?sourceR[i]:0.0f;}
                }else{
                    const int algo=std::clamp(getInt(paramIndex(u,kAlgorithm)),0,static_cast<int>(kAlgorithms.size()-1));
                    units_[u].setAlgorithm(static_cast<trollfx::EffectAlgorithm>(algo));
                    auto p=makeParams(u,algo);units_[u].setParams(p);
                    units_[u].process(sourceL,sourceR,outL,outR,count);
                }
                destinationL=outL;destinationR=outR;
            }
            for(std::size_t i=0;i<count;++i){float l=destinationL[i],r=destinationR[i];
                if(!std::isfinite(l))l=0;if(!std::isfinite(r))r=0;
                const float peak=std::max(std::abs(l),std::abs(r));if(peak>0.98f){const float g=0.98f/peak;l*=g;r*=g;}
                ctx.outputs[0][offset+static_cast<int>(i)]=l;ctx.outputs[1][offset+static_cast<int>(i)]=r;
            }
            offset+=static_cast<int>(count);
        }
    }

    bool formatValue(int index,float plain,std::string& out) override {
        if(index<0||index>=kTotalParams)return false;
        const int unit=index/kUnitParams,local=index%kUnitParams;
        const int algo=getInt(paramIndex(unit,kAlgorithm));char value[48];
        if(local==kTime){const bool hz=algo==7||algo==13||algo==14||algo==17;std::snprintf(value,sizeof(value),hz?"%.0f Hz":"%.1f ms",hz?std::clamp(plain,20.0f,20000.0f):std::clamp(plain,1.0f,10000.0f));out=value;return true;}
        if(local==kDepth){std::snprintf(value,sizeof(value),algo==7?"%.1f dB":(algo==13?"%.1f dB":"%.1f ms"),algo==7||algo==13?plain:std::min(30.0f,plain));out=value;return true;}
        if(algo==13&&local==kRate){std::snprintf(value,sizeof(value),"%.2f kHz",plain);out=value;return true;}
        if(algo==13&&local==kDecay){std::snprintf(value,sizeof(value),"Q %.2f",plain);out=value;return true;}
        if(algo==13&&local==kPreDelay){std::snprintf(value,sizeof(value),"Q %.2f",plain*.024f);out=value;return true;}
        if(local==kWidth&&algo==9){std::snprintf(value,sizeof(value),"%+.1f dB",(plain-50.0f)*0.24f);out=value;return true;}
        if(local==kDiffusion&&algo==8){std::snprintf(value,sizeof(value),"%+.1f dB",(plain-50.0f)*0.24f);out=value;return true;}
        if(local==kDamping&&algo==8){std::snprintf(value,sizeof(value),"%+.1f dB",(plain-50.0f)*0.24f);out=value;return true;}
        return false;
    }

private:
    void addUnitParameters(int u){
        using namespace mpc;const std::string prefix(1,static_cast<char>('a'+u));const std::string label=kUnitNames[static_cast<std::size_t>(u)];
        const auto add=[&](const char* suffix,const char* display,auto factory){
            const std::string id=prefix+"_"+suffix,caption=label+" "+display;
            addParam(factory(id.c_str(),caption.c_str()));
        };
        add("algorithm","Algorithm",[&](const char* i,const char* n){return choiceParam(i,n,kAlgorithms,0);});
        add("mix","Mix",[&](const char* i,const char* n){return floatParam(i,n,0,100,35,"%",0);});
        add("bypass","Bypass",[&](const char* i,const char* n){return boolParam(i,n,u!=0);});
        add("time","Time / Cutoff",[&](const char* i,const char* n){return logParam(i,n,1,20000,420,"",1);});
        add("feedback","Feedback / Regen",[&](const char* i,const char* n){return floatParam(i,n,-92,92,28,"%",0);});
        add("rate","Rate",[&](const char* i,const char* n){return logParam(i,n,0.01f,8.0f,0.27f,"Hz",2);});
        add("depth","Depth / Drive",[&](const char* i,const char* n){return floatParam(i,n,0,48,2,"",1);});
        add("decay","Decay",[&](const char* i,const char* n){return floatParam(i,n,0.2f,60.0f,2.0f,"s",2);});
        add("predelay","Pre-delay",[&](const char* i,const char* n){return floatParam(i,n,0,500,13,"ms",1);});
        add("diffusion","Diffusion / Bass EQ",[&](const char* i,const char* n){return floatParam(i,n,0,100,65,"%",0);});
        add("damping","Damping / Treble EQ",[&](const char* i,const char* n){return floatParam(i,n,0,100,35,"%",0);});
        add("width","Width / Makeup",[&](const char* i,const char* n){return floatParam(i,n,0,100,100,"%",0);});
        add("threshold","Threshold",[&](const char* i,const char* n){return floatParam(i,n,-96,0,-18,"dB",0);});
        add("ratio","Ratio",[&](const char* i,const char* n){return floatParam(i,n,1,40,4," : 1",1);});
        add("attack","Attack",[&](const char* i,const char* n){return floatParam(i,n,0.05f,1000,8,"ms",2);});
        add("release","Release",[&](const char* i,const char* n){return floatParam(i,n,1,10000,120,"ms",1);});
    }

    trollfx::EffectParams makeParams(int u,int algo) const noexcept {
        using namespace trollfx;EffectParams p;
        const auto value=[&](LocalParam id){return get(paramIndex(u,id));};
        p.mix=value(kMix)*0.01f;p.timeMs=std::clamp(value(kTime),1.0f,10000.0f);
        p.feedback=value(kFeedback)*0.01f;p.rateHz=value(kRate);p.depthMs=std::min(30.0f,value(kDepth));
        p.decaySeconds=value(kDecay);p.preDelayMs=value(kPreDelay);p.diffusion=value(kDiffusion)*0.01f;
        p.damping=value(kDamping)*0.01f;p.width=value(kWidth)*0.01f;p.thresholdDb=value(kThreshold);
        p.ratio=value(kRatio);p.attackMs=value(kAttack);p.releaseMs=value(kRelease);
        p.driveDb=std::clamp(value(kDepth),0.0f,48.0f);
        p.distortionGainInDb=12.0f;p.distortionGainOutDb=-6.0f;
        p.preVcfCutoffHz=std::clamp(value(kTime),20.0f,20000.0f);p.postVcfCutoffHz=p.preVcfCutoffHz*0.72f;
        p.preVcfQ=1.4f;p.postVcfQ=0.9f;p.envelopeAttackMs=std::max(0.05f,p.attackMs);p.envelopeReleaseMs=std::max(1.0f,p.releaseMs);
        p.compGainDb=(value(kWidth)-50.0f)*0.24f;
        p.earlyTime1Ms=std::clamp(7.0f+p.timeMs*0.02f,1.0f,120.0f);p.earlyTime2Ms=std::clamp(19.0f+p.timeMs*0.025f,1.0f,120.0f);
        p.earlyLevel1=0.25f;p.earlyLevel2=0.15f;
        p.pannerPhaseOpposed=(u&1)!=0;p.leftDelayMs=p.timeMs;p.rightDelayMs=std::min(1500.0f,p.timeMs*1.17f);
        p.leftEchoMs=std::min(1500.0f,p.timeMs*0.5f);p.rightEchoMs=std::min(1500.0f,p.timeMs*0.73f);
        p.echoLevel=std::clamp(value(kDepth)*0.01f,0.0f,1.0f);
        p.bassGainDb=(value(kDiffusion)-50.0f)*0.24f;p.trebleGainDb=(value(kDamping)-50.0f)*0.24f;
        p.cutoffHz=std::clamp(value(kTime),20.0f,20000.0f);
        if(algo==10||algo==11){p.leftEchoMs=std::clamp(value(kTime)*0.5f,1.0f,1500.0f);p.rightEchoMs=std::clamp(value(kTime)*0.73f,1.0f,1500.0f);p.echoLevel=std::clamp(value(kDepth)*0.02f,0.0f,0.96f);}
        if(algo==12){p.timeMs=std::clamp(value(kTime),8.0f,80.0f);p.depthMs=std::min(value(kDepth),15.0f);}
        if(algo==13){p.mid1FcHz=std::clamp(value(kTime),20.0f,20000.0f);p.mid1GainDb=std::clamp(value(kDepth),-24.0f,24.0f);p.mid1Q=std::clamp(value(kDecay),.25f,12.0f);p.mid2FcHz=std::clamp(value(kRate)*1000.0f,20.0f,20000.0f);p.mid2GainDb=std::clamp(value(kFeedback)*0.26f,-24.0f,24.0f);p.mid2Q=std::clamp(value(kPreDelay)*.024f,.25f,12.0f);p.inputTrimDb=(value(kWidth)-50.0f)*.24f;}
        if(algo==14){p.cutoffHz=std::clamp(value(kTime),1500.0f,20000.0f);p.thresholdDb=value(kThreshold);}
        if(algo==15||algo==16){p.thresholdDb=value(kThreshold);p.mix=1.0f;}
        if(algo==17)p.cutoffHz=std::clamp(value(kTime),20.0f,1000.0f);
        p.sampleHoldHz=0.0f;p.inputTrimDb=-3.0f;
        if(algo==7){p.distortionBypass=false;}
        return p;
    }

    static mpc::PluginInfo makeInfo(){mpc::PluginInfo i;i.name="ESP Lab Series V0.4";i.vendor="TROLLfx";i.uniqueId=MPC_FOURCC('T','f','x','S');i.version=400;i.isInstrument=false;i.numInputs=2;i.numOutputs=2;i.tailSamples=0;return i;}
    std::array<trollfx::EffectsUnit,kUnits> units_{};
    std::vector<float> scratchA_L_,scratchA_R_,scratchB_L_,scratchB_R_;
    double sampleRate_=44100.0;std::size_t maxBlock_=0;bool prepared_=false;
};

MPC_PLUGIN_ENTRY(TROLLfxESPLabSeriesV04)
