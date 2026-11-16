# ES5510 V0.3 Issue #9 — Decay, Feedback and Gate Handoff

## Status

- Branch: `issue8-es5510-v03-reverb-lab`
- Starting point: `49434969b3b95d5af1b7eacf113d1effa1f8be46`
- Implementation and package commit: `901fc065c6c5806883c253f434470ad4b4809fa5` (audio WAV fixtures are committed immediately after it).
- Plugin UID remains `TfxR`; the 54 existing parameter indices and serialized state layout are unchanged.
- Audio remains processed by the standalone MAME-derived ES5510 instruction engine. The host schedules controls and envelopes; no native reverb substituted for the instruction programs.
- The corrected ARMv7 package has been installed on the Force SSD at `/media/SSD250g/VST2 Plugins/TROLLfx - VST - ES5510 NonLin Reverb V0.3`. The previous directory is preserved as `TROLLfx - VST - ES5510 NonLin Reverb V0.3.backup-issue9`. The installed ARMv7 binary SHA-256 matches the local build (`bd8ff36cfd56c19a42e6572c5d4175783360da3faae984565b5d2cfc7727d776`). The plugin host was not restarted or rescanned, so no post-install audio test has occurred.

## Changes and diagnosis

`ReverbBank::setControls()` wrote the feedback GPR from the requested decay using the delay-loop time, then clamped that result to `0.94`. Above the point where the formula exceeded 94%, longer settings wrote the same feedback coefficient. This caused the upper Decay region to flatten. Feedback conversion uses signed/unsigned Q22 and the ES5510 program's feedback GPR (GPR 40 in Room/Hall/Gated, descriptor-selected GPRs for the plates).

The blanket 94% cap is removed. Plate, Hall, Gated and the other feedback paths can reach 99% (0.99, still below unity). A Room-specific top-end mapping compresses the theoretical coefficient above 91% into a continuous maximum of 97.5%. The short single-diffuser Room loop showed sample-rate-dependent tail collapse at higher coefficients; the reduced ceiling keeps it out of that measured region. Thus 99% is exposed where the measured instruction path supports it, while Room has a conservative architecture-specific stability limit. Delay's host parameter now reaches 99%; its V02 processor clamp was raised from 94% to 99%.

NonLin 1/2/3 are one-pass tap programs, not feedback reverbs. Their former `clamp(decay / 1.8, 0.25, 1.0)` reached maximum tap length at 1.8 seconds, leaving most of the displayed range ineffective. The existing `Decay` parameter now maps across its complete 0.2–20 range to 25–100% of the one-pass tap schedule. Its UI label is `Decay / Tap Length`; the formatted display shows tap span for NonLin and seconds for reverberant algorithms. Built-in NonLin presets select the full tap span. The parameter ID/index stays the same, preserving saved states.

The gate previously compared each input sample directly with one threshold. A waveform near the threshold could toggle the detector on adjacent samples, and threshold-to-linear-envelope control happened with no detector hysteresis. It now uses instant peak capture plus a 6 ms release/hold and a Schmitt detector that closes at half the open threshold (about 6 dB hysteresis). Threshold conversion and gate attack/hold/release sample counts are calculated at block control rate rather than with `pow()`/time conversions on every sample. The existing 1 ms minimum Gate Attack remains available for intentionally fast, glitch-prone transitions; default attack/hold/release behavior is smoothed. Reverb RAM is retained so normal tails and retrigger behavior remain available.

## Render and stress measurements

Command used for the strict native capture/test:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -Iexperimental/emulation \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/vst_host/TempoDelayControl.cpp \
  experimental/emulation/vst_host/v02/Es5510V02Processor.cpp \
  experimental/emulation/vst_host/v03/ReverbBank.cpp \
  experimental/emulation/vst_host/v03/tests/issue9_reverb_response_test.cpp \
  -o /tmp/issue9-response
