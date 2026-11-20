# Handoff — ASR-10 ES5510 factory-program capture

## Runtime update — 2026-11-20

The matching ASR-10 v1.5b boot ROMs were found in the private PC backup,
checksum-verified, and passed OpenASR/MAME `-verifyroms`. Headless Ghidra is
configured with OpenJDK 21 through `tools/ghidra_headless.sh`; `--help` runs
without the interactive JDK chooser. Ghidra disassembly and passive runtime
capture now show an ASR boot-ROM ES5510 initialization path.

Important correction: the pinned OpenASR ROM declarations were **not** changed
for the final run. MAME's CPU-space taps under the stock declarations returned
reset vector words `0x0300` and `0x000C`. A temporary swapped-lane experiment
returned `0x0003` and `0x0C00` and was reverted. Earlier claims that stock
OpenASR had the wrong reset vectors are withdrawn.

The 1.61 and 3.53 runtime runs each produced 293 traced CPU-space events,
including 160 combined commits of the all-FF blank instruction image. No
floppy-controller access occurred, so neither OS boot nor effect selection
was established. A separate instrumented callback run observed PA4 modeled
release twice before any program was received, then halt; no factory upload or
factory execution was captured. The traces are incomplete and must not be
replayed as effects. Exact evidence, hashes, commands, limitations and next
steps are in [the runtime report](ASR10_BOOT_ROM_RUNTIME_CHECK.md).

This ASR-10 work is distinct from DP/4: the DP/4 U54/U56 ROM files and prior
Ghidra project are absent from this current machine's inspected paths. The
existing DP/4 bank-9 tag and capture recommendation remain in the DP/4 docs;
do not project ASR-10 firmware routines or `$FC30xx` host addresses onto that
hardware.

## Branch and source pins

- AkaiForceSDK branch: `feature/5510-asr10-factory-programs`
- Base commit: `d1b1b631694a490ffdfde813e25df30ff7b5a93b`
- OpenASR: `241f9331477b07b072330ca6ebd2d7a2e2cc832e`
- MAME: `8be57cdc5f44ab830dff44c06dd39f24190742d4`

## Result

This milestone implements passive capture and offline reconstruction tooling,
documents ASR-10/OpenASR host protocol evidence, and captures the distinction
between the runtime host protocol and the effect-file container. It does **not**
recover a complete replayable factory program. The 1.61 and 3.53 installer
payloads have now been recovered in isolated, network-disabled 32-bit Wine
containers. Their `.EDA` files, reconstructed raw images, OS object and effect
objects are hash-verified and remain outside the public repository. The
images' filesystem structures validate. OpenASR attempts with both images
recorded no floppy-controller access, so OS boot remains unestablished. No authentic factory ES5510 program
has been replayed or runtime-captured. Static inspection of 12 recovered ASR
effect objects found guide-consistent `FxUcode` pointers and six-byte ES5510
candidate regions of 82–103 words with `END` opcodes. These are strong static
factory-program candidates; their exact boundaries, GPR/memory initialization
and loader correlation remain unverified. Details and hashes are in
`docs/ASR10_FIRMWARE_EXTRACTION_REPORT.md` and
`docs/ASR10_FIRMWARE_INVENTORY.md`. A later local PC backup supplies a
fingerprint-verified v1.5b boot ROM pair; runtime boot validation is recorded
in `docs/ASR10_BOOT_ROM_RUNTIME_CHECK.md`.
OpenASR contains prior committed V3.50 reports of firmware transfers
and partial ROM-HALL/44LUSH instruction lists, but raw logs and full arrays are
absent; see `docs/ASR10_FACTORY_LOADER_ANALYSIS.md`. The only new execution
fixture is synthetic and is explicitly rejected as a complete/authentic capture.

Follow-up static analysis disassembled the `44DDL+CHORUS` effect-local update
task. It confirms `FxUpdateTask=0x0452` calls routine `0x0468`, which reads
variable offsets and parameter metadata, smooths/limits stored values, then
invokes OS service `$801e`; that service's implementation and DSP side effects
remain unresolved. The task positions its descriptor pointer at
`FxUcode-14`, but this does not establish a GPR map. The object header stores
output GPR destinations `0x82` and `0x83` and `FxNumVoices=1` for all 12
effects. These are output assignments, not initial register values.

`tools/asr10_factory_manifest.py` emits a metadata-only, machine-readable
cross-version manifest with object hashes, candidate section offsets, word
counts, trailer sizes, END opcode positions, output GPRs, voice count, and bus
labels. It emits no firmware or instruction bytes. New reports:

