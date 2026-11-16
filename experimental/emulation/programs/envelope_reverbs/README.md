# Original gated / reverse ES5510 envelope prototypes

These programs use an ES5510 delay-feedback and allpass instruction path, then
apply a host-scheduled envelope coefficient through an ES5510 MAC. The ES5510
executes the wet audio scaling, feedback and delay processing. Host code must
perform threshold detection and schedule the envelope GPR; these prototypes do
not claim the DP/4's internal trigger logic or pre-trigger-memory behavior.

## Programs and controls

| Program | Words | Structure | Host envelope behavior |
|---|---:|---|---|
| `gated_plate_envelope` | 37 | One first-order allpass per channel in a stereo feedback delay path | Attack, hold, release; a retriggered gate schedule can restart hold. |
| `reverse_one_shot` | 49 | Two allpass sections per channel, feedback delay tail | Single trigger then rising envelope, hold and release; later triggers ignored. |
| `reverse_retrigger` | 49 | Same two-section ES5510 audio network; independent instance/state | Rising envelope restarts when a later threshold crossing is scheduled. |

Reverse 1 and Reverse 2 have identical ES5510 words. Their distinct behavior is
the host control schedule (one-shot versus retrigger); they are not presented
as different processor microcode. A future integrated host must keep envelope
state separate by mode and unit.

| GPR | Meaning |
|---:|---|
| 0 / 1 | Left/right delay read offsets |
| write PCs | L/R feedback delay-bank offsets in generated metadata |
| 40 | Feedback, Q22; tested at 0.62 |
| 41 | Wet return gain, Q22 |
| 42 | Dry signal gain, Q22 |
| 43 | Output envelope gain, Q22, updated by host at control rate |
| 50 / 51 | Allpass coefficients, signed Q22 |
| 90–93 | Allpass state, initialized to zero |

For reverse-2 pre-trigger memory, retain input in a separate delay-bank region
and address it from instructions; this proof does not yet capture it. Trigger
and retrigger levels, smoothing and host automation/state integration remain
unimplemented.

## Reproducibility

```sh
python3 experimental/emulation/programs/envelope_reverbs/build_programs.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/programs/envelope_reverbs/tests/envelope_reverb_smoke.cpp \
  -o /tmp/envelope-reverb-smoke
/tmp/envelope-reverb-smoke \
  experimental/emulation/programs/envelope_reverbs/tests/renders
```

At 44.1 kHz over two seconds, all three programs produced nonzero stereo output
with no clipped samples. Output peaks: gated 0.2805, reverse one-shot 0.2344,
reverse retrigger 0.2344. The one-shot and retrigger WAVs differ by L1 sample
sum 1.2063 because the latter restarts its rising envelope at 0.5 and 1.0 s.
These are control-scheduled prototypes, not complete gated/reverse DP/4
recreations.
