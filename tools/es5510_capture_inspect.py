#!/usr/bin/env python3
"""Validate a local 48-bit ES5510 trace or neutral instruction-word list.

This tool does not load firmware or execute a program. Use --json-output only
for a capture that you are legally entitled to keep; output may contain the
captured program words.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any

try:
    from .es5510_decode import decode_word
except ImportError:  # Script invocation: python tools/es5510_capture_inspect.py
    from es5510_decode import decode_word


MAX_WORDS = 160
MAX_INPUT_BYTES = 64 * 1024
TRACE_RE = re.compile(
    r"^es5510_program\[(?P<index>[0-9]{3})\]="
    r"(?P<word>[0-9a-fA-F]{12})\s+gpr=(?P<gpr>[0-9a-fA-F]{6})$"
)
WORD_RE = re.compile(r"^(?:(?P<index>[0-9]{1,3})\s*[:=]\s*)?(?:0[xX])?(?P<word>[0-9a-fA-F]{1,12})$")


class CaptureError(ValueError):
    pass


def inspect_capture(path: Path) -> dict[str, Any]:
    size = path.stat().st_size
    if size > MAX_INPUT_BYTES:
        raise CaptureError(f"input is {size} bytes; limit is {MAX_INPUT_BYTES}")

    data = path.read_bytes()
    try:
        source = data.decode("ascii")
    except UnicodeDecodeError as exc:
        raise CaptureError("input must be ASCII text; binary/program images are not accepted") from exc

    lines = source.splitlines()
    populated = [(line_no, line.strip()) for line_no, line in enumerate(lines, 1)
                 if line.strip() and not line.lstrip().startswith("#")]
    if not populated:
        raise CaptureError("no instruction records found")
    if len(populated) > MAX_WORDS:
        raise CaptureError(f"capture contains {len(populated)} records; ES5510 program memory holds at most {MAX_WORDS}")

    trace_matches = [TRACE_RE.fullmatch(line) for _, line in populated]
    is_eps16_trace = all(match is not None for match in trace_matches)
    words: list[dict[str, Any]] = []

    if is_eps16_trace:
        for expected_index, ((line_no, _), match) in enumerate(zip(populated, trace_matches)):
            assert match is not None
            index = int(match.group("index"))
            if index != expected_index:
                raise CaptureError(f"line {line_no}: trace index {index} is not contiguous; expected {expected_index}")
            word = int(match.group("word"), 16)
            entry = {
                "index": index,
                "instruction_hex": f"{word:012x}",
                # rom_probe prints es5510_gpr[index], which is not a full GPR
                # initialization image and must not be used as one.
                "trace_gpr_at_same_index_hex": match.group("gpr").lower(),
            }
            entry["decoded_mame_fields"] = decode_word(word)
            words.append(entry)
        source_format = "eps16-trace-es5510-program-lines"
    else:
        for record_index, (line_no, line) in enumerate(populated):
            body = line.split("#", 1)[0].strip()
            if not body:
                continue
            match = WORD_RE.fullmatch(body)
            if not match:
                raise CaptureError(
                    f"line {line_no}: expected up to twelve hex digits, optionally prefixed by an index or 0x"
                )
            supplied_index = match.group("index")
            if supplied_index is not None and int(supplied_index) != record_index:
                raise CaptureError(
                    f"line {line_no}: word index {supplied_index} is not contiguous; expected {record_index}"
                )
            word = int(match.group("word"), 16)
            if word >= (1 << 48):
                raise CaptureError(f"line {line_no}: instruction is wider than 48 bits")
            words.append({"index": record_index, "instruction_hex": f"{word:012x}",
                          "decoded_mame_fields": decode_word(word)})
        source_format = "neutral-hex-48-bit-word-list"

    if not words:
        raise CaptureError("no valid instructions found")

    end_indices = [entry["index"] for entry in words
                   if ((int(entry["instruction_hex"], 16) >> 12) & 0xF) == 0xF]
    return {
        "schema": "trollfx.es5510-capture-inspection.v1",
        "source_format": source_format,
        "input_sha256": hashlib.sha256(data).hexdigest(),
        "instruction_width_bits": 48,
        "instruction_count": len(words),
        "program_memory_capacity": MAX_WORDS,
        "end_opcode_indices_candidate": end_indices,
        "instructions": words,
        "limitations": [
            "This is a word-list inspector, not an EPS/Waveboy binary importer or program loader.",
            "The EPS trace's gpr field is the emulator's GPR at the same numeric index; it is not a complete GPR initialization snapshot.",
            "External DRAM, serial routing, host-serial mode, sample rate, and live parameter state are not represented by this trace format.",
            "An END opcode candidate is reported from the known MAME model's ALU-op field; it is not proof that a capture is executable or authentic.",
        ],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path, help="local ASCII emulator trace or neutral hex word list")
    parser.add_argument("--json-output", type=Path, help="write inspection JSON (may contain captured program data)")
    args = parser.parse_args(argv)
    try:
        result = inspect_capture(args.capture)
    except (OSError, CaptureError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    print(f"format: {result['source_format']}")
    print(f"instructions: {result['instruction_count']}/{MAX_WORDS}")
    print(f"input sha256: {result['input_sha256']}")
    print(f"END-opcode candidates: {result['end_opcode_indices_candidate']}")
    print("No GPR/DRAM/routing initialization or executable bundle is produced.")
    if args.json_output:
        args.json_output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print(f"inspection JSON: {args.json_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
