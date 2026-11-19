# DP/4 hardware architecture audit

Research branch: `issue11-dp4-rom-forensics`
Baseline: `c06ac8992f1e4ff3d20dab0f51f901e5637b543f`

## Current verified position

The base DP/4 digital-board schematic has now been located and checked. See the detailed, schematic-referenced [base-board bus map](DP4_BASE_BOARD_BUS_MAP.md), which supersedes the earlier no-schematic assessment in this file's Git history.

| Question | Current finding | Status |
|---|---|---|
| Host processor | U52 is a 6803-family CPU; sheet 2 of the base drawing shows its address/data/control buses. | Confirmed by the base schematic and service manual p. 6. |
| U56 map | 32 KiB 27256, A0–A14 on the CPU bus, CE qualified by A15; maps at `$8000–$FFFF`. `$FFFE` reset-vector address maps to file offset `$7FFE`. | Confirmed by schematic topology and reset-vector correspondence. |
| U54 map | 128 KiB 27C010; CPU A0–A12 and data bus connect directly; U57 B0–B3 feed U54 A13–A16. | Banked access from U52 confirmed; exact CPU window/chip-select equation unresolved. |
| DSP population | U61, U62, U83, U84 are four ES5510s. U46 generates named ESP1–ESP4 selects from A9–A11; the full upper-address enable remains to be traced. | Confirmed components and wiring; absolute ESP address ranges unresolved. |
| `$1800/$1801` U56 accesses | Match U59 6850 ACIA status/data access, including transmit-ready polling. | Schematic plus firmware evidence; not an ES5510 transaction. |
| Factory U54-to-ESP upload | No six-byte instruction write/commit, GPR setup, or actual host-interface transaction has been verified. | Unresolved. |

## DSP diagnostics

The service manual's printed pp. 6–7 list ESP A–D error groups for UCODE tags, halted/ready state, GPR verification, instruction verification, table verification, and download verification. These labels are useful cross-reference anchors, but error constants alone do not identify their generating routine. U56 is the likely firmware owner because it is the O/S ROM, but the individual checks have not yet been tied to confirmed ESP host cycles.

The next static pass should use the confirmed `$8000` U56 mapping and the base-board decoder/control paths, then trace backward from the manual's GPR/instruction/table verification behaviors. A U54 read or the old `$1800/$1801` ACIA routine is not sufficient evidence of an ES5510 upload.

## Emulation references and limits

- [MAME ES5510 host implementation](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp) is a useful model for device-local GPR, instruction, RAM, and host-control semantics. Its selectors are not absolute DP/4 CPU addresses.
- [OpenASR](https://github.com/peroberg/OpenASR) models an ASR-10 with an MC68302 host. Its HALT/upload/run concepts are useful comparison material; ASR-10 wiring and host addresses do not transfer to the DP/4.
- DEEP/4 is a clean-room DP/4-inspired emulator and may support output comparisons, but it is not a source of factory firmware or a measured upload transaction.
- DP/4+ Rev C drawings remain separate hardware. They are not used to infer base-board chip selects or net names.

## REA and analysis environment

REA 6.9.0 is registered by absolute cached paths in `/mnt/c/Users/Gamer/.codex/config.toml`. Its direct MCP initialize/tool-list check returned 255 tools, and `mcp doctor --json` passed its 255-tool/6-prompt inventory. REA was not exposed in the Codex tool registry; REA `open_binary` rejected U56 as an unsupported format, so no REA firmware/Ghidra analysis ran. A Codex restart/reload is required. The existing private Ghidra 12.1.4 / MC6801-extension installation and its v1.06/v1.15 headless reports are under `/tmp/dp4-rom-forensics`. No ROMs, extracted firmware, schematics, or Ghidra project files are tracked.

## Sources

- Ensoniq, *DP-4 PCB Schematic, Main Board*, drawing 4010015001 Rev J, sheet 2; available via the [R-Massive Ensoniq archive](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/).
- Ensoniq, *DP/4 Service Manual*, printed pp. 6–7, 13, 19: [PDF](https://www.synfo.nl/servicemanuals/Ensoniq/ENSONIQ_DP-4_SERVICE_MANUAL.pdf).
- Ensoniq, *ESP ES5510 Specification Rev. 2.4*, printed pp. 39–44: [PDF](https://gjcp.net/pdf/es5510.pdf), linked from the [R-Massive archive](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/).
