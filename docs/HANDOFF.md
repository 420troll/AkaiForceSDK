# TROLLfx ESP Lab V0.1 — Stage 002 handoff

Date: 2026-11-16 UTC

Stage 002 source and package commit: `ff550e10cf03500beaf102571809bd297545bc11`.

## Build status

- DSP core from `77f4c1e7d2df4310e3a2cb62f322b5c623859ae6` integrated in the
  main project as `c5ca63e` (fractional stereo delay, modulation, bounded
  feedback, cross-feedback and flanger mix).
- Plugin identity: TROLLfx / ESP Lab, VST2 UID `TfxE`; 2-in/2-out effect.
- Official SDK: `inmusic-software/mpc-plugin-builder-sdk`, commit
  `febd2ca5bd9eb16c4a412837196e145566bc981b`.
- Native Linux, ARMv7/Cortex-A17 and ARM64 builds succeeded with Zig 0.17.1.
- SDK validation: 0 errors, 0 warnings. ARMv7 binary exports `VSTPluginMain`,
  requires GLIBC no newer than 2.27, and uses `libm`, `libc`, `libdl` and the
  ARM hard-float loader.
- SDK host simulator: PASS, 20 passed, 0 warnings, 0 failures. Includes 32 and
  512 frame blocks, 200 parameter changes, state round-trip, in-place
  equivalence, finite output, silence and no-denormal checks.
- Standalone DSP smoke build: `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror
  -O2 src/original/fractional_delay_smoke.cpp`; PASS.
- No Force hardware deployment was performed.

## Package and installation

Package: `plugins/TROLLfxESPLab/dist/esp-lab-1.0.0-mpc.zip`

SHA-256: `2271860e5f30dfa6fbfb2e6e7ce878c79ca76e6d2552235c6f13c0f32ea29493`

On the Force, extract the ZIP and copy the contained
`TROLLfx - VST - ESP Lab` directory into the root `VST2 Plugins` directory on
the target drive. The resulting path must be:

```text
<drive>/VST2 Plugins/TROLLfx - VST - ESP Lab/plugin.json
```

Enable External Plugin Support in the Force settings and rescan/restart as
needed. This is an installation instruction only; do not execute it without a
separate deployment request.

## Reproduction

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py plugins/TROLLfxESPLab --target all --toolchain zig --zig /usr/bin/python3
python3 tools/package.py plugins/TROLLfxESPLab
.sdk-tools/host_sim plugins/TROLLfxESPLab/build/native/esp-lab.so test --seconds 2 --wav /tmp/esp-lab-v01.wav
```

## Next steps

1. Review on original Force hardware only after explicit deployment approval.
2. Verify touchscreen sizing and Q-Link groups on hardware.
3. Add smoothed parameter transitions and expand the smoke test for delay
   feedback stability and interpolation edge cases.
4. Keep factory algorithm claims out of product copy; V0.1 is an original
   Ensoniq-inspired effect, not recovered Ensoniq code.
