#include "TempoDelayControl.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

using trollfx::esp::vst::Dp4Division;
using trollfx::esp::vst::TempoDelayControl;
using trollfx::esp::vst::TempoDelayRequest;
using trollfx::esp::vst::TempoSource;

int main() {
    constexpr std::array<double, 12> expectedBeats = {
        0.125, 1.0/6.0, 0.25, 0.375, 1.0/3.0, 0.5,
        0.75, 2.0/3.0, 1.0, 1.5, 4.0/3.0, 2.0
    };
    constexpr std::array<double, 3> tempos = {50.0, 120.0, 250.0};
    constexpr std::array<double, 2> rates = {44100.0, 48000.0};

    for (std::size_t d = 0; d < expectedBeats.size(); ++d) {
        for (double bpm : tempos) {
            for (double rate : rates) {
                TempoDelayRequest req;
                req.sync = true;
                req.source = TempoSource::Internal;
                req.division = static_cast<Dp4Division>(d);
                req.internalBpm = bpm;
                req.sampleRate = rate;
                req.maxDelaySamples = static_cast<std::uint32_t>(rate * 10.0);
                const auto out = TempoDelayControl::calculate(req);
                const double exactMs = 60000.0 * expectedBeats[d] / bpm;
                const auto expectedSamples = static_cast<std::uint32_t>(std::floor(exactMs * rate / 1000.0 + 0.5));
                assert(out.synchronized);
                assert(!out.usedHostTempo);
                assert(!out.usedInternalFallback);
                assert(std::abs(out.requestedDelayMs - exactMs) < 1e-10);
                assert(out.delaySamples == expectedSamples);
            }
        }
    }

    TempoDelayRequest host;
    host.sync = true;
    host.source = TempoSource::Host;
    host.division = Dp4Division::EighthDotted;
    host.internalBpm = 100.0;
    host.hostBpm = 120.0;
    host.hostTempoValid = true;
    auto out = TempoDelayControl::calculate(host);
    assert(out.usedHostTempo && !out.usedInternalFallback);
    assert(out.effectiveBpm == 120.0);
    assert(std::abs(out.requestedDelayMs - 375.0) < 1e-10);

    for (double invalid : {0.0, -10.0, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(), 1001.0}) {
        host.hostBpm = invalid;
        auto fallback = TempoDelayControl::calculate(host);
        assert(!fallback.usedHostTempo && fallback.usedInternalFallback);
        assert(fallback.effectiveBpm == 100.0);
        assert(std::abs(fallback.requestedDelayMs - 450.0) < 1e-10);
    }
    host.hostTempoValid = false;
    auto unavailable = TempoDelayControl::calculate(host);
    assert(unavailable.usedInternalFallback && unavailable.effectiveBpm == 100.0);

    TempoDelayRequest unsynced;
    unsynced.sync = false;
    unsynced.freeDelayMs = 1234.567;
    unsynced.sampleRate = 48000.0;
    unsynced.maxDelaySamples = 480000;
    auto free = TempoDelayControl::calculate(unsynced);
    assert(!free.synchronized);
    assert(free.requestedDelayMs == unsynced.freeDelayMs);
    assert(free.delaySamples == 59259);

    TempoDelayRequest clamped;
    clamped.sync = true;
    clamped.source = TempoSource::Internal;
    clamped.division = Dp4Division::Half;
    clamped.internalBpm = 50.0;
    clamped.sampleRate = 44100.0;
    clamped.maxDelaySamples = 100000;
    auto capped = TempoDelayControl::calculate(clamped);
    assert(capped.requestedDelayMs == 2400.0);
    assert(capped.delaySamples == 100000);

    for (std::size_t i = 0; i < expectedBeats.size(); ++i)
        assert(TempoDelayControl::division(static_cast<Dp4Division>(i)).quarterNumerator != 0);
}
