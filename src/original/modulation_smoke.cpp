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
    s.enabled=true; s.source=ModSource::InternalLfo; s.destination=ModDestination::Decay;
    s.minNorm=.05f; s.maxNorm=.95f; m.setSlot(0,s); m.beginBlock(48000.0f,.5f); m.reset();
    m.process(0,0,base,out);
    assert(out[static_cast<unsigned>(ModDestination::Decay)]>=.05f && out[static_cast<unsigned>(ModDestination::Decay)]<=.95f);
    s.destination=ModDestination::Output; m.setSlot(0,s); m.process(0,0,base,out);
    assert(out[static_cast<unsigned>(ModDestination::Output)]>=.05f && out[static_cast<unsigned>(ModDestination::Output)]<=.95f);
    const auto saved=m.saveState();
    m.setSlot(0,ModulatorSlot{});
    assert(m.loadState(saved));
    assert(m.slot(0).enabled && m.slot(0).destination==ModDestination::Output);
    auto invalid=saved; invalid.version=999; assert(!m.loadState(invalid));
    s.source=static_cast<ModSource>(255); s.destination=static_cast<ModDestination>(255);
    s.minNorm=NAN; s.maxNorm=INFINITY; m.setSlot(0,s);
    assert(m.slot(0).source==ModSource::Off && m.slot(0).destination==ModDestination::Mix);
    assert(m.slot(0).minNorm==0.0f && m.slot(0).maxNorm==0.0f);
    ModulatorSlot host; host.enabled=true; host.source=ModSource::HostController3;
    host.destination=ModDestination::Mix; host.minNorm=.1f; host.maxNorm=.9f; m.setSlot(0,host);
    std::array<float,8> controllers{}; controllers[2]=.9f;
    m.process(0,0,controllers,base,out); assert(out[static_cast<unsigned>(ModDestination::Mix)]>.8f);
    controllers[2]=.1f; m.process(0,0,controllers,base,out);
    assert(out[static_cast<unsigned>(ModDestination::Mix)]<.2f);
    ModulatorSlot second=host; second.source=ModSource::InternalLfo;
    second.minNorm=.3f; second.maxNorm=.7f; m.setSlot(1,second);
    m.reset(); m.process(0,0,controllers,base,out);
    assert(out[static_cast<unsigned>(ModDestination::Mix)]==.5f); // slot 2 wins collision
    std::cout<<"modulation matrix: PASS (LFO audible, envelope, inversion, bounds, off, 0..99)\n";
}
