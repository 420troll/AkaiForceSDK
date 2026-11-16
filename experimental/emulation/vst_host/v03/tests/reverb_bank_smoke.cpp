#include "../ReverbBank.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::uint32_t kRate=44100,kFrames=kRate*2,kBlock=128;
using Bank=trollfx::esp::vst::v03::ReverbBank;
struct Measures {double early=0,late=0,corr=0;float peak=0;std::uint32_t clips=0,first=0;};
Measures render(Bank::Mode mode,bool beat){
    Bank bank;if(!bank.prepare(kRate))throw std::runtime_error("prepare");
    Bank::Parameters p;p.wet=1.0f;p.dry=0.0f;p.decaySeconds=1.8f;p.predelayMs=13;
    p.attackMs=45;p.holdMs=100;p.releaseMs=300;p.triggerDb=-30;p.retriggerDb=-28;
    std::array<float,kBlock> inL{},inR{},outL{},outR{};
    std::vector<std::array<float,2>> audio(kFrames);
    for(std::uint32_t pos=0;pos<kFrames;pos+=kBlock){
        const auto n=std::min(kBlock,kFrames-pos);
        for(std::uint32_t i=0;i<n;++i){const auto frame=pos+i;float x=0;
            if(beat){if(frame%2205==0)x=(frame%8820==0)?0.97f:0.72f;}
            else if(frame==0){x=0.82f;}
            inL[i]=x;inR[i]=beat?x*0.73f:(frame==0?x*0.41f:0.0f);
        }
        bank.processBlock(mode,inL.data(),inR.data(),outL.data(),outR.data(),n,p);
        for(std::uint32_t i=0;i<n;++i)audio[pos+i]={outL[i],outR[i]};
    }
    Measures m;long double ll=0,rr=0,lr=0;for(std::uint32_t i=0;i<kFrames;++i){auto l=audio[i][0],r=audio[i][1];
        const float peak=std::max(std::abs(l),std::abs(r));m.peak=std::max(m.peak,peak);m.clips+=peak>1.0f;
        if(peak>1e-6f&&!m.first)m.first=i;
        const double e=double(l)*l+double(r)*r;if(i<kRate/10)m.early+=e;else m.late+=e;
        ll+=double(l)*l;rr+=double(r)*r;lr+=double(l)*r;
    }
    m.corr=static_cast<double>(lr/std::sqrt(std::max<long double>(1e-30L,ll*rr)));
    m.early=std::sqrt(m.early/(2.0*kRate/10));m.late=std::sqrt(m.late/(2.0*(kFrames-kRate/10)));
    return m;
}
}
int main(){
    const std::array<Bank::Mode,10> modes={Bank::Mode::SmallPlate,Bank::Mode::LargePlate,Bank::Mode::Room,
        Bank::Mode::Hall,Bank::Mode::Gated,Bank::Mode::ReverseOneShot,Bank::Mode::ReverseRetrigger,
        Bank::Mode::NonLin1,Bank::Mode::NonLin2,Bank::Mode::NonLin3};
    std::array<Measures,10> impulse{};std::array<Measures,10> hot{};
    for(std::size_t i=0;i<modes.size();++i){impulse[i]=render(modes[i],false);hot[i]=render(modes[i],true);
        std::cout<<"mode="<<i<<" impulse_peak="<<impulse[i].peak<<" early_rms="<<impulse[i].early
                 <<" late_rms="<<impulse[i].late<<" corr="<<impulse[i].corr
                 <<" beat_peak="<<hot[i].peak<<" clipped="<<hot[i].clips<<std::endl;
        if(impulse[i].peak==0||hot[i].peak==0||hot[i].clips)throw std::runtime_error("silence or clipping at mode="+std::to_string(i));
    }
    double difference=0;for(std::size_t i=0;i<impulse.size();++i)for(std::size_t j=i+1;j<impulse.size();++j)
        difference+=std::abs(impulse[i].early-impulse[j].early)+std::abs(impulse[i].late-impulse[j].late)+std::abs(impulse[i].corr-impulse[j].corr);
    if(difference<0.2)throw std::runtime_error("modes insufficiently differentiated");
    std::cout<<"PASS ten ES5510 reverb bank modes; hot vectors unclipped; aggregate metric distance="<<difference<<"\n";
}
