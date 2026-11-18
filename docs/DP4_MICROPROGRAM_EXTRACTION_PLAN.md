# DP/4 factory microprogram extraction plan

Research branch: `issue11-dp4-rom-forensics`  
Baseline: `9f0f6b98318915871db9fce2dacbfbe8bc73d86d`

Research update: 2026-11-18

## Conclusion

No verified relationship currently connects a particular U54 byte range to the instruction memory of a physical DP/4 ES5510. U54 is verified as the VCODE/UCODE EPROM; MAME's ES5510 device model explains how a host can write instruction memory and GPRs; MAME does **not** model the complete DP/4. The base DP/4 service manual identifies a 6803 at U52, consistent with the U56 6801-family decode. The CPU address map, base-board ROM/DSP bus wiring, and ROM-to-ESP upload routine remain unconfirmed. See [DP/4 hardware architecture audit](DP4_HARDWARE_ARCHITECTURE_AUDIT.md). The shortest evidence-producing next step is a synchronized, read-only bus capture on a DP/4 while booting and selecting one effect, capturing the U54 ROM bus and selected ESP host activity together.

This is not an algorithm recovery. No extracted factory program has been run or compared against a DP/4 output.

The follow-up U56 static pass is recorded in [DP4_U56_LOADER_ANALYSIS.md](DP4_U56_LOADER_ANALYSIS.md). It found a stable status/data sender on the reset path, but its packets are sourced from U56 and it cannot be identified as the ES5510 loader.

## Verified discoveries

### U54 revision structure

The private images and hashes are recorded in [DP4_ROM_MICROCODE_FORENSICS.md](DP4_ROM_MICROCODE_FORENSICS.md). `tools/es5510_rom_forensics.py` now reports change overlap with the detected label regions. On the v1.06/v1.15 pair:

| U54 region | Geometry | Changed bytes | Finding |
|---|---|---:|---|
| Algorithm-name records | `0x0000..0x09F5`, 50 × 51-byte | 0 | Identical across versions |
| Effect/preset-name records | `0x09F6..0x1AF3`, 50 × 87-byte | 0 | Identical across versions |
| Named configuration records | `0x2001..0x3EDC`, 50 × 158-byte | 0 | Identical across versions |
| Outside these detected label regions | Includes all offsets from first difference `0xB4E0` through last `0x19DC0` | 3,351 | 2.56% of the image; relationship to code, tables, or coefficients unknown |

Changed 4 KiB buckets are `0xB000..0xBFFF` (2,239 bytes), `0x12000..0x12FFF` (1), `0x13000..0x13FFF` (1,105), `0x16000..0x16FFF` (3), `0x17000..0x17FFF` (2), and `0x19000..0x19FFF` (1). These are physical image offsets. They do not map to effect numbers or CPU addresses yet. The unchanged name records show that the visible directory labels are not where the revisions differ; they do not prove whether a changed cluster is instruction code, coefficients, host tables, or a different data structure.

The six tested raw 48-bit packing/lane hypotheses still produce median one-word END spans. This weakens simple uniform raw-word storage, but does not rule out block-specific transforms, split fields, compression, relocation, or loader decoding.

### U56 identification and startup

- The service manual identifies U52 as a 6803 (printed p. 6) and U56 as the operating-system EPROM. It does not provide the full CPU, ROM, or ESP address-decoding schematic.
- U56 is 32,768 bytes. Its last two bytes are `0x84E4` in both versions. If the ROM is mapped at CPU address `0x8000..0xFFFF`, the vector at image offset `0x7FFE` is the 6800-family reset vector and points to image offset `0x04E4`. This is a plausible map, not a verified DP/4 memory map.
- Ghidra 12.1.4 with the isolated GhidraMC6800 v1.2 extension reaches 55 functions in each ROM using `MC6801:BE:16:default`; v1.15 contains reachable `ABX` and `MUL`. In the same image, base `MC6800` mode reaches only 2 functions / 59 instructions, while `MC6801` and `HD6301` reach 55 / 1,326. This supports a 6801-family instruction set, consistent with the manual's 6803 identification. The precise CPU suffix and physical ROM decode still need board-level confirmation. See the updated [U56 loader analysis](DP4_U56_LOADER_ANALYSIS.md).
- The DP/4+ Rev C digital-board schematic is a separate-model comparison: it names a 63803 microprocessor, a 27256 O/S ROM, a banked 27C2001 ROM, and shows the UCODE ROM separately. Its ROM bank and ESP signal wiring must not be projected onto the base DP/4.
- Under the assumed map, startup begins at `0x84E4` (file offset `0x04E4`), initializes the stack and several low memory locations, calls helpers, clears RAM ranges and writes I/O-looking addresses including `0x1800/0x1801`. Ghidra's reachable CFG reports an indirect `JSR 0,X` at v1.06 `0x85E7` and v1.15 `0x85EA` with no static destination edge. The pointer is loaded through runtime object fields, so static analysis does not resolve it. No call sequence was tied to ES5510 program writes or U54 source offsets.
- The service manual reports UCODE tag, instruction/table/GPR verify, and download-verify failures. This supports that the firmware validates multiple structured categories; it does not reveal the U54 serialization or transfer routine.

