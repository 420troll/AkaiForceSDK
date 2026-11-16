# Original TROLLfx ES5510 program catalog

All entries below are original TROLLfx test programs assembled for the
standalone, MAME-semantics ES5510 engine. None is recovered Ensoniq microcode,
a factory effect, or evidence of DP/4 algorithm compatibility.

| Program | Words/frame | Audio structure | Controls in bundle/assembler | Status |
|---|---:|---|---|---|
| `original_stereo_feedback_delay.esp` / `_48k.esp` | 19 | Stereo delay RAM; separate feedback loop and output gains; writes `input + feedback * delayed`; outputs `dry * input + wet * delayed` | Sample rate, max delay/bank length, delay time, feedback, wet gain, dry gain | Impulse/chord audio verified, stable, unclipped at defaults |
| `original_stereo_chorus.esp` / `_48k.esp` | 11 | Two dry-written delay banks; independently modulated left/right delay reads; wet plus dry output | Sample rate, bank capacity, base delay, depth, LFO rate/phase, wet gain | Impulse/chord audio verified; host/renderer computes LFO GPR trajectory |
| `mame_pump_summed_delay_test.esp` | 13 | MAME's public synthetic pump fixture: sums three stereo serial buses, writes/reads delay RAM, outputs stereo | `MEMSIZ`, `DLength`, one-frame tap offset | Execution fixture only; not a musical effect |
| `original_stereo_swap.esp` | 3 | Swaps the dry stereo lanes | None | Instruction/serial smoke test |

## Delay storage and controls

The two original effects use special register 244 (`MEMSIZ`) set to zero. In the
MAME model this gives an address shift of zero, so one raw GPR offset unit
selects one 16-bit DRAM word. END decrements DBASE by one word per frame. The
program loader allocates 1,048,576 words (2 MiB); the assembler rejects a
stereo ring that exceeds that modeled capacity. A 10-second bank per channel
fits at 44.1 and 48 kHz. At 96 kHz, the full 10-second stereo ring is too large
and is rejected.

The delay's feedback, wet, and dry gains are independent Q22 values in GPRs 40,
41, and 42. The assembler clamps feedback to 0–0.94 and wet/dry to 0–1. Wet=0
removes delayed output while leaving the delay feedback loop running; dry=0
removes the immediate input path. The example defaults are feedback 0.55, wet
0.5, dry 0.5, and 250 ms delay.

The chorus uses GPR 40 for wet gain. Renderer-only `lfo_gpr` records define
`register base-offset depth rate-Hz phase-cycles minimum-offset maximum-offset`.
The renderer calculates the sine trajectory and writes those GPRs before each
frame; all audio reads, writes, multiplication, mixing, and serial output are
ES5510 microinstructions. The ESP program does not implement an autonomous LFO.
The default is 25 ms base delay, 5 ms depth, 0.5 Hz rate, 50% wet, and opposite
channel phase.

## Source and reproducibility

Readable source: `experimental/emulation/programs/stereo_feedback_delay.asm`
and `stereo_chorus.asm`. The restricted assembler is `tools/es5510_asm.py`.
See `experimental/emulation/programs/PHASE2_README.md` for exact assembly and
render commands, instruction encodings, parameter units, and the demonstrated
parameter-isolation checks.

WAV examples and hashes are in `artifacts/es5510-emulation/PHASE2_RESULTS.txt`.
The source-derived CPU vectors are in
`experimental/emulation/MAME_SOURCE_VECTORS.md`; they are not a runtime
MAME-vs-adapter differential. No standalone reverb program or experimental VST
is included in this phase.
