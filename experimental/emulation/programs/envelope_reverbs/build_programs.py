#!/usr/bin/env python3
"""Build gated/reverse plate-derived output-envelope ES5510 prototypes."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "room_hall"))
from build_programs import enc, mov, add, sub, mac, dil_mac, dol, nop, end, allpass, damping_filter

ROOT=Path(__file__).resolve().parent


def assemble(name, stages):
    w=[mov(10,239,ram=0),mov(11,238,ram=0),dil_mac(14,40),dil_mac(15,40),
       mov(16,10),mov(17,11),add(16,14),add(17,15)]
    for channel,signal in enumerate((16,17)):
        out=34+channel;state=90+channel
        w+=allpass(signal,out,state,50)
        if stages>1:
            out2=36+channel;state2=92+channel
            w+=allpass(out,out2,state2,51)
    for channel,source in enumerate((36 if stages>1 else 34,37 if stages>1 else 35)):
        w+=damping_filter(source,38+channel,94+channel,44)
    write_l,write_r=len(w),len(w)+1
    w += [dol(38),dol(39)]
    # Output delay return is scaled by independent wet and host envelope gains.
    # Dry path uses the same instruction engine and can be zeroed for 100% wet.
    w += [mov(60,14),mov(61,15),mac(60,41),mac(61,41),
          mac(60,43),mac(61,43),mov(62,10),mov(63,11),
          mac(62,42),mac(63,42),add(60,62),add(61,63),
          mov(241,60),mov(240,61),end()]
    if len(w)>160: raise ValueError(f"{name}: {len(w)} words exceeds ES5510 RAM")
    return w,write_l,write_r


def main():
    cases=(('gated_plate_envelope',1),('reverse_one_shot',2),('reverse_retrigger',2))
    meta='#pragma once\nnamespace trollfx::esp::programs::envelope_reverbs {\n'
    for name,stages in cases:
        words,wl,wr=assemble(name,stages)
        (ROOT/f'{name}_words.inc').write_text('\n'.join(f'0x{x:012x}ULL,' for x in words)+'\n')
        meta+=f'inline constexpr unsigned k{name.title().replace("_", "")}Words = {len(words)}, k{name.title().replace("_", "")}WriteL = {wl}, k{name.title().replace("_", "")}WriteR = {wr};\n'
        print(f'{name}: {len(words)} words; writes {wl}/{wr}; envelope GPR43')
    meta+='}\n'
    (ROOT/'program_metadata.hpp').write_text(meta)


if __name__=='__main__':main()
