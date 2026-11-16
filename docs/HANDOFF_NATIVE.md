# Native effects path handoff

The original Track A expansion added ten selectable native algorithms to the
V0.2 engine: Delay, Chorus, Flanger, Room, Hall, Plate, Modulated Reverb, VCF
Distortion, EQ Panner DDL, and Compressor. V0.3 now uses that engine together
with the issue-fix reverb module and per-algorithm host mapping described below.

Room/Hall use short early-reflection taps into a four-line feedback decay field.
Plate uses two serial allpass diffusion stages before the decay field. These
are original structures inferred from public DP/4 signal-flow descriptions,
not factory algorithms or bit-exact topologies. VCF Distortion approximates the
documented high-pass, envelope-modulated resonant stage and soft clipping
sequence. EQ Panner DDL combines stereo LFO panning and cross-fed echoes.
Compressor uses a feed-forward envelope detector and ratio/threshold/attack/
release. V0.2 initially lacked distortion and dynamics controls; V0.3 exposes
drive/cutoff and threshold/ratio/attack/release. Other documented controls
remain partial. No factory parameter mapping is claimed where the catalog marks
an extension or the DSP uses an approximation.

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
algorithm/unit controls, 12 stable modulation controls per unit and eight
global host-automatable controller values (2 globals + 8 controllers +
4 × 34 = 146 parameters). Controller 1..8 are VST host parameters, not MIDI CC.
Each unit's Mod1/Mod2 source choices are Off, Controller 1..8, Internal LFO and
Envelope. Destinations are Time, Feedback, Mix, Rate, Depth, Diffusion, Damping,
Width, Threshold, Ratio, Attack, Release, Cutoff, Decay and Output. Modulation
updates at fixed 8-sample segments. Decay and Output map to the actual
`DP4InspiredReverb` `decaySeconds` and `outputDb` fields; the reverb tail smoke
compares fixed versus modulated tail energy and passes (0.0047241 vs 0.238261).
VST chunk state round-trips all 146 parameters.

The routing selector supports Serial, Parallel, Dual, AB Feedback 1 and AB
Feedback 2. The feedback modes are stereo 1-source approximations: B's output
feeds A through a one-host-block delay, with A's Feedback control setting the
send. Feedback 1 returns the processed AB chain; Feedback 2 blends it equally
with dry input. This does not reproduce the DP/4 physical routing or exact dry
paths. `force_routing_smoke.cpp` passes bounded-send, dry-path and bypass cases.

V0.3 native, ARMv7 and ARM64 SDK builds passed; validator reported 0 errors and
0 warnings. Host simulator passed 20/20, including 32/512 frame blocks,
in-place processing, finite output and all-parameter state restore. Average
desktop timing: 0.86% of a 128-frame block, orientation only. Generated UI has
10 tabs; SDK warns that names over eight characters may truncate and tabs past
six are not validated on hardware. Conditional algorithm visibility and
hardware UI validation remain before release. No Force deployment or ARM
hardware performance measurement occurred.
