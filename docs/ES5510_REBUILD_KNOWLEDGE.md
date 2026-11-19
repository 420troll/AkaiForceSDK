# TROLLfx 5510 rebuild knowledge base

## Direction and boundary

The active development target is a new plugin named **5510**, manufacturer
**TROLLfx**, in `plugins/TROLLfx5510/`. Proposed four-character VST identity:
`T551` (audited against repository and connected Force plugin manifests at
branch creation; repeat the UID audit before release). The new project must
use the MAME-derived ES5510 instruction engine as its only effect DSP path.
Its fixed signal graph is **A → B → C → D**. The host may convert float audio
to/from the ES5510's 16-bit serial interface, assign GPR values, calculate
modulation/tempo trajectories, mix dry and processed paths, and manage UI/state.
It must not substitute C++ effects for effects represented by instructions.

The native algorithms in `src/original/` and older ESP Lab plugin projects are
legacy release implementations. They remain in Git and are not linked into
`TROLLfx5510`. The Force's installed ESP Lab V0.3 and V0.4 folders remain in
place during this rebuild. No ROM, captured factory program, or Force plugin
installation is part of this branch.

## DP/4 firmware and extraction research

The full reports are retained alongside this summary:

- `docs/DP4_ROM_MICROCODE_FORENSICS.md`
- `docs/DP4_U56_LOADER_ANALYSIS.md`
- `docs/DP4_BANKED_ACCESS_TRACE.md`
- `docs/DP4_EXTRACTION_FEASIBILITY_DECISION.md`
- `docs/DP4_MICROPROGRAM_EXTRACTION_PLAN.md`
- `docs/DP4_HARDWARE_ARCHITECTURE_AUDIT.md`
- `docs/DP4_BASE_BOARD_BUS_MAP.md`

ROM bytes, archive contents, Ghidra projects, full disassemblies, and extracted
records stay under private research storage (`/tmp/dp4-rom-forensics`); never
add them or derived firmware/microprogram payloads to Git or a plugin package.
The public reports contain only acquisition metadata, hashes, non-substantial
structure statistics, tools, and conclusions.

| Finding | Evidence status | Development consequence |
|---|---|---|
| U54 is the 128 KiB DP/4 VCODE/UCODE ROM; U56 is the 32 KiB OS ROM. Private v1.06 and v1.15 pairs were identified and compared. | Verified by service manual designators, image sizes, archive names and private-image checks. Member hashes are recorded in `DP4_ROM_MICROCODE_FORENSICS.md`. | Preserve the conclusion and metadata; do not reproduce bytes. |
| Base DP/4 U52 is a 6803-family CPU. Base-board Rev J schematic maps U56 to `$8000–$FFFF`; reset-vector `$FFFE` maps to file offset `0x7FFE`, target `$84E4` to offset `0x04E4`. | Verified by service manual, base-board schematic, vector and coherent U56 startup. Mode 2 is strongly supported by low internal-register accesses, but reset strap levels remain unmeasured. | Do not use the DP/4+ 63803 map as the base DP/4 map. |
| U54 is banked through U57 B0–B3 into U54 A13–A16. U56 selects bank 9 and reads CPU `$2000`; the read corresponds to U54 file offset `0x12000`. The tags are `0x0106` and `0x010F` for the matching private v1.06/v1.15 pair. | Verified cross-version read/tag relationship; exact U57 latch edge and full chip-select equation remain open. | This is a ROM identity check, not a program fetch or upload. |
| U56 has a four-entry selector descriptor table indexed by a low two-bit slot and routines that write the bank latch at `$0600`. Paired bank values are `(0,0x0B)`, `(0,0x0B)`, `(1,0x0C)`, `(2,0x0D)` in both revisions. | Firmware data flow verified. Calling the entries DSP A/B/C/D slots or calling values algorithm banks is still an inference. | Keep slot semantics unverified pending a bus trace. |
| U54 contains 50 × 51-byte algorithm-name records, 50 × 87-byte effect/preset labels, and 50 × 158-byte configuration labels. v1.06/v1.15 label regions are identical; 3,351 bytes differ elsewhere. Six uniform 48-bit raw packing/layout hypotheses did not identify credible program boundaries. | Geometries and revision difference counts verified; record payload fields and the changed regions' meanings unresolved. | Names and records are not evidence of executable microcode. Do not scan/relabel them as programs. |
| U56 `$1800/$1801` traffic maps to U59's 6850 ACIA. A computed call candidate `$82C9 → $30EC` has unresolved runtime object and bank state. | ACIA identification verified on base schematic; indirect-call destination is only a Ghidra candidate. | Neither is a confirmed ES5510 loader. |
| No U56 routine has been tied to an ESP select plus instruction/GPR host writes. MAME has no complete DP/4 machine driver. | Unresolved after bounded two-revision Ghidra/data-flow work. | Factory microprogram recovery is not a prerequisite for original ES5510 microprogram development. Do not build a guessed full DP/4 emulator. |

