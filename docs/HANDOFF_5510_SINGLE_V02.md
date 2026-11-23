# TROLLfx 5510 Single V0.2 — handoff

## Release identity and install state

- Product: **TROLLfx - VST - 5510 Single**, V0.2.0
- Plugin UID remains `T55S`.
- Branch: `feature/5510-single-v02`
- Package: `plugins/TROLLfx5510Single/dist/5510-single-0.2.0-mpc.zip`
- ARMv7 binary: `plugins/TROLLfx5510Single/build/TROLLfx - VST - 5510 Single/VST/5510-single-arm32.so`
- Package SHA-256: `edbbdfa2bf29c6c7183db4814dec6f90f1b0db8c80ad1318b98d3d8a4601232b`
- ARMv7 binary SHA-256: `153071d091bbcdc6221aa3478c3ddee37822656465296c2d7aee6075b376471d`

V0.2 is built and staged locally only. It has **not** been copied to the Force. Read-only verification confirmed the installed V0.1 manifest still says 0.1.0/UID `T55S`, its ARM binary remains SHA-256 `f47034935ba1f039820f9215f95654c4361b4af032fd4ba2b8f3de5865c63772`, and MPC remained running. No existing plugin folder was changed, and no restart/rescan was attempted.

Because V0.2 keeps UID `T55S` and the same plugin folder identity for preset compatibility, it must replace V0.1 when approved; do not install it beside V0.1 in the scan directory.

## DSP and parameters

The existing ES5510 delay and chorus microprograms and their stereo processing remain in use. One MAME-derived ES5510 instance owns a 1,048,576-word delay RAM. No ASR-10 factory program is included.

The first seven parameters keep their V0.1 order and stable IDs. Six are appended: Tempo Sync, Beat Division, Fallback BPM, Fine Offset, Stereo Spread, and Sync Range. A V0.1-to-V0.2 state test confirms all seven old values migrate unchanged and the appended values start at their V0.2 defaults.

- Delay remains free-running in milliseconds with Sync off. Feedback remains bipolar at −99% through +99%; custom display formatting shows a sign and makes zero explicit.
- With Sync on, the plugin uses valid VST host tempo when available, otherwise Fallback BPM. Straight, dotted, and triplet note divisions cover 1/32 through whole notes, plus two bars. Two bars uses the current host time signature (4/4 fallback when the host does not provide one). Fine Offset is ±250 ms.
- VST transport is fetched through the SDK's verified `audioMasterGetTime`/`VstTimeInfo` path. `Transport::tempoValid` is separate from `playing`; the plugin continues to use a valid tempo while stopped. Tempo and division are checked every audio block, and delay read position glides toward the new target.
- Delay RAM capacity remains 441,000 samples per side (10 seconds at 44.1 kHz). A division plus offset outside the available range is not clamped to a different note: the plugin passes dry and reports `Sync Range: Out of Range`.
- Chorus retains the V0.1 rate, depth, delay, and opposite-phase stereo character at the 100% Stereo Spread default. Spread moves the two LFO read schedules from in-phase at 0% to opposite-phase at 100%. Chorus tempo sync is not implemented.
- Wet/bypass crossfade is smoothed over about 10 ms. Feedback, rate, depth, and spread controls glide over about 15 ms; delay-time changes retain the existing 20 ms glide.
- Audio processing uses no callback file I/O, dynamic allocation, or blocking synchronization. No conventional audio DSP replaced the ES5510 programs.

## Force interface and Q-Links

The generated skin contains separate DELAY and CHORUS tabs. `visibleWhen` updates the shown algorithm controls when the algorithm choice changes. The selected tab itself does not change automatically: inspection of the SDK UI format found fixed tabs, choice-conditioned controls, and per-tab static Q-Link maps, but no verified parameter-driven tab-switch action. Tap the other tab manually after changing algorithms.

The local `tools/gen_ui.py` convention and physical mapping mechanism are retained. `tools/finish_ui.py` applies the two per-tab backplate tints and assigns Q1–Q8 in the generated Q-Link file:

| Q-Link | DELAY tab | CHORUS tab |
|---|---|---|
| Q1 | Free Delay Time | LFO Rate |
| Q2 | Bipolar Feedback | LFO Depth |
| Q3 | Wet | Wet |
| Q4 | Tempo Sync | Stereo Spread |
| Q5 | Beat Division | Center Delay Time |
| Q6 | Fine Offset | Unassigned |
| Q7 | Bypass | Bypass |
| Q8 | Algorithm | Algorithm |

Q9–Q10 on DELAY provide Fallback BPM and Sync Range. Since SDK Q-Link assignments are static for each tab, Q1 remains mapped to Time while Sync is on; use Q5 for the synced division. This limitation is a host UI mapping constraint, not a nonfunctional DSP parameter.

## Build and test results

SDK snapshot: official InMusic MPC plugin builder SDK `febd2ca5bd9eb16c4a412837196e145566bc981b` (MIT), as recorded in `docs/SDK_COMPATIBILITY.md`. Toolchain: Zig 0.17.1, sequential build through the checked-in builder:

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py plugins/TROLLfx5510Single --target all --toolchain zig --zig /usr/bin/python3
python3 plugins/TROLLfx5510Single/tools/finish_ui.py \
  'plugins/TROLLfx5510Single/build/TROLLfx - VST - 5510 Single'
