# ASR-10 boot-ROM and headless analysis runtime check

Date: 2026-11-20
Scope: private ASR-10 v1.5b boot ROMs and OS 1.61/3.53 boot attempts in
OpenASR. This is adjacent Ensoniq evidence and must not be treated as DP/4
firmware evidence. DP/4 U54/U56 images are not present in this environment;
see the DP/4 forensic reports for their last verified conclusions.

## Result

The verified ASR-10 ROM pair is accepted by the built OpenASR/MAME executable.
With the unmodified pinned OpenASR ROM declarations, CPU-space reads during
reset decode the initial vectors as stack pointer `0x00000300` and program
counter `0x0000000C`. This corrects a previous static lane-order conclusion:
the temporary ROM-offset swap produced `0x00000003` / `0x00000C00` and was
reverted. The test used passive CPU-space taps and did not alter ROM contents.

The runtime reaches code in the boot ROM and produces ES5510 host transactions,
but it does **not** boot the inserted OS disk or select an effect in the
observed 30-second run. No floppy-controller register accesses were recorded.
The boot routine clears/initializes the modeled ES5510 instruction memory,
then holds the DSP halted before any factory object is transferred. This is
not a factory-program capture.

## Private inputs and integrity

The untouched files are kept in the user-approved private PC backup and in a
read-only container mount. They are excluded from Git.

| Input | Bytes | SHA-256 | Identity check |
|---|---:|---|---|
| ASR-10 low ROM `asr-648c-lo-1.5b.bin` | 131,072 | `94312f142db5b307bf31fbcc9b46e54197460f691596465d24870aa579a892fc` | CRC-32 `8e437843`, SHA-1 `418f042acbc5323f5b59cbbd71fdc8b2d851f7d0` |
| ASR-10 high ROM `asr-65e0-hi-1.5b.bin` | 131,072 | `d3f4863d74b44e85de69432e1618dc9b1df7dcdefe7c79f81af2b7990e0e16eb` | CRC-32 `b37cd3b6`, SHA-1 `c4371848428a628b5e5a50e99be602d7abfc7904` |
| ASR-10 OS 1.61 raw image | 1,638,400 | `2a5cc161e80001daddf532914e80e854d3079f152197ab3def31055f239182f6` | Filesystem and directory signatures validated |
| ASR-10 OS 3.53 raw image | 1,638,400 | `335838dcf96dcec6e7fc2db4d671e2ac5075976999762beef74f472b439a98e9` | Filesystem and directory signatures validated |

The downloaded OpenASR source archive is commit
`241f9331477b07b072330ca6ebd2d7a2e2cc832e`, SHA-256
`513223705bb446672061d502a67f89f736a2a7cd6887cbc2b4cebfb3d0762fa8`.
The MAME ES5510 comparison reference is commit
`8be57cdc5f44ab830dff44c06dd39f24190742d4`. The OpenASR execution core is
locally modified; its four file hashes and limitations are recorded in
`ASR10_ES5510_ARCHITECTURE.md`.

## Headless Ghidra configuration and analysis

The research container has Ghidra `12.1.3_PUBLIC` and OpenJDK 21 installed.
The release archive was verified against the publisher checksum
`93a5d11a9ad510622acaaf908c556a7b9b764d338e78a7567f3689bf5081fd54`.
`tools/ghidra_headless.sh` sets `JAVA_HOME` from `javac` when possible and
exits with a useful error if a JDK is unavailable. It calls
`support/analyzeHeadless` directly, so no interactive JDK chooser is shown.
Validation command (run inside the research container):

```sh
JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64 \
GHIDRA_HOME=/opt/ghidra/ghidra_12.1.3_PUBLIC \
PATH=/usr/lib/jvm/java-21-openjdk-amd64/bin:$PATH \
  tools/ghidra_headless.sh --help
```

The command printed Headless Analyzer usage without prompting. A private
Ghidra raw-binary analysis imported the interleaved diagnostic image as
Motorola 68000 big-endian. It decoded the reset code and the OS/ESP startup
routine around image offset `0x17342` (runtime alias printed by the script as
`0xF97342`). The script's alias is a mapping hypothesis used for readable
cross-reference; verify actual ROM decode before treating it as an electrical
address.

