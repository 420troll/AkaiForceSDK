# ASR-10 and ES5510 architecture findings

## ASR-10 integration in OpenASR

Source basis: OpenASR `241f9331477b07b072330ca6ebd2d7a2e2cc832e`,
`src/mame/ensoniq/asr10_boot.cpp`.

| ASR-10 side | OpenASR mapping/behavior | Evidence status |
|---|---|---|
| Host interface window | CPU program space `$FC3000–$FC303F` maps to ES5510 `host_r/w` at matching offsets. | Verified in address map. |
| Read-select | `$FC3100–$FC3101` wrapper calls host offset `$80`. | Verified model mapping. |
| GPR commit | `$FC3140–$FC3141` calls host offset `$A0`. | Verified model mapping. |
| Instruction commit | `$FC3180–$FC3181` calls host offset `$C0`. | Verified model mapping. |
| Combined commit | `$FC31C0–$FC31C1` calls host offset `$E0`. | Verified model mapping. |
| RUN/HALT | MC68302 PA4 (bit 4 of PADAT `$FC6823`) is modeled low=halt and high=run. | Verified OpenASR glue, not independently measured pin voltage. |
| Boot lifecycle | Reset path asserts host HALT; write wrappers set a “program received” flag; PA4 release updates program-loaded/pump state. | Verified model code. Actual firmware reaches every path only when run against appropriate media. |
| Audio route | OpenASR contains a provisional pump mapping with serial registers routed between ES5506 buses and output. | Explicitly provisional model policy; not proof of factory audio behavior. |
| Scheduler | Current ASR-10 config calls `es5510_host.set_disable()` and uses a separate pump adapter. | Verified source. The normal machine configuration does not execute the ES5510 as a scheduled device. |

The supported observation is CPU-side transactions into OpenASR's address map.
The address mapping is ASR-10-specific and must not be projected onto a DP/4.
An observed CPU write tap demonstrates that the running emulated CPU caused a
bus write in this machine model. It does not establish that the real board uses
identical glue or that the ES5510 execution/pump model is accurate.

## MAME ES5510 execution model

Source basis: MAME `8be57cdc5f44ab830dff44c06dd39f24190742d4`.

- The microinstruction is 48 bits. MAME's `DESCRIBE_INSTR` extracts D/C/B/A
  register fields from bits 40/32/24/16, ALU opcode 12–15, operand selector
  8–11, conditional skip bit 7, MAC accumulation bit 6, and RAM control bits
  3–5. Bits 0–2 are unused in this decoder.
- Program storage is modeled as 160 words. Host latch offsets `$03–$08` receive
  six bytes most-significant first.
- Host latch offsets `$00–$02` receive a 24-bit GPR value most-significant
  byte first.
- Host offset `$80` selects/readbacks instruction and GPR/special-register
  state; `$A0` commits GPR/special-register state; `$C0` commits instruction
  state for index `< $A0`; `$E0` commits both applicable values.
- Host offsets `$0F–$11` form DADR; a write to `$0F` triggers the modeled
  external DRAM access. `$12` is host control, `$14` selects RAM, `$18` sets
  host serial control, and `$1F` is halt-enable behavior.
- Special registers include MEMSIZ, DLENGTH, ABASE, BBASE, DBASE, SIGREG, CCR,
  CMR and constants. MAME reset values and memory pipeline details are in the
  pinned source and project analysis.
- MAME `es5510d.cpp` in this pinned revision provides no usable file
  disassembler: its `disassemble()` returns one byte and does not decode
  formats. `DESCRIBE_INSTR` is an internal logging/listing decoder, not a disk
  file parser.

The execution implementation has known fidelity limits recorded in
[MAME_ES5510_ANALYSIS.md](MAME_ES5510_ANALYSIS.md): DRAM capacity is marked
unverified, I/O RAM is incomplete, and DOL rounding/host timing are not fully
settled. The adapter is a useful executable reference, not a proof of complete
chip accuracy. MAME's ES5510 files are BSD-3-Clause; retain the notice already
in `experimental/emulation/THIRD_PARTY_NOTICES.md`.

## Firmware execution vs modeled capability

The source contains the upload mechanism. A firmware-generated program capture
requires actual execution of the ASR firmware with its required ROM and boot
media, followed by observation of the mapped write/commit sequence. The current
checkout lacks those images, and the standard config disables ES5510 scheduler
execution. This pass therefore makes no new claim that a factory effect has
been captured or executed.

## References and licensing

- [OpenASR `asr10_boot.cpp` at pinned commit](https://github.com/peroberg/OpenASR/blob/241f9331477b07b072330ca6ebd2d7a2e2cc832e/src/mame/ensoniq/asr10_boot.cpp)
  and its project `docs/LICENSE` (CC0 1.0). OpenASR code was inspected, not
  copied into the recorder.
- [MAME ES5510 core at pinned commit](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp)
  and corresponding header/disassembler source (BSD-3-Clause).
- Ensoniq, *ES5510 Specification*, Rev. 2.4, and the unofficial *ESP1
  Microcode Programming Guide*, listed in the
  [Ensoniq technical documents archive](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/).
  Private local copies exist outside Git. The extracted ES5510 text file is
  blank (scan/no usable OCR), so chip details in this report are sourced from
  MAME and prior project research; the unofficial guide informs the mixed
  effects-file structure, not a primary silicon specification.
- Ensoniq, *VFX Musician's Manual*, Appendix A §3.1.4, [PDF](https://www.polynominal.com/ensoniq-vfx/Ensoniq-VFX-manual.pdf).
  It says the VFX MIDI ESP microcode-load command type 03 was reserved and
  unimplemented. This is VFX-only context, not evidence of ASR-10 loader
  behavior.