/tmp/issue9-response /tmp/issue9-wavs-final > /tmp/issue9-results-final.csv
```

The test ran 44.1 and 48 kHz impulse responses at Decay 0.5, 2 and 20 seconds for Small Plate, Large Plate, Room and Hall. It measured RMS over 0–1, 1–3, 3–6 and 6–12 second windows, peak level and the last sample above `1e-4` (-80 dBFS). The fixed-window tail endpoint is reported as at least 12 s when still audible at the end of the render.

Selected tail-window RMS (3–6 s) and measured tail endpoint:

| Algorithm | 44.1 kHz, Decay 0.5 / 2 / 20 s: RMS; tail ms | 48 kHz, Decay 0.5 / 2 / 20 s: RMS; tail ms |
|---|---|---|
| Small Plate | `3.06e-5 / 3.23e-5 / 3.20e-3`; `1308 / ≥11988 / ≥11988` | `3.06e-5 / 3.22e-5 / 2.95e-3`; `1218 / ≥11986 / ≥11986` |
| Large Plate | `8.13e-7 / 3.27e-6 / 2.24e-3`; `1386 / ≥11993 / ≥11993` | `7.80e-7 / 3.12e-6 / 2.06e-3`; `1263 / ≥11998 / ≥11998` |
| Room | `1.06e-6 / 4.50e-4 / 8.43e-6`; `3327 / 7469 / 6007` | `1.61e-6 / 4.60e-6 / 9.85e-6`; `3377 / 5395 / 6269` |
| Hall | `8.95e-6 / 2.81e-4 / 8.94e-4`; `5137 / 11557 / 11557` | `6.11e-6 / 2.26e-4 / 8.10e-4`; `4750 / 11875 / 11875` |

These responses are not guaranteed to be monotonically increasing at every intermediate Decay value: the short feedback networks have fixed-point comb cancellation and sample-rate-sensitive quantization. The endpoint settings produce clear differences. Room's measured upper setting is longer than its minimum, but its intermediate response is especially non-monotonic; the 97.5% ceiling limits the worst upper feedback region without pretending the original microcode or an exact RT60 response has been recovered.

NonLin tap endpoints at 0.2 versus 20 on the same impulse were:

| Algorithm | 44.1 kHz | 48 kHz |
|---|---:|---:|
| NonLin 1 | 125 / 500 ms | 125 / 500 ms |
| NonLin 2 | 374 / 1497 ms | 374 / 1497 ms |
| NonLin 3 | 8.7 / 34.7 ms | 8.7 / 34.7 ms |

Stereo delay impulse feedback stress (0.12 s delay) tested 0, 50, 94 and 99% at both rates. At 44.1 kHz, 1–3 s stereo RMS was `0 / 9.95e-6 / 0.00371 / 0.00768`; at 48 kHz it was `0 / 9.54e-6 / 0.00356 / 0.00736`. Peaks were 0.665 in all cases. The 99% tail RMS is over twice the 94% result; no NaN/Inf or output over 0.98 was observed.

Hot-beat stress covered both rates and all feedback reverb modes. Peaks remained finite; the V0.3 stereo-linked output safety limiter held the maximum at 0.98. Gate retrigger stress used repeated 110 Hz bursts with 250 ms spacing and 90 ms gate hold. Post-input samples above -80 dBFS lasted 3166 samples (71.8 ms) at 44.1 kHz and 3872 (80.7 ms) at 48 kHz, consistent with the configured release. This validates a deterministic release tail and silence handling; it does not prove that all threshold-noise patterns are inaudible on hardware.

CSV and matched stereo impulse WAVs are in `artifacts/es5510-emulation/v03/issue9-final/`. WAVs use identical 44.1 kHz stereo impulses and wet-only output at Decay 0.5 and 20 seconds for each feedback reverb, plus the 99% delay render. Full 44.1/48 kHz measurements are in `MEASUREMENTS.csv`.

## Build and validation

Official Akai MPC Plugin Builder SDK checkout: `/tmp/mpc-plugin-builder-sdk`, commit `febd2ca5bd9eb16c4a412837196e145566bc981b`; Zig 0.17.1.

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 /tmp/mpc-plugin-builder-sdk/tools/build.py \
  plugins/TROLLfxES5510ReverbV03 --target all --toolchain zig \
  --zig /usr/bin/python3
python3 /tmp/mpc-plugin-builder-sdk/tools/validate.py --strict \
  plugins/TROLLfxES5510ReverbV03
```

Native, ARMv7 hard-float and ARM64 binaries built successfully. SDK strict validation: 0 errors, 0 warnings; UID `TfxR`; VST2 entry point, 2-in/2-out, six skin tabs and Q-Link maps valid. The ARMv7 plugin is 265 KB and requires max GLIBC 2.27.

Host simulation passed 20/20 checks at 44.1 kHz / 128 frames and 48 kHz / 4096 frames, including 32/512 frame re-prepare blocks, finite output during parameter sweeps, in-place processing, silence, and byte-identical 54-parameter state restoration. This is desktop simulation, not a hardware real-time measurement.

The strict native `issue9_reverb_response_test` and existing ten-mode `reverb_bank_smoke` pass. Phaser regression at 44.1/48 kHz passes with requested feedback `+/-0.9`, zero output clipping, and preserved notches (-64.3/-64.7 dB at 44.1 kHz; -64.9/-71.0 dB at 48 kHz). The phaser's actual stable feedback remains internally bounded to +/-0.75; the V0.3 UI continues to present that truthful range.

## Release package

