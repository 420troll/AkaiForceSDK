# TROLLfx ES5510 Experimental VST V0.1 — Handoff

## Release

- Branch: `issue4-es5510-standalone`
- Starting Phase 2 commit: `fdc25514e6c74b9ae0e3fe1da6ca02f62e11a429`
- Plugin: `TROLLfx - VST - ES5510 Experimental` (SDK folder identity); host label `ES5510 Experimental` by TROLLfx
- VST2 UID: `TfxX`; 2-in/2-out stereo effect
- Modes: DELAY (original TROLLfx 19-word ES5510 program) and CHORUS (original TROLLfx 11-word ES5510 program)
- Both programs are original research examples; they are not Ensoniq factory programs or claims of DP/4 compatibility.

## What is implemented

`Es5510EffectProcessor` embeds the standalone MAME-derived ES5510 core and runs both programs through its instruction execution path. The chorus LFO is host-computed and updates the program's delay-address GPRs in opposite phase; its delay storage and audio processing still execute as ES5510 instructions. The adapter preallocates two 1 MiB-word delay RAM vectors during `prepare`, supports 44.1/48 kHz and arbitrary block lengths, smooths parameter/mode/bypass changes, supports in-place stereo processing, and does no callback allocation, locking, file access or program parsing.

The DELAY page exposes Time (1–10,000 ms), Feedback (0–94%), Wet and Dry. CHORUS exposes Rate (0.05–10 Hz), Depth (0–20 ms) and Wet; the Dry parameter is hidden there because the original 11-word chorus program already adds its direct input. The two generated Force UI tabs use the SDK control kit and Q-Link banks. SDK host parameter chunks and the two built-in programs preserve/restore state.

MAME BSD-3-Clause terms are in `experimental/emulation/THIRD_PARTY_NOTICES.md` and included in the ZIP. No firmware or proprietary effect programs are included.

## Build and verification

- Official SDK: `/tmp/mpc-plugin-builder-sdk`, pinned local checkout `febd2ca5bd9eb16c4a412837196e145566bc981b`; Zig 0.17.1.
- Build command:

  ```sh
  python3 /tmp/mpc-plugin-builder-sdk/tools/build.py plugins/TROLLfxES5510Experimental \
    --target all --toolchain zig --verbose
  ```

- Result: native, ARM32 and ARM64 builds passed. ARM32 binary is 209,904 bytes; the SDK's ELF validator reports 32-bit ARM, `VSTPluginMain`, and maximum GLIBC requirement 2.27. ARM64 is included as an optional extra.
- Strict native syntax build passed with `g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -fsyntax-only` for the wrapper and processor translation units.
- SDK validation: 0 errors, 0 warnings; identity, channel counts, UID, both skins and Q-Link maps validated.
- Host simulator: 16 invocations across 44.1/48 kHz and declared block sizes 32, 64, 96, 128, 192, 256, 512 and 4096. Each reported 20 passed, 0 warnings, 0 failures, including finite audio, in-place processing, automation sweeps and state chunk round-trip. These are desktop host-simulator results, not Force measurements.
- Direct adapter-vs-standalone renderer comparison: delay and chorus each matched sample-for-sample after PCM conversion at both rates. At 44.1 kHz, each comparison covered 176,400 frames; at 48 kHz, each covered 96,000 frames. Maximum PCM delta was 0. Tests used mixed 128/4096-frame chunks. The comparison harness was temporary and not committed; existing Phase 2 renders and programs are retained in the repository.
- Native and ARM32 binaries compile the same standalone processor source, not a generic C++ effect substitute.

## Expected CPU and limitations

On the user's Force, prior standalone benchmarks were 2.36921 µs/frame delay and 1.74279 µs/frame chorus at 44.1 kHz, and 2.35868 µs/frame delay and 1.67872 µs/frame chorus at 48 kHz. The adapter currently runs both engines to retain their histories and crossfade mode changes. Summing those existing timings gives a rough estimate of 18.13% of one core at 44.1 kHz and 19.38% at 48 kHz, before host-wrapper overhead. This is an inference, not a measurement of the VST in the host. No plugin was installed or run on hardware.

The skin has only been SDK-generated and host-validated, not viewed on a Force. Hardware CPU, scanning, sound, mount permissions, and compatibility with the specific AZ0x image remain for user testing. The Force target must provide the SDK's ARM32 runtime ABI; the built plugin's maximum GLIBC requirement is 2.27. Supported DSP rates are 44.1 and 48 kHz; unsupported rates produce silence after prepare fails.

## Package

- Path: `artifacts/es5510-emulation/vst-v01/trollfx-es5510-experimental-v0.1-akai-force-armv7.zip`
- SHA-256: `f4562ff61a8d4b0c46a8f78038da44717db1dceea1dd725393ea662769641bf9`
- Contains the ARM32 plugin, optional ARM64 plugin, `plugin.json`, `version.xml`, generated `Plugin Skins/TUI.json` and `Q-Links.json`, skin assets, installation instructions and the MAME license notice.
- Package creator: `tools/package_es5510_vst.py`; archive member timestamps are fixed for repeatable packaging.

Rebuild and package:

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 /tmp/mpc-plugin-builder-sdk/tools/build.py plugins/TROLLfxES5510Experimental \
  --target all --toolchain zig --zig /usr/bin/python3
python3 /tmp/mpc-plugin-builder-sdk/tools/validate.py \
  'plugins/TROLLfxES5510Experimental/build/TROLLfx - VST - ES5510 Experimental' \
  --params plugins/TROLLfxES5510Experimental/build/params.json --strict
python3 tools/package_es5510_vst.py
sha256sum artifacts/es5510-emulation/vst-v01/trollfx-es5510-experimental-v0.1-akai-force-armv7.zip
```

## Installation and scan

Do not unpack over an existing plugin directory. Copy the ZIP to the SSD root, check mount options using the command in `plugins/TROLLfxES5510Experimental/INSTALL.txt`, then run the included POSIX `sh` commands. They refuse to extract if the destination plugin directory already exists and `unzip -n` does not overwrite files. If `/proc/mounts` shows `noexec`, stop; no mount setting was changed during this work.

Enable External Plugin Support in Force Preferences > Enhancements (Beta), then restart the Force to rescan. This instruction has not been executed. No Force plugin was installed or replaced.

## Commits

Phase 2 base: `fdc25514e6c74b9ae0e3fe1da6ca02f62e11a429`.

- Wrapper/UI scaffold: `f78a741`, `cff5104`, `d2ce71b`
- Real-time processor adapter: `d936b1a`, `1dbfd8a`
- VST/package release milestone: `a950ea1c788b2170719cc967c68f78a368fd8cfc`

This handoff update is a docs-only commit immediately after that release milestone.

## Next steps

1. Review and push the release branch.
2. Perform the documented install on the original Force after reviewing the SSD mount's `exec/noexec` options.
3. Confirm plugin scan, DELAY/CHORUS selection, parameter response, state restoration, stereo output and CPU load at 44.1/48 kHz with multiple buffer sizes.
4. Keep this experimental plugin separate from ESP Lab V0.1–V0.3 and do not claim factory DP/4 compatibility.
