#!/usr/bin/env python3
"""Validate an ASR raw floppy image and catalog its root directory.

The tool emits names, extents, sizes, and hashes only. With --extract-dir it
can copy OS/effect file data to a private directory outside this repository.
It does not parse or execute the contents of those files.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

BLOCK = 512
BLOCKS = 3200
FAT_START = 5
FAT_ENTRIES_PER_BLOCK = 170
REPO = Path(__file__).resolve().parents[1]
PRIVATE_ROOT = REPO / "research_local/asr10_firmware"


def catalog(image: bytes) -> list[dict]:
    if len(image) != BLOCK * BLOCKS:
        raise ValueError("ASR HD image must contain exactly 3200 512-byte blocks")
    if image[BLOCK + 38:BLOCK + 40] != b"ID":
        raise ValueError("ASR volume ID signature missing")
    if image[2 * BLOCK + 28:2 * BLOCK + 30] != b"OS":
        raise ValueError("ASR volume OS-block signature missing")
    if image[4 * BLOCK + 510:5 * BLOCK] != b"DR":
        raise ValueError("ASR root-directory signature missing")

    def fat(block: int) -> int:
        if not 0 <= block < BLOCKS:
            raise ValueError("FAT lookup block outside ASR volume")
        sector, slot = divmod(block, FAT_ENTRIES_PER_BLOCK)
        offset = (FAT_START + sector) * BLOCK + slot * 3
        return int.from_bytes(image[offset:offset + 3], "big")

    rows = []
    directory = image[3 * BLOCK:5 * BLOCK]
    for index in range(39):
        entry = directory[index * 26:(index + 1) * 26]
        kind = entry[1]
        if kind in (0, 2, 8):
            continue
        name = entry[2:14].decode("ascii", "replace").rstrip("\0 ")
        size = int.from_bytes(entry[14:16], "big")
        contiguous = int.from_bytes(entry[16:18], "big")
        start = int.from_bytes(entry[18:22], "big")
        if (not name or not size or size > BLOCKS or not contiguous or
                contiguous > size or start >= BLOCKS or start + contiguous > BLOCKS):
            raise ValueError(f"invalid root directory entry {index}")
        chain = list(range(start, start + contiguous))
        if chain[-1] >= BLOCKS:
            raise ValueError(f"entry {index} contiguous extent exceeds volume")
        next_block = fat(chain[-1])
        while len(chain) < size:
            if next_block in (0, 1) or next_block >= BLOCKS or next_block in chain:
                raise ValueError(f"invalid/cyclic FAT chain for entry {index}")
            chain.append(next_block)
            next_block = fat(next_block)
        if next_block != 1:
            raise ValueError(f"FAT chain for entry {index} does not end at EOF")
        payload = b"".join(image[n * BLOCK:(n + 1) * BLOCK] for n in chain)
        rows.append({"index": index, "type": kind, "name": name,
                     "blocks": size, "contiguous_blocks": contiguous,
                     "start_block": start, "sha256": hashlib.sha256(payload).hexdigest(),
                     "_payload": payload})
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("image", type=Path, help="private raw ASR IMG")
    ap.add_argument("--extract-dir", type=Path,
                    help="optional private destination for ASR-OS/ASR-EFF payloads")
    args = ap.parse_args()
    rows = catalog(args.image.read_bytes())
    if args.extract_dir:
        dest = args.extract_dir.resolve()
        if dest == REPO or (REPO in dest.parents and PRIVATE_ROOT not in dest.parents and dest != PRIVATE_ROOT):
            ap.error("payloads inside the repository may be written only under ignored research_local/asr10_firmware")
        dest.mkdir(parents=True, exist_ok=True)
        for row in rows:
            if row["type"] in (32, 33):
                path = dest / f"{row['index']:02d}_{row['name']}.payload.bin"
                path.write_bytes(row["_payload"])
                row["extracted_path"] = str(path)
    for row in rows:
        row.pop("_payload")
    print(json.dumps({"format": "ASR-10 raw floppy image", "image_bytes": BLOCK * BLOCKS,
                      "entries": rows}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
