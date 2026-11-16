#include "ReverbBank.hpp"

#include "../../programs/envelope_reverbs/program_metadata.hpp"
#include "../../programs/reverbs/ReverbPrograms.hpp"
#include "../../programs/room_hall/program_metadata.hpp"
#include "../../programs/nonlin_tap/NonLinTapPrograms.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace trollfx::esp::vst::v03 {
namespace {
using namespace trollfx::esp::programs;
using namespace trollfx::esp::programs::room_hall;
using namespace trollfx::esp::programs::envelope_reverbs;
constexpr double kPi=3.14159265358979323846;
constexpr std::array<std::uint64_t,kGatedPlateEnvelopeWords> kGated={
#include "../../programs/envelope_reverbs/gated_plate_envelope_words.inc"
};
constexpr std::array<std::uint64_t,kReverseOneShotWords> kReverseOneShot={
#include "../../programs/envelope_reverbs/reverse_one_shot_words.inc"
};
constexpr std::array<std::uint64_t,kReverseRetriggerWords> kReverseRetrigger={
#include "../../programs/envelope_reverbs/reverse_retrigger_words.inc"
};
constexpr std::array<std::uint64_t,kRoomWords> kRoom={
#include "../../programs/room_hall/room_feedback_diffusion_words.inc"
};
constexpr std::array<std::uint64_t,kHallWords> kHall={
#include "../../programs/room_hall/hall_feedback_diffusion_words.inc"
};
constexpr std::array<float,2> kSmallCoefficients={0.58f,-0.52f};
constexpr std::array<float,3> kLargeCoefficients={0.63f,-0.59f,0.67f};
constexpr float kRoomCoefficient=0.78f,kHallCoefficient1=0.58f,kHallCoefficient2=0.30f;
inline float sane(float x,float fallback) noexcept{return std::isfinite(x)?x:fallback;}
}

std::int16_t ReverbBank::toPcm(float x) noexcept {
    if(!std::isfinite(x))return 0;
    const long v=std::clamp<long>(std::lround(std::clamp(x,-1.0f,0.999969f)*32768.0f),-32768,32767);
    return static_cast<std::int16_t>(v);
}
float ReverbBank::fromPcm(std::int16_t x) noexcept{return static_cast<float>(x)*(1.0f/32768.0f);}
std::uint32_t ReverbBank::q22(float x) noexcept {
    const auto q=std::clamp<long>(std::lround(static_cast<double>(x)*4194304.0),0,0x7fffff);
    return static_cast<std::uint32_t>(q);
}
std::uint32_t ReverbBank::q22Signed(float x) noexcept {
    const auto q=std::clamp<long>(std::lround(static_cast<double>(x)*4194304.0),-0x800000,0x7fffff);
    return static_cast<std::uint32_t>(q)&0xffffffu;
}

bool ReverbBank::prepare(double sampleRate) noexcept {
    if(!std::isfinite(sampleRate)||(std::abs(sampleRate-44100.0)>0.01&&std::abs(sampleRate-48000.0)>0.01))return false;
    sampleRate_=sampleRate;sampleRateInt_=static_cast<std::uint32_t>(std::lround(sampleRate));
    for(std::size_t i=0;i<kCount;++i)if(!setup(static_cast<Mode>(i)))return false;
    active_=fadeFrom_=fadeTo_=Mode::SmallPlate;fadeRemaining_=fadeTotal_=0;processedFrames_=0;prepared_=true;return true;
}

