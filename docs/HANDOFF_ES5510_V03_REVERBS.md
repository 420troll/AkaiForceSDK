# TROLLfx ES5510 V0.3 — Nonlinear & Reverb Lab handoff

## Release identity and status

- Branch: `issue8-es5510-v03-reverb-lab`
- Plugin host name: `ES5510 NonLin Reverb V0.3`
- Force folder: `TROLLfx - VST - ES5510 NonLin Reverb V0.3`
- VST2 UID: `TfxR` (separate from ESP Lab V0.3 `Tfx3`, ES5510 V0.1 `TfxX`, and V0.2 `TfxV`)
- Plugin is one consolidated stereo effect containing delay, chorus, phaser,
  panner, rotary, three nonlinear tap reverbs, two plates, room, hall, gated,
  and two reverse modes. The old plugin folders are not build/runtime
  dependencies.
- V0.3 has not been installed or tested on a Force. It remains alongside older
  installed prototypes until the user verifies this build.

Repository UID audit (source manifests and matching wrapper identities):

| Plugin | UID |
|---|---|
| TROLLfx ESP Lab | `TfxE` |
| TROLLfx ESP Lab V0.2 | `Tfx2` |
| TROLLfx ESP Lab V0.3 | `Tfx3` |
| ES5510 Experimental V0.1 | `TfxX` |
| ES5510 Expanded V0.2 | `TfxV` |
| ES5510 NonLin Reverb V0.3 | `TfxR` |

The six source-manifest UIDs are unique. `tools/package_es5510_v03.py` enforces
uniqueness across `plugins/*/plugin.json` before packaging.
The release-blocking duplicate with ESP Lab V0.3 (`Tfx3`) was the reason for
changing this plugin's prior identity. The V0.3 wrapper, source manifest,
packaged manifest, and ARM32/ARM64 binaries now all report `TfxR`.

## What the DSP does

All effect audio processing in the supported modes executes in the standalone
MAME-derived ES5510 core. Host code schedules controls, delay addresses and
trigger envelopes. The program images below are original TROLLfx programs,
not recovered Ensoniq firmware or factory microcode.

| Modes | Processing structure | ES5510 words | Relevant controls |
|---|---|---:|---|
| Delay / ping-pong | Stereo feedback delay; ten-second ring | 19 | Time, feedback, wet/dry, ping-pong, tempo sync |
| Chorus | Short stereo delay with opposite-phase host LFO address schedule | 11 | Rate, depth, wet/dry |
| Phaser | Twelve cascaded allpass stages | 131 | Rate, depth, center, spread, feedback |
| Panner / rotary | Stereo gain/pan program; dual-band delay/modulation rotor program | 17 / 40 | Position, width, motion; horn/drum speed, acceleration, Doppler, drive |
| NonLin 1 | One input diffuser, nine independently addressed one-pass stereo taps | 81 | Nine tap levels, decay/tap scale, predelay, diffusion, wet/dry |
| NonLin 2 | Four input diffusers and nine long, distributed one-pass stereo taps | 117 | Nine tap levels, decay/tap scale, predelay, diffusion, wet/dry |
| NonLin 3 | Centered drum input, one diffuser/channel, nine short taps | 86 | Nine tap levels, decay/tap scale, predelay, diffusion, wet/dry |
| Small / Large Plate | Two / three serial diffusers per channel, cross-coupled feedback returns, controllable one-pole HF damping | 55 / 65 | Decay, predelay, diffusion 1/2, HF damping, wet/dry |
| Room / Hall | Different one/two-stage diffusion paths, early taps, feedback/damping; Hall uses cross-coupled return | 59 / 71 | Decay, predelay, early level, diffusion, HF damping, wet/dry |
| Gated / Reverse 1 / Reverse 2 | Plate-derived delayed returns with host-scheduled gate or rising output envelope | 51 / 63 / 63 | Attack, hold, release, trigger/retrigger thresholds, damping, wet/dry |

The NonLin programs implement nine level controls as GPR 150–158. They are
single-pass tap schedules rather than feedback reverbs; their programmed tap
times establish different transient, bloom, and centered-drum characters. At
44.1 kHz the last nonzero impulse samples are 22,052 / 66,017 / 1,532 for
NonLin 1/2/3; at 48 kHz they are 24,002 / 71,855 / 1,667. These are measured
tail endpoints, not RT60 values.

The plate, room/hall, and gate/reverse damping controls now alter one-pole
filters assembled from ES5510 ALU/MAC instructions. Damping maps a 0–99 control
to an 18 kHz–1 kHz coefficient range. Room, Hall, plates, and envelope modes
have separate instruction images, feedback layouts, delay schedules, and/or
diffuser counts; they are not labels over one common reverb.

### Phaser headroom fix