- ZIP: `artifacts/es5510-emulation/v03/trollfx-es5510-v03-nonlinear-reverb-akai-force-armv7.zip`
- SHA-256: `8e822326819a5fc355b570ed520f9460e1f0d0f5c4a8cc15a10a1e596e6927de`
- Folder: `TROLLfx - VST - ES5510 NonLin Reverb V0.3`
- The package includes ARM32 and ARM64 VST binaries, V0.3 metadata/skin and third-party notices.

Recreate and verify:

```sh
python3 tools/package_es5510_v03.py
sha256sum artifacts/es5510-emulation/v03/trollfx-es5510-v03-nonlinear-reverb-akai-force-armv7.zip
unzip -t artifacts/es5510-emulation/v03/trollfx-es5510-v03-nonlinear-reverb-akai-force-armv7.zip
```

## Safe Force replacement and rollback

The connected Force is reachable as SSH alias `akai-force`. The Issue #9 ZIP was copied to `/tmp/trollfx-es5510-v03-issue9.zip`, staged and installed at the path above. The prior V0.3 folder was preserved by rename. The installed ARMv7 binary was SHA-256 checked against the local build. The Force SSD mounts at `/media/SSD250g` without `noexec`; inspect `/proc/mounts` again before future writes because exFAT mount options can change. No persistent mount configuration was changed. Use POSIX `sh` (no Bash-specific syntax).

For a future replacement on another install, preserve the old V0.3 directory by an atomic rename before swapping. The commands below show the completed procedure; the current Force already has the backup directory, so the `test ! -e "$BACKUP"` guard correctly stops a second run:

```sh
scp artifacts/es5510-emulation/v03/trollfx-es5510-v03-nonlinear-reverb-akai-force-armv7.zip \
  akai-force:/tmp/trollfx-es5510-v03-issue9.zip
ssh akai-force

set -eu
ROOT='/media/SSD250g/VST2 Plugins'
LIVE="$ROOT/TROLLfx - VST - ES5510 NonLin Reverb V0.3"
BACKUP="$ROOT/TROLLfx - VST - ES5510 NonLin Reverb V0.3.backup-issue9"
STAGE="$ROOT/.trollfx-es5510-v03-issue9-stage"
ZIP='/tmp/trollfx-es5510-v03-issue9.zip'
grep ' /media/SSD250g ' /proc/mounts
case " $(awk '$2 == "/media/SSD250g" {print $4}' /proc/mounts) " in
  *' noexec '*) echo 'SSD is mounted noexec; stop before replacement'; exit 1 ;;
esac
test -d "$LIVE" && test ! -e "$BACKUP" && test ! -e "$STAGE"
mkdir "$STAGE"
unzip -q "$ZIP" -d "$STAGE"
NEW="$STAGE/TROLLfx - VST - ES5510 NonLin Reverb V0.3"
test -f "$NEW/plugin.json"
grep -q 'TfxR' "$NEW/plugin.json"
test -f "$NEW/VST/es5510-nonlin-reverb-v03-arm32.so"
# Retain the prior install; if the second rename fails, restore it.
mv "$LIVE" "$BACKUP"
if ! mv "$NEW" "$LIVE"; then mv "$BACKUP" "$LIVE"; exit 1; fi
rmdir "$STAGE"
```

Rollback, preserving the corrected build for inspection:

```sh
set -eu
ROOT='/media/SSD250g/VST2 Plugins'
LIVE="$ROOT/TROLLfx - VST - ES5510 NonLin Reverb V0.3"
BACKUP="$ROOT/TROLLfx - VST - ES5510 NonLin Reverb V0.3.backup-issue9"
FAILED="$ROOT/TROLLfx - VST - ES5510 NonLin Reverb V0.3.issue9-failed"
test -d "$LIVE" && test -d "$BACKUP" && test ! -e "$FAILED"
mv "$LIVE" "$FAILED"
mv "$BACKUP" "$LIVE"
```

After replacement, restart/rescan the Force plug-in host before testing audio. Verify state restoration, decay range, hot beat feedback, silence/retrigger and CPU use. Do not delete the `.backup-issue9` directory until the corrected release has passed hardware checks. To restore immediately if the replacement folder fails to move, rename the backup back to the live folder. No Force reboot or plugin-host restart has been run; the new package is installed on disk but not yet scanned by the live host.

## Remaining limits

- Hardware audio and CPU validation of this corrected binary are still outstanding.
- Room feedback tops out at 97.5% based on measured short-loop stability; other reverb feedback paths reach 99%.
- Room tail measurements vary with sample rate and feedback-loop comb phase. Values represent rendered fixed-point behavior; they are not accurate DP/4 RT60 measurements.
- Gate retrigger was tested with repeated tone bursts and silence. The DP/4 trigger detector and proprietary firmware behavior remain unrecovered.
- The final output limiter can engage on maximum feedback/hot-beat stress. Output stayed at or below 0.98; internal ES5510 overflow flags are not exposed by the standalone API.
