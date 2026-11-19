# Base DP/4 CPU, ROM, and ESP bus map

Branch: `issue11-dp4-rom-forensics`
Baseline: `c06ac8992f1e4ff3d20dab0f51f901e5637b543f`
Status: schematic-backed static findings; no live board capture has been made.

## Primary result

A complete base DP/4 main-board schematic is publicly available. The copy reviewed here is titled **PCB Schematic, Main Board, DP-4**, drawing **4010015001**, revision **J**, sheets **1–5**. Sheet 2 is the digital section. It is the base DP/4 drawing, not the separate DP/4+ Rev C drawing. The downloaded PDF and all image extracts remain under `/tmp/dp4-rom-forensics`; neither is copied into this repository.

The drawing confirms the CPU-to-ROM buses and identifies four separate ES5510 host interfaces. A cross-revision U56 tag check now corroborates the U54 bank-9 access window. The complete U54 chip-enable/alias equation and the ESP decoder's full high-address enable terms have not yet been reduced to Boolean equations. No absolute ESP host-register transaction has been observed on hardware.

## Confirmed hardware connections

| Component / signal | Schematic evidence | Conclusion |
|---|---|---|
| U52 | Sheet 2 labels U52 `6803`; address lines A8–A15, multiplexed A/D0–A7, E, AS, and R/W connect to the digital bus. The service manual independently names the 6803 at U52 (printed p. 6). | U52 is the DP/4 host CPU and the bus master for the ROM and ESP host interface. The base DP/4 is not a 68030 system. |
| U56 | Sheet 2 labels U56 `27256`; A0–A14 and D0–D7 join the U52 bus. Its active-low CE is driven by U37/C (74F00) from A15 and +5 V, selecting the high half when A15 is high; its output-enable is gated by the read/write control. | U56 occupies CPU addresses `$8000–$FFFF` (32 KiB). The 6803 reset-vector fetch at `$FFFE–$FFFF` maps to U56 file offsets `$7FFE–$7FFF`; the private v1.06/v1.15 vector `$84E4` therefore enters at file offset `$04E4`. |
| U58 | Sheet 2 labels U58 `62256`; A0–A14 and D0–D7 join the CPU bus, with distinct CE/OE wiring. | A 32 KiB external SRAM is confirmed. Its exact CPU address range and interaction with the banked U54 window are not established until its complete CE path is traced. |
| U54 | Sheet 2 labels U54 `27C010`. A0–A12 and D0–D7 connect to the U52 address/data bus. U54 A13–A16 connect to U57 outputs B0–B3. U54 CE/OE are separate from the U56 decode. | A firmware-correlated 8 KiB CPU window is at `$2000–$3FFF`: the U56 code selects bank 9, reads `$2000`, and checks a revision tag matching physical U54 offset `0x12000` in both revisions. The complete CE equation and any aliases remain unresolved. |
| U57 | Sheet 2 labels a 74HCT377; its outputs include B0–B3 and HALT1–HALT4. B0–B3 route to U54 A13–A16; HALT1–HALT4 route to the corresponding ESP halt inputs. | The same latch provides four U54 bank bits and four independent DSP halt controls. U56 writes `$0600` with bank 9 before the U54 bank-9 tag read; the exact latch edge and full address decode remain to be traced. |
| U61, U62, U83, U84 | Sheet 2 labels four `5510` ESPs. Each has its own ESP1–ESP4 select, HALT1–HALT4, and local DRAM signals; their host/data and control connections reach the shared U52 bus. | The base DP/4 uses four ES5510s with individually selectable host interfaces. |
| U46 | Sheet 2 labels U46 `74HC138`; its A/B/C inputs connect to CPU A9/A10/A11, and outputs Y4–Y7 are named ESP1–ESP4. | The low three decoder inputs distinguish the four ESP outputs. The other decoder enables determine the overall address region; that full qualification still needs tracing before giving absolute ESP CPU addresses. |
| U59 | Sheet 2 labels U59 `6850`; its RS, CS0–CS2, R/W, E, RXDATA, and TXDATA pins connect to the CPU/peripheral circuitry. | This is the ACIA, distinct from the four ESPs. |

The service manual calls U54 the VCODE/UCODE ROM and U56 the O/S ROM (printed pp. 13 and 19). Its board-layout picture is not the source for the bus claims above; those come from the base main-board schematic.

