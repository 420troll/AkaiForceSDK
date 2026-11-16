# Experimental ES5510 execution handoff

## Stage: standalone instruction-to-audio harness

This experimental path is separate from the native TROLLfx effects plugin.
Its first standalone processor and renderer are in
`experimental/emulation/mame_es5510_standalone.*` and `tools/es5510_render.cpp`.
The engine is an isolated C++ adapter of MAME's ES5510 execution model, not a
link against the MAME application. MAME attribution and BSD-3-Clause terms are
in `experimental/emulation/THIRD_PARTY_NOTICES.md`.

### What runs

- The loader accepts a local text bundle containing up to 160 48-bit
  microinstruction words, sample-rate metadata, Host Serial Control, optional
  GPR initialization and optional delay-RAM initialization. It rejects
  malformed records and programs without an END instruction.
- The standalone core executes the parallel ALU/MAC pipeline, 24-bit GPR and
  special-register paths, 48-bit MAC state/overflow, condition flags and
  skip, serial registers, DOL FIFO, delayed delay-RAM operations, END and the
  pump's per-frame run lifecycle. It uses a bounded 160-word instruction
  budget per frame so a bad local bundle cannot hang the renderer.
- The audio harness uses the MAME VFX pump mapping: stereo lanes for FX1, FX2
  and dry on serial ports 0–2, processed stereo on port 3. The current tool
  feeds its input WAV to the dry pair and writes the processed pair.
- `tools/es5510_render.cpp` reads stereo 16-bit PCM WAV and writes stereo
  16-bit PCM WAV. `experimental/emulation/programs/original_stereo_swap.esp`
  is an original hand-authored test: MOV dry-right to output-left, MOV
  dry-left to output-right, END. It contains no Ensoniq microcode.

### Reproduce

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/mame_es5510_standalone_test.cpp \
  -o /tmp/es5510-standalone-test
/tmp/es5510-standalone-test

g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp tools/es5510_render.cpp \
  -o /tmp/es5510-render
/tmp/es5510-render --make-impulse /tmp/es5510-in.wav 44100 4
/tmp/es5510-render experimental/emulation/programs/original_stereo_swap.esp \
  /tmp/es5510-in.wav /tmp/es5510-out.wav
```

Benchmark command:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp tools/es5510_bench.cpp \
  -o /tmp/es5510-bench
/tmp/es5510-bench 44100 160
```

### Results and confidence

| Check | Result | What it establishes |
|---|---|---|
| Native build, warnings as errors | PASS | standalone engine, loader/renderer and benchmark compile |
| Instruction smoke | PASS | two 3-word program frames exchange left/right dry lanes exactly; 6 instructions counted |
| 44.1 kHz, 4-second impulse render | PASS | 176,400 frames, 529,200 executed words, nonzero output; peak 24,576 / 32,767 |
| ASan/UBSan | PASS | instruction smoke and four-second render completed without sanitizer reports |
| Native benchmark, 89 words/frame | PASS | median of 3 runs: 44,100 frames in 0.109474 s; 2.48239 µs/frame, or 0.10947 s of host time per audio second |
| Native benchmark, 160 words/frame | PASS | median of 3 runs: 44,100 frames in 0.162326 s; 3.68087 µs/frame, or 0.16233 s of host time per audio second |
| ARMv7/Cortex-A17 build | PASS | hard-float ARM EABI5 renderer and benchmark binaries cross-compile with Zig 0.17.1 |
| ARMv7 runtime benchmark | NOT MEASURED | `qemu-arm` is unavailable and package installation was denied; no Force hardware timing is claimed |
| MAME differential run | NOT DONE | the original MAME device was inspected as source but a full MAME host runtime was not built for a differential vector |
| Historical Ensoniq effect-program run | BLOCKED | no licensed user-supplied effect program/state bundle is available in this workspace |
| Force hardware run | NOT DONE | no deployment or hardware test occurred |

The original test output WAV is
`artifacts/es5510-emulation/original_stereo_swap_impulse.wav`. The second
execution vector rendered `artifacts/es5510-emulation/mame_pump_summed_delay_chord.wav`:
13 instructions per frame, 1,719,900 total; peak 23,304/32,767 and RMS 0.09258
full scale. Its output exactly matches the six input serial lanes summed and
delayed by one frame (NRMSE 0.0), exercising the MAME fixture's delay-RAM path.
This remains synthetic MAME test code, not a historical effect program. Its first
output frame is the input's stereo pair swapped (L=8192, R=24576); the rest
of the 4-second impulse render is silence. It demonstrates real instruction
execution and serial routing, not an Ensoniq effect sound or a
production-quality effect. The program has 3 instructions (2 MOV plus END);
program and audio SHA-256 values are recorded in
`artifacts/es5510-emulation/RESULTS.txt`.