Selected disassembly observations (file offsets are within the interleaved
diagnostic image):

| File offset | Printed runtime alias | Observed operation | Confidence / limit |
|---:|---:|---|---|
| `0x17354` | `0xF97354` | Writes selector `0x9F` to combined-commit offset `+0x1C0` from the host base pointer | Strongly indicates initializing a special GPR/instruction index; the base pointer's physical decode follows OpenASR model |
| `0x17360–0x17380` | `0xF97360–0xF97380` | Loads values `0x00FF00`, `0xFFFFFF`, `0x0F0000`, then commits selector values `0xF4`, `0xF5`, `0xF8` through the GPR commit aperture | Verified instruction bytes; semantic register names derive from MAME/OpenASR model, not the DP/4 |
| `0x17386–0x17392` | `0xF97386–0xF97392` | Writes host-control values `3`, `2`, and serial-control `0x48` | Verified code; model-specific interpretation |
| `0x1739A–0x173B4` | `0xF9739A–0xF973B4` | Writes `0xFF` to latch offsets at a two-byte stride, then iterates commit selectors from 0 through `0x9F` | Matches observed 160-word blank instruction-memory initialization sequence |
| `0x173B6–0x173DC` | `0xF973B6–0xF973DC` | Writes `0xFFFFFF` to selected GPR and zeroes selected indices `0xEA–0xF1` | Matches observed boot-time host writes; not factory parameter initialization |
| `0x173F0` onward | `0xF973F0` onward | Stores A3 at RAM pointer `0x0E7E`, calls helpers, and begins parsing a record stream | Candidate loader/parser entry only; runtime did not reach a factory transfer |

The code is executable 68000-family disassembly in an ASR-10 image, not DP/4
6803 code. It demonstrates that OpenASR's ASR boot firmware has an ESP host
initialization path. It says nothing about DP/4 U56's transfer format or bus
addresses.

## Runtime observations

OpenASR/MAME was built in the persistent `trollfx-research` container from
the pinned source archive. Build dependency and linker fixes were local to the
container; no source under the repository was replaced by the private MAME
working tree. The final binary is a diagnostic build with a PA4 callback log,
SHA-256 `2efca4267006da9d05bafddb86b666570b08cd5f7bfcecce36e6e4f2d94531e6`.
It identifies as MAME `0.288 (unknown)` because the source archive had no Git
metadata. `-verifyroms asr10booth -rompath /src/roms` reported the ROM set
good. SDL dummy video/audio were used; these runs are not hardware or audio
validation.

For both OS images the passive Lua trace recorded 293 events during the
30-second run: 87 boot-ROM reads, 30 direct host writes, 161 combined
instruction/GPR commits, 12 GPR-only commits, and 3 host reads. It recorded
zero accesses to the mapped floppy-controller window and no OS-disk-sector
transfer. These counts describe taps from this machine model/run only.

Of the 161 combined commits, one early commit targets index `0x9F` before the
latch-clear loop; its latched bytes are not established by the passive trace.
The following 160 commits are the boot blanking loop: instruction indices
`0x00..0x9F` each receive six `0xFF` bytes (`0xFFFFFFFFFFFF`). MAME decodes
these as END opcodes. Do not call these a factory algorithm. The other
observed initialization writes were:

| Register/index as modeled | Value | Evidence |
|---|---:|---|
| `0xF4` (modeled DIL/MEMSIZ slot) | `0x0000FF` | Combined transaction capture |
| `0xF5` (modeled DLENGTH slot) | `0xFFFFFF` | Combined transaction capture |
| `0xF8` (modeled DBASE slot) | `0x000F00` | Combined transaction capture |
| `0xF9` | `0xFFFFFF` | GPR-only commit and host readback |
| `0xEA..0xF1` | `0x000000` | GPR-only commits |
| host offset `0x1F` | `0x02`, then `0xFF` | Direct host writes |
| host offset `0x12` | `0x03`, then `0x02` | Direct host writes |
| host offset `0x18` | `0x48` | Direct host write |

No writes to `0xF6`/`0xF7` (modeled ABASE/BBASE) or host DADR offsets
`0x0F..0x11` occurred in the capture. The observed boot initialization is
therefore not enough to reconstruct external DRAM configuration.

