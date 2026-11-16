# TROLLfx ES5510 Expanded V0.2 — Development Handoff

## Release state

- Branch: `issue6-7-es5510-modfx-v02`
- Release: `ES5510 Expanded V0.2`, stereo VST2, UID `TfxV`.
- V0.1 remains UID `TfxX`, in its existing project/package. This release has its
  own plugin folder and package. No hardware installation was performed.
- Release commit: recorded below after integration/push.
- Package: `artifacts/es5510-emulation/v02/trollfx-es5510-expanded-v0.2-akai-force-armv7.zip`
- SHA-256: `e409b080817256a17867734feb8b1593c4e6c6d500bceb5edf66d5c5ac14e29e`

## Included modes

| Mode | Instruction program | Main host controls | Evidence and limits |
|---|---:|---|---|
| Stereo delay | 19 words | Time, feedback, wet/dry, sync/source/division/BPM | Sync OFF passed sample-exact comparison with V0.1 at 44.1 and 48 kHz. |
| Chorus | 11 words | Rate, depth, wet | Opposite-phase host delay-address schedule; audio filtering/delay executes in ES5510. V0.1 program retained. |
| Phaser | 131 words | Rate, depth, center, stereo spread, signed feedback, wet/dry | 12 first-order allpass stages per channel. Sweep verifies moving notches near 500 Hz and 2 kHz. Original TROLLfx program, not a Waveboy or factory program. |
| Motion panner | 17 words | Position, width, motion rate/waveform/direction, wet/dry, optional clock division | Host computes matrix gains; ES5510 instructions apply stereo matrix. |
| Ping-pong delay | 19 words | Time, feedback, wet/dry | Original cross-coupled delay program, independent from stereo delay. |
| Dual-band rotary | 40 words | Slow/fast/stop, horn/drum rates, ramp, width, Doppler, drive, wet/dry | ES5510 program performs two one-pole crossover paths, complementary bands, moving taps and stereo gain. Input drive and speed inertia are host-computed approximations; cabinet input is downmixed to mono. |

All ES5510 programs are original TROLLfx experiments. None are claimed to
reproduce Ensoniq factory microcode. See `docs/ES5510_PROGRAM_CATALOG.md` and
`docs/WAVEBOY_EPS16_RESEARCH.md` for program provenance and historical loading
limitations.

## Processor and real-time behavior

The V2 adapter in `experimental/emulation/vst_host/v02/` embeds the standalone
MAME-derived ES5510 core. Host code computes modulation/control values and
updates GPRs; effect audio processing is performed by the instruction engine.
The phaser coefficient is recalculated every eight frames. The panner's
position/width matrix coefficients are host-computed. Rotary speed inertia and
drive are host-side. No host BPM is invented: `HOST` sync falls back to the
internal BPM if the SDK transport does not report valid tempo.

Only the selected program runs, except a bounded 10 ms two-program crossfade on
program changes. Inactive engines retain their RAM history but do not advance;
tails therefore pause while another program is selected. Program memories are
allocated in `prepare`, not in the audio callback. Memory for the four 10-second
ring engines is about 14.1 MB at 44.1 kHz and 15.4 MB at 48 kHz, plus small
processor state. No allocation, locks, file access, or program loading occurs
in processing. No hardware or VST-host CPU benchmark was made for V0.2; prior
V0.1 standalone Force numbers are not a V0.2 estimate.

Delay sync exposes the 12 DP/4-derived rhythm names from Issue #6. Internal BPM
is 50–250. The official SDK checkout exposes `mpc::Transport` tempo fields and
the wrapper receives the host transport through the VST time callback in
simulation. Whether the original Force VST host supplies tempo is unresolved;
the plugin labels unavailable host tempo and uses internal BPM. Sync OFF uses
V0.1's original program path.

## Build and validation

Build environment: official `inmusic-software/mpc-plugin-builder-sdk`, commit
`febd2ca5bd9eb16c4a412837196e145566bc981b`; Zig 0.17.1.

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 /tmp/mpc-plugin-builder-sdk/tools/build.py \
  plugins/TROLLfxES5510ExpandedV02 --target all --toolchain zig --zig /usr/bin/python3
python3 /tmp/mpc-plugin-builder-sdk/tools/validate.py --strict \
  plugins/TROLLfxES5510ExpandedV02
