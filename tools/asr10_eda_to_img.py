#!/usr/bin/env python3
"""Convert an ASR Giebler EDA disk image to raw 1.6 MB sector bytes.

This only expands the documented 512-byte block skip map. It does not parse
ASR filesystem structures or execute any content. Use private paths for both
input and output; recovered firmware must remain outside Git.
"""
from __future__ import annotations

import argparse
from pathlib import Path

BLOCK_SIZE = 512
BLOCK_COUNT = 3200
SKIP_TABLE_OFFSET = 0x60
SKIP_TABLE_SIZE = 400
HEADER = b"\r\nASR-10 Disk"
SKIPPED_BLOCK = (b"\x6d\xb6" * (BLOCK_SIZE // 2))
REPO = Path(__file__).resolve().parents[1]
PRIVATE_ROOT = REPO / "research_local/asr10_firmware"


def expand_eda(data: bytes) -> tuple[bytes, int]:
    if len(data) < BLOCK_SIZE or not data.startswith(HEADER):
        raise ValueError("missing ASR EDA signature or incomplete 512-byte header")
    table = data[SKIP_TABLE_OFFSET:SKIP_TABLE_OFFSET + SKIP_TABLE_SIZE]
    if len(table) != SKIP_TABLE_SIZE:
        raise ValueError("incomplete ASR skip table")

    out = bytearray()
    cursor = BLOCK_SIZE
    used = 0
    for block in range(BLOCK_COUNT):
        flags = table[block // 8]
        skipped = bool(flags & (0x80 >> (block % 8)))
        if skipped:
            out.extend(SKIPPED_BLOCK)
        else:
            end = cursor + BLOCK_SIZE
            if end > len(data):
                raise ValueError(f"truncated EDA data at source block {block}")
            out.extend(data[cursor:end])
            cursor = end
            used += 1

    # The supplied Ensoniq installers' files have one trailing NUL after the
    # compressed block payload. Accept exactly that, but no unparsed payload.
    tail = data[cursor:]
    if tail not in (b"", b"\0"):
        raise ValueError(f"unexpected {len(tail)} trailing EDA byte(s)")
    if cursor != BLOCK_SIZE + used * BLOCK_SIZE:
        raise AssertionError("internal block accounting error")
    return bytes(out), used


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="private .EDA file")
    parser.add_argument("output", type=Path, help="private raw .IMG destination")
    args = parser.parse_args()
    image, used = expand_eda(args.input.read_bytes())
    if len(image) != BLOCK_SIZE * BLOCK_COUNT:
        raise AssertionError("wrong reconstructed ASR image size")
    output = args.output.resolve()
    if output == REPO or (REPO in output.parents and PRIVATE_ROOT not in output.parents and output != PRIVATE_ROOT):
        parser.error("raw proprietary images may be written only outside the repository or under ignored research_local/asr10_firmware")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(image)
    print(f"expanded {used} stored blocks to {len(image)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
