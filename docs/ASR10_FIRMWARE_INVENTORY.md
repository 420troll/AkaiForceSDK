# ASR-10 firmware and effect-media inventory

Updated 2026-11-20. Proprietary installers and recovered payloads remain under
the ignored `research_local/asr10_firmware/` tree. No firmware, disk image, or
factory effect bytes are tracked or distributed by this repository.

## OS installer and recovered payload inventory

The installers were fetched from the Chicken Systems URLs linked by the
[Ensoniqsamplers OS index](https://www.ensoniqsamplers.com/epsasr/026.html).
Installer hashes were rechecked before execution. Both Windows NE self-
extractors were run in separate ephemeral 32-bit Wine prefixes inside isolated
containers with networking disabled, the installer mounted read-only, and a
private output directory mounted writable. This was not a genuine Windows 3.x
installation. Setup's own UI extracted an Ensoniq DiskWriter package to
`C:\EXTRACTO`; its `setup.stx` lists the `.EDA`, `ede.exe`, and `extract.txt`
members and their expected sizes. The recovered files below match those
installer manifests.

| OS | Original installer (bytes; SHA-256) | Recovered file | Bytes | SHA-256 | Evidence / classification |
|---|---|---|---:|---|---|
| 1.61 | `asr161os.exe`; 255,393; `32109774e2b5cd808ebd6572122e77585cd67846c30f06543a6a80449643b680` | `asr161os.eda` | 168,449 | `ab540dc90ee1b11a47c009f16d71a9c6b0807f1c16fd670d42f716e1de7d99d9` | Installer manifest and header verified; sparse image expanded and validated |
| 3.53 | `asr353os.exe`; 295,581; `3276bf47f578398d608f2b33e8350f5db66f95c4ae62e8f8a13fbda38d44a571` | `asr353os.eda` | 275,457 | `d6ff7b93f2ad8f3a554597dba692e8627af4b7ecce8748abcf3c03b94380e7fd` | Installer manifest and header verified; sparse image expanded and validated |

Full SHA-256 values of every extracted member:

| Version | File | Bytes | SHA-256 | Finding |
|---|---|---:|---|---|
| 1.61 | `asr161os.eda` | 168,449 | `ab540dc90ee1b11a47c009f16d71a9c6b0807f1c16fd670d42f716e1de7d99d9` | ASR-10 DiskWriter EDA payload; converted to validated raw image |
| 1.61 | `ede.exe` | 61,632 | `907933e1317f32d13da3e6b2b43bbc4aca2492c2dd06c104bb1e79b92a254143` | DOS MZ Ensoniq Disk Extractor 1.0.9 utility |
| 1.61 | `extract.txt` | 690 | `aab3080717fab6f4bbd1b6a2fa94d2b9955c9300523472c172f110227c48d886` | DiskWriter instructions; says EDE writes/formats Ensoniq floppy media |
| 3.53 | `asr353os.eda` | 275,457 | `d6ff7b93f2ad8f3a554597dba692e8627af4b7ecce8748abcf3c03b94380e7fd` | ASR-10 DiskWriter EDA payload; converted to validated raw image |
| 3.53 | `ede.exe` | 61,632 | `907933e1317f32d13da3e6b2b43bbc4aca2492c2dd06c104bb1e79b92a254143` | Same Disk Extractor utility as 1.61 |
| 3.53 | `extract.txt` | 690 | `aab3080717fab6f4bbd1b6a2fa94d2b9955c9300523472c172f110227c48d886` | Same DiskWriter instructions |

The `.EDA` files begin with bytes `0D 0A 41 53 52 2D 31 30 20 44 69 73 6B`
(`CR LF ASR-10 Disk`). Their sparse block map was expanded with
`tools/asr10_eda_to_img.py`, following the documented Giebler format and
EpsLin Neo v1.58's independently available parser logic. Each reconstructed
image is exactly 3,200 × 512 bytes and passes the ASR ID/OS/root-directory
signature checks in `tools/asr10_disk_catalog.py`. They have not been booted
in OpenASR.

The two EDA files differ substantially: sizes are 168,449 and 275,457 bytes;
the first differing byte is offset 137; 136,119 bytes differ within their
common length and 302 of 329 complete 512-byte windows contain a difference.
These measurements establish version-specific payloads, not their internal
record or OS-code layout. The peculiar lengths (`512*n + 1`) are not evidence
of raw sector files.

Private paths:

- `research_local/asr10_firmware/asr161os.exe`
- `research_local/asr10_firmware/asr353os.exe`
- `research_local/asr10_firmware/recovered/161/`
- `research_local/asr10_firmware/recovered/353/`

`.gitignore` excludes `research_local/`; verify with `git check-ignore` before
adding any research artifacts.

## Boot ROM pair: expected fingerprints, not acquired

OpenASR's ROM README and pinned MAME driver specify two 131,072-byte images.
Neither file is present locally, so no acquisition or fingerprint match is
claimed.

| Chip/file | Expected CRC32 | Expected SHA-1 | Local status |
|---|---|---|---|
| lower `asr-648c-lo-1.5b.bin` | `8e437843` | `418f042acbc5323f5b59cbbd71fdc8b2d851f7d0` | Missing |
| upper `asr-65e0-hi-1.5b.bin` | `b37cd3b6` | `c4371848428a628b5e5a50e99be602d7abfc7904` | Missing |

MAME `esqasr.cpp` at commit
`8be57cdc5f44ab830dff44c06dd39f24190742d4` declares these filenames,
lengths, and fingerprints. OpenASR `roms/README.md` gives the same values.
Best reproducible lawful source is a dump from owned chips checked against
those references.

## OS images and effect media

| Version | Local file | Status |
|---|---|---|
| 1.61 | `asr161os.img` (private) | Reconstructed 1,638,400-byte raw ASR image; disk structure validated. SHA-256 `2a5cc161e80001daddf532914e80e854d3079f152197ab3def31055f239182f6` |
| 3.50 | `V350.img` absent | OpenASR documents this name, but the service manual says 3.50 ROMs are ASR-88-only; compatibility unresolved |
| 3.53 | `asr353os.img` (private) | Reconstructed 1,638,400-byte raw ASR image; disk structure validated. SHA-256 `335838dcf96dcec6e7fc2db4d671e2ac5075976999762beef74f472b439a98e9` |

The root directory has 14 populated entries in each image: entry 0 is type 32
(`ASR-10 OS`); entries 1–12 are ASR effect files (type 33); entry 13 is an
EPS-16 Plus effect file (type 24) named `PARALLEL EFX`. The OS file begins at
block 24 and is contiguous (173 blocks in 1.61, 382 in 3.53). Private extracted
OS object hashes are respectively
`86c933fa7e0a425ba4385b26065dc0cb312549bb9aa55b58e22c7cc58b639130` and
`2f9db68847002ce0af4c9ab4f364f12d27088ef1f02edc5c4971012f6cb5ee90`.

| Root entry | Type | Data blocks | 1.61 start | 3.53 start | Payload SHA-256 (same in both versions) |
|---|---:|---:|---:|---:|---|
| `44DDL+CHORUS` | 33 | 7 | 197 | 406 | `0b58e96ecaf251d3496f529d120537ba2262ddde3dd696d3c6f5ce594ef6d9c8` |
| `44DDL+CH+REV` | 33 | 15 | 204 | 413 | `8c571f395a72f8c539d4f76b757f63a217eca81f9dbb0f5a27ef3011cb7e70df` |
| `44DLYLFO+REV` | 33 | 10 | 219 | 428 | `3d168e430e42c9a32dd4f2504c86bdb9439767b4b6c2b3d5f6c22b3f1d74a5fe` |
| `44EQ+DDL` | 33 | 10 | 229 | 438 | `27af713a28e93880bd42840d04b67fe01f571443b897535d5abc7f95360d17fb` |
| `44EQ+DDL+CHO` | 33 | 15 | 239 | 448 | `810fd36a0907de24beab4b15f23ffdf40ae644eb528e42d0a90d820a4e578fdd` |
| `44EQ+REVERB` | 33 | 12 | 254 | 463 | `618c0c6d25ec9e476eaf43c84fd37a31284dcce3d77a212ac4a64df148e81c8c` |
| `44EQ+ROT+DDL` | 33 | 11 | 266 | 475 | `eb17859f1a5e3deeb9d3a54981210d39428a2237fe37ddccd8ba6b4139d26aa8` |
| `44LUSH PLAT2` | 33 | 8 | 277 | 486 | `4e7b8160918220c27fa45d807476f4fa350354209073d55644807a217e07d60f` |
| `44LUSH PLATE` | 33 | 8 | 285 | 494 | `7a3664920a47df87d15b0cf559cedb316d6f792da54b7db11fa16024f5b30c47` |
| `44PARAM EQ` | 33 | 6 | 293 | 502 | `5cdf8e88f505779a17a169d69768acb7a6916633b4f9d1aa19635dafe5f4b18e` |
| `44PERC PLATE` | 33 | 8 | 299 | 508 | `c29989762d0f1d1248c3d7462daa611aa2fbaeceab80ccd945190abab0145ac4` |
| `44ROTO+REVRB` | 33 | 11 | 307 | 516 | `8f8bdf017c323315e91b9d09c746ed4dd45d3e8a21e67cb04fe7a54b2d70059d` |
| `PARALLEL EFX` | 24 | 10 | 318 | 527 | `405d01f88a0229de11c151bc4d491589c30bd7b234d179b11f0b72807087de8e` |

All 13 effect object payloads are byte-identical across OS 1.61 and 3.53.
This is verified by matching per-object SHA-256 values. These are actual
firmware-disk effect objects, not inferred from preset names alone.

## Static ES5510 section evidence

The unofficial ESP1 guide describes a mixed effect object header with a
24-byte spaced `FxName` beginning at offset 10, `FxSize` at offset 34,
`FxUcode` at offset 38, `FxTable` at 40, `FxInitCode` at 42, and update-task
pointer at 44. The recovered effect files match that layout: names, version
field 5, sizes and in-range pointers are coherent. `FxUcode` points into each
object to sections which, when read as consecutive six-byte big-endian ES5510
words, contain between 82 and 103 candidate words and the ES5510 `END` ALU
opcode. All candidate counts fit the 160-word program memory. See
`tools/asr10_efe_inspect.py` for metadata-only statistics.

This is compelling static identification of factory ESP microcode sections,
but not yet a recovered-and-executed effect program in the strict runtime
sense: the exact section terminator, any bytes after the final aligned word,
GPR/delay-memory initialization and the actual firmware upload transaction
have not been correlated. The effect header's `FxInitCode`/update-task code is
host-side 68k code and has not yet been disassembled. Do not feed a candidate
section into the 5510 executor or label it fully validated until that setup is
recovered and MAME execution is compared.

No separate factory effect disk was acquired; the OS installer disks themselves
contain the 12 ASR effect objects and one EPS-16 Plus parallel-effects object
listed above. R-Massive also lists an ASR-10 Factory Bundle, but linked Mega
items have no clear redistribution license and were not downloaded. OpenASR
distributes no commercial media. The combined `ensoniqos.zip` endpoint
previously returned HTTP 404.

EpsLin Neo v1.58 source at commit
`cf20955e6f9f64dca3c06deaa220598e5fe1bd08` (GPL-2.0) documents an ASR raw
image layout of 1,638,400 bytes, 512-byte blocks, volume/OS/directory/FAT
structures, and ASR-10 Effect File directory type `0x21`. Its source served as
an independent structural cross-check; none of its code is copied here.

## Reproducible inventory command

```sh
python3 tools/asr10_firmware_inventory.py research_local/asr10_firmware
python3 -m unittest tools.tests.test_asr10_inventory -v
python3 tools/asr10_eda_to_img.py private.eda private.img
python3 tools/asr10_disk_catalog.py private.img --extract-dir /private/output
python3 tools/asr10_efe_inspect.py /private/output/*.payload.bin
```

The tools report metadata/hashes only and never execute inputs or print
proprietary payload bytes. Use private paths for extracted outputs.

## Sources

- [R-Massive ASR/EPS archive](https://zine.r-massive.com/ensoniq-asr-eps-archive/) — archive inventory only.
- [Ensoniqsamplers OS index](https://www.ensoniqsamplers.com/epsasr/026.html) — links Chicken Systems packages and lists OS versions.
- [OpenASR repository](https://github.com/peroberg/OpenASR) — ROM/media requirements.
- Ensoniq, *ASR Service Manual*, Software Notes pp. 25–26; [PDF](https://deepsonic.ch/deep/docs_manuals/ensoniq_asr_service_manual.pdf).
- [EpsLin Neo](https://github.com/handsthatstrike/EpsLinNeo) — GPL-2.0 disk/EFE structure reference.
- [Chicken Systems Ensoniq Disk Tools manual](https://www.chickensys.com/_docs/pdf_dt.pdf), Giebler Disk Image Format section — 512-byte header and sparse-block description.
- EpsLin Neo v1.58 source at commit `cf20955e6f9f64dca3c06deaa220598e5fe1bd08`, `EpsLin_v1.58.c` — independently cross-checked skip-map expansion, ASR geometry and EFE directory records. No EpsLin code is copied into this project.
- Unofficial [ESP1 Microcode Programming Guide](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/), effect-file header and ESP code sections — header field map; not a primary Ensoniq specification.
- Local installer `setup.stx`, `extract.txt`, and EDE strings, privately retained; hashes above identify extracted files.
