# Research index

Research date: 2026-11-19 UTC. Evidence labels used throughout:

- **Verified**: directly supported by a cited primary document, upstream code,
  build/test output, or repository state.
- **Inferred**: a reasoned interpretation that still needs measurement or a
  more direct source.
- **Unresolved**: conflicting, incomplete, or absent evidence.

## Recovery inventory

| Item | Result | Evidence/status |
|---|---|---|
| Local `TROLLfx-DP4-Recovery` | Not found under `/mnt/c/Users/Gamer` in a name search to depth 6 | Verified for searched filesystem scope; deeper/offline storage unresolved |
| Local `TROLLfx-Force-SDK` | Not found before this handoff | Verified; this repository was initialized during Handoff 001 |
| Local Ensoniq/ES5510/DP4 notes | No textual matches in the inspected `Desktop/akaiVibe` and `TROLLfx-Helios-Mirror` trees | Verified for those trees |
| Nearby Force work | Multiple existing TROLLfx plugin projects and `PROJECT_STATUS.md` under `Desktop/akaiVibe`; none contained Ensoniq/ES5510/DP4 matches | Verified; no files were modified |
| GitHub account/installation | Searches for `TROLLfx` exposed only private `420troll/TROLLfx-Spatial-Studio`; neither named recovery repository was present | Verified for the connected GitHub installation and public repository search |
| Git metadata in `Desktop/akaiVibe` | `.git` directory exists but contains no normal repository metadata; `git status` reports not a repository | Verified |

## Primary technical sources

