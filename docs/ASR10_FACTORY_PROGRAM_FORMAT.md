# ASR-10 factory ES5510 program format: current evidence

## Effect-file evidence from the unofficial ESP1 guide

The local, gitignored `esp1-microcode-guide-unofficial.pdf/.txt` describes an
EPS-16 Plus / ASR-10 effects-file container with global data and parameter
structures, 68k init/task code, and an ESP code section. It documents a header
field `FxUcode` pointing to the ESP section relative to the effect-file base;
the sample program uses a dummy ESP area and states that real ESP code is
assembled/linked separately. The guide also says an ESP section is required
for download, even if it is only a pass-through program.

The extracted ASR disk images now supply real factory effect objects. Their
headers are consistent with this field map: `FxName` at offset 10 (24 spaced
bytes), `FxSize` at 34, `FxUcode` at 38, `FxTable` at 40, `FxInitCode` at 42,
and update-task pointer at 44. Their `FxUcode` values point inside the objects
to sections that decode as consecutive six-byte big-endian words. Those
sections contain 82–103 words, including the ES5510 `END` opcode, and fit the
160-word program memory limit. The header/object relationship and section
statistics are reproduced by `tools/asr10_efe_inspect.py`.

This is strong static identification of factory microprogram sections in
disk-loaded ASR effect objects. It does not establish that these objects
account for all built-in effects, nor does it confirm exact code termination,
the meaning of residual bytes, GPR/delay-memory initialization, host transfer
boundaries, or execution. The guide is unofficial and still requires a
firmware loader/host-transaction correlation.

## Static recovery status and unresolved runtime format

An entire ASR effect file is not a raw instruction stream: the header includes
host-side pointers and the object contains other data/code before and around
the `FxUcode` region. The located code section is a candidate 48-bit stream,
not yet proven to be the exact bytes uploaded. MAME's pinned `es5510d.cpp` is a
stub and does not define the disk file format. OpenASR's host interface accepts
six-byte instruction latches, 24-bit GPR latches, selected commits and control
writes. Firmware-generated CPU-to-ES5510 transactions remain the strongest
runtime validation boundary.

The disk-loaded effect object contains both candidate DSP code and host code.
The 13 ASR effect payloads were byte-identical across OS 1.61 and 3.53. Their
header pointers and code-section statistics are listed in
`ASR10_FIRMWARE_INVENTORY.md`. A candidate region is stronger than a preset
name, but full authenticity still needs loader correlation and execution.

## Recovered objects and candidate code statistics

The following information is emitted by the metadata-only inspector; no
program bytes are printed. Word counts use the FxUcode-to-FxSize interval in
six-byte steps, and `END` counts use the pinned MAME field decoder. Remaining
1–5 bytes within the declared logical object size are unresolved trailer data.

| Object | FxSize | FxUcode | Candidate words | END words |
|---|---:|---:|---:|---:|
| `44DDL+CHORUS` | 3161 | `0x0a3c` | 90 | 14 |
| `44DDL+CH+REV` | 7458 | `0x1abe` | 102 | 14 |
| `44DLYLFO+REV` | 4873 | `0x109e` | 103 | 22 |
| `44EQ+DDL` | 4623 | `0x1000` | 87 | 4 |
| `44EQ+DDL+CHO` | 7261 | `0x1a5e` | 85 | 19 |
| `44EQ+REVERB` | 6093 | `0x1578` | 99 | 31 |
| `44EQ+ROT+DDL` | 5353 | `0x12c2` | 91 | 5 |
| `44LUSH PLAT2` | 3829 | `0x0ca6` | 98 | 25 |
| `44LUSH PLATE` | 3984 | `0x0d42` | 98 | 17 |
| `44PARAM EQ` | 3047 | `0x09fa` | 82 | 2 |
| `44PERC PLATE` | 3984 | `0x0d42` | 98 | 17 |
| `44ROTO+REVRB` | 5121 | `0x1194` | 103 | 20 |

The object table has coherent headers and a common 160-word-compatible size,
but these facts alone cannot prove every word belongs to the live program. In
particular, the `FxUcode` region's remainder must be explained from loader
code or real host commits.

The VFX Musician's Manual independently documents a MIDI command type 03
labelled “ESP Microcode Program Load” but explicitly says that command is
reserved and not implemented in that manual's VFX firmware. This is evidence
about the VFX MIDI interface only, not the ASR-10's internal effect loader; it
must not be used to infer ASR-10 behavior. [Manual section 3.1.4](https://www.polynominal.com/ensoniq-vfx/Ensoniq-VFX-manual.pdf)

## Host transaction representation

The passive tap tool emits tab-separated rows:

```text
T551CAP v1 sequence address data mask pc event openasr_commit mame_commit time
```

Addresses are CPU program-space addresses, byte writes preserve their captured
data/mask, and event types distinguish direct host-window reads/writes,
read-select and each commit window, plus PADAT writes. Timestamps are MAME
emulation time. This stream preserves ordering and should be treated as
copyright-sensitive when it contains program bytes.

`tools/asr10_capture_reconstruct.py` reconstructs latched bytes and commit
events in order. Six instruction latch writes at `$FC3003–$FC3008` form the
48-bit word in MSB-first order. Three GPR latch writes at `$FC3000–$FC3002`
form a 24-bit value, also MSB-first. `$FC3181` commits an instruction selected
by its data byte; `$FC3141` commits a GPR/special register; `$FC31C1` commits
both. This is the host-port transaction format in OpenASR/MAME, not a claim
about how effect files are serialized on disk.

OpenASR's prior V3.50 runtime report says record type 1 emitted paired GPR and
instruction latch data through the `$E0` combined commit. That proves the
runtime pairing, but it does not prove a fixed nine-byte source record format.
An OS effect file is reportedly a mixed CPU/DSP executable object; firmware
may decode headers or transform records before issuing these writes.
`docs/ASR10_FACTORY_LOADER_ANALYSIS.md` records the reported loader sequence,
program endpoints and missing raw trace.

The parser preserves all original events and only reconstructs commits actually
seen. It does not fill absent words, infer missing GPRs, infer algorithm names,
or fabricate external DRAM contents. A first END opcode is reported only as a
candidate marker. Even a contiguous sequence plus HALT/RUN and register setup
requires firmware selection correlation and replay validation before calling
it a captured factory program.

## Supported MAME renderer bundle

After human review, `tools/asr10_capture_to_bundle.py --accept-candidate` can
write the project's `ES5510_BUNDLE 1` text format outside Git. It contains
captured instruction words, captured GPR values, optional observed host serial
setting and sample rate. It does not yet encode the full ordered lifecycle,
readback proof, DADR/DRAM initialization history, or effect selection proof.
The explicit opt-in is intentional. Bundle conversion is not proof of
authenticity or exact hardware behavior.

## Spec versus emulator

The ESP documentation set and MAME/MAME-style code are described in
[ES5510_REBUILD_KNOWLEDGE.md](ES5510_REBUILD_KNOWLEDGE.md) and
[MAME_ES5510_ANALYSIS.md](MAME_ES5510_ANALYSIS.md). Keep ESP1/ES5510 separate
from ESP2/ES5511. MAME's executable host model is the current protocol oracle;
its unverified timing, DRAM size and rounding choices remain emulator
assumptions, not confirmed ASR-10 disk-format facts.
