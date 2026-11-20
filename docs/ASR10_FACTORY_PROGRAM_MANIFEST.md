# ASR-10 factory effect-object manifest

Generated from the private recovered OS 1.61 and 3.53 effect-object directories
on 2026-11-20 with `tools/asr10_factory_manifest.py`. Machine-readable details
are in [ASR10_FACTORY_PROGRAM_MANIFEST.json](ASR10_FACTORY_PROGRAM_MANIFEST.json).
The JSON contains metadata and cryptographic hashes only; no firmware or
instruction bytes are included.

## Interpretation

**Verified static facts:** each of the 12 ASR factory effect objects has a
coherent EFE header; `FxUcode` points inside the declared `FxSize`; reading
from that offset in consecutive six-byte big-endian units yields 82–103 words
with `END` opcode fields; all regions fit the 160-word ES5510 program store.
All 12 object payload hashes match between OS 1.61 and 3.53.

All 12 headers also contain left/right output GPR indices `0x82`/`0x83` and
`FxNumVoices=1`. These are destinations recorded in the effect header, not
initial values for those GPRs. The machine-readable manifest preserves the
three bus labels as stored.

**Not verified:** the object range is not yet proven to equal the upload range.
`FxSize` is the declared logical object size. The residual 0–5 bytes after the
last complete six-byte unit are not explained. An `END` field does not alone
identify stored-program termination, and multiple END words occur in every
candidate. No factory transaction capture, complete GPR initialization, delay
RAM setup, or replay is available.

The `END` indices in the manifest are zero-based word positions from the
candidate `FxUcode` address. Field extraction follows the instruction layout
derived from pinned MAME source. Pinned MAME `es5510d.cpp` is a stub rather
than a disk-file disassembler; these are decoder statistics, not output from a
working upstream file-format disassembler.

## Candidate inventory

| Factory object | Extracted payload SHA-256 (same in both disks; includes disk-block padding) | FxUcode | FxSize | Words | Trailer bytes | END word indices |
|---|---|---:|---:|---:|---:|---|
| 44DDL+CH+REV | `8c571f395a72f8c539d4f76b757f63a217eca81f9dbb0f5a27ef3011cb7e70df` | `0x1abe` | `0x1d22` | 102 | 0 | 2, 32, 35, 44, 47, 50, 59, 80, 83, 86, 87, 88, 89, 101 |
| 44DDL+CHORUS | `0b58e96ecaf251d3496f529d120537ba2262ddde3dd696d3c6f5ce594ef6d9c8` | `0x0a3c` | `0x0c59` | 90 | 1 | 2, 8, 27, 29, 30, 32, 35, 38, 41, 44, 84, 87, 88, 89 |
| 44DLYLFO+REV | `3d168e430e42c9a32dd4f2504c86bdb9439767b4b6c2b3d5f6c22b3f1d74a5fe` | `0x109e` | `0x1309` | 103 | 1 | 2, 8, 9, 23, 35, 38, 44, 47, 50, 53, 62, 65, 68, 71, 74, 77, 83, 86, 89, 90, 101, 102 |
| 44EQ+DDL | `27af713a28e93880bd42840d04b67fe01f571443b897535d5abc7f95360d17fb` | `0x1000` | `0x120f` | 87 | 5 | 2, 5, 26, 53 |
| 44EQ+DDL+CHO | `810fd36a0907de24beab4b15f23ffdf40ae644eb528e42d0a90d820a4e578fdd` | `0x1a5e` | `0x1c5d` | 85 | 1 | 2, 11, 23, 33, 34, 35, 36, 38, 40, 41, 46, 65, 69, 70, 71, 80, 82, 83, 84 |
| 44EQ+REVERB | `618c0c6d25ec9e476eaf43c84fd37a31284dcce3d77a212ac4a64df148e81c8c` | `0x1578` | `0x17cd` | 99 | 3 | 2, 11, 14, 20, 23, 32, 43, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 57, 58, 61, 66, 68, 69, 70, 71, 72, 73, 74, 76, 87, 98 |
| 44EQ+ROT+DDL | `eb17859f1a5e3deeb9d3a54981210d39428a2237fe37ddccd8ba6b4139d26aa8` | `0x12c2` | `0x14e9` | 91 | 5 | 2, 5, 18, 29, 53 |
| 44LUSH PLAT2 | `4e7b8160918220c27fa45d807476f4fa350354209073d55644807a217e07d60f` | `0x0ca6` | `0x0ef5` | 98 | 3 | 2, 8, 11, 14, 17, 23, 26, 29, 38, 41, 44, 47, 68, 71, 77, 78, 79, 81, 83, 84, 85, 86, 88, 89, 97 |
| 44LUSH PLATE | `7a3664920a47df87d15b0cf559cedb316d6f792da54b7db11fa16024f5b30c47` | `0x0d42` | `0x0f90` | 98 | 2 | 2, 5, 8, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 97 |
| 44PARAM EQ | `5cdf8e88f505779a17a169d69768acb7a6916633b4f9d1aa19635dafe5f4b18e` | `0x09fa` | `0x0be7` | 82 | 1 | 80, 81 |
| 44PERC PLATE | `c29989762d0f1d1248c3d7462daa611aa2fbaeceab80ccd945190abab0145ac4` | `0x0d42` | `0x0f90` | 98 | 2 | 2, 5, 8, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 97 |
| 44ROTO+REVRB | `8f8bdf017c323315e91b9d09c746ed4dd45d3e8a21e67cb04fe7a54b2d70059d` | `0x1194` | `0x1401` | 103 | 3 | 2, 14, 21, 35, 38, 44, 47, 53, 56, 59, 62, 65, 68, 71, 74, 77, 83, 86, 89, 102 |

The two plate objects `44LUSH PLATE` and `44PERC PLATE` have the same candidate
offset, size, and instruction statistics but different extracted-payload hashes.
This does not establish that their DSP sections are identical; the manifest
intentionally hashes the whole proprietary object rather than publishing a
microcode-section digest. The SHA-256 values identify the extracted disk
payload files, including bytes after declared `FxSize` that pad their final
512-byte disk-block extent; `container_padding_bytes` records that count.

## Reproduce

```sh
python3 tools/asr10_factory_manifest.py \
  research_local/asr10_firmware/recovered/161/objects \
  research_local/asr10_firmware/recovered/353/objects \
  --output docs/ASR10_FACTORY_PROGRAM_MANIFEST.json
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
  tools.tests.test_asr10_factory_manifest \
  tools.tests.test_asr10_efe_inspect -v
```

The report can be regenerated only where the private files are lawfully
available. Do not commit those files or microinstruction contents.
