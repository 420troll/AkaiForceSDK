# DP/4 U56 loader investigation

Branch: `issue11-dp4-rom-forensics`  
Original analysis commit: `e3cc99bf77bb360766d4eba2eeb5af33fff43f3f`
Hardware-map update baseline: `c06ac8992f1e4ff3d20dab0f51f901e5637b543f`
Original analysis: `2026-11-18`; schematic review update: `2026-11-19`
ROMs: private DP4 U56 v1.06 and v1.15 images; hashes are in [DP4_ROM_MICROCODE_FORENSICS.md](DP4_ROM_MICROCODE_FORENSICS.md). No ROM, extracted binary, or full disassembly is included here.

## Result

**No ES5510 factory-program loader routine is identified.** A stable, byte-for-byte-identical U56 routine pair was found that transmits short packets through a memory-mapped status/data pair. Its reset-path caller sources two 8-byte blocks from U56 ROM itself. The base DP/4 schematic now identifies U59 as a 6850 ACIA: the `$1801` write and `$1800` status poll match its transmit data/status registers. This is verified serial peripheral I/O, not an ES5510 register transaction. There remains no evidence that this routine reads U54 or writes ES5510 instruction/GPR memory.

The DP/4 service manual identifies the host CPU as a 6803 at U52. The U56 instruction-family decode is consistent with this 6801-family CPU. The base board schematic resolves the U56 window and shows U54 banked directly on the U52 bus. See [base-board bus map](DP4_BASE_BOARD_BUS_MAP.md). The exact U54 CPU window and DSP host address decode remain unresolved.

## Analysis environment and tool evidence

| Check | Result |
|---|---|
| Correct project | `/tmp/trollfx-issue4`, branch `issue11-dp4-rom-forensics`, starting commit `e3cc99bf77bb360766d4eba2eeb5af33fff43f3f`; linked worktree of `/mnt/c/Users/Gamer/TROLLfx-Force-SDK/.git`, remote `https://github.com/420troll/AkaiForceSDK.git`. |
| Private ROMs | Recovered under `/tmp/dp4-rom-forensics/extracted/{v106,v115}`; all four U54/U56 SHA-256 values match `DP4_ROM_MICROCODE_FORENSICS.md`. No payload or full reports are in this repository. |
| REA package / MCP self-check | Cached `rea-agents` 6.9.0 at `/home/gamer/.npm/_npx/9b9809298fe40d2c/node_modules/rea-agents`; current direct `mcp doctor --json` passed with 255 tools and 6 prompts. |
| REA MCP in this session | REA tools were absent from the Codex tool registry. A direct stdio MCP handshake/tool-list returned 255 tools, while `list_documents` said no app was open and `open_binary` rejected this MC6801 image as an unsupported format. Local `capabilities`, `guided-workflows`, `guidance`, and `mcp doctor` commands ran; no REA Ghidra analysis result is claimed. Restart/reload Codex to expose the registered MCP server. |
| Ghidra | Installed privately at `/tmp/dp4-rom-forensics/tooling/ghidra-install/ghidra_12.1.4_PUBLIC`, with Temurin JDK 21 and isolated `GhidraMC6800` extension v1.2. Headless analyzer and REA doctor passed. Extension supplies `MC6801:BE:16:default` and `HD6301:BE:16:default`; no system or repository environment was overwritten. |
| Ghidra analysis | Ghidra headless analyzed both revisions in MC6801 mode and v1.15 in MC6800 and HD6301 modes. Private reports and projects are under `/tmp/dp4-rom-forensics`; fresh v1.06/v1.15 MC6801 imports use the schematic-confirmed base `0x8000`, reset vector `0xFFFE`. |

The checked-in helper `tools/dp4_ghidra/run_u56.sh` imports a caller-supplied U56 image into a new Ghidra project under a caller-owned private work directory and writes a report there. It never copies an image into the repository. `experimental/emulation/capture/tests/test_dp4_ghidra_smoke.py` is an opt-in integration test built from synthetic bytes.

The base DP/4 schematic confirms the U56 27256 window at CPU `0x8000..0xFFFF`: U56 A0–A14 connect to the bus and its active-low CE is qualified by A15. The reset vector at CPU `0xFFFE` is image offset `0x7FFE`, value `0x84E4`, so reset entry is file offset `0x04E4`.

The U56 work below uses Ghidra's MC6801 disassembly, cross-references, computed-flow classification and basic blocks, compared across v1.06/v1.15. The decoded language agrees with the schematic and service-manual 6803-at-U52 identification. See [DP/4 hardware architecture audit](DP4_HARDWARE_ARCHITECTURE_AUDIT.md) and the [base-board bus map](DP4_BASE_BOARD_BUS_MAP.md).

## Reset and boot path evidence

With the schematic-confirmed U56 map, Capstone reaches the following startup path in both revisions:

