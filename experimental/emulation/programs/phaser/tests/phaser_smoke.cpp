#include "../../../mame_es5510_standalone.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr std::array<std::uint64_t, 131> kProgram = {
#include "../phaser_12stage_words.inc"
};
constexpr double kPi = 3.1415926535897932384626433832795;

std::uint32_t q22(float value) {
    long v = std::lround(static_cast<double>(value) * 4194304.0);
    v = std::clamp(v, -4194304L, 4194303L);
    return static_cast<std::uint32_t>(v) & 0xffffffu;
}

float allpassCoefficient(double cornerHz, double sampleRate) {
    // The cascade's first 50/50 cancellation notch occurs when each of 12
    // identical first-order sections contributes pi/12 radians. Solve the
    // bilinear allpass coefficient for that phase at the requested center.
    const double t = std::tan(kPi * cornerHz / sampleRate);
    const double q = std::tan(kPi / 24.0);
    return static_cast<float>((q - t) / (q + t));
}

void configure(trollfx::esp::MameES5510& cpu, double sampleRate,
               double centerHz, double stereoSpreadOctaves,
               float feedback, float wet, float dry) {
    if (!cpu.prepare() || !cpu.loadProgram(kProgram.data(), kProgram.size()))
        throw std::runtime_error("ES5510 prepare/program load failed");
    cpu.setHostSerial(0x20);
    cpu.setRegister(152, q22(feedback));
    (void)wet; (void)dry; // Host output stage performs the final wet/dry blend.
    for (int stage = 0; stage < 12; ++stage) {
        const double leftHz = std::clamp(centerHz, 30.0, sampleRate * 0.45);
        const double rightHz = std::clamp(leftHz * std::exp2(stereoSpreadOctaves), 30.0, sampleRate * 0.45);
        cpu.setRegister(static_cast<std::uint8_t>(64 + stage), q22(allpassCoefficient(leftHz, sampleRate)));
        cpu.setRegister(static_cast<std::uint8_t>(80 + stage), q22(allpassCoefficient(rightHz, sampleRate)));
    }
}

std::int16_t process(trollfx::esp::MameES5510& cpu, float left, float right,
                     std::int16_t* outRight = nullptr) {
    const auto to16 = [](float x) {
        const long v = std::lround(std::clamp(x, -1.0f, 0.999969f) * 32768.0f);
        return static_cast<std::int16_t>(v);
    };
    const std::array<std::int16_t, 6> in = {0, 0, 0, 0, to16(left), to16(right)};
    std::array<std::int16_t, 2> out{};
    cpu.processFrame(in, out);
    if (outRight) *outRight = out[1];
    return out[0];
}

float responseRms(double inputHz, double centerHz, double stereoSpread,
                  double sampleRate = 44100.0) {
    trollfx::esp::MameES5510 cpu;
    configure(cpu, sampleRate, centerHz, stereoSpread, 0.0f, 0.5f, 0.5f);
    constexpr int warmFrames = 4096;
    constexpr int measureFrames = 4096;
    double sum = 0.0;
    for (int i = 0; i < warmFrames + measureFrames; ++i) {
        const float x = 0.2f * static_cast<float>(std::sin(2.0 * kPi * inputHz * i / sampleRate));
        const float wet = static_cast<float>(process(cpu, x, x)) / 32768.0f;
        const float y = 0.5f * wet + 0.5f * x;
        if (i >= warmFrames) sum += static_cast<double>(y) * y;
    }
    return static_cast<float>(std::sqrt(sum / measureFrames));
}

