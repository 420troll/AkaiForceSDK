#include "../Es5510V02Processor.hpp"
#include "../../Es5510EffectProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

using trollfx::esp::vst::Es5510EffectProcessor;
using trollfx::esp::vst::v02::Processor;

namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<float> makeInput(std::size_t frames,double fs,double hz){std::vector<float> x(frames);for(std::size_t i=0;i<frames;++i)x[i]=0.2f*static_cast<float>(std::sin(6.283185307179586*hz*double(i)/fs));return x;}
void compareDelay(double sr){
    constexpr std::size_t n=8192;
    auto l=makeInput(n,sr,137.0),r=makeInput(n,sr,223.0);std::vector<float> oldL(n),oldR(n),newL(n),newR(n);
    Es5510EffectProcessor old;Processor next;
    require(old.prepare(sr,512),"V0.1 processor prepare failed");require(next.prepare(sr,512),"V0.2 processor prepare failed");
    Es5510EffectProcessor::Parameters a;a.delayMs=250;a.feedback=0.55f;a.wet=0.5f;a.dry=0.5f;
    Processor::Parameters b;b.mode=Processor::Mode::Delay;b.delayMs=250;b.feedback=0.55f;b.wet=0.5f;b.dry=0.5f;b.sync=false;
    old.processBlock(l.data(),r.data(),oldL.data(),oldR.data(),n,a);next.processBlock(l.data(),r.data(),newL.data(),newR.data(),n,b);
    float maxDelta=0;for(std::size_t i=0;i<n;++i){maxDelta=std::max(maxDelta,std::abs(oldL[i]-newL[i]));maxDelta=std::max(maxDelta,std::abs(oldR[i]-newR[i]));}
    require(maxDelta==0.0f,"sync-off V0.2 delay changed V0.1 output");
}
void exerciseModes(double sr){
    constexpr std::size_t n=4096;auto l=makeInput(n,sr,220.0),r=makeInput(n,sr,330.0);std::vector<float> ol(n),orr(n);
    Processor p;require(p.prepare(sr,256),"V0.2 prepare failed");Processor::Parameters q;q.feedback=0.65f;q.wet=0.65f;q.dry=0.35f;
    for(auto mode:{Processor::Mode::Chorus,Processor::Mode::Phaser,Processor::Mode::Panner,Processor::Mode::Rotary,Processor::Mode::Delay}){
        q.mode=mode;q.bypass=false;
        if(mode==Processor::Mode::Phaser){q.phaserCenterHz=700;q.phaserDepthOctaves=1.5f;q.phaserFeedback=-0.5f;}
        if(mode==Processor::Mode::Panner){q.panPosition=0;q.panWidth=1.2f;q.panMotion=0.7f;q.panRateHz=0.4f;}
        if(mode==Processor::Mode::Rotary){q.rotarySpeed=Processor::RotarySpeed::Fast;q.hornSpeedHz=6.5f;q.drumSpeedHz=1.7f;q.drive=0.1f;}
        p.processBlock(l.data(),r.data(),ol.data(),orr.data(),n,q);
        double energy=0;for(std::size_t i=0;i<n;++i){require(std::isfinite(ol[i])&&std::isfinite(orr[i]),"nonfinite audio");energy+=double(ol[i])*ol[i]+double(orr[i])*orr[i];}
        require(energy>0.01,"selected mode produced silence");
    }
    q.mode=Processor::Mode::Delay;q.sync=true;q.tempoSource=trollfx::esp::vst::TempoSource::Host;q.hostTempoValid=true;q.hostBpm=137;q.division=trollfx::esp::vst::Dp4Division::EighthDotted;
    p.processBlock(l.data(),r.data(),ol.data(),orr.data(),n,q);
    q.pingPong=true;p.processBlock(l.data(),r.data(),ol.data(),orr.data(),n,q);
    q.bypass=true;p.processBlock(l.data(),r.data(),ol.data(),orr.data(),n,q);
    require(p.processedFrames()>=n*8,"frame accounting missed mode/bypass runs");
}
}
int main(){try{compareDelay(44100);compareDelay(48000);exerciseModes(44100);exerciseModes(48000);std::cout<<"PASS V0.2 prepare/modes/bypass/tempo; DELAY Sync OFF is sample-identical to V0.1 at 44.1/48 kHz\n";}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}return 0;}
