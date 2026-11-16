#include "modulation.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using namespace trollfx;
    ModulationMatrix m; m.beginBlock(48000.0f, 1.0f);
    ModulationMatrix::Values base{}, out{}; base.fill(.25f);
    m.process(0,0,base,out); assert(out[static_cast<unsigned>(ModDestination::Mix)]==.25f);
    ModulatorSlot s; s.enabled=true; s.source=ModSource::InternalLfo;
    s.destination=ModDestination::Mix; s.minNorm=.2f; s.maxNorm=.8f; m.setSlot(0,s);
    bool varied=false;
    for(int i=0;i<48000;++i){m.process(0,0,base,out); const float v=out[2]; assert(v>=.2f&&v<=.8f); if(std::fabs(v-.25f)>.05f)varied=true;}
    assert(varied);
    for(int i=0;i<12000;++i)m.process(0,0,base,out);
    s.invert=true; m.setSlot(0,s); ModulationMatrix::Values inv{}; m.process(0,0,base,inv);
    assert(inv[2]<.21f);
    s.source=ModSource::Envelope; s.destination=ModDestination::Feedback; s.minNorm=.1f; s.maxNorm=.9f; s.invert=false; m.setSlot(0,s); m.reset();
    for(int i=0;i<4000;++i)m.process(.8f,.8f,base,out);
    assert(out[static_cast<unsigned>(ModDestination::Feedback)]>.72f);
    s.enabled=false; m.setSlot(0,s); m.process(1,1,base,out); assert(out[static_cast<unsigned>(ModDestination::Feedback)]==base[1]);
    assert(ModulationMatrix::toDisplay99(ModulationMatrix::fromDisplay99(73))==73);
    s.source=ModSource::HostController3; s.destination=ModDestination::Mix; s.minNorm=.1f; s.maxNorm=.9f; s.invert=false; s.enabled=true; m.setSlot(0,s);
    std::array<float,8> controllers{}; controllers[2]=.9f; m.process(0,0,controllers,base,out); assert(out[2]>.8f);
    controllers[2]=.1f; m.process(0,0,controllers,base,out); assert(out[2]<.2f);
    s.enabled=true; s.source=ModSource::InternalLfo; s.destination=ModDestination::Decay;
    s.minNorm=.05f; s.maxNorm=.95f; m.setSlot(0,s); m.beginBlock(48000.0f,.5f); m.reset();
    m.process(0,0,base,out);
    assert(out[static_cast<unsigned>(ModDestination::Decay)]>=.05f && out[static_cast<unsigned>(ModDestination::Decay)]<=.95f);
    s.destination=ModDestination::Output; m.setSlot(0,s); m.process(0,0,base,out);
    assert(out[static_cast<unsigned>(ModDestination::Output)]>=.05f && out[static_cast<unsigned>(ModDestination::Output)]<=.95f);
    s.source=ModSource::HostController3; s.destination=ModDestination::Mix; s.minNorm=.1f; s.maxNorm=.9f; s.invert=false; s.enabled=true; m.setSlot(0,s);
    std::array<float,8> controllers{}; controllers[2]=.9f; m.process(0,0,controllers,base,out); assert(out[2]>.8f);
    controllers[2]=.1f; m.process(0,0,controllers,base,out); assert(out[2]<.2f);
    std::cout<<"modulation matrix: PASS (LFO audible, envelope, inversion, bounds, off, 0..99)\n";
}
