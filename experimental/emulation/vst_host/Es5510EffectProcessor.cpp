// Copyright (c) 2026 TROLLfx contributors.
//
// The two instruction images below are original TROLLfx programs. The CPU
// implementation they execute is MAME-derived; retain the BSD-3-Clause
// attribution in experimental/emulation/THIRD_PARTY_NOTICES.md.
#include "Es5510EffectProcessor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace trollfx::esp::vst {
namespace {

constexpr std::array<std::uint64_t, 19> kStereoDelayProgram = {
    0xffffef0a9000ULL, 0xffffee0b9000ULL,
    0x280e000c9c30ULL, 0x280f000d9c30ULL,
    0xffff0a109030ULL, 0xffff0b119030ULL,
    0xffff0e100030ULL, 0xffff0f110030ULL,
    0xffff10009508ULL, 0xffff11009508ULL,
    0x290c0a149030ULL, 0x290d0b159030ULL,
    0x2a14ffff4030ULL, 0x2a15ffff4030ULL,
    0xffff0c140030ULL, 0xffff0d150030ULL,
    0xffff14f19030ULL, 0xffff15f09030ULL,
    0xfffffffff000ULL
};

constexpr std::array<std::uint64_t, 11> kStereoChorusProgram = {
    0xffffef0a9000ULL, 0xffffee0b9000ULL,
    0x280e000c9c30ULL, 0x280f000d9c30ULL,
    0xffff0a009508ULL, 0xffff0b009508ULL,
    0xffff0e0a0030ULL, 0xffff0f0b0030ULL,
    0xffff0af19030ULL, 0xffff0bf09030ULL,
    0xfffffffff000ULL
};

constexpr float kDefaultDelayMs = 250.0f;
constexpr float kDefaultFeedback = 0.55f;
constexpr float kDefaultWet = 0.5f;
constexpr float kDefaultDry = 0.5f;
constexpr float kDefaultChorusRate = 0.5f;
constexpr float kDefaultChorusDepthMs = 5.0f;
constexpr double kTwoPi = 6.283185307179586476925286766559;

inline float finiteOr(float value, float fallback) noexcept {
    return std::isfinite(value) ? value : fallback;
}

} // namespace

void Es5510EffectProcessor::SmoothedValue::setImmediate(float v) noexcept {
    value = target = v;
    step = 0.0f;
    remaining = 0;
}

void Es5510EffectProcessor::SmoothedValue::setTarget(
    float v, std::uint32_t rampFrames) noexcept {
    if (v == target) return;
    target = v;
    if (rampFrames == 0 || value == target) {
        setImmediate(target);
        return;
    }
    remaining = rampFrames;
    step = (target - value) / static_cast<float>(remaining);
}

float Es5510EffectProcessor::SmoothedValue::next() noexcept {
    if (remaining != 0) {
        value += step;
        if (--remaining == 0) {
            value = target;
            step = 0.0f;
        }
    }
    return value;
}

bool Es5510EffectProcessor::supportedRate(double rate,
                                           std::uint32_t& integerRate) noexcept {
    if (!std::isfinite(rate)) return false;
    const auto rounded = static_cast<std::uint32_t>(std::lround(rate));
    if (std::abs(rate - static_cast<double>(rounded)) > 0.01) return false;
    if (rounded != 44100 && rounded != 48000) return false;
    integerRate = rounded;
    return true;
}

float Es5510EffectProcessor::sanitize(float value, float fallback,
                                       float minimum, float maximum) noexcept {
    value = finiteOr(value, fallback);
    return std::clamp(value, minimum, maximum);
}

std::uint32_t Es5510EffectProcessor::q22(float value) noexcept {
    const auto rounded = std::lround(static_cast<double>(value) * 4194304.0);
    return static_cast<std::uint32_t>(std::clamp<long>(rounded, 0, 0x400000));
}

std::int16_t Es5510EffectProcessor::toPcm16(float sample) noexcept {
    if (!std::isfinite(sample)) return 0;
    if (sample <= -1.0f) return static_cast<std::int16_t>(-32768);
    if (sample >= 1.0f) return static_cast<std::int16_t>(32767);
    const auto q = std::lround(static_cast<double>(sample) * 32768.0);
    return static_cast<std::int16_t>(std::clamp<long>(q, -32768, 32767));
}

float Es5510EffectProcessor::fromPcm16(std::int16_t sample) noexcept {
    return static_cast<float>(sample) * (1.0f / 32768.0f);
}

