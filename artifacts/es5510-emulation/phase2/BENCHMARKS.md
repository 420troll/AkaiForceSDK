# ES5510 Phase 2 benchmark results

Recorded in the development container on 2026-11-16. These are host-side
measurements only; they are not Akai Force performance claims.

## Native matrix

Host: x86_64, Intel Core i7-10750H @ 2.60 GHz, Ubuntu toolchain g++ 13.3.0.
The benchmark ran the standalone MAME-semantics core in optimized mode. For
each long row, median is from 3 runs; each short row is median of 9 runs.
The short span is 256 emulated audio frames and the long span is one second
(44,100 or 48,000 frames). The `outer block` field groups the loop in the
benchmark executable; it does not model host callback overhead or change the
per-frame ES5510 execution schedule.

| Rate | Span | Frames | Program | Instructions/frame | Median µs/frame | Median elapsed |
|---:|---:|---:|---|---:|---:|---:|
| 44.1 kHz | short_256 | 256 | original stereo swap | 3 | 0.0859 | 0.000022 s |
| 44.1 kHz | short_256 | 256 | MAME pump delay fixture (MEMSIZ initialized) | 13 | 0.4535 | 0.000116 s |
| 44.1 kHz | short_256 | 256 | 160-word no-op ceiling | 160 | 4.4852 | 0.001148 s |
| 44.1 kHz | long_1sec | 44,100 | original stereo swap | 3 | 0.1147 | 0.005058 s |
| 44.1 kHz | long_1sec | 44,100 | MAME pump delay fixture (MEMSIZ initialized) | 13 | 0.5873 | 0.025898 s |
| 44.1 kHz | long_1sec | 44,100 | 160-word no-op ceiling | 160 | 5.2903 | 0.233302 s |
| 48 kHz | short_256 | 256 | original stereo swap | 3 | 0.0895 | 0.000023 s |
| 48 kHz | short_256 | 256 | MAME pump delay fixture (MEMSIZ initialized) | 13 | 0.4840 | 0.000124 s |
| 48 kHz | short_256 | 256 | 160-word no-op ceiling | 160 | 4.3274 | 0.001108 s |
| 48 kHz | long_1sec | 48,000 | original stereo swap | 3 | 0.0944 | 0.004532 s |
| 48 kHz | long_1sec | 48,000 | MAME pump delay fixture (MEMSIZ initialized) | 13 | 0.5635 | 0.027050 s |
| 48 kHz | long_1sec | 48,000 | 160-word no-op ceiling | 160 | 5.1989 | 0.249548 s |

## Original TROLLfx effect programs

The following additional timings use the final independent-gain stereo delay
(19 instructions/frame) and modulated stereo chorus (11 instructions/frame).
The benchmark loads each `ES5510_BUNDLE`; chorus includes per-frame host LFO
GPR updates. Each short row is the median of 9 runs; each one-second row is the
median of 3 runs. These are native x86 measurements, not ARM/Force estimates.

| Rate | Span | Frames | Program | Instructions/frame | Median µs/frame | Median elapsed |
|---:|---:|---:|---|---:|---:|---:|
| 44.1 kHz | short_256 | 256 | TROLLfx feedback delay | 19 | 0.5332 | 0.000136 s |
| 44.1 kHz | short_256 | 256 | TROLLfx modulated chorus | 11 | 0.4047 | 0.000104 s |
| 44.1 kHz | long_1sec | 44,100 | TROLLfx feedback delay | 19 | 0.5036 | 0.022208 s |
| 44.1 kHz | long_1sec | 44,100 | TROLLfx modulated chorus | 11 | 0.4298 | 0.018955 s |
| 48 kHz | short_256 | 256 | TROLLfx feedback delay | 19 | 0.4504 | 0.000115 s |
| 48 kHz | short_256 | 256 | TROLLfx modulated chorus | 11 | 0.3234 | 0.000083 s |
| 48 kHz | long_1sec | 48,000 | TROLLfx feedback delay | 19 | 0.5021 | 0.024100 s |
| 48 kHz | long_1sec | 48,000 | TROLLfx modulated chorus | 11 | 0.3357 | 0.016113 s |

Full rows with repeat counts and the workload output are in
`EFFECT_BENCHMARKS.csv`. The `160-word no-op ceiling` remains 4.5–5.3 us/frame
on this desktop and is the instruction-count upper workload, not the actual
cost of these effects.

The pump fixture benchmark uses the corrected `gpr 244 0xff` MEMSIZ initialization;
the prior zero-init delay-RAM results are invalid and excluded. This public MAME
test program performs its fixed 7,440-frame summed delay. It is a useful delay-RAM
execution profile but not the adjustable stereo delay or chorus program requested
for Phase 2. Its timing cannot predict those program costs. Program
and native CSV data are retained in `NATIVE_BENCH_MATRIX.csv`.

## ARMv7 build

The final renderer and benchmark, including bundle LFO scheduling, cross-compile with Zig 0.17.1:

```sh
/tmp/trollfx-zig/ziglang/zig c++ \
  -target arm-linux-gnueabihf.2.31 -mcpu=cortex_a17 -O2 \
  -Wl,--strip-all -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp tools/es5510_bench.cpp \
  -o artifacts/es5510-emulation/phase2/es5510-bench-armv7
```

Result: PASS. ELF32 ARM EABI5 hard-float, Cortex-A17/ARMv7-A build. The stripped ARMv7 renderer and benchmark are in `phase2/armv7-testkit/`; exact
hashes and package digest are in `PHASE2_RESULTS.txt`.

No `qemu-arm` or Force SSH session was available, so the ARM executable was
not run. ARMv7 execution cost, number of simultaneous engines per Force, and
real-time safety remain unmeasured. Do not extrapolate from the x86 results.

## Repeat the workload

Strict native build:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp tools/es5510_bench.cpp \
  -o /tmp/es5510-bench
```

Examples (put a bundle path after `--program` to execute that actual program):

```sh
/tmp/es5510-bench --rate 44100 --frames 256 --block 64 \
  --program experimental/emulation/programs/original_stereo_feedback_delay.esp
/tmp/es5510-bench --rate 44100 --frames 44100 --block 4096 \
  --program experimental/emulation/programs/original_stereo_chorus.esp
/tmp/es5510-bench --rate 48000 --frames 48000 --block 4096 \
  --program experimental/emulation/programs/original_stereo_feedback_delay.esp
```