Hot -0.1/-1 dBFS-like stress signals at 44.1 and 48 kHz, positive/negative
requested feedback ±0.9, and both modes of stereo input are processed with no
full-scale clips. The phaser internally trims the cascade by 12 dB, restores
that gain on the wet leg, clamps effective feedback to ±0.75, and applies a
stereo-linked 0.98 ceiling. The measured 500 Hz / 2 kHz notches remain about
-64/-65 dB at 44.1 kHz and -65/-71 dB at 48 kHz. The fix preserves deep moving
notches while preventing the former 24-bit ALU wraps and hot wet-mix overload.

## Evidence and limits

**Verified from the public DP/4 Musician’s Manual:** separate algorithm names,
control families, input/output envelope behavior for gated/reverse/nonlinear
families, plate/room/hall parameter concepts, and exposed unit/routing
behavior. See `docs/DP4_ALGORITHM_RESEARCH.md`, especially its algorithm
structure notes, and the manual's reverb descriptions/parameter tables
(roughly printed pp. 61–72):
<https://umlsrt.com/StudioDocuments/Ensoniq%20DP4%20Manual%20.pdf>.

**Verified by code/render:** V0.3 original instruction counts, memory sizes,
impulse timing endpoints, hot-input clipping checks, host simulation, and SDK
metadata/skin validity. Reproducible test commands are below.

**Approximation:** manuals describe exposed behavior and broad processing
relationships, not the original ES5510 microcode. Our delay lengths,
allpass coefficients, feedback values, damping curves, and envelope schedules
are TROLLfx designs. Gated/reverse triggering and attack/hold/release are
host-computed control schedules; retrigger/pre-trigger semantics are not a
verified reproduction of DP/4 firmware. The NonLin timing curve is a nine-tap
high-level implementation, not an exact DP/4 envelope curve. No factory
algorithm accuracy is claimed.

## Build and validation

Official Akai SDK checkout: `/tmp/mpc-plugin-builder-sdk`, commit
`febd2ca5bd9eb16c4a412837196e145566bc981b`; Zig 0.17.1. The Force target is
32-bit ARM EABI hard-float, and the ARM32 binary reports max GLIBC 2.27.

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

Result: native, ARM32 and ARM64 builds pass. SDK validation reports 0 errors,
0 warnings, six TUI tabs, and all Q-Link parameter indices in range. Binary
sizes: native 3.0 MB, ARM32 262 KB, ARM64 303 KB. ARM32 imports `libm`, `libc`,
`libdl`, and `ld-linux-armhf`; it exports `VSTPluginMain`.

Desktop host simulator passes 20/20 checks at 44.1 kHz / 128 frames and
48 kHz / 4096 frames, including declared 32/512 block passes, parameter sweep,
state roundtrip, in-place processing, silence and finite output. This is not
an ARM hardware timing result.

Strict native tests passed with `g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic
-Werror`:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/programs/nonlin_tap/tests/nonlin_tap_smoke.cpp \
  -o /tmp/nonlin-tap-smoke
/tmp/nonlin-tap-smoke /tmp/nonlin-tap-wavs

g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/vst_host/v03/ReverbBank.cpp \
  experimental/emulation/vst_host/v03/tests/reverb_bank_smoke.cpp \
  -o /tmp/reverb-bank-smoke
/tmp/reverb-bank-smoke
```

All ten bank modes produced nonzero impulse and hot-beat output without
clipping. Aggregate metrics distinguish the ten modes. The standalone
Room/Hall test also passes with distinct renders. Phaser headroom tests pass at
44.1/48 kHz with no over-full-scale output. The VST host simulator exposed and
helped fix a re-prepare coefficient-array overrun; re-prepare now passes.

Sixteen 44.1 kHz / 4 second WAV demos are in
`artifacts/es5510-emulation/v03/demos/`: delay, ping-pong, chorus, 12-stage
phaser, panner, rotary, Small/Large Plate, Room, Hall, Gated, Reverse 1/2, and
NonLin 1/2/3. Integrated demo peaks range from 0.17 to 0.98 FS; the renderer
reports no clipped samples. Additional NonLin 44.1/48 kHz impulse and beat
fixtures are in `experimental/emulation/programs/nonlin_tap/renders/`.
Recreate the integrated audio set with:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iexperimental/emulation \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/vst_host/TempoDelayControl.cpp \
  experimental/emulation/vst_host/v02/Es5510V02Processor.cpp \
  experimental/emulation/vst_host/v03/ReverbBank.cpp \
  tools/es5510_v03_demo_render.cpp -o /tmp/es5510-v03-demo
/tmp/es5510-v03-demo artifacts/es5510-emulation/v03/demos
```

