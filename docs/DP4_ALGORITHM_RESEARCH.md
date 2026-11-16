# DP/4 algorithm and routing research — handoff for Codex
Research date: 2026-11-15 (America/Vancouver). Scope: original Ensoniq DP/4 first; DP/4+ additions labeled separately.

## Primary sources and evidence quality
1. Ensoniq *DP/4 Musician's Manual* v1.02 (1994): https://umlsrt.com/StudioDocuments/Ensoniq%20DP4%20Manual%20.pdf — primary source for original algorithm names, parameter ranges and signal-flow descriptions; sections 3–5 and Appendix algorithm list.
2. *Transoniq Hacker* collected DP/4 articles: https://drummachinefunk.com/files/DP_4_-_Transoniq_Hacker.pdf — contemporary user/developer discussions; includes original specifications, practical delay/reverb programming, and DP/4+ differences. Some early specifications differ from the later v1.02 manual.
3. DP/4+ reference-manual index: https://www.manualslib.com/products/Ensoniq-Dp-4Plus-3245402.html — use DP/4+ manual for confirmation before adding + exclusive algorithms.
4. Existing internal findings: docs/RESEARCH_INDEX.md, docs/ENSONIQ_DSP_ARCHITECTURES.md and docs/MAME_ES5510_ANALYSIS.md.

**Evidence distinction:** the manual documents *user-visible topology, controls and intended behavior*. It does not expose actual microcode, all internal delay lengths, filter coefficients, phase-response curves, numerical formats or the exact implementation of factory algorithms. A code implementation based on the manual is original/historically informed, **not** binary-identical emulation.

## Complete original DP/4 algorithm inventory

The v1.02 Appendix lists **44 single-unit entries** (including No Effect), **two 2-unit entries** and **one 4-unit entry**, i.e. **47 named entries including bypass**. Earlier published specifications counted 45 algorithms, likely different revision or accounting; retain this discrepancy as source-specific, not a universal number.

### Reverb — 11 single-unit
1. Small Room Rev
2. Large Room Rev
3. Hall Reverb
4. Small Plate
5. Large Plate
6. Reverse Reverb
7. ReverseReverb 2
8. Gated Reverb
9. Non Lin Reverb
10. NonLin Reverb2
11. NonLin Reverb3

### Delay — 4 single-unit plus 1 dual-unit
12. MultiTap Delay
13. Dual Delay
14. Tempo Delay
15. EQ-DDL-withLFO
**2U:** 3.3 sec DDL 2U

### Distortion/amp/speaker/rotary — 7 single-unit
16. VCF-Distortion
17. Guitar Amp 1
18. Guitar Amp 2
19. Guitar Amp 3
20. Speaker Cabinet
21. TunableSpeaker
22. Rotating Spkr

### Hybrid modulation/EQ/delay — 6 single-unit
23. EQ-Chorus-DDL
24. EQ-Vibrato-DDL
25. EQ-Panner-DDL
26. EQ-Flanger-DDL
27. EQ-Tremolo-DDL
28. Phaser - DDL

### Dedicated modulation — 2 single-unit
29. 8 Voice Chorus
30. Flanger

### Pitch processing — 3 single-unit plus 1 dual-unit
31. Pitch Shifter
32. Pitch Shift-DDL
33. FastPitchShift
**2U:** Pitch Shift 2U

### Dynamics, filtering, signal generation — 12 single-unit (including bypass)
34. EQ-Compressor
35. Expander
36. Keyed Expander
37. InversExpander
38. De-esser
39. Ducker / Gate
40. Rumble Filter
41. Parametric EQ
42. VandrPolFilter
43. Sine/Noise Gen
44. No Effect (Bypass Preset)

**4U:** Vocoder

