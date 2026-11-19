#!/usr/bin/env python3
"""Small assembler for original TROLLfx ES5510 Phase 2 test programs.

Instruction source syntax is documented in the .asm files. This intentionally
supports only the MOV/ADD/MAC+ALU forms needed by the checked-in programs.
"""
import argparse
from pathlib import Path

SEL_REG = 0
SEL_ALU_TO_DOL = 5
SEL_DIL_MAC_TO_GPR = 12

def encode(a=0,b=0,c=0,d=0,op=4,select=0,ram=6):
    return ((d&255)<<40)|((c&255)<<32)|((b&255)<<24)|((a&255)<<16)|((op&15)<<12)|((select&15)<<8)|((ram&7)<<3)

def parse(src):
    out=[]
    for raw in Path(src).read_text().splitlines():
        line=raw.split('#',1)[0].strip()
        if not line: continue
        fields=line.replace(',', ' ').split()
        op=fields[0].upper()
        n=[int(x,0) for x in fields[1:]]
        if op=='MOV' and len(n)==6:
            a,b,c,d,sel,ram=n
            # Every instruction runs the MAC in parallel. C/D=255 makes its
            # destination a non-writable constant register for non-MAC ops.
            word=encode(a,b,255,255,9,sel,ram)
        elif op=='ADD' and len(n)==6:
            a,b,c,d,sel,ram=n
            # Source A is the in-place destination; source B is the addend.
            word=encode(a,b,255,255,0,sel,ram)
        elif op=='MACDIL_MOV' and len(n)==5:
            a,c,d,sel,ram=n
            if sel != SEL_DIL_MAC_TO_GPR:
                raise ValueError('MACDIL_MOV must select 12 (ALU and MAC read DIL)')
            word=encode(a,0,c,d,9,sel,ram)
        elif op=='MOV_DOL' and len(n)==6:
            a,b,c,d,sel,ram=n
            if sel != SEL_ALU_TO_DOL:
                raise ValueError('MOV_DOL must select 5 (ALU destination DOL)')
            word=encode(a,b,255,255,9,sel,ram)
        elif op=='END' and not n:
            word=encode(255,255,255,255,15,0,0)
        elif op=='NOP' and not n:
            word=encode(c=255,d=255,op=4,ram=6)
        else:
            raise ValueError(f'unsupported instruction syntax: {raw}')
        out.append(word)
    if not out or ((out[-1]>>12)&15) != 15: raise ValueError('program must end with END')
    if len(out)>160: raise ValueError('program exceeds ES5510 160-word instruction RAM')
    return out

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('kind',choices=['delay','chorus'])
    ap.add_argument('output')
    ap.add_argument('--sample-rate',type=int,default=44100)
    ap.add_argument('--delay-ms',type=float,default=250.0)
    ap.add_argument('--max-delay-ms',type=float,default=10000.0)
    ap.add_argument('--feedback',type=float,default=0.55)
    ap.add_argument('--wet',type=float,default=0.5)
    ap.add_argument('--depth-ms',type=float,default=5.0)
    ap.add_argument('--rate-hz',type=float,default=0.5)
    a=ap.parse_args()
    if not 8000<=a.sample_rate<=192000: ap.error('sample-rate out of range')
    delay=max(1,round(a.delay_ms*a.sample_rate/1000))
    capacity=max(delay+2,round(a.max_delay_ms*a.sample_rate/1000))
    if a.kind=='delay' and not 0.0<=a.feedback<=0.94: ap.error('feedback must be in [0, 0.94]')
    if a.kind=='chorus' and not 0.0<=a.wet<=1.0: ap.error('wet must be in [0, 1]')
    depth=max(0,round(a.depth_ms*a.sample_rate/1000))
    if a.kind=='chorus' and (depth>=delay or a.rate_hz<0): ap.error('chorus depth must be less than base delay and rate must be nonnegative')
    # One ring of 2*capacity words holds two non-overlapping stereo banks.
    ring_samples=capacity*2
    if ring_samples > (1<<20): ap.error('stereo ring exceeds the emulator 1M-word DRAM')
    dlength=(ring_samples-1)
    bank=capacity
    read_l=delay
    read_r=(bank+delay)
    coef=round((a.feedback if a.kind=='delay' else a.wet)*(1<<22))
    source=Path(__file__).resolve().parents[1]/'experimental/emulation/programs'/('stereo_feedback_delay.asm' if a.kind=='delay' else 'stereo_chorus.asm')
    words=parse(source)
    lines=[
        '# Original TROLLfx ES5510 microprogram. Not Ensoniq firmware or a factory effect.',
        f'# Assembled from {source.name}; delay={delay} samples, max-bank={capacity}, coefficient={coef}.',
        f'# MEMSIZ=0 (1 offset unit/DRAM word), DLength={dlength} (ring={ring_samples} samples).',
        'ES5510_BUNDLE 1',f'sample_rate {a.sample_rate}','host_serial 0x20',
        'gpr 244 0',f'gpr 245 0x{dlength:x}','gpr 248 0',
        f'gpr 0 0x{read_l:x}',f'gpr 1 0x{read_r:x}',
        f'gpr {6 if a.kind=="delay" else 4} 0',
        f'gpr {7 if a.kind=="delay" else 5} 0x{bank:x}',f'gpr 40 0x{coef:x}',
    ]
    if a.kind=='chorus':
        lines.extend((
            f'lfo_gpr 0 {delay} {depth} {a.rate_hz} 0 {delay-depth} {delay+depth}',
            f'lfo_gpr 1 {bank+delay} {depth} {a.rate_hz} 0.5 {bank+delay-depth} {bank+delay+depth}',
        ))
    lines.extend(f'instruction 0x{word:012x}' for word in words)
    Path(a.output).write_text('\n'.join(lines)+'\n')
    print(f'assembled {len(words)} words; delay={delay} frames, bank={capacity}, ring={ring_samples}, coefficient=0x{coef:x}' + (f', LFO={a.rate_hz}Hz depth={depth} frames' if a.kind=='chorus' else ''))

if __name__=='__main__': main()
