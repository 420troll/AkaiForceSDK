# Akai/InMusic SDK compatibility

## Inspected source

- Official repository: [inmusic-software/mpc-plugin-builder-sdk](https://github.com/inmusic-software/mpc-plugin-builder-sdk)
- Commit: `febd2ca5bd9eb16c4a412837196e145566bc981b`
- Commit timestamp: 2026-11-07T00:03:02Z
- SDK license: MIT
- Development host tested: Linux x86_64, Python 3.12.3, GCC 13 system C++,
  Zig Python package 0.17.1

This is a fast-moving SDK snapshot. Reconfirm the commit and host/firmware
contract before implementing or deploying a plugin.

## Actual plugin format and API

The deployable unit is a self-contained folder named
`<Manufacturer> - VST - <Name>` containing:

- `plugin.json` with name, manufacturer, version, `format: vst2`, kind, four-
  character UID, I/O counts, category and `file32`/`file64` library paths;
- `VST/<slug>-arm32.so` and normally `<slug>-arm64.so`;
- `version.xml`;
- optional `Plugin Skins/TUI.json`, `Q-Links.json` and images; and
- optional `Content/` resources.

The binary follows the VST 2.4 ABI. The SDK provides a clean-room ABI header
and a header-only C++17 framework; it does not require JUCE or the discontinued
Steinberg SDK for this path. `VSTPluginMain` is the required exported symbol.

Plugin authors derive from `mpc::Plugin`, declare `PluginInfo`, add stable
parameters in the constructor, allocate/initialize DSP in `prepare()`, clear
runtime state in `reset()`, and implement `process(ProcessContext&)`.
`ProcessContext` supplies non-interleaved float buffers, frame count, MIDI,
sample rate and transport.

## Host contract relevant to Force

| Property | Inspected SDK contract |
|---|---|
| Sample rate | 44,100 Hz fixed on MPC standalone |
| Samples | 32-bit float, non-interleaved |
| Block sizes | User may select 32, 64, 96, 128, 192, 256 or 512; announced maximum and actual frames can differ |
| Effect I/O | Normally two inputs/two outputs; in-place processing must be supported |
| Instrument I/O | Normally zero inputs and stereo output; multiple stereo outputs supported |
| MIDI | Instruments receive MIDI; the documented MPC host does not deliver MIDI to effects; plugin MIDI output is discarded |
| Threading | Audio callback is real time; parameter/state/UI calls may be concurrent |
| State | Framework uses VST chunks keyed by stable parameter ID and supports extra state hooks |
| Transport | Tempo, PPQ, bar/time signature, playback/record/loop state exposed per block |
| GUI | MPC does not open a plugin-native editor; touchscreen UI is JSON data |
| Process isolation | None; plugin faults can crash/restart the MPC application |

The framework uses atomic normalized parameter storage, defers state-load
handling to the audio thread, and wraps processing with denormal controls.
Large buffers belong on the heap and must be allocated outside `process()`.

## Parameter system

The host-facing range is always normalized `0..1`. Framework constructors map
that to:

- `floatParam`: linear or skewed range;
- `logParam`: positive logarithmic range;
- `intParam`: rounded integer range;
- `boolParam`; and
- `choiceParam`: discrete string choices.

Each parameter has a stable machine `id`, short display `name`, unit, min/max,
default, mapping and formatting. The MPC does not consume VST extended step
metadata; discrete touchscreen/data-wheel behavior comes from replacement
strings in the generated skin. Parameter count must not change at runtime.

The host reportedly toggles parameter zero during prepare and restores it.
Parameter zero must therefore tolerate spurious writes without audible or
expensive side effects.

## Touchscreen and Q-Link integration

`ui.json` is the convenient authoring layer. `tools/gen_ui.py` converts pages
of up to 16 controls into the load-bearing `TUI.json` and `Q-Links.json` files.
Supported convenient widgets include knobs, faders, switches, generic sliders,
text and empty cells, with choice/bool visibility conditions.

Important constraints from the inspected SDK:

- Use related groups of four because Q-Links operate in banks of four and the
  host draws a focus rectangle around the active group.
- TUI bindings use parameter indices; generator input uses parameter display
  names exactly. Q-Link JSON maps parameter indices per tab.
- A hand-authored skin can use arbitrary coordinates and richer components,
  but field order/casing/version values are strict. The parser intentionally
  accepts duplicate `version` keys used by some overlay objects, which normal
  JSON tooling may corrupt.
- The documented canvas is 1280x800, with a 1280x628 tab content area starting
  110 pixels below the top. Force presentation should still be verified on
  hardware before finalizing layout.
- Missing/broken skin files can fall back differently: malformed TUI can leave
  no tabs, while absent files may use generic controls/automatic Q-Links.

## Toolchains and original Force target

The default builder uses Zig and, at this commit, emits ARM32 with:

```text
-target arm-linux-gnueabihf.2.31 -mcpu=cortex_a17
-std=c++17 -O2 -fPIC -shared -fvisibility=hidden
```

The original Force is included in this ARM32 family. The GNU alternative uses
`-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard` and static C++/GCC runtime
flags. ARM64 uses AArch64/Cortex-A55. The validator checks ELF class/machine,
exports, dependency allowlist and requested GLIBC/GLIBCXX versions.

The README says external-plugin support requires MPC OS 3.10.1 or later and
the External Plugin Support enhancement enabled. That is a deployment
prerequisite only; it was not checked on hardware in this handoff.

## Reproduced build and test results

### Native GCC build

Command:

```sh
python3 tools/build.py examples/StereoDelay --target native --toolchain gcc --verbose
```

Result: native shared library and host simulator built successfully. GCC emitted
one `-Wformat-truncation` warning at `framework/mpc_plugin.h:734` for an 8-byte
pin short-label buffer. This is an SDK warning, not a StereoDelay DSP failure.

Host test command:

```sh
.sdk-tools/host_sim examples/StereoDelay/build/native/stereo-delay.so \
  test --seconds 2 --wav /tmp/stereo-delay-test.wav
```

Result: **PASS**, 20 passed, 0 warnings, 0 failures. Verified MPC-style load,
prepare, audio render, 32/512-frame calls, 200 random parameter changes, state
round trip and invalid-state tolerance, in-place equality, silence/tail,
finite/no-clip/no-denormal output and teardown. Average desktop process time
was 0.11% of a 128-frame block, worst 1.02% (orientation only). Render SHA-256:
`4052288064508fa608d6b2a8d66a5fee6fd299362502cbc9d8348fd5043c7f0`.

Running `validate.py` directly after the native-only build failed as expected
because no assembled plugin folder existed. This failure is recorded rather
than hidden.

### ARM32-only attempt

The SDK's automatic Zig bootstrap first failed because `ensurepip`/the system
`python3-venv` package was unavailable. `ziglang==0.17.1` was installed into
`/tmp/trollfx-zig` instead. The first compile then failed because Zig tried to
write `/home/gamer/.cache/zig`, which is read-only in this environment. Setting
`ZIG_GLOBAL_CACHE_DIR` and `ZIG_LOCAL_CACHE_DIR` to `/tmp` fixed it.

ARM32 compiled successfully (32-bit ARM shared object, Cortex-A17 target), but
the `--target arm32` run's final validator reported one error because the
example manifest also declares a missing `file64`. This is a builder/manifest
interaction, not an ARM32 binary failure.

### Full portable build

Command:

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py examples/StereoDelay --target all --toolchain zig \
  --zig /usr/bin/python3 --verbose
```

Result: native, ARM32 and ARM64 builds succeeded. UI/Q-Link generation
succeeded. Validator result: **0 errors, 0 warnings**. ARM32 required GLIBC
2.27 at most and exported only the required entry point; its dynamic needs were
`libm`, `libc`, ARM loader and `libdl`. No Force deployment or runtime test was
performed.

## Compatibility conclusion

The inspected SDK can produce a structurally valid original-Force ARMv7 plugin
from this development environment, and the example passes comprehensive native
host simulation. Actual Force compatibility remains unverified until the user
authorizes a non-destructive hardware test on a unit meeting the firmware and
feature prerequisites.

