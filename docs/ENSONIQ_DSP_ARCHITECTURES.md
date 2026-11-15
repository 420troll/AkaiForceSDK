# Ensoniq DSP architectures

## Scope and terminology

This document separates first-generation ESP/ES5510 evidence from
second-generation ESP2/ES5511 evidence and from user-visible product behavior.
`ESP1` is used informally here only to contrast the original ESP with ESP2;
the supported part identifier is ES5510. No relevant evidence for an Ensoniq
`ES5100` part was found.

## Product lineage

| Product/family | Effects architecture | Confidence and source |
|---|---|---|
| EPS (original) | MAME groups it with ES5505 + ES5510 systems, but the inspected EPS service material emphasizes DOC II and did not yield a searchable ESP part reference | **Unresolved at board-revision level.** Do not equate the original EPS signal path with EPS-16 Plus solely from the shared MAME driver |
| EPS-16 Plus | ES5510-class ESP with downloadable effect programs and external effect files | **Verified** by EPS-16 service diagnostics and the unofficial effect-file guide; exact part/revision should be confirmed from schematic/board photography |
| VFX/VFX-SD, SD-1/32, SQ-series, KS-32 | ES5505-family voice engine feeding ES5510 effects | **Verified in MAME architecture and driver comments**; product board confirmation varies |
| ASR-10 | ES5506 voice engine plus ES5510 effects | **Verified in current MAME driver comments** and supported by ASR effects documentation |
| KT/TS family | Two ES5506 devices plus ES5510 effects | **Verified in MAME driver comments** |
| DP/2, DP/4/4+ | Standalone parallel-effects family; DP/4 exposes four ESP processing units | **Verified at product/manual level**; DP/4+ schematic identifies an ESP Rev-7/5510 |
| ASR-X | ES5506 plus ES5511/ESP2 | **Probable/driver-verified, emulation unresolved**; MAME labels the device ES5511 but instantiates a disabled ES5510 placeholder |
| MR-61/MR-76/MR-Rack | Two ES5506 devices plus ES5511/ESP2 | **Verified by MAME board notes/driver comments; no working ESP2 emulation inspected** |
| DP/Pro | Two second-generation, 24-bit ESP2 processors, one algorithm per processor at a time | **Verified** by the DP/Pro reference manual |

Related Soundscape/arcade use demonstrates that ES5510 was not limited to one
keyboard or effects rack, but those systems are not substitutes for musical
product validation.

## ES5510 / original ESP

### Execution model visible in MAME

At inspected MAME commit `8be57cdc5f44ab830dff44c06dd39f24190742d4`:

- The instruction word is 48 bits. It encodes four 8-bit register fields, a
  4-bit ALU operation, a 4-bit operand-routing selector, a skippable bit, a
  multiply/accumulate bit, and a 3-bit RAM-control operation.
- MAME models 160 instruction locations and 192 general-purpose registers
  (`0x00..0xbf`). Special registers include four stereo serial pairs, MAC low
  and high, DIL/MEMSIZ, delay length, A/B/delay bases, signature/control,
  condition code/mask, and fixed constants.
- Each instruction coordinates ALU, multiplier/accumulator, and an external
  memory action. The ALU set modeled by MAME is ADD, SUB, unsigned variants,
  CMP, AND/OR/XOR, ABS, MOV, fixed shifts, DIFF, ASR, and END.
- Operands/results can be routed between registers and the delay input/output
  latches. RAM-control modes select delay, table A, table B, FIFO dump, or I/O.
- The datapath is 24-bit fixed point; the multiply/accumulate state is 48 bits.
  Saturation, flags, skip semantics and pipeline ordering materially affect
  output and therefore must be tested, not simplified casually.
- External DRAM samples are modeled as 16-bit values. DIL and DOL are
  left-justified in the 24-bit internal path. MAME contains an explicit open
  question about how negative DOL values round/truncate.
- Delay addressing is based on `DBASE + offset`, modulo `DLENGTH + one memory
  increment`, then masked/shifted according to `MEMSIZ`. Table A and B use
  their independent bases and common memory mapping.
- MAME's VFX-family pump writes three stereo buses (FX1, FX2 and DRY) into
  serial pairs 0–2, executes the ESP program once per audio sample, and reads
  the processed stereo result from serial pair 3. That pump is system-specific,
  not a universal description of every Ensoniq product's routing.

These are facts about the current emulator implementation. Hardware claims
need the ES5510 specification or measured hardware to raise confidence.

### Host and program behavior

