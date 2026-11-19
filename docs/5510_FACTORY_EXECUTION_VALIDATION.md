# 5510 factory execution validation status

## Result for this branch

**No complete ASR-10 factory microprogram was recovered, replayed, or validated
in this work session.** OpenASR's prior V3.50 reports document firmware-driven
uploads, readback checks and partial ROM-HALL/44LUSH instruction-image hashes,
terminal words and END indices. The complete instruction arrays, per-index GPR
state and raw logs are missing from the current workspace, so these partial
reports cannot be loaded or independently checked. The OS disk and boot ROM
inputs are also absent. No factory program was replayed against the standalone
executor or executing MAME device.

The existing plugin continues to run original TROLLfx diagnostic delay and
chorus programs. No plugin source, binary, UID or Force installation was
modified by the research changes.

## Validation performed

- Inspected OpenASR transfer code at commit
  `241f9331477b07b072330ca6ebd2d7a2e2cc832e`.
- Inspected MAME ES5510 host/core/disassembler source at commit
  `8be57cdc5f44ab830dff44c06dd39f24190742d4`.
- Added an original passive Lua observer, ordered transaction reconstruction,
  candidate disassembler and explicit candidate-to-renderer-bundle converter.
- Synthetic tests verify byte-latch ordering, combined commits, non-authentic
  fixture classification and malformed/no-event rejection.

This is tool validation only. No authentic program has yet been compared in
MAME and the standalone executor; no ARMv7 build or DSP benchmark is relevant
to this documentation/tooling-only milestone.

## Comparison plan once a real trace exists

1. Re-run the same legally held OS/effect selection with the passive tap and
   retain private hashes and complete logs.
2. Check raw latch/commit order against `asr10_boot.cpp` wrappers and the pinned
   ES5510 `host_w` semantics.
3. Check firmware readbacks against committed GPR/program state.
4. Convert only an explicitly reviewed capture to `ES5510_BUNDLE 1` and render
   known impulse, sine, noise and musical inputs.
5. Run identical instruction/GPR/DRAM/serial state in MAME and the standalone
   adapter; compare per-frame output and selected register/memory checkpoints.
6. Treat audio similarity as execution validation, not by itself as proof that
   the program is an original commercial factory algorithm.

## Current status matrix

| Claim | Status |
|---|---|
| ASR-10 mapped host transfer protocol | Verified in OpenASR source model |
| ASR firmware executed a particular upload in this pass | Not run; media absent |
| Complete factory instructions and GPRs captured | No |
| Authentic effect boundary and algorithm identity established | No |
| Factory program replays in standalone adapter | No |
| MAME-vs-adapter differential result for factory code | No |
| Physical ASR-10/Force validation | No |
