# OpenASR ES5510 transfer trace

## What is directly established by source

In OpenASR commit `241f9331477b07b072330ca6ebd2d7a2e2cc832e`,
`src/mame/ensoniq/asr10_boot.cpp` routes CPU writes as follows:

| CPU address | Wrapper | ES5510 host offset | Meaning in pinned MAME ES5510 |
|---|---|---:|---|
| `$FC3000–$FC303F` | direct host map | `$00–$3F` | byte latches/control/memory access |
| `$FC3100–$FC3101` | `es5510_host_read_select_w` | `$80` | read-select index |
| `$FC3140–$FC3141` | `es5510_host_write_select_gpr_w` | `$A0` | commit latched GPR/special register |
| `$FC3180–$FC3181` | `es5510_host_write_select_instr_w` | `$C0` | commit latched instruction |
| `$FC31C0–$FC31C1` | `es5510_host_write_select_gpr_instr_w` | `$E0` | combined instruction and GPR/special commit |

The source comment and `mc68302_pa_w` identify PA4 (PADAT `$FC6823`, bit 4) as
the modeled DSP RUN/HALT line. Low asserts halt; high releases it. The
configuration also calls `set_disable()` on the ES5510 device, so this wiring
does not mean the default scheduler is executing a loaded effect program.

## Historical OpenASR report, not independently reproducible in this checkout

Earlier local OpenASR investigation notes report a V3.50 firmware run with
combined commit traffic and later readback, including instruction-sequence
hashes for names reported as `44LUSH` and `ROM-01 HALL`. The notes say a
temporary Lua trace was used and that readback matched a previously captured
sequence. However, the raw tap logs, complete arrays, GPR snapshots and input
floppy image are not present here. Hashes alone cannot reconstruct a program.
These are **reported prior observations**, not new results from this branch,
and must be reacquired from a legally held image before program replay claims.

Further details in the current OpenASR investigation docs report that the
loader begins at `$F973F0`, chooses a host selector from a record type near
`$F97450`, and for type 1 writes GPR and instruction latches before a combined
commit at `$FC31C1`. The report records a 44LUSH first-END index `$3B` and
ROM-HALL first-END index `$58`, with committed words `$7A81FFFFF040` and
`$3662FFFFF040`, respectively. It includes hashes for each complete index/
word list, but the lists and captured GPR states are only in reduced logs
referenced under `/private/tmp/`; those logs are absent from this checkout.
See [the loader analysis](ASR10_FACTORY_LOADER_ANALYSIS.md) for exact hashes
and the limits on interpreting these results.

## Passive recorder

`tools/asr10_es5510_trace.lua` observes CPU program-space reads/writes in the
ES5510 windows and PA4 data-register writes. It prints `T551CAP` TSV rows with
sequence, address, data, mask, current PC, event class, source revisions and
emulated time. Tap handles remain rooted for the life of the script. It does
not modify host or DSP state. The PC is contextual evidence only; host
address-space writes are the event of interest.

Run from an OpenASR build with a user-supplied OS floppy and matching ROMs,
using MAME's Lua autoboot facility, for example:

```sh
./mame asr10booth -rompath roms -flop1 floppies/asr10booth/V350.img \
  -autoboot_script /absolute/path/to/tools/asr10_es5510_trace.lua \
  -console > /private/path/asr10-es5510-tap.tsv 2>&1
```

Exact command-line spelling can vary with the OpenASR/MAME build. Confirm Lua
support with that binary's `-showusage` and verify the metadata banner appears.
Do not add the OS image, logs containing programs, reconstructed JSON, or
rendered dumps to Git. The recorder does not yet capture a physical board; it
records MAME's emulated CPU transactions.

## Current execution result

No new firmware boot or transaction trace was run in this pass: OpenASR's
documented floppy/ROM directories contain no images and the local machine has
no user-provided ASR-10 OS image. Therefore the current branch has **zero
firmware-generated capture events**. The synthetic unit test is only a parser
test, explicitly tagged as synthetic and never accepted as an authentic
program.
