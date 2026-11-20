# Handoff — ASR-10 ES5510 factory-program capture

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
objects are hash-verified and remain under ignored
`research_local/asr10_firmware/recovered/`. The images' filesystem structures
validate, but they have not been booted. No authentic factory ES5510 program
has been replayed or runtime-captured. Static inspection of 12 recovered ASR
effect objects found guide-consistent `FxUcode` pointers and six-byte ES5510
candidate regions of 82–103 words with `END` opcodes. These are strong static
factory-program candidates; their exact boundaries, GPR/memory initialization
and loader correlation remain unverified. Details and hashes are in
`docs/ASR10_FIRMWARE_EXTRACTION_REPORT.md` and
`docs/ASR10_FIRMWARE_INVENTORY.md`. The boot ROM pair remains unavailable.
OpenASR contains prior committed V3.50 reports of firmware transfers
and partial ROM-HALL/44LUSH instruction lists, but raw logs and full arrays are
absent; see `docs/ASR10_FACTORY_LOADER_ANALYSIS.md`. The only new execution
fixture is synthetic and is explicitly rejected as a complete/authentic capture.

OpenASR maps `$FC3000–$FC303F` to ES5510 host offsets and `$FC3100/3140/3180/31C0`
to read/GPR/instruction/combined commit offsets `$80/$A0/$C0/$E0`. PA4 at
`$FC6823` bit 4 models RUN/HALT. Current machine configuration calls
`es5510_host.set_disable()`, so this host model does not mean the normal machine
scheduler executes the chip.

The local unofficial ESP1 guide describes an ASR/EPS effects-file structure
that can contain global/parameter data, 68k init/task code and a separate ESP
microcode section referenced from the header. This narrows likely acquisition
options but does not establish the exact factory file encoding or any actual
factory program. The VFX manual's MIDI command 03 is explicitly reserved and
unimplemented for VFX; that finding is not transferred to ASR-10 behavior.

## New tools

- `tools/asr10_es5510_trace.lua`: passive OpenASR/MAME CPU-space taps; no state
  changes. Retains tap handles and logs reads/writes/commits/PADAT with PC and
  emulation time.
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

## Validation performed

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools/tests -p 'test_asr10_*.py' -v
PYTHONDONTWRITEBYTECODE=1 python3 tools/asr10_firmware_inventory.py research_local/asr10_firmware
git diff --check
```

Python syntax was also checked with the built-in `compile()` on each changed
tool module, without writing bytecode into the repository.

Result: all 14 ASR-10 capture, EDA expansion, disk catalog, EFE inspection, and
firmware inventory tests passed; Python compilation and whitespace validation
passed.
The Lua script was source-reviewed but could not be executed because no Lua
interpreter or OpenASR/MAME binary with the required private media was available
in this environment. No native/ARM DSP build is claimed for this research-only
milestone.

## Next exact action

Disassemble the ASR OS objects and effect-local `FxInitCode`/update-task
routines as 68k. Trace how they consume `FxUcode` and initialize GPR/delay
state. Obtain a lawful matching boot-ROM pair, then boot the reconstructed
disks in pinned OpenASR with the passive observer during effect selection and
a parameter edit. Keep traces and firmware private. Compare static candidate
words to actual commits, replay one complete program with captured setup, and
differential-test against MAME.

## Preserved project boundaries

No Force plugin code, package, UID, installation, or existing project was
modified. No proprietary media or program payload is tracked. The existing
TROLLfx 5510 remains at UID `T551`, with its original TROLLfx diagnostic delay
and chorus programs and fixed A → B → C → D routing.
