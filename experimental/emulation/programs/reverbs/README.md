# ES5510 reverb prototypes (original TROLLfx microcode)

These are executable instruction programs for the standalone MAME-semantics
ES5510 engine. They are not Ensoniq factory microcode or recovered DP/4
algorithms. Each image is below the 160-word instruction RAM limit. Audio
filtering, allpass diffusion, delay-memory traffic, feedback multiplication,
and stereo output are ES5510 instructions. The host initializes GPRs and the
program's DBASE ring memory.

## Catalog/API

`ReverbPrograms.hpp` defines `trollfx::esp::programs::reverbs::Id`,
`Definition`, `kPlateCatalog`, and `find()`. Only Small/Large Plate are in the
ready descriptor catalog. The Non Lin-inspired output images are research
fixtures, explicitly excluded from V0.3 integration until a verified
nine-level output envelope is implemented. All use 65,536 words of DRAM: two
32,768-word banks. The program reads from instruction address GPR0/GPR1 and
writes feedback at the program-specific `leftWriteAddressGpr` and
`rightWriteAddressGpr`. GPR160 is feedback, GPR161 wet, GPR162 dry. GPR64+i are
left allpass coefficients and GPR80+i are right coefficients, with 16-register
channel stride. Signed Q22 values are represented in 24-bit GPRs.

| ID | Words | Distinct structure | Defaults at 44.1 kHz |
|---|---:|---|---|
| Non Lin 1 transitional fixture | 31 | One short allpass diffuser/channel and independent short feedback lanes | taps 317/431 samples (~7.2/9.8 ms); feedback .24 |
| Non Lin 2 transitional fixture | 61 | Four serial allpass stages/channel and long cross-coupled feedback | taps 1877/2347 (~42.6/53.2 ms); feedback .79 |
| Non Lin 3 transitional fixture | 36 | One allpass/channel, mono-centered feed and short cross-coupled returns | 509/509 (~11.5 ms); feedback .46 |
| `SmallPlate` | 41 | Two allpass stages/channel before cross-coupled feedback | 1301/1499 (~29.5/34.0 ms); feedback .68 |
| `LargePlate` | 51 | Three allpass stages/channel with longer cross-coupled paths | 2801/3163 (~63.5/71.7 ms); feedback .82 |

The allpass stage executes `y[n] = x[n-1] + a * (y[n-1] - x[n])`; delay feedback
is then applied by the 24-bit ALU/MAC and RAM instruction path. The final wet/dry
controls are independent. Changes to tap addresses and coefficients should be
smoothed by the host when used as live controls.

The DP/4 manual describes Non Lin 1/2/3 as separate algorithms. These
transitional test images do **not** implement the verified nine-level output
envelope or measured nonlinear trigger/hold/retrigger process, so they are not
V0.3 Non Lin effects. Their only nonlinearities are ES5510 fixed-point
saturation/rounding. Do not describe them as recreations of the original
algorithms. Manual-supported plate concepts include multiple diffusers,
Definition/decay, cross-coupled stereo feedback, and output filtering; these
prototypes implement diffuser cascades and cross-coupled returns but currently
lack the documented output damping/early-reflection controls.

## Build and test

```sh
python3 experimental/emulation/programs/reverbs/generate.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/programs/reverbs/tests/reverb_prototype_smoke.cpp \
  -o /tmp/reverb-prototype-smoke
mkdir -p artifacts/es5510-emulation/v03/reverb-prototypes
/tmp/reverb-prototype-smoke artifacts/es5510-emulation/v03/reverb-prototypes
```

The smoke renderer produces 44.1 kHz stereo 16-bit matched impulse and hot
beat WAVs for each program. It runs the same hot transient loop through all
five programs and fails if an output hits 16-bit rails. Measurements are in
`artifacts/es5510-emulation/v03/reverb-prototypes/RESULTS.txt`. This test checks
stability and distinguishable structures; it does not establish musical quality
or Force CPU cost.
