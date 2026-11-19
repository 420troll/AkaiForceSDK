# ASR-10 ES5510 research baseline

Date of this pass: 2026-11-19. Research branch: `feature/5510-asr10-factory-programs`.

## Project state carried forward

The 5510 plugin at `plugins/TROLLfx5510/` has four independent MAME-semantics
ES5510 units in fixed A → B → C → D routing. Its delay/chorus programs are
original TROLLfx diagnostics, not Ensoniq factory algorithms. The prior
handoff records native, ARMv7, ARM64 builds and host simulation at commit
`d1b1b631694a490ffdfde813e25df30ff7b5a93b`; this research branch does not
change the plugin or its UID `T551`.

## Evidence inventory

| Area | Verified | Unresolved / boundary |
|---|---|---|
| ES5510 execution | Existing standalone adapter, program loader, render tool, 48-bit instruction decoder, original delay/chorus bundles and tests are in `experimental/emulation/` and `tools/`. MAME source is BSD-3-Clause. | Adapter is not full host/system emulation. Existing programs are not factory code. |
| MAME source | Pinned locally to `mamedev/mame` `8be57cdc5f44ab830dff44c06dd39f24190742d4`; instruction/register/RAM/host behavior is documented in [MAME_ES5510_ANALYSIS.md](MAME_ES5510_ANALYSIS.md). | `es5510d.cpp` is a stub at this revision; it does not provide the asserted file-format importers. |
| OpenASR source | `peroberg/OpenASR` `241f9331477b07b072330ca6ebd2d7a2e2cc832e`, CC0 project license. The ASR-10 map has ES5510 host windows and PA4 glue; details in [ASR10_ES5510_ARCHITECTURE.md](ASR10_ES5510_ARCHITECTURE.md). | Current machine config calls `set_disable()` on the ES5510 core. Host transaction modeling does not establish correct DSP execution or physical behavior. |
| Prior V3.50 traces | OpenASR research notes report firmware execution reaching upload/readback and reconstruct two selected program images through their END indices; list hashes and terminal words are summarized in [ASR10_FACTORY_LOADER_ANALYSIS.md](ASR10_FACTORY_LOADER_ANALYSIS.md). | Raw logs, full instruction arrays, GPR snapshots and exact input-image hashes are unavailable here. The reports cannot be replayed or independently audited from this checkout. |
| ASR factory format | No native ASR-10 effect-file or OS-image format has been validated as a directly executable ES5510 stream. Firmware host commits are the relevant observable boundary. | Whether effect objects contain code, parameters, or both; program library boundaries; algorithm-to-object mapping remain unresolved. |
| DP/4 | Keep all DP/4 U54/U56 studies, ROM bank evidence, Ghidra loader analysis and hardware-capture decision in the existing docs. | DP/4 CPU/bus mapping differs from ASR-10. DP/4 findings do not identify ASR-10 host transactions. |
| Hardware | Previous project handoffs describe Force ARMv7 builds and installation practices. No Force is touched by this branch. | No physical ASR-10 bus capture or Force test is part of this pass. |

## Inputs and privacy

No ASR-10 OS disk image or system ROM was present in the OpenASR checkout or
the checked local paths during this pass. Two Chicken Systems OS self-extractor
wrappers (1.61 and 3.53) were acquired under `research_local/`, hash-verified,
and inspected as inert NE data; their embedded EDA payloads were not extracted.
OpenASR's `floppies/README.md` and `roms/README.md` require user-provided images
and distribute none. Do not place OS images, ROMs, factory effect payloads,
reconstructed programs, or full disassemblies in this repository.

## Reproducible source revisions

- OpenASR: `peroberg/OpenASR` commit `241f9331477b07b072330ca6ebd2d7a2e2cc832e`,
  `src/mame/ensoniq/asr10_boot.cpp`.
- MAME ES5510: `mamedev/mame` commit
  `8be57cdc5f44ab830dff44c06dd39f24190742d4`, `src/devices/cpu/es5510/`.
- Project baseline: `420troll/AkaiForceSDK` commit
  `d1b1b631694a490ffdfde813e25df30ff7b5a93b`.

The next concrete inputs are a user-lawfully held matching boot-ROM pair and
the extracted 1.61 or 3.53 OS disk image. Prefer those releases until the
OpenASR V3.50 compatibility claim is resolved against the service manual. Run
a passive host-bus trace during boot, effect selection and one parameter edit;
retain image hashes privately and preserve the ordered tap log outside Git.