At 44.1 kHz, delay/chorus/panner/rotary each use 10 seconds of stereo delay
memory; the four large legacy rings total about 7.1 MB. Reverb engines now use
mode-specific rings, totaling about 1.6 MB rather than allocating ten maximum
banks (~3.8 MB). Total DSP delay RAM is roughly 8.7 MB, plus code/state and
small scratch buffers. Each processing frame runs only the selected program,
except during bounded 10 ms mode crossfades. Force CPU/runtime latency remain
unmeasured for the combined V0.3 plugin; prior V0.1/V0.2 per-effect hardware
benchmarks are not a measurement of this release.

## Package and installation

- Package: `artifacts/es5510-emulation/v03/trollfx-es5510-v03-nonlinear-reverb-akai-force-armv7.zip`
- SHA-256: `7ce484e3d42eff95ff0ec32f974013b6b7e276a81238100b42b2c7513003ff95`
- Folder inside ZIP: `TROLLfx - VST - ES5510 NonLin Reverb V0.3`
- Package creator: `tools/package_es5510_v03.py` (fixed ZIP timestamps).
- UID audit: source plugin manifests under `plugins/*/plugin.json` were checked;
  `TfxR` is unique. The package script now fails if any source-manifest UID is
  duplicated.

Run on a computer to build the package:

```sh
python3 tools/package_es5510_v03.py
sha256sum artifacts/es5510-emulation/v03/trollfx-es5510-v03-nonlinear-reverb-akai-force-armv7.zip
```

No installation has been performed. After copying the archive to the Force
SSD root and checking that the target folder does not exist, a POSIX `sh`
installation command is:

```sh
grep '/media/SSD250g' /proc/mounts
SRC='/media/SSD250g/TROLLfx - VST - ES5510 NonLin Reverb V0.3'
DST='/media/SSD250g/VST2 Plugins/TROLLfx - VST - ES5510 NonLin Reverb V0.3'
test -f "$SRC/plugin.json" && test ! -e "$DST" && cp -a "$SRC" "$DST"
```

If the SSD mount shows `noexec`, stop and use a supported executable plugin
location; do not change mount settings persistently. Enable External Plugin
Support, restart the Force to rescan, then insert “ES5510 NonLin Reverb V0.3”.

### Retire the prototypes only after V0.3 works on hardware

This archives the older folders without deleting them or overwriting an
existing archive. Do not run it before confirming V0.3 scan, state restore,
audio, and CPU load on the Force.

```sh
#!/bin/sh
set -eu
PLUGIN_ROOT='/media/SSD250g/VST2 Plugins'
V03="$PLUGIN_ROOT/TROLLfx - VST - ES5510 NonLin Reverb V0.3"
V01="$PLUGIN_ROOT/TROLLfx - VST - ES5510 Experimental"
V02="$PLUGIN_ROOT/TROLLfx - VST - ES5510 Expanded V0.2"
ARCHIVE="$PLUGIN_ROOT/TROLLfx-archive-after-v03-verified"
test -f "$V03/plugin.json" || { echo 'V0.3 not present; nothing archived' >&2; exit 1; }
test ! -e "$ARCHIVE" || { echo 'Archive already exists; inspect it first' >&2; exit 1; }
mkdir "$ARCHIVE"
if [ -d "$V01" ]; then mv "$V01" "$ARCHIVE/ES5510-Experimental-V0.1"; fi
if [ -d "$V02" ]; then mv "$V02" "$ARCHIVE/ES5510-Expanded-V0.2"; fi
echo 'V0.1/V0.2 folders archived; restart Force to rescan.'
```

Rollback after retirement: move the archived folder(s) back to
`$PLUGIN_ROOT`, ensuring the destination does not already exist, then restart
to rescan. Do not erase the archive until projects and presets have been
checked.

## Commits and continuation

Contributing milestones on this branch include phaser headroom repair
`f72f4ce`, Room/Hall/envelope programs `9f5ff30` / `7657784`, plate prototypes
`e5a2d54`, and integrated NonLin tap programs `46c7642`.

- Consolidated V0.3 source, ARM32/ARM64 package, and results: `3033e1b3ddc2ec94b61e6bd03c2817d93bbc529e`.
- Integrated 16-file demo render commit: `a06851fea609764000594a904ee3a7040255331f`.
- UID collision repair, ARM binaries, and regenerated package/hash:
  `8479bcc63cc33a4fa2596dc86edf9d736d75151a`.
- This file's follow-up handoff revision is a docs-only commit after that
  release commit.
- The final GitHub branch HEAD also includes the docs-only commit that records
  these two full hashes. The package SHA above does not change with that edit.

Next: user installs V0.3 beside the existing test prototypes and evaluates
sound, scan, controls, state restore, and CPU on the Force. Based on that
hardware feedback, refine control ranges and room/Hall/gated voicing before
spending time on additional UI or factory-algorithm compatibility claims.
