# Native effects path handoff

V0.2 native DSP milestone: `c405145` (current final integration continues in
later commits). `src/original/audio_engine.hpp/.cpp` defines the stereo engine
interface and `EffectsUnit` algorithms Delay, Chorus, Flanger, Room, Hall,
Plate and Modulated Reverb. It preallocates two 10-second stereo buffers and a
four-line feedback tank in `prepare()`. Parameters are bounded; process is
allocation-free. Flanger modulation is constrained to the documented 0.1–15 ms
range.

Native smoke: `g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror
src/original/audio_engine.cpp src/original/audio_engine_smoke.cpp -o
/tmp/espfx-smoke && /tmp/espfx-smoke` — PASS. This is original simplified DSP
guided by public delay, diffusion, damping and modulation controls. It is not
factory code or a recovered Ensoniq topology. Remaining work: parameter
smoothing, actual predelay, route-specific test renders and ARM performance
measurement on device.
