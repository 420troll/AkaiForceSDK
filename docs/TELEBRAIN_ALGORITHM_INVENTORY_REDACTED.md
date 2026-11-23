# TeleBrain algorithm/table inventory (redacted)

This inventory reports identifiers, schema reuse, and association confidence. It
does not reproduce CSV parameter rows or other archive payload. The private
catalog contains 52 numbered tables and 1,137 rows.

## Table and algorithm association

The Max patch has four unit cores. Each core contains a `read $1.csv` message
feeding the script that reads a semicolon-delimited parameter table. The script
uses each row's display number in the outgoing parameter-change message, reads
the raw range for its slider scaling, and keeps the displayed number distinct
from the internal parameter offset retained in the catalog. This verifies the
editor's table-selection convention: an algorithm/menu value `N` requests
`N.csv`. It does not prove that every file's header is correct or that a file
contains all controls accepted by the hardware.

The names below are redacted to the algorithm identity level. An association
is **loader-supported** when the patch requests the corresponding numbered CSV;
the exact algorithm name remains subject to the source label and DP/4 manual.
An asterisk marks a family/variant label rather than a claim of separate DSP
microcode.

| Table / algorithm index | Algorithm label | Association status |
|---:|---|---|
| 0 | No Effect | Loader-supported |
| 1 | Small Room Reverb | Shared schema with 2–3 |
| 2 | Large Room Reverb | Shared schema with 1, 3 |
| 3 | Hall Reverb | Shared schema with 1–2 |
| 4 | NonLin Reverb 1 | Shared schema with 46, 59 |
| 5 | Small Plate | Shared schema with 6 |
| 6 | Large Plate | Shared schema with 5 |
| 7 | Gated Reverb | Loader-supported |
| 8 | Reverse Reverb | Loader-supported; distinct from table 54 |
| 9 | MultiTap Delay | Loader-supported |
| 10 | Dual Delay | Loader-supported; confirmed by public SysEx listing |
| 11 | Tempo Delay | Loader-supported |
| 12 | Parametric EQ | Loader-supported |
| 13, 27 | Guitar Amp 1 variants | Same schema; algorithm variants remain separate IDs |
| 14 | VCF Distortion | Loader-supported |
| 15 | Speaker Cabinet | Source label; index/name check recommended |
| 16 | Rotating Speaker | Loader-supported |
| 17 | EQ Compressor | Loader-supported |
| 19 | Expander | Loader-supported |
| 20 | Inverse Expander | Loader-supported |
| 21 | 8 Voice Chorus | Loader-supported |
| 22 | EQ Chorus DDL | Loader-supported |
| 23 | Flanger | Loader-supported; confirmed by public SysEx listing |
| 24 | Phaser DDL | Loader-supported |
| 25 | Pitch Shift DDL | Loader-supported |
| 26 | Pitch Shifter | Loader-supported |
| 28 | Vander Pol Filter | Loader-supported |
| 29 | Sine/Noise Generator | Loader-supported |
| 32–35 | Vocoder band tables* | Shared schema; exact band-to-index assignment unresolved |
| 36–37 | Pitch Shift 2U variants | Same schema |
| 38 | EQ Tremolo DDL | Loader-supported |
| 39 | EQ Vibrato DDL | Loader-supported |
| 40 | EQ Panner DDL | Loader-supported |
| 41 | De-esser | Loader-supported |
| 42 | Ducker / Gate | Loader-supported |
| 43–44 | 3.3 sec DDL 2U variants | Same schema |
| 45 | Tunable Speaker | Loader-supported |
| 46 | NonLin Reverb 2 | Shared schema with 4, 59 |
| 47 | EQ Flanger DDL | Loader-supported |
| 48 | Fast Pitch Shift | Loader-supported |
| 50 | Guitar Amp 3 | Loader-supported |
| 53 | EQ DDL with LFO | Loader-supported |
| 54 | Reverse Reverb 2 | **Ambiguous header**; see below |
| 55 | Rumble Filter | Loader-supported |
| 58 | Keyed Expander | Loader-supported |
| 59 | NonLin Reverb 3 | Shared schema with 4, 46 |

The archive has 52 present table files, corresponding to 52 of the editor's
numbered lookup slots. Seven schema signatures repeat exactly, leaving 41
distinct row layouts. The exact repeated groups are `1/2/3`, `4/46/59`, `5/6`,
`13/27`, `32/33/34/35`, `36/37`, and `43/44`. A repeated schema establishes a
shared control layout, not a shared algorithm implementation.

## Ambiguity: reverse reverb table 54

Table 54's CSV header claims algorithm index 08 even though the patch's
`read $1.csv` convention selects file 54 for selector value 54. The public
DP/4+ SysEx specification separately lists Reverse Reverb at index 08 and
Reverse Reverb 2 at index 54. This supports keeping the two tables and IDs
separate, while flagging the table-54 header as inconsistent. It does not
resolve whether the mismatch is an authoring/transcription error or a special
editor behavior. Do not merge these tables.

## Search and generation

The full normalized local JSON is private. Generate it from the already-created
local extractor catalog and search it with `tools/normalize_telebrain.py`; see
[`TELEBRAIN_CATALOG_MILESTONE_2.md`](TELEBRAIN_CATALOG_MILESTONE_2.md).
