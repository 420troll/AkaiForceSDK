# ASR-10 SysEx, effect media, and OpenASR capture notes

**Research update:** 2026-11-22  
**Scope:** ASR-10/OpenASR factory ES5510 program recovery. No production Force
plugin or firmware payload was changed or added.

## Evidence summary

| Finding | Status | Evidence and limit |
|---|---|---|
| Effect parameters are controllable through the ASR's effect modulation/controller system. | **Verified from the ASR Musician's Manual** | The manual describes effect-specific parameter destinations and external control sources, with effect modulation enabled per instrument/track. This demonstrates parameter control, but does not itself prove a SysEx encoding. |
| The ASR External Command Specification likely includes algorithm-dependent effect parameter commands. | **Strong lead; exact command format not independently verified** | The R-Massive archive lists the official ASR-10/ASR-88 External Command Specification. A contemporary user report quoting its section 9.12 describes SysEx high-byte `0x30`, low-byte `0x00–0x09`, and algorithm-dependent parameters/ranges. The actual specification is hosted behind Mega and was not readable in this session. Do not implement a packet parser from the secondary quote alone. |
| SysEx has been shown to expose raw ES5510 instruction-memory bytes or an instruction-upload operation. | **Unresolved / no evidence found** | The public material located documents remote parameter commands, instrument/sample transfer, and storage of generic SysEx files. None of that proves raw ESP instruction-memory access. Factory effect files are separate disk objects with a candidate `FxUcode` section; runtime host upload must still be captured at the emulator/device boundary. |
| Standalone effect objects can be placed on an ASR-formatted disk image. | **Verified at the disk-tool/source level** | EpsLin Neo supports ASR image creation, EFE extraction/insertion, and identifies ASR effect-file directory type `33` (`0x21`). Its program is a disk/file utility, not an ES5510 disassembler. |
| One-effect disk creation and mounting in the current OpenASR GUI. | **Feasible; user's runtime result reported, not independently reproduced here** | EFE2IMG/EFE2HFE documentation says supplied EFE files are written to Omniflop images and HFE images; the multiple-file version groups inputs by folder. This supports a single-effect image workflow. The current checkout contains no 3.53 image, effect object, GUI capture, or new load log, so the reported successful 3.53 load cannot be independently tied to a specific artifact or DSP transaction here. |
| ASR service documentation supplies a complete ES5510 host map or initialization recipe. | **Not found in the material reviewed** | The service manual is useful for board/service diagnostics and identifies failed ESP downloads as a cause of dry, garbled, silent, or system-error output. That is diagnostic evidence, not a register map or microcode format. Existing OpenASR host-window documentation remains the better host-interface source. |
| OS 3.0 changes the effect-selection/editing workflow. | **No change established** | The ASR Version 3 OS documentation identifies new 44.1 kHz effects. The Musician's Manual describes effect mode selection, effect-file loading to the Bank Effect slot, and Edit/Effects parameter pages. The OS 3.0 addendum itself is listed in the archive but was not directly readable from its Mega-hosted copy in this session. |

## SysEx: what it can and cannot currently tell us

The public ASR Musician's Manual describes three effect contexts: instrument,
bank, and ROM. Instrument effects and their parameter sets are stored with
instruments; bank effects are global and may be replaced by loading an
independent effect file; ROM effects are selected from built-in algorithms.
Changing the selected effect changes the available effect-edit pages and
selecting a new effect initializes its parameters to defaults. These are
useful state transitions for a capture experiment, but none exposes
microinstructions by itself.

There is credible secondary evidence that the External Command Specification
has algorithm-dependent effect parameter SysEx under a `0x30` command family
(section 9.12, subcommands `0x00–0x09`). Treat this as a locator for the
official document, not as a verified packet definition. Before use, obtain and
read the actual ASR specification, record its revision and pages, and validate
one harmless parameter request/update against the running emulator. In
particular, do not assume that a parameter request returns GPR contents: it
may return only a user-facing parameter value.

No retrieved source shows SysEx commands for dumping an ES5510's instruction
RAM, GPR file, special registers, or external delay RAM. SysEx object transfer
and SysEx parameter access are not equivalent to an ESP host-interface trace.
The efficient path to authentic program recovery remains: load an effect file
through firmware, then record the firmware-generated ES5510 host writes and
readbacks.

## Disk format/tool comparison

EpsLin Neo v1.58 source (`handsthatstrike/EpsLinNeo`, commit
`cf20955e6f9f64dca3c06deaa220598e5fe1bd08`, GPL-2.0) defines the ASR floppy
image size as 1,638,400 bytes (3200 512-byte blocks), the two-block directory,
FAT placement, and ASR effect type `33` / `0x21`. Its `GetEFEs` follows
contiguous extents and then FAT links, so fragmented effect files are copied
correctly. `PutEFE` accepts individual EFE/EFA/INS files and writes the
directory/FAT/data blocks into an image. The R-Massive guide documents
conversion from EDA/EDE/IMG/EFE to Omniflop/HFE formats and EFE-to-disk
generation.

Our existing `tools/asr10_eda_to_img.py` has a narrower role: it expands the
EDA skip map into raw sectors and verifies the exact 3200-block output. It
does not read or write Ensoniq directory/FAT structures. `tools/asr10_disk_catalog.py`
then validates the ASR volume signatures, root entries and FAT chains, and
can export only OS/effect payload blocks to the ignored private research area.
The existing tools are intentionally read/validation-oriented; EpsLin adds the
missing disk-image write path. No EpsLin code was copied into this repository.

### Reproducible single-effect media procedure

For a legally held individual EFE exported from a private image:

