# MAME ES5510 analysis

## Snapshot

- Upstream: [mamedev/mame](https://github.com/mamedev/mame)
- Inspected commit: `8be57cdc5f44ab830dff44c06dd39f24190742d4`
- Paths: `src/devices/cpu/es5510/`, `src/devices/sound/esqpump.*`, and selected
  `src/mame/ensoniq/` drivers
- License: BSD-3-Clause in the ES5510 and pump source headers
- Inspection method: sparse, depth-one checkout; no ROM sets were fetched

## What the implementation establishes

The core is a useful executable description of the original ESP:

| Area | MAME implementation |
|---|---|
| Program | 160 x 48-bit instruction storage; PC restarts at zero and END halts |
| Registers | 192 24-bit GPRs plus serial, MAC, DIL/DOL-related, address, condition and constant registers |
| Arithmetic | 24-bit ALU, saturation/flags, a 48-bit multiply/accumulate path with configurable shift |
| Instruction parallelism | ALU and MAC operations are pipelined alongside one RAM-control operation |
| Memory | 16-bit DRAM array; delay/table A/table B/I/O modes; delay modulo and table base addressing |
| Audio I/O | Four stereo serial register pairs; VFX-family pump uses three input stereo buses and one processed output pair |
| Host | Byte-wise latches for 24-bit GPR/data/address values and 48-bit instructions; halt/control and RAM access |
| Debugging | Program listing/disassembly names used/read registers and decodes ALU, MAC, routing and RAM actions |

The exact bit extraction is visible in `DESCRIBE_INSTR`: D, C, B and A register
fields occupy bits 40, 32, 24 and 16; ALU opcode bits 12–15; operand select
bits 8–11; skip bit 7; MAC accumulate bit 6; RAM control bits 3–5.

## Pipeline and memory observations

MAME advances current, previous and two-cycles-previous RAM operations. A read
scheduled in instruction N becomes DIL data through the delayed RAM state;
writes consume a small DOL FIFO. MAC and ALU results are also committed with
pipeline staging. A simplified block processor that executes all sub-operations
immediately will therefore disagree on programs that depend on hazards.

Delay addresses use:

`((DBASE + offset) mod (DLENGTH + memory_increment)) & memory_mask`

before the configured right shift. Table A/B addresses use their respective
bases plus the per-instruction GPR offset. `MEMSIZ` changes mask, shift and
increment; current host DADR access was recently fixed to use the same mapping.

## Product integration represented by MAME

- `esq5505.cpp`: VFX/VFX-SD, EPS/EPS-16 Plus, SD-1 variants, SQ-series and
  KS-32 share the ES5505+ES5510 driver family.
- `esqpump.cpp`: model specifically states it represents VFX-family routing.
- `esqkt.cpp`: KT/TS family comments specify two ES5506 devices and ES5510.
- `esqasr.cpp`: ASR-10 specifies ES5506+ES5510; ASR-X specifies ES5511, but the
  latter uses a disabled ES5510 placeholder and is skeletal.
- `esqmr.cpp`: MR family comments specify ES5511; no ESP2 device implementation
  was found in the inspected source.

This means current MAME is strong evidence for ES5510 program execution and
selected system integration, but it is not an ESP2 emulator and it is not a
DP/4 application/ROM dump.

## Explicit limitations and open implementation questions

1. `DRAM_SIZE` is 1M entries, accompanied by a source TODO saying it is not
   verified and that most games use 128 KB.
2. DOL conversion has a compile-time `DOL_ROUND_TOWARDS_ZERO` option set to
   false and a comment that behavior remains under investigation.
3. RAM-clear timing and MEMSIZ/DLENGTH behavior are marked TODO.
4. Memory-mapped I/O reads/writes are placeholders.
5. Host-control read and test-PC behavior are incomplete/synthetic.
6. MAME's working-system claim is empirical. It does not establish bit-perfect
   equivalence for every opcode, product, DRAM configuration or effect.

Recent history reinforces the need for caution:

- `313473d667b5c7786fae22c6d9cb0f56ceda014d` (2026-02-18) corrected ABS,
  CMP, signature-controlled multiplier shift, CCR/CMR encoding and shift flags.
- `3618bc3f...` proposed different DOL rounding; `6c483aee...` partially
  reverted it after hardware investigation while preserving 16-bit DIL/DOL.
- `206047f600db8ce4b8aa95f39998a21dc30b140d` (2026-06-28) fixed register-write
  macros whose bug was often masked by coincidentally correct caller data.
- `d154d2a69e626b4f9113c494f4d304488a141a04` (2026-11-03) fixed host DRAM
  accesses when MEMSIZ mapping is active.

## License boundary

MAME's ES5510 files are BSD-3-Clause. An adaptation may be legally practical,
but it must retain copyright/license notices in source distributions and
reproduce them in binary documentation, and it must not use contributor names
for endorsement. The cleanest project structure remains:

- use MAME as a cited behavioral oracle and optional experimental emulator;
- write original production DSP independently under the project's chosen
  license; and
- never combine proprietary effect programs/ROMs with the repository.

The mardlib EPS-16 Plus project reports adapting MAME semantics into a
standalone core and documents substantial original-OS regression coverage.
However, no top-level license was found in that repository during this review.
Its original code must not be copied until the author clarifies licensing.

## Recommended future validation

Create a small, license-clean ES5510 conformance harness in
`experimental/emulation/` using hand-authored microprograms only. Cover every
ALU opcode, saturation boundary, flags/skip behavior, MAC latency, DIL/DOL
conversion, FIFO ordering, MEMSIZ modes and delay wrap. Compare MAME and any
independent core first; hardware comparison can follow only with authorized
equipment and a non-destructive test protocol.

## Issue #4 follow-up (2026-11-16)

`experimental/emulation/mame_es5510_standalone.*` now adapts the MAME device's
instruction pipeline and VFX pump serial frame to a headless program/WAV
harness. Its source attribution and BSD-3-Clause terms are in
`experimental/emulation/THIRD_PARTY_NOTICES.md`. The hand-authored stereo-swap
microprogram executes 2 MOV instructions plus END per audio frame and produces
a reproducible impulse WAV; this is a real instruction path but not an effect
algorithm. Native and ARMv7/Cortex-A17 compile passed. No runtime differential
against a full MAME process, factory microprogram, or Force performance run is
available yet. See `docs/HANDOFF_EMULATION.md` and
`artifacts/es5510-emulation/RESULTS.txt` for measurements and limits.

OpenASR `master` was inspected at
`241f9331477b07b072330ca6ebd2d7a2e2cc832e`. The ASR effect-upload notes verify
host-side instruction/GPR commits and readback for local ROM-backed tests;
its execution-contract notes explicitly leave the ASR pump/HALT/serial policy
open. MAME's ASR driver currently disables the ES5510 device. This is not
evidence of a runnable ASR factory program in a standalone harness.



### Issue #4 fixture correction (2026-11-16)

The checked-in MAME pump test bundle must initialize special register 244
(MEMSIZ) to `0xff`. MAME derives an 8-bit memory shift from its low eight one
bits; with the fixture's `0x100` GPR offset this selects the next 16-bit DRAM
word. The earlier bundle omitted that init, left the standalone default shift
at 24, and therefore did not validate the intended long delay. The bundle and
render are corrected; the impulse now first appears at frame 7440 (the 7440
sample delay encoded by the fixture). Do not use the superseded one-frame claim.