A separate diagnostic build logged the OpenASR PA callback:

```text
T551_PA data=18FC run=1 received=0 loaded=0
T551_PA data=18F8 run=1 received=0 loaded=0
T551_PA data=18E8 run=0 received=0 loaded=0
```

This model's PA4 interpretation is bit 4: two release/high transitions before
any program receive flag, followed by a halt/low transition. No later release
was logged. Thus the processor finishes the observed setup halted. This is
not an electrical measurement and `received/loaded` are OpenASR model flags.
The CPU progressed through the boot ROM during the run, but no floppy window
was accessed; an OS boot or selected-effect path is not established.

The all-FF initialization was identical in the 1.61 and 3.53 event projections
for this run configuration. That does **not** establish identical OS loader
code: the OS disk was not read. The factory effect objects are separately
known to be byte-identical across these images from static inventory.

## Evidence classification

| Statement | Status |
|---|---|
| Boot ROM identities and expected fingerprints match | **Verified locally** |
| Stock OpenASR declarations return vector words `0x0300` and `0x000C` | **Verified by CPU-space runtime taps** |
| The temporary swapped-lane test was wrong and reverted | **Verified by its observed words** |
| Headless Ghidra works with explicit JDK21; no chooser appears | **Verified** |
| ASR boot ROM executes code that initializes ES5510 host/program state in OpenASR | **Verified for the emulated firmware/model path** |
| The 160 all-FF words are a selected Ensoniq effect | **False / unsupported** |
| ASR OS 1.61 or 3.53 booted from floppy | **Not established; no FDC register accesses** |
| Any factory effect was uploaded in this run | **No** |
| ASR-10 routines or mappings apply to DP/4 | **No; separate machines and host CPUs** |
| A DP/4 U54/U56 image is available in the inspected PC backup/container | **Not found** |

## Reproducible commands and private evidence

ROM verification:

```sh
sha256sum /asr10/boot-roms/v1.5b/asr-648c-lo-1.5b.bin \
          /asr10/boot-roms/v1.5b/asr-65e0-hi-1.5b.bin
/src/mame -verifyroms asr10booth -rompath /src/roms
```

Runtime invocations used `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`,
`asr10booth`, matching private ROMs, one private writable copy of each OS
image under `/src/floppies/asr10booth/`, `-autoboot_script tools/asr10_es5510_trace.lua`,
`-seconds_to_run 30`, `-video none -sound none -nothrottle -skip_gameinfo`.
Exact unfiltered logs/JSON, Ghidra projects, and ROMs remain in the private
research environment and are intentionally not committed.

The MAME target was built with this target/minimal-feature configuration:

```sh
make SOURCES=src/mame/ensoniq/asr10_boot.cpp USE_QTDEBUG=0 NO_X11=1 \
  NO_OPENGL=1 NO_USE_MIDI=1 NO_USE_PORTAUDIO=1 NO_USE_PULSEAUDIO=1 \
  NO_USE_PIPEWIRE=1 CXXFLAGS=-std=c++20 NOWERROR=1 -j4
```

The container required SDL2_ttf/Qt6 development packages and generated-link
flag adjustments to finish the final link. Several earlier attempts failed;
the final existing executable passed `-verifyroms`. Reproducible machine
state should be rebuilt from the pinned source archive, not assumed from this
binary alone.

## Next evidence needed

For ASR-10: resolve why OpenASR does not issue floppy-register accesses with
the recovered 1.61/3.53 disk images. First use the boot CPU trace and MAME
device/status logs to identify the waiting loop and required boot media/device
state. Then run until OS sectors are read, select `44DDL+CHORUS`, and capture
the transfer before constructing a private program bundle.

For DP/4: reacquire or restore the private U54 v1.06/v1.15 and U56 v1.06/v1.15
images plus the previously generated Ghidra projects/reports. Current
repository docs verify the bank-9 tag read (`U54 offset 0x12000`) and bank
selector behavior but no ESP host transaction. The next decisive evidence
remains synchronized physical DP/4 bus capture of U54 address/data/bank and a
selected ES5510 host bus, or a verified complete address-decoder/loader path.
Do not reuse ASR-10 `$FC30xx` addresses or the 68000 routines above for the
DP/4.