1. Make a fresh, private 1.6 MB ASR raw image using EpsLin's ASR image-format
   mode (`-fi image.img asr`).
2. Insert only the chosen EFE at a non-OS directory index (`-p1 image.img
   effect.efe`). Confirm the directory has one effect object and no unexpected
   entries with EpsLin's listing/check mode and our `asr10_disk_catalog.py`.
3. If OpenASR expects a raw floppy image, use the raw `.img`; if it expects
   HFE, convert that image with the documented EFE2HFE/HxC utility or supported
   image converter. Do not pass HFE to a raw-image slot by assumption.
4. Boot the known-good OS image separately, mount the generated effect image
   as a second floppy, navigate to Load → Effects, and load the effect. Record
   the exact UI action, image hash, effect name, and emulator build.
5. Start the ES5510 transaction recorder before load. Retain the event log
   privately and publish only metadata/hashes and redacted summaries.

Steps 1–3 are supported by public tool documentation and source. Step 4's
success is **reported by the user for the current OpenASR GUI and OS 3.53**;
the independent reproducibility record is pending. Step 5 is the decisive
capture: require instruction commits, GPR writes, special-register and delay
memory setup, HALT/RUN transitions, and parameter edits before calling the
result a complete program.

### Synthetic media smoke test

EpsLin Neo v1.58 source was compiled locally (not copied into the repository):

```sh
gcc -O2 -Wall -Wextra /tmp/EpsLinNeo/EpsLin_v1.58.c \
  -o /tmp/epslin-native -lm
/tmp/epslin-native -q -fi /tmp/test/one.img asr
cd /tmp/test
/tmp/epslin-native -q -p1 one.img fake_effect.efe
/tmp/epslin-native -D one.img
python3 tools/asr10_disk_catalog.py one.img
```

The build succeeded with compiler warnings in legacy paths (including possible
uninitialized variables and bounded string-copy warnings); the one-entry
synthetic fixture formatted, inserted, listed, and passed our raw-image
signature/directory/FAT validation. It reported directory index 1, type 33,
name `SYNTH TEST`, one 512-byte content block. Example image SHA-256:
`7dd4db17e9d928c8fd324fb076a497539dd004595b0cdd7266c4612d2388da92`.
This is strictly a disk-format test: the fake effect contains no authentic
code, was not mounted in OpenASR, and says nothing about the sound engine.

## ESP architecture implications

The ASR service manual's ESP failure guidance is operationally useful: an ESP
download fault can manifest as a system error or dry/garbled/silent output.
It supports using “effect loaded and audible” as a useful functional test, but
not as proof that the exact program boundary, GPR initialization, delay-RAM
state, or physical timing is correct. The manual material found does not
improve the current ES5510 host-register model beyond what is already
documented from OpenASR and MAME. Keep ASR-specific host addresses separate
from DP/4 wiring.

## Next capture experiment (highest value)

Use the newly reported working OS 3.53 flow; do not return to ROM-only
guesswork. Start passive logging at the ES5510 host-interface boundary, then:

1. Boot OS 3.53 and record the build, ROM/OS identifiers, mounted image names,
   and image hashes privately.
2. Load exactly one independent effect object from the one-effect image.
3. Save the complete ordered host transaction trace: host byte latches,
   read-select/readback, instruction/GPR/combined commits, special-register
   writes, external-memory setup, and HALT/RUN.
4. Change a single effect parameter and record the delta in host transactions.
5. Repeat the same sequence from a clean boot and compare commit stream and
   final state hashes.

If the effect loads but no instruction commits occur, determine whether the
selected item was an effect file, an instrument carrying an effect, or a ROM
effect. If commits occur but the decoder cannot identify a stable boundary,
correlate the ordered commits with the selected object's private `FxUcode`
candidate and its surrounding data. Do not infer a program only from matching
instruction-looking bytes.

## References (checked 2026-11-22)

- Ensoniq, *ASR-10 Musician's Manual*, Section 2 (MIDI SysEx and program
  changes), Section 5 (effect selection/concepts), and Section 6 (effect
  parameter editing):
  <https://www.vintagesynth.com/sites/default/files/2017-05/ASR10_manual.pdf>
- Ensoniq, *ASR-10/ASR-88 Service Manual*, ESP troubleshooting and service
  diagnostics:
  <https://www.deepsonic.ch/deep/docs_manuals/ensoniq_asr_service_manual.pdf>
- R-Massive, [Ensoniq ASR/EPS Archive](https://zine.r-massive.com/ensoniq-asr-eps-archive/)
  (external-command spec, OS 3.0 addendum, service manual, EFE2IMG/EFE2HFE
  descriptions and image creation workflow).
- R-Massive, [Ensoniq technical docs and schematics](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/)
  (official ASR/ASR-88 External Command Specification is listed; current
  linked copy is Mega-hosted and was not inspected directly).
- [EpsLin Neo source](https://github.com/handsthatstrike/EpsLinNeo), GPL-2.0,
  inspected at commit `cf20955e6f9f64dca3c06deaa220598e5fe1bd08`; especially
  `EpsLin_v1.58.c` constants, `GetEFEs`, `PutEFE`, and image-format code.
- Steve Quartly & Gary Giebler, *Ensoniq EFE File Format*, listed in R-Massive
  technical-document index. Use as a container-format reference only.
- Contemporary 1990s ASR discussion quoting External Command Specification
  §9.12 (secondary evidence):
  <https://manuals.plus/m/f154ef6a8bd9bea6b69a44ae5e16f80b9ef7522ea5afdd81490b61ce63fd0584.pdf>

