# ASR-10 firmware and effect-media inventory

Checked on 2026-11-19. Proprietary and downloaded binaries reside only in the
gitignored `research_local/` directory. No payload is committed.

## Acquired local files

These two files were fetched as inert data from the Chicken Systems URLs linked
by the Ensoniqsamplers OS index. They were never executed. `file` identifies
both as 16-bit Windows NE executables; strings expose a SetupMN archive and
setup components. The installer archive was **not** extracted, so OS disk
payloads/member names are not verified. SHA-256/SHA-1 values below identify the
downloaded wrappers only, not the enclosed Ensoniq OS data.

| Local path | Source | Bytes | CRC32 | SHA-1 | SHA-256 | Result |
|---|---|---:|---|---|---|---|
| `research_local/asr10_firmware/asr161os.exe` | `https://www.chickensys.com/bin/asr161os.exe` (linked from `ensoniqsamplers.com/epsasr/026.html`) | 255,393 | `e7dfa592` | `023d7e73af4753c0734e6c33756afcd9187b17ea` | `32109774e2b5cd808ebd6572122e77585cd67846c30f06543a6a80449643b680` | Present; NE container; OS member not extracted |
| `research_local/asr10_firmware/asr353os.exe` | `https://www.chickensys.com/bin/asr353os.exe` (same index) | 295,581 | `48a5047c` | `8c37d5d01bdca17d4475ac6ef73aebb169106c36` | `3276bf47f578398d608f2b33e8350f5db66f95c4ae62e8f8a13fbda38d44a571` | Present; NE container; OS member not extracted |

Both have an NE header at file offset `0x100`, four segments, alignment shift
8, and strings referencing `ASETUP.EXE`, `SETUPMN.ARV`, `SETUPMN.EXE`,
`CTL3DV2.DLL`, `asr161os.eda` or `asr353os.eda`, `ede.exe`, and `remove.exe`.
These are embedded filename strings, not a verified member listing. The first
bytewise difference is at `0x4f1c`; only four bytes differ in the
`0x4000–0x4fff` 4 KiB block. Subsequent 4 KiB blocks from approximately
`0x1e000` differ heavily. This supports that the wrappers
share a setup front end and contain version-specific payload data; it does not
identify the OS bytes or their archive framing. No standard ZIP signature was
found. A compatible non-executing extractor for this old NE/ARV package is
still needed.

## Boot ROM pair: expected fingerprints, not acquired

OpenASR's ROM README and pinned MAME driver both specify two 131,072-byte
images. Neither file is present locally, so **no ROM acquisition or local
fingerprint match is claimed**.

| Chip/file | Expected CRC32 | Expected SHA-1 | Local status |
|---|---|---|---|
| lower `asr-648c-lo-1.5b.bin` | `8e437843` | `418f042acbc5323f5b59cbbd71fdc8b2d851f7d0` | Missing |
| upper `asr-65e0-hi-1.5b.bin` | `b37cd3b6` | `c4371848428a628b5e5a50e99be602d7abfc7904` | Missing |

MAME `esqasr.cpp` at commit
`8be57cdc5f44ab830dff44c06dd39f24190742d4` declares the filenames, 0x20000
lengths and CRC/SHA-1 values. OpenASR `roms/README.md` gives the same expected
hashes. The R-Massive archive lists ASR-10 1.50b EPROMs, but its link is a
third-party Mega archive and does not establish redistribution permission;
we did not download those ROMs. Best reproducible lawful source is a user
dump from owned chips compared against the MAME/OpenASR fingerprints.

## OS image inventory

| Version | Local image | Source inventory | Status |
|---|---|---|---|
| 1.61 | `V161.img` absent | Chicken Systems NE download wrapper acquired (above); OpenASR documents `V161.img`. | Wrapper present; disk image not extracted |
| 3.50 | `V350.img` absent | OpenASR documents this filename; no corresponding direct vendor download found in the cited index. | Missing; compatibility must be resolved (see below) |
| 3.53 | `V353.img` absent | Chicken Systems NE download wrapper acquired; R-Massive lists an OS 3.53 bootable CD image. | Wrapper present; disk image not extracted |

The combined `ensoniqos.zip` endpoint linked by the OS index returned HTTP
404 during this pass. The direct 1.61 and 3.53 `.exe` endpoints returned
HTTP 200 and were saved above. The R-Massive Mega links did not expose direct
download metadata to the browser and were not used.

Important source discrepancy: OpenASR documents V3.50 as its boot baseline,
but the ASR Service Manual says “Version 3.50 ROMS are for the ASR-88 only”
(Software Notes, p.25). The 3.50 compatibility claim is therefore unresolved
for an ASR-10 and must not be accepted from the emulator README alone. The
same manual says V3.00 is an ASR-10 OS and requires 1.50 ROMs; the
Ensoniqsamplers OS index identifies 1.61 and 3.53 as ASR releases. Use 1.61 or
3.53 for initial ASR-10 boot work unless OpenASR's exact V3.50 build/test
provenance resolves the discrepancy.

OpenASR's `floppies/` and `roms/` directories contain README files only. Its
`samples/` directory holds CC0 floppy-mechanism WAVs, not firmware/effects.
There are no `V161.img`, `V350.img`, `V353.img`, EFE, EDA, HFE or boot ROM files
in that checkout.

## Factory effects and sound disks

R-Massive's archive page lists an “ASR-10 Factory Bundle” with Essential Sound
Disks AD-001 through AD-008, including a “Demo Effects” item, plus OS images
and the ASR-10 1.50b EPROM pair. These linked items are Mega-hosted; no clear
redistribution license is shown on the page. No factory sound disk or effect
file was downloaded or locally found. `OpenASR` explicitly says commercial
media is not distributed and must come from legally obtained physical media
or authentic backups.

The EpsLin Neo v1.58 source at commit
`cf20955e6f9f64dca3c06deaa220598e5fe1bd08` is GPL-2.0. Its source documents
ASR raw image size 1,638,400 bytes, 512-byte blocks, a volume ID block, OS
blocks, two directory blocks, and FAT-linked file extents; it recognizes
ASR-10 Effect File directory type 33 (`0x21`). It can extract EFEs from lawful
user images without writing the source code into this project. This directory
structure does not itself prove that an EFE's microcode section is uncompressed
or immediately executable.

## Reproducible inventory command

```sh
python3 tools/asr10_firmware_inventory.py research_local/asr10_firmware
```

The tool reports sizes, CRC32, SHA-1/SHA-256, known ROM fingerprint matches,
NE headers, referenced component names, ZIP member names and expected-but-
missing standard media filenames. It never executes archives and does not
print binary payload bytes. `research_local/` is ignored by Git.

## Sources

- [R-Massive ASR/EPS archive](https://zine.r-massive.com/ensoniq-asr-eps-archive/) — archive inventory only; linked downloads not acquired.
- [Ensoniqsamplers OS index](https://www.ensoniqsamplers.com/epsasr/026.html) — links the two Chicken Systems packages and lists OS versions.
- [OpenASR repository](https://github.com/peroberg/OpenASR) — pinned local source revision and documented ROM/media requirements.
- Ensoniq, *ASR Service Manual*, Software Notes pp.25–26 and V3.00 notes; [PDF](https://deepsonic.ch/deep/docs_manuals/ensoniq_asr_service_manual.pdf).
- [EpsLin Neo](https://github.com/handsthatstrike/EpsLinNeo) — GPL-2.0 ASR disk/EFE structure reference; source inspected at pinned commit above.
