#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace trollfx {

enum class ReverbKind { Room, Hall, Plate, Modulated };

struct ReverbParams {
    float decaySeconds = 2.0f;
    float preDelayMs = 0.0f;
    float diffusion = 0.65f;
    float damping = 0.35f;
    float earlyLevel1 = 0.25f;
    float earlyLevel2 = 0.15f;
    float earlyTime1Ms = 17.0f;
    float earlyTime2Ms = 31.0f;
    float width = 1.0f;
    float mix = 1.0f;
    float outputDb = 0.0f;
    float modRateHz = 0.25f;
    float modDepthMs = 0.7f;
};

// Original, manual-informed reverb circuits. The room/hall input path has
// predelay, two independently timed early taps and a four-line decay field.
// Plate adds two serial allpass diffusion stages. This is not factory code.
class DP4InspiredReverb {
public:
    bool prepare(float sampleRate) {
        if (!(sampleRate >= 8000.0f && sampleRate <= 192000.0f)) return false;
        sr_ = sampleRate;
        const std::size_t preN = static_cast<std::size_t>(sr_ * 0.51f) + 4;
        const std::size_t lengths[4] = {
            static_cast<std::size_t>(sr_ * 0.0311f) + 1,
            static_cast<std::size_t>(sr_ * 0.0377f) + 1,
            static_cast<std::size_t>(sr_ * 0.0419f) + 1,
            static_cast<std::size_t>(sr_ * 0.0487f) + 1};
        const std::size_t diff[4] = {
            static_cast<std::size_t>(sr_ * 0.0031f) + 1,
            static_cast<std::size_t>(sr_ * 0.0047f) + 1,
            static_cast<std::size_t>(sr_ * 0.0067f) + 1,
            static_cast<std::size_t>(sr_ * 0.0091f) + 1};
        try {
            preL_.assign(preN, 0.0f); preR_.assign(preN, 0.0f);
            for (int i = 0; i < 4; ++i) { tank_[i].assign(lengths[i], 0.0f); diff_[i].assign(diff[i], 0.0f); }
        } catch (...) { return false; }
        reset();
        return true;
    }

    void setKind(ReverbKind kind) noexcept { kind_ = kind; }
    void setParams(const ReverbParams& p) noexcept {
        p_.decaySeconds = std::clamp(p.decaySeconds, 0.2f, 60.0f);
        p_.preDelayMs = std::clamp(p.preDelayMs, 0.0f, 500.0f);
        p_.diffusion = std::clamp(p.diffusion, 0.0f, 1.0f);
        p_.damping = std::clamp(p.damping, 0.0f, 0.98f);
        p_.earlyLevel1 = std::clamp(p.earlyLevel1, 0.0f, 1.0f);
        p_.earlyLevel2 = std::clamp(p.earlyLevel2, 0.0f, 1.0f);
        p_.earlyTime1Ms = std::clamp(p.earlyTime1Ms, 0.0f, 120.0f);
        p_.earlyTime2Ms = std::clamp(p.earlyTime2Ms, 0.0f, 120.0f);
        p_.width = std::clamp(p.width, 0.0f, 1.0f);
        p_.mix = std::clamp(p.mix, 0.0f, 1.0f);
        p_.outputDb = std::clamp(p.outputDb, -24.0f, 12.0f);
        p_.modRateHz = std::clamp(p.modRateHz, 0.01f, 8.0f);
        p_.modDepthMs = std::clamp(p.modDepthMs, 0.0f, 2.0f);
    }

    void reset() noexcept {
        std::fill(preL_.begin(), preL_.end(), 0.0f); std::fill(preR_.begin(), preR_.end(), 0.0f);
        for (auto& x : tank_) std::fill(x.begin(), x.end(), 0.0f);
        for (auto& x : diff_) std::fill(x.begin(), x.end(), 0.0f);
        prePos_ = 0; phase_ = 0.0f; std::fill(damp_, damp_ + 4, 0.0f);
        std::fill(pos_, pos_ + 4, 0); std::fill(diffPos_, diffPos_ + 4, 0);
    }