The decisive extraction experiment remains a common-timebase capture of U54
address/data/bank/read strobes and a selected ES5510 host bus/select/HALT
transaction during cold boot and one algorithm change. Board revision, exact
silicon pinout, buffering, reset and clock must be recorded. U54-only or DSP
only traces cannot prove source-to-destination transfer. The complete channel
plan is in `DP4_EXTRACTION_FEASIBILITY_DECISION.md`.

## ES5510 model and MAME foundation

Reference source is MAME commit
`8be57cdc5f44ab830dff44c06dd39f24190742d4` (BSD-3-Clause). The pinned source
is inspected in `docs/MAME_ES5510_ANALYSIS.md` and `docs/ES5510_ARTIFACT_AUDIT.md`.
Third-party notice must accompany source/binary redistribution:
`experimental/emulation/THIRD_PARTY_NOTICES.md`.

MAME models a 160-word, 48-bit microprogram store; 192 24-bit GPRs; a host
instruction latch written as six bytes; host GPR writes as three bytes; selector
transactions for program/GPR commits; parallel ALU/MAC pipelines; condition
flags and skip; serial registers and DIL/DOL; delayed delay-RAM operations;
MEMSIZ/address generation; and END/HALT frame execution. MAME host register
offsets and write sequences describe the MAME device model. They are **not**
confirmed DP/4 CPU addresses or a captured base-DP/4 upload transcript.

MAME has ES5510 device support in EPS/VFX-family and other Ensoniq drivers, but
device support is not complete DP/4 machine support. MAME's ASR-X driver uses a
disabled ES5510 placeholder for ES5511; it does not establish working ESP2
execution. The portable standalone core is a bounded adapter of selected MAME
behavior, not a linked full MAME runtime. It retains unimplemented MAME I/O
callbacks and still needs differential comparison against the pinned device.
Do not describe it as a complete or cycle-accurate chip emulator until those
gaps are tested.

Current standalone source/test set:

- `experimental/emulation/mame_es5510_standalone.hpp/.cpp`
- `experimental/emulation/mame_es5510_standalone_test.cpp`
- `tools/es5510_render.cpp`, `tools/es5510_bench.cpp`
- `tools/es5510_decode.py`, `tools/es5510_asm.py`
- `experimental/emulation/programs/`

Its first original 3-word `original_stereo_swap.esp` program executes an exact
two-frame stereo swap through serial registers. The test establishes this
implemented path, not all ISA behavior.

The previous Phase 2 delay and chorus examples are original TROLLfx programs,
not Ensoniq factory effects:

- `original_stereo_feedback_delay.esp`: 11 instructions/frame; serial input,
  delay RAM reads/writes, DIL/MAC feedback, ALU sum, serial output. GPR 40 is
  a Q22 feedback coefficient; MEMSIZ zero means one address unit per 16-bit
  DRAM word. Previous bundle defaults used 250 ms and 55% feedback, with a
  10-second stereo ring budget. Delay/mix behavior is instruction executed.
- `original_stereo_chorus.esp`: 11 instructions/frame; delayed wet signal is
  MAC-scaled and mixed with dry in the instruction program. Its LFO read
  offsets are host-computed GPR updates with opposite phase. The host LFO is
  not an ESP instruction oscillator; audio storage/mixing remains ES5510 work.
