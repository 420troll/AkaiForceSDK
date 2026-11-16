# Experimental ESP emulation handoff

The partial ES5510 execution core is in `experimental/emulation/`. It is an independent implementation; no MAME
source or proprietary firmware is included. Deterministic test:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
  experimental/emulation/es5510_minimal_test.cpp -o /tmp/es5510-test
/tmp/es5510-test
```

Result: PASS. It currently covers a limited signed 24-bit ALU subset, 48-bit
MAC state, 192 GPRs, 160 instruction words and an audio bridge. It does not
implement instruction pipeline timing, complete opcodes/flags, saturation
conformance, delay RAM scheduling, or authentic serial I/O. ES5511/ESP2 remains
unimplemented. Do not claim faithful DP/4 execution. Compare each addition to
the public ES5510 specification and MAME's BSD-3-Clause model; add hand-written
tests and small original programs only. See `MAME_ES5510_ANALYSIS.md`.

The current public-artifact audit is in `ES5510_ARTIFACT_AUDIT.md`. The public
ESP2 Object Format Specification exposes an object record format and field map,
but this investigation did not locate a source-available, license-clear
assembler tool. The unofficial ESP1 guide documents the proprietary instruction
set and host-side effect-file structure; its example is expressly without ESP
ucode. Firmware images seen in archive indexes were not downloaded or used.

The DP/4 manual research in `DP4_ALGORITHM_RESEARCH.md` is useful for native
DSP topology and routing only; it supplies no factory microcode or emulator
golden vectors. Thus the minimal test results describe this prototype, not
hardware conformance. Addressing, I/O, skip/CCR, saturation and pipeline work
remain blocked on stronger vectors/spec details and are not silently guessed.