- `docs/ASR10_FACTORY_PROGRAM_MANIFEST.md` and `.json`
- `docs/ASR10_FXUCODE_LOADER_TRACE.md`
- `docs/ASR10_GPR_INITIALIZATION.md`
- `docs/ASR10_FIRST_FACTORY_EXECUTION.md`

The first authentic execution remains **blocked**. Do not replay the static
candidate with guessed GPR, special-register, or DRAM values.

OpenASR maps `$FC3000–$FC303F` to ES5510 host offsets and `$FC3100/3140/3180/31C0`
to read/GPR/instruction/combined commit offsets `$80/$A0/$C0/$E0`. PA4 at
`$FC6823` bit 4 models RUN/HALT. The machine calls `es5510_host.set_disable()`
to remove normal scheduler execution; however, the audio pump explicitly calls
`run_once()` per sample when HALT is released and `PUMP_FAKE_ESP_PROCESSING=0`.
Thus the source can execute uploaded instructions through the pump, but this
does not establish that a compatible firmware boot uploads a complete factory
program or that the modeled audio route/timing matches hardware.

The local unofficial ESP1 guide describes an ASR/EPS effects-file structure
that can contain global/parameter data, 68k init/task code and a separate ESP
microcode section referenced from the header. This narrows likely acquisition
options but does not establish the exact factory file encoding or any actual
factory program. The VFX manual's MIDI command 03 is explicitly reserved and
unimplemented for VFX; that finding is not transferred to ASR-10 behavior.

## New tools

- `tools/asr10_es5510_trace.lua`: passive OpenASR/MAME CPU-space taps; no state
  changes. Retains tap handles and logs program-space reads/writes, commits,
  boot reads, floppy-window activity, PC and emulation time. PA4 is recorded
  separately at the OpenASR callback boundary, not inferred from fallback RAM.
- `tools/asr10_capture_reconstruct.py`: reconstructs byte-latch values at
  commit time, preserves raw events and labels incomplete captures. It refuses
  inputs/outputs inside the repository.
- `tools/asr10_capture_disassemble.py`: prints instruction field decodes to
  stdout for private review.
- `tools/asr10_capture_to_bundle.py`: explicit opt-in conversion of reviewed
  complete candidates to the current renderer's reduced `.esp` bundle; it
  refuses incomplete and synthetic captures and writes only outside Git.
- `tools/tests/test_asr10_capture.py`: tests byte ordering, commit behavior,
  synthetic classification and invalid input.
- `tools/asr10_firmware_inventory.py`: hashes/classifies local packages,
  checks boot-ROM fingerprints, reports missing media, inspects ZIP/NE
  metadata, and detects the EDA signature without claiming to parse its format.
- `tools/asr10_factory_manifest.py`: produces metadata-only 1.61/3.53 effect
  object comparison; refuses incomplete 12-effect inventories.

## Validation performed

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools/tests -p 'test_asr10_*.py' -v
PYTHONDONTWRITEBYTECODE=1 python3 tools/asr10_firmware_inventory.py research_local/asr10_firmware
git diff --check
```

Python syntax was also checked with the built-in `compile()` on each changed
tool module, without writing bytecode into the repository.

The Lua recorder was executed with the private ROM/media in the research
container. The full suite was run after the manifest extension:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s tools/tests -p 'test_asr10_*.py' -v
git diff --check
```

The ASR suite was rerun on 2026-11-20 after the capture-address correction;
all 16 tests passed. `sh -n tools/ghidra_headless.sh` and `git diff --check`
passed. The Ghidra wrapper displayed headless usage with an explicit JDK and
no picker. MAME `-verifyroms asr10booth -rompath /src/roms` passed in the
research container. No native or ARM DSP build/audio test is claimed because
complete authentic initialization is still missing.

## Next exact action

Diagnose why the current OpenASR boot run does not access the inserted OS
floppy. Instrument FDC command/status, DMA and interrupt/wait state, and
determine whether the emulated controller requires media geometry or status
behavior not currently provided. Once OS sectors are read, use the passive
observer during `44DDL+CHORUS` selection and a parameter edit. Keep traces and
firmware private. Correlate the extracted EFE payload hash with instruction
commits, GPR values, special-register state, delay-memory setup, HALT/RUN, and
readback. Replay the captured state and differential-test against MAME.

## Preserved project boundaries

No Force plugin code, package, UID, installation, or existing project was
modified. No proprietary media or program payload is tracked. The existing
TROLLfx 5510 remains at UID `T551`, with its original TROLLfx diagnostic delay
and chorus programs and fixed A → B → C → D routing.
