// Deterministic, MAME-source-derived vectors for the standalone ES5510 adapter.
// These are not a runtime differential test against a MAME binary. See
// MAME_SOURCE_VECTORS.md for provenance and the remaining differential work.
#include "mame_es5510_standalone.hpp"

#include <array>
#include <cstdint>
#include <iostream>

using Cpu = trollfx::esp::MameES5510;

static bool run(Cpu& cpu, std::initializer_list<std::uint64_t> words) {
    const auto* begin = words.begin();
    if (!cpu.loadProgram(begin, words.size())) return false;
    std::array<std::int16_t,6> in{};
    std::array<std::int16_t,2> out{};
    cpu.processFrame(in,out);
    return true;
}

int main() {
    Cpu cpu;
    if (!cpu.prepare()) return 1;
    const auto end = Cpu::encode(0,0,0,0,15);

    // MAME ALU opcode 0: signed saturating add. (0x7fffff)+1 saturates.
    cpu.setRegister(1,0x7fffff); cpu.setRegister(2,1);
    if (!run(cpu,{Cpu::encode(1,2,0,0,0),end})) return 2;
    if (cpu.registerValue(1)!=0x7fffff || cpu.registerValue(250)!=0x200000) {
        std::cerr<<"saturating add mismatch result="<<std::hex<<cpu.registerValue(1)
                 <<" CCR="<<cpu.registerValue(250)<<'\n'; return 3;
    }

    // Opcode 2 is wrapping 24-bit add; overflow flags remain visible.
    cpu.setRegister(1,0x7fffff); cpu.setRegister(2,1);
    if (!run(cpu,{Cpu::encode(1,2,0,0,2),end})) return 4;
    if (cpu.registerValue(1)!=0x800000 || cpu.registerValue(250)!=0xa00000) {
        std::cerr<<"wrapping add mismatch result="<<std::hex<<cpu.registerValue(1)
                 <<" CCR="<<cpu.registerValue(250)<<'\n'; return 5;
    }

    // MAC uses signed 24-bit operands and Q-format shift x4; a quarter-scale
    // product returns quarter scale in the high 24-bit MAC view after a cycle.
    cpu.setRegister(5,0x400000); cpu.setRegister(6,0x400000);
    if (!run(cpu,{Cpu::encode(0,0,5,6,9),end})) return 6;
    if (cpu.registerValue(5)!=0x400000 || cpu.registerValue(243)!=0x400000) {
        std::cerr<<"MAC pipeline mismatch GPR5="<<std::hex<<cpu.registerValue(5)
                 <<" MACHI="<<cpu.registerValue(243)<<'\n'; return 7;
    }

    // A true CCR/CMR condition plus the skippable bit suppresses a MOV write.
    cpu.setRegister(3,0x123456); cpu.setRegister(4,0x654321);
    cpu.setRegister(250,0x800000); cpu.setRegister(251,0x800000);
    if (!run(cpu,{Cpu::encode(3,4,0,0,9,0,0,true),end})) return 8;
    if (cpu.registerValue(3)!=0x123456) {
        std::cerr<<"conditional skip failed: GPR3="<<std::hex<<cpu.registerValue(3)<<'\n'; return 9;
    }
    std::cout<<"MAME-source-derived ES5510 vectors PASS: saturated/wrapping ALU, CCR, MAC pipeline, conditional skip (not a runtime MAME differential)\n";
    return 0;
}