## Actual four-unit routing semantics — essential to recreate
- DP/4 has four physical ESP processors A/B/C/D and 1-, 2-, 3- and 4-source configurations. 1-source can combine all four, 2-source groups AB/CD, 3-source uses A, B, CD, 4-source runs each independently (different physical I/O than our stereo VST).
- For a **1-source config**, AB and CD each independently select **serial / parallel / feedback 1 / feedback 2**, while AB-to-CD selects serial or parallel. The original manual states **32 selectable ABCD routing combinations**, visually reduced to 18 when feedback dry-path variants are collapsed.
- Feedback 1 and feedback 2 differ in the **dry signal path**; a feedback amount controls B→A (or D→C).
- Bypass and Kill are different: **Bypass passes dry through the unit**, **Kill produces silence from that unit**, including to downstream chains.
- Config presets store all active algorithms, parameter settings, routing and mix. Preserve these concepts instead of implementing a naive single serial chain.
- A stereo Force VST has **2 inputs/2 outputs**; emulate 1-source stereo and AB/CD grouped combinations within the insert effect first. Do not claim physical 4-input/4-output DP/4 equivalence from a stereo VST.

## DSP behavior specifically supported by manual

### Reverbs
- **Small/Large Room and Hall** share essentially one topology: per-input lowpass → input diffuser → early echo taps → 'Definition' decay diffusion; stereo taps; damping / frequency-dependent decay and feedback. The manual includes **two discrete reflection times 0–120 ms** with separate send/level, and **three position-balance controls**, a useful signature for distance/depth.
- **Small/Large Plate** share another topology: two diffusers before decay 'Definition', cross-coupled stereo feedback, output lowpass. They differ by internal values. **Small Plate decay 0.20–100 s, predelay 0–500 ms; Large Plate decay 0.40–140 s, predelay 0–430 ms**. Controls include HF damping, HF bandwidth, Diffusion 1/2, Decay Definition, four early reflection levels and stereo balance.
- **Reverse Reverb** uses plate-derived reverb followed by a rising output envelope triggered by a threshold; first version completes its envelope without retrigger. **Reverse Reverb 2** supports retriggering.
- **Gated Reverb** derives from plate, with attack, hold, release, **separate trigger and retrigger thresholds**, and controllable underlying decay. Nonlinear 1/2/3 are separate algorithms—must extract their envelope/trigger behavior independently, not treat them as one preset.

### Delay/modulation
- **Flanger** standalone has separate **LFO Rate**, **LFO Width**, **Flanger Center**, and **signed Regen (-99..+99)**. Manual explicitly says it has deeper notches than the EQ-Flanger-DDL algorithm. A narrow *internal* flanger sweep range is appropriate; keep separate delay times in hybrid EQ-Flanger-DDL for the long echoes.
- **EQ-Panner-DDL** includes EQ → panner → stereo feedback echoes; L/R panner LFO can be in-phase or out-of-phase; ping-pong discrete echoes with regeneration and echo output level. This is a particularly distinctive hybrid to prioritize.
- **EQ-Tremolo-DDL** includes a sample-and-hold LFO option and extreme tremolo rates reaching ring/amplitude modulation; don't implement solely as a standard slow tremolo.
- **8 Voice Chorus** is a multi-voice chorus with additional regeneration delay controls; do not mistake it for a generic two-tap chorus.
- **Pitch Shift 2U** is a splicer algorithm with **zero-crossing synchronization** using an additional ESP (manual gives favorable detection band approximately 55–555 Hz). The 1U shifter is splicer-style without this zero-crossing processing. Implement modern independent approximations only after characterization.

### Nonlinearity, dynamics and EQ
- **VCF-Distortion topology:** variable highpass → resonant pre-distortion VCF with envelope follower → gain and clip stage → post-distortion VCF with envelope follower → output. Can operate as wah/autowah; speaker/cabinet algorithms are useful after it.
- **EQ-Compressor:** full-featured dynamics with EQ in signal *and* sidechain paths. Ratio **1:1–40:1/infinity**, threshold **-96..0 dB**, attack **50 µs–100 ms**, release **1 ms–10 s**, gate with distinct open/close thresholds, bass and treble shelving.
- **De-esser:** stereo compressor using **EQ analysis sidechain separate from audio synthesis path**, configurable HP sidechain **4–8000 Hz**, EQ bands, ratio/threshold/attack/release, output gain and noise gate. More general than a simple fixed-frequency de-esser.
- **Parametric EQ:** four-band minimum-phase design: bass, two parametric mid bands (center/gain/Q), treble, and input attenuation. Documented shelf gain spans **-48..+24 dB**.
- **Van der Pol Filter:** filter then nonlinear harmonic/transient enhancement; manual does **not** specify exact oscillator equation or internal coefficients. Avoid labeling an arbitrary van-der-Pol ODE solver as a faithful copy.
- Expanders, inverse expander, keyed expander, ducker/gate, rumble filter, tunable speaker, guitar amps 1–3, rotary and vocoder each deserve their own algorithm parameter/topology extraction before implementation.

