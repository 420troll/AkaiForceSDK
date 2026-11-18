# DP/4 hardware architecture audit

Research branch: `issue11-dp4-rom-forensics`

Baseline: `9f0f6b98318915871db9fce2dacbfbe8bc73d86d`

Status: evidence audit; base DP/4 digital-board schematic not located.

## Findings at a glance

| Question | Finding | Evidence status |
|---|---|---|
| Main host processor | The DP/4 service manual identifies a **6803 at U52**. This is the best available identification of the base unit's host CPU. The exact manufacturer suffix/package revision remains unverified. | Confirmed by Ensoniq service procedure, printed page 6. |
| U56 executor | The reset path in the private U56 images decodes as 6801-family code, consistent with the manual's 6803 identification. The physical ROM-to-CPU wiring and full address map are not shown in the service manual. | Strongly supported, not independently verified from a base-board schematic or trace. |
| U54 role | U54 is named the UCODE/VCODE ROM; U56 is the O/S ROM. | Confirmed by the service manual and ROM replacement instructions. |
| U54 bus mapping | Direct map, banked map, and mediated access remain possible. No base DP/4 schematic or capture establishes the address decode, chip select, banking, or which bus master reads it. | Unknown. |
| DSP population | Service diagnostics enumerate ESP A, B, C, and D and report per-ESP failures. | Four logical ESP channels confirmed; base-model chip part markings/wiring are not in the schematic material found. |
| ES5510 host interface | The generic ES5510 interface and a DP/4+ schematic provide signal-level comparison points. They do not establish base DP/4 register addresses or wiring. | Chip behavior supported by MAME; DP/4+ wiring confirmed only for DP/4+. |

## Processor identity and the 68030 discrepancy

The Ensoniq DP/4 Service Manual's “No Display or Self-Test” procedure explicitly says to jumper **pin 6 of the 6803 (U52)** to ground while troubleshooting (printed page 6). It is direct model-specific evidence for a Motorola 6803-family device at U52. The same manual describes U56 as the O/S ROM and says the ROM checksum display labels OS as U56 and UCODE as U54 (printed pages 13 and 19).

The prior reading of a board/layout image as “68030” is not corroborated by searchable text in the service manual. A 68030 interpretation also conflicts with both the manual's explicit “6803 (U52)” identification and the U56 reachable code: under the assumed ROM window, U56 contains 6801-family instructions such as `ABX` and `MUL`, and decodes as 6801/HD6301 in Ghidra. The defensible conclusion is **6803-family host at U52**, not a 68030. If a physical board marking is thought to say 68030, a sharp package photograph and board revision are needed to resolve that discrepancy; no 32-bit CPU analysis should be based on the earlier layout reading.

The 6803 belongs to the 6801 instruction family, so the U56 disassembly is compatible with the manual's CPU identification. This corrects the earlier statement that the host processor part was wholly unknown. It does not, by itself, prove the U56 EPROM's chip-enable wiring, reset-vector placement, complete memory map, or any U54 access path.

### ROM offsets versus CPU addresses

The two private U56 images are 32 KiB. Their final two bytes are `84 E4`, giving reset-vector value `$84E4`. Prior analysis imported each image into Ghidra with an **assumed** window `$8000–$FFFF`, so the entry is shown at logical `$84E4`, file offset `$04E4`. This is a useful disassembly hypothesis, and the 6803 identification makes the instruction family plausible, but the base DP/4 service manual does not provide a circuit schematic proving that window. Keep file offsets authoritative until the board decode is traced.

| Region | Established information | Unresolved mapping |
|---|---|---|
| U56 | 32 KiB O/S EPROM, private v1.06 and v1.15 dumps validated by SHA-256 in `DP4_ROM_MICROCODE_FORENSICS.md`; reset vector bytes at file offset `0x7FFE` are `84 E4`. | CPU address window, chip select equation, wait-state/bus timing and any aliasing. |
| U54 | 128 KiB UCODE/VCODE EPROM; private v1.06 and v1.15 dumps validated by SHA-256 in `DP4_ROM_MICROCODE_FORENSICS.md`. | Which controller reads it, address window, bank selection, access sequencing and data transformation. |
| Host RAM | The service manual describes the unit as a computer with 32 KiB RAM (printed page 1). | Physical RAM device(s), address ranges and whether this refers solely to host RAM. |
| ESP A–D | Per-channel diagnostics and four logical ESPs are documented. | Base-model host-interface addresses, chip-select decode, halt/reset source, and shared versus separate buses. |

