# TeleBrain parameter-catalog handoff — DP/4 DSP and Force UI

**Source inspected:** User-provided archive `Ariè - DP4 TeleBrain.zip` containing a Max for Live `.amxd`, `patchScripter3.3.js`, and 52 numbered CSV parameter tables. The separate Windows-external archive only contains binary Max external modules.

**Scope:** research and original DSP/skin implementation; no MIDI hardware debugging or protocol flood testing.

## Extraction results (local, not redistributed)

A read-only extractor is in `tools/extract_telebrain.py`. Run it against the user's own archive:

```sh
python3 tools/extract_telebrain.py "/path/to/Ariè - DP4 TeleBrain.zip" --output build/telebrain_catalog.local.json
```

The run on the supplied archive yielded **52 CSV files**, **1,137 total parameter rows**, with 4 duplicate-display-order warnings, 3 duplicate-internal-offset warnings, 3 missing-modulation-offset warnings and 3 nonnumeric-display-order warnings. Some warnings co-occur in the same table. These are *source text quality signals*, not evidence of bad hardware or editor behavior.

Each local JSON row retains original label, display ordinal, underlying internal parameter offset, value type, internal raw range, displayed raw range, table/algorithm filename, original line and a modulation-role annotation where its offset is unambiguous. The program does not interpret `Table` indices as calibrated physical values or infer missing table entries. **Do not commit the generated JSON, the paid TeleBrain editor, CSV files or their artwork into this public repository without redistribution rights.** Local catalog remains available for Codex on the user's WSL machine.

The `.amxd` is a Max patch bundle with a readable JSON `patcher`, and the JS reveals dynamic UI construction and controller labels. Its MIDI output is not relevant to the Force DSP implementation. Do not execute unknown Max/JS content in the build.

## Key DSP/control findings (examples, not a complete parameter dump)

* **Small/Large Room + Hall** (tables 1/2/3): shared control family, 30 rows each. There is a distinct Room/Hall Decay (table index), Predelay Time displayed 0–450, LF Decay Time, HF Damping, HF Bandwidth, Diffusion 1/2, Decay Definition, Detune Rate/Depth, Primary Send, Reflection 1/2 Time displayed 0–120, Reflection Levels/Sends and three Position Balance controls. Implement these *as separate audible controls*, not generic Time/Depth.
* **Small/Large Plate** (tables 5/6): 22 rows each. Decay and Predelay, HF Damping/Bandwidth, Diffusion 1/2, Decay Definition, four early-reflection levels and Left/Right Balance. These differ structurally from room/hall parameters.
* **VCF-Distortion** (table 14): 22 rows. Separate Distortion Level In/Out, Pre-distortion VCF Fc and Q, pre-filter envelope-follow amount, Post-distortion VCF Fc/Q, post-filter envelope-follow amount, follower attack/release, distortion bypass and pre-EQ high-pass cutoff. Existing V0.2 source implements a simplified filtered soft clip; it needs real independent pre/post filter stages and corresponding control mappings.
* **EQ-Compressor** (table 17): 24 rows, with Compressor Gain, Ratio, Threshold, Attack, Release, two gate thresholds, Gate Release, Bass/Treble frequency/gain and EQ Input Trim. `GainChange` is flagged Function/non-editable, so render it as a meter, not a knob. The source header includes a malformed '48,48' internal range for compressor gain; keep displayed -48..48 pending manual check.
* **Flanger** (table 23): 14 rows; only four effect-specific editable controls before generic Mix/Volume and dual modulation: LFO Rate, LFO Width, Center, and signed Regen. No generic 10-second Time control belongs on the flanger UI.
* **EQ-Panner-DDL** (table 40): 24 rows; Panner Rate, Width + Left/Right LFO (merged ambiguous source cell), Sample & Hold Rate, Left and Right Delay Time, Regen, Left and Right Echo Time, Echo Level, Bass/Treble EQ and Input Level Trim. Source CSV has duplicate display ordinals and a duplicate internal offset: verify against original manual before assigning stable param IDs.
* **De-esser** (table 41): 30 rows including Output Gain, Ratio/Threshold/Attack/Release, noise gate thresholds, sidechain EQ HighPass, bass, two mid parametric sections, treble and sidechain trim. The 'Gain Change' Function row is a meter, not an editable parameter.

**Dual modulation:** Most files contain the same eight **internal offsets 24–31**: Mod1 Source, Destination, Range Min/Max; Mod2 Source, Destination, Range Min/Max. Source values are Off plus Controller 1..8. A source table may have misspellings like `Modi` or `Modl`; prefer internal offset classification where consistent. Some rows are erroneous or absent; warnings must be resolved per manual. Important: these controller numbers describe DP/4 system controller slots, **not** a guarantee that arbitrary MIDI CC reaches Akai Force effect plug-ins.

