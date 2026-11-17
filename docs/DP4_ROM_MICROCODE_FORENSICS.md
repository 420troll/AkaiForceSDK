# DP/4 ROM microcode forensics

Research branch: `issue11-dp4-rom-forensics`  
Research date: 2026-11-17  
Scope: privately acquired DP/4 v1.06 and v1.15 EPROM images; metadata and synthetic tools only are published.

## Finding in one sentence

The images are correctly identified as U54 VCODE/UCODE and U56 operating-system ROMs, and U54 contains structured algorithm/preset/configuration data, but this investigation **has not yet isolated or executed an authentic factory ES5510 program**. Names in a ROM are not proof that their associated processing code has been decoded.

## Evidence and confidence

| Statement | Status | Evidence and limit |
|---|---|---|
| U54 is the DP/4 VCODE/UCODE ROM; U56 is O/S ROM | **Verified** | The DP/4 service manual calls the two EPROMs UCODE ROM U54 and O/S ROM U56, and identifies the burn-in checksum labels OS=U56 / UC=U54. The archive member names and the 128 KiB / 32 KiB sizes agree with those roles. |
| v1.06 and v1.15 images are available in the cited archives | **Verified for the downloaded archive copies** | Both DBWBP and Midisoft list both revisions. Corresponding ZIP files downloaded from the two hosts were byte-identical per revision. Public listing does not establish a license to redistribute their contents. |
| U54 starts with 50 regular 51-byte algorithm-name records | **Verified as byte geometry; semantics only partly inferred** | Offsets `0x0000` through `0x09F5` form 50 records at stride `0x33`. Labels include Small/Large Room, Hall, Small/Large Plate, Reverse, Retrig Reverse, Gated, Non Lin 1/2/3, Phaser-DDL, Rotating Speaker and No Effect. The following bytes are not decoded as a documented directory structure. |
| U54 has additional 50-record label-bearing regions | **Verified as byte geometry; payload meaning unresolved** | A second candidate region starts at `0x09F6`, stride `0x57`, ends exclusive at `0x1AF4`; it contains effect/preset-style names such as “3.3 sec Delay 2U”, “Big Non Lin”, “Luscious Plate”, “Versatile Hall”, “Warm Room” and “Science Lab”. A third starts at `0x2001`, stride `0x9E`, and has 50 configuration-like labels ending with “Dense Mist”. These are label-bearing records, not proven executable-program records. |
| U54 v1.06 and v1.15 differ in part of the image | **Verified** | Same 131,072-byte length. 3,351 bytes differ (2.5566%), across 417 contiguous runs; first changed offset `0xB4E0`, last `0x19DC0`. The named regions above precede the first differing byte. Mapping differences to algorithms or code is unresolved. |
| U56 revisions differ substantially | **Verified** | Both are 32,768 bytes; 30,080 differ (91.7969%), first offset `0x000D`, last `0x7FF9`. Both have reset vector `0x84E4` in the final two bytes. Similar vector value does not imply identical initialization or loader code. |
| U56 contains 6800-family-compatible boot instructions | **Inferred, not silicon identification** | With Capstone's M680X 6801 and 6811 modes and an inferred `0x8000` ROM map, reset vector `0x84E4` maps to file offset `0x04E4`. The initial boot sequence decodes identically in these modes. It does not identify 68HC11 specifically; the earlier 68HC11-family wording was too strong. |
| DP/4 host processor part number | **Unresolved for base DP/4** | A separate DP/4+ Rev C digital-board schematic names a 63803 controller and 27256 O/S ROM. It is a useful family clue only; it does not prove the base DP/4 uses the same processor or address map. |
| U56 microcode uploader/configuration routine is identified | **Unresolved** | The service manual's error descriptions refer to UCODE tag / instruction / table / GPR verification and download verification. This verifies that firmware handles structured ESP program and state data, but does not identify the actual upload routine, bus/register map, framing or U54 offsets. The current quick control-flow trace is not enough to make that mapping. |
| Authentic factory ES5510 microprogram has been extracted and executed | **No** | None of the ROM bytes has yet been established as a complete instruction bundle with initial GPR, external delay-memory setup, audio routing and scheduling, then executed against an expected factory output. |

## Private image inventory

All downloaded material, extracted images, the service manual, temporary disassembly and full metadata scan output remain under `/tmp/dp4-rom-forensics` on the research machine. Nothing from that directory is in this repository. The archive ZIPs at DBWBP and Midisoft were identical per revision; hashes below are for the downloaded ZIP bytes.

| Revision | Archive SHA-256 | Member | Size | Member SHA-256 |
|---|---|---|---:|---|
| 1.06 | `9b7aad663a4858e9ffc554b9fe1a7534392429b6c178da03d17dfeffc68267cf` | `DP4_V106_ucode.bin` (U54) | 131,072 | `844e81e261b14c96d74267928f9e26e91ff8dc109a8ae7d97f95f9ac5480577c` |
| 1.06 | same ZIP | `DP4_V106_os.bin` (U56) | 32,768 | `b15aa2be05233356fae39b52d87d978d84055c7accb032fe742b861501966f8c` |
| 1.15 | `c5b41b43ca0b77615b48f8108ffd699ac557b5c23865d9aa96a0bc2ac6da5f94` | `DP4_V115_ucode.bin` (U54) | 131,072 | `392f689ef2b9b2fcdfe0ce021fc8b5ea04d609fe5ce1d4928f47a321d7311302` |
| 1.15 | same ZIP | `DP4_V115_os.bin` (U56) | 32,768 | `664c7c0f89ca9a4055bc5d19b84e5f561065dd44ee8dd3282f87ac2e1d7c2465` |