bool ReverbBank::setup(Mode mode) noexcept {
    auto& e=engines_[static_cast<std::size_t>(mode)];
    e.leftTap=e.rightTap=e.bankSamples=0;e.writeLeft=e.writeRight=0;e.feedbackGpr=40;e.wetGpr=41;e.dryGpr=42;
    e.coefficient.fill(0.0f);e.coefficientGpr.fill(0);e.coefficientCount=0;e.envGpr=e.dampingGpr=0;
    e.tapLevelGpr.fill(0);e.tapLeftGpr.fill(0);e.tapRightGpr.fill(0);e.tapLeft.fill(0);e.tapRight.fill(0);e.defaultTapLevels.fill(0.0f);e.tapCount=0;
    e.feedbackBase=0.65f;e.holdRemaining=0;e.envelope=0.0f;e.triggered=e.previousAbove=false;e.frames=0;
    // Allocate each engine only for its longest required reflection/tap plus
    // the 500 ms maximum predelay. NonLin 2 alone needs the full 96k bank.
    std::uint32_t engineBank=32768;
    if(mode==Mode::NonLin1)engineBank=49152;
    else if(mode==Mode::NonLin2)engineBank=96000;
    const std::size_t dramWords=static_cast<std::size_t>(engineBank)*2;
    if(dramWords>MameES5510::kDramWords||!e.cpu.prepare(dramWords))return false;
    e.bankSamples=engineBank;e.cpu.setHostSerial(0x20);e.cpu.setRegister(244,0);
    e.cpu.setRegister(245,static_cast<std::uint32_t>(dramWords-1));e.cpu.setRegister(248,0);
    const std::uint64_t* words=nullptr;std::size_t count=0;
    const float scale=static_cast<float>(sampleRate_/44100.0);
    auto setTap=[&](std::uint32_t l,std::uint32_t r,std::uint8_t wl,std::uint8_t wr,float fb){
        e.leftTap=static_cast<std::uint32_t>(std::lround(l*scale));e.rightTap=static_cast<std::uint32_t>(std::lround(r*scale));
        e.writeLeft=wl;e.writeRight=wr;e.feedbackBase=fb;
    };
    switch(mode){
    case Mode::SmallPlate:{const auto*d=programs::reverbs::find(programs::reverbs::Id::SmallPlate);if(!d)return false;words=d->words;count=d->wordCount;
        setTap(d->defaultLeftTap,d->defaultRightTap,static_cast<std::uint8_t>(d->leftWriteAddressGpr),static_cast<std::uint8_t>(d->rightWriteAddressGpr),d->defaultFeedback);
        e.feedbackGpr=d->feedbackGpr;e.wetGpr=d->wetGpr;e.dryGpr=d->dryGpr;
        for(std::size_t j=0;j<kSmallCoefficients.size();++j){e.coefficient[e.coefficientCount]=kSmallCoefficients[j];e.coefficientGpr[e.coefficientCount++]=static_cast<std::uint8_t>(d->leftCoefficientBase+j);e.coefficient[e.coefficientCount]=kSmallCoefficients[j];e.coefficientGpr[e.coefficientCount++]=static_cast<std::uint8_t>(d->rightCoefficientBase+j);}break;}
    case Mode::LargePlate:{const auto*d=programs::reverbs::find(programs::reverbs::Id::LargePlate);if(!d)return false;words=d->words;count=d->wordCount;
        setTap(d->defaultLeftTap,d->defaultRightTap,static_cast<std::uint8_t>(d->leftWriteAddressGpr),static_cast<std::uint8_t>(d->rightWriteAddressGpr),d->defaultFeedback);
        e.feedbackGpr=d->feedbackGpr;e.wetGpr=d->wetGpr;e.dryGpr=d->dryGpr;
        for(std::size_t j=0;j<kLargeCoefficients.size();++j){e.coefficient[e.coefficientCount]=kLargeCoefficients[j];e.coefficientGpr[e.coefficientCount++]=static_cast<std::uint8_t>(d->leftCoefficientBase+j);e.coefficient[e.coefficientCount]=kLargeCoefficients[j];e.coefficientGpr[e.coefficientCount++]=static_cast<std::uint8_t>(d->rightCoefficientBase+j);}break;}
    case Mode::Room:setTap(1050,1277,kRoomWriteL,kRoomWriteR,0.66f);words=kRoom.data();count=kRoom.size();e.feedbackGpr=40;e.wetGpr=42;e.dryGpr=43;e.coefficient[0]=kRoomCoefficient;e.coefficientGpr[0]=50;e.coefficientCount=1;break;
    case Mode::Hall:setTap(4200,5100,kHallWriteL,kHallWriteR,0.66f);words=kHall.data();count=kHall.size();e.feedbackGpr=40;e.wetGpr=42;e.dryGpr=43;e.coefficient[0]=kHallCoefficient1;e.coefficientGpr[0]=50;e.coefficient[1]=kHallCoefficient2;e.coefficientGpr[1]=51;e.coefficientCount=2;break;
    case Mode::Gated:setTap(1103,1103,kGatedPlateEnvelopeWriteL,kGatedPlateEnvelopeWriteR,0.62f);words=kGated.data();count=kGated.size();e.feedbackGpr=40;e.wetGpr=41;e.dryGpr=42;e.envGpr=43;e.coefficient[0]=0.72f;e.coefficientGpr[0]=50;e.coefficient[1]=0.42f;e.coefficientGpr[1]=51;e.coefficientCount=2;break;
    case Mode::ReverseOneShot:setTap(1103,1103,kReverseOneShotWriteL,kReverseOneShotWriteR,0.62f);words=kReverseOneShot.data();count=kReverseOneShot.size();e.feedbackGpr=40;e.wetGpr=41;e.dryGpr=42;e.envGpr=43;e.coefficient[0]=0.72f;e.coefficientGpr[0]=50;e.coefficient[1]=0.42f;e.coefficientGpr[1]=51;e.coefficientCount=2;break;
    case Mode::ReverseRetrigger:setTap(1103,1103,kReverseRetriggerWriteL,kReverseRetriggerWriteR,0.62f);words=kReverseRetrigger.data();count=kReverseRetrigger.size();e.feedbackGpr=40;e.wetGpr=41;e.dryGpr=42;e.envGpr=43;e.coefficient[0]=0.72f;e.coefficientGpr[0]=50;e.coefficient[1]=0.42f;e.coefficientGpr[1]=51;e.coefficientCount=2;break;
    case Mode::NonLin1:case Mode::NonLin2:case Mode::NonLin3:{
        const auto id=mode==Mode::NonLin1?programs::nonlin_tap::Id::NonLin1:mode==Mode::NonLin2?programs::nonlin_tap::Id::NonLin2:programs::nonlin_tap::Id::NonLin3;
        const auto*d=programs::nonlin_tap::find(id);if(!d)return false;words=d->words;count=d->wordCount;
        e.writeLeft=d->writeLeftPc;e.writeRight=d->writeRightPc;e.feedbackGpr=180;e.wetGpr=180;e.dryGpr=181;
        e.tapCount=9;e.tapLeft=d->tapLeft;e.tapRight=d->tapRight;e.defaultTapLevels=d->defaultLevels;
        for(std::size_t j=0;j<9;++j){e.tapLeftGpr[j]=static_cast<std::uint8_t>(d->readLeftPc[j]);e.tapRightGpr[j]=static_cast<std::uint8_t>(d->readRightPc[j]);e.tapLevelGpr[j]=static_cast<std::uint8_t>(150+j);}
        for(std::size_t j=0;j<d->coefficientCount;++j){e.coefficient[e.coefficientCount]=d->coefficients[j].second;e.coefficientGpr[e.coefficientCount++]=static_cast<std::uint8_t>(d->coefficients[j].first);}
        break;}
    default:return false;
    }
    // Static image load happens only in prepare, never on the audio thread.
    if(!e.cpu.loadProgram(words,count))return false;
    e.cpu.setRegister(0,e.leftTap);e.cpu.setRegister(1,e.bankSamples+e.rightTap);
    e.cpu.setRegister(e.writeLeft,0);e.cpu.setRegister(e.writeRight,e.bankSamples);
    e.cpu.setRegister(e.feedbackGpr,q22(e.feedbackBase));e.cpu.setRegister(e.wetGpr,q22(0.65f));e.cpu.setRegister(e.dryGpr,0);
    if(mode==Mode::NonLin1||mode==Mode::NonLin2||mode==Mode::NonLin3){
        e.cpu.setRegister(e.writeLeft,0);e.cpu.setRegister(e.writeRight,e.bankSamples);
        for(std::size_t j=0;j<e.tapCount;++j){e.cpu.setRegister(e.tapLeftGpr[j],e.tapLeft[j]);e.cpu.setRegister(e.tapRightGpr[j],e.bankSamples+e.tapRight[j]);e.cpu.setRegister(e.tapLevelGpr[j],q22(e.defaultTapLevels[j]*0.1f));}
    }
    for(std::size_t j=0;j<e.coefficientCount;++j)e.cpu.setRegister(e.coefficientGpr[j],q22Signed(e.coefficient[j]));
    if(mode==Mode::Room||mode==Mode::Hall){e.cpu.setRegister(4,310);e.cpu.setRegister(5,e.bankSamples+437);e.cpu.setRegister(41,q22(0.24f));e.dampingGpr=52;}
    if(mode==Mode::Gated||mode==Mode::ReverseOneShot||mode==Mode::ReverseRetrigger)e.dampingGpr=44;
    if(mode==Mode::SmallPlate||mode==Mode::LargePlate)e.dampingGpr=163;
    if(e.envGpr)e.cpu.setRegister(e.envGpr,0);
    maxInstructionCount_=std::max(maxInstructionCount_,static_cast<std::uint32_t>(count));
    return true;
}