void Es5510EffectProcessor::configureEngine(
    MameES5510& engine, const std::uint64_t* program,
    std::size_t programWords, std::uint32_t capacitySamples,
    std::uint32_t sampleRate, bool delayProgram,
    const Parameters& parameters) noexcept {
    const auto ringWords = capacitySamples * 2u;
    const auto dLength = ringWords - 1u;
    if (!engine.loadProgram(program, programWords)) return;
    engine.setHostSerial(0x20);
    engine.setRegister(244, 0);             // MEMSIZ: one GPR unit per DRAM word
    engine.setRegister(245, dLength);       // ring length; END advances DBASE
    engine.setRegister(248, 0);             // DBASE begins at zero

    const std::uint32_t baseDelay = std::max<std::uint32_t>(
        1, static_cast<std::uint32_t>(std::lround(sampleRate * 0.025)));
    if (delayProgram) {
        const std::uint32_t delayFrames = std::clamp<std::uint32_t>(
            static_cast<std::uint32_t>(std::lround(parameters.delayMs * sampleRate / 1000.0)),
            1, capacitySamples - 2);
        engine.setRegister(8, 0);
        engine.setRegister(9, capacitySamples);
        engine.setRegister(0, delayFrames);
        engine.setRegister(1, capacitySamples + delayFrames);
        engine.setRegister(40, q22(parameters.feedback));
        engine.setRegister(41, q22(parameters.wet));
        engine.setRegister(42, q22(parameters.dry));
    } else {
        engine.setRegister(4, 0);
        engine.setRegister(5, capacitySamples);
        engine.setRegister(0, baseDelay);
        engine.setRegister(1, capacitySamples + baseDelay);
        engine.setRegister(40, q22(parameters.wet));
    }
}

bool Es5510EffectProcessor::prepare(double sampleRate,
                                    std::size_t maximumBlockSize) noexcept {
    prepared_ = false;
    std::uint32_t integerRate = 0;
    if (!supportedRate(sampleRate, integerRate) || maximumBlockSize == 0) {
        sampleRate_ = 0.0;
        maximumBlockSize_ = 0;
        return false;
    }

    const auto capacity = static_cast<std::uint32_t>(
        std::lround(static_cast<double>(integerRate) * kMaxDelaySeconds));
    if (capacity * 2u > MameES5510::kDramWords) return false;

    if (!delayEngine_.prepare() || !chorusEngine_.prepare()) return false;
    sampleRateInteger_ = integerRate;
    capacitySamples_ = capacity;
    Parameters defaults{};
    if (!delayEngine_.loadProgram(kStereoDelayProgram.data(), kProgramDelayWords) ||
        !chorusEngine_.loadProgram(kStereoChorusProgram.data(), kProgramChorusWords)) {
        return false;
    }
    delayEngine_.setHostSerial(0x20);
    chorusEngine_.setHostSerial(0x20);
    for (MameES5510* engine : {&delayEngine_, &chorusEngine_}) {
        engine->setRegister(244, 0);
        engine->setRegister(245, capacity * 2u - 1u);
        engine->setRegister(248, 0);
    }

    delayEngine_.setRegister(8, 0);
    delayEngine_.setRegister(9, capacity);
    delayEngine_.setRegister(0, static_cast<std::uint32_t>(
        std::lround(integerRate * (kDefaultDelayMs / 1000.0))));
    delayEngine_.setRegister(1, capacity + delayEngine_.registerValue(0));
    delayEngine_.setRegister(40, q22(kDefaultFeedback));
    delayEngine_.setRegister(41, q22(kDefaultWet));
    delayEngine_.setRegister(42, q22(kDefaultDry));

    chorusEngine_.setRegister(4, 0);
    chorusEngine_.setRegister(5, capacity);
    const auto chorusBase = static_cast<std::uint32_t>(std::lround(integerRate * 0.025));
    chorusEngine_.setRegister(0, chorusBase);
    chorusEngine_.setRegister(1, capacity + chorusBase);
    chorusEngine_.setRegister(40, q22(kDefaultWet));

    sampleRateInteger_ = integerRate;
    sampleRate_ = static_cast<double>(integerRate);
    maximumBlockSize_ = maximumBlockSize;
    capacitySamples_ = capacity;
    processedFrames_ = 0;
    chorusPhaseCycles_ = 0.0;
    parametersInitialized_ = false;
    modeMix_ = modeTarget_ = 0.0f;
    modeStep_ = 0.0f;
    modeRampRemaining_ = 0;
    bypassMix_ = bypassTarget_ = 0.0f;
    bypassStep_ = 0.0f;
    bypassRampRemaining_ = 0;
    delaySamples_.setImmediate(kDefaultDelayMs);
    feedback_.setImmediate(kDefaultFeedback);
    delayWet_.setImmediate(kDefaultWet);
    delayDry_.setImmediate(kDefaultDry);
    chorusRate_.setImmediate(kDefaultChorusRate);
    chorusDepthSamples_.setImmediate(
        static_cast<float>(integerRate) * kDefaultChorusDepthMs / 1000.0f);
    chorusWet_.setImmediate(kDefaultWet);
    prepared_ = true;
    return true;
}

