# DP/4 ROM forensics — interrupted-session resume audit

Date: 2026-11-20
Branch: `issue11-dp4-rom-forensics`
Starting commit: `93bd52e` (`Trace DP4 bank selection and define capture milestone`)

## Decision

The existing DP/4 research is preserved, but the private DP/4 U54/U56 images
and their old Ghidra project were not found in the paths available in this
session. The current container has ASR-10 ROMs and an OpenASR/MAME build, not
DP/4 firmware. Therefore no new DP/4 instruction bytes, loader routine, or
factory effect were recovered. Do not treat ASR-10 findings as DP/4 evidence.

The strongest DP/4 result remains the verified U56-to-U54 revision-tag read
and bank-selection data flow documented in the existing reports. The unresolved
bridge is still a selected U54 source read correlated to an ES5510 host
instruction/GPR transaction. The recommended next decisive evidence remains a
synchronized source/destination hardware trace; details are in
`DP4_EXTRACTION_FEASIBILITY_DECISION.md`.

## Workspace preservation audit

| Location | Observed state | Handling |
|---|---|---|
| `/mnt/c/Users/Gamer/TROLLfx-Force-SDK` | Main worktree on `main`, ahead of its remote by four commits and with user changes in DSP/documentation/artifact paths | Left untouched |
| `/mnt/c/Users/Gamer/research/trollfx-asr10` | Separate ASR-10 research worktree; resumed findings committed and pushed as `491dccf42965e586fe52d88eddc8e47d70896c46` | Preserved independently; not merged into DP/4 branch |
| `/mnt/c/Users/Gamer/research/trollfx-dp4` | Dedicated worktree on `issue11-dp4-rom-forensics`, starting at `93bd52e` | Used for this DP/4 resume report and headless runner fix |
| Docker `trollfx-research` | Running Ubuntu 24.04 container with OpenASR source, ASR boot ROMs mounted read-only, private run logs and Ghidra/MAME tools | No DP/4 ROMs found in its mounts; private contents remain outside Git |
| `/tmp/dp4-rom-forensics`, `/private/tmp/dp4-rom-forensics`, `/home/gamer/dp4-rom-forensics`, `/mnt/c/Users/Gamer/research/dp4-rom-forensics` | Absent | No claims revalidated from these old private paths |

A filename search under Downloads and research found no
`DP4_V106_ucode.bin`, `DP4_V115_ucode.bin`, `DP4_V106_os.bin`, or
`DP4_V115_os.bin`. Two files whose names include “DP4 TeleBrain” are present,
but they are not ROMs by filename and were not treated as firmware. Existing
ROM hashes and conclusions below are carried forward from the committed
reports, not freshly recomputed from local binaries.

No tracked or untracked file from the main worktree was moved, reset, or
overwritten. The ASR-10 and DP/4 work remain on separate branches/worktrees.

## Findings carried forward from DP/4 reports

These conclusions are reproduced here as summaries of the existing reports;
the private EPROM files were not available for another independent check.

| Topic | Evidence and status | What it does not prove |
|---|---|---|
| Host CPU | The base DP/4 service manual and Rev J main-board schematic identify U52 as a 6803. | It does not establish exact silicon suffix or reset strap voltages. Do not use the DP/4+ 63803 design as base-model evidence. |
| U56 mapping | Rev J schematic maps the 32 KiB U56 OS ROM to `$8000–$FFFF`; reset vector at file offset `0x7FFE` resolves to `$84E4`. The prior U56 analyses used Ghidra MC6801 language for the 6803-family code. | The mapping does not locate an ESP loader by itself. |
| U54 banked read | U56 chooses bank 9 through `$0600`, reads `$2000`, and checks the revision word at U54 physical offset `0x12000`: v1.06 tag `0x0106`, v1.15 tag `0x010F`. | A tag check is not an algorithm fetch or a microprogram transfer. |
| Runtime bank table | Prior cross-version analysis resolves a four-slot descriptor table and helper writes to `$0600`; values include paired banks `(0,0x0B)`, `(0,0x0B)`, `(1,0x0C)`, `(2,0x0D)`. | Assigning slots to ESP A/B/C/D and interpreting pairs as program/configuration banks remains a hypothesis. |
| Possible computed U54 call | Ghidra reports `$82C9: JSR 0,X` with candidate target `$30EC`; X is loaded from a runtime object. | Its object origin and selected bank are unknown. This is not an observed instruction fetch. |
| `$1800/$1801` path | Rev J maps it to U59 6850 ACIA; previous candidates for the ESP sender were rejected. | It is not evidence of the ESP host interface. |
| Four ESPs | Schematic labels four ES5510 devices and independent U57 HALT outputs; actual absolute CPU decode equations remain incomplete. | No per-unit host transactions have been captured. |
| U54 algorithm/name records | Three stable label-bearing record geometries and 50 entries in each were cataloged. Records contain bytes beyond names. Six simple raw-48-bit layouts did not yield credible END-delimited programs. | Pointer/size schema, instruction packing, algorithm/GPR tables, and loader transforms are not identified. |
| ES5510 transfer | The pinned MAME source documents generic host latch/commit semantics; no DP/4 system driver exists in the source checked for this project. | Device support is not complete DP/4 machine support, and MAME does not reveal U56 transactions. |

See `DP4_ROM_MICROCODE_FORENSICS.md`, `DP4_BANKED_ACCESS_TRACE.md`,
`DP4_U56_LOADER_ANALYSIS.md`, `DP4_HARDWARE_ARCHITECTURE_AUDIT.md`, and
`DP4_EXTRACTION_FEASIBILITY_DECISION.md` for the detailed prior evidence,
offsets, confidence, and rejected hypotheses.

