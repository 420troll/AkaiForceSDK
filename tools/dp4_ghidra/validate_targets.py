#!/usr/bin/env python3
"""Validate proposed addresses against instruction starts in a private Ghidra report."""
import argparse
import re
import sys
from pathlib import Path


BOUNDARY = re.compile(r"^INSTRUCTION_BOUNDARY\s+([0-9a-fA-F]+)\s*$")


def validate(report: str, targets: list[int]) -> list[tuple[int, bool]]:
    boundaries = {
        int(match.group(1), 16)
        for line in report.splitlines()
        if (match := BOUNDARY.match(line))
    }
    if not boundaries:
        raise ValueError("report contains no INSTRUCTION_BOUNDARY records")
    return [(target, target in boundaries) for target in targets]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, help="private Dp4Report output")
    parser.add_argument("targets", nargs="+", type=lambda s: int(s, 0))
    args = parser.parse_args()
    try:
        results = validate(args.report.read_text(encoding="utf-8"), args.targets)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    for target, valid in results:
        print(f"0x{target:04X}: {'instruction-boundary' if valid else 'not-an-instruction-boundary'}")
    return 0 if all(valid for _, valid in results) else 1


if __name__ == "__main__":
    sys.exit(main())
