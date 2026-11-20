#!/usr/bin/env python3
"""Emit a metadata-only manifest for private ASR-10 factory effect objects.

The JSON contains names, file/object hashes, pointer values, candidate section
offsets, decoded instruction statistics, and cross-version hash comparisons.
It never emits any firmware or instruction bytes. This is static evidence,
not proof that the candidate section was uploaded or fully initialized.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

try:
    from .asr10_efe_inspect import inspect
except ImportError:
    from asr10_efe_inspect import inspect


def scan(root: Path) -> dict[str, dict]:
    result: dict[str, dict] = {}
    for path in sorted(root.glob("*.payload.bin")):
        try:
            row = inspect(path.read_bytes())
        except ValueError:
            # The extracted directory also contains the OS object, which is
            # not an EFE container. A malformed effect still causes the
            # expected twelve-object count check below to fail.
            continue
        if not row.get("candidate_microcode"):
            continue
        name = str(row["name"])
        if name in result:
            raise ValueError(f"duplicate effect name in {root}: {name}")
        result[name] = row
    if len(result) != 12:
        raise ValueError(f"expected 12 ASR effect objects under {root}; found {len(result)}")
    return result


def build(v161: Path, v353: Path) -> dict:
    a, b = scan(v161), scan(v353)
    if set(a) != set(b):
        raise ValueError("OS 1.61 and 3.53 effect name sets differ")
    effects = []
    for name in sorted(a):
        left, right = a[name], b[name]
        effects.append({
            "name": name,
            "os_1_61": {k: v for k, v in left.items()
                        if k not in {"fx_blocksize_field_hex", "fx_ptr_offset", "fx_ptr_more"}},
            "os_3_53": {k: v for k, v in right.items()
                        if k not in {"fx_blocksize_field_hex", "fx_ptr_offset", "fx_ptr_more"}},
            "payloads_byte_identical": left["payload_sha256"] == right["payload_sha256"],
        })
    return {
        "schema": "trollfx.asr10-factory-effect-manifest.v1",
        "classification": "static private firmware object metadata; not an executed program capture",
        "boundary_method": "candidate starts at header FxUcode; candidate extent ends at declared FxSize; complete 6-byte word count plus any remainder is reported",
        "instruction_field_decoder": "tools/es5510_decode.py, layout derived from pinned MAME ES5510 source; no file-format parser is provided by pinned MAME es5510d.cpp",
        "versions": ["ASR-10 OS 1.61", "ASR-10 OS 3.53"],
        "effects": effects,
        "limitations": [
            "FxUcode-to-FxSize is a candidate stored section, not proven exact upload boundaries.",
            "END indices are opcode-field observations; END does not by itself prove storage termination or frame scheduling.",
            "No firmware-generated host transaction capture for these 1.61/3.53 objects is available.",
            "GPR and special-register initialization, external delay-RAM state, and parameter-to-GPR mapping are unresolved.",
            "No factory object bytes or decoded instruction words are included in this output.",
        ],
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("os161_objects", type=Path)
    ap.add_argument("os353_objects", type=Path)
    ap.add_argument("--output", type=Path, help="write metadata JSON (default stdout)")
    args = ap.parse_args()
    payload = json.dumps(build(args.os161_objects, args.os353_objects), indent=2) + "\n"
    if args.output:
        args.output.write_text(payload)
    else:
        print(payload, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
