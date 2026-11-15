# Handoff 001 — recovery and research baseline

Date: 2026-11-15 UTC

## Executive decision

Start production with an **original modular delay/modulation architecture**,
not a cycle-exact emulator and not a claimed DP/4 clone. Build a reusable
fractional stereo delay, filtering, smoothing, LFO and routing core. This is
the fastest low-risk route to authentic *behavioral vocabulary* across the
EPS-16/ASR/DP families while meeting original Force CPU constraints.

Run ES5510 instruction emulation as a separate research track. MAME makes the
ES5510 the best emulation starting point; ESP2 is architecturally richer but
lacks comparable public emulator coverage.

## 1. What was already known/recovered

- The local development machine has substantial TROLLfx Force plugin work
  under `Desktop/akaiVibe`, but no Ensoniq/ES5510/DP4 textual research was
  found in the inspected trees.
- Neither `TROLLfx-DP4-Recovery` nor `TROLLfx-Force-SDK` existed locally before
  this pass.
- The connected `420troll` GitHub installation exposed only
  `TROLLfx-Spatial-Studio` for the TROLLfx search. Public/exact searches did
  not find the two named repositories.
- There were therefore no prior algorithm claims, tests or source references
  to merge. This handoff preserves that negative result so later agents do not
  repeat the same filesystem/account search blindly.

## 2. New evidence discovered

### Architecture

- Public ES5510 v2.4 and ES5511/ESP2 technical specifications were located and
  hashed. The ESP2 document is text-readable and establishes a 96-bit, three-
  function-unit architecture with 24-bit operands, 10-bit register fields,
  eight-region modulo AGEN, 24-bit external memory bus and eight stereo serial
  lines.
- The original ESP is concretely represented by MAME as a compact 24-bit
  fixed-point processor with 48-bit instructions, 160 program positions,
  parallel ALU/MAC/memory actions, 16-bit delay memory and four stereo serial
  pairs.
- Product scope is broader than DP/4: current MAME drivers map the architecture
  across VFX/SD/SQ/KS, EPS-16, ASR-10 and KT/TS systems; ASR-X/MR identify
  ESP2/ES5511; the DP/Pro manual explicitly identifies two ESP2 processors.

### Algorithms and routing

- DP/4 documentation confirms four independent processing units, serial and
  parallel routing, and ganged multi-unit algorithms.
- DP/Pro documentation publishes 42 algorithm slots and rich parameter tables.
  It confirms specific high-level structures such as a 32-tap variable delay
  with four diffusion stages, eight spatial delays, six-stage/eight-delay
  expert reverb, and six-voice pitch shifting with splice controls.
- These are behavior/topology clues, not recovered microcode. Exact matrices,
  coefficients, delay lengths and nonlinearities remain unverified.

### Emulator state

- MAME commit `8be57cdc5f44ab830dff44c06dd39f24190742d4` was inspected directly under
  BSD-3-Clause headers.
- Recent fixes show active convergence but also invalidate any assumption that
  the model has always been bit-perfect. DRAM size, I/O and DOL rounding retain
  explicit uncertainties.
- A public EPS-16 Plus emulator reports extensive original-OS integration and
  an ES5510 core adapted from MAME. It does not distribute ROM/OS images, but
  no top-level repository license was found, so its original code is not a
  reuse source at present.

### Akai SDK

- Official SDK commit `febd2ca5bd9eb16c4a412837196e145566bc981b` was cloned and inspected.
- The API is a header-only C++17 VST2 framework with normalized typed
  parameters, chunk state, transport/MIDI context and JSON touchscreen/Q-Link
  skins.
- The original Force target is ARMv7 hard-float/Cortex-A17. The SDK also emits
  ARM64 and native builds.
- Native StereoDelay host simulation passed 20/20 checks. Full native + ARM32
  + ARM64 build and validation completed with 0 errors and 0 warnings. No
  hardware installation was performed.

