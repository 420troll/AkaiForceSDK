# ASR-10 factory effect loader — current call/data-flow evidence

## Historical executed-firmware report

OpenASR's committed investigations report a real V3.50 boot and runtime effect
selection using an ASR-10 OS floppy image. This pass inspected those reports
at pinned OpenASR source commit `241f9331477b07b072330ca6ebd2d7a2e2cc832e`;
it did **not** reproduce the run because the media image and reduced trace logs
are no longer present. Treat the detailed numbers below as **prior project
runtime evidence documented in OpenASR**, not new measurements from this branch.

References inside OpenASR:

- [`es5510-effect-upload-v350.md`](https://github.com/peroberg/OpenASR/blob/241f9331477b07b072330ca6ebd2d7a2e2cc832e/docs/asr10/investigations/es5510-effect-upload-v350.md)
- [`es5510-upload-executable-contract-v350.md`](https://github.com/peroberg/OpenASR/blob/241f9331477b07b072330ca6ebd2d7a2e2cc832e/docs/asr10/investigations/es5510-upload-executable-contract-v350.md)
- [`effect-rate-state-provenance-v350.md`](https://github.com/peroberg/OpenASR/blob/241f9331477b07b072330ca6ebd2d7a2e2cc832e/docs/asr10/investigations/effect-rate-state-provenance-v350.md)

These are research reports committed by the OpenASR project, not Ensoniq
primary documentation. The underlying raw runtime logs remain outside that
repository.

Those reports describe a V3.50 firmware run where the ASR CPU writes ES5510
host latches, commits through the combined `$E0` path, and performs explicit
host readback. They record 822 readback bytes matching expected firmware data
`D2`, with zero mismatches. This validates upload into the **OpenASR/MAME host
device model**, not physical hardware.

## Loader and effect selection call/data flow

```text
selected firmware effect object / record stream
  -> OS/ROM-side 68k loader at $F973F0 (reported V3.50)
  -> parser cursor $0E7E; selected transfer object base $0E8E
  -> record-type branch near $F97450
  -> latch writes in $FC3000-$FC303F
  -> selector/commit windows in $FC3100-$FC31C1
  -> OpenASR wrapper -> generic ES5510 host offset $80/$A0/$C0/$E0
  -> modeled instruction/GPR arrays
  -> $80 read-select + latch-byte reads
  -> firmware compares host bytes near $F97574-$F9757A
```

The specific record type 1 upload described by the report is:

| Role | ASR CPU writes | ES5510 generic host model |
|---|---|---|
| GPR latch, 24 bits | `$FC3001,$FC3003,$FC3005` | offsets `$00,$01,$02` on the low-byte map |
| instruction latch, 48 bits | `$FC3007,$FC3009,$FC300B,$FC300D,$FC300F,$FC3011` | offsets `$03..$08` |
| combined commit | `$FC31C1 <- index` | offset `$E0`, index selected by data byte |
| verify select | `$FC3101 <- index` | offset `$80` |
| verify read | `$FC3001...` | GPR/instruction latch bytes reloaded by read-select |

The instruction is therefore **paired with GPR data at upload time** in the
observed type-1 transaction. That is a runtime host-transfer representation;
it does not prove that the source disk object uses fixed nine-byte records.
Firmware may parse headers, mode-specific records or other data before
emitting these bus writes.

Other record types reportedly select `$C0` and `$A0` commit paths. The full
branch table and all source byte reads must be reacquired from an actual OS
image before treating type codes or source layout as exhaustive.

## Initialization and bounded program observations

The prior V3.50 runtime report also records:

- HALT-enable writes `$FC303E <- $02` and later `$FF` at startup/loader PCs;
- Host Control writes `$FC3024 <- $03` and `$02`;
- Host Serial Control `$FC3030 <- $48` before the upload;
- combined commits at indices `$00..$0B` with instruction latch value
  `$FFFFFFFFFFFF` to initialize those locations to generic `END` words;
- selected ROM HALL object base `$FFF9B626` and 44LUSH object base `$0062B600`;
- ROM HALL first observed END at index `$58`, committed word
  `$3662FFFFF040`, list hash
  `fa65ea95fab476bf2da29b66bdd6aeb7995319495701c70c565b08aa06431004`;
- 44LUSH first observed END at index `$3B`, committed word
  `$7A81FFFFF040`, list hash
  `35810521e01b2168ac528ae1c7fd228368aeb0313ebc877f8a11d3a5962c3770`.

The OpenASR report says the full instruction index/word lists were byte-for-byte
identical across repeated A/B selections. It does not include the complete
word arrays or captured GPR arrays. The corresponding raw observations were
kept at paths such as `/private/tmp/asr10-es5510-program-aba-r2.log` and
`/private/tmp/asr10-es5510-contract.log`; searches of current `/tmp`,
`/private/tmp`, and home paths did not find them. A SHA-256 of a proprietary
instruction list is not enough to recreate it.

The “first END” is a boundary in the generic MAME interpretation; it is not a
physical proof of ASR frame scheduling, HALT timing or factory effect sound.
The ASR machine calls `set_disable()` to remove the ES5510 from normal
scheduler execution, while `esqpump` calls `run_once()` for each sample when
released and built with real processing. The former does not by itself imply
the loaded program is never executed. The pump's serial policy is still
provisional, and the historical routing spikes were not physical board
measurements.

## Effect parameters and reconfiguration

The historical report verifies two different effect objects loaded through
the same host uploader, followed by firmware reads from the selected object
and an ES5506 ACTV write `$1F -> $17 -> $1F`. The object-to-ACTV relation is
reported as runtime dataflow. It does **not** yet identify the upstream named
descriptor field supplying the rate/voice setting or a parameter-to-GPR map
for reverb decay/mix. The report says the loader cursor/base fields are
provenance pointers and explicitly leaves object fields such as `+0x08`
unnamed. Do not interpret those as algorithm indices or DSP coefficients.

## Version coverage

Only V3.50 is covered by the historical loader trace reports reviewed here.
OS 1.61 and 3.53 installers have since been executed in isolated Wine
containers; their EDA images were expanded and cataloged. The two disks
contain byte-identical ASR effect objects, including candidate FxUcode
sections, while their OS objects differ. See
`ASR10_FIRMWARE_EXTRACTION_REPORT.md` and
`ASR10_FIRMWARE_INVENTORY.md`. No cross-version loader comparison or
firmware-to-host-commit trace has yet been performed. V3.50 ASR-10
compatibility remains unresolved: OpenASR documents it as a baseline, while
the ASR service manual says V3.50 ROMs are for ASR-88 only.
V3.50 ASR-10 compatibility is also unresolved: OpenASR documents it as a
baseline, while the ASR service manual says V3.50 ROMs are for ASR-88 only.

## Recovery status

| Milestone | Status |
|---|---|
| Firmware CPU-to-ES5510 host transaction | Reported and source/model-correlated in prior OpenASR runtime notes |
| Authentic transfer triggered by firmware | Reported, not reproduced in this pass |
| Host readback verifies stored data | Reported 822/822 matches in prior run |
| Two selected program boundaries/end words and hashes | Reported; full arrays missing |
| Complete instruction words and per-index GPR initialization | Not available in current workspace |
| Special-register, external delay RAM and full lifecycle capture | Incomplete/unavailable |
| Reconstructed program replayed in this repository's 5510 executor | No |
| Same program compared against an executing MAME ES5510 | No reproducible arrays/logs |
| Authentic factory algorithm recovered to success criterion | **No** |

The most productive next step is to acquire the exact OS image used by the
historical OpenASR V3.50 run (or a lawful 1.61/3.53 image), acquire matching
boot ROM dumps from owned hardware, and rerun the existing passive recorder
with full event logging. For the previous V3.50 result, request/restore the
reduced logs and exact media hashes from the OpenASR research author if still
available; otherwise rerun and regenerate them. Do not substitute the two
reported program hashes or END words for missing instruction/GPR contents.
