# Algorithm catalog

This catalog records public behavior, not recovered factory code. `First`
means a practical original implementation milestone, not an authenticity claim.

## Cross-product families

| Family | Public evidence across products | Likely building blocks (inference) | Priority |
|---|---|---|---|
| Stereo/dual/multitap delay | EPS-16/ASR delay effects; DP/4 Multi Tap, Dual, Tempo and EQ-DDL combinations; DP/Pro variable-tap, spatial, ping-pong and stereo delays | Circular buffers, integer/fractional taps, tap gain/pan, filtered feedback/cross-feedback, tempo conversion | **First** |
| Chorus/flanger/vibrato | DP/4 EQ+Chorus/Vibrato/Flanger and 8 Voice Chorus; DP/Pro Multi-Chorus, Flanger and combination algorithms | Fractional delay, interpolator, stereo LFO, phase offsets, smoothing, filtered regeneration | **First** after delay core |
| Tremolo/panner | DP/4 and DP/Pro families | Gain modulation, equal-power pan, flexible LFO | **First/easy** |
| Parametric/shelving/EQ | Present standalone and inside many algorithms | Biquads or state-variable filters, gain staging | **First/supporting** |
| Compression/gate/limiting/de-essing | DP/4 dynamics algorithms; DP/Pro adds lookahead and multiband variants | Envelope followers, side-chain filters, soft knee, delay-based lookahead | Second |
| Phaser | DP/4 Phaser-DDL; DP/Pro stereo phaser | Cascaded all-pass sections, stereo LFO, feedback/cross-feedback | Second |
| Distortion/VCF combinations | DP/4 VCF-Distortion and combinations; DP/Pro distortion combinations | Nonlinearity, pre/post filter, level compensation | Second; behavior measurements needed |
| Room/hall/plate reverb | EPS/ASR reverbs; DP/4 room/hall; DP/Pro expert reverb/ambience | Predelay, input filters, serial all-pass diffusion, multi-delay feedback network, frequency-dependent damping, slow modulation | Third |
| Nonlinear/gated/reverse reverb | DP/4 and DP/Pro public families | Shaped tap/envelope structures and/or gated late field | Third; topology unverified |
| Pitch shifting/harmonizing | DP/4 pitch variants including ganged 2-unit; ASR algorithms; DP/Pro six-voice Multi-Pitch Shift | Dual-head/windowed delay or granular splice engine, crossfade windows, feedback/filtering | Third/high risk |
| Resonant/creative modulation | EPS/ASR external-effect ecosystem and DP combinations | Product-specific; implement only from public behavior or new design | Later |

## DP/4 public algorithm set

The manual lists families including Small Room, Large Room, Hall, Reverse,
Gated and Nonlinear reverbs; multitap/dual/tempo delays; EQ+DDL/LFO,
VCF+Distortion, EQ+Chorus/Vibrato/Panner/Flanger/Tremolo, Phaser+DDL,
8-Voice Chorus, Flanger, several Pitch Shift variants, EQ+Compressor and
Parametric EQ. Some algorithms require one unit, while expensive ones can gang
two or four processing units.

The DP/4 routing architecture is itself an algorithmic feature: four units can
operate independently, in serial/parallel combinations, or ganged. A future
TROLLfx product can capture this spirit with a small explicit graph rather than
hard-coding every factory configuration.

## DP/Pro public algorithm set

The manual enumerates 42 algorithm slots:

1. No Effect
2. Stereo Limiter
3. Compressor/Limiter
4. Compressor
5. Servo Limiter
6. 3-Band Compressor/Limiter
7. Expander
8. Stereo Gate
9. Dual-Mono Gate
10. De-Esser
11. Stereo Synthesizer
12. Stereo Re-Imager
13. Variable-Tap Delay
14. Spatial Delays
15. Ping-Pong Delay
16. Stereo Delay
17. (manual list continues with its reverb entry before Expert Reverb)
18. Expert Reverb
19. Expert Reverb 2
20. Reflection Modeler
21. Small Ambience 1
22. Small Ambience 2
23. Non-Lin Reverb 1
24. Non-Lin Reverb 2
25. Multi-Pitch Shift
26. Multi-Chorus
27. Flanger
28. Phaser
29. Tremolo
30. Panner
31. Chatter Box
32. (verify name from a cleaner manual table before implementation)
33. Tunable Speaker
34. Parametric EQ
35. Octave EQ
36. Delay+Chorus
37. Delay+Flanger
38. Delay+Phaser
39. Distortion+Chorus
40. Distortion+Flanger
41. Distortion+Phaser
42. Signal Generator

Entries 17 and 32 are deliberately not guessed from imperfect text extraction.
This is an example of the evidence policy: a gap remains a gap until a manual
page is checked visually.

Notable verified parameter behavior includes:

- Variable-Tap Delay: up to 32 taps, delay-spacing and amplitude/pan curves,
  four diffusion stages, EQ, regeneration, and curve-warp/offset controls.
- Spatial Delays: eight modulatable delays (four per side), per-tap level/pan,
  shelving EQ and a master size control.
- Stereo Delay/Flanger/Phaser: flexible stereo LFO, left/right phase, smoothing,
  filtered regeneration and cross-regeneration.
- Expert Reverb: six-stage diffuser, predelay, frequency-dependent decay,
  density and eight adjustable delay times.
- Multi-Pitch Shift: six independent voices, ±8-octave published range, splice
  width, delay, filtering, regeneration, level and pan.

## Parameter behavior rules for original implementations

- Preserve units and perceptual mappings from public ranges where useful, but
  do not assume a displayed `0..99` value is linearly mapped internally.
- Every continuous time/rate/gain change must be smoothed or crossfaded.
- Feedback controls need explicit stable limits and finite-value recovery.
- Modulation phase and stereo correlation are first-class parameters.
- Bypass must define tail behavior. DP/Pro publicly distinguishes normal
  bypass, input mute (tail continues), output mute, and all mute.
- A routing/preset change may reinitialize an algorithm; a mere parameter
  change should not cause the audible program-load gap described by vintage
  hardware manuals.

## Recommended first deliverable

Implement `TROLLfx Mod Delay Lab`, an original effect with:

- stereo delay with independent/free or tempo-synced time;
- linear first, then optional 3rd-order Lagrange interpolation;
- stereo sine/triangle LFO with phase offset and rate/depth;
- high-pass and low-pass filtering in feedback;
- normal, cross and ping-pong feedback routes;
- parameter smoothing and tail-safe input-mute bypass;
- native golden renders and ARMv7 validation.

This produces reusable primitives for chorus, flanger, vibrato, spatial delay
and the first diffusion experiments without making an unsupported recovery
claim.