python3 tools/validate.py \
  'plugins/TROLLfx5510Single/build/TROLLfx - VST - 5510 Single' --strict
python3 tools/package.py plugins/TROLLfx5510Single
```

Results:

- Native, ARMv7 hard-float/Cortex-A17, and ARM64 builds passed. ARMv7 is ELF32 ARM EABI5, exports `VSTPluginMain`, and requires at most GLIBC 2.27. ARM64 requires at most GLIBC 2.29.
- Strict SDK validation: **0 errors, 0 warnings**, including two tabs, all handles/images, 13 parameters, and two Q-Link tab maps.
- Default host simulation: **20 passed, 0 warnings, 0 failures** at 44.1 kHz/64 frames; finite output, peak −4.58 dBFS, no clipping. Mean desktop callback cost 1.79% of the 64-frame time budget; worst-block percentage varies with host/desktop load. These are not Force measurements.
- Delay negative, zero, and positive feedback endpoint host tests each passed 20/20 without clipping; +99% peaked just below 0 dBFS under the simulator's −12 dBFS signal plus transient test burst.
- Chorus host simulations at 0% and 100% stereo spread both passed. Default 100% retains V0.1 opposite-phase motion.
- Host-tempo sync passed at 90, 120, and 140 BPM. A simulated live change from 90 to 140 BPM changed the 2-second rendered WAV versus fixed 90 BPM (different SHA-256) while remaining finite and passing 20/20 host checks.
- Out-of-range two-bar sync at 30 BPM reports `Out of Range`; the dry safety path was exercised. Two-bar durations at 90 BPM and 3/4 and 4/4 math are covered by the strict tempo unit test.
- `tempo_math_test.cpp`, compiled with `-Wall -Wextra -Wpedantic -Werror`, passed straight/dotted/triplet, BPM 90/120/140, meter, fine-offset, and over-capacity cases.
- `test_v01_state_migration.sh` passed against V0.1 commit `40d4e8ed4bf5ca522137478d1576675ae6b6cf70`.
- No V0.2 Force audition, ARM hardware CPU measurement, or physical screen/Q-Link verification has been performed.

## Install and rollback — requires later approval

The ZIP is staged locally at the path above. Do not extract it over the existing V0.1 directory until the project is saved, a complete backup is verified, and replacement is explicitly approved. The Force SSD is exFAT with `noexec`; do not change its mount options. Existing external plugins already load from this configured volume.

When ready to replace V0.1, recheck SSH, architecture, destination, active MPC state, and hashes. Save the project and avoid interrupting an active session. Copy the package to `/tmp`, validate its SHA-256, then use a temporary extraction directory outside the plugin scan root. Move the old folder to a backup location outside `VST2 Plugins` before moving V0.2 into the scan root:

```sh
scp plugins/TROLLfx5510Single/dist/5510-single-0.2.0-mpc.zip akai-force:/tmp/5510-single-0.2.0-mpc.zip
ssh akai-force 'set -eu; target="/media/SSD250g/VST2 Plugins/TROLLfx - VST - 5510 Single"; backup="/media/SSD250g/TROLLfx-backups/5510-single-v0.1"; stage="/media/SSD250g/.5510-single-v0.2-stage"; test "$(uname -m)" = armv7l; test -d "$target"; test ! -e "$backup"; test ! -e "$stage"; mkdir -p "$(dirname "$backup")"; mkdir "$stage"; unzip -tq /tmp/5510-single-0.2.0-mpc.zip; unzip -q /tmp/5510-single-0.2.0-mpc.zip -d "$stage"; candidate="$stage/TROLLfx - VST - 5510 Single"; test "$(grep -c T55S "$candidate/plugin.json")" -ge 1; test "$(sha256sum "$candidate/VST/5510-single-arm32.so" | cut -d " " -f1)" = 153071d091bbcdc6221aa3478c3ddee37822656465296c2d7aee6075b376471d; chmod 755 "$candidate/VST/5510-single-arm32.so"; mv "$target" "$backup"; mv "$candidate" "$target"; rmdir "$stage"; sha256sum "$target/VST/5510-single-arm32.so"'
```

Do not restart MPC automatically. If V0.2 does not appear in External Plugins, save the session and obtain approval before any host restart. Roll back only this plugin by moving V0.2 aside/removing that exact folder and restoring the backup:

```sh
ssh akai-force 'set -eu; target="/media/SSD250g/VST2 Plugins/TROLLfx - VST - 5510 Single"; backup="/media/SSD250g/TROLLfx-backups/5510-single-v0.1"; test -d "$backup"; rm -rf -- "$target"; mv "$backup" "$target"'
```

## Next step

Review the package and Q-Link assignments, then authorize a V0.1 backup and replacement when hardware testing is safe. Audition free/synced delay, signed feedback, tempo changes, both chorus stereo-spread extremes, and preset recall on the Force. Keep factory ASR-10 capture loading as a separate later task.
