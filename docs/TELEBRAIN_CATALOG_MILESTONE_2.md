# TeleBrain catalog normalization — Milestone 2

## Result

Added a deterministic normalizer/search utility for the private 52-table,
1,137-row extractor result. Each parameter retains the source label, display
parameter number and internal offset as separate fields, raw type, exact raw
and display range strings, parsed bounds when unambiguous, source line, and
nullable `default` and `unit`. It does not infer units, factory defaults, or
physical scaling from numeric ranges. `Table` indices remain indexes, not
physical values.

The normalized catalog was generated locally at:

`/mnt/c/Users/Gamer/research/private-results/telebrain_catalog.normalized.local.json`

That file is proprietary-derived local research data and is intentionally not
tracked. The original archive was read only; it was not changed. No CSV rows,
archive payload, Max patch, or editor assets are included in this branch.

## Repeatable local workflow

If the existing private extractor catalog is available:

```sh
python3 tools/normalize_telebrain.py \
  /mnt/c/Users/Gamer/research/private-results/telebrain_catalog.local.json \
  --output /mnt/c/Users/Gamer/research/private-results/telebrain_catalog.normalized.local.json
python3 tools/normalize_telebrain.py \
  /mnt/c/Users/Gamer/research/private-results/telebrain_catalog.local.json \
  --search "feedback" --table 24
```

Regenerate the extractor catalog from the user's licensed archive when needed:

```sh
python3 tools/extract_telebrain.py "/local/path/to/DP4 TeleBrain.zip" \
  --output /mnt/c/Users/Gamer/research/private-results/telebrain_catalog.local.json
```

Search supports case-insensitive label/algorithm text plus `--table`,
`--offset`, and `--display-number` filters. Output is one JSON record per match.
Diagnostics preserve malformed source fields and report repeated display
numbers, repeated offsets, and ambiguous table/algorithm associations.

## Evidence and SysEx cross-check

### Confirmed from the Max patch

- Unit A/B/C/D each has a core that selects a file via a `read $1.csv` message.
- The JS reads semicolon-separated rows, uses the display parameter number to
  instantiate a DP/4 parameter-change sender, and uses the raw range for the
  editor slider's numeric scale.
- The internal offset is not the emitted display parameter number. They must
  never be conflated in DSP parameter IDs.
- A `Signed Frac` branch applies a custom transform before sending. This is
  evidence of an editor-specific encoding step, not proof that the transformed
  value is a physical parameter unit.

### Independently cross-checked

The public **DP/4+** MIDI SysEx specification describes parameter-change
commands, preset/edit-buffer dumps, algorithm indices, and per-algorithm
parameter display number, structure offset, type, legal stored range, and
front-panel display range. It explicitly distinguishes the display number
from the algorithm-block offset and defines type-specific scaling. Selected
table families and numeric fields can therefore be compared independently.
The spec is a DP/4+ document, however, so a matching DP/4 field is a useful
cross-check, not blanket proof that every DP/4 and DP/4+ algorithm is identical.

Specific checks confirmed in the available source text include the Flanger
family (algorithm index 23, signed regeneration range) and Reverse Reverb
family indices 08 and 54; the reverse-reverb check exposes the table-54 header
conflict. Room/Hall and Plate layouts have same-named fields in the TeleBrain
tables, but full range equivalence should be verified against the original DP/4
v1.02 spec before treating the DP/4+ source as authoritative for that product.

For a concrete matched tuple, table 23 records **Flanger Regen** as display
parameter 6, internal offset 4, `Signed Frac`, raw `-127..127`, displayed
`-98..99`. The public SysEx algorithm-23 listing independently gives that same
parameter number, block offset, type, and ranges. This is verified for that
field in the DP/4+ document and supports the schema's two-number model; it is
not a claim that all source rows have been reconciled.

The spec's parameter-change command carries a unit, display parameter number,
and a 16-bit value; an 8-bit parameter is sent with the high byte zero. The
bulk preset format separately defines internal structure offsets. Thus SysEx
can independently check both identifiers and legal value representations.
These are stored/editor scales; a range such as `0..127` does not by itself
mean milliseconds, hertz, or a linear physical mapping. The spec describes
preset and parameter data, not access to ES5510 instruction RAM.

