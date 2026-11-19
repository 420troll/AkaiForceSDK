# TROLLfx 5510 — development build

This is a new, separate VST2 stereo effect project. Its product name is **5510**
and its UID is `T551`. It is an experimental MAME-semantics ES5510 instruction
runner, not a recovered DP/4 algorithm collection.

## Current audio programs

Each of four units owns an independent 1,048,576-word 16-bit external DRAM and
executes an 11-word original TROLLfx ES5510 program. Units are always connected
in series: **A → B → C → D**.

- **Delay:** stereo feedback delay. Time changes the read GPRs; signed feedback
  from -99% to +99% changes the program's MAC coefficient; Wet crossfades the
  ES5510 result with the host dry signal.
- **Chorus:** stereo modulated delay. Time is the center delay, Depth is the
  modulation excursion, Rate is a host-calculated LFO schedule written to the
  ES5510 read GPRs, and Wet blends the instruction output with dry audio.
- **Bypass:** passes the unit input through unchanged.

Rate/Depth are active in Chorus; Feedback is active in Delay. These controls
remain available for automation and can be prepared while selecting the other
algorithm. Audio delay storage, feedback, delay reads, instruction mixing and
serial output are executed by ES5510 microinstructions. Host-side float/16-bit
conversion, wet/dry blending and chorus coefficient scheduling are not chip
instructions.

## Build and package

From the SDK repository root:

```sh
python3 tools/build.py plugins/TROLLfx5510 --target all --toolchain zig --zig /path/to/zig
python3 tools/package.py plugins/TROLLfx5510
```

The Force package is a separate folder named `TROLLfx - VST - 5510`; it does
not replace ESP Lab V0.3/V0.4. Installation is intentionally not performed by
this project build.

## Current limitations

- Programs are original diagnostic TROLLfx microprograms, not Ensoniq factory
  programs. No proprietary ROM or firmware is included.
- The standalone engine adapts selected behavior from the pinned MAME ES5510
  model; it is not a proven cycle-accurate DP/4 emulator. Host I/O callbacks
  remain out of scope.
- Program changes take effect at the next audio block and preserve DRAM so the
  callback does not clear megabytes of delay memory. No crossfade is applied.
- Four processors allocate about 8 MiB of external DRAM per plugin instance.
- Native SDK simulation and cross-compilation are not Force hardware tests.
