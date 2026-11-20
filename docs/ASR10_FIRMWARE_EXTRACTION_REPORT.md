# ASR-10 OS installer extraction report

Date: 2026-11-20. Branch: `feature/5510-asr10-factory-programs`. Proprietary
installers, images, extracted OS/effect data, and Wine prefixes remain under
gitignored `research_local/asr10_firmware/`.

## Outcome

Both authorized Windows NE self-extractors were executed in isolated, offline
32-bit Wine containers. Setup installed the Ensoniq DiskWriter payloads and
the accompanying EDE utility/instructions. The resulting `.EDA` images were
expanded by a new read-only parser into raw ASR 1.6 MB images. Their ID block,
OS block, root directory, FAT chains, and file extents validate.

Static disk analysis recovered an ASR OS object and 13 accompanying effect
objects. The ASR effect headers match the mixed effect-object layout described
by the unofficial ESP1 guide. `FxUcode` pointers lead to sections that decode
as 82–103 six-byte ES5510 instruction words, contain `END` opcodes, and fit the
160-word program memory. All 13 ASR effect payloads are byte-identical between
OS 1.61 and 3.53. This is strong static evidence for authentic factory DSP
sections. It does not yet prove exact program boundaries, associated GPR and
delay-memory setup, firmware upload behavior, or successful execution. No
factory program has yet been replayed in TROLLfx 5510 or compared with MAME.

## Inputs and isolated execution

Before execution the installer hashes were rechecked:

```text
asr161os.exe 255393 bytes
SHA-256 32109774e2b5cd808ebd6572122e77585cd67846c30f06543a6a80449643b680
asr353os.exe 295581 bytes
SHA-256 3276bf47f578398d608f2b33e8350f5db66f95c4ae62e8f8a13fbda38d44a571
```

7-Zip 26.01 could not open either NE SFX or its carved `ARCV` payload. An
InstallExplorer/WCX reader also failed. After the user's explicit
authorization, each installer was run in an ephemeral container with 32-bit
Wine 8.0, Xvfb, fresh Wine prefix, `--network none`, read-only installer mount,
and private writable output mount. No unrelated local files or credentials
were mounted. This was Wine compatibility execution, not genuine Windows 3.x.
The 16-bit setup UI emitted `setupmn.exe` and `setup.stx`, then wrote files to
`C:\EXTRACTO`. `setup.stx` members and sizes agree with the recovered files.

## Recovered file hashes

| Version | File | Bytes | SHA-256 |
|---|---|---:|---|
| 1.61 | `asr161os.eda` | 168,449 | `ab540dc90ee1b11a47c009f16d71a9c6b0807f1c16fd670d42f716e1de7d99d9` |
| 1.61 | `ede.exe` | 61,632 | `907933e1317f32d13da3e6b2b43bbc4aca2492c2dd06c104bb1e79b92a254143` |
| 1.61 | `extract.txt` | 690 | `aab3080717fab6f4bbd1b6a2fa94d2b9955c9300523472c172f110227c48d886` |
| 3.53 | `asr353os.eda` | 275,457 | `d6ff7b93f2ad8f3a554597dba692e8627af4b7ecce8748abcf3c03b94380e7fd` |
| 3.53 | `ede.exe` | 61,632 | `907933e1317f32d13da3e6b2b43bbc4aca2492c2dd06c104bb1e79b92a254143` |
| 3.53 | `extract.txt` | 690 | `aab3080717fab6f4bbd1b6a2fa94d2b9955c9300523472c172f110227c48d886` |
| 1.61 | `asr161os.img` | 1,638,400 | `2a5cc161e80001daddf532914e80e854d3079f152197ab3def31055f239182f6` |
| 1.61 | extracted `ASR-10 OS` object | 88,576 | `86c933fa7e0a425ba4385b26065dc0cb312549bb9aa55b58e22c7cc58b639130` |
| 3.53 | `asr353os.img` | 1,638,400 | `335838dcf96dcec6e7fc2db4d671e2ac5075976999762beef74f472b439a98e9` |
| 3.53 | extracted `ASR-10 OS` object | 195,584 | `2f9db68847002ce0af4c9ab4f364f12d27088ef1f02edc5c4971012f6cb5ee90` |

