# DP/4 U56 loader investigation

Branch: `issue11-dp4-rom-forensics`  
Starting commit: `e3cc99bf77bb360766d4eba2eeb5af33fff43f3f`
Analysis date: `2026-11-18`
ROMs: private DP4 U56 v1.06 and v1.15 images; hashes are in [DP4_ROM_MICROCODE_FORENSICS.md](DP4_ROM_MICROCODE_FORENSICS.md). No ROM, extracted binary, or full disassembly is included here.

## Result

**No ES5510 factory-program loader routine is identified.** A stable, byte-for-byte-identical U56 routine pair was found that transmits short packets through a memory-mapped status/data pair. Its reset-path caller sources two 8-byte blocks from U56 ROM itself. This is verified firmware I/O, but there is no evidence that it reads U54 or writes ES5510 instruction/GPR memory. It is a candidate peripheral initializer, not a decoded DSP loader.

The DP/4 service manual identifies the host CPU as a 6803 at U52. The U56 instruction-family decode is consistent with this 6801-family CPU. The CPU address map and U54 bus mapping are still unverified, so this corrects the processor-family uncertainty without resolving the DSP loader.

## Analysis environment and tool evidence

| Check | Result |
|---|---|
| Correct project | `/tmp/trollfx-issue4`, branch `issue11-dp4-rom-forensics`, starting commit `e3cc99bf77bb360766d4eba2eeb5af33fff43f3f`; linked worktree of `/mnt/c/Users/Gamer/TROLLfx-Force-SDK/.git`, remote `https://github.com/420troll/AkaiForceSDK.git`. |
| Private ROMs | Recovered under `/tmp/dp4-rom-forensics/extracted/{v106,v115}`; all four U54/U56 SHA-256 values match `DP4_ROM_MICROCODE_FORENSICS.md`. No payload or full reports are in this repository. |
| REA package / direct MCP | Cached `rea-agents` 6.9.0 at `/home/gamer/.npm/_npx/9b9809298fe40d2c/node_modules/rea-agents`; direct MCP initialize and `tools/list` succeeded (255 tools), `prompts/list` returned 6. |
| REA MCP in this session | REA-specific tools are absent from the active Codex MCP tool registry. Earlier direct initialize/tool discovery succeeded, but this session cannot invoke those tools. No REA analysis result is claimed for this run. |
| Ghidra | Installed privately at `/tmp/dp4-rom-forensics/tooling/ghidra-install/ghidra_12.1.4_PUBLIC`, with Temurin JDK 21 and isolated `GhidraMC6800` extension v1.2. Headless analyzer and REA doctor passed. Extension supplies `MC6801:BE:16:default` and `HD6301:BE:16:default`; no system or repository environment was overwritten. |
| Ghidra analysis | Ghidra headless analyzed both revisions in MC6801 mode and v1.15 in MC6800 and HD6301 modes. Private reports are under `/tmp/dp4-rom-forensics/reports`; analysis used imported 32 KiB images at assumed base `0x8000`, reset vector `0xFFFE`. |

The checked-in helper `tools/dp4_ghidra/run_u56.sh` imports a caller-supplied U56 image into a new Ghidra project under a caller-owned private work directory and writes a report there. It never copies an image into the repository. `experimental/emulation/capture/tests/test_dp4_ghidra_smoke.py` is an opt-in integration test built from synthetic bytes.

Capstone input mapping used for this static pass is also hypothetical: a 32 KiB U56 image at CPU `0x8000..0xFFFF`, with the reset vector at image offset `0x7FFE`. The vector is `0x84E4`, mapping to file offset `0x04E4`. The service manual identifies a 6803 at U52, consistent with the decoded instruction family, but does not verify the ROM address window or chip-select mapping.

The U56 work below uses Ghidra's MC6801 disassembly, cross-references, computed-flow classification and basic blocks, compared across v1.06/v1.15. The language is a decoding model; it agrees with the service manual's 6803-at-U52 identification but does not prove the ROM map. See [DP/4 hardware architecture audit](DP4_HARDWARE_ARCHITECTURE_AUDIT.md) for the evidence and remaining bus-map unknowns.

## Reset and boot path evidence

With the assumed U56 map, Capstone reaches the following startup path in both revisions:

- Reset at logical `0x84E4`, file `0x04E4`.
- Initialization then clears memory and calls common setup helpers.
- v1.06 logical `0x8608` (file `0x0608`) calls `0xB342` (file `0x3342`); v1.15 logical `0x860B` (file `0x060B`) calls `0xB35C` (file `0x335C`).
- After the call, each revision enters setup code at logical `0x803E` (file `0x003E`).

The caller's addresses and contents shifted across revisions, but the call's high-level action remains a short initialization exchange. It is not a scan or transfer of the 128 KiB U54 image.

## Stable peripheral-write candidate

These CPU-address annotations assume U56 at base `0x8000`; offsets in parentheses are image offsets.

### v1.15

- `0xB385` (file `0x3385`), 11-byte routine: write accumulator A to `$1801`; load mask `2`; poll `$1800` until bit 1 is set; return.
- `0xB373` (file `0x3373`), 18-byte routine: send byte `0x93`; send B; then send 8 bytes from `0,X`, incrementing X for each byte; return.
- `0xB35C` (file `0x335C`) calls the 8-byte sender twice, with B values 13 and 14, using pointers `0xCE97` and `0xCE9F`. Under the hypothesized map, the two arrays are U56 offsets `0x4E97` and `0x4E9F`. It then sends `0x81` and `0x83` through the same byte writer.
- The arrays are exactly 8 bytes apart and are present in U56. Their 8-byte sequences have zero exact occurrences in the corresponding U54 v1.15 image.
- Nearby setup routines at logical `0x8689` and `0x8699` also write/read `$1800` and `$1801`.

### v1.06 comparison

The corresponding routine addresses are `0xB36B` (file `0x336B`) for the byte writer, `0xB359` (file `0x3359`) for the 8-byte sender, and `0xB342` (file `0x3342`) for the caller. The caller uses pointers `0xCE7B` and `0xCE83`, corresponding to U56 offsets `0x4E7B` and `0x4E83` under the same assumed map. Those U56 8-byte blocks also have zero exact occurrences in U54 v1.06.

The 11-byte byte-writer body and 18-byte sender body are byte-for-byte identical between the two OS revisions. Their SHA-256 values, calculated privately from those short code spans, are respectively:

- byte writer: `f399567e576f27dcba7763b26e49f54a8511ada6dfa4b2564d8e188c6be28336`
- 8-byte sender: `a7fcde13a31a63f8f992caab0bd371dd00120ac833d83372b12115590c0cf5b3`

These hashes establish stability only; they do not label the peripheral. A status/data pair with a ready-bit poll is consistent with a serial or gate-array peripheral. The DP/4+ Rev C schematic separately shows a 63B50 serial/I/O device, but its address mapping and applicability to the base DP/4 are unverified. Do not call `$1800/$1801` a MIDI port, UART, or ESP interface without the base DP/4 schematic or a bus trace.

## Why this is not the MAME ES5510 instruction loader

At pinned MAME commit `8be57cdc5f44ab830dff44c06dd39f24190742d4`, `es5510_device::host_w` models a host register interface: six byte writes at offsets `0x03..0x08` assemble a 48-bit instruction latch, selector `0xC0` commits a word to an instruction index, GPR bytes use offsets `0x00..0x02`, and selectors `0xA0`/`0xE0` write register state. The U56 candidate instead issues a two-block, eight-byte-per-block transaction through `$1800/$1801`; its direct access pattern does not resemble the MAME handler's individual register offsets.

An intervening DP/4 gate array could translate a high-level packet into ESP host transactions, so the mismatch does **not** prove the packet is unrelated to DSP setup. However:

1. MAME has no complete DP/4 machine driver to reveal such a translation.
2. The current service material does not map base-DP/4 CPU addresses `$1800/$1801` to a chip.
3. The packets read bytes from U56, not from the known U54 image region.
4. Neither the U54 source address nor any 48-bit instruction index/GPR target is observed.

Therefore, there is no supported basis to identify this as the factory microprogram loader.

## Main path barrier

In v1.15 startup/main-loop code, an object walk reaches an indirect call at logical `0x85EA` (file `0x05EA`): the code obtains a pointer through fields in an object and executes `JSR 0,X`. The routine targets are runtime values. Resolving them requires:

- the verified U56 CPU address mapping and initialized object/descriptor contents, including any data copied from U54;
- U54 bank-select and gate-array behavior;
- observed writes to the ES5510 host interface or an emulator driver that models the same path.

Ghidra classifies the instruction as a computed call but records no destination edge. In v1.06 the matching site is `0x85E7`; in v1.15 it is `0x85EA`. Around it, the code follows object fields (`LDX 7,X`, then `LDX 0,X`) before `JSR 0,X`; the pointer value is runtime data. Ghidra reports 55 functions and 1,323 instructions in v1.06, and 55 functions / 1,326 instructions in v1.15 under the same assumed map. The call remains unresolved; static CFG does not recover the descriptor contents or any banked U54 mapping.

Language comparison on v1.15 yields only 2 functions / 59 instructions in base MC6800 mode, versus 55 / 1,326 in MC6801 and 55 / 1,326 in HD6301 mode. Reachable code includes 6801-family instructions such as `ABX` and `MUL` (including `MUL` in the function at `0x8DE9`). This supports an MC6801/HD6301-compatible ISA model over base MC6800 and agrees with the manual's identification of a 6803 at U52. Exact CPU suffix and physical ROM decode remain to be verified.

## Finding and confidence

| Finding | Confidence |
|---|---|
| U56 v1.06 and v1.15 have a stable 18-byte packet sender and 11-byte status/data writer at the offsets listed above | **High** for bytes and Ghidra/Capstone decode; **medium** for assumed CPU address mapping |
| U52 host CPU is a Motorola 6803-family part | **Confirmed by the base DP/4 service manual** identifying “6803 (U52)” on printed p. 6; exact suffix/package marking not established |
| U56 code decodes in the 6801-family instruction set | **Strongly supported** by reachable `ABX`/`MUL` and Ghidra language comparison; consistent with 6803 |
| U56 ROM CPU window/reset-vector mapping | **Unresolved**; `$8000–$FFFF` remains an analysis assumption despite the valid-looking reset vector |
| `JSR 0,X` target at v1.06 `0x85E7` / v1.15 `0x85EA` | **Unresolved**; computed call has no static target edge |
| The reset path calls a routine that sends two 8-byte U56-resident payloads with IDs 13 and 14, then sends two short values | **High** for the static call path under the assumed memory map |
| `$1800/$1801` are a ready/status plus data peripheral | **Medium** from the access pattern |
| That peripheral is the 63B50/UART/MIDI | **Low/unresolved**; DP/4+ schematic is a different model and does not map these addresses |
| This routine loads ES5510 microinstructions | **Unsupported** |
| A U54-to-ES5510 byte transformation or instruction stream has been identified | **No** |

## Shortest next observation

Capture a base DP/4's U54 bus and ESP host activity simultaneously during power-on and selection of one simple algorithm. First document the board revision and verify U52/U54/U56/ESP package pins against base-board documentation. The capture must include U52 address/data/control/reset/clock, U54 address/data/enable, and each ESP host address/data/control/halt signal on one timebase; the actual base-board net names and pins must be identified before probing. This gives the missing CPU map and U54-to-ESP transaction evidence. Do not infer a loader from the stable `$1800/$1801` helper alone.

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

- [DP/4 Service Manual](https://www.vintagesynthparts.com/wp-content/uploads/2016/08/Ensoniq-DP4-Service-Manual.pdf) — identifies “6803 (U52)” on printed p. 6 and U56 as OS ROM / U54 as UCODE on pp. 13 and 19; it does not provide the base digital-board bus map. See [hardware architecture audit](DP4_HARDWARE_ARCHITECTURE_AUDIT.md).
- [DP/4+ Rev C digital-board schematic](https://convexoptimization.com/EnsoniqDP4/Ensoniq%20DP4%2B%2C%20Digital%20Board%2C%20401024201%20Rev%20C.pdf) — comparison only; identifies 63803, separate OS/UCODE ROMs, 63B50 serial/I/O and ESP sections, but not the base DP/4 map.
- [MAME ES5510 host implementation](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp) — generic device behavior at pinned BSD-3-Clause source commit, not a DP/4 host-driver trace.