void ReverbBank::setControls(Mode mode,Engine& e,const Parameters& p) noexcept {
    const float wet=std::clamp(sane(p.wet,0.65f),0.0f,1.0f),dry=std::clamp(sane(p.dry,0.0f),0.0f,1.0f);
    const float decay=std::clamp(sane(p.decaySeconds,1.8f),0.2f,20.0f);
    const float pre=std::clamp(sane(p.predelayMs,0.0f),0.0f,500.0f)*static_cast<float>(sampleRate_/1000.0);
    const auto l=static_cast<std::uint32_t>(std::clamp(std::lround(e.leftTap+pre),1L,static_cast<long>(e.bankSamples-2)));
    const auto r=static_cast<std::uint32_t>(std::clamp(std::lround(e.rightTap+pre),1L,static_cast<long>(e.bankSamples-2)));
    e.cpu.setRegister(0,l);e.cpu.setRegister(1,e.bankSamples+r);
    const float feedback=std::clamp(std::pow(0.001f,static_cast<float>(e.leftTap+e.rightTap)/(2.0f*static_cast<float>(sampleRate_)*decay)),0.0f,0.94f);
    e.cpu.setRegister(e.feedbackGpr,q22(feedback));e.cpu.setRegister(e.wetGpr,q22(wet));e.cpu.setRegister(e.dryGpr,q22(dry));
    const float d1=std::clamp(sane(p.diffusion1,55.0f)/55.0f,0.0f,1.4f),d2=std::clamp(sane(p.diffusion2,50.0f)/50.0f,0.0f,1.4f);
    for(std::size_t i=0;i<e.coefficientCount;++i){float scale=((i/2)%2==0)?d1:d2;const float val=std::clamp(e.coefficient[i]*scale,-0.92f,0.92f);e.cpu.setRegister(e.coefficientGpr[i],q22Signed(val));}
    if(e.dampingGpr){const float amount=std::clamp(sane(p.damping,35.0f),0.0f,99.0f)/99.0f;const float cutoff=18000.0f*std::pow(0.0555556f,amount);const float alpha=1.0f-std::exp(-2.0f*static_cast<float>(kPi)*cutoff/static_cast<float>(sampleRate_));e.cpu.setRegister(e.dampingGpr,q22Signed(alpha));if(mode==Mode::Room||mode==Mode::Hall)e.cpu.setRegister(static_cast<std::uint8_t>(e.dampingGpr+1),q22Signed(alpha));}
    if(e.tapCount){const float scale=std::clamp(decay/1.8f,0.25f,1.0f);for(std::size_t j=0;j<e.tapCount;++j){const auto tl=static_cast<std::uint32_t>(std::clamp(std::lround(e.tapLeft[j]*scale*sampleRate_/44100.0+pre),1L,static_cast<long>(e.bankSamples-2)));const auto tr=static_cast<std::uint32_t>(std::clamp(std::lround(e.tapRight[j]*scale*sampleRate_/44100.0+pre),1L,static_cast<long>(e.bankSamples-2)));e.cpu.setRegister(e.tapLeftGpr[j],tl);e.cpu.setRegister(e.tapRightGpr[j],e.bankSamples+tr);const float level=std::clamp(sane(p.tapLevels[j],e.defaultTapLevels[j]*100.0f),0.0f,99.0f)*0.001f;e.cpu.setRegister(e.tapLevelGpr[j],q22(level));}}
    if(mode==Mode::Room||mode==Mode::Hall){
        e.cpu.setRegister(4,static_cast<std::uint32_t>(std::clamp(std::lround((0.007f+pre/sampleRate_)*sampleRate_),1L,static_cast<long>(e.bankSamples-2))));
        e.cpu.setRegister(5,e.bankSamples+static_cast<std::uint32_t>(std::clamp(std::lround((0.0099f+pre/sampleRate_)*sampleRate_),1L,static_cast<long>(e.bankSamples-2))));
        e.cpu.setRegister(41,q22(std::clamp(sane(p.earlyLevel,25.0f)*0.01f,0.0f,0.99f)));
    }
}

