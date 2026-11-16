# TROLLfx ESP Lab Series V0.4

Four separately controlled stereo units run in a fixed serial order: A → B → C → D. The chain order is intentionally fixed. Each unit has its own algorithm, mix, bypass and parameter state.

Implemented algorithm choices are Delay, Chorus, Flanger, Room, Hall, Plate, Modulated Reverb, VCF Distortion, EQ-Panner-DDL, Compressor, Multitap Delay, Dual Delay, Eight-Voice Chorus, Parametric EQ, De-Esser, Expander, Gate and Rumble Filter. The later eight are original TROLLfx implementations of documented DP/4 function families, not claims of recovered factory algorithms. Some controls are repurposed by algorithm (for example, Time/Cutoff); consult the on-screen value display.

The DSP uses TROLLfx's original C++ processing engine with preallocated delay and state buffers, informed by the DP/4 Musician's Manual and local TeleBrain parameter tables. Processing performs no allocation, locking or I/O. It is a native original-DSP design, not an ES5510 instruction emulator and not recovered Ensoniq factory microcode. The separate `TfxR` ES5510 NonLin Reverb V0.3 plugin remains independent.

## Force package

Build with the official Akai MPC Plugin Builder SDK. The SDK package includes ARMv7 hard-float and ARM64 binaries, TUI/Q-Link skin, installation notes and third-party notices. Install as a separate plugin folder. Do not remove the V0.3 ES5510 folder or its Issue #9 backup.
