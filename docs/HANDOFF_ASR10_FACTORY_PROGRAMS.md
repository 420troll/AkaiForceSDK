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
recover a factory program. No OS image or ROMs were available; OpenASR's local
media directories contain only README files. The only new execution fixture is
synthetic and is explicitly rejected as a complete/authentic capture.

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

## Validation performed

```sh
python3 -m unittest discover -s tools/tests -p 'test_asr10_capture.py' -v
python3 -m py_compile tools/asr10_capture_reconstruct.py tools/asr10_capture_to_bundle.py tools/asr10_capture_disassemble.py
git diff --check
```

Result: 3 tests passed; Python compilation and whitespace validation passed.
The Lua script was source-reviewed but could not be executed because no Lua
interpreter or OpenASR/MAME binary with the required private media was available
in this environment. No native/ARM DSP build is claimed for this research-only
milestone.

## Next exact action

Obtain the user's legally held ASR-10 OS V3.50 floppy image and matching system
ROM files. Place them locally according to `/tmp/OpenASR-issue4/floppies/README.md`
and `roms/README.md`; do not add them to Git. Build/run the pinned OpenASR
`asr10booth` with the passive Lua observer during boot, effect selection, and a
single parameter edit. Save output only in a private directory, then reconstruct
and inspect it. If the firmware does not reach a complete upload, identify the
exact emulation blocker before changing machine behavior. Do not label a
candidate factory code until instruction/GPR/RAM state, boundaries, lifecycle,
readback, and differential execution are validated.

## Preserved project boundaries

No Force plugin code, package, UID, installation, or existing project was
modified. No proprietary media or program payload is tracked. The existing
TROLLfx 5510 remains at UID `T551`, with its original TROLLfx diagnostic delay
and chorus programs and fixed A → B → C → D routing.