float ReverbBank::nextEnvelope(Mode mode,Engine& e,float inL,float inR,const Parameters& p) noexcept {
    if(mode!=Mode::Gated&&mode!=Mode::ReverseOneShot&&mode!=Mode::ReverseRetrigger)return 1.0f;
    const float peak=std::max(std::abs(inL),std::abs(inR));
    const float threshold=std::pow(10.0f,std::clamp(sane(p.triggerDb,-32.0f),-96.0f,0.0f)/20.0f);
    const bool above=peak>=threshold;
    const bool edge=above&&!e.previousAbove;
    const auto hold=static_cast<std::uint64_t>(std::max(0.0f,sane(p.holdMs,80.0f))*sampleRate_/1000.0);
    const auto attack=std::max<std::uint64_t>(1,static_cast<std::uint64_t>(std::max(1.0f,sane(p.attackMs,35.0f))*sampleRate_/1000.0));
    const auto release=std::max<std::uint64_t>(1,static_cast<std::uint64_t>(std::max(1.0f,sane(p.releaseMs,240.0f))*sampleRate_/1000.0));
    if(mode==Mode::Gated){
        if(above&&(edge||p.retrigger)){e.holdRemaining=hold;}
        if(e.holdRemaining){--e.holdRemaining;e.envelope=std::min(1.0f,e.envelope+1.0f/static_cast<float>(attack));}
        else e.envelope=std::max(0.0f,e.envelope-1.0f/static_cast<float>(release));
    }else{
        const float retrigger=std::pow(10.0f,std::clamp(sane(p.retriggerDb,-28.0f),-96.0f,0.0f)/20.0f);
        const bool trigger=above&&edge&&peak>=retrigger;
        if(trigger&&(mode==Mode::ReverseRetrigger||!e.triggered)){e.triggered=true;e.envelope=0.0f;e.holdRemaining=hold;}
        if(e.triggered){
            if(e.envelope<1.0f)e.envelope=std::min(1.0f,e.envelope+1.0f/static_cast<float>(attack));
            else if(e.holdRemaining)--e.holdRemaining;
            else e.envelope=std::max(0.0f,e.envelope-1.0f/static_cast<float>(release));
        }
    }
    e.previousAbove=above;
    return std::clamp(e.envelope,0.0f,1.0f);
}