## Firmware-correlated U54 bank and tag access

The ROM-private verifier `tools/dp4_ghidra/verify_u54_tag.py` checks a revision-tag comparison found by Ghidra in both U56 images against U54 bank 9. It reports metadata only and never writes or emits firmware bytes:

| Revision | U56 tag-check CPU address / file offset | Expected tag | U54 bank 9 file offset | Match |
|---|---|---:|---:|---|
| v1.06 | `$8572` / `0x0572` | `0x0106` | `0x12000` | yes |
| v1.15 | `$8575` / `0x0575` | `0x010F` | `0x12000` | yes |

In each revision, the startup chain calls the bank-selection helper before the tag read. The helper writes bank value 9 to `$0600`; the schematic connects U57 outputs B0–B3 to U54 address inputs A13–A16. The 6803 then reads `$2000`, where CPU A0–A12 are zero, and compares the returned word to its version-specific expected tag. This is strong, cross-revision evidence for the CPU `$2000` access to bank 9 and a firmware-correlated U54 window. It validates a CPU-to-U54 tag read only; it does not identify an ES5510 instruction write or program boundary.

## U56 firmware accesses `$1800/$1801`

The repeated U56 access pair is now attributable to the **U59 6850 ACIA**, not an ES5510 loader:

- U56 v1.15 CPU address `$B385` (file offset `$3385`) writes A to `$1801`; `$B38A` (offset `$338A`) reads `$1800`, masks bit 1, and waits until it is set.
- U56 v1.06 does the same at `$B36B` (offset `$336B`) and `$B370` (offset `$3370`).
- The 6850's status/data register split and transmit-data-register-empty status bit match this write/poll sequence. The board drawing identifies U59 as a 6850 and routes its serial data pins. This is strong schematic-plus-instruction evidence for an ACIA transmit transaction; it is not an ESP access and does not establish a U54-to-ESP transfer.
- The adjacent v1.15 routine at `$B373` (file `$3373`) sends a selector-like byte, a second byte, and eight payload bytes through that ACIA writer. The caller at `$B35C` (file `$335C`) supplies IDs 13 and 14 and U56-resident payloads. Do not classify it as the microprogram loader.

This corrects the earlier uncertainty about `$1800/$1801`: the board schematic provides a specific non-DSP peripheral match. It does not resolve the unrelated indirect `JSR 0,X` or prove where DSP instruction data is uploaded.

The A9–A11 input pattern at `$1800` would select U46 Y4, which is wired to ESP1, but the decoder's additional enables are not active there: the schematic separately maps `$1800/$1801` to U59's 6850 ACIA, and its status/data access pattern matches the firmware. Therefore these accesses are not evidence that ESP1 is selected.

## ESP host-interface capture signals

The Ensoniq ES5510 specification Rev. 2.4 describes an 8-bit multiplexed host address/data bus. The falling edge of AS latches the host register address; R/W distinguishes read from write; active-low CS initiates the transfer. HALT allows host access to internal ESP registers and controls program execution (specification printed pp. 39–40, 43–44). Those functions match the base schematic's separate ESP select and HALT lines.

For a **single selected DSP**, the minimum synchronous capture is:

1. U52 multiplexed A/D0–A/D7 / ES5510 HDB0–HDB7.
2. AS, R/W, and CPU E.
3. The selected ESPn active-low chip-select (`ESP1`, `ESP2`, `ESP3`, or `ESP4`).
4. Its corresponding HALTn line.

For an all-channel cold-boot capture, retain all four ESP selects and HALT lines, plus U52 A8–A15, the U54 A0–A12 and CE/OE lines, and U57 B0–B3. This captures ROM-bank changes and allows the host address/data phase to be aligned with the selected DSP. Use the actual base-board nets and package pin numbers from drawing 4010015001 Rev J; do not substitute DP/4+ signal labels. Do not connect a probe until the physical board revision and package pinout are checked.

The ES5510 specification's host sequence provides the decoding key after capture: AS low-to-high phase transition latches the internal address; the following data phase and R/W level identify the register access. A verified instruction load would additionally need the six instruction-byte writes, instruction selector/commit, GPR initialization, any table/delay-RAM setup, and halt/run behavior. No such transaction has yet been captured.

## Remaining questions and next analysis