    void process(const float* inL, const float* inR, float* outL, float* outR,
                 std::size_t n) noexcept {
        if (preL_.empty() || !inL || !inR || !outL || !outR) return;
        constexpr float pi = 3.14159265358979323846f;
        const float angleInc = 2.0f * pi * p_.modRateHz / sr_;
        const float outGain = std::pow(10.0f, p_.outputDb / 20.0f);
        const float lengths[4] = {float(tank_[0].size()), float(tank_[1].size()), float(tank_[2].size()), float(tank_[3].size())};
        const float loopSeconds = (lengths[0] + lengths[1] + lengths[2] + lengths[3]) / (4.0f * sr_);
        const float decayGain = std::min(0.9995f, std::pow(10.0f, -3.0f * loopSeconds / p_.decaySeconds));
        const bool roomFamily = kind_ == ReverbKind::Room || kind_ == ReverbKind::Hall;
        for (std::size_t i = 0; i < n; ++i) {
            const float dryL = inL[i], dryR = inR[i];
            const float preSamp = std::clamp(p_.preDelayMs * .001f * sr_, 1.0f, float(preL_.size() - 2));
            const float predL = read(preL_, prePos_, preSamp), predR = read(preR_, prePos_, preSamp);
            const float early1L = read(preL_, prePos_, std::clamp(p_.earlyTime1Ms * .001f * sr_, 1.0f, float(preL_.size() - 2)));
            const float early2R = read(preR_, prePos_, std::clamp(p_.earlyTime2Ms * .001f * sr_, 1.0f, float(preR_.size() - 2)));
            preL_[prePos_] = dryL; preR_[prePos_] = dryR; if (++prePos_ == preL_.size()) prePos_ = 0;
            float xL = predL, xR = predR;
            if (roomFamily) { xL += p_.earlyLevel1 * early1L; xR += p_.earlyLevel2 * early2R; }
            else if (kind_ == ReverbKind::Plate) { xL = diffuse(0, xL); xL = diffuse(1, xL); xR = diffuse(2, xR); xR = diffuse(3, xR); }
            const float mod = kind_ == ReverbKind::Modulated ? std::sin(phase_) * p_.modDepthMs * .001f * sr_ : 0.0f;
            phase_ += angleInc; if (phase_ >= 2.0f * pi) phase_ -= 2.0f * pi;
            float d[4], v[4], mean = 0.0f;
            for (int k = 0; k < 4; ++k) {
                d[k] = read(tank_[k], pos_[k], std::max(1.0f, lengths[k] - 1.0f + mod));
                // Damping is in the feedback signal path: this is the issue #1 fix.
                const float dampCoeff = std::clamp(1.0f - p_.damping * 0.22f, 0.78f, 1.0f);
                v[k] = damp_[k] + (d[k] - damp_[k]) * dampCoeff;
                damp_[k] = v[k]; mean += v[k];
            }
            mean *= 0.25f;
            const float input = 0.5f * (xL + xR) * (0.35f + 0.2f * p_.diffusion);
            float wetL = 0.0f, wetR = 0.0f;
            for (int k = 0; k < 4; ++k) {
                const float sign = (k & 1) ? -1.0f : 1.0f;
                const float injected = input * sign;
                tank_[k][pos_[k]] = injected + (2.0f * mean - v[k]) * decayGain;
                if (++pos_[k] == tank_[k].size()) pos_[k] = 0;
                if (k == 0 || k == 2) wetL += d[k]; else wetR += d[k];
            }
            wetL *= 0.5f; wetR *= 0.5f;
            const float mid = 0.5f * (wetL + wetR), side = 0.5f * (wetL - wetR) * p_.width;
            wetL = (mid + side) * outGain; wetR = (mid - side) * outGain;
            outL[i] = dryL + (wetL - dryL) * p_.mix;
            outR[i] = dryR + (wetR - dryR) * p_.mix;
        }
    }

private:
    static float read(const std::vector<float>& b, std::size_t pos, float delay) noexcept {
        if (b.size() < 2) return 0.0f;
        delay = std::clamp(delay, 1.0f, float(b.size() - 1));
        float p = float(pos) - delay; if (p < 0.0f) p += float(b.size());
        const auto i = static_cast<std::size_t>(p), j = (i + 1) % b.size();
        return b[i] + (b[j] - b[i]) * (p - float(i));
    }
    float diffuse(int ix, float x) noexcept {
        const float g = 0.25f + 0.55f * p_.diffusion;
        const float delayed = read(diff_[ix], diffPos_[ix], float(diff_[ix].size() - 1));
        const float y = delayed - g * x;
        diff_[ix][diffPos_[ix]] = x + g * y;
        if (++diffPos_[ix] == diff_[ix].size()) diffPos_[ix] = 0;
        return y;
    }
    float sr_ = 44100.0f, phase_ = 0.0f;
    ReverbKind kind_ = ReverbKind::Room;
    ReverbParams p_{};
    std::vector<float> preL_, preR_;
    std::vector<float> tank_[4], diff_[4];
    std::size_t prePos_ = 0, pos_[4]{}, diffPos_[4]{};
    float damp_[4]{};
};

} // namespace trollfx
