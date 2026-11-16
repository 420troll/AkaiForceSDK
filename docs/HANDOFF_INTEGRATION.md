# TROLLfx ESP Lab V0.2 integration handoff

V0.2 is a separate plugin identity (`Tfx2`); V0.1 project and package are not
modified. Four stereo `EffectsUnit` instances share the `AudioEngine` interface
and provide Delay, Chorus, Flanger, Room, Hall, Plate and Modulated Reverb.
The controls persist through the SDK's parameter chunk state. Touchscreen
contains ROUTING and UNIT A-D tabs; the SDK generator emits one Q-Link map per
tab.

Verified on desktop with the official SDK build at the repository's pinned
snapshot: native, ARM32/Cortex-A17 and ARM64 builds; 0 validator errors and
warnings; host simulator 20 passed, 0 failed. This does not establish Force
hardware UI fit or audio performance. Four-unit routing is functional software
routing, not a recreation of DP/4 factory topology.

Final plugin source: `plugins/TROLLfxESPLabV02/`. ARMv7 binary:
`plugins/TROLLfxESPLabV02/build/TROLLfx - VST - ESP Lab V0.2/VST/esp-lab-v02-arm32.so`.
Install package: `plugins/TROLLfxESPLabV02/dist/esp-lab-v0-2-0.2.0-mpc.zip`
(SHA-256 `4020e995d604a5a0fde3c3f6f31ca027e018e53384385040fe07076d8f3d8919`).
Extract the ZIP and copy `TROLLfx - VST - ESP Lab V0.2` into the Force drive's
root `VST2 Plugins` folder. V0.1 remains available under its separate identity.

The effects use an original four-line feedback delay network informed by
published reverb controls/structures. No factory code is used and no original
algorithm recovery is claimed. Preset recall currently relies on host plugin
state chunks; native preset bank browsing remains future work.

Next: add route-specific automated renders, smooth algorithm/parameter changes,
and verify four-page Q-Link focus on Force hardware after explicit deployment
approval. Preserve the V0.1 package as rollback.
