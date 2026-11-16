#include "es5510_minimal.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using trollfx::esp::ES5510Minimal;
int main() {
    ES5510Minimal cpu;
    cpu.setRegister(6,0x7fffff); cpu.setRegister(5,1);
    const std::uint64_t add[]={ES5510Minimal::encode(6,5,20,21,0),ES5510Minimal::encode(0,0,0,0,15)};
    assert(cpu.loadProgram(add,2)); cpu.step(); assert(cpu.registerValue(6)==-8388608);
    cpu.step(); assert(cpu.halted());
    cpu.reset(); cpu.setRegister(6,0x400000); cpu.setRegister(4,0x400000);
    const std::uint64_t mac[]={ES5510Minimal::encode(8,9,6,4,9),ES5510Minimal::encode(0,0,0,0,15)};
    assert(cpu.loadProgram(mac,2)); cpu.step(); assert(cpu.accumulator()==(std::int64_t(0x400000)*0x400000));
    cpu.reset(); const std::uint64_t audio[]={ES5510Minimal::encode(2,0,0,0,9),ES5510Minimal::encode(3,1,0,0,9),ES5510Minimal::encode(0,0,0,0,15)};
    assert(cpu.loadProgram(audio,3)); float l=0,r=0; cpu.processFrame(0.25f,-0.5f,l,r,2);
    assert(std::fabs(l-0.25f)<1e-6f && std::fabs(r+0.5f)<1e-6f);
    std::cout<<"ES5510 minimal core: PASS\n";
}