void Es5510EffectProcessor::initializeEngines(
    const Parameters& parameters) noexcept {
    delayEngine_.reset();
    chorusEngine_.reset();
    configureEngine(delayEngine_, kStereoDelayProgram.data(),
                    kProgramDelayWords, capacitySamples_, sampleRateInteger_,
                    true, parameters);
    configureEngine(chorusEngine_, kStereoChorusProgram.data(),
                    kProgramChorusWords, capacitySamples_, sampleRateInteger_,
                    false, parameters);
}

void Es5510EffectProcessor::reset() noexcept {
    if (!prepared_) return;
    Parameters defaults{};
    initializeEngines(defaults);
    processedFrames_ = 0;
    chorusPhaseCycles_ = 0.0;
    parametersInitialized_ = false;
    modeMix_ = modeTarget_ = 0.0f;
    modeStep_ = 0.0f;
    modeRampRemaining_ = 0;
    bypassMix_ = bypassTarget_ = 0.0f;
    bypassStep_ = 0.0f;
    bypassRampRemaining_ = 0;
    delaySamples_.setImmediate(kDefaultDelayMs);
    feedback_.setImmediate(kDefaultFeedback);
    delayWet_.setImmediate(kDefaultWet);
    delayDry_.setImmediate(kDefaultDry);
    chorusRate_.setImmediate(kDefaultChorusRate);
    chorusDepthSamples_.setImmediate(
        static_cast<float>(sampleRateInteger_) * kDefaultChorusDepthMs / 1000.0f);
    chorusWet_.setImmediate(kDefaultWet);
}

void Es5510EffectProcessor::applyBlockParameters(
    const Parameters& parameters) noexcept {
    const std::uint32_t rampFrames = std::max<std::uint32_t>(
        1, static_cast<std::uint32_t>(std::lround(sampleRate_ * kSmoothingSeconds)));
    const float delay = sanitize(parameters.delayMs, kDefaultDelayMs, 1.0f, 10000.0f);
    const float feedback = sanitize(parameters.feedback, kDefaultFeedback, 0.0f, 0.94f);
    const float wet = sanitize(parameters.wet, kDefaultWet, 0.0f, 1.0f);
    const float dry = sanitize(parameters.dry, kDefaultDry, 0.0f, 1.0f);
    const float rate = sanitize(parameters.rateHz, kDefaultChorusRate, 0.0f, 10.0f);
    const float depthMs = sanitize(parameters.depthMs, kDefaultChorusDepthMs, 0.0f, 24.0f);
    const float depthSamples = depthMs * static_cast<float>(sampleRate_) / 1000.0f;

    const float requestedMode = parameters.mode == Mode::Chorus ? 1.0f : 0.0f;
    const float requestedBypass = parameters.bypass ? 1.0f : 0.0f;
    if (!parametersInitialized_) {
        delaySamples_.setImmediate(delay);
        feedback_.setImmediate(feedback);
        delayWet_.setImmediate(wet);
        delayDry_.setImmediate(dry);
        chorusRate_.setImmediate(rate);
        chorusDepthSamples_.setImmediate(depthSamples);
        chorusWet_.setImmediate(wet);
        modeMix_ = modeTarget_ = requestedMode;
        modeRampRemaining_ = 0;
        bypassMix_ = bypassTarget_ = requestedBypass;
        bypassRampRemaining_ = 0;
        parametersInitialized_ = true;
        return;
    }

    delaySamples_.setTarget(delay, rampFrames);
    feedback_.setTarget(feedback, rampFrames);
    delayWet_.setTarget(wet, rampFrames);
    delayDry_.setTarget(dry, rampFrames);
    chorusRate_.setTarget(rate, rampFrames);
    chorusDepthSamples_.setTarget(depthSamples, rampFrames);
    chorusWet_.setTarget(wet, rampFrames);

    if (requestedMode != modeTarget_) {
        modeTarget_ = requestedMode;
        modeRampRemaining_ = rampFrames;
        modeStep_ = (modeTarget_ - modeMix_) / static_cast<float>(rampFrames);
    }
    if (requestedBypass != bypassTarget_) {
        bypassTarget_ = requestedBypass;
        bypassRampRemaining_ = rampFrames;
        bypassStep_ = (bypassTarget_ - bypassMix_) / static_cast<float>(rampFrames);
    }
}