### MAME host-interface model and system coverage

Pinned source is `mamedev/mame` commit `8be57cdc5f44ab830dff44c06dd39f24190742d4`, BSD-3-Clause. In `src/devices/cpu/es5510/es5510.cpp`, MAME models:

1. Host writes to offsets `0x03..0x08` fill a 48-bit instruction latch, most-significant byte first.
2. A write-select transaction at `0xC0`, with the program index on the data bus, commits that latch to instruction index 0–159. MAME limits program memory to 160 words.
3. Offsets `0x00..0x02` fill a 24-bit GPR latch; selector `0xA0` commits it to the selected register. Selector `0xE0` writes instruction and register state together. Selector `0x80` reads instruction/GPR state into host latches.
4. Other modelled host offsets include host control/RAM clear (`0x12`), RAM selection (`0x14`), host serial mode (`0x18`), and halt enable (`0x1F`). The HALT input controls whether host access is allowed; MAME's execution model restarts at PC 0 and uses END/halt behavior to frame processing.

These are **MAME device semantics**, not an observed DP/4 bus transcript and not a claim that physical DP/4 firmware uses those exact write sequences. MAME's source also contains `LOG` calls for host latch and selector writes, but `VERBOSE_EXEC` is hard-coded to 0 in the pinned source. Enabling it gives instruction-level logging as well as host logs and can be extremely verbose.

The Ensoniq MAME source tree has ES5510 device instances in other system drivers (`esq5505.cpp`, `esqkt.cpp`, `esqasr.cpp`). A repository-wide source search finds no DP/4 machine driver. Device-level ES5510 support is therefore not DP/4 machine support. The logs can trace host writes in a supported MAME machine; they cannot execute DP/4 U56/U54 firmware or trace its loader without a DP/4 system driver.

## What is still unresolved

1. The exact U52 6803 variant/package, clock, and verified U56 reset/vector mapping.
2. The base DP/4 CPU address map, U54 bank/enable logic, and whether U54 reads are direct, banked, interleaved, or copied through another memory region.
3. The physical ES5510 revision/package pinout used on the target base DP/4 board and the exact host address/data/strobe connections.
4. The DP/4 host's ESP register transactions, including whether it uses six instruction-latch byte writes followed by a selector, combined writes, bursts, or a different access path.
5. U54 record schemas and the transformation from ROM bytes to instructions/tables/GPR initialization.
6. Per-algorithm program boundaries, actual GPR/control maps, DRAM sizing/address maps, serial routing and frame scheduling.
7. Whether v1.06/v1.15 changed factory algorithm bodies, coefficients, loader tables, or unrelated code in the changed clusters.

## Shortest reproducible path to one decoded algorithm

### Preferred: one synchronized hardware capture

Use a known-good DP/4 with the case open only by a qualified electronics technician. Use high-impedance, buffered digital probes and verify signal voltage/timing against the exact ES5510 revision datasheet and the actual board revision. The service manual warns to unplug the unit before opening it; do not attach or remove probes while powered. Do not use the DP/4+ schematic as a pinout for the original DP/4.

Capture two buses concurrently, with a common timebase and trigger:

**U54 source side**

- U54 address `A0..A16` (or all actual address/bank-select lines present on the board).
- U54 data `D0..D7`.
- U54 chip-enable and output-enable strobes (actual active-low/active-high polarity from the board schematic).
- Host reset and relevant bank-select signals, so every output byte is associated with the physical U54 offset and revision.

**ES5510 destination side**

- The selected ESP's multiplexed host address/data bus and latched data/address signals as applicable to that silicon revision.
- Host address strobe (`AS` where the exact revision/board uses it), read/write, ESP chip select, HALT/host-ready, and DSP clock/reset as supported by the exact pinout.
- Capture the four serial channels and frame/clock signals if channels permit; otherwise capture them in a second synchronized run after program load.

The DP/4+ Rev C schematic is a helpful *comparison-only* reference: it labels a multiplexed host A/D bus, control bus, `AS`, `R/W`, ESP chip select, HALT, ESP clocks and serial signals. It identifies U25 63803 and four ESP sections on that later model. The corresponding signal names and package pins on a base DP/4 must be checked before probing.

**Capture sequence**