## What the base DP/4 service material shows

The DP/4 service manual is a diagnostic and module-replacement manual, not a digital-board circuit schematic. Figure 10 (printed page 12) is a system-level block diagram: it shows the main board, transformer secondary board, and keypad/display board, plus the digital display/knob/DSPIN/DSPOUT interconnects. It does not trace U52, U54, U56, or the ESP buses. Figures 11–12 are board-removal/layout illustrations and should not be read as pin-level wiring.

The service manual labels U54 as UCODE/VCODE and U56 as O/S ROM, and its burn-in procedure checks ROM checksums, host DRAM, ESP DRAM A–D, and OS RAM. These operations establish functional roles and the existence of four tested ESP/DRAM paths, but not their address decode or loader protocol.

## DP/4+ schematic: useful comparison, not a substitute

The available Ensoniq DP-4+ Rev C digital-board schematic is explicitly titled **DP-4+**, drawing 4010024201, revision C. Its differences prevent using it as proof for the base DP/4. It shows:

| DP/4+ evidence | Schematic location | What it does and does not establish |
|---|---|---|
| U25 is a 63803; U28 is the OS ROM; U27 is a 27C2001 ROM with `ROMBS0–ROMBS3`; U26 is RAM; buses include address, control, and multiplexed A/D. | Sheet 2 | A concrete DP/4+ controller/ROM/bank architecture. It does **not** identify base DP/4 U52/U54/U56 wiring or prove that U54 uses the DP/4+ `ROMBS` scheme. |
| Four ESP sections use 5510 devices, e.g. U19/U13/U7/U1; each section includes `HA/D0–HA/D7`, `IOSEL`, chip-select, `R/W`, `AS/`, halt/gate, local DRAM control and serial nets. | Sheets 5–8 | Shows how a related product connects four ESPs and their host-facing signals. It does **not** establish base DP/4 net names, chip selects, or address mapping. |

In particular, `ROMBS0–ROMBS3`, `ESPCSx/`, `E1HALT/` etc. are DP/4+ net names. Do not use these as probe labels on the base unit until the base board is traced.

## Host-interface reference model

Pinned MAME ES5510 source (`8be57cdc5f44ab830dff44c06dd39f24190742d4`, BSD-3-Clause) implements the **device-local** host port. It is useful for recognizing a transfer in a trace, not for deriving a DP/4 CPU address:

| ES5510 host offset | MAME operation |
|---|---|
| `0x00–0x02` | GPR latch bytes |
| `0x03–0x08` | Six bytes of instruction latch |
| `0x0F–0x11` | DADR latch; a write to `0x0F` performs selected DRAM read/write using the latch state |
| `0x12` | Host control, including DRAM clear while halted |
| `0x14` | RAM selection (`bit 7` in this implementation) |
| selector values `0x80`, `0xA0`, `0xC0`, `0xE0` | Read select, GPR write, instruction write, and combined GPR/instruction write behavior in MAME's host-select path |

MAME's `host_w`/`host_r` decode offsets on the ES5510's local interface. They are **not** absolute 6803 memory addresses and do not prove that the DP/4 host directly exposes this interface without glue logic. MAME source is a model, not a DP/4 upload trace.

The DP/4+ schematic's `HA/D0–HA/D7`, `IOSEL`, `R/W`, `AS/`, chip select and HALT/GATE nets are reasonable categories of signal to identify on a base board; they are not verified base DP/4 probe names.

## Related emulation work and transfer limits