The public catalog names the original DP/4 v1.02 SysEx spec, but its download
host is MediaFire. The accessible PDF used for this milestone is the DP/4+
v2.05.01 document hosted by R-Massive. **No exact original DP/4 v1.02 PDF was
locally available for direct independent comparison in this run.**

## Validation findings

The normalizer produced 52 tables / 1,137 rows. It reports existing source
inconsistencies rather than repairing them: four rows with nonnumeric
identifiers, four duplicate display-number instances, three duplicate-offset
instances, the extractor's missing modulation-offset annotations, and the
ambiguous table-54 algorithm association. Extractor warning records are kept
at table level. Exact repeated schema groups are recorded in the redacted
inventory. The complete row catalog remains local.

Synthetic unit tests cover:

- independent preservation of display numbers and internal offsets;
- malformed fields retained as null with a diagnostic;
- duplicate display and internal identifiers;
- explicitly ambiguous algorithm/table association and search;
- duplicate table identifiers.

Run with:

```sh
python3 -m unittest discover -s tests -v
```

## Proposed Force Q-Link groups

These are proposed groupings, not implemented controls or evidence of behavior.
Keep a consistent navigation row across each family: **Q1 algorithm/preset**,
**Q2 Mix**, **Q3 Volume**, **Q4 Bypass/Enable** when the host exposes these.
Use the remaining four links for the selected algorithm's meaningful controls:

| Family / algorithm IDs | Q5 | Q6 | Q7 | Q8 |
|---|---|---|---|---|
| Room/Hall (1–3) | Decay | Predelay | HF damping | Diffusion / early-reflection level |
| Plate (5–6) | Decay | Predelay | HF damping | Diffusion / early-reflection balance |
| NonLin/Gated/Reverse reverb (4, 7, 8, 46, 54, 59) | Envelope/decay or hold | Threshold | Diffusion | Damping / pre-trigger memory if present |
| Delay / multitap (9–11, 43–44) | Delay/time | Fine time | Feedback | Pan / echo level |
| Modulation / hybrid delay (21–25, 38–40, 47, 53) | Rate | Depth/width | Center or delay time | Feedback / pan |
| EQ / filter / distortion / speaker (12, 14–15, 28, 45, 55) | Frequency | Gain/drive | Resonance/Q | Output/mix |
| Dynamics (17, 19–20, 41–42, 58) | Threshold | Ratio | Attack | Release / gate |
| Rotary / amp (13, 16, 27, 50) | Rate/speed | Drive | Tone/cabinet | Level |
| Pitch (25–26, 36–37, 48) | Shift interval | Fine tune | Modulation | Delay/mix |
| Vocoder / signal generator (29, 32–35) | Frequency/band | Balance | Filter/EQ | Level |

Resolve duplicate identifiers before assigning stable host automation IDs.
For tables with a `Function` type, render it as a meter/status rather than an
editable Q-Link.

## Source and rights notes

- Private licensed TeleBrain archive and extracted tables: local only.
- Ensoniq, *DP/4+ MIDI SysEx Specification* v2.05.01, §§3.1.1, 3.3–3.4, 4, and 7; public reference copy at [R-Massive](https://godlike.au/fileadmin/godlike/hardware/ensoniq/DP4__Sysex_OCR.pdf), accessed 2026-11-23. The parameter message is described on appendix page A-3; the algorithm fields are in §7. Product scope remains DP/4+.
- [R-Massive Ensoniq technical document index](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/), accessed 2026-11-23: lists the DP/4 v1.02 and DP/4+ SysEx references separately.
- DP/4 algorithm names/behavior: project research and the DP/4 Musician's Manual link in `docs/DP4_ALGORITHM_RESEARCH.md`.

## Next experiment

Acquire/read the original DP/4 SysEx v1.02 document, then compare each source
table's tuple `(display number, offset, type, internal range, display range)`
against the matching algorithm section. First resolve table 54, table 40's
duplicate IDs, the malformed values in tables 12/17/19/24, and whether table
groups 32–35 are four names sharing one layout or a single family schema reused
by distinct algorithms.