float Es5510EffectProcessor::nextModeMix() noexcept {
    if (modeRampRemaining_ != 0) {
        modeMix_ += modeStep_;
        if (--modeRampRemaining_ == 0) {
            modeMix_ = modeTarget_;
            modeStep_ = 0.0f;
        }
    }
    return modeMix_;
}

float Es5510EffectProcessor::nextBypassMix() noexcept {
    if (bypassRampRemaining_ != 0) {
        bypassMix_ += bypassStep_;
        if (--bypassRampRemaining_ == 0) {
            bypassMix_ = bypassTarget_;
            bypassStep_ = 0.0f;
        }
    }
    return bypassMix_;
}

void Es5510EffectProcessor::processBlock(
    const float* inputLeft, const float* inputRight,
    float* outputLeft, float* outputRight, std::size_t frames,
    const Parameters& parameters) noexcept {
    if (!prepared_) {
        for (std::size_t i = 0; i < frames; ++i) {
            if (outputLeft) outputLeft[i] = 0.0f;
            if (outputRight) outputRight[i] = 0.0f;
        }
        return;
    }

    applyBlockParameters(parameters);
    const double sampleRate = sampleRate_;
    const auto rateIncrement = 1.0 / sampleRate;
    const float defaultChorusBase = 0.025f * static_cast<float>(sampleRate_);

    for (std::size_t i = 0; i < frames; ++i) {
        const float inL = inputLeft ? finiteOr(inputLeft[i], 0.0f) : 0.0f;
        const float inR = inputRight ? finiteOr(inputRight[i], 0.0f) : 0.0f;
        const std::int16_t pcmL = toPcm16(inL);
        const std::int16_t pcmR = toPcm16(inR);
        const auto delayFrames = static_cast<std::uint32_t>(std::clamp<long>(
            std::lround(static_cast<double>(delaySamples_.next()) * sampleRate / 1000.0),
            1, static_cast<long>(capacitySamples_ - 2)));
        delayEngine_.setRegister(0, delayFrames);
        delayEngine_.setRegister(1, capacitySamples_ + delayFrames);
        delayEngine_.setRegister(40, q22(feedback_.next()));
        delayEngine_.setRegister(41, q22(delayWet_.next()));
        delayEngine_.setRegister(42, q22(delayDry_.next()));

        const double phase = kTwoPi * chorusPhaseCycles_;
        const double depth = static_cast<double>(chorusDepthSamples_.next());
        const double offset = static_cast<double>(defaultChorusBase) +
                              depth * std::sin(phase);
        const double oppositeOffset = static_cast<double>(defaultChorusBase) +
                                     depth * std::sin(phase + kTwoPi * 0.5);
        const auto chorusOffsetL = static_cast<std::uint32_t>(std::clamp<long>(
            std::lround(offset), 1, static_cast<long>(capacitySamples_ - 2)));
        const auto chorusOffsetR = static_cast<std::uint32_t>(std::clamp<long>(
            std::lround(oppositeOffset), 1, static_cast<long>(capacitySamples_ - 2)));
        chorusEngine_.setRegister(0, chorusOffsetL);
        chorusEngine_.setRegister(1, capacitySamples_ + chorusOffsetR);
        chorusEngine_.setRegister(40, q22(chorusWet_.next()));
        chorusPhaseCycles_ += static_cast<double>(chorusRate_.next()) * rateIncrement;
        if (chorusPhaseCycles_ >= 1.0) chorusPhaseCycles_ -= std::floor(chorusPhaseCycles_);

        const std::array<std::int16_t, 6> bus = {0, 0, 0, 0, pcmL, pcmR};
        std::array<std::int16_t, 2> delayOut{};
        std::array<std::int16_t, 2> chorusOut{};
        delayEngine_.processFrame(bus, delayOut);
        chorusEngine_.processFrame(bus, chorusOut);

        const float mode = nextModeMix();
        const float bypass = nextBypassMix();
        const float effectL = fromPcm16(delayOut[0]) * (1.0f - mode) +
                              fromPcm16(chorusOut[0]) * mode;
        const float effectR = fromPcm16(delayOut[1]) * (1.0f - mode) +
                              fromPcm16(chorusOut[1]) * mode;
        const float outL = effectL * (1.0f - bypass) + inL * bypass;
        const float outR = effectR * (1.0f - bypass) + inR * bypass;
        if (outputLeft) outputLeft[i] = outL;
        if (outputRight) outputRight[i] = outR;
        ++processedFrames_;
    }
}

} // namespace trollfx::esp::vst
