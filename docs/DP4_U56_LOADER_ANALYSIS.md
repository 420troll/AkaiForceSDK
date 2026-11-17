# DP/4 U56 loader investigation

Branch: `issue11-dp4-rom-forensics`  
Starting commit: `988281c9d539c2ff27239850f3f332c3f7467c12`  
Analysis date: 2026-11-17  
ROMs: private DP4 U56 v1.06 and v1.15 images; hashes are in [DP4_ROM_MICROCODE_FORENSICS.md](DP4_ROM_MICROCODE_FORENSICS.md). No ROM, extracted binary, or full disassembly is included here.

## Result

**No ES5510 factory-program loader routine is identified.** A stable, byte-for-byte-identical U56 routine pair was found that transmits short packets through a memory-mapped status/data pair. Its reset-path caller sources two 8-byte blocks from U56 ROM itself. This is verified firmware I/O, but there is no evidence that it reads U54 or writes ES5510 instruction/GPR memory. It is a candidate peripheral initializer, not a decoded DSP loader.

The remaining algorithm-selection path contains an indirect `JSR 0,X` dispatch. Its target depends on a function pointer in runtime data structures; with the base DP/4 CPU, ROM mapping, and U54 bank mapping unknown, the pointer tables cannot be assigned to code or U54 content confidently. The likely U54 loader may be reachable through that dispatch, but that remains a hypothesis.

## REA and disassembler availability

| Check | Result |
|---|---|
| REA package | `rea-agents` 6.9.0, cached and invocable via `npx` |
| REA MCP server self-check | `npx rea-agents mcp doctor --json`: healthy; 255/255 expected tools and 6/6 prompts observed |
| Codex runtime tool registry at start of this turn | No REA tools exposed |
| Codex MCP registration | Registered globally using REA's `mcp add`; config backup retained at `/home/gamer/.codex/config.toml.rea.backup`. Registration is for a subsequent/restarted Codex session; the current tool registry did not hot-reload. |
| Ghidra | Not installed; `ghidra`, `analyzeHeadless`, and `ghidraRun` are absent. REA's Ghidra provider reports `not_configured`, requiring Ghidra 12.1+ and `GHIDRA_INSTALL_DIR`. Ghidra was not installed. |
| REA firmware-region scan | Tried on U56 v1.15; unavailable because `REA_BINWALK_COMMAND` is not set and no caller-supplied `binwalk` executable is configured. |
| Disassembler used | Existing private Capstone 5.0.6 installation, M680X 6801 mode. The mode is a working ISA hypothesis supported by the DP/4+ 63803 clue, not confirmed base-DP/4 silicon identification. 6800, 6801, 6301, and 6811 modes decode the examined routines identically. |

Capstone input mapping used for this static pass is also hypothetical: a 32 KiB U56 image at CPU `0x8000..0xFFFF`, with the reset vector at image offset `0x7FFE`. The vector is `0x84E4`, mapping to file offset `0x04E4`. The DP/4+ schematic cannot verify the base DP/4 map or CPU part.

REA's MCP process is healthy, but the current Codex session could not invoke those MCP tools because its tool registry was fixed before the global registration. The analysis below used REA CLI health/capability checks and Capstone; it did not use Ghidra or claim a Ghidra decompilation.

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

- the actual base DP/4 memory map and CPU variant;
- initialized object/descriptor contents, including any data copied from U54;
- U54 bank-select and gate-array behavior;
- observed writes to the ES5510 host interface or an emulator driver that models the same path.

The entry bytes and static direct calls alone do not provide these values. Ghidra would improve flow graphs after choosing a valid CPU language and memory map, but it would not make unresolved bank/indirect data references known. In this environment it was not available.

## Finding and confidence

| Finding | Confidence |
|---|---|
| U56 v1.06 and v1.15 have a stable 18-byte packet sender and 11-byte status/data writer at the offsets listed above | **High** for the bytes and Capstone M6801 decode; **medium** for the assumed CPU address mapping |
| The reset path calls a routine that sends two 8-byte U56-resident payloads with IDs 13 and 14, then sends two short values | **High** for the static call path under the assumed memory map |
| `$1800/$1801` are a ready/status plus data peripheral | **Medium** from the access pattern |
| That peripheral is the 63B50/UART/MIDI | **Low/unresolved**; DP/4+ schematic is a different model and does not map these addresses |
| This routine loads ES5510 microinstructions | **Unsupported** |
| A U54-to-ES5510 byte transformation or instruction stream has been identified | **No** |

## Shortest next observation

Capture a base DP/4's U54 bus and ES5510 host bus simultaneously during power-on and selection of one simple algorithm. The minimum decisive data are U54 address/data/enable, CPU reset/bank-select, and selected ES5510 host address/data/read-write/chip-select/HALT signals with a shared timebase. First identify the actual CPU and board revision from the installed package marking and a base-DP/4 schematic. Then map the U56 startup and indirect dispatch with that verified memory map. Do not infer a loader from the stable `$1800/$1801` helper alone.

## Reproduction notes

Private analysis used the locally extracted ROMs under `/tmp/dp4-rom-forensics` and Capstone 5.0.6, installed only under `/tmp/dp4-rom-forensics/capstone`. No code or ROM payload from the private directory is included in the repository. The exact Capstone command is reproducible as:

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

This is a linear disassembly of a candidate span, not a complete loadable Ghidra project or validated firmware emulation. Existing synthetic repository tests cover the ROM scanner and MAME-field decoder; no new decoder or trace tool was added because the actual loader protocol is still unknown.

## Sources

- [DP/4 Service Manual](https://www.synfo.nl/servicemanuals/Ensoniq/ENSONIQ_DP-4_SERVICE_MANUAL.pdf) — confirms U56 is OS ROM and U54 is VCODE/UCODE; does not identify base-model CPU or memory map.
- [DP/4+ Rev C digital-board schematic](https://convexoptimization.com/EnsoniqDP4/Ensoniq%20DP4%2B%2C%20Digital%20Board%2C%20401024201%20Rev%20C.pdf) — comparison only; identifies 63803, separate OS/UCODE ROMs, 63B50 serial/I/O and ESP sections, but not the base DP/4 map.
- [MAME ES5510 host implementation](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp) — generic device behavior at pinned BSD-3-Clause source commit, not a DP/4 host-driver trace.
