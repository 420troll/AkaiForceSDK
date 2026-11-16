# TROLLfx ES5510 Expanded V0.2

Experimental stereo VST2 effect for Akai Force ARMv7. Unique VST UID: `TfxV`.
Includes the V0.1 delay and chorus plus original ES5510-instruction programs for
12-stage phaser, stereo motion panner, ping-pong delay and dual-band rotary.
These are original TROLLfx experiments, not recovered Ensoniq factory effects.

The host supplies modulation/control trajectories; audio filtering, delay,
feedback and matrix operations execute in the MAME-derived ES5510 core. Rotary
input drive and speed inertia are host-side control approximations. See
`docs/HANDOFF_ES5510_MODFX.md` for measured limitations and validation.

The plugin is a separate folder and identity from V0.1 (`TfxX`).
