# Experimental emulation

Instruction-level and hardware-behavior experiments belong here. Keep their
licenses and build targets separate from production DSP. No proprietary
program images belong in this repository.

`mame_es5510_standalone.*` is a BSD-3-Clause-attributed adapter of MAME's
ES5510 execution semantics; see `THIRD_PARTY_NOTICES.md` and
`docs/HANDOFF_EMULATION.md`. The headless runner accepts a local textual
program/state bundle. Its checked-in program is an original stereo swap test,
not Ensoniq firmware or a factory effect. A second checked-in test bundle
adapts MAME’s public PUMP_REPLACE_ESP_PROGRAM fixture to exercise serial-bus
summing and delay RAM. It sets MEMSIZ to 0xff (8-bit shift) so the 0x100
GPR offset is one 16-bit DRAM word; it is synthetic emulator test code.

Run the smoke test from repository root:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/mame_es5510_standalone_test.cpp -o /tmp/es5510-test
/tmp/es5510-test
```


Phase 2 original programs, parameter controls, and renders are documented in
`docs/HANDOFF_EMULATION_PHASE2.md` and `docs/ES5510_PROGRAM_CATALOG.md`; results
and ARMv7 test kit are in `artifacts/es5510-emulation/PHASE2_RESULTS.txt`.