1. Trace U54 CE and the high-address enables around U46/U37 to derive the exact banked U54 CPU window and ESP absolute address ranges.
2. Continue from the now-verified U54 tag path in U56 v1.06 and v1.15. Follow the service-manual UCODE-tag, instruction, GPR, and table verification paths, resolving runtime pointer values and bank writes. Record an ESP access only when it resolves to a schematic-confirmed selected ES5510 bus cycle.
3. Compare candidate U54 reads and U57 bank writes against the diagnostic paths. A U54 read alone is not proof of an ES5510 upload.
4. If static evidence still cannot identify a DSP register transfer, capture the signals listed above during cold boot and selection of one simple effect.

## Tool status and reproducibility

- **REA MCP in this session:** no REA tools were exposed in the Codex tool registry. `CODEX_CLI_PATH` points to the Windows Codex executable, so the active configuration is `/mnt/c/Users/Gamer/.codex/config.toml`; it registers Node `/home/gamer/.nvm/versions/node/v24.22.2/bin/node` and REA `/home/gamer/.npm/_npx/9b9809298fe40d2c/node_modules/rea-agents/scripts/rea.mjs` by absolute WSL paths (package root `/home/gamer/.npm/_npx/9b9809298fe40d2c/node_modules/rea-agents`). The separate `/home/gamer/.codex/config.toml` is not the active config and still has an `npx` registration. A direct stdio MCP initialize plus `tools/list` using the active config's command returned REA 6.9.0 and all 255 tools. `rea mcp doctor --json` also passed its 255-tool/6-prompt inventory and request-flow checks. Codex still needs a restart/reload to expose REA in this conversation.
- **REA tools actually used:** `capabilities`, `guided-workflows` (including the rendered `investigate_feature` workflow), `guidance`, `mcp doctor`, direct MCP `tools/list`, `list_documents`, and `open_binary`. The direct tool inventory includes `procedure_assembly`, `procedure_callers`, `procedure_references`, `xrefs`, `analyze_function`, `resolve_native_call_targets`, `read_bytes`, and `search_bytes`. `list_documents` returned `target_unavailable: No app is open`; `open_binary` on the private U56 image returned `unsupported binary format`. REA's `open_binary` raw-binary architecture enum lacks MC6801, so no REA firmware-analysis result is claimed. Ghidra headless was used directly.
- **Ghidra:** the existing private Ghidra 12.1.4 install and MC6801 extension remain at `/tmp/dp4-rom-forensics/tooling/ghidra-install/ghidra_12.1.4_PUBLIC`. Fresh headless v1.06/v1.15 analyses at the confirmed `$8000` base report 1,323 and 1,326 decoded instructions and retain the `$1800/$1801` cross-references. Reports/projects are under `/tmp/dp4-rom-forensics/ghidra-projects/schematic-confirmed/`; the ROM files and Ghidra projects are not in Git.
- **ROM privacy:** no ROM image, extracted firmware, schematic scan, or private Ghidra project is added to this change.
- **Synthetic tests:** `python3 -m unittest experimental.emulation.capture.tests.test_dp4_target_boundaries -v` checks U56 file offsets, U54 four-bit bank arithmetic, U46 raw Y-output selection, and instruction-boundary validation using generated fixtures only.

## Sources

- Ensoniq, **DP-4 PCB Schematic, Main Board**, drawing 4010015001 Rev J, sheet 2 (digital section), available through the [R-Massive Ensoniq technical-document archive](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) at its DP4 schematic entry. The scanned PDF reviewed is `Ensoniq DP4 Schematics.pdf`; its direct archive URL is `https://audiopro.r-massive.com/Ensoniq/Schematics/Ensoniq%20DP4%20Schematics.pdf`.
- Ensoniq, **DP/4 Service Manual**, printed pp. 6, 13, 19: [PDF](https://www.synfo.nl/servicemanuals/Ensoniq/ENSONIQ_DP-4_SERVICE_MANUAL.pdf).
- Ensoniq, **ESP ES5510 Specification Rev. 2.4**, printed pp. 39–44 (HALT, host bus, AS, CS, pinout): [PDF](https://gjcp.net/pdf/es5510.pdf), linked from the [R-Massive technical-documents archive](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/).
- MAME, ES5510 host-interface model at commit `8be57cdc5f44ab830dff44c06dd39f24190742d4`: [source](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp). Device-local semantics are a reference for interpreting a future capture, not evidence of a DP/4 transfer.