1. Start capture before power-on and trigger on reset release. Record the complete boot-time U54 reads and ESP writes.
2. After boot, select exactly one simple effect (start with a one-unit delay) and capture the complete algorithm-change/init interval.
3. Without changing algorithm, adjust one exposed parameter and capture again. This distinguishes program load from coefficient/GPR updates.
4. Repeat the same sequence to verify deterministic offsets and transaction ordering. Record board revision, EPROM labels/revision, chip markings, probe points, sample rate and instrument settings.
5. Decode the destination bus transactions first, independently of U54. Reconstruct the actual host writes to the ES5510 and compare the observed sequences against the MAME 48-bit latch/select model. Do not impose MAME byte ordering if the trace disagrees.
6. Correlate the source-side U54 read cycles with destination writes by time and data. Test transformations only after each bus is independently decoded. Identify one algorithm only when a contiguous complete instruction set (≤160 words per program), required table/GPR initialization, delay RAM configuration, serial route and end/frame behavior are all explained.
7. Keep the capture and extracted proprietary program private. Publish only a rights-reviewed hash, offset map, method, non-substantial statistics, and a synthetic regression fixture.

If channel count is limited, use a multi-channel logic analyzer or two analyzers with a shared external trigger/reference clock. A two-pass source/destination capture is not sufficient to prove the exact byte-to-instruction mapping unless the boot and selection sequences are demonstrably deterministic and the two runs can be aligned unambiguously.

### Alternative: add a DP/4 machine model to MAME

This requires the base DP/4 CPU identity/address map, exact board logic, ROM declarations, ES5510 revision/clocking, audio ADC/DAC path and serial routing. Porting the 6801-compatible U56 image into a guessed machine would repeat the unsupported mapping assumptions above. First use a bus capture to establish the map; then add a MAME driver that logs U54 reads and calls the existing ES5510 host handlers. Keep the logger behind a command-line/debug flag, output transaction address/data/time only, and write synthetic tests against the reconstructed transaction parser.

## Extraction validation checklist

For the first candidate factory algorithm, require all of the following before calling it decoded or recovered:

- U54 revision and SHA-256 match the private acquisition record.
- Each source byte has an observed CPU-side U54 address/read event.
- Each ES5510 write is observed and reconstructed with the exact target index/register.
- Instruction words agree with the ES5510 technical specification and MAME field model; all control bits and instruction boundaries are accounted for.
- Program ends/dispatch, GPR/table initialization, external DRAM setup and serial schedule are included.
- The program executes from initialization in an emulator or compatible hardware, produces non-zero finite output, and follows the manual's exposed controls.
- A matched hardware audio capture or known-good reference output is compared; any accuracy claim includes its method and error metrics.
- Copyrighted ROM/program bytes stay out of Git and release packages.

Until this passes, continue to call the TROLLfx instruction bundles original examples, not DP/4 factory programs.

## Tooling and tests

Version compare and table overlap map (ROMs must remain private):

```sh
python3 tools/es5510_rom_forensics.py /private/DP4_V106_ucode.bin \
  --compare /private/DP4_V115_ucode.bin \
  --json-output /private/u54-v106-v115-metadata.json
python3 -m unittest discover -s experimental/emulation/capture/tests -v
```

The scanner reports only hashes, labels and structural metadata; do not commit generated reports if they contain proprietary labels. The current synthetic tests confirm label-table geometry, MAME field decoding, candidate word-scan behavior, and changed-byte overlap calculations. They do not validate a factory program.

## Sources

- Ensoniq, *DP/4 Service Manual*, U54/U56 designators, ROM replacement and initialization notes: [PDF](https://www.synfo.nl/servicemanuals/Ensoniq/ENSONIQ_DP-4_SERVICE_MANUAL.pdf).
- Ensoniq, *DP/4+ Digital Board Schematic*, Rev C, especially microprocessor/ROM sheet 2 and ESP sheets 5–8. This is for DP/4+, not the base DP/4: [schematic PDF](https://convexoptimization.com/EnsoniqDP4/Ensoniq%20DP4%2B%2C%20Digital%20Board%2C%20401024201%20Rev%20C.pdf).
- Hitachi, *HD6303/HD63B03 CMOS Microprocessing Unit* family description, for the 63803/6801-compatible comparison: [datasheet copy](https://manuals.plus/m/23e47fd270b889dcfe58cc7dcf417d39e44a5320f894e7682f360fcbc7ea8473).
- MAME, pinned ES5510 CPU implementation (`es5510.cpp`, `es5510.h`, `es5510d.cpp`) at `8be57cdc5f44ab830dff44c06dd39f24190742d4`: [source tree](https://github.com/mamedev/mame/tree/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510). License: BSD-3-Clause.
- MAME Ensoniq machine source (`esq5505.cpp`, `esqkt.cpp`, `esqasr.cpp`) at the same commit. These model other Ensoniq systems and do not contain a DP/4 machine driver: [Ensoniq source directory](https://github.com/mamedev/mame/tree/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/mame/ensoniq).
- Ensoniq, *ESP (ES5510) Technical Specification v2.4* and unofficial ESP microcode programming guide are listed in the [R-Massive Ensoniq technical-document index](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/). Use the revision-appropriate pinout before hardware probing.

## Routing constraint for future integration

The user's plugin topology is always **A → B → C → D serial**. This board-level extraction work neither changes nor emulates DP/4's parallel/feedback routing options.
