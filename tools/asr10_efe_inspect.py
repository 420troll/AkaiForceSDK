#!/usr/bin/env python3
"""Inspect ASR effect object headers and summarize candidate ESP sections.

This emits header fields and instruction statistics only; it never prints or
exports proprietary object bytes. The FxUcode boundary is treated as a
specification-backed candidate and must still be confirmed against a firmware
upload trace before claiming an executed factory program.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path

try:
    from .es5510_decode import ALU_NAMES, RAM_NAMES, SPECIAL_REGISTERS
except ImportError:
    from es5510_decode import ALU_NAMES, RAM_NAMES, SPECIAL_REGISTERS


def inspect(data: bytes) -> dict:
    if len(data) < 46:
        raise ValueError("effect object is shorter than the documented header")
    name_bytes = data[10:34]
    name = "".join(chr(c) for c in name_bytes[::2] if 32 <= c < 127).strip()
    fx_size = int.from_bytes(data[34:38], "big")
    ucode = int.from_bytes(data[38:40], "big")
    table = int.from_bytes(data[40:42], "big")
    init = int.from_bytes(data[42:44], "big")
    task = int.from_bytes(data[44:46], "big")
    if not 46 <= fx_size <= len(data):
        raise ValueError("FxSize is outside the object")
    if ucode and not 46 <= ucode < fx_size:
        raise ValueError("FxUcode pointer is outside the object")
    result = {"name": name, "version": int.from_bytes(data[8:10], "big"),
              "fx_blocksize_field_hex": data[:4].hex(),
              "fx_ptr_offset": int.from_bytes(data[4:6], "big"),
              "fx_ptr_more": int.from_bytes(data[6:8], "big"),
              "fx_size": fx_size, "fx_ucode": ucode, "fx_table": table,
              "fx_init_code": init, "fx_update_task": task,
              "container_padding_bytes": len(data) - fx_size}
    if not ucode:
        result["candidate_microcode"] = False
        return result

    words = (fx_size - ucode) // 6
    remainder = (fx_size - ucode) % 6
    alu: Counter[str] = Counter()
    ram: Counter[str] = Counter()
    regs: Counter[int] = Counter()
    for index in range(words):
        word = int.from_bytes(data[ucode + index * 6:ucode + (index + 1) * 6], "big")
        alu[(word >> 12) & 15] += 1
        ram[(word >> 3) & 7] += 1
        regs.update(((word >> 16) & 255, (word >> 24) & 255,
                     (word >> 32) & 255, (word >> 40) & 255))
    result.update(candidate_microcode=True, candidate_words=words,
                  section_remainder_bytes=remainder,
                  end_opcode_words=alu[15],
                  alu_counts={ALU_NAMES[k]: v for k, v in sorted(alu.items())},
                  ram_control_counts={RAM_NAMES[k]: v for k, v in sorted(ram.items())},
                  distinct_register_fields=len(regs),
                  special_register_field_uses=sum(v for r, v in regs.items() if r in SPECIAL_REGISTERS),
                  fits_es5510_160_word_program_limit=words <= 160)
    return result


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("files", nargs="+", type=Path, help="private extracted EFE payloads")
    args = ap.parse_args()
    rows = []
    for path in args.files:
        row = inspect(path.read_bytes())
        row["file"] = path.name
        rows.append(row)
    print(json.dumps({"schema": "trollfx.asr10-efe-inspection.v1", "files": rows}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
