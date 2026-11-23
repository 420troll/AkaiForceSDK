#!/usr/bin/env python3
"""Extract only instruction words from a private ES5510C capture.

Runtime GPR, special-register and delay-RAM sections are deliberately ignored:
captures taken after execution are not valid pristine initialization state.
"""
import argparse
import struct
from pathlib import Path

MAGIC = b"ES5510C\0"
HEADER = 44
MAX_WORDS = 160

def extract(src: Path, dst: Path, sample_rate: int, host_serial: int) -> int:
    data = src.read_bytes()
    if len(data) < HEADER or data[:8] != MAGIC:
        raise ValueError("not an ES5510C capture")
    version, header, ni, ng, ns, nd, flags, nstate, payload = struct.unpack_from("<9I", data, 8)
    expected = ni * 6 + ng * 3 + ns * 3 + nstate + nd * 2
    if (version, header, ni, ng, ns, nd, flags, nstate) != (1, HEADER, 160, 192, 22, 1 << 20, 1, 74):
        raise ValueError("unsupported capture dimensions")
    if payload != expected or len(data) != HEADER + payload:
        raise ValueError("truncated or inconsistent capture")
    if not 8000 <= sample_rate <= 192000 or not 0 <= host_serial <= 255:
        raise ValueError("invalid sample rate or host serial")
    p = HEADER
    words = [int.from_bytes(data[p+i:p+i+6], "big") for i in range(0, ni*6, 6)]
    end = next((i for i, word in enumerate(words) if (word >> 12) & 15 == 15), None)
    if end is None:
        raise ValueError("program has no END")
    lines = ["# Program words only. Add verified pristine initialization records separately.",
             "# Capture GPR/SPR/DRAM snapshots were intentionally not imported.",
             "ES5510_BUNDLE 1", f"sample_rate {sample_rate}", f"host_serial 0x{host_serial:02x}"]
    lines.extend(f"instruction_at {i} 0x{words[i]:012x}" for i in range(end+1))
    dst.write_text("\n".join(lines) + "\n")
    return end + 1

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("capture", type=Path)
    p.add_argument("bundle", type=Path)
    p.add_argument("--sample-rate", type=int, default=44100)
    p.add_argument("--host-serial", type=lambda x: int(x, 0), required=True,
                   help="verified host serial routing value; never inferred from capture runtime state")
    a = p.parse_args()
    try:
        count = extract(a.capture, a.bundle, a.sample_rate, a.host_serial)
    except (OSError, ValueError) as e:
        p.error(str(e))
    print(f"extracted {count} instruction words through END; snapshots omitted")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