## Available MAME/OpenASR work

The live `trollfx-research` container holds OpenASR commit
`241f9331477b07b072330ca6ebd2d7a2e2cc832e` and a diagnostic MAME 0.288 build.
The ASR-10 target build command completed with exit status 0 on 2026-11-20:

```sh
cd /src
make SOURCES=src/mame/ensoniq/asr10_boot.cpp USE_QTDEBUG=0 NO_X11=1 \
  NO_OPENGL=1 NO_USE_MIDI=1 NO_USE_PORTAUDIO=1 NO_USE_PULSEAUDIO=1 \
  NO_USE_PIPEWIRE=1 CXXFLAGS=-std=c++20 NOWERROR=1 -j4
```

`/src/mame -verifyroms asr10booth -rompath /src/roms` reports the ASR-10 ROM
set good. The machine list includes Ensoniq ASR-10 and standalone ES5510
device support, but no DP/4 machine driver. This is useful for ASR host-protocol
tests only; it cannot execute DP/4 U56/U54 code. ASR-10 boot traces, including
blank ES5510 memory initialization and the absence of floppy-register traffic,
are documented separately in
[`ASR10_BOOT_ROM_RUNTIME_CHECK.md`](ASR10_BOOT_ROM_RUNTIME_CHECK.md) on the
ASR branch.

## Headless Ghidra/JDK repair

The research container has Ghidra `12.1.3_PUBLIC` and OpenJDK 21. The standard
Ghidra MC6800 module contains a 6809 language with external-name aliases for
6800/6801/6803; those aliases are not sufficient reason to use the 6809
semantics for a DP/4 6803. The Apache-2.0
[GhidraMC6800 extension](https://github.com/sigurasg/GhidraMC6800) provides
separate `MC6801:BE:16:default` and `HD6301:BE:16:default` languages. Its
published v1.2 asset for Ghidra 12.1.3 has SHA-256
`51ba635a18d87f21a3cb06e12cec2360d5fdc246e9727525f3fd225b8c7a73e0` and was
installed only into the private research container's Ghidra Extensions
directory; no extension binary or private ROM was added to Git.

`tools/ghidra_headless.sh` now resolves `JAVA_HOME` from `javac` when it is not
set, validates a JDK, and executes `support/analyzeHeadless` directly. This
avoids the interactive JDK chooser. `tools/dp4_ghidra/run_u56.sh` delegates to
this wrapper, so callers may set `GHIDRA_INSTALL_DIR` (or `GHIDRA_HOME`) and
leave `JAVA_HOME` unset if the JDK's `javac` is on `PATH`.

Headless validation was performed against only a synthetic 32 KiB fixture:

```sh
GHIDRA_INSTALL_DIR=/opt/ghidra/ghidra_12.1.3_PUBLIC \
JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64 \
python3 -m unittest experimental.emulation.capture.tests.test_dp4_ghidra_smoke -v
```

Result: one test passed; the report identified `MC6801:BE:16:default`, decoded
the synthetic reset vector at `$FFFE` to `$8000`, and disassembled the synthetic
entry point. No DP/4 firmware was loaded. A separate wrapper `--help` check
with `JAVA_HOME` unset printed headless usage and no picker; `analyzeHeadless`
returns status 1 for its help/usage path.

## Evidence still needed for authentic DP/4 effects

At least one of the following is needed to resume ROM-specific static work:

1. Restore the four previously analyzed private files (U54/U56, v1.06 and
   v1.15) plus their acquisition hashes; or reacquire matching dumps and run
   `tools/dp4_ghidra/verify_u54_tag.py` before analysis. The current docs retain
   prior hashes but not the binary data.
2. Restore the old Ghidra projects/reports only as convenience; fresh raw ROM
   imports are reproducible once the images and the extension are available.
3. For an end-to-end claim, capture one real algorithm change with a common
   timebase: U54 address/data/chip-select and U57 bank lines on the source
   side; selected ES5510 host address/data/strobes, CS, HALT and reset on the
   destination side. Add serial clocks/data and board revision metadata if
   possible. Capture reset, select a simple one-unit algorithm, then change
   one parameter and repeat for determinism.
4. Reconstruct source-to-instruction transformation, full instruction words,
   GPR/table initialization, delay-memory setup, serial route, and halt/run
   order. Validate those transactions against the pinned MAME ES5510 host
   model and a real DP/4 output before calling the program recovered.

The first two allow static review; only synchronized source/destination
observation or an equivalent verified firmware-to-host dataflow closes the
current proof gap. Keep all proprietary ROM and program bytes private.

## Commands and test results

On this branch, after the JDK wrapper/runner changes:

```sh
sh -n tools/ghidra_headless.sh
python3 -m unittest discover -s experimental/emulation/capture/tests -p 'test_*.py' -v
git diff --check
```

The synthetic Ghidra smoke test was separately run with the required
Ghidra/JDK environment and passed. The ROM scanner/boundary tests do not require
private firmware. No test in this resume report claims to authenticate a DP/4
factory program.

## Scope boundary

No plugin source, Force installation, or unrelated project was changed. The
ASR-10 firmware and OpenASR observations remain a separate research track.
The DP/4 path remains at: verified banked U54 access, unresolved algorithm
loader/ES5510 destination dataflow, and no factory microprogram execution.
