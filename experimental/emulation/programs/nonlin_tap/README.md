# Original ES5510 one-pass nonlinear-style reverb prototypes

These original TROLLfx microprograms implement a single-pass input diffuser,
one stereo delay-memory write per frame, nine independently addressed delayed
returns per channel, nine envelope-level GPR coefficients, and an ES5510 MAC
sum to stereo output. They are not Ensoniq factory programs or recovered DP/4
microcode. Their layout follows the DP/4 manual's documented non-exponential
tap-envelope family at a high level; exact factory timing and nonlinear curves
are not public.

| Program | Words | Diffusion | Tap span at 44.1 kHz | Character |
|---|---:|---:|---:|---|
| NonLin 1 transient | 81 | 1 serial allpass/channel | 3.9–500 ms | short dense onset with sparse late taps |
| NonLin 2 bloom | 117 | 4 serial allpasses/channel | 113 ms–1.50 s | long distributed bloom |
| NonLin 3 drum | 86 | 1 allpass/channel, centered mono input | 2.2–34.7 ms | tight centered transient |

Tap levels are GPR 150–158 (Q22, host mapping 0–99 to 0–0.10). The conservative
gain keeps a worst-case coincident sum below unity; the DP/4 manual itself
warns against excessive average envelope-tap levels. Wet/dry gains are GPR180
and GPR181. The host may automate the nine levels, but the effect audio is
stored, read, multiplied, summed and output by ES5510 instructions. Input
diffusion coefficients live in per-channel GPRs 128 onward. The engine uses a
two-bank, 96,000-word/channel-capable DRAM region; the maximum supported delay
is limited by available DRAM and configured ring length.

## Reproduce

```sh
python3 experimental/emulation/programs/nonlin_tap/generate.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/programs/nonlin_tap/tests/nonlin_tap_smoke.cpp \
  -o /tmp/nonlin-tap-smoke
mkdir -p /tmp/nonlin-tap-wavs
/tmp/nonlin-tap-smoke /tmp/nonlin-tap-wavs
```

The test renders 44.1/48 kHz stereo impulses and hot-beat sequences and reports
program words, tail endpoints, stereo correlation, and clipping counts.

The smoke renderer's committed test source writes repeatable impulse and hot
beat WAVs when run. At the supplied conservative defaults, 44.1 kHz impulse
tail endpoints are 22,052 frames (NonLin 1), 66,017 (NonLin 2), and 1,532
(NonLin 3); at 48 kHz they scale to 24,002, 71,855, and 1,667 frames. These
are last nonzero samples, not measured RT60 values. The one-pass architecture
has no feedback loop, so decay is defined by the programmed tap schedule and
its nine level controls. No DP/4 exact timing, trigger response, frequency
dependent damping, or factory nonlinear transfer curve is claimed.

Included 44.1 kHz comparison renders are in `renders/`: each program has an
impulse response and the same deterministic hot-beat pulse sequence. Both are
16-bit stereo PCM, 2 seconds, rendered through the standalone MAME-derived
ES5510 engine. The smoke test also verifies that lowering all nine tap-level
GPRs changes the rendered audio and that the hot-beat sequence does not clip.
