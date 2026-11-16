# Experimental ESP emulation handoff

The partial ES5510 execution core is in `experimental/emulation/` and was
added in milestone `1e0f183`. It is an independent implementation; no MAME
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
