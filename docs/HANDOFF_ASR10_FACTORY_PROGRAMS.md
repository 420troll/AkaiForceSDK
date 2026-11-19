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
recover a complete replayable factory program. Two Chicken Systems OS update
self-extractors (1.61 and 3.53) were downloaded as inert data to ignored
`research_local/asr10_firmware/` and hash-verified. Their embedded EDA images
remain unextracted. No boot ROMs, raw OS images or factory effect files were
available. OpenASR contains prior committed V3.50 reports of firmware transfers
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
  checks known boot-ROM fingerprints, reports missing media and inspects
  ZIP/NE metadata without running installers.

## Validation performed

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools/tests -p 'test_asr10_*.py' -v
PYTHONDONTWRITEBYTECODE=1 python3 tools/asr10_firmware_inventory.py research_local/asr10_firmware
git diff --check
```

Python syntax was also checked with the built-in `compile()` on each changed
tool module, without writing bytecode into the repository.

Result: three capture tests and two inventory tests passed; Python compilation
and whitespace validation passed.
The Lua script was source-reviewed but could not be executed because no Lua
interpreter or OpenASR/MAME binary with the required private media was available
in this environment. No native/ARM DSP build is claimed for this research-only
milestone.

## Next exact action

Obtain a legally held ASR-10 boot ROM pair matching the MAME fingerprints and
an OS image. Prefer 1.61 or 3.53 until OpenASR's V3.50 baseline is reconciled
with the service manual note that V3.50 ROMs are ASR-88 only. Extract
`asr161os.eda` or `asr353os.eda` from the ignored wrappers with a compatible
non-executing reader, or use a user backup. Run pinned OpenASR with the passive
observer during boot, effect selection and a parameter edit. Keep logs and
reconstructed outputs private. Recover full instruction/GPR/RAM state and run
differential execution before calling an authentic program captured.

## Preserved project boundaries

No Force plugin code, package, UID, installation, or existing project was
modified. No proprietary media or program payload is tracked. The existing
TROLLfx 5510 remains at UID `T551`, with its original TROLLfx diagnostic delay
and chorus programs and fixed A → B → C → D routing.
