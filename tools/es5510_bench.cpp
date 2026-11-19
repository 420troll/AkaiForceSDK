#include "../experimental/emulation/mame_es5510_standalone.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc,char**argv){
    const std::uint64_t frames=argc>1?std::stoull(argv[1]):44100;
    const std::size_t instructionCount=argc>2?static_cast<std::size_t>(std::stoul(argv[2])):160u;
    if(instructionCount<1||instructionCount>trollfx::esp::MameES5510::kInstructionCount)return 2;
    trollfx::esp::MameES5510 cpu;if(!cpu.prepare())return 1;
    std::array<std::uint64_t,trollfx::esp::MameES5510::kInstructionCount> p{};
    p.fill(trollfx::esp::MameES5510::encode(0,0,0,0,9));
    p[instructionCount-1]=trollfx::esp::MameES5510::encode(0,0,0,0,15);
    if(!cpu.loadProgram(p.data(),instructionCount)) return 2;
    cpu.setHostSerial(0x20);
    std::array<std::int16_t,6> in{};std::array<std::int16_t,2> out{};in[4]=123;in[5]=-456;
    const auto start=std::chrono::steady_clock::now();
    for(std::uint64_t i=0;i<frames;++i)cpu.processFrame(in,out);
    const auto stop=std::chrono::steady_clock::now();const double seconds=std::chrono::duration<double>(stop-start).count();
    const double realtime=seconds*44100.0/static_cast<double>(frames);
    std::cout<<"frames="<<frames<<" program_words="<<instructionCount<<" instructions="<<cpu.instructionsExecuted()<<" elapsed_s="<<seconds<<" us_per_frame="<<seconds*1e6/frames<<" realtime_factor="<<realtime<<"x checksum="<<int(out[0])+int(out[1])<<'\n';
}