python3 tools/package_es5510_expanded_v02.py
sha256sum artifacts/es5510-emulation/v02/trollfx-es5510-expanded-v0.2-akai-force-armv7.zip
```

Results:

- Native shared library: 2.8 MB; ARMv7 hard-float shared library: 235 KB;
  ARM64 extra: 275 KB.
- ARM32 ELF exports `VSTPluginMain`; maximum GLIBC requirement 2.27.
- Strict SDK validation: 0 errors, 0 warnings; UID, metadata, both binaries,
  five UI tabs, skin assets and five Q-Link banks validated.
- SDK `host_sim`: both 44.1 and 48 kHz at blocks 32, 64, 96, 128, 192, 256,
  and 512. All 14 invocations passed (19–20 checks each, no warnings or
  failures), including finite output, in-place processing, parameter sweep,
  state roundtrip and silence. These are desktop simulations, not Force tests.
- Native processor smoke test: all modes finite/nonzero; delay Sync OFF was
  sample-identical with V0.1 at both rates over 8,192 frames (max float delta
  0); mode changes, bypass and tempo controls passed.
- Phaser: 500 Hz target notch measured at 484.841 Hz (-29.543 dB coarse,
  -85.778 dB at center); 2 kHz target at 2,001.56 Hz (-61.218 dB coarse,
  -86.265 dB at center); feedback +/-0.85 did not clip in the test.
- Rotary: low path 100 Hz/5 kHz RMS 0.06930/0.00142, high path
  0.01888/0.07151; speed schedules and separate horn/drum changes altered
  rendered output; no clipping in the test.
- Spatial program checks passed center/mono/pan/width and alternating ping-pong
  echo timing checks.
- Six 44.1 kHz, 4-second stereo WAV demonstrations are in
  `artifacts/es5510-emulation/v02/`. They are instruction-engine renders;
  all measured files contain nonzero audio and zero clipped samples. Stereo
  tempo delay, ping-pong, phaser, panner and rotary are included, as is chorus.
- ARM32 performance on the Force, multiple-instance CPU budget, touchscreen
  rendering on hardware, host tempo delivery on hardware and audio judgment
  remain unmeasured. Do not infer them from the prior V0.1 CPU numbers.

## Installation and rollback

Do not install over V0.1. Extract the ZIP on a computer and copy only the
`TROLLfx - VST - ES5510 Expanded V0.2` directory to:

`/media/SSD250g/VST2 Plugins/`

On the Force, inspect mount options before copying/executing from the SSD:

```sh
grep '/media/SSD250g' /proc/mounts
```

If the relevant mount is `noexec`, do not persistently change mount settings;
use a supported executable plugin location. Ensure the destination V0.2 folder
does not already exist; the package instructions refuse a merge. Enable
External Plugin Support in Force settings, restart to rescan, then add
“ES5510 Expanded V0.2” as a stereo insert. Scan and hardware tests have not
been performed. For rollback, delete only that V0.2 folder and restart; retain
the V0.1 folder (`TfxX`). Full POSIX `sh` steps are in the ZIP's `INSTALL.txt`.

## Demonstrations

- `artifacts/es5510-emulation/v02/tempo-delay-1-8-dotted-120bpm.wav`
- `artifacts/es5510-emulation/v02/chorus.wav`
- `artifacts/es5510-emulation/v02/stereo-phaser-12-stage.wav`
- `artifacts/es5510-emulation/v02/stereo-motion-panner.wav`
- `artifacts/es5510-emulation/v02/stereo-pingpong-delay.wav`
- `artifacts/es5510-emulation/v02/dual-band-rotary.wav`

Regenerate with strict native C++17 compilation:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iexperimental/emulation \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/vst_host/TempoDelayControl.cpp \
  experimental/emulation/vst_host/v02/Es5510V02Processor.cpp \
  tools/es5510_v02_demo_render.cpp -o /tmp/es5510-v02-demo
/tmp/es5510-v02-demo artifacts/es5510-emulation/v02
```

## Commits

- V0.1 base: `7974955910f0b1fff861cc3fdfcffe73b55bb45d`
- Tempo sync: `9f63623`
- EPS/Waveboy research and capture inspector: `5e0724b`
- ES5510 phaser: `f24be58`
- ES5510 rotary: `c88eb05`
- V0.2 integration/release commit: to be filled after commit.

No proprietary ROM, firmware, or commercial effect program is included. MAME
BSD-3-Clause attribution and conditions are included in the package.