void writeWav(const std::string& path, const std::vector<std::array<std::int16_t,2>>& frames,
              std::uint32_t sampleRate) {
    std::ofstream out(path, std::ios::binary);
    const auto u16 = [&out](std::uint16_t v) { out.put(static_cast<char>(v)); out.put(static_cast<char>(v >> 8)); };
    const auto u32 = [&out](std::uint32_t v) { for (int i=0;i<4;++i) out.put(static_cast<char>(v >> (8*i))); };
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(frames.size() * 4);
    out.write("RIFF",4); u32(36 + dataBytes); out.write("WAVEfmt ",8); u32(16); u16(1); u16(2);
    u32(sampleRate); u32(sampleRate * 4); u16(4); u16(16); out.write("data",4); u32(dataBytes);
    for (const auto& f : frames) { u16(static_cast<std::uint16_t>(f[0])); u16(static_cast<std::uint16_t>(f[1])); }
    if (!out) throw std::runtime_error("WAV write failed: " + path);
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string outDir = argc > 1 ? argv[1] : ".";
        std::ofstream csv(outDir + "/phaser_frequency_response.csv");
        csv << "input_hz,center_500_hz_db,center_2000_hz_db\n";
        float minDb500 = 0.0f, minDb2000 = 0.0f;
        double notch500 = 0.0, notch2000 = 0.0;
        for (int i = 0; i < 72; ++i) {
            const double inputHz = 60.0 * std::pow(12000.0 / 60.0, i / 71.0);
            const float r500 = responseRms(inputHz, 500.0, 0.0);
            const float r2000 = responseRms(inputHz, 2000.0, 0.0);
            const float db500 = 20.0f * std::log10(std::max(r500, 1.0e-9f) / 0.2f);
            const float db2000 = 20.0f * std::log10(std::max(r2000, 1.0e-9f) / 0.2f);
            csv << inputHz << ',' << db500 << ',' << db2000 << '\n';
            if (inputHz > 100 && inputHz < 10000 && db500 < minDb500) { minDb500=db500; notch500=inputHz; }
            if (inputHz > 100 && inputHz < 10000 && db2000 < minDb2000) { minDb2000=db2000; notch2000=inputHz; }
        }
        if (!csv) throw std::runtime_error("response CSV write failed");

        const float at500 = responseRms(500.0, 500.0, 0.0);
        const float at2000 = responseRms(2000.0, 2000.0, 0.0);
        const float at500Db = 20.0f * std::log10(std::max(at500, 1.0e-9f) / 0.2f);
        const float at2000Db = 20.0f * std::log10(std::max(at2000, 1.0e-9f) / 0.2f);
        // The 500 Hz notch is narrow; measure precisely at the requested center
        // in addition to the coarse logarithmic response sweep.
        if (at500Db > -18.0f || at2000Db > -18.0f)
            throw std::runtime_error("expected moving allpass cancellation notch was not deep");

        // Dynamic center sweep plus stereo phase spread demonstration.
        constexpr double fs = 44100.0;
        constexpr int framesCount = 44100 * 4;
        trollfx::esp::MameES5510 moving;
        configure(moving, fs, 1000.0, 0.25, 0.65f, 0.65f, 0.35f);
        std::vector<std::array<std::int16_t,2>> audio;
        audio.reserve(framesCount);
        for (int n = 0; n < framesCount; ++n) {
            const double t = static_cast<double>(n) / fs;
            const double center = 900.0 * std::exp2(1.0 * std::sin(2.0 * kPi * 0.18 * t));
            for (int stage = 0; stage < 12; ++stage) {
                const double leftHz = std::clamp(center, 30.0, fs*0.45);
                const double rightHz = std::clamp(leftHz * std::exp2(0.25), 30.0, fs*0.45);
                moving.setRegister(static_cast<std::uint8_t>(64+stage), q22(allpassCoefficient(leftHz,fs)));
                moving.setRegister(static_cast<std::uint8_t>(80+stage), q22(allpassCoefficient(rightHz,fs)));
            }
            const float x = (n % 44100 == 0) ? 0.35f : 0.0f;
            std::int16_t right = 0;
            const auto left = process(moving, x, x, &right);
            const float dry = x;
            audio.push_back({static_cast<std::int16_t>(std::lround(0.65f*left+0.35f*dry*32768.0f)),
                             static_cast<std::int16_t>(std::lround(0.65f*right+0.35f*dry*32768.0f))});
        }
        writeWav(outDir + "/phaser_center_sweep.wav", audio, 44100);
        double stereoSum=0.0, leftEnergy=0.0, rightEnergy=0.0;
        for (const auto& frame : audio) {
            const double l=frame[0], r=frame[1];
            stereoSum += l*r; leftEnergy += l*l; rightEnergy += r*r;
        }
        const double stereoCorrelation=stereoSum/std::sqrt(leftEnergy*rightEnergy);
        if (!(std::abs(stereoCorrelation) < 0.9999))
            throw std::runtime_error("stereo spread did not create distinct channel motion");

        // Positive and negative feedback must remain finite and avoid rail lock.
        for (float fb : {0.85f, -0.85f}) {
            trollfx::esp::MameES5510 cpu;
            configure(cpu, 44100.0, 1000.0, 0.2, fb, 1.0f, 0.0f);
            unsigned clips = 0;
            for (int n=0; n<44100*2; ++n) {
                const auto y = process(cpu, n==0?0.25f:0.0f, n==0?0.25f:0.0f);
                if (y == 32767 || y == -32768) ++clips;
            }
            if (clips != 0) throw std::runtime_error("feedback stability test clipped");
        }
        std::cout << "program_words=" << kProgram.size() << " allpass_stages_per_channel=12"
                  << " response_notch_500_center_hz=" << notch500 << " depth_db=" << minDb500
                  << " response_notch_2000_center_hz=" << notch2000 << " depth_db=" << minDb2000
                  << " response_at_center_500_db=" << at500Db << " response_at_center_2000_db=" << at2000Db
                  << " feedback_pm_0.85_clips=0 stereo_correlation=" << stereoCorrelation << "\n";
        if (!(notch500 > 0 && notch2000 > 0 && std::abs(std::log2(notch2000/notch500)) > 0.25))
            throw std::runtime_error("center control did not move response minima enough");
    } catch (const std::exception& e) {
        std::cerr << "phaser test failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
