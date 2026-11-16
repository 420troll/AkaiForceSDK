#!/usr/bin/env python3
"""Generate the original TROLLfx stereo matrix panner ES5510 image."""
from pathlib import Path
import argparse

ROOT = Path(__file__).resolve().parents[4]


def encode(a=255, b=255, c=255, d=255, op=4, select=0, ram=6, accumulate=False):
    return ((d & 255) << 40) | ((c & 255) << 32) | ((b & 255) << 24) | \
           ((a & 255) << 16) | ((op & 15) << 12) | ((select & 15) << 8) | \
           ((int(accumulate) & 1) << 6) | ((ram & 7) << 3)


def assemble(source):
    words=[]
    for raw in source.read_text().splitlines():
        line=raw.split('#',1)[0].strip()
        if not line: continue
        f=line.split(); op=f[0].upper(); n=[int(x,0) for x in f[1:]]
        if op == 'MOV' and len(n)==6:
            a,b,_,_,select,ram=n
            words.append(encode(a,b,255,255,9,select,ram))
        elif op == 'MAC' and len(n)==4:
            c,d,acc,ram=n
            words.append(encode(255,255,c,d,4,0,ram,bool(acc)))
        elif op == 'NOP' and not n:
            words.append(encode())
        elif op == 'END' and not n:
            words.append(encode(255,255,255,255,15,0,0))
        else: raise ValueError(f'unsupported panner syntax: {raw}')
    if len(words)>160 or not words or ((words[-1]>>12)&15)!=15:
        raise ValueError('invalid or oversized ES5510 program')
    return words


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--output',type=Path)
    a=ap.parse_args(); source=Path(__file__).with_name('panner_stereo.asm')
    out=a.output or Path(__file__).with_name('panner_stereo_words.inc')
    words=assemble(source); out.write_text('\n'.join(f'0x{x:012x}ULL,' for x in words)+'\n')
    print(f'{len(words)} words -> {out}')

if __name__=='__main__': main()
