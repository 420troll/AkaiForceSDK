# ASR-10 factory ES5510 program format: current evidence

## Effect-file evidence from the unofficial ESP1 guide

The local, gitignored `esp1-microcode-guide-unofficial.pdf/.txt` describes an
EPS-16 Plus / ASR-10 effects-file container with global data and parameter
structures, 68k init/task code, and an ESP code section. It documents a header
field `FxUcode` pointing to the ESP section relative to the effect-file base;
the sample program uses a dummy ESP area and states that real ESP code is
assembled/linked separately. The guide also says an ESP section is required
for download, even if it is only a pass-through program.

This materially narrows the search: a compatible ASR-10 effect file can be a
mixed host-CPU/DSP executable object, not just a parameter preset. It does not
reveal the actual factory files' byte layout, instruction packing, compression,
transfer boundaries, or whether built-in effects are stored in OS ROM, disk
files, or both. The guide is explicitly unofficial and cannot stand in for a
captured factory transfer. It does **not** establish that raw files have the
same header/version layout across EPS-16 Plus and ASR-10 revisions.

## No factory binary format is yet verified

There is currently no evidence in this checkout that an ASR-10 effect disk
file is itself a raw 48-bit instruction stream. MAME's ES5510 logging decoder
does not define an ASR-10 disk format. Its pinned `es5510d.cpp` is a stub.
OpenASR's actual host interface accepts six byte instruction latches, 24-bit
GPR latches, selected commits and control writes. This makes the executed
CPU-to-ES5510 transaction stream the best-supported acquisition boundary.

Possible origins include OS-resident code, a disk-loaded effect object, or a
mixture of DSP program and parameters. These remain hypotheses until firmware
reads are correlated with ES5510 commits during effect selection. A preset
name or parameter block alone is not executable microcode evidence.

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