`tools/asr10_eda_to_img.py` checks the ASR EDA signature, 512-byte header,
skip-map length, exact stored-block count, and the one trailing zero present in
both vendor files. It reconstructs 3,200 blocks (fill pattern `6D B6` for
skipped blocks). `tools/asr10_disk_catalog.py` checks volume signatures,
decodes the root directory/FAT, verifies each extent, emits a hash-only
catalog, and can write extracted object bytes only outside the repository or
under ignored `research_local/asr10_firmware/`.

These tools implement the format documented in the Chicken Systems Disk Tools
manual and cross-check the structure implemented by EpsLin Neo v1.58 at commit
`cf20955e6f9f64dca3c06deaa220598e5fe1bd08`. No EpsLin source is copied.

## Disk contents and comparison

The EDA files differ substantially: sizes are 168,449 and 275,457 bytes; first
difference offset 137; 136,119 differing bytes within the common length; 302
of 329 full 512-byte windows differ. Reconstructed raw image labels are
`ASR161OID` and `ASR353OID`.

Both root directories contain entry 0 type 32 `ASR-10 OS`, starting at block
24 (173 contiguous blocks for 1.61, 382 for 3.53). Entries 1–12 are type 33
ASR effect files; entry 13 is type 24 `PARALLEL EFX`. The effect payload names,
block counts, disk offsets, and SHA-256 hashes are cataloged in
`ASR10_FIRMWARE_INVENTORY.md`. The shared effect bytes establish that the
factory effect objects did not change between these two OS disks, even though
the OS object differs in size and hash.

The 1.61 and 3.53 OS objects begin with identical 68k-like bytes, while their
firmware strings and version metadata differ. Their actual CPU mapping and
loader routines have not been disassembled in this milestone. Do not infer
executable entry points from a plausible vector-like prefix alone.

The recovered effect headers match the guide's layout: the spaced `FxName`
begins at offset 10, `FxSize` at 34, `FxUcode` at 38, `FxTable` at 40,
`FxInitCode` at 42, and update-task pointer at 44. For example,
`44DDL+CHORUS` has `FxSize=3161`, `FxUcode=0x0a3c`, and a 3,584-byte disk
extent. The section from `FxUcode` to `FxSize` contains 90 complete words and
one trailing byte. Other effects have 82–103 candidate words and 0–5 trailing
bytes. `tools/asr10_efe_inspect.py` reports header fields and instruction
statistics without printing the proprietary bytes. The exact section trailer
semantics and GPR/memory initialization are unresolved.

## Sources and next experiments

- [Ensoniqsamplers OS index](https://www.ensoniqsamplers.com/epsasr/026.html) — installer links.
- [Chicken Systems Disk Tools manual](https://www.chickensys.com/_docs/pdf_dt.pdf) — Giebler EDA header and skip-table format.
- [EpsLin Neo](https://github.com/handsthatstrike/EpsLinNeo), GPL-2.0, inspected at commit `cf20955e6f9f64dca3c06deaa220598e5fe1bd08`.
- Unofficial [ESP1 Microcode Programming Guide](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) — effect header and microcode section field map.

1. Disassemble the two OS objects and effect-local `FxInitCode`/update-task
   routines as 68k. Identify code that reads `FxUcode`, initializes GPRs and
   delay-memory registers, and commits instructions to the ES5510.
2. Confirm microcode end/trailer semantics against that loader, and compare
   candidate programs with MAME execution/readback.
3. Obtain the still-missing lawful ASR boot ROM pair, boot the reconstructed
   disks in pinned OpenASR, and record actual ES5510 transactions during effect
   selection and parameter edits.
4. Replay one complete program with captured init state through the standalone
   executor before claiming authentic execution.

## Validation and privacy

Synthetic tests cover EDA bit ordering, truncation, trailing-data checks, disk
geometry, directory records, FAT termination, EFE header pointers and
microcode-stat bounds. Python test command:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools/tests -p 'test_asr10_*.py' -v
```

No proprietary binaries or extracted program bytes are tracked. No Force
plugin or hardware installation was changed.
