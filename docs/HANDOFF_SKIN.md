# ESP Lab V0.3 touchscreen skin

Base: V0.3 plugin identity `Tfx3`; final integrated source is on `main`.

The V0.3 skin uses a warm amber control accent, cream labels, and a charcoal
fallback background. It adds the generated abstract brushed-metal panel
image to focused and unfocused page backgrounds. The source PNG is preserved
in `plugins/TROLLfxESPLabV03/assets/esp-lab-panel.png` (SHA-256
`ab99180d34bf949a8bf5b46e6f4b04a69e0b14c933b87dbcc1cbc6942fcd7067`). No
Ensoniq logos or product photography are used.

The pages are ROUTING, CTRL SRC, UNIT A-D, and MOD A-D. Each unit page presents common
controls and conditional VCF Distortion / Compressor controls. Each modulation
page binds both stable, per-unit slots directly: Enabled, Source, Destination,
Min, Max, and Invert. This preserves the host's built-in parameter chunk state.
The CTRL SRC page exposes all eight global host-automatable Controller values.
They can drive per-unit modulation sources. They are host parameters, not MIDI
CC assignments.

The official SDK limits a page to 16 controls. Its generator now accepts an
optional `backgroundImage` filename in `ui.json`; it places that image in both
TUI background states. The regular background colour remains present as a
fallback. The build copies `assets/` into `Plugin Skins/` and validates the
image reference.

Verification used SDK build command:

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py plugins/TROLLfxESPLabV03 --target all --toolchain zig --zig /usr/bin/python3
```

Native, ARM32/Cortex-A17, and ARM64 builds succeeded. SDK validation reported
0 errors and 0 warnings; all 10 TUI tabs, 10 Q-Link maps, 146 parameter handles,
and the background image passed validation. SDK output notes that Force
navigation beyond six function keys is untested. Actual screen fit and image
rendering require hardware review; no hardware deployment was performed.

Installable package: `plugins/TROLLfxESPLabV03/dist/esp-lab-v0-3-0.3.0-mpc.zip`

Package SHA-256 after integrated-main rebuild: `aaee7c24a49a3fc9512ded6336db5cb548bdfb75092ea0b3d21cb44d602ed46e`.
