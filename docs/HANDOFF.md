# TROLLfx ESP Lab — Development handoff

## Current milestone: ESP Lab Series V0.4

Date: 2026-11-16 UTC

Branch: `issue8-es5510-v03-reverb-lab`.

Source and package commit: `70e423f` (`Add fixed-series ESP Lab V0.4 plugin`).

### Architecture and scope

- New independent VST2 identity: TROLLfx ESP Lab Series V0.4, UID `TfxS`.
- Four independently parameterized stereo units always run in series A → B →
  C → D. The order is not user configurable. Per-unit bypass is a transparent
  pass-through and each unit has its own mix.
- Choices implemented: Delay, Chorus, Flanger, Room, Hall, Plate, Modulated
  Reverb, VCF Distortion, EQ-Panner-DDL, Compressor, Multitap Delay, Dual
  Delay, Eight-Voice Chorus, Parametric EQ, De-Esser, Expander, Gate and Rumble
  Filter.
- The original ten native engines are retained. The last eight are original
  TROLLfx implementations of DP/4 documented effect families; they are not
  recovered factory algorithms. The local TeleBrain Max for Live editor
  extraction supplied names and schema clues (52 tables, 1,137 rows), but is
  excluded from source control and redistribution. Table offsets are not
  treated as physical parameter values.
- Audio processing uses the native C++ DSP engine, not the ES5510 emulator.
  V0.4 is in a separate UID/folder and does not replace the installed `TfxR`
  ES5510 NonLin Reverb plugin.

### Build and verification

- Official Akai SDK: `inmusic-software/mpc-plugin-builder-sdk`, commit
  `febd2ca5bd9eb16c4a412837196e145566bc981b`; Zig 0.17.1.
- Native strict DSP smoke test: PASS for the fixed four-unit chain and all 18
  algorithm selections; finite output at 44.1/48 kHz.
- Official SDK built native Linux, ARMv7 hard-float and ARM64 binaries. ARMv7
  exports `VSTPluginMain`, is UID-matched to `TfxS`, and requires GLIBC ≤2.27.
- Strict SDK validation: PASS, 0 errors, 0 warnings; 5 TUI tabs and Q-Link maps
  validate, all 64 parameters are in range.
- SDK host simulation at 44.1 kHz/128 frames and 48 kHz/4096 frames: PASS, 20
  checks each. Includes 32/512 block handling, parameter sweep, state restore,
  in-place processing, silence and finite-output checks. Desktop timing is not
  a Force CPU measurement.
- Commands:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I. \
  src/original/audio_engine.cpp \
  plugins/TROLLfxESPLabSeriesV04/tests/series_smoke.cpp \
  -o /tmp/series-smoke && /tmp/series-smoke
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 /tmp/mpc-plugin-builder-sdk/tools/build.py \
  plugins/TROLLfxESPLabSeriesV04 --target all --toolchain zig --zig /usr/bin/python3
python3 /tmp/mpc-plugin-builder-sdk/tools/validate.py --strict \
  'plugins/TROLLfxESPLabSeriesV04/build/TROLLfx - VST - ESP Lab Series V0.4'
```

### Package and installation

- Package: `artifacts/esplab-series/v04/trollfx-esp-lab-series-v04-akai-force-armv7.zip`
- SHA-256: `5da46ee9ded6525ca15b65ceb66c9b6fb4acc7b3f468267016fba175a7a0bfa1`
- Package contents include ARMv7 and ARM64 VST binaries, metadata, Force
  touchscreen/Q-Link assets, and instructions.
- Force install path:
  `/media/SSD250g/VST2 Plugins/TROLLfx - VST - ESP Lab Series V0.4`
- After extracting the ZIP on a host, install with (first verify the target
  folder does not already exist):

```sh
cp -a 'TROLLfx - VST - ESP Lab Series V0.4' \
  '/media/SSD250g/VST2 Plugins/'
```

- Deployment status: the TfxS folder has been copied to the Force after
  confirming the destination was absent, `/proc/mounts` had no `noexec` flag,
  and the TfxR folder remained present. Remote and local ARM32 binary SHA-256s
  match (`3cd060044b934745de2d0fe3ce2c7d44f5e26b74fa8f4e3ac01680dd978b1822`).
  The plugin host was not restarted or rescanned, so hardware discovery and
  playback remain unverified.
- Rollback removes only the V0.4 folder:
  `rm -rf '/media/SSD250g/VST2 Plugins/TROLLfx - VST - ESP Lab Series V0.4'`.
  Keep the separate `TfxR` installation and its existing backup untouched.

## Earlier V0.1 Stage 002 handoff

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
