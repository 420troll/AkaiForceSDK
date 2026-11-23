#include "es5510.h"
#include "../../experimental/emulation/mame_es5510_standalone.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static std::uint64_t instruction(unsigned a, unsigned b, unsigned op) {
    return (std::uint64_t(a) << 16) | (std::uint64_t(b) << 24) |
           (std::uint64_t(op) << 12);
}
static void install(ES5510& cpu, unsigned index, std::uint64_t word) {
    for (unsigned n=0; n<6; ++n)
        cpu.host_w(3+n, static_cast<std::uint8_t>(word >> ((5-n)*8)));
    cpu.host_w(0xc0, static_cast<std::uint8_t>(index));
}
static void set_gpr(ES5510& cpu, std::uint8_t reg, std::uint32_t value) {
    cpu.host_w(0, static_cast<std::uint8_t>(value >> 16));
    cpu.host_w(1, static_cast<std::uint8_t>(value >> 8));
    cpu.host_w(2, static_cast<std::uint8_t>(value));
    cpu.host_w(0xa0, reg);
}

static void render(const std::string& mode, bool delay, const std::string& input,
                   const std::string& output) {
    const std::uint64_t delayProgram[] = {
        0xffffef0a9000ULL, 0xffffee0b9000ULL, 0x280e000c9c30ULL,
        0x280f000d9c30ULL, 0xffff0e0a0030ULL, 0xffff0f0b0030ULL,
        0xffff0a009508ULL, 0xffff0b009508ULL, 0xffff0af19030ULL,
        0xffff0bf09030ULL, 0xfffffffff000ULL
    };
    const std::uint64_t passProgram[] = {
        instruction(241,235,9), instruction(240,234,9), 0xf000ULL
    };
    const bool original = mode == "original";
    if (!original && mode != "mame") throw std::runtime_error("unknown engine");
    ES5510 reference;
    trollfx::esp::MameES5510 adapter;
    if (!original && !adapter.prepare()) throw std::runtime_error("adapter prepare failed");
    const auto* program = delay ? delayProgram : passProgram;
    const std::size_t count = delay ? 11 : 3;
    if (original) {
        for (std::size_t i=0; i<count; ++i) install(reference, static_cast<unsigned>(i), program[i]);
        if (delay) {
            // Same startup state on both engines: MEMSIZ 0, empty DRAM,
            // 882000-word ring, 11025-frame stereo delay, 55% feedback.
            set_gpr(reference,244,0); set_gpr(reference,245,0x0d754f); set_gpr(reference,248,0);
            set_gpr(reference,0,0x002b11); set_gpr(reference,1,0x06e5b9);
            set_gpr(reference,6,0); set_gpr(reference,7,0x06baa8); set_gpr(reference,40,0x233333);
        }
        reference.host_w(0x18,0x20);
    } else {
        if (!adapter.loadProgram(program,count)) throw std::runtime_error("adapter program load failed");
        if (delay) {
            adapter.setRegister(244,0); adapter.setRegister(245,0x0d754f); adapter.setRegister(248,0);
            adapter.setRegister(0,0x002b11); adapter.setRegister(1,0x06e5b9);
            adapter.setRegister(6,0); adapter.setRegister(7,0x06baa8); adapter.setRegister(40,0x233333);
        }
        adapter.setHostSerial(0x20);
    }
    std::ifstream in(input); std::ofstream out(output);
    if (!in || !out) throw std::runtime_error("cannot open stimulus or output CSV");
    out << "in_l,in_r,out_l,out_r\n";
    int left, right;
    while (in >> left) {
        char comma;
        if (!(in >> comma) || comma != ',' || !(in >> right) ||
            left < -32768 || left > 32767 || right < -32768 || right > 32767)
            throw std::runtime_error("malformed stereo stimulus CSV");
        if (original) {
            reference.ser_w(delay?4:0, static_cast<std::int16_t>(left));
            reference.ser_w(delay?5:1, static_cast<std::int16_t>(right));
            reference.run_frame();
            out << left << ',' << right << ',' << reference.ser_r(6) << ',' << reference.ser_r(7) << '\n';
        } else {
            std::array<std::int16_t,6> inputFrame{};
            inputFrame[delay?4:0]=static_cast<std::int16_t>(left);
            inputFrame[delay?5:1]=static_cast<std::int16_t>(right);
            std::array<std::int16_t,2> outputFrame{};
            adapter.processFrame(inputFrame,outputFrame);
            out << left << ',' << right << ',' << outputFrame[0] << ',' << outputFrame[1] << '\n';
        }
    }
    if (!in.eof() || !out.good()) throw std::runtime_error("I/O failure during audio comparison");
}

int main(int argc, char** argv) {
    try {
        if (argc != 5) {
            std::cerr << "usage: es5510-audio-compare original|mame pass|delay INPUT.csv OUTPUT.csv\n";
            return 2;
        }
        const std::string algorithm=argv[2];
        if (algorithm != "pass" && algorithm != "delay") return 2;
        render(argv[1],algorithm=="delay",argv[3],argv[4]);
    } catch (const std::exception& e) {
        std::cerr << "comparison error: " << e.what() << '\n'; return 1;
    }
    return 0;
}