- Reset at logical `0x84E4`, file `0x04E4`.
- Initialization then clears memory and calls common setup helpers.
- v1.06 logical `0x8608` (file `0x0608`) calls `0xB342` (file `0x3342`); v1.15 logical `0x860B` (file `0x060B`) calls `0xB35C` (file `0x335C`).
- After the call, each revision enters setup code at logical `0x803E` (file `0x003E`).

The caller's addresses and contents shifted across revisions, but the call's high-level action remains a short initialization exchange. It is not a scan or transfer of the 128 KiB U54 image.

## Stable peripheral-write candidate

These CPU-address annotations use the schematic-confirmed U56 base `0x8000`; offsets in parentheses are image offsets.

### v1.15

- `0xB385` (file `0x3385`), 11-byte routine: write accumulator A to `$1801`; load mask `2`; poll `$1800` until bit 1 is set; return.
- `0xB373` (file `0x3373`), 18-byte routine: send byte `0x93`; send B; then send 8 bytes from `0,X`, incrementing X for each byte; return.
- `0xB35C` (file `0x335C`) calls the 8-byte sender twice, with B values 13 and 14, using pointers `0xCE97` and `0xCE9F`. Under the hypothesized map, the two arrays are U56 offsets `0x4E97` and `0x4E9F`. It then sends `0x81` and `0x83` through the same byte writer.
- The arrays are exactly 8 bytes apart and are present in U56. Their 8-byte sequences have zero exact occurrences in the corresponding U54 v1.15 image.
- Nearby setup routines at logical `0x8689` and `0x8699` also write/read `$1800` and `$1801`.

### v1.06 comparison

The corresponding routine addresses are `0xB36B` (file `0x336B`) for the byte writer, `0xB359` (file `0x3359`) for the 8-byte sender, and `0xB342` (file `0x3342`) for the caller. The caller uses pointers `0xCE7B` and `0xCE83`, corresponding to U56 offsets `0x4E7B` and `0x4E83` under the schematic-confirmed map. Those U56 8-byte blocks also have zero exact occurrences in U54 v1.06.

The 11-byte byte-writer body and 18-byte sender body are byte-for-byte identical between the two OS revisions. Their SHA-256 values, calculated privately from those short code spans, are respectively:

- byte writer: `f399567e576f27dcba7763b26e49f54a8511ada6dfa4b2564d8e188c6be28336`
- 8-byte sender: `a7fcde13a31a63f8f992caab0bd371dd00120ac833d83372b12115590c0cf5b3`

These code hashes establish byte stability. The newly located base DP/4 main-board schematic identifies U59 as an MC6850-compatible ACIA; `$1801` is its data register and `$1800` its status/control register. Polling status bit 1 after the data write matches transmit-data-register-empty behavior. Thus `$1800/$1801` is not an ES5510 interface. The routine likely sends serial data; the exact message purpose is outside this conclusion.

## Why this is not the MAME ES5510 instruction loader

At pinned MAME commit `8be57cdc5f44ab830dff44c06dd39f24190742d4`, `es5510_device::host_w` models a host register interface: six byte writes at offsets `0x03..0x08` assemble a 48-bit instruction latch, selector `0xC0` commits a word to an instruction index, GPR bytes use offsets `0x00..0x02`, and selectors `0xA0`/`0xE0` write register state. The U56 candidate instead issues a two-block, eight-byte-per-block transaction through `$1800/$1801`; its direct access pattern does not resemble the MAME handler's individual register offsets.

A separate application-level relationship to DSP setup is not impossible, but the observed hardware transaction itself is ACIA I/O, not an ES5510 host-register access. Supporting evidence would be needed to connect these packets to DSP loading. In particular:

1. MAME has no complete DP/4 machine driver to reveal such a translation.
2. The base schematic maps U59 as the 6850 ACIA, and the firmware access pattern matches its status/data registers.
3. The packets read bytes from U56, not from the known U54 image region.
4. Neither the U54 source address nor any 48-bit instruction index/GPR target is observed.

Therefore, there is no supported basis to identify this as the factory microprogram loader.

## Main path barrier

In v1.15 startup/main-loop code, an object walk reaches an indirect call at logical `0x85EA` (file `0x05EA`): the code obtains a pointer through fields in an object and executes `JSR 0,X`. The routine targets are runtime values. Resolving them requires:

- initialized object/descriptor contents, including any data copied from U54;
- the exact U54 window/CE decode and bank-latch write path;
- observed writes on the schematic-confirmed ES5510 host interface or an emulator driver that models the same path.

Ghidra classifies the instruction as a computed call but records no destination edge. In v1.06 the matching site is `0x85E7`; in v1.15 it is `0x85EA`. Around it, the code follows object fields (`LDX 7,X`, then `LDX 0,X`) before `JSR 0,X`; the pointer value is runtime data. Ghidra reports 55 functions and 1,323 instructions in v1.06, and 55 functions / 1,326 instructions in v1.15 under the schematic-confirmed map. The call remains unresolved; static CFG does not recover the descriptor contents or U54 bank-latch state.