The stripped ARMv7 test kit is
`artifacts/es5510-emulation/es5510-armv7-testkit.zip` (SHA-256
`4f29c52d73aa4ddc575525b7cdea9c896d0465d77a98d1c2c7650e6e367918ac`). It
contains the standalone renderer, benchmark, original stereo-swap program,
MAME pump summing/delay fixture, and license notice. After manually copying it to a compatible ARMv7 Linux environment,
unzip and run `./es5510-bench 44100 89` or `./es5510-bench 44100 160` to obtain
target-specific timing. This kit is a command-line research harness, not an
installable VST, and it has not been copied to or run on the Force.

For listening, `artifacts/es5510-emulation/original_stereo_swap_chord.wav` is
a three-second, original synthetic chord input with its output from the same
ES5510 program. The two channels are sample-exactly exchanged over all 132,300
frames. The input WAV is included to make that comparison reproducible. The
pair's SHA-256 values and RMS/peak measurements are in
`artifacts/es5510-emulation/RESULTS.txt`.

## MAME and OpenASR source investigation

The local source checkout used for comparison is MAME commit
`8be57cdc5f44ab830dff44c06dd39f24190742d4` (2026-11-15). OpenASR `master`
resolved to `241f9331477b07b072330ca6ebd2d7a2e2cc832e` when fetched.

| Path/system | Source evidence | Status |
|---|---|---|
| MAME `esq5505.cpp` family (VFX, VFX-SD, SD-1, SQ, EPS/EPS-16 Plus, KS-32 variants) | Shared machine configuration instantiates ES5510 and `esq_5505_5510_pump_device`; the DUART callback maps bit 6 to ESPHALT and changes the pump's halted state. The firmware ROM sets are external MAME ROM assets. | The source provides a real host-controlled execution path when compatible ROMs are present. This work did not obtain or execute any ROM/effect payload. |
| MAME `esqpump.cpp` | Writes three stereo buses to serial 0–2, calls `run_once()` while released, then reads serial 3 as processed stereo. `run_once()` executes until END/HALT. | Verified as the selected VFX-family audio adapter contract, not a universal Ensoniq routing claim. |
| MAME `esqasr.cpp` ASR-10 / ASR-X | The ASR machine configurations call `m_esp->set_disable()`. ASR-X comments identify ES5511 but use an ES5510 placeholder. | Host-device presence is not proof of audio microprogram execution; ASR-X is not ESP2 emulation. |
| OpenASR | Its public status says ASR-10 boot, effect record upload, compilation/parameter updates and routing. Its effect-upload and execution-contract notes distinguish verified host upload/readback from ESP execution and say the ASR MAME device is disabled; the pump's serial/HALT contract remains open. Its README says ROM/OS images are not distributed. | Useful host lifecycle investigation; not an executable public effect-program catalog and not a distributable firmware source. The execution-contract notes are more cautious than broad status bullets. |

Sources: [MAME ES5510](https://github.com/mamedev/mame/tree/master/src/devices/cpu/es5510),
[MAME ESQPump](https://github.com/mamedev/mame/blob/master/src/devices/sound/esqpump.cpp),
[MAME Ensoniq drivers](https://github.com/mamedev/mame/tree/master/src/mame/ensoniq),
[OpenASR status](https://github.com/peroberg/OpenASR/blob/master/STATUS.md),
[OpenASR effect upload investigation](https://github.com/peroberg/OpenASR/blob/master/docs/asr10/investigations/es5510-effect-upload-v350.md),
[OpenASR execution contract](https://github.com/peroberg/OpenASR/blob/master/docs/asr10/investigations/es5510-upload-executable-contract-v350.md).

## Accuracy limits / next work

This isolated port has not yet been differentially tested against a running
MAME binary. Memory-mapped I/O callbacks remain the same unimplemented/zero
behavior as the MAME device; table RAM and delay RAM paths are implemented
from the inspected source but need targeted vectors. The 160-word safety cap
and per-frame adapter must be compared against MAME's HALT behavior. OpenASR's
ASR-specific board routing is unresolved. No claim is made that a DP/4 or any
Ensoniq factory algorithm has run here.

Next coding task: make a tiny test-only MAME reference executable (or add a
license-clean differential harness against the exact source snapshot), then
compare ALU flags/saturation, MAC pipeline, DOL, delay RAM wrap and stereo
serial traces. After that, accept a locally supplied, rights-cleared MAME
effect bundle, record its SHA-256 and initialization, and render it. Only
after native/ARMv7 budget checks should this be wrapped as a distinct
experimental Force plugin; do not modify ESP Lab release identities.