| Source | What it supports | Provenance notes |
|---|---|---|
| [Ensoniq ESP (ES5510) Technical Specification v2.4](https://drive.google.com/file/d/1dBr7-l7GJVJIKj2Y-Th4l1RPE24h2YUW/view) | ES5510 primary chip specification | Public scan; local research copy SHA-256 `822edf5f79b54a2e20c6b9b4b10be572a56eba212c5bc49835decb6b06ba74a0`; scan was not committed |
| [Ensoniq Signal Processor 2: Instruction and Hardware Specification](https://drive.google.com/file/d/1P6nKQmOaObH1qeVMYLeC4KAUSwX4LBal/view) | ESP2 architecture, instruction execution, registers, AGEN, external memory, serial and host interfaces | Authored by David Andreas, William Mauchly, Jon Dattorro; local research copy SHA-256 `9b6c67915e26506942970dd62f0a520deeb5072338b7551c63619053766ba92c` |
| [Unofficial ESP microcode programming guide](https://drive.google.com/file/d/1ApgQCMRmZU_dpVMTadUc4rn1PVqLVkED/view) | EPS-16 Plus/ASR effect-file structure and relationship between 68k parameter code and ESP microcode | Secondary/unofficial; SHA-256 `997f40169912eb7ad6a4fec34e04e50e6339842205ecd576d5b33fa5c67101b4`; do not treat as an Ensoniq specification |
| [DP/4 Musician's Manual](https://umlsrt.com/StudioDocuments/Ensoniq%20DP4%20Manual%20.pdf) | Four processing units, routing/configs, public algorithm/parameter catalog | Ensoniq product manual; SHA-256 of accessed copy `667e3520412d9fca0f6e93396f1b286e8f9fbdfeaa60687d4ac9b20913c31240` |
| [DP/Pro Reference Manual](https://www.handelima.net/studiogear/manuals/Ensoniq/ENSONIQ%20DP-PRO/DPpro_manual.pdf) | Two second-generation ESP2 processors, routing, 42 public algorithm families and parameter ranges | Ensoniq product manual; SHA-256 `ca928203497364103cb4231b993fc73485f5b3f9ff01b5d5482df9e8f38849ef` |
| [EPS-16 Plus service manual](https://www.vintagesynthparts.com/wp-content/uploads/2018/01/Ensoniq-EPS16-Service-Manual-with-searchable-text.pdf) | ESP test names, ESP RAM/GPR/instruction diagnostics, public factory-effect names and historical OS notes | SHA-256 `5b95c60687c590b1ca5f785b7b933ea62ae405c2670c6f30ac96d1dbbb973e7f` |
| [ASR-10 effects musician's manual](https://datassette.s3.us-west-004.backblazeb2.com/manuais/ensoniq_eq_musicians_manual.pdf) | Effect selection/loading behavior, buses, user-visible algorithms and parameters | SHA-256 `e8ab5100904caf91fe355065ed9646aff197761b0663a7d452dc98af1f9a29dc` |
| [Ensoniq technical-document catalog](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) | Discovery index for chip specs, manuals and schematics | Secondary index; links to the public scans above |
| [ASR/EPS archive](https://zine.r-massive.com/ensoniq-asr-eps-archive/) | Lists factory sound-disk bundles, OS versions and ASR-10 1.50b EPROMs | Third-party index; linked Mega items not downloaded because redistribution status was not established |
| [Ensoniqsamplers OS index](https://www.ensoniqsamplers.com/epsasr/026.html) | Links ASR OS 1.61 and 3.53 update packages; describes versions and ROM requirements | Direct Chicken Systems NE wrappers were downloaded as inert data to ignored `research_local/`, hash recorded in `ASR10_FIRMWARE_INVENTORY.md`; payload not extracted |
| [OpenASR](https://github.com/peroberg/OpenASR) | ASR-10 MC68302/ES5510 machine model, host interface map, media requirements, prior reported V3.50 loader traces | Inspected commit `241f9331477b07b072330ca6ebd2d7a2e2cc832e`; no ROM/OS images distributed in checkout; current ES5510 config is disabled |
| [EpsLin Neo](https://github.com/handsthatstrike/EpsLinNeo) | ASR/EPS disk-image and EFE directory/FAT structure; EFE extraction utility | GPL-2.0, inspected commit `cf20955e6f9f64dca3c06deaa220598e5fe1bd08`; compiled only in `/tmp` for CLI inspection; no code copied |

The public manuals were used for research only and are deliberately absent
from this repository.

## Emulator and implementation sources

| Project | Revision inspected | Finding | License/reuse status |
|---|---|---|---|
| [MAME ES5510 core](https://github.com/mamedev/mame/tree/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510) | `8be57cdc5f44ab830dff44c06dd39f24190742d4` | Executable ES5510 model, internal instruction-field logger/decoder, host interface, 24-bit ALU/GPR model, 48-bit instruction/MAC path, delay/table addressing, serial pump | BSD-3-Clause in each source header and MAME license; `es5510d.cpp` disassembler entrypoint is a stub at this pin; no raw-file importer. Attribution/conditions required |
| [MAME Ensoniq drivers](https://github.com/mamedev/mame/tree/master/src/mame/ensoniq) | same | Product mappings for VFX/EPS/EPS-16+/SD/SQ/KT/TS/ASR/MR; ES5511 machines currently use or mention the ES5510 device only as a disabled/skeleton substitute | BSD-3-Clause; driver comments are useful evidence but not a primary hardware specification |
| [mardlib Ensoniq EPS-16 Plus](https://github.com/mardlib/Ensoniq-EPS-16-Plus) | default branch inspected 2026-11-15 | Claims a standalone ES5510 core adapted from MAME and requires user-supplied ROM/OS; useful independent integration/test lead | No top-level `LICENSE` or `LICENSE.md` was found. `THIRD_PARTY_NOTICES.md` covers dependencies, not necessarily all original repository code. Do not copy code until licensing is clarified |
| [Musashi](https://github.com/kstenerud/Musashi) | not cloned | 68000 core used by the EPS emulator | Permissive notice reproduced by the dependent project; re-check upstream revision before use |

MAME history is active evidence, not proof of chip perfection. Relevant
upstream commits include `313473d` (2026 opcode/flag fixes), `6c483aee`
(DOL rounding investigation), `206047f` (register-write macro fix), and
`d154d2a` (host DRAM address mapping). Current source still marks DRAM size as
unverified and retains a selectable/under-investigation DOL rounding behavior.

## Official Akai/InMusic SDK source

- Repository: [inmusic-software/mpc-plugin-builder-sdk](https://github.com/inmusic-software/mpc-plugin-builder-sdk)
- Inspected commit: `febd2ca5bd9eb16c4a412837196e145566bc981b`
- Commit date/message: 2026-11-07, `Add plugin format to plugin.json`
- SDK license: MIT; its README states it contains no Steinberg VST SDK code.
- Verified build and host results are in `SDK_COMPATIBILITY.md`.

## ASR-10 acquisition and loader handoffs

- `ASR10_CAPTURE_SPRINT.md`: OS 3.53 OpenASR preflight, passive bus/PA
  recorder validation, private launcher, user test steps, and strict replay
  gate.
- `ASR10_SYSEX_AND_SINGLE_EFFECT_CAPTURE.md`: separates likely SysEx effect
  parameter access from unproven instruction-memory access; compares EpsLin
  Neo against the private-safe catalog/extraction tools; documents the
  single-effect image workflow and the new user-reported OS 3.53 GUI load.
- `ASR10_FIRMWARE_INVENTORY.md`: acquired NE wrapper hashes, absent boot ROM
  fingerprints, missing OS image/effect media, archive-source status, and the
  V3.50 ASR-10/ASR-88 compatibility conflict.
- `ASR10_FACTORY_LOADER_ANALYSIS.md`: historical V3.50 host upload/readback
  path, prior reported ROM-HALL/44LUSH endpoints/hashes, and what is missing for
  replay.
- `OPENASR_DSP_TRANSFER_TRACE.md`, `ASR10_FACTORY_PROGRAM_CAPTURE.md`, and
  `5510_FACTORY_EXECUTION_VALIDATION.md`: passive trace tooling and explicit
  non-claims. No complete factory program is checked in.
- `tools/asr10_firmware_inventory.py` hashes/classifies private inputs without
  executing Windows installer files. `.gitignore` excludes the entire local
  `research_local/` tree.

## Evidence table

| Claim | Status | Basis / caveat |
|---|---|---|
| ES5510 is the first-generation Ensoniq ESP identifier used by MAME and public chip material | Verified | ES5510 spec title and MAME device/source headers |
| `ES5100` is an alternate identifier for the Ensoniq ESP | Unresolved; currently unsupported | Exact public searches produced no relevant Ensoniq source; no inspected primary document uses it |
| ESP1 uses 24-bit registers/data and 48-bit microinstructions/MAC state | Verified for the MAME model; primary scan should be manually page-checked | MAME fields, masks, register map, 48-bit instruction latch and accumulator path |
| ESP1 has 160 instruction slots and 192 writable GPRs in MAME | Verified for MAME, not yet independently hardware-verified | MAME allocates 160 instructions, resets 160, and treats registers below `0xc0` as GPRs |
| ESP1 external delay memory is 16-bit in the currently modeled DIL/DOL path | Verified for MAME and supported by its cited spec excerpt | MAME uses 16-bit DRAM and left-justifies DIL/DOL in the 24-bit data path |
| MAME's configured 1M-word ES5510 DRAM size is authentic for every product | Unresolved | Source explicitly says size is not verified and most games use 128 KB |
| ESP2/ES5511 is a substantially expanded architecture, not a rename | Verified | Primary ESP2 spec: parallel MAC/ALU/AGEN, 96-bit instructions, 10-bit operands, many more registers, eight regions, 24-bit external bus |
| ASR-10 uses ES5510, while ASR-X and MR use ES5511 | Verified by current MAME driver comments; board/service confirmation still desirable | `esqasr.cpp` and `esqmr.cpp`; ASR-X/MR emulation is skeletal |
| DP/4 uses four ESP processors and supports serial, parallel, and ganged routing | Verified at user-visible level | DP/4 manual, sections on routing and algorithms |
| DP/Pro uses two second-generation 24-bit ESP2 processors | Verified | DP/Pro manual introduction and algorithm/routing chapters |
| Product manuals reveal exact original microcode/topology | False | They reveal behavior, ranges and conceptual diagrams; not instruction images or complete coefficients |
| Original Ensoniq algorithms have been recovered here | False | No proprietary algorithm image or verified microcode reconstruction exists in this repository |
| A useful Ensoniq-inspired Force effect requires instruction-level emulation | Hypothesis rejected as a project dependency | Native DSP can reproduce documented behaviors efficiently; emulation remains valuable for research/validation |

