#!/usr/bin/env python3
"""Offline, metadata-only scan for candidate ES5510 content in a ROM image.

The scanner does not write ROM bytes, disassemblies, or extracted programs.
All raw-48-bit boundaries reported are heuristic and require independent
loader/structure evidence before being called executable firmware.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import statistics
import sys
from typing import Any

try:
    from .es5510_decode import ALU_NAMES, RAM_NAMES, SPECIAL_REGISTERS
except ImportError:  # Script invocation: python tools/es5510_rom_forensics.py
    from es5510_decode import ALU_NAMES, RAM_NAMES, SPECIAL_REGISTERS

MAX_ROM_BYTES = 16 * 1024 * 1024
WORD_BYTES = 6
PACKINGS = {
    "be48": (0, 1, 2, 3, 4, 5),
    "le48": (5, 4, 3, 2, 1, 0),
    "swap16bytes": (1, 0, 3, 2, 5, 4),
    "reverse16lanes": (4, 5, 2, 3, 0, 1),
    "swap24halves": (3, 4, 5, 0, 1, 2),
    "reverse24halves": (2, 1, 0, 5, 4, 3),
}
ALGORITHM_LABELS = (
    b"Small Room Rev", b"Large Room Rev", b"Hall Reverb",
    b"Small Plate Rev", b"Large Plate Rev", b"Reverse Reverb",
    b"Gated Reverb", b"Non Lin Reverb 1", b"Non Lin Reverb 2",
    b"Non Lin Reverb 3", b"Rotating Speaker", b"Phaser-DDL",
)

# The DP/4 images exhibit these regular, label-bearing tables.  The scanner
# reports their geometry only; the binary fields following labels are not
# decoded or emitted because their semantics and copyright status are unknown.
LABEL_TABLE_HYPOTHESES = (
    {"name": "algorithm-name records", "start": 0x0000, "stride": 0x33, "count": 50, "prefix": b"3"},
    {"name": "effect/preset-name records", "start": 0x09F6, "stride": 0x57, "count": 50, "prefix": b"W"},
    {"name": "named configuration records", "start": 0x2001, "stride": 0x9E, "count": 50, "prefix": b""},
)


class RomScanError(ValueError):
    pass


def _word(data: bytes, offset: int, permutation: tuple[int, ...]) -> int:
    chunk = data[offset:offset + WORD_BYTES]
    raw = bytes(chunk[i] for i in permutation)
    return int.from_bytes(raw, "big")


def scan_stream(data: bytes, offset: int, permutation: tuple[int, ...]) -> dict[str, Any]:
    complete = max(0, (len(data) - offset) // WORD_BYTES)
    alu = Counter()
    selectors = Counter()
    ram = Counter()
    registers: Counter[int] = Counter()
    runs: list[int] = []
    pending = 0
    for ix in range(complete):
        w = _word(data, offset + ix * WORD_BYTES, permutation)
        op = (w >> 12) & 15
        alu[ALU_NAMES[op]] += 1
        selectors[(w >> 8) & 15] += 1
        ram[RAM_NAMES[(w >> 3) & 7]] += 1
        registers.update(((w >> 16) & 255, (w >> 24) & 255,
                          (w >> 32) & 255, (w >> 40) & 255))
        pending += 1
        if op == 15:
            runs.append(pending)
            pending = 0
    ends = alu.get("END", 0)
    return {
        "complete_words": complete,
        "trailing_bytes": len(data) - offset - complete * WORD_BYTES,
        "end_opcode_count": ends,
        "end_opcode_fraction": ends / complete if complete else 0.0,
        "end_terminated_spans_1_to_160": sum(1 <= n <= 160 for n in runs),
        "end_span_median_words": statistics.median(runs) if runs else None,
        "end_span_p95_words": sorted(runs)[int(.95 * (len(runs) - 1))] if runs else None,
        "longest_span_before_end_words": max(runs) if runs else 0,
        "alu_op_counts": dict(sorted(alu.items())),
        "operand_select_counts": {str(k): selectors[k] for k in sorted(selectors)},
        "ram_control_counts": dict(sorted(ram.items())),
        "distinct_register_fields": len(registers),
        "top_register_fields": [
            {"reg": r, "name": SPECIAL_REGISTERS.get(r, f"GPR_{r:02X}"), "uses": n}
            for r, n in registers.most_common(16)
        ],
        "special_register_field_uses": sum(n for r, n in registers.items() if r in SPECIAL_REGISTERS),
    }


def _diff(a: bytes, b: bytes) -> dict[str, Any]:
    if len(a) != len(b):
        return {"same_size": False, "a_size": len(a), "b_size": len(b)}
    changed = [i for i, (x, y) in enumerate(zip(a, b)) if x != y]
    ranges: list[list[int]] = []
    for i in changed:
        if not ranges or i != ranges[-1][1] + 1:
            ranges.append([i, i])
        else:
            ranges[-1][1] = i
    buckets = []
    bucket_size = 4096
    for start in range(0, len(a), bucket_size):
        n = sum(x != y for x, y in zip(a[start:start + bucket_size], b[start:start + bucket_size]))
        if n:
            buckets.append({"start": start, "end_exclusive": min(len(a), start + bucket_size), "changed_bytes": n})
    return {
        "same_size": True,
        "size": len(a),
        "changed_bytes": len(changed),
        "changed_fraction": len(changed) / len(a) if a else 0.0,
        "first_changed_offset": changed[0] if changed else None,
        "last_changed_offset": changed[-1] if changed else None,
        "contiguous_changed_ranges": len(ranges),
        "changed_4k_buckets": buckets,
    }


def analyze_rom(path: Path, compare: Path | None = None) -> dict[str, Any]:
    size = path.stat().st_size
    if size <= 0 or size > MAX_ROM_BYTES:
        raise RomScanError(f"ROM size {size} outside supported 1..{MAX_ROM_BYTES} byte range")
    data = path.read_bytes()
    ascii_matches = []
    for label in ALGORITHM_LABELS:
        pos = 0
        while (pos := data.find(label, pos)) >= 0:
            ascii_matches.append({"offset": pos, "label": label.decode("ascii")})
            pos += 1
    ascii_matches.sort(key=lambda e: e["offset"])
    label_tables = []
    for table in LABEL_TABLE_HYPOTHESES:
        records = []
        for index in range(table["count"]):
            offset = table["start"] + index * table["stride"]
            if offset >= len(data):
                break
            record = data[offset:offset + table["stride"]]
            if table["prefix"] and not record.startswith(table["prefix"]):
                break
            label_bytes = record[:20].split(b"\0", 1)[0]
            label_bytes = label_bytes[:next((i for i, c in enumerate(label_bytes) if c < 0x20 or c > 0x7E), len(label_bytes))]
            label = label_bytes.decode("ascii", errors="replace").strip()
            if not label:
                break
            records.append({"index": index, "offset": offset, "label": label})
        if records:
            label_tables.append({
                "name": table["name"], "start": table["start"],
                "stride_bytes": table["stride"], "candidate_records": len(records),
                "end_exclusive": table["start"] + len(records) * table["stride"],
                "records": records,
                "interpretation": "regular label-bearing region; record field meanings unverified",
            })
    packings: dict[str, Any] = {}
    for name, permutation in PACKINGS.items():
        packings[name] = [
            {"byte_alignment": alignment, **scan_stream(data, alignment, permutation)}
            for alignment in range(WORD_BYTES)
        ]
    result: dict[str, Any] = {
        "schema": "trollfx.es5510-rom-forensics.v1",
        "source_file_name": path.name,
        "size_bytes": size,
        "sha256": hashlib.sha256(data).hexdigest(),
        "ascii_algorithm_label_occurrences": ascii_matches,
        "label_table_hypotheses": label_tables,
        "raw_48bit_packing_hypotheses": packings,
        "limits": [
            "The UCODE/OS role is not inferred from bytes; correlate file names and labels with service documentation.",
            "END/opcode spans are a hypothesis scan over data, not recovered program boundaries.",
            "ES5510 GPR values, external DRAM, upload order, per-processor routing and sample schedule are not recoverable from a word scan alone.",
            "Only MAME-defined ES5510 fields are decoded; reserved bits and hardware deviations remain unverified.",
            "No binary instruction words or disassembly are emitted by this scanner.",
        ],
    }
    if compare:
        other = compare.read_bytes()
        result["comparison"] = {
            "other_file_name": compare.name,
            "other_size_bytes": len(other),
            "other_sha256": hashlib.sha256(other).hexdigest(),
            **_diff(data, other),
        }
    return result


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rom", type=Path, help="private local ROM image; never put it in Git")
    ap.add_argument("--compare", type=Path, help="compare with another same-size image")
    ap.add_argument("--json-output", type=Path, help="write metadata-only report, never ROM bytes")
    ap.add_argument("--compact", action="store_true", help="write compact JSON")
    args = ap.parse_args(argv)
    try:
        report = analyze_rom(args.rom, args.compare)
    except (OSError, RomScanError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    rendered = json.dumps(report, indent=None if args.compact else 2, sort_keys=True)
    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(rendered + "\n", encoding="utf-8")
    be = report["raw_48bit_packing_hypotheses"]["be48"][0]
    le = report["raw_48bit_packing_hypotheses"]["le48"][0]
    print(f"file: {report['source_file_name']} ({report['size_bytes']} bytes)")
    print(f"sha256: {report['sha256']}")
    print(f"algorithm-label byte-string hits: {len(report['ascii_algorithm_label_occurrences'])}")
    for table in report.get("label_table_hypotheses", []):
        print(f"table candidate: {table['name']}: {table['candidate_records']} records, "
              f"stride {table['stride_bytes']} bytes, "
              f"offsets 0x{table['start']:x}..0x{table['end_exclusive']:x} (exclusive)")
    print(f"be48@0: END={be['end_opcode_count']} ({be['end_opcode_fraction']:.1%}), "
          f"median END span={be['end_span_median_words']} words, "
          f"bounded spans={be['end_terminated_spans_1_to_160']}")
    print(f"le48@0: END={le['end_opcode_count']} ({le['end_opcode_fraction']:.1%}), "
          f"median END span={le['end_span_median_words']} words, "
          f"bounded spans={le['end_terminated_spans_1_to_160']}")
    if "comparison" in report:
        c = report["comparison"]
        print(f"compare: {c.get('changed_bytes', 'size mismatch')} changed bytes, "
              f"{c.get('changed_fraction', 0.0):.3%}, "
              f"offsets {c.get('first_changed_offset')}..{c.get('last_changed_offset')}")
    if args.json_output:
        print(f"metadata report: {args.json_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

