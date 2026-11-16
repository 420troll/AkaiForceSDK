#include "mpc_plugin.h"
#include "../../../src/original/audio_engine.hpp"
#include "../../../src/original/dp4_reverb.hpp"
#include "../../../src/original/modulation.hpp"
#include "../../../src/original/force_routing.hpp"
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

namespace {
constexpr trollfx::EffectAlgorithm algorithm(int x) { return static_cast<trollfx::EffectAlgorithm>(x); }
}

class ESPLabV03 final : public mpc::Plugin {
    static constexpr int N=4, PER=34, BASE=2;
    enum Global { kRoute=0,kActive=1 };
    enum Unit { kAlgo=0,kTime,kFeedback,kRate,kDepth,kDecay,kPredelay,kDamping,kDiffusion,kWidth,kCutoff,kDrive,kRatio,kThreshold,kAttack,kRelease,kBypass,kMix,kEarlyTime1,kEarlyLevel1,kEarlyTime2,kEarlyLevel2,kM1Enabled,kM1Source,kM1Dest,kM1Min,kM1Max,kM1Invert,kM2Enabled,kM2Source,kM2Dest,kM2Min,kM2Max,kM2Invert };
    static int ix(int u,int p){ return BASE+u*PER+p; }
    static float logNorm(float x,float lo,float hi){return std::clamp(std::log(std::max(x,lo)/lo)/std::log(hi/lo),0.0f,1.0f);}
    static float logDenorm(float x,float lo,float hi){return lo*std::pow(hi/lo,std::clamp(x,0.0f,1.0f));}
    static mpc::PluginInfo info(){ mpc::PluginInfo i; i.name="ESP Lab V0.3"; i.vendor="TROLLfx"; i.uniqueId=MPC_FOURCC('T','f','x','3'); i.version=3000; i.isInstrument=false; i.numInputs=2; i.numOutputs=2; return i; }
public:
    ESPLabV03():mpc::Plugin(info()) {
        using namespace mpc;
        addParam(choiceParam("routing","Routing",{"Serial","Parallel","Dual","AB Feedback 1","AB Feedback 2"},0));
        addParam(intParam("active","Active Unit",0,3,0));
        for(int u=0;u<N;++u){ const char c=char('A'+u); const std::string s(1,c);
            addParam(choiceParam(("algo"+s).c_str(),("Unit "+s+" Algorithm").c_str(),{"Delay","Chorus","Flanger","Room","Hall","Plate","Mod Reverb","VCF Distortion","EQ Panner DDL","Compressor"},1));
            addParam(logParam(("time"+s).c_str(),("Unit "+s+" Time").c_str(),0.1f,10000,350,"ms",0));
            addParam(floatParam(("fb"+s).c_str(),("Unit "+s+" Feedback").c_str(),-99,99,20,"%",0));
            addParam(logParam(("rate"+s).c_str(),("Unit "+s+" Rate").c_str(),0.01f,8,0.35f,"Hz",2));
            addParam(floatParam(("depth"+s).c_str(),("Unit "+s+" Depth").c_str(),0,30,4,"ms",1));
            addParam(logParam(("decay"+s).c_str(),("Unit "+s+" Decay").c_str(),0.2f,60,2,"s",1));
            addParam(floatParam(("predelay"+s).c_str(),("Unit "+s+" Predelay").c_str(),0,500,20,"ms",0));
            addParam(floatParam(("damping"+s).c_str(),("Unit "+s+" Damping").c_str(),0,99,35,"",0));
            addParam(floatParam(("diffusion"+s).c_str(),("Unit "+s+" Diffusion").c_str(),0,99,65,"",0));
            addParam(floatParam(("width"+s).c_str(),("Unit "+s+" Width").c_str(),0,100,100,"%",0));
            addParam(logParam(("cutoff"+s).c_str(),("Unit "+s+" VCF Cutoff").c_str(),80,10000,1800,"Hz",0));
            addParam(floatParam(("drive"+s).c_str(),("Unit "+s+" Drive").c_str(),0,48,12,"dB",0));
            addParam(floatParam(("ratio"+s).c_str(),("Unit "+s+" Ratio").c_str(),1,40,4,":1",1));
            addParam(floatParam(("threshold"+s).c_str(),("Unit "+s+" Threshold").c_str(),-96,0,-18,"dB",0));
            addParam(floatParam(("attack"+s).c_str(),("Unit "+s+" Attack").c_str(),0.05f,100,8,"ms",1));
            addParam(logParam(("release"+s).c_str(),("Unit "+s+" Release").c_str(),1,10000,120,"ms",1));
            addParam(boolParam(("bypass"+s).c_str(),("Unit "+s+" Bypass").c_str(),false));
            addParam(floatParam(("mix"+s).c_str(),("Unit "+s+" Mix").c_str(),0,100,50,"%",0));
            addParam(floatParam(("early1time"+s).c_str(),("Unit "+s+" Early Time 1").c_str(),0,120,17,"ms",0));
            addParam(floatParam(("early1level"+s).c_str(),("Unit "+s+" Early Level 1").c_str(),0,100,25,"%",0));
            addParam(floatParam(("early2time"+s).c_str(),("Unit "+s+" Early Time 2").c_str(),0,120,31,"ms",0));
            addParam(floatParam(("early2level"+s).c_str(),("Unit "+s+" Early Level 2").c_str(),0,100,15,"%",0));
            for(int slot=0;slot<2;++slot){ const std::string z=std::to_string(slot+1)+s; const int en=slot?kM2Enabled:kM1Enabled, so=slot?kM2Source:kM1Source, de=slot?kM2Dest:kM1Dest, mi=slot?kM2Min:kM1Min, ma=slot?kM2Max:kM1Max, iv=slot?kM2Invert:kM1Invert; (void)en;(void)so;(void)de;(void)mi;(void)ma;(void)iv;
                addParam(boolParam(("mod"+z+"en").c_str(),("Unit "+s+" Mod"+std::to_string(slot+1)+" Enabled").c_str(),false));
                addParam(choiceParam(("mod"+z+"src").c_str(),("Unit "+s+" Mod"+std::to_string(slot+1)+" Source").c_str(),{"Off","Internal LFO","Envelope"},0));
                addParam(choiceParam(("mod"+z+"dst").c_str(),("Unit "+s+" Mod"+std::to_string(slot+1)+" Destination").c_str(),{"Time","Feedback","Mix","Rate","Depth","Diffusion","Damping","Width","Threshold","Ratio","Attack","Release","Cutoff","Decay","Output"},2));
                addParam(floatParam(("mod"+z+"min").c_str(),("Unit "+s+" Mod"+std::to_string(slot+1)+" Min").c_str(),0,100,0,"%",0));
                addParam(floatParam(("mod"+z+"max").c_str(),("Unit "+s+" Mod"+std::to_string(slot+1)+" Max").c_str(),0,100,100,"%",0));
                addParam(boolParam(("mod"+z+"inv").c_str(),("Unit "+s+" Mod"+std::to_string(slot+1)+" Invert").c_str(),false));
            }
        }
    }
    void prepare(double sr,int) override { for(auto& e:engines_) e.prepare(float(sr),512); for(auto& e:reverbs_) e.prepare(float(sr)); for(auto& b:buffers_) b.assign(512,0); feedbackL_.assign(512,0); feedbackR_.assign(512,0); }
    void reset() override { for(auto& e:engines_) e.reset(); for(auto& e:reverbs_) e.reset(); for(auto& m:modulators_)m.reset(); std::fill(feedbackL_.begin(),feedbackL_.end(),0); std::fill(feedbackR_.begin(),feedbackR_.end(),0); }
    void process(mpc::ProcessContext& c) override {
        const float* l=c.inputs[0]; const float* r=c.numInputs>1?c.inputs[1]:c.inputs[0];
        float* ol=c.outputs[0]; float* or_=c.numOutputs>1?c.outputs[1]:c.outputs[0];
        const std::size_t n=std::min<std::size_t>(std::size_t(c.numFrames),buffers_[0].size());
        for(int u=0;u<N;++u){
            for(int slot=0;slot<2;++slot){const int b=slot?kM2Enabled:kM1Enabled; trollfx::ModulatorSlot m; m.enabled=getBool(ix(u,b)); m.source=static_cast<trollfx::ModSource>(std::clamp(getInt(ix(u,b+1)),0,2)); m.destination=static_cast<trollfx::ModDestination>(std::clamp(getInt(ix(u,b+2)),0,14)); m.minNorm=get(ix(u,b+3))*.01f; m.maxNorm=get(ix(u,b+4))*.01f; m.invert=getBool(ix(u,b+5)); modulators_[u].setSlot(static_cast<std::size_t>(slot),m); }
            modulators_[u].beginBlock(static_cast<float>(c.sampleRate),get(ix(u,kRate)));
        }
        const int route=getInt(kRoute);
        float* aL=buffers_[0].data(); float* aR=buffers_[1].data();
        float* bL=buffers_[2].data(); float* bR=buffers_[3].data();
        float* cL=buffers_[4].data(); float* cR=buffers_[5].data();
        float* dL=buffers_[6].data(); float* dR=buffers_[7].data();
        auto runUnit=[&](int u,const float* srcL,const float* srcR,float* dstL,float* dstR) {
            if(getBool(ix(u,kBypass))) { for(std::size_t i=0;i<n;++i){dstL[i]=srcL[i];dstR[i]=srcR[i];} }
            else {
                const int ai=getInt(ix(u,kAlgo));
                trollfx::EffectParams baseP; baseP.timeMs=get(ix(u,kTime)); baseP.feedback=get(ix(u,kFeedback))*.01f; baseP.rateHz=get(ix(u,kRate)); baseP.depthMs=get(ix(u,kDepth)); baseP.mix=get(ix(u,kMix))*.01f; baseP.diffusion=get(ix(u,kDiffusion))*.01f; baseP.damping=get(ix(u,kDamping))*.01f; baseP.width=get(ix(u,kWidth))*.01f; baseP.cutoffHz=get(ix(u,kCutoff)); baseP.depthMs=std::min(30.0f,baseP.depthMs+get(ix(u,kDrive))*.2f); baseP.ratio=get(ix(u,kRatio)); baseP.thresholdDb=get(ix(u,kThreshold)); baseP.attackMs=get(ix(u,kAttack)); baseP.releaseMs=get(ix(u,kRelease));
                trollfx::ModulationMatrix::Values base{},out{};
                base[0]=logNorm(baseP.timeMs,.1f,10000); base[1]=(baseP.feedback+.92f)/1.84f; base[2]=baseP.mix; base[3]=logNorm(baseP.rateHz,.01f,8); base[4]=baseP.depthMs/30; base[5]=baseP.diffusion; base[6]=baseP.damping/.98f; base[7]=baseP.width; base[8]=(baseP.thresholdDb+96)/96; base[9]=(baseP.ratio-1)/39; base[10]=logNorm(baseP.attackMs,.05f,100); base[11]=logNorm(baseP.releaseMs,1,10000); base[12]=logNorm(baseP.cutoffHz,80,10000); base[13]=logNorm(get(ix(u,kDecay)),.2f,60); base[14]=(0.0f+24.0f)/36.0f;
                engines_[u].setAlgorithm(algorithm(std::min(ai,9)));
                const auto rk=ai==3?trollfx::ReverbKind::Room:ai==4?trollfx::ReverbKind::Hall:ai==5?trollfx::ReverbKind::Plate:trollfx::ReverbKind::Modulated;
                std::size_t off=0;
                while(off<n){ const std::size_t count=std::min<std::size_t>(8,n-off);
                    for(std::size_t j=0;j<count;++j) modulators_[u].process(srcL[off+j],srcR[off+j],base,out);
                    trollfx::EffectParams p=baseP; p.timeMs=logDenorm(out[0],.1f,10000); p.feedback=(out[1]*1.84f)-.92f; p.mix=out[2]; p.rateHz=logDenorm(out[3],.01f,8); p.depthMs=out[4]*30; p.diffusion=out[5]; p.damping=out[6]*.98f; p.width=out[7]; p.thresholdDb=out[8]*96-96; p.ratio=1+out[9]*39; p.attackMs=logDenorm(out[10],.05f,100); p.releaseMs=logDenorm(out[11],1,10000); p.cutoffHz=logDenorm(out[12],80,10000); engines_[u].setParams(p);
                    trollfx::ReverbParams rp; rp.decaySeconds=logDenorm(out[13],.2f,60); rp.outputDb=out[14]*36.0f-24.0f; rp.preDelayMs=get(ix(u,kPredelay)); rp.damping=p.damping; rp.diffusion=p.diffusion; rp.width=p.width; rp.mix=p.mix; rp.earlyTime1Ms=get(ix(u,kEarlyTime1)); rp.earlyLevel1=get(ix(u,kEarlyLevel1))*.01f; rp.earlyTime2Ms=get(ix(u,kEarlyTime2)); rp.earlyLevel2=get(ix(u,kEarlyLevel2))*.01f; rp.modRateHz=p.rateHz; rp.modDepthMs=std::min(2.0f,p.depthMs*.1f); reverbs_[u].setKind(rk); reverbs_[u].setParams(rp);
                    if(ai>=3&&ai<=6) reverbs_[u].process(srcL+off,srcR+off,dstL+off,dstR+off,count); else engines_[u].process(srcL+off,srcR+off,dstL+off,dstR+off,count);
                    off+=count;
                }
            }
        };
        if(route==0) { // four units in series
            const float* srcL=l; const float* srcR=r;
            for(int u=0;u<N;++u){ float* dstL=(u&1)?bL:aL; float* dstR=(u&1)?bR:aR; runUnit(u,srcL,srcR,dstL,dstR); srcL=dstL; srcR=dstR; }
            for(std::size_t i=0;i<n;++i){ol[i]=srcL[i];or_[i]=srcR[i];}
        } else if(route==1) { // four parallel stereo paths, equal-power average
            std::fill_n(aL,n,0.0f); std::fill_n(aR,n,0.0f);
            for(int u=0;u<N;++u){ runUnit(u,l,r,cL,cR); for(std::size_t i=0;i<n;++i){aL[i]+=cL[i]*0.25f;aR[i]+=cR[i]*0.25f;} }
            for(std::size_t i=0;i<n;++i){ol[i]=aL[i];or_[i]=aR[i];}
        } else if(route==2) { // dual stereo chains A->B and C->D, averaged at the output
            runUnit(0,l,r,bL,bR); runUnit(1,bL,bR,aL,aR);
            runUnit(2,l,r,dL,dR); runUnit(3,dL,dR,cL,cR);
            for(std::size_t i=0;i<n;++i){ol[i]=0.5f*(aL[i]+cL[i]);or_[i]=0.5f*(aR[i]+cR[i]);}
        } else { // stereo 1-source AB feedback approximation; feedback is one block delayed
            const float fb=std::clamp(get(ix(0,kFeedback))*.01f,-.92f,.92f);
            for(std::size_t i=0;i<n;++i){ aL[i]=trollfx::routeFeedbackInput(l[i],feedbackL_[i],fb); aR[i]=trollfx::routeFeedbackInput(r[i],feedbackR_[i],fb); }
            runUnit(0,aL,aR,cL,cR); runUnit(1,cL,cR,bL,bR);
            for(std::size_t i=0;i<n;++i){ feedbackL_[i]=bL[i]; feedbackR_[i]=bR[i]; trollfx::routeFeedbackOutput(route==3?0:1,l[i],r[i],bL[i],bR[i],ol[i],or_[i]); }
        }
    }
private: std::array<trollfx::EffectsUnit,N> engines_; std::array<trollfx::DP4InspiredReverb,N> reverbs_; std::array<trollfx::ModulationMatrix,N> modulators_; std::array<std::vector<float>,8> buffers_; std::vector<float> feedbackL_,feedbackR_;
};
MPC_PLUGIN_ENTRY(ESPLabV03)
