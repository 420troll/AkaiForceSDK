# Original Room / Hall ES5510 feedback-diffusion prototypes

These are TROLLfx instruction programs for the MAME-derived standalone
ES5510 engine. They are not Ensoniq microcode. Both execute stereo read,
feedback, diffusion-state, delay-memory, tap-mix, and serial-output work in the
ES5510 instruction stream.

## Structures

- `room_feedback_diffusion_words.inc` (45 words): two independent channel
  feedback delay lines, one first-order allpass state section per channel, one
  decay return and a separate early return. The test uses a short asymmetric
  pair of early and decay taps for a compact room response.
- `hall_feedback_diffusion_words.inc` (57 words): cross-coupled L/R feedback,
  two allpass sections per channel and a longer decay tap. The test uses an
  asymmetric stereo tap schedule for a broader, slower onset.

This implements the manual-level distinction that room/hall use early echoes,
diffusion and a continuing decay network; the manual describes an input LPF,
input diffuser, two echo taps, Definition decay diffuser and decay feedback.
The prototypes do not implement the full internal DP/4 signal flow or its
filters. The DP/4 manual provides no factory microcode, delay lengths, or
coefficient values; these are original simplified networks.

## Host GPR setup

All values are per engine, fixed before render or updated at control rate:

| GPR | Meaning |
|---:|---|
| 0 / 1 | Main feedback delay tap, L / R, DRAM word offsets |
| 4 / 5 | Early tap, L / R, DRAM word offsets |
| write PCs | Write-bank offsets, generated in `program_metadata.hpp` |
| 40 | Feedback coefficient, Q22, recommended 0.0–0.78 |
| 41 | Early reflection send coefficient, Q22 |
| 42 | Main decay tap output coefficient, Q22 |
| 43 | Dry path coefficient, Q22 |
| 50 / 51 | Allpass coefficients, signed Q22; room uses 50, hall also uses 51 |
| 90–93 | Allpass input-memory state, initialized to zero |

Set `MEMSIZ` register 244 to zero. A pair of channel rings occupies
`2 * bankSize` 16-bit words, `DLength = 2 * bankSize - 1`; main and early tap
GPR offsets must stay within their banks and at least one word from the live
write head. See `tests/room_hall_smoke.cpp` for a complete setup.

## Reproducibility

```sh
python3 experimental/emulation/programs/room_hall/build_programs.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/programs/room_hall/tests/room_hall_smoke.cpp \
  -o /tmp/room-hall-smoke
/tmp/room-hall-smoke \
  experimental/emulation/programs/room_hall/tests/renders/room.wav \
  experimental/emulation/programs/room_hall/tests/renders/hall.wav
```

The 44.1 kHz, two-second impulse renders pass with 45 and 57 executed words
per frame, nonzero outputs and no clipped samples. Current asymmetric taps:
Room onset 310 samples (7.03 ms); Hall onset 217 samples (4.92 ms) from its
left early tap, with the long return later. Stereo correlations were 0.00015
and 0.0 in this particular impulse test. Energy windows and peak are printed
by the smoke test. These are prototype measurements, not acoustic validation
on hardware. The short ring test does not establish multi-second tail fidelity.