## 3. What remains unverified

- `ES5100` has no supporting Ensoniq evidence in this pass.
- Original EPS board-level ESP identity is not sufficiently established.
- Exact chip revisions, product DRAM configurations and DOL rounding behavior.
- ESP2/ES5511 execution emulation and precise ASR-X/MR/DP-Pro silicon details.
- Any exact Ensoniq factory algorithm microcode, coefficient set or topology.
- Hardware-measured parameter curves, interpolation/windowing, saturation and
  extreme feedback behavior.
- Actual runtime, UI and Q-Link behavior on the user's original Force.

See `OPEN_QUESTIONS.md` for the actionable list.

## 4. Best practical starting architecture

For a product: a native C++ graph with contiguous floating-point delay memory,
fractional interpolation, filtered feedback/cross-feedback, control smoothing,
stereo modulation and explicit serial/parallel routes. This is portable,
testable, efficient and does not bind product delivery to proprietary images or
instruction-level overhead.

For emulation research: ES5510 using MAME as a BSD-licensed reference oracle.
Do not put the emulator in the production audio path until conformance,
performance and licensing are demonstrated.

## 5. Realistic first algorithms

1. Clean stereo/dual/tempo delay.
2. Chorus, vibrato and flanger variants reusing fractional delay + LFO.
3. Tremolo/panner and parametric/shelving EQ.
4. Phaser and basic dynamics.
5. Diffusion/FDN room reverb after delay primitives and tests are mature.
6. Pitch shifting only after a dedicated window/splice prototype and CPU
   measurements; it is not a first milestone.

## 6. Next concrete coding task

Create a small C++17 DSP library and native tests for `FractionalDelay`:

- preallocated power-of-two or explicitly wrapped stereo buffer;
- integer and linear interpolation modes, then a tested optional Lagrange mode;
- smoothed delay changes without clicks;
- bounded read/write indices for every 32–512 frame block;
- impulse and swept-delay golden renders;
- feedback safety/finite recovery tests; and
- zero allocation in `process()`.

Wrap it in the SDK as `TROLLfx Mod Delay Lab` only after the DSP tests pass.
Expose eight first-page controls in two coherent Q-Link groups: Time, Feedback,
Mod Rate, Mod Depth; Low Cut, High Cut, Route, Mix. Add Sync/Division on a
second page or by conditional visibility without breaking group-of-four layout.

## Reproducibility log

Key commands (temporary research checkouts, no Force access):

```sh
git clone https://github.com/inmusic-software/mpc-plugin-builder-sdk.git /tmp/mpc-plugin-builder-sdk
git clone --depth 1 --filter=blob:none --sparse https://github.com/mamedev/mame.git /tmp/mame-es5510

python3 tools/build.py examples/StereoDelay --target native --toolchain gcc --verbose
.sdk-tools/host_sim examples/StereoDelay/build/native/stereo-delay.so test \
  --seconds 2 --wav /tmp/stereo-delay-test.wav

PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py examples/StereoDelay --target all --toolchain zig \
  --zig /usr/bin/python3 --verbose
```

Recorded failures:

- SDK automatic Zig bootstrap: failed because `ensurepip`/`python3-venv` was
  unavailable.
- First temporary Zig run: failed on read-only `/home/gamer/.cache/zig`.
- ARM32-only run: binary succeeded; validation failed only because manifest
  still declared absent ARM64.
- Native-only direct validator: failed because a deployable output folder had
  not been assembled.

Workarounds and final results are documented in `SDK_COMPATIBILITY.md`.

## Repository/push status

The handoff was prepared in local repository
`/mnt/c/Users/Gamer/TROLLfx-Force-SDK`. GitHub repository creation is not
available through the connected GitHub tool, and local `gh` reports an invalid
token for `420troll`. If `420troll/TROLLfx-Force-SDK` is created or access is
restored, add it as `origin` and push the recorded local commit. No other
repository or Force plugin was modified.

