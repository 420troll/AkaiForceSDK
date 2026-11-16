# ES5510 / ESP2 execution artifacts audit

Checked: 2026-11-16. This audit supports the experimental emulation track; it
does not establish DP/4 algorithm recovery.

## Public material located

| Artifact | What it provides | What it does not establish | Handling |
|---|---|---|---|
| ES5510 Technical Specification v2.4, linked from the [Ensoniq technical-document index](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) | Primary processor architecture and hardware specification | It is not an assembler distribution or factory program collection | External reference only |
| Unofficial [ESP microcode programming guide](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) | Secondary notes on ESP1 effect-file structure, host-side parameter/glue code and the proprietary ESP1 instruction set | It explicitly does not include ESP microcode in its example 68k effect-file source; it is not an assembler implementation | External reference only; treat as secondary |
| ESP2 Object Format Specification ([PDF](https://convexoptimization.com/TOOLS/ESP2APPX.pdf)) | ESP2 96-bit microinstruction field map and object-file records, including assembled instruction records, initialization, relocation and checksum sections | The document describes a tool/object format, not a verified available assembler executable or source code; it is ESP2, not ESP1 | Public technical document; no code copied |
| MAME ES5510 device ([source](https://github.com/mamedev/mame/tree/master/src/devices/cpu/es5510)) | Public BSD-3-Clause ES5510 implementation and disassembler, useful behavioral reference | Does not prove bit-exact hardware behavior; its TODOs retain open questions | A limited standalone adapter/port now exists in `experimental/emulation/mame_es5510_standalone.*`; attribution and full BSD terms are retained in `experimental/emulation/THIRD_PARTY_NOTICES.md` |
| OpenASR ([repository](https://github.com/peroberg/OpenASR)) | Public MAME-derived ASR-10 system emulation effort; its source documents upload/readback and board execution boundaries | It is not an ES5510 assembler distribution; firmware/media use requires separate rights and is excluded here. ASR MAME device is disabled and execution routing remains open in the inspected notes. | Source and notes inspected at commit `241f9331477b07b072330ca6ebd2d7a2e2cc832e`; no OpenASR code or media copied |
| Public firmware/EPROM archive indexes | Search results indicate Ensoniq firmware dumps are circulating | Availability is not authorization to redistribute; binaries are not source-level microcode or a clean-room test oracle | Not downloaded or committed |

## Findings from the DP/4 research

`docs/DP4_ALGORITHM_RESEARCH.md` explicitly distinguishes the DP/4 manual's
signal-flow descriptions from actual microcode. Its routing semantics and named
topologies can guide independently written DSP, but cannot validate an
instruction-level implementation or provide expected instruction traces.
There are no algorithm programs or numerical golden vectors in that manual.

## Implementation consequence

The standalone MAME-semantics adapter accepts original or locally supplied
program bundles and drives an offline PCM harness. Its first original
microprogram swaps two serial channels and establishes only that selected
instructions produce audio. It has not been differentially compared against a
running MAME build and is not a factory algorithm. Remaining MAME TODOs and
system-specific loading, memory, serial and HALT behavior must remain explicit
until supported by differential vectors or lawful user-supplied artifacts.
