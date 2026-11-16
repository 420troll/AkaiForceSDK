# Original TROLLfx two-band rotary speaker proof

This is an original ES5510 program, not recovered Ensoniq or Waveboy firmware.
It uses a **40-word, 48-bit microprogram** and keeps its audio path in the
standalone MAME-semantics ES5510 engine:

1. The program sums stereo input with a saturating ALU add and halves it to a
   mono cabinet signal.
2. Two cascaded one-pole low-pass sections form a low drum band. Their
   complementary difference from the input is the high horn band. The host
   chooses a crossover coefficient; the test sets two one-pole sections near
   700 Hz.
3. ES5510 DOL instructions write the bands into separate regions of a circular
   delay RAM ring.
4. ES5510 DIL reads fetch each band's independently moving delay tap.
5. Separate MAC paths apply four host-modulated horn/drum left/right gains and
   sum to stereo serial outputs.

This is not an autopanner: the audible output is split into low/high signals,
the two bands use separate RAM histories, the delays move independently, and
the horn and drum have independent speed phases and acceleration trajectories.
The host computes those phase, gain and delay-offset values; it does not
synthesize or filter the audio.

## Program and GPR map

The generated image is `rotary_2band_words.inc`. Its 40 instructions are below
the 160-word ES5510 program limit.

- `GPR[19]`: PC 19 DOL write offset for horn/high band (normally 0).
- `GPR[20]`: PC 20 DOL write offset for drum/low band (bank size).
- `GPR[21]`: PC 21 moving horn read offset.
- `GPR[24]`: PC 24 moving drum read offset (low bank size plus tap).
- `GPR 54`: one-pole low-pass coefficient in Q22, duplicated for both stages.
- `GPR 50/51`: horn left/right gain Q22.
- `GPR 52/53`: drum left/right gain Q22.
- `GPR 60/61`: persistent low-pass stage state.

The renderer/test initializes `MEMSIZ=0`, `DLength=2*bank-1`, `DBASE=0`, and
uses two 2,048-sample banks at 44.1 kHz. The demo host trajectory uses distinct
horn and drum rates, equal-power left/right gains, a 3 ms horn base delay with
2 ms depth, and a 6 ms drum base delay with 4 ms depth. Slow/fast/stop choices
can be represented as host target speeds; the host acceleration time controls
inertia. The microprogram itself is rate-agnostic, while its ring/GPR parameters
must be configured for the current sample rate.

## Test and render

```sh
python3 experimental/emulation/programs/rotary/rotary_asm.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/programs/rotary/tests/rotary_smoke.cpp \
  -o /tmp/es5510-rotary-smoke
mkdir -p /tmp/es5510-rotary-results
/tmp/es5510-rotary-smoke /tmp/es5510-rotary-results
```

The deterministic test measures low- and high-band response at 100 Hz and
5 kHz, changes horn and drum speeds independently, compares short and long
acceleration times, checks stereo movement, checks output rails, and writes the
four-second 44.1 kHz WAV demonstration. Committed results and CSV are in
`test-artifacts/`.

## Scope and limits

The prototype downmixes stereo input to mono before simulating the rotary
cabinet. Its crossover is a simple two-section one-pole split; it is not an
original rotary loudspeaker/cabinet model. Moving sample taps and band
filtering run inside ES5510 instructions. Speed inertia, stereo equal-power
motion, delay-offset trajectories and gain modulation are host-computed.
There is no drive/nonlinear tube stage, no proprietary factory program
compatibility claim, and no ARMv7 VST integration or Force hardware benchmark in
this isolated prototype.
