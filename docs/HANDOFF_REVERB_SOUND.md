# ESP Lab reverb sound milestone

Branch: `feature/esp-lab-reverb-sound-first`

This milestone improves the actual `DP4InspiredReverb` implementation used by
the ESP Lab V0.3 wrapper for Room, Hall, Plate, and Modulated Reverb. It does
not change parameter IDs, plugin UID `Tfx3`, preset state format, the ROM
forensics work, or the existing V0.1/V0.2 packages. No Force installation was
performed.

## Evidence boundary

The DP/4 manual documents two broad reverb families: Room/Hall with input
filtering/diffusion, early reflections and a decay field; Plate with serial
diffusion and cross-coupled stereo feedback. It documents controls such as
decay, predelay, damping, diffusion, reflection time/level, and stereo
position. It does **not** publish factory delay lengths, coefficients, exact
mix matrices, or source code. The structures below are original
manual-informed DSP, not recovered or bit-accurate DP/4 algorithms.

## Implemented structures

- **Room:** two explicit, independently timed reflection taps, compact input
  diffusion, and a four-line Householder feedback field. The reflection
  pattern and short field produce a close, sparse onset.
- **Hall:** a denser reflection cloud derived from the same exposed reflection
  controls, two diffusion stages, and an eight-line feedback field. Its longer
  and more distributed onset is distinct from Room.
- **Plate:** two serial allpass diffusion stages per side, alternating
  mid/side excitation, a normalized cross-coupled four-line Hadamard tank,
  and a less correlated stereo output.
- Hall and Plate use the existing modulation controls for subtle moving delay
  lengths; the dedicated Modulated Reverb retains its modulation behavior.
- Decay sets each tank line's RT60-derived feedback coefficient. Damping is
  inside the feedback path. Predelay, reflection times/levels, diffusion,
  width, output and wet/dry all affect audio. Continuous values receive a
  5 ms smoothing ramp. Feedback/output coefficients are prepared outside the
  per-sample inner loop. Hall/Plate modulation uses a prepared interpolated
  LFO table instead of a sine call per sample. Audio processing allocates no
  memory.

These choices intentionally make Room, Hall and Plate more clearly different
than the manual's shared Room/Hall topology description. That differentiation
is a TROLLfx design decision, not a historical claim.

## Listening and measurement artifacts

`artifacts/esp-lab-v03-reverb-comparison/` contains three matched wet-only stereo impulse
responses, a matched Room → Hall → Plate rhythmic demonstration, and the
measurement report. WAVs are 44.1 kHz, 16-bit PCM. For the IRs, every algorithm
receives the same 0.8 impulse with 20 ms predelay, 2.5 s decay, 0.72 diffusion,
0.34 damping, full width, and identical reflection settings.

Measured from the generated floating-point responses before PCM conversion:

| Algorithm | First peak (0–250 ms) | Active samples, 30–250 ms | 0.1–0.2 s energy | 1.0–1.1 s energy | Stereo correlation, 0.25–2.5 s |
|---|---:|---:|---:|---:|---:|
| Room | 48.93 ms | 432 | 0.138735 | 0.000316239 | 0.1253 |
| Hall | 61.45 ms | 715 | 0.057407 | 0.000114043 | 0.2586 |
| Plate | 54.26 ms | 357 | 0.098037 | 0.000195706 | 0.5487 |

“Active samples” counts frames with either channel above 10% of that response's
first-window peak; it is a simple onset-density indicator, not a perceptual
standard. Integrated energy from 0.1–4 s gives Room/Hall 2.495 and Plate/Hall
1.677 under the matched settings. The energy curves and generation details
are in `artifacts/esp-lab-v03-reverb-comparison/MEASUREMENTS.txt`.

## Verification

Strict smoke commands:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  src/original/reverb_character_smoke.cpp -o /tmp/reverb-character-smoke
/tmp/reverb-character-smoke
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  src/original/reverb_tail_smoke.cpp -o /tmp/reverb-tail-smoke
/tmp/reverb-tail-smoke
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  src/original/reverb_modulation_smoke.cpp -o /tmp/reverb-modulation-smoke
/tmp/reverb-modulation-smoke
```

Results: all passed. Character tests confirm different output for all three
algorithm pairs; audible/measurable response to decay, damping and width;
finite output at 60 s decay under hot input; and successful prepare at 44.1 and
48 kHz. The tail smoke confirms nonzero tails and longer 1 s tail energy when
decay increases from 0.5 to 8 s. Existing modulation-to-decay smoke also
passes.

Official SDK build and host simulation:

```sh
python3 -m pip install --target /tmp/trollfx-zig ziglang
python3 tools/build.py plugins/TROLLfxESPLabV03 --target all \
  --toolchain zig --zig /tmp/trollfx-zig/ziglang/zig
.sdk-tools/host_sim plugins/TROLLfxESPLabV03/build/native/esp-lab-v0-3.so \
  test --seconds 2 --wav /tmp/esp-lab-reverb-v03-smoke.wav
python3 tools/package.py plugins/TROLLfxESPLabV03
```

Build completed with Zig 0.17.1: native, ARM32/Cortex-A17, and ARM64 binaries
compiled. SDK validation reported 0 errors and 0 warnings. Host simulator
passed 20/20 checks (state round-trip, parameter sweep, finite output,
in-place processing, 32/128/512-frame buffers and silence). Desktop simulator
reported 1.14% average block CPU and 3.28% worst block; this is not an ARM
performance measurement. The SDK compiler emitted six existing unused-private
field warnings from `src/original/audio_engine.hpp`; its validator still
reported no warnings or errors. ARMv7 binary built and validated, but has not
been run on the Force.

## Evaluation package

The original tracked V0.3 package remains unchanged. The updated evaluation ZIP
is:

`plugins/TROLLfxESPLabV03/dist/esp-lab-v0-3-0.3.0-reverb-sound-eval.zip`

SHA-256: `b7302750fc3567b89935683573d72190b1a7cbcc7aea2e2daa015ea76d71db94`

The ZIP has the same plugin UID and install folder as V0.3; it is intended as
a replacement candidate. Back up the installed V0.3 folder before a later
hardware evaluation. Do not install this candidate until the user chooses to
run the evaluation.

## Next milestone

Listen to the matched sequence and IRs on monitors/headphones, then tune the
three algorithms against musical material. Next priorities are algorithm
specific decay/damping ranges and exposing the DP/4-inspired reflection sends,
position balance, and independent diffusion controls without changing existing
parameter identities or presets.