The host can halt the chip, transfer 24-bit GPR values, transfer 48-bit
instructions, and access external memory through data/address latches. The
unofficial EPS/ASR effect-file guide describes a downloadable effect as a
combination of parameter/global data, host 68k routines, and ESP microcode.
That is plausible and useful, but the guide is secondary evidence and must not
be confused with permission to redistribute factory effects.

### Why this architecture matters

The original ESP demonstrates a compact audio-specialized pattern valuable on
Force even without emulation: one sample loop combines fixed-point arithmetic,
MAC work, and circular delay/table access. Its constraints encourage shared
delay memory, small state, deterministic execution, and parameter updates from
a slower control path.

## ESP2 / ES5511

The primary ESP2 specification describes a substantial redesign begun in 1990.
Its important verified properties are:

- Three function units execute in parallel on each program line: MAC, ALU and
  AGEN (address generator). All ESP2 instructions complete in one instruction
  cycle, with documented pipeline/availability latencies.
- The instruction word is 96 bits. The initial design describes a 300 x 96-bit
  internal instruction memory (10-bit fields permit future expansion to 1024).
  At 40 MHz and 44.1 kHz, the document states 226 instruction cycles per sample.
- Internal operands are 24-bit. A 24 x 24 multiplier feeds a 52-bit accumulator
  and a 60-bit barrel-shift path. The ALU offers 32 fundamental operations plus
  pseudo-operations and conditional/program-control mechanisms.
- The 10-bit internal-register map allows GPR, AOR and SPR spaces. The document
  describes 456 addressable GPR positions and 452 AOR positions, while noting
  that initial revisions planned 256 physical GPRs and 256 AORs.
- AGEN provides modulo addressing for eight independently located/sized
  external-memory regions. Region `BASE`, `SIZEM1`, and `END` registers plus
  AOR offsets support delay lines, tables, plus-one access and base updates.
- The external interface is a 24-bit address and 24-bit data bus, capable of
  DRAM/SRAM and memory-mapped I/O. The design addresses up to 16 megawords.
- External reads must be scheduled far enough ahead for DIL availability;
  MAC-to-DOL writes may be scheduled earlier than ALU-to-DOL writes. These
  pipeline rules are essential for any future emulator.
- Sixteen 24-bit serial data SPRs provide eight stereo serial lines, each
  configurable as input/output with programmable clock grouping.
- The asynchronous host side is 8-bit while internal transfers are 24-bit.
  Host/register transfers occur under program control; instruction memory has
  a separate host path.
- The chip includes conditional execution, jumps/subroutines, and a repeat
  mechanism. The document calls out latent control instructions and hazards.

The ESP2 specification discusses algorithm applications and design philosophy,
but the accessed document does not provide DP/Pro factory programs. A generic
ESP2 architecture model is not an original DP/Pro algorithm recovery.

## Delay, modulation, diffusion, pitch and reverb implications

The product manuals verify behavior and parameter vocabulary:

- DP/4 reverbs expose predelay, decay, frequency-dependent decay/bandwidth,
  diffusion, and decay modulation. The conceptual diagrams support a chain of
  input filtering, diffusers and a reverberant network, but not exact delay
  lengths, matrix signs or coefficients.
- DP/Pro Expert Reverb exposes a six-stage diffuser and eight user delay-time
  controls plus size, density and frequency-dependent decay. This is strong
  evidence for a configurable multi-delay/diffusion design, not a complete
  topology specification.
- Modulation effects expose delay time, LFO rate/depth/waveshape/phase,
  smoothing, filtered regeneration and cross-regeneration. An original
  implementation using fractional delay interpolation can reproduce these
  behaviors without copying microcode.
- DP/4 pitch algorithms distinguish single-unit, two-unit/ganged and fast
  variants. DP/Pro Multi-Pitch Shift exposes six voices, splice width, delay,
  filtering and regeneration. `Splice width` strongly suggests windowed/time-
  domain segment management, but exact window shape and scheduling remain
  unresolved.
- DP/4 can route units serially or in parallel and gang processors for more
  expensive algorithms. DP/Pro routes two ESP2 processors and restricts
  algorithm choices according to the selected routing.

## Practical conclusion

For production development, the best starting architecture is not a cycle-
exact ES5510 clone. It is an original floating-point/C++ modular graph informed
by the documented behavior: shared fractional-delay storage, filtered feedback,
modulators, explicit serial/parallel routing, diffusion stages, and a separate
control-rate parameter layer. This maps cleanly to the Force host and is easier
to validate and optimize.

The ES5510 is the best **emulation research** starting point because MAME has a
working BSD-licensed model and real product programs exercise it. ESP2 offers a
more capable architectural template, but no inspected public emulator provides
comparable execution coverage, and the exact ES5511 silicon/product mappings
remain less complete.