### Modulation/preset control
- Every effect uses **01 Mix (00–99)** and **02 Volume (00–99)**; **00** selects algorithm.
- Every algorithm provides **two assignable parameter modulators** with Source (off or one of eight globally selected controllers), Destination and minimum/maximum travel range; inverting travel endpoints inverts modulation.
- Config presets recall full four-unit routing and effects state.
- The **DP/4+** adds alternate algorithms, including Guitar Amp 4, Digital Tube Amp, Dynamic Tube Amp, VCF Distortion 2, plus utility additions such as Guitar Tuner and Vocal Remover. Keep these as **DP/4+ research**, not original DP/4 inventory. Its smoother effect switching uses a temporary digital dry crossfade, unlike original switching pauses.

## Build priorities for TROLLfx ESP Lab

**Shared platform first (used by both parallel paths):**
- Four logical units with algorithm-specific parameter schemas (not one global parameter set), unit state, preset persistence, bypass vs kill.
- Stereo 1-source routing with AB/CD serial, parallel, feedback1/feedback2 and serial/parallel combination. Keep DSP feedback graph explicit and stable.
- DP/4-style two per-unit modulation assignments and selectable sources, plus Q-Link mappings and coherent touchscreen pages.
- Algorithm slots must support a **1U or 2U or 4U cost/resource tag** for historical comparison even if native Force implementation budgets differ.

**Track A — Original native C++ effects:**
1. Fix long delay range (requested up to 10 s) separately from flanger's narrow sweep.
2. Implement EQ-Panner-DDL, EQ-Compressor, Parametric EQ, VCF-Distortion and De-esser alongside reverb work (not reverbs-only).
3. Implement room/hall 'Definition' network and plate diffusion network as **distinct original circuits** faithful to manual-level topology, with reflection sends, multi-stage diffusion, feedback damping and positioning.
4. Add Gated/Reverse/Nonlinear reverb envelopes, then chorus, expander/ducker, guitar amps and splicer pitch.
5. Validate audible effect, reasonable stability, CPU cost and preset save/restore; skip redundant generic frameworks.

**Track B — ESP execution investigation:**
1. Continue ES5510/ES5511 instruction, register, fixed-point, I/O and delay memory modeling with public specifications and MAME. Implement original tiny assembly/test programs.
2. Determine which public artifacts actually contain **algorithm microcode or tunings**. User manual topology alone cannot prove bit-level fidelity.
3. Keep emulator code isolated, license-compatible and benchmarked; distinguish working ISA subset from successfully running a factory algorithm.
4. Compare measured outputs between native approximation and emulator where useful, but do not block Track A.

## Specific Codex implementation handoff
- First inspect current Git branch and any ongoing V0.2 work; DO NOT duplicate existing efforts, overwrite running agents' changes or break installed V0.1.
- Treat **this file as the DP/4 baseline inventory**. Extract full per-algorithm parameter names, physical ranges, routing diagrams and trigger semantics from the cited manual into a machine-readable index incrementally.
- Prioritize exact documented controller behavior and named signal flow over invented brand-style descriptors.
- Produce concise handoff separating *verified from manual* / *inferred topology* / *implemented and tested* / *awaiting emulator verification*.
- Keep the source manuals external: no copyrighted PDF or proprietary ROM committed.

Sources:
- DP/4 Musician's Manual: https://umlsrt.com/StudioDocuments/Ensoniq%20DP4%20Manual%20.pdf
- Collected Transoniq Hacker articles: https://drummachinefunk.com/files/DP_4_-_Transoniq_Hacker.pdf
- DP/4+ Reference Manual index: https://www.manualslib.com/products/Ensoniq-Dp-4Plus-3245402.html
