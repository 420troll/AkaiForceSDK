# TROLLfx 5510 — Rebuild handoff

## Product and boundary

The new plugin is named **5510** (manufacturer TROLLfx), UID `T551`, in
`plugins/TROLLfx5510/`. It is separate from the installed ESP Lab V0.3/V0.4
projects. Their folders and binaries were not modified. The required routing is
fixed **A → B → C → D**.

The plugin has four independent MAME-semantics ES5510 instances. Each instance
owns a separately allocated 1,048,576-word signed 16-bit delay RAM and executes
one of two original 11-instruction TROLLfx programs: stereo feedback delay or
stereo modulated delay/chorus. A third original pass/swap program remains in
the standalone test catalog. Nothing claims these are recovered Ensoniq/DP/4
factory microprograms.

## What is verified

- All effect storage and effect mixing in the delay/chorus programs runs via
  48-bit ES5510 instruction words on the standalone adapter. Delay time,
  feedback, and chorus read positions are set through GPRs. Chorus LFO values
  are host-computed (table lookup) GPR schedules.
- The SDK `host_sim` passed 20 checks at 44.1 kHz / 64-frame processing and
  48 kHz / 128-frame processing, including block sizes 32 and 512, random
  parameter sweeps, save/restore, in-place buffers, silence and finite output.
- Native strict MAME adapter test passed an exact serial stereo-swap program.
- 44.1 kHz, 3-second rendered chorus output changed in 262,059 of 264,600
  samples when compared to the same program with the host LFO schedule removed
  (maximum absolute delta 9,999 PCM counts). Both runs executed 11
  microinstructions per sample. This confirms the host schedule changes audio
  while audio delay/mix work remains in the ES5510 program.
- Native, ARMv7 hard-float and ARM64 builds completed through the official SDK
  build script. Strict SDK validation returned zero errors and warnings; the
  generated TUI and Q-Link maps contain four unit pages and valid handles.
- The per-plugin VST runtime DRAM allocation is 4 × 1,048,576 × 2 bytes = 8 MiB
  (plus small state). Allocation happens in `prepare`, not `process`.

## Commands and results

Toolchain: Zig 0.17.1 at `/tmp/trollfx-zig/ziglang/zig`.

```sh
python3 tools/build.py plugins/TROLLfx5510 --target all --toolchain zig --zig /tmp/trollfx-zig/ziglang/zig
./.sdk-tools/host_sim plugins/TROLLfx5510/build/native/5510.so test --seconds 0.5 --sr 44100 --block 64 --wav /tmp/5510-host.wav
./.sdk-tools/host_sim plugins/TROLLfx5510/build/native/5510.so test --seconds 0.5 --sr 48000 --block 128 --wav /tmp/5510-host-48k.wav
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror experimental/emulation/mame_es5510_standalone.cpp experimental/emulation/mame_es5510_standalone_test.cpp -o /tmp/5510-mame-test
/tmp/5510-mame-test
python3 tools/validate.py 'plugins/TROLLfx5510/build/TROLLfx - VST - 5510' --strict
python3 tools/package.py plugins/TROLLfx5510
```

Both host simulations: PASS (20 passed, 0 warnings, 0 failures). Desktop timing
was 8.39% average / 20.18% worst for 64 frames at 44.1 kHz and 8.69% average /
13.84% worst for 128 frames at 48 kHz. These values are desktop-only and do
not predict Force CPU use. Existing old standalone Force benchmark numbers are
not measurements of this four-engine plugin.

The native renderer produced 132,300-frame 44.1 kHz stereo delay and chorus
WAVs from the diagnostic impulse. On a 3-second sine, LFO-on/off chorus
comparison demonstrated measurable modulation. Temporary WAVs were written
under `/tmp` and are not bundled as release evidence. The earlier impulse
renderer's short impulse produced only four changed samples under modulation;
the sustained-tone comparison is the meaningful modulation check.

## Package and installation

Expected artifact from `tools/package.py`:

`plugins/TROLLfx5510/dist/5510-0.1.0-mpc.zip`

The archive contains the distinct folder `TROLLfx - VST - 5510/`, ARM32
binary `VST/5510-arm32.so`, ARM64 binary, generated Force skin/Q-Links, plugin
metadata, and MAME BSD-3-Clause notice. SHA-256:
`c1061504fd0e38875e76bb22efb48bffcd6432d1d2014f821cb84967525d03ab`.

Installation is **not authorized/performed** by this handoff. For a later
hardware evaluation, copy the complete `TROLLfx - VST - 5510` folder into
`/media/SSD250g/VST2 Plugins/`; do not overwrite any existing folder. Enable
external plugins and rescan/restart the host if needed. Roll back by removing
only the `TROLLfx - VST - 5510` folder. Leave V0.3/V0.4 and RTA untouched.

## Known limitations and next work

1. Run actual Force ARMv7 runtime and four-active-unit CPU tests before asking
   for hardware audition. Prior per-program CPU numbers are not enough.
2. Add dedicated plugin-level rendered comparisons for delay time, signed
   feedback stress, chorus rate/depth and four-unit serial order.
3. Improve program switching (bounded transition/crossfade) if CPU allows;
   current mode switch reloads fixed instruction words at block setup without
   clearing old external DRAM or crossfading.
4. Continue differential tests against the pinned MAME core. This adapter
   implements the necessary execution path but still lacks MAME host I/O and
   full chip/system conformance.
5. Add more original ES5510 programs only after instruction-level render tests.
   Keep DP/4 firmware extraction separate; no factory algorithm was decoded.

## Git status

Branch: `feature/esp-lab-es5510-rebuild`.

Implementation and consolidated research commit:
`13c11576386949248b56c4696e776324719f32c6`.
