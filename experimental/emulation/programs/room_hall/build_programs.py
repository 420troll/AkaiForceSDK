#!/usr/bin/env python3
"""Assemble original stereo Room/Hall ES5510 feedback-diffusion prototypes."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def enc(a=255, b=255, c=255, d=255, op=4, sel=0, ram=6):
    return ((d & 255) << 40) | ((c & 255) << 32) | ((b & 255) << 24) | \
           ((a & 255) << 16) | ((op & 15) << 12) | ((sel & 15) << 8) | ((ram & 7) << 3)


def mov(dst, src, ram=6, sel=0): return enc(dst, src, op=9, ram=ram, sel=sel)
def add(dst, src): return enc(dst, src, op=0)
def sub(dst, src): return enc(dst, src, op=1)
def mac(dst, coef): return enc(c=dst, d=coef, op=4)
def dil_mac(dst, coef): return enc(a=255, b=255, c=dst, d=coef, op=4, sel=12)
def dol(src): return enc(a=255, b=src, op=9, sel=5, ram=1)
def nop(): return enc(op=4)
def end(): return enc(op=15, ram=0)


def allpass(signal, out, state, coef):
    # Difference, one execution slot for ALU retirement, coefficient MAC,
    # summing restoration, then save the old input as section memory.
    return [mov(out, signal), sub(out, state), nop(), mac(out, coef),
            add(out, state), mov(state, signal)]


def assemble(kind):
    hall = kind == "hall"
    w = [
        mov(10, 239, ram=0),  # read left decay bank
        mov(11, 238, ram=0),  # read right decay bank
        dil_mac(14, 40),     # delayed left * feedback
        dil_mac(15, 40),
        mov(12, 239, ram=0), # early left read, independently addressed
        mov(13, 238, ram=0), # early right read
        dil_mac(22, 41),
        dil_mac(23, 41),
        mov(16, 10),         # input L
        mov(17, 11),         # input R
    ]
    w += [add(16, 15 if hall else 14), add(17, 14 if hall else 15)]
    # Room is a compact single allpass per side. Hall cascades two sections,
    # adds a second, slower stage and uses cross-coupled stereo feedback.
    w += allpass(16, 34, 90, 50)
    w += allpass(17, 35, 91, 50)
    if hall:
        w += allpass(34, 36, 92, 51)
        w += allpass(35, 37, 93, 51)
    write_l, write_r = len(w), len(w) + 1
    w += [dol(36 if hall else 34), dol(37 if hall else 35)]
    # Tap mix: long decay return plus independently tunable early return.
    w += [mov(60, 14), mov(61, 15), mac(60, 42), mac(61, 42),
          mov(62, 22), mov(63, 23), mac(62, 41), mac(63, 41),
          add(60, 62), add(61, 63), mov(62, 10), mov(63, 11),
          mac(62, 43), mac(63, 43), add(60, 62), add(61, 63),
          mov(241, 60), mov(240, 61), end()]
    if hall:
        # Keep the existing stereo bank but use a substantially longer tap.
        pass
    if len(w) > 160:
        raise ValueError(f"{kind} exceeds ES5510 program memory: {len(w)}")
    return w, write_l, write_r


def main():
    records = []
    for kind in ("room", "hall"):
        words, wl, wr = assemble(kind)
        (ROOT / f"{kind}_feedback_diffusion_words.inc").write_text(
            "\n".join(f"0x{x:012x}ULL," for x in words) + "\n")
        records.append((kind, len(words), wl, wr))
        print(f"{kind}: {len(words)} words; DOL PCs L/R={wl}/{wr}")
    metadata = "#pragma once\nnamespace trollfx::esp::programs::room_hall {\n"
    metadata += "inline constexpr unsigned kRoomWords = %d, kRoomWriteL = %d, kRoomWriteR = %d;\n" % records[0][1:]
    metadata += "inline constexpr unsigned kHallWords = %d, kHallWriteL = %d, kHallWriteR = %d;\n" % records[1][1:]
    metadata += "}\n"
    (ROOT / "program_metadata.hpp").write_text(metadata)


if __name__ == "__main__":
    main()