| Project/source | Reusable evidence or idea | Boundary for this investigation |
|---|---|---|
| MAME ES5510 device at pinned commit `8be57cdc5f44ab830dff44c06dd39f24190742d4` | Device-local latch/register semantics and a target for validating a transaction parser. | MAME's Ensoniq machine drivers cover other products; no DP/4 system driver or DP/4 host trace was found. |
| OpenASR (MAME-based ASR-10 work) | Its public project description identifies a different host, MC68302, and describes controlling ES5510 halt/upload/run lifecycle via MC68302 PA4. Local notes in the OpenASR checkout describe ASR-specific host transactions. | ASR-10 CPU, PA4 wiring, absolute addresses and upload sequence cannot be transferred to DP/4. Use only as an example of the evidence a functioning host model can expose. |
| Temecula DSP DEEP/4 | Public product material documents a DP/4-inspired emulator and hardware comparisons; the developer describes the implementation as clean-room/native code without original firmware. It may provide black-box output comparisons for algorithm behavior. | It does not provide an authentic U54-to-ES5510 transaction trace or factory microprogram for this audit. Do not infer transfer routines or firmware bytes from algorithm-level agreement. |

For a verified upload path, the useful MAME-derived abstractions are the instruction latch, program selector/commit, GPR latch, DADR/DRAM access, host-control DRAM clear, and halt/run transition. The DP/4+ schematic demonstrates related signal categories, but only a base DP/4 trace can join those device operations to U52, U54, or actual host addresses.

## Diagnostic messages as reverse-engineering anchors

The service manual calls unexpected-event messages software-development diagnostics, not hardware diagnostics (printed page 7). It lists main-board errors including event `13` “incompatible version of UCODE EPROM” and per-ESP errors:

| Event family | A/B/C/D codes | Manual wording | Implied operation to trace (hypothesis) |
|---|---:|---|---|
| `3x` | 30–33 | bad ucode tag | U54 tag/record validation |
| `3Y` | 35–38 | ESP not halted | Halt-state test before host access |
| `4x` | 40–43 | ESP never ready | Ready/status polling |
| `4Y` | 45–48 | ESP sync error | ESP synchronization check |
| `5x` | 50–53 | GPR verify error | GPR write/readback comparison |
| `6x` | 60–63 | Inst verify failure | Instruction write/readback comparison |
| `7x` | 70–73 | both verify failure | Combined GPR and instruction verification |
| `8x` | 80–83 | table verify failure | Table transfer/readback check |
| `8Y` | 85–88 | DRAM write error | ESP DRAM write test |
| `9x` | 90–93 | download verify failure | A broader transfer validation |

The ESP A–D number suffixes and message taxonomy are confirmed. They make U56's error-generation paths a better static-analysis anchor than generic control-flow scanning. The current Ghidra script's scan for immediate constants is only a candidate finder: ubiquitous constants such as `0x30`, `0x40`, and `0x50` do **not** identify event-generation code. No individual diagnostic caller, status register or ESP host access sequence has yet been verified in U56. Because the manual describes the codes as software-engineering diagnostics and the ROM is the O/S image, U56 is the first firmware to inspect, but assigning a specific diagnostic implementation to a CPU image needs a cross-reference or runtime trace.

## Capture plan based on verified facts and gaps

The next decisive evidence should come from a base DP/4 board schematic or a read-only synchronized capture. Do not probe the Force. Do not infer base-board pin names from the DP/4+ drawing.

1. Record the exact base main-board revision and clear markings for U52, U54, U56, and all four ESP packages. Confirm U52 pinout against the actual 6803-family datasheet before attaching probes.
2. Obtain/trace the base schematic far enough to identify U52 address/data, E/clock, R/W and reset; U54 address/data, chip select and output enable; and each ESP host-side data/address/select/read-write/halt or reset signals. Record actual net names and package pins from the board.
3. Use a shared-timebase logic analyzer for the 6803 bus, both ROM buses/selects, and the four ESP host interfaces. Trigger at reset release and retain the full U54 address/data/select activity and each ESP HALT transition.
4. Capture a cold boot, then a controlled selection of one simple effect. Preserve the original capture privately. Decode candidate ES5510 writes using the local host-port semantics only after confirming how base-board select/address signals map onto ES5510-local offsets.
5. A factory instruction sequence is verified only when six byte writes, an instruction address/selector commit, and the selected ESP are reconstructed consistently; correlate accompanying GPR, DADR/DRAM/table writes and HALT/run transitions. Cross-check reads if the bus allows it.

