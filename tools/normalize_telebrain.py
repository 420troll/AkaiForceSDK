#!/usr/bin/env python3
"""Normalize a private TeleBrain extractor JSON without copying source tables.

The output is a local research artifact. This tool and its synthetic fixtures are
the redistributable parts; do not commit the normalized catalog.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

FORMAT = "trollfx.telebrain.normalized-local.v2"

# Names identify DP/4 manual families; table association remains separate.
# A numbered table is loaded from a matching numbered filename by the patch.
ALGORITHMS: dict[str, str] = {
    "0": "No Effect", "1": "Small Room Reverb", "2": "Large Room Reverb",
    "3": "Hall Reverb", "4": "NonLin Reverb 1", "5": "Small Plate",
    "6": "Large Plate", "7": "Gated Reverb", "8": "Reverse Reverb",
    "9": "MultiTap Delay", "10": "Dual Delay", "11": "Tempo Delay",
    "12": "Parametric EQ", "13": "Guitar Amp 1", "14": "VCF Distortion",
    "15": "Speaker Cabinet", "16": "Rotating Speaker", "17": "EQ Compressor",
    "19": "Expander", "20": "Inverse Expander", "21": "8 Voice Chorus",
    "22": "EQ Chorus DDL", "23": "Flanger", "24": "Phaser DDL",
    "25": "Pitch Shift DDL", "26": "Pitch Shifter", "27": "Guitar Amp 1 variant",
    "28": "Vander Pol Filter", "29": "Sine/Noise Generator",
    "32": "Vocoder band (table label grouping unresolved)",
    "33": "Vocoder band (table label grouping unresolved)",
    "34": "Vocoder band (table label grouping unresolved)",
    "35": "Vocoder band (table label grouping unresolved)",
    "36": "Pitch Shift 2U", "37": "Pitch Shift 2U variant",
    "38": "EQ Tremolo DDL", "39": "EQ Vibrato DDL", "40": "EQ Panner DDL",
    "41": "De-esser", "42": "Ducker / Gate", "43": "3.3 sec DDL 2U",
    "44": "3.3 sec DDL 2U variant", "45": "Tunable Speaker",
    "46": "NonLin Reverb 2", "47": "EQ Flanger DDL", "48": "Fast Pitch Shift",
    "50": "Guitar Amp 3", "53": "EQ DDL with LFO", "54": "Reverse Reverb 2",
    "55": "Rumble Filter", "58": "Keyed Expander", "59": "NonLin Reverb 3",
}

def _integer(raw: Any) -> int | None:
    if isinstance(raw, int) and not isinstance(raw, bool):
        return raw
    if isinstance(raw, str) and re.fullmatch(r"\s*[+-]?\d+\s*", raw):
        return int(raw)
    return None

def _bounds(raw: Any) -> list[int] | None:
    if not isinstance(raw, str):
        return None
    match = re.fullmatch(r"\s*([+-]?\d+)\s*,\s*([+-]?\d+)\s*", raw)
    return [int(match.group(1)), int(match.group(2))] if match else None

def normalize(source: dict[str, Any]) -> dict[str, Any]:
    if source.get("format") != "trollfx.telebrain.local-catalog.v1":
        raise ValueError("expected trollfx.telebrain.local-catalog.v1 input")
    source_tables = source.get("tables")
    if not isinstance(source_tables, dict):
        raise ValueError("input tables must be an object")

    tables: list[dict[str, Any]] = []
    diagnostics: list[dict[str, Any]] = []
    seen_table_ids: set[str] = set()
    for key, table in source_tables.items():
        table_id = str(key)
        if table_id in seen_table_ids:
            diagnostics.append({"severity": "error", "code": "duplicate_table_id", "table_id": table_id})
        seen_table_ids.add(table_id)
        if not re.fullmatch(r"\d+", table_id):
            diagnostics.append({"severity": "error", "code": "malformed_table_id", "table_id": table_id})
        if not isinstance(table, dict) or not isinstance(table.get("rows"), list):
            diagnostics.append({"severity": "error", "code": "malformed_table", "table_id": table_id})
            continue

        associations: list[dict[str, Any]] = []
        numeric_id = _integer(table_id)
        if numeric_id is not None and table_id in ALGORITHMS:
            associations.append({
                "algorithm_id": numeric_id,
                "algorithm_name": ALGORITHMS[table_id],
                "relationship": "dynamic_filename_lookup_candidate",
                "evidence": "Max patch constructs read $1.csv; table number correspondence is the filename convention",
                "confidence": "medium",
            })
        if table_id == "54":
            associations.append({
                "algorithm_id": 8,
                "algorithm_name": "Reverse Reverb",
                "relationship": "header_and_spec_conflict_review",
                "evidence": "table 54 header claims index 08 while table 8 and public SysEx index listings distinguish 08 and 54",
                "confidence": "ambiguous",
            })
        source_warnings = table.get("warnings", [])
        for item in source_warnings if isinstance(source_warnings, list) else []:
            if isinstance(item, dict) and item.get("code") == "missing_modulation_offsets":
                diagnostics.append({"severity": "warning", "code": "missing_modulation_offsets",
                                    "table_id": table_id, "values": item.get("values", [])})

        rows: list[dict[str, Any]] = []
        display_seen: dict[int, list[int]] = {}
        offset_seen: dict[int, list[int]] = {}
        for pos, row in enumerate(table["rows"]):
            if not isinstance(row, dict):
                diagnostics.append({"severity": "error", "code": "malformed_row", "table_id": table_id, "row_index": pos})
                continue
            line = _integer(row.get("line"))
            display = _integer(row.get("display_order"))
            offset = _integer(row.get("offset"))
            label = row.get("name_raw")
            kind = row.get("type_raw")
            raw_range = row.get("internal_range_raw")
            display_range = row.get("display_range_raw")
            malformed: list[str] = []
            if not isinstance(label, str) or not label.strip(): malformed.append("label")
            if display is None and row.get("display_order_raw") not in (None, ""): malformed.append("display_parameter_number")
            if offset is None and row.get("offset_raw") not in (None, ""): malformed.append("internal_offset")
            if not isinstance(kind, str) or not kind.strip(): malformed.append("control_type")
            if not isinstance(raw_range, str): malformed.append("raw_range")
            if not isinstance(display_range, str): malformed.append("display_range")
            if malformed:
                diagnostics.append({"severity": "warning", "code": "malformed_row_fields", "table_id": table_id, "line": line, "fields": malformed})
            if display is not None: display_seen.setdefault(display, []).append(line if line is not None else pos)
            if offset is not None: offset_seen.setdefault(offset, []).append(line if line is not None else pos)
            rows.append({
                "source_line": line,
                "label": label.strip() if isinstance(label, str) else None,
                "display_parameter_number": display,
                "display_parameter_number_raw": row.get("display_order_raw"),
                "internal_offset": offset,
                "internal_offset_raw": row.get("offset_raw"),
                "control_type": kind.strip() if isinstance(kind, str) else None,
                "raw_range": raw_range,
                "raw_bounds": _bounds(raw_range),
                "display_range": display_range,
                "display_bounds": _bounds(display_range),
                "default": None,
                "unit": None,
                "editable": row.get("editable") if isinstance(row.get("editable"), bool) else None,
                "source_modulation_role": row.get("mod_role"),
            })
        for value, lines in display_seen.items():
            if len(lines) > 1:
                diagnostics.append({"severity": "warning", "code": "duplicate_display_parameter_number", "table_id": table_id, "value": value, "source_lines": lines})
        for value, lines in offset_seen.items():
            if len(lines) > 1:
                diagnostics.append({"severity": "warning", "code": "duplicate_internal_offset", "table_id": table_id, "value": value, "source_lines": lines})
        if any(a["confidence"] == "ambiguous" for a in associations):
            diagnostics.append({"severity": "warning", "code": "ambiguous_algorithm_mapping", "table_id": table_id, "candidate_ids": [a["algorithm_id"] for a in associations]})

        tables.append({
            "table_id": numeric_id,
            "table_filename": table.get("source_filename"),
            "source_sha256": table.get("source_sha256"),
            "source_warnings": source_warnings if isinstance(source_warnings, list) else [],
            "algorithm_associations": associations,
            "associations_ambiguous": any(a["confidence"] == "ambiguous" for a in associations),
            "rows": rows,
        })
    tables.sort(key=lambda t: (t["table_id"] is None, t["table_id"] or 0))
    return {
        "format": FORMAT,
        "source_note": "Private user-licensed TeleBrain source. Keep this generated catalog outside Git.",
        "defaults_policy": "unknown unless independently documented; null means not supplied by source",
        "units_policy": "unknown unless independently documented; raw/display ranges are not units",
        "tables": tables,
        "diagnostics": diagnostics,
    }

def search(catalog: dict[str, Any], query: str = "", table_id: int | None = None,
           offset: int | None = None, display_number: int | None = None) -> list[dict[str, Any]]:
    q = query.casefold()
    found: list[dict[str, Any]] = []
    for table in catalog.get("tables", []):
        if table_id is not None and table.get("table_id") != table_id: continue
        names = " ".join(a.get("algorithm_name", "") for a in table.get("algorithm_associations", []))
        for row in table.get("rows", []):
            if offset is not None and row.get("internal_offset") != offset: continue
            if display_number is not None and row.get("display_parameter_number") != display_number: continue
            if q and q not in (row.get("label", "") + " " + names + " " + str(table.get("table_id"))).casefold(): continue
            found.append({"table_id": table.get("table_id"), "algorithm_names": names,
                          "label": row.get("label"), "display_parameter_number": row.get("display_parameter_number"),
                          "internal_offset": row.get("internal_offset"), "control_type": row.get("control_type"),
                          "raw_range": row.get("raw_range"), "display_range": row.get("display_range"),
                          "default": row.get("default"), "unit": row.get("unit")})
    return found

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="private local-catalog.v1 JSON from extract_telebrain.py")
    parser.add_argument("--output", type=Path, help="write normalized local JSON (do not commit)")
    parser.add_argument("--search", default="", help="case-insensitive label or algorithm-name query")
    parser.add_argument("--table", type=int)
    parser.add_argument("--offset", type=int)
    parser.add_argument("--display-number", type=int)
    args = parser.parse_args()
    try:
        catalog = normalize(json.loads(args.input.read_text(encoding="utf-8")))
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(f"normalized catalog written: {args.output}")
    results = search(catalog, args.search, args.table, args.offset, args.display_number)
    for row in results:
        print(json.dumps(row, ensure_ascii=False, sort_keys=True))
    errors = sum(d["severity"] == "error" for d in catalog["diagnostics"])
    warnings = sum(d["severity"] == "warning" for d in catalog["diagnostics"])
    print(f"tables={len(catalog['tables'])} rows={sum(len(t['rows']) for t in catalog['tables'])} errors={errors} warnings={warnings}", file=sys.stderr)
    return 1 if errors else 0

if __name__ == "__main__":
    raise SystemExit(main())
