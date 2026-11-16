# ES5510 tempo clock API findings

## SDK evidence

The pinned official MPC Plugin Builder SDK checkout is `febd2ca5bd9eb16c4a412837196e145566bc981b`. Its `framework/mpc_plugin.h` defines `mpc::Transport` with `valid`, `tempoValid`, `bpm`, PPQ position and play state. Every plugin `process(ProcessContext&)` receives this as `ctx.transport`. The VST2 wrapper requests `kVstTempoValid` through `audioMasterGetTime`; it sets `tempoValid` only when the returned `VstTimeInfo.flags` includes that bit and copies `VstTimeInfo.tempo` to `ctx.transport.bpm`.

The SDK's `tools/host-sim/host_sim.cpp` supplies a fixed 120 BPM transport in its simulator. That proves the SDK adapter path and allows plugin simulation; it does not prove that an actual Force host reports its current tempo to this VST on hardware. No Force V0.2 plugin run has verified delivery yet. The UI must be able to show host tempo unavailable and use internal BPM until hardware confirms delivery.

## Division mapping

The DP/4 Tempo Delay manual's 12 values are represented as exact quarter-note beat ratios:

| DP/4 division | Quarter-note beats |
|---|---:|
| 1/32 | 1/8 |
| 1/16 triplet | 1/6 |
| 1/16 | 1/4 |
| 1/16 dotted | 3/8 |
| 1/8 triplet | 1/3 |
| 1/8 | 1/2 |
| 1/8 dotted | 3/4 |
| 1/4 triplet | 2/3 |
| 1/4 | 1 |
| 1/4 dotted | 3/2 |
| 1/2 triplet | 4/3 |
| 1/2 | 2 |

Delay milliseconds are `60000 * beats / BPM`; sample count is rounded to nearest sample, then bounded to the configured delay RAM capacity. Internal BPM is clamped to the original DP/4 documented 50–250 BPM range. A valid positive finite host BPM (up to 1000 BPM) is used without changing it; missing or invalid host tempo falls back to the clamped internal BPM. The helper is stateless and intentionally does not retain stale host BPM.

Sync OFF passes the free-running millisecond value through exactly; only the derived sample address is rounded and clamped. No sample-position phase alignment is implemented: this helper provides tempo-derived interval sync, not transport-start/retrigger sync.

## API and tests

`experimental/emulation/vst_host/TempoDelayControl.hpp/.cpp` provides `TempoDelayControl::calculate()`. It does no allocation, callback, file, parser or logging work and only returns requested milliseconds, bounded sample count, BPM source evidence, and fallback flags. Integrators should write the result's sample count to the delay read-offset GPRs; the ES5510 program continues to perform all audio and feedback processing. Changing that GPR preserves delay RAM; glide/crossfade policy belongs to the host adapter.

Run the dependency-free tests:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/vst_host/TempoDelayControl.cpp \
  experimental/emulation/vst_host/TempoDelayControl_test.cpp \
  -o /tmp/tempo-delay-control-test
/tmp/tempo-delay-control-test
```

Coverage includes all 12 exact ratios at 50/120/250 BPM and 44.1/48 kHz, host BPM validity and fallback, capacity clamping, and sync-off pass-through. These are helper tests; no original Force host tempo delivery is asserted.
