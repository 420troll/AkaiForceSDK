#!/usr/bin/env python3
"""Generate the original 40-word dual-band ES5510 rotary proof program."""
from pathlib import Path


def enc(a=255, b=255, c=255, d=255, op=4, sel=0, ram=6):
    return ((d & 255) << 40) | ((c & 255) << 32) | ((b & 255) << 24) | \
           ((a & 255) << 16) | ((op & 15) << 12) | ((sel & 15) << 8) | ((ram & 7) << 3)


def mov(dst, src): return enc(dst, src, op=9)
def alu(dst, src, op): return enc(dst, src, op=op)
def mac(dst, coefficient): return enc(255, 255, dst, coefficient, op=4)
def nop(): return enc(255, 255, 255, 255, op=4)

def mov_dol(src): return enc(0, src, op=9, sel=5, ram=1)
def mov_dil(dst): return enc(dst, 0, op=9, sel=9, ram=6)
def read_dbase(): return enc(255, 255, 255, 255, op=4, sel=0, ram=0)


def make_program():
    words = [
        mov(32, 239),              # mono downmix starts with SER2L
        alu(32, 238, 0),           # saturated L+R sum
        alu(33, 32, 14),           # arithmetic divide by two
        nop(),
        mov(34, 33),               # diff = input - LP1
        alu(34, 60, 3),            # diff -= LP1
        nop(),
        mac(34, 54),               # diff *= crossover coefficient
        alu(60, 34, 2),            # LP1 += diff
        nop(),
        mov(34, 60),               # diff = LP1 - LP2
        alu(34, 61, 3),
        nop(),
        mac(34, 54),
        alu(61, 34, 2),            # LP2 += diff; low/drum band
        nop(),
        mov(35, 33),               # high/horn = input - LP2
        alu(35, 61, 3),
        nop(),
        mov_dol(35),               # store high band at DBASE + GPR[19]
        mov_dol(61),               # store low band at DBASE + GPR[20]
        read_dbase(),              # dynamic high read offset at GPR[21]
        nop(),
        mov_dil(36),               # high-band delayed sample from DIL
        read_dbase(),              # dynamic low read offset at GPR[24]
        nop(),
        mov_dil(37),               # low-band delayed sample from DIL
        mov(100, 36),              # preserve independent copies for both outputs
        mov(101, 37),
        mov(102, 36),
        mov(103, 37),
        mac(100, 50),              # high/horn left
        mac(101, 52),              # low/drum left
        alu(100, 101, 2),          # left sum
        mac(102, 51),              # high/horn right
        mac(103, 53),              # low/drum right
        alu(102, 103, 2),          # right sum
        mov(241, 100),
        mov(240, 102),
        enc(255, 255, 255, 255, op=15, sel=0, ram=0),
    ]
    if len(words) > 160: raise ValueError(f"program exceeds 160 words: {len(words)}")
    return words


def main():
    path=Path(__file__).resolve().parent/"rotary_2band_words.inc"
    words=make_program()
    path.write_text("\n".join(f"0x{w:012x}ULL," for w in words)+"\n")
    print(f"generated {len(words)} words -> {path}")

if __name__ == "__main__": main()
