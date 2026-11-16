# Native effects path handoff

The latest Track A expansion adds ten selectable native algorithms to the
V0.2 plugin: Delay, Chorus, Flanger, Room, Hall, Plate, Modulated Reverb, VCF
Distortion, EQ Panner DDL, and Compressor. It extends the existing `AudioEngine`
and `EffectsUnit` API in `src/original/audio_engine.hpp/.cpp`.

Room/Hall use short early-reflection taps into a four-line feedback decay field.
Plate uses two serial allpass diffusion stages before the decay field. These
are original structures inferred from public DP/4 signal-flow descriptions,
not factory algorithms or bit-exact topologies. VCF Distortion approximates the
documented high-pass, envelope-modulated resonant stage and soft clipping
sequence. EQ Panner DDL combines stereo LFO panning and cross-fed echoes.
Compressor uses a feed-forward envelope detector and ratio/threshold/attack/
release defaults. Distortion-specific and dynamics-specific parameters are not
yet exposed through the host UI; no vintage parameter mapping is claimed.

The unit retains stereo delay buffers up to 10 seconds and the flanger sweep is
bounded to 0.1–15 ms. DSP allocation occurs in `prepare()`, not `process()`.

## Verification

- Engine smoke command:
  `g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror plugins/TROLLfxESPLabV02/src/audio_engine.cpp plugins/TROLLfxESPLabV02/src/engine_smoke.cpp -o /tmp/esp-lab-v02-engine-smoke && /tmp/esp-lab-v02-engine-smoke`
  Result: PASS across ten algorithms, delay/flanger bounds and finite output.
- SDK build for native Linux, ARMv7 and ARM64 succeeded.
- SDK validator: 0 errors, 0 warnings; touchscreen has five tabs and Q-Link
  maps validate for all controls.
- SDK host simulator: PASS, 20 passed, 0 warnings, 0 failures; state restore,
  32/512 blocks, in-place processing, finite output and silence checks passed.
- Desktop timing was 0.53% average of a 128-frame block (orientation only; not
  an ARM performance measurement).

## Remaining work

Add parameter smoothing, expose the algorithm-specific dynamics/distortion
controls, refine early reflection/predelay controls, and test on Force only
after deployment authorization. This is original Ensoniq-inspired DSP and does
not claim recovery of Ensoniq programs.

## Parameter catalog and V0.3 state

`research/dp4_parameter_catalog.json` records manual-visible control names and
ranges for the ten current choices, with source page and a `verified` or
`extension` label. TROLLfx-only controls are labeled extensions.

The isolated `plugins/TROLLfxESPLabV03/` plugin uses UID `Tfx3` and has 22
algorithm/unit controls plus 12 stable modulation controls per unit (2 globals
+ 4 × 34 = 138 parameters). Its modulation page identifiers are `Unit A Mod1
Enabled`, `Unit A Mod1 Source`, `Unit A Mod1 Destination`, `Unit A Mod1 Min`,
`Unit A Mod1 Max`, `Unit A Mod1 Invert`, then Mod2, repeated for B/C/D. Sources
are Off, Internal LFO and Envelope; destinations are Time, Feedback, Mix, Rate,
Depth, Diffusion, Damping, Width, Threshold, Ratio, Attack, Release and Cutoff.
Modulation updates at fixed 8-sample segments. VST chunk state round-trips all
138 parameters. Algorithms consume only relevant DSP fields; the host UI still
needs conditional visibility for unrelated controls before release.

V0.3 native, ARMv7 and ARM64 SDK builds passed; validator reported 0 errors and
0 warnings. Host simulator passed 20/20, including 32/512 frame blocks,
in-place processing, finite output and all-parameter state restore. Average
desktop timing: 1.04% of a 128-frame block, orientation only. No Force hardware
deployment or performance measurement was performed.