std::array<std::int16_t,2> ReverbBank::run(Mode mode,const std::array<std::int16_t,6>& input,const Parameters& p) noexcept {
    auto& e=engines_[static_cast<std::size_t>(mode)];
    const float l=fromPcm(input[4]),r=fromPcm(input[5]);
    if(e.envGpr)e.cpu.setRegister(e.envGpr,q22(nextEnvelope(mode,e,l,r,p)));
    std::array<std::int16_t,2> out{};e.cpu.processFrame(input,out);++e.frames;return out;
}

void ReverbBank::processBlock(Mode mode,const float* inL,const float* inR,float* outL,float* outR,
                              std::size_t frames,const Parameters& p) noexcept {
    if(!prepared_){for(std::size_t i=0;i<frames;++i){if(outL)outL[i]=0;if(outR)outR[i]=0;}return;}
    const auto target=static_cast<std::size_t>(mode)>=kCount?Mode::SmallPlate:mode;
    if(target!=active_&&fadeRemaining_==0){fadeFrom_=active_;fadeTo_=target;fadeTotal_=fadeRemaining_=static_cast<std::uint32_t>(sampleRate_*0.01);}
    setControls(target,engines_[static_cast<std::size_t>(target)],p);
    if(fadeRemaining_)setControls(fadeFrom_,engines_[static_cast<std::size_t>(fadeFrom_)],p);
    for(std::size_t i=0;i<frames;++i){
        const float l=inL&&std::isfinite(inL[i])?inL[i]:0.0f,r=inR&&std::isfinite(inR[i])?inR[i]:0.0f;
        const std::array<std::int16_t,6> input={0,0,0,0,toPcm(l),toPcm(r)};
        auto render=[&](Mode m){return run(m,input,p);};
        float yl=0,yr=0;
        if(fadeRemaining_){const auto a=render(fadeFrom_),b=render(fadeTo_);const float x=1.0f-static_cast<float>(fadeRemaining_)/fadeTotal_;
            yl=fromPcm(a[0])*(1-x)+fromPcm(b[0])*x;yr=fromPcm(a[1])*(1-x)+fromPcm(b[1])*x;
            if(--fadeRemaining_==0)active_=fadeTo_;
        }else{const auto y=render(active_);yl=fromPcm(y[0]);yr=fromPcm(y[1]);}
        // Safety ceiling for hot stereo material; stereo linked, fast attack,
        // no additional attenuation below the ceiling.
        const float peak=std::max(std::abs(yl),std::abs(yr));if(peak>0.98f){const float g=0.98f/peak;yl*=g;yr*=g;}
        if(outL)outL[i]=yl;
        if(outR)outR[i]=yr;
        ++processedFrames_;
    }
}

} // namespace trollfx::esp::vst::v03
