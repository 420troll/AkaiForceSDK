#!/usr/bin/env python3
"""Verify the U56-to-U54 revision-tag relationship without publishing ROM data.

The utility only prints offsets, the revision tag, and hashes. Firmware images
must be supplied from a private working directory and are never copied.
"""

from __future__ import annotations

import argparse
import hashlib
from dataclasses import dataclass
from pathlib import Path

try:
    from tools.dp4_ghidra.dp4_bus_map import u54_cpu_file_offset
except ModuleNotFoundError:  # Direct invocation as `python path/to/script.py`.
    from dp4_bus_map import u54_cpu_file_offset

U56_SIZE = 0x8000
U54_SIZE = 0x20000
U56_CPU_BASE = 0x8000
TAG_READ_ADDRESS = 0x2000
TAG_BANK = 9


@dataclass(frozen=True)
class TagMatch:
    u56_file_offset: int
    u56_cpu_address: int
    u54_file_offset: int
    tag: int


def find_tag_check(u56: bytes) -> tuple[int, int]:
    """Find unique MC6801 `LDX $2000; CPX #imm16; BEQ rel` check.

    The instruction opcodes/addressing forms are from Motorola's public
    MC6801/03 ISA. No instruction bytes or disassembly are returned.
    """
    if len(u56) != U56_SIZE:
        raise ValueError(f"U56 must be exactly {U56_SIZE} bytes")

    hits: list[tuple[int, int]] = []
    # LDX extended = FE hi lo; CPX immediate = 8C hi lo; BEQ = 27 rel8.
    for offset in range(len(u56) - 8 + 1):
        if (u56[offset] == 0xFE and
                u56[offset + 1:offset + 3] == TAG_READ_ADDRESS.to_bytes(2, "big") and
                u56[offset + 3] == 0x8C and u56[offset + 6] == 0x27):
            tag = int.from_bytes(u56[offset + 4:offset + 6], "big")
            hits.append((offset, tag))

    if len(hits) != 1:
        raise ValueError(f"expected one U56 revision-tag check; found {len(hits)}")
    return hits[0]


def verify_tag_pair(u56: bytes, u54: bytes, bank: int = TAG_BANK) -> TagMatch:
    if len(u54) != U54_SIZE:
        raise ValueError(f"U54 must be exactly {U54_SIZE} bytes")
    offset, expected = find_tag_check(u56)
    u54_offset = u54_cpu_file_offset(bank, TAG_READ_ADDRESS)
    observed = int.from_bytes(u54[u54_offset:u54_offset + 2], "big")
    if observed != expected:
        raise ValueError(
            f"U56 expects UCODE tag 0x{expected:04x}, but bank {bank} "
            f"at U54 file offset 0x{u54_offset:05x} contains 0x{observed:04x}"
        )
    return TagMatch(offset, U56_CPU_BASE + offset, u54_offset, expected)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("u56", type=Path, help="private U56 operating-system image")
    parser.add_argument("u54", type=Path, help="private U54 UCODE image")
    args = parser.parse_args()
    u56 = args.u56.read_bytes()
    u54 = args.u54.read_bytes()
    match = verify_tag_pair(u56, u54)
    print(f"U56_SHA256 {hashlib.sha256(u56).hexdigest()}")
    print(f"U54_SHA256 {hashlib.sha256(u54).hexdigest()}")
    print(f"U56_TAG_CHECK file=0x{match.u56_file_offset:04x} cpu=0x{match.u56_cpu_address:04x}")
    print(f"U54_TAG bank={TAG_BANK} file=0x{match.u54_file_offset:05x} value=0x{match.tag:04x}")
    print("MATCH verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
