#include "../experimental/emulation/mame_es5510_standalone.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
using trollfx::esp::MameES5510;

struct Bundle {
    std::uint8_t hostSerial = 0x20;
    std::vector<std::uint64_t> words;
    std::vector<std::pair<std::uint8_t, std::uint32_t>> registers;
    std::vector<std::pair<std::uint32_t, std::int16_t>> dram;
    struct LfoGpr {
        std::uint8_t reg;
        double base, depth, rate, phase, minimum, maximum;
    };
    std::vector<LfoGpr> lfoGpr;
};

bool loadBundle(const std::string& path, Bundle& b) {
    std::ifstream file(path);
    if (!file) return false;
    std::string line;
    bool header = false;
    while (std::getline(file, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        std::istringstream in(line);
        std::string key;
        if (!(in >> key)) continue;
        if (!header) {
            std::string version;
            if (key != "ES5510_BUNDLE" || !(in >> version) || version != "1") return false;
            header = true;
            continue;
        }
        if (key == "instruction") {
            std::uint64_t word = 0;
            if (!(in >> std::hex >> word) || word > 0xffffffffffffULL || b.words.size() >= MameES5510::kInstructionCount) return false;
            b.words.push_back(word);
        } else if (key == "host_serial") {
            unsigned value = 0;
            if (!(in >> std::setbase(0) >> value) || value > 255) return false;
            b.hostSerial = static_cast<std::uint8_t>(value);
        } else if (key == "gpr") {
            unsigned reg = 0;
            std::uint32_t value = 0;
            if (!(in >> std::setbase(0) >> reg >> std::setbase(0) >> value) || reg > 255 || value > 0xffffff) return false;
            b.registers.emplace_back(static_cast<std::uint8_t>(reg), value);
        } else if (key == "dram") {
            std::uint32_t address = 0;
            int value = 0;
            if (!(in >> std::setbase(0) >> address >> value) || address >= MameES5510::kDramWords || value < -32768 || value > 32767) return false;
            b.dram.emplace_back(address, static_cast<std::int16_t>(value));
        } else if (key == "lfo_gpr") {
            unsigned reg = 0;
            Bundle::LfoGpr lfo{};
            if (!(in >> reg >> lfo.base >> lfo.depth >> lfo.rate >> lfo.phase >> lfo.minimum >> lfo.maximum) ||
                reg >= 192 || !std::isfinite(lfo.base) || !std::isfinite(lfo.depth) ||
                !std::isfinite(lfo.rate) || !std::isfinite(lfo.phase) ||
                !std::isfinite(lfo.minimum) || !std::isfinite(lfo.maximum) ||
                lfo.minimum < 0 || lfo.maximum < lfo.minimum || lfo.maximum > 0xffffff) return false;
            lfo.reg = static_cast<std::uint8_t>(reg);
            b.lfoGpr.push_back(lfo);
        } else if (key != "sample_rate") {
            return false;
        }
    }
    if (!header || b.words.empty()) return false;
    return std::any_of(b.words.begin(), b.words.end(), [](std::uint64_t word) {
        return ((word >> 12) & 15U) == 15U;
    });
}

std::uint32_t triangle(std::uint64_t frame, unsigned phase) {
    const std::uint32_t p = static_cast<std::uint32_t>((frame + phase) % 2048);
    const std::uint32_t ramp = p < 1024 ? p : 2047 - p;
    return ramp * 48;
}

int run(const std::string& programPath, unsigned sampleRate, std::uint64_t frames,
        std::uint32_t blockFrames, bool reportProgram, std::size_t noOpWords = 160) {
    Bundle bundle;
    if (!programPath.empty()) {
        if (!loadBundle(programPath, bundle)) {
            std::cerr << "invalid ES5510 bundle: " << programPath << '\n';
            return 2;
        }
    } else {
        bundle.words.resize(noOpWords,
                             MameES5510::encode(0, 0, 0, 0, 9));
        bundle.words.back() = MameES5510::encode(0, 0, 0, 0, 15);
    }

    MameES5510 cpu;
    if (!cpu.prepare() || !cpu.loadProgram(bundle.words.data(), bundle.words.size())) return 1;
    cpu.setHostSerial(bundle.hostSerial);
    for (const auto& [reg, value] : bundle.registers) cpu.setRegister(reg, value);
    for (const auto& [address, value] : bundle.dram) cpu.setDramWord(address, value);

    std::array<std::int16_t, 6> input{};
    std::array<std::int16_t, 2> output{};
    std::int64_t checksum = 0;
    constexpr double twoPi = 6.283185307179586476925286766559;
    const auto start = std::chrono::steady_clock::now();
    for (std::uint64_t base = 0; base < frames; base += blockFrames) {
        const auto end = std::min<std::uint64_t>(frames, base + blockFrames);
        for (std::uint64_t frame = base; frame < end; ++frame) {
            for (const auto& lfo : bundle.lfoGpr) {
                const double phase = twoPi * (lfo.rate * static_cast<double>(frame) / sampleRate + lfo.phase);
                const double value = std::clamp(lfo.base + lfo.depth * std::sin(phase), lfo.minimum, lfo.maximum);
                cpu.setRegister(lfo.reg, static_cast<std::uint32_t>(std::llround(value)));
            }
            input = {};
            input[4] = static_cast<std::int16_t>(static_cast<int>(triangle(frame, 0)) - 24552);
            input[5] = static_cast<std::int16_t>(static_cast<int>(triangle(frame, 701)) - 24552);
            cpu.processFrame(input, output);
            checksum += static_cast<std::int32_t>(output[0]) + output[1];
        }
    }
    const auto stop = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(stop - start).count();
    const double audioSeconds = static_cast<double>(frames) / sampleRate;
    const double factor = elapsed / audioSeconds;
    std::cout << "sample_rate=" << sampleRate << " frames=" << frames
              << " block_frames=" << blockFrames << " program_words=" << bundle.words.size()
              << " instructions=" << cpu.instructionsExecuted()
              << " us_per_frame=" << elapsed * 1e6 / frames
              << " elapsed_s=" << elapsed << " realtime_factor=" << factor
              << "x checksum=" << checksum;
    if (reportProgram) std::cout << " program=" << programPath;
    std::cout << '\n';
    return 0;
}

void usage() {
    std::cerr << "Usage:\n"
              << "  es5510-bench FRAMES [PROGRAM_WORDS]   (legacy no-op workload)\n"
              << "  es5510-bench --rate HZ --frames N --block N [--program BUNDLE.esp]\n";
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") { usage(); return 0; }
    if (argc >= 2 && argv[1][0] != '-') {
        const std::uint64_t frames = std::stoull(argv[1]);
        const std::size_t words = argc > 2 ? std::stoul(argv[2]) : 160;
        if (words == 0 || words > MameES5510::kInstructionCount || frames == 0) return 2;
        return run({}, 44100, frames, static_cast<std::uint32_t>(std::min<std::uint64_t>(frames, 256)), false, words);
    }

    unsigned rate = 44100;
    std::uint64_t frames = 44100;
    std::uint32_t block = 256;
    std::string program;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (i + 1 >= argc) { usage(); return 2; }
        if (arg == "--rate") rate = static_cast<unsigned>(std::stoul(argv[++i]));
        else if (arg == "--frames") frames = std::stoull(argv[++i]);
        else if (arg == "--block") block = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        else if (arg == "--program") program = argv[++i];
        else { usage(); return 2; }
    }
    if (rate < 8000 || rate > 192000 || frames == 0 || block == 0) return 2;
    return run(program, rate, frames, block, !program.empty());
}
