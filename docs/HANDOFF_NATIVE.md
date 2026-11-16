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
