# ESP Lab V0.3 touchscreen skin

Base: V0.3 plugin commit `11984a4` (138 parameters, stable per-unit state).

The V0.3 skin uses a warm amber control accent, cream labels, and a charcoal
fallback background. It adds the generated abstract brushed-metal panel
image to focused and unfocused page backgrounds. The source PNG is preserved
in `plugins/TROLLfxESPLabV03/assets/esp-lab-panel.png` (SHA-256
`ab99180d34bf949a8bf5b46e6f4b04a69e0b14c933b87dbcc1cbc6942fcd7067`). No
Ensoniq logos or product photography are used.

The pages are ROUTING, UNIT A-D, and MOD A-D. Each unit page presents common
controls and conditional VCF Distortion / Compressor controls. Each modulation
page binds both stable, per-unit slots directly: Enabled, Source, Destination,
Min, Max, and Invert. This preserves the host's built-in parameter chunk state.
The DSP modulation core recognizes host Controller 1-8 sources, but this V0.3
plugin constructor does not expose host-automatable Controller 1-8 values and
the unit Source choices remain Off/Internal LFO/Envelope. Therefore no separate
CONTROLLERS page is included yet.

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
0 errors and 0 warnings; all 9 TUI tabs, 9 Q-Link maps, 138 parameter handles,
and the background image passed validation. SDK output notes that Force
navigation beyond six function keys is untested. Actual screen fit and image
rendering require hardware review; no hardware deployment was performed.