- The WAV renderer originally rejected `lfo_gpr`; the rebuild branch adds
  parser/render support for deterministic host-scheduled modulation. This is
  an integration improvement, not a change to ES5510 instructions.

Past results documented in `docs/HANDOFF_EMULATION.md` and
`artifacts/es5510-emulation/phase2/` include native and ARMv7 cross-builds,
stereo-swap output, delay/chorus renders, and Force-side benchmark reports.
Those old timings establish that small diagnostic programs can run at useful
speed, not final VST real-time performance with four units. New release claims
must be measured again on the actual current plugin.

## Force SDK and deployment facts

The official MPC plugin SDK is in-repository under `framework/` and `tools/`;
the host wrapper is C++17/VST2 with `VSTPluginMain`. Build manifests declare a
32-bit hard-float ARMv7/Cortex-A17 binary (`arm-linux-gnueabihf.2.31`) for the
original Force and may also declare ARM64 for later devices. SDK `prepare()` is
for allocation/engine setup; `process()` receives non-interleaved float
buffers and varying blocks (32–512 in host simulation). The Force standalone
path is normally 44.1 kHz; native diagnostics also use 48 kHz. UI is JSON
(`ui.json` to generated TUI/Q-Links); no native plugin editor is opened. Stable
parameter IDs and normalized atomics support preset/chunk save/restore.

The real Force was inspected over the existing SSH alias `akai-force`:

- SSD plugin root: `/media/SSD250g/VST2 Plugins/`.
- exFAT mount was `rw,nosuid,nodev,...,fmask=0022,dmask=0022`, with no `noexec`.
- Existing ESP Lab V0.3 uses UID `Tfx3`; V0.4 Series uses UID `TfxS`.
- The V0.3 reverb evaluation build is installed at the existing V0.3 path;
  the original V0.3 backup is
  `/media/SSD250g/.trollfx-backups/esp-lab-v03-pre-reverb-20261118-212328`.
- RTA has a separate plugin identity and directory. Do not change it.
- Plugin discovery occurs through MPC External Plugins support. A new folder
  should be added under `VST2 Plugins`; a new or changed binary may need an MPC
  rescan/restart before a real MPC insert loads it. SDK host simulation is not
  proof of browser discovery or listening quality.

The new `5510` plugin must remain a separate install folder and UID until it
passes native/ARM build, direct ARM host simulation, Force browser insertion,
and audio audition. Do not replace V0.3/V0.4 during this phase.

## Legacy preservation

The native approximate effects under `src/original/`, earlier `TROLLfxESPLab`
projects, and their algorithm demos remain in history/source for rollback and
the already-installed releases. They must not be included or called by
`plugins/TROLLfx5510`. Continue to maintain the A→B→C→D serial contract in the
new plugin; do not bring back selectable parallel/feedback route options in
this product unless explicitly changed by the user.

## Rebuild milestone implemented

The new separate project is `plugins/TROLLfx5510/`, with exact product name
**5510**, UID `T551`, and fixed four-unit A→B→C→D stereo routing. Each unit
owns an independent MAME-semantics processor and 1,048,576-word DRAM. The
active plugin includes the standalone ES5510 adapter and original TROLLfx
instruction programs only; it does not link or call `src/original/` or any
legacy approximate algorithm. Existing ESP Lab projects and installed Force
folders were left unchanged.

The plugin currently exposes instruction-executed stereo feedback delay and
stereo modulated delay/chorus. Time and signed feedback update delay/GPR
coefficients; Chorus time, depth and rate update read GPRs from a host LFO
lookup table. Wet/dry blending and float/16-bit conversion are host-side
integration. The delay storage, feedback, and program signal mixing run through
the ES5510 instruction words.

Native, ARMv7 and ARM64 SDK builds, strict validation, and 44.1/48 kHz host
simulations passed. See `docs/HANDOFF_5510_REBUILD.md` for exact commands and
results. No hardware installation or audition occurred. Next, measure the
current four-engine plugin on the Force/ARMv7 hardware and differential-test
more instructions against pinned MAME before adding microprograms. No DP/4
factory effect was identified or recovered.
