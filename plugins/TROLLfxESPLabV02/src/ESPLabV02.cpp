#include "mpc_plugin.h"
#include "../../../src/original/audio_engine.hpp"
#include <array>
#include <vector>
#include <algorithm>

namespace {
constexpr trollfx::EffectAlgorithm algorithm(int x) { return static_cast<trollfx::EffectAlgorithm>(x); }
}

class ESPLabV02 final : public mpc::Plugin {
    static constexpr int N=4, PER=7, BASE=2;
    enum Global { kRoute=0,kActive=1 };
    enum Unit { kAlgo=0,kTime,kDepth,kRate,kFeedback,kBypass,kMix };
    static int ix(int u,int p){ return BASE+u*PER+p; }
    static mpc::PluginInfo info(){ mpc::PluginInfo i; i.name="ESP Lab V0.2"; i.vendor="TROLLfx"; i.uniqueId=MPC_FOURCC('T','f','x','2'); i.version=2000; i.isInstrument=false; i.numInputs=2; i.numOutputs=2; return i; }
public:
    ESPLabV02():mpc::Plugin(info()) {
        using namespace mpc;
        addParam(choiceParam("routing","Routing",{"Serial","Parallel","Dual"},0));
        addParam(intParam("active","Active Unit",0,3,0));
        for(int u=0;u<N;++u){ const char c=char('A'+u); const std::string s(1,c);
            addParam(choiceParam(("algo"+s).c_str(),("Unit "+s+" Algorithm").c_str(),{"Delay","Chorus","Flanger","Room","Hall","Plate","Mod Reverb"},1));
            addParam(logParam(("time"+s).c_str(),("Unit "+s+" Time").c_str(),1,10000,350,"ms",0));
            addParam(floatParam(("depth"+s).c_str(),("Unit "+s+" Depth").c_str(),0,100,4,"ms",1));
            addParam(logParam(("rate"+s).c_str(),("Unit "+s+" Rate").c_str(),0.01f,8,0.35f,"Hz",2));
            addParam(floatParam(("fb"+s).c_str(),("Unit "+s+" Feedback").c_str(),-92,92,20,"%",0));
            addParam(boolParam(("bypass"+s).c_str(),("Unit "+s+" Bypass").c_str(),false));
            addParam(floatParam(("mix"+s).c_str(),("Unit "+s+" Mix").c_str(),0,100,50,"%",0));
        }
    }
    void prepare(double sr,int) override { for(auto& e:engines_) e.prepare(float(sr),512); for(auto& b:buffers_) b.assign(512,0); }
    void reset() override { for(auto& e:engines_) e.reset(); }
    void process(mpc::ProcessContext& c) override {
        const float* l=c.inputs[0]; const float* r=c.numInputs>1?c.inputs[1]:c.inputs[0];
        float* ol=c.outputs[0]; float* or_=c.numOutputs>1?c.outputs[1]:c.outputs[0];
        const std::size_t n=std::min<std::size_t>(std::size_t(c.numFrames),buffers_[0].size());
        for(int u=0;u<N;++u){ const int a=ix(u,kAlgo); trollfx::EffectParams p; p.timeMs=get(ix(u,kTime)); p.depthMs=get(ix(u,kDepth)); p.rateHz=get(ix(u,kRate)); p.feedback=get(ix(u,kFeedback))*0.01f; p.mix=get(ix(u,kMix))*0.01f; p.diffusion=.65f; p.damping=.35f; engines_[u].setAlgorithm(algorithm(std::min(getInt(a),6))); engines_[u].setParams(p); }
        const int route=getInt(kRoute);
        float* aL=buffers_[0].data(); float* aR=buffers_[1].data();
        float* bL=buffers_[2].data(); float* bR=buffers_[3].data();
        float* cL=buffers_[4].data(); float* cR=buffers_[5].data();
        float* dL=buffers_[6].data(); float* dR=buffers_[7].data();
        auto runUnit=[&](int u,const float* srcL,const float* srcR,float* dstL,float* dstR) {
            if(getBool(ix(u,kBypass))) { for(std::size_t i=0;i<n;++i){dstL[i]=srcL[i];dstR[i]=srcR[i];} }
            else engines_[u].process(srcL,srcR,dstL,dstR,n);
        };
        if(route==0) { // four units in series
            const float* srcL=l; const float* srcR=r;
            for(int u=0;u<N;++u){ float* dstL=(u&1)?bL:aL; float* dstR=(u&1)?bR:aR; runUnit(u,srcL,srcR,dstL,dstR); srcL=dstL; srcR=dstR; }
            for(std::size_t i=0;i<n;++i){ol[i]=srcL[i];or_[i]=srcR[i];}
        } else if(route==1) { // four parallel stereo paths, equal-power average
            std::fill_n(aL,n,0.0f); std::fill_n(aR,n,0.0f);
            for(int u=0;u<N;++u){ runUnit(u,l,r,cL,cR); for(std::size_t i=0;i<n;++i){aL[i]+=cL[i]*0.25f;aR[i]+=cR[i]*0.25f;} }
            for(std::size_t i=0;i<n;++i){ol[i]=aL[i];or_[i]=aR[i];}
        } else { // dual stereo chains A->B and C->D, averaged at the output
            runUnit(0,l,r,bL,bR); runUnit(1,bL,bR,aL,aR);
            runUnit(2,l,r,dL,dR); runUnit(3,dL,dR,cL,cR);
            for(std::size_t i=0;i<n;++i){ol[i]=0.5f*(aL[i]+cL[i]);or_[i]=0.5f*(aR[i]+cR[i]);}
        }
    }
private: std::array<trollfx::EffectsUnit,N> engines_; std::array<std::vector<float>,8> buffers_;
};
MPC_PLUGIN_ENTRY(ESPLabV02)
