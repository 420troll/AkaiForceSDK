# Original TROLLfx 12-stage stereo phaser prototype

This is an original ES5510 program, not recovered EPS-16+ or Waveboy firmware.
The program executes **12 cascaded first-order allpass sections per channel**
(24 stereo-channel sections) in 131 48-bit words, below the processor's
160-word instruction RAM. The allpass difference and feedback MAC, state update,
and serial output are ES5510 instructions. The host updates signed Q22
coefficients (modulation control) and applies only the final wet/dry blend.

Each section implements the first-order recurrence

```text
d = y[n-1] - x[n]
y[n] = x[n-1] + a*d
x[n-1] <- x[n]
```

The twelve identical coefficients are solved so the 12-section cascade reaches
180 degrees at the requested center frequency; mixing equal dry and wet signals
therefore creates a deep cancellation notch there. For sample rate `Fs` and
center `fc`, the host computes `t=tan(pi*fc/Fs)`, `q=tan(pi/24)`, and
`a=(q-t)/(q+t)`. The stereo spread shifts the right channel's target by an
octave ratio. Coefficients clamp to stable, representable signed Q22 values.
The test uses a 0.25-octave right-channel spread.

## GPR map

- 32/33: feedback-added left/right input
- 64–75: left allpass coefficient, duplicated across its twelve stages
- 80–91: right allpass coefficient, duplicated across its twelve stages
- 100–111 / 124–135: per-stage previous input state
- 112–123 / 136–147: per-stage previous output state
- 150/151: prior-frame final allpass output for left/right feedback
- 152: signed feedback Q22, supported range -0.90..+0.90

The input, internal allpass stages, feedback state and output lanes are stereo.
Positive and negative feedback are supported. With `abs(feedback)<1`, the
feedback path is bounded in the tested emulator vectors; aggressive levels may
still saturate the 24-bit ES5510 data path. Wet/dry mixing is outside the
microprogram, while all phase-shifting and feedback audio processing runs in
ES5510 instructions.

## Build and test

From repository root:

```sh
python3 experimental/emulation/programs/phaser/phaser_asm.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/programs/phaser/tests/phaser_smoke.cpp \
  -o /tmp/es5510-phaser-smoke
mkdir -p /tmp/es5510-phaser-results
/tmp/es5510-phaser-smoke /tmp/es5510-phaser-results
```

The deterministic smoke run measures logarithmic frequency response with 500 Hz
and 2 kHz center settings, generates a four-second center-sweep WAV, checks
stereo difference, and runs two-second positive/negative feedback tail tests.
Recorded artifacts are in `test-artifacts/`:

- `phaser_frequency_response.csv`
- `phaser_center_sweep.wav` (44.1 kHz stereo)
- `RESULTS.txt`

This is a native emulator prototype. It has not been integrated into the VST or
benchmarked on ARMv7 hardware yet. It does not claim compatibility with a
factory EPS-16+ effect.
