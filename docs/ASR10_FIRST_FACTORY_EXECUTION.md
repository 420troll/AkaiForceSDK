# First ASR-10 factory ES5510 execution — status

**Status: blocked before authentic replay.** As of 2026-11-20, this project
has not executed an ASR-10 factory effect through the standalone ES5510 engine.
The static EFE candidates are now cataloged more precisely, and one
object-local parameter task has been disassembled. Required initialization
and runtime correlation are still absent. No factory bundle or diagnostic
WAV was generated because filling unknown GPR/special-register values would
misrepresent the result.

## Candidate selected for next replay

`44DDL+CHORUS` is the smallest and clearest delay-bearing candidate:

- OS 1.61/3.53 extracted EFE payload SHA-256 (including block padding):
  `0b58e96ecaf251d3496f529d120537ba2262ddde3dd696d3c6f5ce594ef6d9c8`
- `FxSize=3161` (`0x0c59`); `FxUcode=0x0a3c`.
- Static candidate interval `[0x0a3c,0x0c59)`: 90 full six-byte words and one
  unexplained trailing byte.
- The candidate's opcode fields include 14 END words at indices listed in
  [the manifest](ASR10_FACTORY_PROGRAM_MANIFEST.md). This is not an established
  end-of-program marker.
- `FxInitCode=0`; `FxUpdateTask=0x0452`.
- The update task calls object-local routine `0x0468` twice through a helper
  that points A2 to `FxUcode-14`. The updater interpolates/slews parameter
  values and calls OS service `$801e`; the service and its DSP side effects
  are unresolved.

## Execution and validation performed

No factory instruction stream was loaded into the standalone executor, and no
audio render is presented as factory output. The executor's bundle format
requires instruction words and permits GPRs, host serial, and DRAM seeds, but
the source object has not yet been correlated with actual host commits and the
required GPR/special-register/delay setup is unknown. Using invented setup
would not meet the task's authenticity requirement.

The new manifest generator was tested with synthetic fixtures and run against
the two private effect-object directories. Existing object-inspector,
firmware-inventory tests also passed. These validate metadata tooling only,
not DSP execution:

The full ASR suite passed (16 tests):

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s tools/tests -p 'test_asr10_*.py' -v
```

No ARMv7 or Force build was needed because no plugin code was changed.

## Exact blockers

1. OpenASR accepts the hash-verified boot-ROM pair. CPU-space taps under the
   unmodified pinned driver return reset vector words `0x0300` and `0x000C`;
   the earlier swapped-lane interpretation was incorrect.
2. A 30-second run with each recovered 1.61/3.53 disk produced boot-ROM
   traffic and blank ES5510 setup, but no floppy-controller accesses. OS boot
   and effect selection are not established. The older V3.50 research
   report's raw traces and full instruction/GPR arrays are absent and V3.50
   ASR-10 compatibility is unresolved.
3. The complete 68k OS service implementation for `$801e` and relevant
   initialization calls is not mapped from the disk object to a running
   firmware address space.
4. Special-register, host serial/control, external delay RAM, and parameter
   update transactions have not been recorded for a selected recovered
   effect.
5. The pinned MAME `es5510d.cpp` is not a functional file disassembler; the
   field decoder in this repository is metadata tooling, not an instruction
   semantic validator. The pinned MAME execution core remains the runtime
   reference, but no captured state is available to feed it.

## Next concrete milestone

Diagnose the OS boot wait first: add passive logging for FDC command/status,
DMA, IRQ and relevant firmware wait state, then establish that OS sectors are
actually read from 1.61 or 3.53. Once the UI is running, select
`44DDL+CHORUS` and capture, without filtering, host latch writes, commits,
readbacks, HALT/RUN and serial/control operations. Capture effect-variable
edits as a second run. Use the captured upload—not the static candidate
interval—to reconstruct a private bundle with complete setup. Differentially
execute that bundle in MAME and TROLLfx, then render silence, impulse, sine and
transients. This is the shortest non-speculative path to first factory
execution.

Runtime details, exact input hashes, and the distinction between ASR-10
evidence and DP/4 findings are in `ASR10_BOOT_ROM_RUNTIME_CHECK.md`.

Never publish firmware, effect payloads, captured instruction arrays or
private replay bundles. No Force plugin or installation was modified.