The service manual additionally describes v1.06 changes to VCF Distortion follower release and polyphase interpolation in FastPitchShift, EQ-DDL-LFO, and EQ-Chorus-DDL, with pitch-shift timing fixes. Those are software release notes, not proof that the changed U54 byte ranges implement those algorithms. No equivalent v1.15 change mapping has been established.

## Packing and execution hypotheses tested

The new `tools/es5510_decode.py` extracts instruction fields defined by the pinned MAME ES5510 core. `tools/es5510_rom_forensics.py` hashes a local image, finds selected public algorithm labels, reports the regular label-record hypotheses above, compares revisions, and scans raw 48-bit candidates across all six byte alignments for six plausible byte/lane permutations:

- big-endian 48-bit words;
- fully reversed byte order;
- byte-swapped 16-bit lanes;
- reversed 16-bit lane order;
- swapped 24-bit halves;
- reversed byte order in each 24-bit half.

For U54 v1.06, the naïve big-endian stream at offset zero marks 4,380 of 21,845 candidate words (20.1%) as END, with a median END-delimited span of one word. Little-endian gives 4,669 ENDs (21.4%), also median one word. Restricting the scan to the differing region beginning at `0xB4E0` still gives roughly 21–24% END and median one-word spans across tested layouts. These are poor signatures for a collection of complete programs bounded by END, and the regular labels/tables show that the image is not one uniform instruction stream.

This rejects only the tested **uniform raw-stream** hypotheses. It does not rule out packed instructions, banked/interleaved code, compressed blocks, split instruction fields, encrypted/obfuscated data, a loader transformation, or code stored alongside tables. MAME's `es5510d.cpp` disassembler is a stub in pinned commit `8be57cdc5f44ab830dff44c06dd39f24190742d4`; the field map used here comes from `es5510.cpp`, and the tests validate the decoder against synthetic words. Decoder agreement with MAME's source is not evidence that a U54 byte sequence is executable.

## What can and cannot be said about effects

The algorithm names in U54 align with the DP/4 manual's exposed inventory, and the named configuration records include numerous reverb/effect labels. This supports a role as a DSP/effect data ROM, consistent with the service-manual name “UCODE ROM.” It does **not** reveal the internal reverb topology, instruction count, diffusion taps, coefficient schedule, GPR assignments or delay-memory map for Small/Large Plate, Room/Hall, NonLin, Gated/Reverse, Phaser-DDL or Rotary Speaker.

Therefore, no structural comparison between factory and TROLLfx-authored reverb microprograms is currently defensible. Existing TROLLfx programs remain original, separately authored ES5510 examples; do not relabel them as DP/4-derived or recovered. Further algorithm-specific claims require executable boundary and initialization evidence.

## Tools and reproducibility

The committed tools never write ROM bytes or candidate disassembly. Full metadata reports can contain proprietary labels, so retain reports locally. The capture inspector continues to accept only a local ASCII trace/neutral instruction-word list, not arbitrary firmware. No MAME implementation source was copied in this change; the BSD-3-Clause notices in `experimental/emulation/THIRD_PARTY_NOTICES.md` remain applicable to the existing emulator.

Synthetic validation:

```sh
python3 -m unittest \
  experimental.emulation.capture.tests.test_inspect \
  experimental.emulation.capture.tests.test_rom_forensics -v
```

Private scan command (example; substitute locally acquired files, never check them in):

```sh
python3 tools/es5510_rom_forensics.py /private/path/DP4_V106_ucode.bin \
  --compare /private/path/DP4_V115_ucode.bin
```

Pinned MAME source reference: `mamedev/mame`, commit `8be57cdc5f44ab830dff44c06dd39f24190742d4`, `src/devices/cpu/es5510/es5510.cpp` and `es5510d.cpp`; BSD-3-Clause. The scanner consumes local binaries only and has no runtime MAME dependency.

## Sources

- Ensoniq, *DP/4 Service Manual*, especially UCODE/O/S ROM identification and checksum section (U54/U56), plus v1.06 release notes. [Service-manual PDF](https://www.synfo.nl/servicemanuals/Ensoniq/ENSONIQ_DP-4_SERVICE_MANUAL.pdf).
- DBWBP, *Synthesizer EPROM dumps*, listing DP/4 1.06 and 1.15 archives: [archive index](https://dbwbp.com/index.php/9-misc/37-synth-eprom-dumps).
- Midisoft, *Synthesizer Firmware Updates, E-PROMs & Sound ROMs*, listing DP4 1.06 and 1.15: [archive index](https://www.midisoft.de/firmware/Synthesizer_Firmware_Update_Eproms.html).
- Ensoniq, *DP/4 Musician's Manual*, for documented algorithm names, controls and user-visible behavior: [manual PDF](https://umlsrt.com/StudioDocuments/Ensoniq%20DP4%20Manual%20.pdf). It does not document factory instruction bytes.
- MAME pinned processor implementation: [ES5510 source directory](https://github.com/mamedev/mame/tree/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510).

## Next experiments

1. Obtain a board-level schematic/logic trace or a legally obtained emulator trace that records U54 reads and ES5510 program-memory writes; map the DP/4 controller's actual ROM address space and ES5510 bus registers.
2. Add upload tracing to an authorized DP/4-capable emulator (or hardware bus capture) and record instruction writes, table/GPR writes, delay-memory setup and serial routing for one effect. Preserve only minimal, rights-reviewed traces privately until redistribution permission is clear.
3. Align captured ES5510 write sequences with U54 byte ranges. Only then test alternative compression/packing or relocation transforms.
4. Use the original DP/4 manual's disclosed controls to select a simple delay or plate program and compare parameter updates against captured GPR/table writes.
5. Keep all four Force units in the user's chosen A→B→C→D serial order in any future host integration; this forensics branch does not alter plugin routing.
