# V0.3 modulation engine handoff

Branch: `codex/v03-modulation` (from `origin/main` at `f3ed7aa`).

`src/original/modulation.hpp` defines a no-allocation, per-unit matrix with two
slots. Each slot has enable, Off/Internal LFO/Envelope source, continuous
destination, normalized 0..1 min/max endpoints and inversion. Endpoints map
directly to 0..99-style parameter travel: active modulation replaces that
destination's base value; disabled or Off leaves host automation unchanged.
If both slots select the same destination, slot 2 wins. `beginBlock()` receives
sample rate and unit LFO rate; `process()` is called per sample with stereo
input, normalized base values and a normalized output array.

Internal LFO and envelope are TROLLfx additions. They do not emulate the DP/4's
globally selected external Controller 1..8, and no MIDI CC is exposed. The
native DSP/UI owners can bind the matrix to existing `EffectParams` and host
automation without making modulation a MIDI feature.

Essential test:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
  src/original/modulation_smoke.cpp -o /tmp/modulation-smoke
/tmp/modulation-smoke
```

Result: PASS. Confirms audible LFO variation, output bounds, inversion,
envelope response, Off/disabled base pass-through and exact display 0..99
round-trip. Plugin integration is separate; this header does not claim
touchscreen/Force behavior until its schema and host test are integrated.
