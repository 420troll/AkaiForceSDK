# EPS-16 Plus / Waveboy ES5510 research

## Scope and source revision

Reviewed the public [mardlib/Ensoniq-EPS-16-Plus repository](https://github.com/mardlib/Ensoniq-EPS-16-Plus) at commit
[`c33f531d8353f36b8ee0bed944b576a537e996bd`](https://github.com/mardlib/Ensoniq-EPS-16-Plus/tree/c33f531d8353f36b8ee0bed944b576a537e996bd), including `native/rom_probe.c`, `native/es5510_core.{c,h}`, `docs/bringup.md`, `docs/parameter-mapping.md`, the README and `THIRD_PARTY_NOTICES.md`.

This is a research summary, not an endorsement of that emulator or a claim of
EPS/Waveboy compatibility for TROLLfx. No source files from that repository
were copied into this project. The repository has no root license granting a
general license to its own implementation. Its `THIRD_PARTY_NOTICES.md`
contains notices for Musashi (MIT), MAME Ensoniq reference material
(BSD-3-Clause), and JUCE (dual-licensed), but these notices do not license the
whole repository. We therefore reuse only factual observations and protocol
descriptions, not its code. Ensoniq ROMs, OS disks, KPC firmware, and Waveboy
files remain copyrighted and are not included here.

## Verified in the EPS emulator source and its documentation

| Topic | Evidence | What is established |
|---|---|---|
| Program size and instruction width | `native/es5510_core.h`; `native/rom_probe.c` | The model has 160 instruction slots. Instructions are held in 64-bit containers but masked to 48 bits. |
| Instruction host transfer | `native/rom_probe.c`, `es5510_read` / `es5510_write` at ES5510 base `$260000` | Six byte lanes at word offsets 3–8 assemble a 48-bit instruction latch. A subsequent write to selector offset `$c0` stores the latch at the selected instruction index; `$e0` can commit an instruction and a GPR write together. Offsets `$80` and `$c0` support program-word selection/readback and write. |
| GPR transfer | same host interface | Three byte lanes at offsets 0–2 form a 24-bit GPR latch. Writing the GPR selector at `$a0` commits it; `$80` selects/readbacks a GPR. `$e0` also commits a GPR value. |
| Host transfer/execution control | same host interface | Offset `$1f` marks host upload activity and halts ES5510 execution. Offset `$12` is Host Control: bit 1 clears external delay RAM only while halted; bit 0 releases execution and ends the upload hold. The emulator extends a host-access hold while upload/readback continues to avoid the running DSP changing a GPR during OS verification. |
| External delay RAM | same host interface and `es5510_core_dram_address` | Three-byte DADR latch at offsets `$15`–`$17`; writing the first/high byte triggers a read or write using the active MEMSIZ address mapping. DIL/DOL data latches occupy offsets `$09`–`$0e`. The core models 1,048,576 16-bit words (2 MiB), and special registers include DLENGTH, ABASE, BBASE, DBASE, SIGREG, CCR and CMR. This describes the emulator's modeled interface; exact board details still need primary-spec/manual comparison. |
| Audio routing | EPS `docs/bringup.md` | The documented EPS host-serial mode for effects is `0x48`; ports 0, 2 and 3 receive ES5505 effect buses and serial output 1 feeds the main DAC. ES5505 routes 0/2/3 map to effect buses 1/2/3; Aux1 is separate. These are EPS board routing facts, not Force VST routing. |
| ROM and external programs | README and `docs/bringup.md` | The emulator reports the original OS loading/running ROM effect programs 10–13 and external Waveboy effects. It has a separate host-upload/readback protocol and uses the original OS's download flow. It does not identify effect names by interpreting the program or displayed parameter page. |
| Parameters | `docs/parameter-mapping.md` | Effect parameters are marked algorithm-dependent and owned by the original OS/editor. The repository explicitly says a fixed parameter schema is not valid; individual parameter round-trip remains pending there. |
| Trace | `native/rom_probe.c`, `EPS16_TRACE_ES5510_PROGRAM` diagnostic | The diagnostic emits 160 lines `es5510_program[NNN]=hhhhhhhhhhhh gpr=hhhhhh`. Each line contains a 48-bit program slot and the current `es5510_gpr[NNN]`. That second field is only the same-index GPR and is **not** a full 256-entry GPR initialization image. It also omits external DRAM and audio configuration. |

## Waveboy loading: established versus unresolved

The public emulator README and bring-up notes state that the EPS OS can download
external Waveboy programs and that longer files exposed an execution/host-access
race during verification. The documented implementation halts the DSP during
transfer and keeps host access alive through the verifier. The OS subsequently
executes the uploaded 48-bit words with initialized GPR/special-register state,
delay RAM, host serial mode and EPS bus routing.

What the reviewed public repository does **not** establish:

- a standalone Waveboy file/container format or complete byte-to-instruction
  mapping independent of the OS;
- a legally redistributable Waveboy program image, full parameter map, or
  parameter-to-GPR update table;
- a complete trace of every host transaction, all GPRs, delay RAM, timing and
  routing sufficient to recreate a captured effect by itself;
- that any Waveboy program can be executed by loading its word array into the
  TROLLfx standalone core without reproducing EPS initialization and board
  signal routing.

Therefore there is no claimed EPS/Waveboy importer or compatibility layer in
this milestone. The documented OS download procedure is evidence of a working
historical path inside the full EPS emulation, not evidence of a generic effect
file loader. Use only files whose acquisition and use are lawful; do not
publish or commit captured commercial program data.

## Local-only trace inspector

`tools/es5510_capture_inspect.py` validates either the exact ASCII diagnostic
line format above or a neutral one-word-per-line 48-bit hex list. It checks
syntax, contiguous indexes, the 160-word capacity, hashes the source text, and
can optionally write a JSON inspection record. It does not accept ROMs or
unknown binary containers, execute instructions, infer parameter maps, or
produce an executable bundle. Optional JSON includes the instruction words and
may itself contain copyrighted program data, so it is created only at a
user-selected path and is not suitable for publishing without permission.

Tests use generated synthetic inputs only:

```sh
python3 -m unittest discover -s experimental/emulation/capture/tests -v
python3 tools/es5510_capture_inspect.py /path/to/local-trace.txt
```

## References

- [EPS-16 Plus emulator repository](https://github.com/mardlib/Ensoniq-EPS-16-Plus)
- [Pinned host/device interface](https://github.com/mardlib/Ensoniq-EPS-16-Plus/blob/c33f531d8353f36b8ee0bed944b576a537e996bd/native/rom_probe.c)
- [Pinned ES5510 model header](https://github.com/mardlib/Ensoniq-EPS-16-Plus/blob/c33f531d8353f36b8ee0bed944b576a537e996bd/native/es5510_core.h)
- [Pinned EPS bring-up and routing notes](https://github.com/mardlib/Ensoniq-EPS-16-Plus/blob/c33f531d8353f36b8ee0bed944b576a537e996bd/docs/bringup.md)
- [Pinned parameter ownership notes](https://github.com/mardlib/Ensoniq-EPS-16-Plus/blob/c33f531d8353f36b8ee0bed944b576a537e996bd/docs/parameter-mapping.md)
- [Upstream third-party notices](https://github.com/mardlib/Ensoniq-EPS-16-Plus/blob/c33f531d8353f36b8ee0bed944b576a537e996bd/THIRD_PARTY_NOTICES.md)
- [Waveboy catalog](https://chickensys.com/products2/sounds/waveboy/) and [SamplerZone catalog](https://samplerzone.com/products/parallel-effects): product/catalog references only; not technical file-format evidence.
