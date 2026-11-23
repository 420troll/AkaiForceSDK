#include "mame_es5510_standalone.hpp"

#include <array>
#include <cstdint>
#include <iostream>

int main() {
    trollfx::esp::MameES5510 cpu;
    if (!cpu.prepare()) return 1;
    constexpr std::array<std::uint64_t, 3> program = {
        trollfx::esp::MameES5510::encode(241,238,0,0,9),
        trollfx::esp::MameES5510::encode(240,239,0,0,9),
        trollfx::esp::MameES5510::encode(0,0,0,0,15)
    };
    if (!cpu.loadProgram(program.data(),program.size())) return 2;
    cpu.setHostSerial(0x20);
    std::array<std::int16_t,6> input{}; input[4]=12345; input[5]=-23456;
    std::array<std::int16_t,2> output{};
    cpu.processFrame(input,output);
    if(output[0]!=-23456||output[1]!=12345||cpu.instructionsExecuted()!=3||cpu.frames()!=1) {
        std::cerr<<"ES5510 stereo swap failed: "<<output[0]<<", "<<output[1]<<" instructions="<<cpu.instructionsExecuted()<<'\n';return 3;
    }
    input[4]=-1;input[5]=1;cpu.processFrame(input,output);
    if(output[0]!=1||output[1]!=-1||cpu.instructionsExecuted()!=6||cpu.frames()!=2)return 4;
    constexpr std::array<std::uint64_t, 1> noEnd = {trollfx::esp::MameES5510::encode(0,0,0,0,9)};
    if(cpu.loadProgram(noEnd.data(),noEnd.size())) return 5;
    cpu.reset();
    if(cpu.frames()!=0||cpu.instructionsExecuted()!=0||cpu.registerValue(0)!=0) return 6;
    if(!cpu.loadProgram(program.data(),program.size())) return 7;
    cpu.setHostSerial(0x20); input[4]=12345; input[5]=-23456; cpu.processFrame(input,output);
    if(output[0]!=-23456||output[1]!=12345||cpu.frames()!=1||cpu.instructionsExecuted()!=3) return 8;
    std::cout<<"MAME-semantics ES5510 microprogram PASS: two frames, six ALU/END instructions, exact 16-bit serial swap\n";
    return 0;
}