Until the base schematic or trace exists, a smaller “minimum probe list” with named DP/4 nets or CPU addresses would be invented. The required blocker is therefore precise: **base DP/4 CPU↔ROM and CPU↔ESP decoding is undocumented in the service manual material located so far.**

## Tool and artifact status

- The Windows Codex config at `/mnt/c/Users/Gamer/.codex/config.toml` contains the REA MCP registration through `C:\\Windows\\System32\\wsl.exe`, using cached Node `/home/gamer/.nvm/versions/node/v24.22.2/bin/node` and `/home/gamer/.npm/_npx/9b9809298fe40d2c/node_modules/rea-agents/scripts/rea.mjs`. The WSL config `/home/gamer/.codex/config.toml` still contains a separate `npx`-based registration. REA does not appear in this session's exposed Codex tool registry, so the client has not made its tools available here; restart Codex to reload the configured server. The active app configuration cannot be conclusively selected from the shell alone.
- Running the cached REA 6.9.0 `mcp doctor --json` directly passed initialization, request flow, and exact inventory checks: 255 tools and 6 prompts, no missing/unexpected/duplicate entries. This verifies the adapter process, not the active Codex client's connection. No REA firmware-analysis tool was available to invoke in this session.
- Existing private Ghidra 12.1.4 / MC6801 analysis and reports remain outside Git. The private U54/U56 images were rehashed against `DP4_ROM_MICROCODE_FORENSICS.md`; all four SHA-256 values matched.
- Firmware and reports remain under `/tmp/dp4-rom-forensics` and are not included here.
- The working tree already contained uncommitted indirect-call utilities and Ghidra report changes before this audit. They are preserved; this audit did not extend indirect-call analysis.

## Sources

- Ensoniq, *DP/4 Service Manual*, especially printed pp. 6–7 (6803 U52 and ESP diagnostic event families), pp. 12–13 (system diagram and burn-in checks), and p. 19 (U54/U56 identification): [service manual PDF](https://www.vintagesynthparts.com/wp-content/uploads/2016/08/Ensoniq-DP4-Service-Manual.pdf).
- Motorola, *MC6801/MC6803 Microcontroller/Microprocessor Technical Data*: [data book PDF](https://bitsavers.trailing-edge.com/components/motorola/_dataBooks/1988_DL139_Motorola_Microcontroller_and_Microprocessor_Families_Volume_1.pdf). It describes the MC6803 as an MC6801 operating in modes 2 or 3, supporting the instruction-family comparison.
- Ensoniq, *DP-4+ Digital Board*, drawing 4010024201, Rev C, sheets 2 and 5–8; comparison only: [schematic PDF](https://convexoptimization.com/EnsoniqDP4/Ensoniq%20DP4%2B%2C%20Digital%20Board%2C%20401024201%20Rev%20C.pdf).
- MAME, ES5510 host-interface implementation at pinned commit: [source file](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp).
- OpenASR project description, for ASR-10-specific MCU/ES5510 lifecycle comparison: [GitHub repository](https://github.com/peroberg/OpenASR).
- Temecula DSP, DEEP/4 product information: [product page](https://www.temeculadsp.com/deep4); clean-room/no-original-firmware statement by its developer: [public discussion](https://gearspace.com/threads/temecula-dsp-releases-deep-4-ensoniq-dp-4-emulation.1461746/page-20).
- Existing local source checkout for MAME-derived OpenASR is a different Ensoniq product (ASR-10) and should be used only as a comparison, as already documented in [DP4 microprogram extraction plan](DP4_MICROPROGRAM_EXTRACTION_PLAN.md).
