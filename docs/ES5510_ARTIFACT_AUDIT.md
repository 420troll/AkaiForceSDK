# ES5510 / ESP2 execution artifacts audit

Checked: 2026-11-16. This audit supports the experimental emulation track; it
does not establish DP/4 algorithm recovery.

## Public material located

| Artifact | What it provides | What it does not establish | Handling |
|---|---|---|---|
| ES5510 Technical Specification v2.4, linked from the [Ensoniq technical-document index](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) | Primary processor architecture and hardware specification | It is not an assembler distribution or factory program collection | External reference only |
| Unofficial [ESP microcode programming guide](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) | Secondary notes on ESP1 effect-file structure, host-side parameter/glue code and the proprietary ESP1 instruction set | It explicitly does not include ESP microcode in its example 68k effect-file source; it is not an assembler implementation | External reference only; treat as secondary |
| ESP2 Object Format Specification ([PDF](https://convexoptimization.com/TOOLS/ESP2APPX.pdf)) | ESP2 96-bit microinstruction field map and object-file records, including assembled instruction records, initialization, relocation and checksum sections | The document describes a tool/object format, not a verified available assembler executable or source code; it is ESP2, not ESP1 | Public technical document; no code copied |
| MAME ES5510 device ([source](https://github.com/mamedev/mame/tree/master/src/devices/cpu/es5510)) | Public BSD-3-Clause ES5510 implementation and disassembler, useful behavioral reference | Does not prove bit-exact hardware behavior; its TODOs retain open questions | Source read as reference, not copied into this prototype |
| OpenASR ([repository](https://github.com/peroberg/OpenASR)) | Public MAME-derived ASR-10 system emulation effort, including ES5510 integration | It is not an ES5510 assembler distribution; firmware/media use requires separate rights and is excluded here | Reference lead only; no source or media copied |
| Public firmware/EPROM archive indexes | Search results indicate Ensoniq firmware dumps are circulating | Availability is not authorization to redistribute; binaries are not source-level microcode or a clean-room test oracle | Not downloaded or committed |

## Findings from the DP/4 research

`docs/DP4_ALGORITHM_RESEARCH.md` explicitly distinguishes the DP/4 manual's
signal-flow descriptions from actual microcode. Its routing semantics and named
topologies can guide independently written DSP, but cannot validate an
instruction-level implementation or provide expected instruction traces.
There are no algorithm programs or numerical golden vectors in that manual.

## Implementation consequence

The local ES5510 core remains a deliberately small instruction harness. Its
test assertions verify only the behavior implemented by that harness; they
must not be read as hardware conformance results. Further execution changes
should require a primary-spec statement or an isolated MAME behavior with an
explicit reference. Preserve ambiguous details such as condition flags,
saturation corner cases, DOL conversion, bus timing and address-region
configuration as unresolved until supported by reliable evidence.
