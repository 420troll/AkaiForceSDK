# ES5510 emulation Phase 2 handoff

## Result

The standalone MAME-semantics execution path now runs original stereo feedback
delay and modulated chorus microprograms as ES5510 instruction words, through
its ALU/MAC, delayed DIL/DOL, delay RAM, END/DBASE movement, serial registers,
and per-frame program lifecycle. This is not a conventional C++ audio effect.
The native ESP Lab plugin and prior releases are untouched.

Programs, parameter definitions, assembly source and regeneration instructions:
[ES5510_PROGRAM_CATALOG.md](ES5510_PROGRAM_CATALOG.md) and
[PHASE2_README.md](../experimental/emulation/programs/PHASE2_README.md).
The repeatable WAV measurements, file SHA-256 values, test/build log and ARM
binary hashes are in
[PHASE2_RESULTS.txt](../artifacts/es5510-emulation/PHASE2_RESULTS.txt).
Native performance details are in
[phase2/BENCHMARKS.md](../artifacts/es5510-emulation/phase2/BENCHMARKS.md).

## Programs and audio

- **Stereo feedback delay:** 19 words/frame. The feedback loop stores `input +
  feedback * delayed`; the output independently combines `dry * input + wet *
  delayed`. Feedback, wet, dry, and delay length/time change independently.
  Default feedback 0.55, wet/dry 0.5, delay 250 ms. The 44.1 kHz impulse has
  stereo echoes at 0, 250, 500, 750 ms, and onward without clipping.
- **Stereo chorus:** 11 words/frame. The ESP performs stereo delay reads/writes,
  MAC gain and output mixing. The offline host renderer supplies opposite
  phase sine values to two delay-offset GPRs once per frame. It is a host-LFO
  experiment, not a self-running ESP oscillator.
- **Reverb:** not implemented. This phase validates two practical programs
  first; a multitap/diffusion network remains future work.

All five Phase 2 WAVs are synthetic, reproducible test material at 44.1 kHz.
Impulse and musical-input renders are in
[`artifacts/es5510-emulation/phase2/`](../artifacts/es5510-emulation/phase2/).
The final stereo delay impulse/chord outputs are also mirrored in `phase2/wetdry`
for the independent wet/dry control validation.

## Conformance and limitations

The available MAME checkout is pinned to `8be57cdc5f44ab830dff44c06dd39f24190742d4`.
It is sparse: ES5510, pump, and Ensoniq machine-driver source is present, but
MAME's `src/emu`, build files, and executable are absent. The new vectors derive
expected results from that exact source and cover saturating/wrapping 24-bit
ADD, CCR, MAC output/pipeline and conditional skip. **They are not a live
MAME differential run.** Delay mapping was corrected after tests revealed
that `MEMSIZ=0xff` aliases addresses beyond 65,535 words. The original effects
now use `MEMSIZ=0` and raw word offsets. Non-MAC words target the non-writable
GPR 255 in parallel MAC fields; the END word uses all-ones unused register
fields so it does not corrupt GPR0 at the next frame restart.

There is no licensed factory effect bundle available, so no historical Ensoniq
effect program was executed. No DP/4 compatibility or factory algorithm
accuracy is claimed. MAME BSD-3-Clause terms remain in
[THIRD_PARTY_NOTICES.md](../experimental/emulation/THIRD_PARTY_NOTICES.md).

## Performance and Force gate

The optimized native x86-64 benchmark matrix measures the actual 19-word delay
and 11-word chorus at 44.1/48 kHz, 256-frame and one-second spans, as well as
MAME fixture and 160-instruction ceiling workloads. See the benchmark table.
ARMv7 renderer and benchmark are compiled as ELF32 ARM EABI5 hard-float for
Cortex-A17/ARMv7-A. **ARM runtime time and Force CPU budget are unmeasured:**
there is no qemu-arm or Force SSH session. A VST is therefore not produced in
this milestone; do not interpret the x86 timings as a Force result.

The command-line ARM test kit is
`artifacts/es5510-emulation/phase2/es5510-phase2-armv7-testkit.zip` (SHA-256
`c7ff6784d7a10fa7c9342c89d4d68f0eec9be2f228e6231979c2c3f366b45eeb`). It
contains 44.1 and 48 kHz program bundles. It is not an Akai VST and has not
been deployed. No hardware project or plugin was
modified.

## Next work

1. Run the included ARM benchmark on an authorized Force session from a
   temporary directory; record model/OS, 44.1/48 kHz and 256/4096/one-second
   spans, including the 160-word ceiling.
2. Build a genuine differential test around a full, buildable MAME checkout
   pinned to the cited commit. Compare per-step registers, MAC/CCR, DIL/DOL,
   DRAM and audio traces.
3. Consider a small multitap reverb only after delay-address/MAC/DOL differential
   coverage improves.
4. Only if measured ARM performance supports it, build a separate experimental
   VST identity. Load programs before callback entry; update only prepared GPRs
   during real-time processing; keep all effect DSP in emulated instructions.

## Source milestones

This development branch contains the original Issue #4 standalone harness and
correction, Phase 2 source-derived vectors, the extended benchmark, renderer
LFO schedule, original delay/chorus programs, and independent delay wet/dry
controls. The final integrated commit is recorded in the publishing handoff
and git history.
