# TROLLfx 5510 Single V0.1

A separate one-engine ES5510 instruction-processing insert for Akai Force hardware evaluation. UID: `T55S`.

The shipped DSP path is the original TROLLfx diagnostic **Delay** and **Chorus** microprogram, executed by the MAME-derived standalone ES5510 adapter. It does not claim to contain or execute the private ASR-10 factory captures. Those captures are runtime snapshots, not clean boot images; their internal pipeline state is absent, and they are not yet safely wired into the plugin's state-loading lifecycle. The factory captures remain outside this project and package.

Controls: Algorithm (TROLLfx Delay/TROLLfx Chorus), Time, Feedback (Delay), Rate and Depth (Chorus), Wet, and Bypass. Processing uses one separately allocated ES5510 delay RAM, stereo host input, and the diagnostic program's DOL/SER3 output path. No file I/O or allocation occurs in `process()`.
