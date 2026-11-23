# TROLLfx 5510 Single V0.1 — hardware test handoff

## Release identity

- Product: **TROLLfx - VST - 5510 Single**, version 0.1.0
- Plugin UID: `T55S` (audited against checked-in plugin manifests; distinct from four-unit `T551`, ESP Lab `TfxE`, and ESP Lab V0.2/V0.3 UIDs)
- Development branch: `feature/5510-single-hwtest`
- Package: `plugins/TROLLfx5510Single/dist/5510-single-0.1.0-mpc.zip`
- Package SHA-256: `2acc0552d96e8b7be9836ac1add5f19873d7144f742c5d302229dacbe1279c29`
- ARMv7 binary SHA-256: `f47034935ba1f039820f9215f95654c4361b4af032fd4ba2b8f3de5865c63772`

The package is a local build artifact and is git-ignored; only source, metadata, UI, licensing, and this handoff are committed.

## DSP actually included

The plugin embeds one MAME-derived standalone ES5510 engine and its own 1,048,576-word external delay RAM. It exposes two **original TROLLfx diagnostic programs**, `TROLLfx Delay` and `TROLLfx Chorus`, with algorithm, time, feedback, rate, depth, wet, and bypass controls. Their audio path executes ES5510 microinstructions; the chorus LFO schedule is host-computed. The diagnostic programs use the existing MAME pump DOL/SER3 output route.

The private ASR-10 captures `44DDL-chorus-linux.esp` and `hall-os353.esp` were located and reviewed, but are not in the plugin or ZIP. They are runtime snapshots missing transient processor pipeline state and are not yet safe, fresh initialization bundles for this plugin. Therefore this release does **not** claim to load either Ensoniq factory effect or route a factory program through ASR SER1. Captures, firmware, effect bytes, and WAVs remain outside Git and the package. The selected diagnostic program is clearly labeled as TROLLfx in the host algorithm choices and README.

## Build and validation

Official Akai/InMusic SDK snapshot documented in `docs/SDK_COMPATIBILITY.md`: upstream commit `febd2ca5bd9eb16c4a412837196e145566bc981b` (MIT). The project builder is the checked-in SDK build/validation tooling.

Build command (Zig 0.17.1):

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py plugins/TROLLfx5510Single --target all --toolchain zig --zig /usr/bin/python3
```

Results:

- Native Linux build: passed.
- ARMv7 hard-float/Cortex-A17 build: passed; 32-bit ARM EABI5 shared object, maximum GLIBC requirement 2.27; exports `VSTPluginMain`.
- ARM64 build: passed; maximum GLIBC requirement 2.29.
- Strict SDK validator: **0 errors, 0 warnings**; UID and binary identity match; one touch page and one Q-Link tab validate.
- Native `host_sim` at 44.1 kHz, 64-frame blocks: **20 passed, 0 warnings, 0 failures**. It rendered finite nonzero audio (peak -4.58 dBFS), tested 32/512-frame calls, parameter sweeps, state round trip, in-place processing, silence, no clipping, and teardown.
- Average native desktop processing cost: 1.83% of a 64-frame block; worst measured block 8.10%. This is not a Force CPU measurement.
- No hardware load, Force discovery, or audition has been performed.

## Force installation (completed; load/audition pending)

- SSH alias `akai-force` resolved to `192.168.50.47`; Force `wlan0` confirmed `192.168.50.47/24`.
- Force identity: hostname `force`, `armv7l`, kernel `6.18.44-az01-2026-08-10-rt6`.
- `/media/SSD250g`: 223.6 GB total, 47.7 GB used, 175.9 GB available.
- The destination folder was confirmed absent before installation. It now exists at `/media/SSD250g/VST2 Plugins/TROLLfx - VST - 5510 Single/`.
- The downloaded ZIP hash was verified on the Force before extraction. The ARMv7 binary hash after installation is `f47034935ba1f039820f9215f95654c4361b4af032fd4ba2b8f3de5865c63772`, matching the local release.
- Binary mode is `-rwxr-xr-x`; metadata UID is `T55S`. The temporary extraction directory and transferred `/tmp/5510-single-0.1.0-mpc.zip` were removed after verification.
- The original VST2 directory contained the existing TROLLfx and RTA folders; none was modified or replaced.
- SSD mount options include `noexec`. No mount changes were made.
- MPC process PID 328 remained running. The new `.so` was not present in `/proc/328/maps` at verification time. Therefore installation is verified, but host discovery, plugin loading, and audio audition are **not** verified. No restart or scan action was triggered.

Commands used from the development computer:

```sh
scp plugins/TROLLfx5510Single/dist/5510-single-0.1.0-mpc.zip akai-force:/tmp/5510-single-0.1.0-mpc.zip
ssh akai-force '...verify package SHA-256; unzip to a unique staging directory; confirm UID T55S; verify ARMv7 binary SHA-256; move only the new folder into "/media/SSD250g/VST2 Plugins"...'
```

### Load and audition

On the Force, open an audio track or mixer channel that carries live audio, tap an unused insert slot, then choose **External Plugins → TROLLfx → 5510 Single**. Its one touchscreen page / Q-Link bank exposes Algorithm, Time, Feedback, Wet, Rate, Depth, and Bypass. Set Bypass off and Wet to taste. Choose **TROLLfx Delay** and adjust Time/Feedback; then choose **TROLLfx Chorus** and adjust Rate/Depth. Check the track's stereo output meters while auditioning.

If the plugin is absent from the External Plugins browser, do not interrupt an active session. Save the project and request/perform a plugin rescan or MPC restart only when ready; neither has been performed here.

Rollback affects only this plugin folder:

```sh
ssh akai-force 'rm -rf -- "/media/SSD250g/VST2 Plugins/TROLLfx - VST - 5510 Single"'
```

## Next milestone

Restore Force network reachability, verify the live target state, install this distinct package without replacing anything, and audition live stereo input with `TROLLfx Delay` and `TROLLfx Chorus`. Capture host discovery/load evidence and real Force CPU use. Continue factory capture initialization separately; do not put the current runtime snapshots into the callback or advertise them as clean Ensoniq programs.
