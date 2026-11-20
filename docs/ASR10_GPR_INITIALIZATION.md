# ASR-10 factory effect GPR and memory initialization status

## Result

No factory effect currently has a complete, evidence-backed initialization
image suitable for authentic replay. The effect files contain useful host-side
code and data, but the current static evidence does not map the effect
parameters or object bytes to final ES5510 GPRs, special registers, or external
delay-memory contents.

The v1.5b boot ROM now has a private runtime trace. It writes blank
instruction-memory words and startup/special-register defaults before the
OpenASR PA4 callback halts the DSP. That initialization is not associated with
any factory effect and is incomplete for external DRAM (no DADR writes and no
modeled ABASE/BBASE writes). It must not be substituted for the per-effect
initialization listed below. Exact values are in
`ASR10_BOOT_ROM_RUNTIME_CHECK.md`.

## Evidence inventory

| Initialization element | Available evidence | Status |
|---|---|---|
| Candidate instruction section | Each EFE has an `FxUcode` pointer into the object and a six-byte candidate region bounded for inventory purposes by `FxSize`. | Strong static candidate; exact upload start/end unverified |
| Effect output GPRs | The guide's header includes `FxOutputGprL/R`; all 12 extracted ASR EFE objects store left output GPR `0x82`, right output GPR `0x83`, and `FxNumVoices=1`. | Verified header metadata; identifies output-register assignments, not initial register values |
| Effect buses | Three NUL-terminated bus-name fields occur after the fixed header; e.g. `44DDL+CHORUS` has `DDL+CHORUS`, `DRY`, `DRY`, while the plate objects have `REVERB`, `REVERB`, `DRY`. | Verified stored routing labels; relationship to host mixing implementation not independently traced |
| Effect-local init entry | `FxInitCode` is in the header; several effects have nonzero 68k entry points. `44DDL+CHORUS` and `44EQ+DDL` have zero. | Header field verified; system-wide initialization still unknown |
| User-variable defaults/state | Header provides four `FxVar` offsets and parameter structures refer to variable offsets. | Object structure exists; values are host parameter state, not proven DSP GPR values |
| Parameter update code | `44DDL+CHORUS` update task at `0x0452` computes/smooths values and calls `$801e`. | Static behavior verified; callee unresolved |
| GPR register numbers and initial values | Output GPR numbers are stored in the object header as above. No usable per-index transaction capture or mapped OS service body provides initial values for those or other GPRs. | Output destination indices verified; register initialization values missing |
| Special registers: MEMSIZ, DLENGTH, DBASE, ABASE, BBASE, SIGREG | No correlated ASR 1.61/3.53 host writes. Prior V3.50 report is incomplete and not reproducible from raw logs. | Missing |
| Host serial config and control lifecycle | Prior OpenASR V3.50 report includes `$48` host serial, host-control values, and HALT transactions. | Secondary historical runtime report only; not attached to these objects or full program arrays |
| External delay RAM initialization | No observed bulk writes, memory-control setup, or proven zeroing sequence for the selected EFE. | Missing |
| Parameter-to-GPR scaling | UI parameter types/ranges and variable fields exist, but the `$801e` routine and corresponding transaction trace are unavailable. | Missing |

The MAME-derived standalone loader can set registers and individual DRAM
words, but using guessed or zero values would make a synthetic experiment,
not an authentic factory-program reconstruction. No such bundle or audio is
claimed here.

## Relevant source constraints

The unofficial ESP1 guide documents an effect-file header, object-local
68k initialization/update tasks, and four variable areas. It does not supply
the ASR operating system's service implementation or prove how the host
converts each object variable into an ES5510 host-port transaction. The
effect-local code's absolute calls (including `$801e` and, in other objects,
`$8030`) remain unresolved without a correctly mapped OS runtime or captured
host transactions.

The pinned OpenASR report documents an ASR CPU to ES5510 host transaction
protocol and selected firmware execution under a V3.50 model, but the underlying
raw logs and complete program/GPR arrays are absent. It is not permissible to
fill those gaps using the reported program hashes, END words, current TROLLfx
diagnostic GPRs, or nominal ES5510 reset values.

## Minimum evidence for completing initialization

For one selected effect, retain one capture containing:

1. OS/disk version and hashes; selected EFE filename, full object hash, and
   effect-selection event.
2. HALT assertion, host control/serial writes, every GPR/instruction latch
   write and commit in order, plus host readback.
3. Final instruction words and their indices, including program tail and
   boundary evidence.
4. Every relevant special-register write/readback and DADR/DRAM operation,
   including evidence for required delay-memory initialization.
5. RUN transition and subsequent parameter edits, with each changed UI
   variable correlated to changed GPR(s), values, scaling and timing.
6. A deterministic rerun yielding identical committed state.

Only then construct a private replay bundle. Until then, factory GPR setup,
special-register setup and parameter mappings remain unresolved.

## Reproducibility

The metadata manifest is generated with `tools/asr10_factory_manifest.py` and
contains no proprietary bytes. It inventories headers and candidate sections,
not initialization transactions. Existing synthetic host-protocol tests
validate the recorder/reconstructor's mechanics but are not ASR firmware
captures.
