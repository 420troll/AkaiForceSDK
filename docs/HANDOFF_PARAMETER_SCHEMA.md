# V0.3 parameter schema handoff

Branch: `codex/v03-parameter-schema`  
Base: current `origin/main` at `4521c24` (includes modulation API `0ada6f8`)  
SDK: `inmusic-software/mpc-plugin-builder-sdk` at `febd2ca5bd9eb16c4a412837196e145566bc981b`

## Implementation

`plugins/TROLLfxESPLabV03/src/parameter_schema.hpp` defines the typed
algorithm-specific parameter records. The wrapper registers the four-unit
parameter bank after the original 146 parameters. It appends 27 parameters for
each unit, preserving existing IDs, order, ranges, and defaults. Each added
parameter is a regular host parameter and is stored by VST chunk state.

The added IDs use a semantic stem followed by the unit letter: `distInA`,
`gateReleaseC`, `trebleFcD`, and so on. The exact descriptors, ranges, evidence
labels, algorithm masks, display names, and nullable TeleBrain provenance are
in `research/esp_lab_parameter_schema.json`. All 254 parameters are mapped to
the corresponding `EffectParams` fields. The legacy `Unit X Drive` remains the
independent clipping drive control (`driveDb`); `Unit X Distortion Input` maps
to pre-distortion input gain.

Manual-backed ranges include compressor gain (-48..+48 dB), gate thresholds
(-96..0 dB), gate release (1..10000 ms), panner delay/echo (0..1500 ms), and
Treble Fc (1000..16000 Hz). Sources are the public DP/4 manual pages 97, 98,
and 114, as recorded in the schema. Table indices and offsets are metadata
only. Table 17's malformed Compressor Gain range, Table 40's duplicate offset
08, its combined panner width/phase row, and the Treble Fc 1..16 kHz encoding
are documented as source comparison notes. No TeleBrain payload or local
extract was added to Git; unmapped table and offset fields remain null.

## Verification

Commands run from the repository root:

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py plugins/TROLLfxESPLabV03 --target all --toolchain zig --zig /usr/bin/python3
```

Result: native, ARM32, and ARM64 builds succeeded. SDK validation reported 0
errors and 0 warnings. ARM32 output is a 32-bit ARM hard-float shared object,
exports `VSTPluginMain`, and requires GLIBC no newer than 2.27. Parameter
handles were validated in range 0..253. The existing TUI is unchanged in this
branch; it has not yet been bound to the 108 newly appended parameters.

```sh
.sdk-tools/host_sim plugins/TROLLfxESPLabV03/build/native/esp-lab-v0-3.so test --seconds 1 --wav /tmp/esp-lab-parameter-schema.wav
```

Result: PASS, 20 checks, 254 parameters; state restored for every parameter
and re-saved byte-identically. Additional direct C++ smoke checks passed:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/original src/original/audio_engine_smoke.cpp src/original/audio_engine.cpp -o /tmp/audio_engine_smoke-params && /tmp/audio_engine_smoke-params
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/original src/original/effects_parameter_smoke.cpp src/original/audio_engine.cpp -o /tmp/effects_parameter_smoke-params && /tmp/effects_parameter_smoke-params
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/original src/original/force_routing_smoke.cpp src/original/audio_engine.cpp -o /tmp/force_routing_smoke-params && /tmp/force_routing_smoke-params
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/original src/original/modulation_smoke.cpp src/original/audio_engine.cpp -o /tmp/modulation_smoke-params && /tmp/modulation_smoke-params
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/original src/original/reverb_tail_smoke.cpp -o /tmp/reverb-tail-params && /tmp/reverb-tail-params
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/original src/original/reverb_modulation_smoke.cpp -o /tmp/reverb-mod-params && /tmp/reverb-mod-params
```

All six smoke commands passed. The reverb modulation test reported finite,
different fixed and modulated renders. An initial attempt to link the inline
reverb header tests against a nonexistent `dp4_reverb.cpp` failed; rerunning
the commands without that nonexistent source passed.

## Next work

Update the V0.3 TUI and Q-Link mappings to bind relevant controls from these
exact display names/IDs, with algorithm-specific visibility. Confirm tab
accessibility on Force hardware; no hardware deployment was performed here.
