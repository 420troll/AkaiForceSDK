# TROLLfx ESP Lab V0.3 handoff

V0.3 is a separate plugin identity (`Tfx3`) at
`plugins/TROLLfxESPLabV03/`; V0.1 and V0.2 projects and packages remain
available. Latest main source milestone: `36b47c3` before this handoff update.

## Implemented

- Four stereo units with per-unit algorithm, mix, bypass, serial/parallel/dual
  routing, and stereo AB Feedback 1/2 approximations. The feedback paths use a
  one-host-block return delay and distinct dry mixes; they are not exact DP/4
  physical routing.
- Ten algorithms with individual control mappings: Delay, Chorus, Flanger,
  Room, Hall, Plate, Modulated Reverb, VCF Distortion, EQ-Panner-DDL and
  Compressor. The catalog separates manually verified controls from TROLLfx
  extensions in `research/dp4_parameter_catalog.json`.
- Separate room/hall early-reflection and decay-field paths, plate diffusion,
  explicit RT60/predelay/damping/diffusion/width controls, and wet output gain.
- Two persisted modulation slots per unit. Sources are Off, host-automatable
  Controller 1–8, internal LFO and envelope; the latter two are TROLLfx
  extensions. Destinations include Time, Feedback, Mix, Rate, Depth, Diffusion,
  Damping, Width, Threshold, Ratio, Attack, Release, Cutoff, Decay and Output.
  Modulated decay/output reaches the live reverb parameters. MIDI CC is not
  exposed because the SDK host does not promise MIDI delivery to insert FX.
- Original amber/charcoal rack skin, four-unit panel artwork, ROUTING, UNIT A–D,
  MOD A–D and CTRL SRC pages, plus generated Q-Link mappings. TUI has ten tabs;
  SDK warns navigation beyond six Force function keys is unverified. Panel
  image is a background; SDK controls remain functional over it.

## Verification

- Native, ARMv7/Cortex-A17 and ARM64 builds succeeded using the official SDK at
  `febd2ca5bd9eb16c4a412837196e145566bc981b` and Zig 0.17.1.
- SDK validation: 0 errors, 0 warnings; 146 parameter handles, ten TUI tabs,
  ten Q-Link maps and background image validated.
- Host simulator: 20/20 passed, including parameter sweep, chunk round-trip for
  all 146 parameters, in-place processing, finite output and 32/512-frame
  blocks. Desktop average was 1.25% of a 128-frame block; not an ARM benchmark.
- DSP smoke tests passed with `-Wall -Wextra -Wpedantic -Werror`: modulation,
  four-unit feedback routing, and reverb tails at 0.1/0.5/1 second. A fixed vs
  modulated reverb decay test measured tail energy 0.0047241 vs 0.238261.
- Hardware UI fit and ARM performance remain unverified; no deployment was
  performed.

## Package

ZIP: `plugins/TROLLfxESPLabV03/dist/esp-lab-v0-3-0.3.0-mpc.zip`

SHA-256: `aaee7c24a49a3fc9512ded6336db5cb548bdfb75092ea0b3d21cb44d602ed46e`

Install command: extract the ZIP, then copy its `TROLLfx - VST - ESP Lab V0.3`
folder into `<Force drive>/VST2 Plugins/`. Do not install over V0.1 or V0.2.

## Known limitations / next work

- Several verified manual parameters (full reflection sends, position balances,
  EQ bands and compressor sidechain filters) remain in the catalog but are not
  yet wired to the V0.3 surface.
- Ten tabs exceed the SDK's six-function-key navigation validation; verify on
  Force before a hardware session.
- Reverb and algorithm controls update in 8-sample segments without complete
  parameter smoothing. Improve where measured artifacts occur.
- Preserve the evidence boundary: this is original DP/4-inspired DSP, not
  recovered factory microcode or bit-exact DP/4 execution.
