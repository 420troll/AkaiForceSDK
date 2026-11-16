# V0.3 modulation engine handoff

Branch: `codex/v03-modulation` (from `origin/main` at `f3ed7aa`).

`src/original/modulation.hpp` defines a no-allocation, per-unit matrix with two
slots. Each slot has enable, Off/Controller 1..8/Internal LFO/Envelope source, continuous
destination, normalized 0..1 min/max endpoints and inversion. Endpoints map
directly to 0..99-style parameter travel: active modulation replaces that
destination's base value; disabled or Off leaves host automation unchanged.
If both slots select the same destination, slot 2 wins. `beginBlock()` receives
sample rate and unit LFO rate; `process()` is called per sample with stereo
input, normalized base values and a normalized output array. The V0.3 wrapper
evaluates it in fixed 8-sample segments to balance control rate and CPU cost.

Controller 1..8 are global normalized host-automatable plugin parameters,
proposed stable identifiers `controller1`..`controller8` and display labels
`Controller 1`..`Controller 8`. They are never sourced from MIDI CC, which the
insert FX host does not promise. Internal LFO and envelope remain TROLLfx
extensions. Native DSP/UI owners can bind the matrix to existing `EffectParams`
and host automation.

Essential test:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
  src/original/modulation_smoke.cpp -o /tmp/modulation-smoke
/tmp/modulation-smoke
```

Result: PASS. Confirms audible LFO variation, output bounds, inversion,
envelope response, changing Host Controller 3 input, Off/disabled base
pass-through and exact display 0..99 round-trip. V0.3 maps modulation to live
DSP, including actual reverb decay/output; an impulse-tail modulation test
passes. MIDI CC is not used, and Force touchscreen navigation remains
unverified.
