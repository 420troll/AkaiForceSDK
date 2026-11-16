# Experimental ES5510 core — Stage 002

Status: partial, independently written research scaffold. It is not a faithful
ES5510 or DP/4 emulator and is not linked into production plugin targets.

## Implemented

`experimental/emulation/es5510_minimal.hpp` provides 192 signed 24-bit GPRs,
160 48-bit instruction slots, the public field layout, one-instruction stepping,
END halt, a subset of 16 ALU opcodes, signed 48-bit MAC accumulation, Q1.23
sample conversion and a deliberately simple sample bridge using GPR 0/1 for
stereo inputs and GPR 2/3 for outputs. ALU and MAC operations are executed
immediately. This is a deterministic test harness, not the real device's
pipeline schedule.

## Verification

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
  experimental/emulation/es5510_minimal_test.cpp -o /tmp/es5510_minimal_test
/tmp/es5510_minimal_test
```

Tests cover 24-bit signed wrap, END halt, one 48-bit MAC result and the sample
bridge. They passed on the development host.

## Accuracy limits

ALU opcode indices and instruction bit positions follow the public ES5510
technical specification and the disassembly layout recorded in
`docs/MAME_ES5510_ANALYSIS.md`. The behavior is not cross-validated against
physical silicon. Unimplemented: exact saturation/CCR semantics, operand
routing modes to delay/DIL/DOL, multi-cycle pipeline and hazards, MAC shifts
and flags, skip behavior, external DRAM and MEMSIZ address mapping, delay/table
RAM schedules, serial register map and host interface. The direct MAC output
mapping is a local testing convention, not an Ensoniq register definition.

MAME is used as a reference and no MAME source was copied. Its BSD-3-Clause
license therefore does not apply to this independently written prototype.

## ESP2 / ES5511

The existing primary-spec analysis documents ESP2's 96-bit instruction word,
parallel MAC/ALU/AGEN units, 24-bit operands, 52-bit accumulator, eight modulo
address regions and eight stereo serial streams. This core does not model
ESP2. No ES5511 emulator was identified in the examined public MAME code;
ASR-X/MR driver references are not evidence of a working ESP2 implementation.
The ES5510 and ESP2 instruction sets are distinct and must not share a decoder
by assumption.

## Next work

Build opcode-by-opcode tests against a second independent implementation or
documented vectors; add observable condition-code/saturation rules; then model
RAM scheduling with tiny original instruction programs. Do not add ROMs or
factory effect programs. Only after core conformance should the frame bridge
be considered as an experimental audio engine.