**Catalog coverage caveat:** Numbered files are *editor data tables*, not 52 proven unique DP/4 algorithms. Some algorithm numbers share identical tables and some header OCR/transcription artifacts name several algorithms in one file. Filename/index convention is a lookup key, not guaranteed to match the original DP/4 physical algorithm number in all cases. Cross-check against `docs/DP4_ALGORITHM_RESEARCH.md` and the original DP/4 manual before claiming catalog completeness, numerical units, defaults, or exact source code behavior.


## Table reuse and scope

Comparing normalized parameter field sequences (display label, internal offset, type, display range), the 52 files have **41 distinct parameter-table structures**. Exact repeats: `[1,2,3]` (Room/Hall), `[4,46,59]` (Nonlinear Reverb), `[5,6]` (Plate), `[13,27]` (Guitar Amp variants), `[32,33,34,35]` (Vocoder bands), `[36,37]` (two-unit pitch), and `[43,44]` (two-unit delay). **8.csv and 54.csv are *not* equivalent** despite both identifying Reverse Reverb; inspect the difference before merging them. File numbers absent from the archive do not establish algorithms absent from hardware.

Observed row value-type distribution: Pos Frac 353, Pos Int 293, Table 187, Signed Int 114, Signed Frac 88, Word 70, Word 2 18, Function 7, Pos Int 2 5, combined Pos Frac Table 2. The mixed `Pos Frac Table` rows are ambiguous and need manual review. Parameter names/descriptions are facts from a third-party editor data table, not proof of algorithm-level coefficients.

## Priorities for Codex

1. **Make reverbs audible first.** In `plugins/TROLLfxESPLabV02/src/audio_engine.cpp` `processReverb` computes a damped tank output `v` but does not use it in feedback; it also does not link UI Time to decay. Trace wet output and routing with isolated wet-only impulse renders. Implement clear Decay, Pre-delay, Diffusion 1/2, damping, reflection taps and width; measurable tails at suitable times.
2. **Introduce a typed parameter-spec layer** keyed by stable algorithm and parameter IDs. Keep display label, units, physical min/max, mapping function, per-algorithm visibility and modulation eligibility separate from DP/4 internal offset metadata. Table-indexed values need reconstructed lookup tables from a validated source, not raw 0–127 treated as milliseconds or seconds.
3. **Connect algorithm-specific DSP and controls.** For currently implemented algorithms prioritize Room/Hall, Plate, EQ-Compressor, VCF-Distortion, Flanger, EQ-Panner-DDL. Don't draw controls whose audio effect is stubbed. Stage newer algorithms such as De-esser, gate, amp models, pitch shift according to actual DSP readiness.
4. **Implement two functional modulation slots per unit** (source, destination, min/max, invert by reversed endpoints), starting with on-device internal LFO/envelope and Force automation where available; do not rely on unsupported insert-FX MIDI events. Modulated values must reach the correct DSP method. Persist four units and routes.
5. **Build a DP/4-inspired TROLLfx skin** in the official Akai SDK JSON format; A/B/C/D unit overview, separate algorithm-aware main control and MOD pages, Q-Links in groups of four. Use original original graphics, not reusing paid editor assets or Ensoniq logos. The SDK's parameter collection is fixed at initialization, so design visibility/paging against the actual TUI conditional support and test tab switching with the host simulator and then Force.
6. **Run necessary tests only:** native/ARM32 build, SDK validator/host 20 checks, a wet-only reverb impulse render, one mod-to-DSP audio test, and state restoration. Package separately as V0.3 and retain V0.1/V0.2. No hardware deployment.

## Acceptance checks

- Each chosen algorithm exposes controls meaningful for *that* DSP and no dead knobs.
- Room/Hall/Plate audible tails with wet at 100%; damping changes spectrum and decay changes tail time.
- VCF pre/post and EQ-Compressor gain/threshold/ratio have audible effects and valid units.
- Two independent mod slots can target different supported parameters in the same unit; changes survive preset reload.
- The Force screen shows routing and four units with readable touch targets; Q-Links correspond to visible controls.
- GitHub commit contains original code, extractor and handoff, **not** the purchased TeleBrain editor or its CSV database.

## Sources
- User-provided TeleBrain .zip (private)
- Ensoniq DP/4 manual: https://umlsrt.com/StudioDocuments/Ensoniq%20DP4%20Manual%20.pdf
- Internal research: `docs/DP4_ALGORITHM_RESEARCH.md`
- Active issues: #1 (reverbs), #2 (modulation/skin)
