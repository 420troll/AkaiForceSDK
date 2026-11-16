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
        int route=getInt(kRoute); const float* inL=l; const float* inR=r;
        std::array<float*,2> prev{buffers_[0].data(),buffers_[1].data()};
        std::array<float*,2> next{buffers_[2].data(),buffers_[3].data()};
        if(route==0){ for(int u=0;u<N;++u){ engines_[u].process(inL,inR,next[0],next[1],n); if(!getBool(ix(u,kBypass))){ std::swap(prev,next); inL=prev[0]; inR=prev[1]; } } }
        else if(route==1){ std::fill_n(prev[0],n,0); std::fill_n(prev[1],n,0); for(int u=0;u<N;++u){ engines_[u].process(l,r,next[0],next[1],n); if(!getBool(ix(u,kBypass))) for(std::size_t i=0;i<n;++i){prev[0][i]+=next[0][i]*0.25f;prev[1][i]+=next[1][i]*0.25f;} } }
        else { engines_[0].process(l,l,next[0],next[1],n); engines_[1].process(next[0],next[1],prev[0],prev[1],n); engines_[2].process(r,r,next[0],next[1],n); engines_[3].process(next[0],next[1],prev[0],prev[1],n); }
        for(std::size_t i=0;i<n;++i){ const float wetL=route==2?0.5f*(prev[0][i]+prev[1][i]):prev[0][i]; const float wetR=route==2?wetL:prev[1][i]; ol[i]=l[i]+(wetL-l[i])*0.5f; or_[i]=r[i]+(wetR-r[i])*0.5f; }
    }
private: std::array<trollfx::EffectsUnit,N> engines_; std::array<std::vector<float>,4> buffers_;
};
MPC_PLUGIN_ENTRY(ESPLabV02)