Language comparison on v1.15 yields only 2 functions / 59 instructions in base MC6800 mode, versus 55 / 1,326 in MC6801 and 55 / 1,326 in HD6301 mode. Reachable code includes 6801-family instructions such as `ABX` and `MUL` (including `MUL` in the function at `0x8DE9`). This supports an MC6801/HD6301-compatible ISA model over base MC6800 and agrees with the manual's identification of a 6803 at U52. Exact CPU suffix is unverified; the U56 physical ROM decode is now confirmed by the base schematic.

## Finding and confidence

| Finding | Confidence |
|---|---|
| U56 v1.06 and v1.15 have a stable 18-byte sender and 11-byte ACIA status/data writer at the offsets listed above | **High** for bytes, decoding, and U56 address mapping |
| U52 host CPU is a Motorola 6803-family part | **Confirmed by the base DP/4 service manual** identifying “6803 (U52)” on printed p. 6; exact suffix/package marking not established |
| U56 code decodes in the 6801-family instruction set | **Strongly supported** by reachable `ABX`/`MUL` and Ghidra language comparison; consistent with 6803 |
| U56 ROM CPU window/reset-vector mapping | **Confirmed** by U56 A0–A14 wiring and A15-qualified CE; `$8000–$FFFF`, vector offset `$7FFE` |
| `JSR 0,X` target at v1.06 `0x85E7` / v1.15 `0x85EA` | **Unresolved**; computed call has no static target edge |
| The reset path calls a routine that sends two 8-byte U56-resident payloads with IDs 13 and 14, then sends two short values | **High** for the static call path under the schematic-confirmed U56 map |
| `$1800/$1801` are U59 6850 status/control and data registers | **Strong**: schematic component plus matching firmware poll/write pattern |
| `$1800/$1801` accesses are an ES5510 transaction | **Refuted by the base schematic mapping to U59 ACIA** |
| This routine loads ES5510 microinstructions | **Unsupported** |
| A U54-to-ES5510 byte transformation or instruction stream has been identified | **No** |

## Shortest next observation

The schematic now gives a practical probe plan; see [base-board bus map](DP4_BASE_BOARD_BUS_MAP.md). For one ESP, capture U52 A/D0–A/D7, AS, R/W, E, the selected ESPn active-low CS, and HALTn. For a four-DSP boot trace, capture ESP1–ESP4 and HALT1–HALT4 plus U52 A8–A15, U54 A0–A12 and CE/OE, and U57 B0–B3. `$1800/$1801` is the ACIA and must not be used as the ESP trigger. No physical capture has yet been made.

## Reproduction notes

Private analysis used locally extracted ROMs under `/tmp/dp4-rom-forensics`. Ghidra scripts are published under `tools/dp4_ghidra/`; use the runner documented there with a private work directory. The report files and projects stay outside the repository. No code or ROM payload from the private directory is included here. The exact Capstone spot-check remains reproducible as:

```sh
PYTHONPATH=/tmp/dp4-rom-forensics/capstone python3 - <<'PY'
from capstone import *
from pathlib import Path
rom = Path("/tmp/dp4-rom-forensics/extracted/v115/DP4_V115_os.bin").read_bytes()
cpu = Cs(CS_ARCH_M680X, CS_MODE_M680X_6801)
for insn in cpu.disasm(rom[0x3385:0x3385 + 11], 0xB385):
    print(f"{insn.address:04x}: {insn.mnemonic} {insn.op_str}")
PY
```

This is a linear disassembly spot-check, not a validated firmware emulation. Ghidra's computed-call report likewise constrains but does not resolve the dynamic target. A synthetic Ghidra smoke test exercises import, reset-vector marking, analysis, and report generation without proprietary firmware.

## Sources

- [DP/4 Service Manual](https://www.synfo.nl/servicemanuals/Ensoniq/ENSONIQ_DP-4_SERVICE_MANUAL.pdf) — identifies “6803 (U52)” on printed p. 6 and U56 as OS ROM / U54 as UCODE on pp. 13 and 19. The bus map comes from drawing 4010015001 Rev J; see [base-board bus map](DP4_BASE_BOARD_BUS_MAP.md).
- [DP/4+ Rev C digital-board schematic](https://convexoptimization.com/EnsoniqDP4/Ensoniq%20DP4%2B%2C%20Digital%20Board%2C%20401024201%20Rev%20C.pdf) — comparison only; identifies 63803, separate OS/UCODE ROMs, 63B50 serial/I/O and ESP sections, but not the base DP/4 map.
- [MAME ES5510 host implementation](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp) — generic device behavior at pinned BSD-3-Clause source commit, not a DP/4 host-driver trace.
