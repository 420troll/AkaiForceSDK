#!/usr/bin/env python3
"""Assemble original one-pass ES5510 nonlinear-style density networks."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent

def enc(a=255, b=255, c=255, d=255, op=4, sel=0, ram=6):
    return ((d & 255) << 40) | ((c & 255) << 32) | ((b & 255) << 24) | \
           ((a & 255) << 16) | ((op & 15) << 12) | ((sel & 15) << 8) | ((ram & 7) << 3)
def mov(dst, src): return enc(dst, src, op=9)
def add(dst, src): return enc(dst, src, op=0)
def nop(ram=6): return enc(ram=ram)
def mac(dst, coef): return enc(c=dst, d=coef, op=4)
def dil_mac(dst, coef): return enc(c=dst, d=coef, op=4, sel=12)
def dol(src): return enc(b=src, op=9, sel=5, ram=1)
def end(): return enc(op=15, ram=0)

def allpass(signal, out, state, coef):
    # One first-order diffuser. ALU and MAC state updates run in the ESP.
    return [mov(out, signal), enc(out, state, op=1), nop(), mac(out, coef),
            enc(out, state, op=0), mov(state, signal)]

def assemble(name, stages, taps, levels, coeffs, centered=False):
    w = [mov(12, 239), mov(13, 238)]  # save unprocessed stereo input for Dry
    # Optional mono-centering before the channel diffuser paths.
    if centered:
        w += [mov(14, 12), add(14, 13), enc(14, 14, op=14), mov(12, 14), mov(13, 14)]
    signals = []
    coeff_regs = []
    for ch, source in enumerate((12, 13)):
        sig = source
        for i, coeff in enumerate(stages):
            state = 90 + ch * 8 + i
            out = 30 + ch * 8 + i
            reg = 128 + ch * 8 + i
            w += allpass(sig, out, state, reg)
            coeff_regs.append((reg, coeff))
            sig = out
        signals.append(sig)
    # Store one diffused input sample per channel into a shared-frame stereo ring.
    write_l = len(w)
    w += [dol(signals[0])]
    write_r = len(w)
    w += [dol(signals[1])]

    products_l, products_r = [], []
    read_pcs=[]
    for i, (left_tap, right_tap) in enumerate(taps):
        read_pcs.append((len(w), len(w)+1))
        # The two RAM-read requests feed the following paired DIL MACs through
        # the ES5510 RAM pipeline, just as in the verified Room/Hall fixtures.
        w += [nop(ram=0), nop(ram=0)]
        # Keep MAC result registers out of the low GPRs also used as per-PC
        # delay offsets, and clear of allpass/level/control register banks.
        pl, pr = 104+i, 113+i
        w += [dil_mac(pl, 150+i), dil_mac(pr, 150+i)]
        products_l.append(pl); products_r.append(pr)

    w += [mov(60, products_l[0]), mov(61, products_r[0])]
    for pl, pr in zip(products_l[1:], products_r[1:]):
        w += [add(60, pl), add(61, pr)]
    # Wet return and independent direct path are both ES5510 MAC/ALU work.
    w += [mov(62, 12), mov(63, 13), mac(60, 180), mac(61, 180), mac(62, 181), mac(63, 181),
          add(60, 62), add(61, 63), mov(241, 60), mov(240, 61), end()]
    if len(w) > 160:
        raise ValueError(f"{name}: {len(w)} words exceeds the ES5510 160-word RAM")
    return w, write_l, write_r, read_pcs, coeff_regs

def main():
    # Delay layouts use coprime, increasing offsets. NonLin3 is deliberately
    # centered and short; NonLin2 carries a longer, four-stage bloom.
    specs = [
        ("nonlin1_transient", [0.46], [(173,211),(293,347),(467,523),(719,797),(1031,1151),(1453,1597),(2027,2237),(11519,12101),(21101,22051)], [76,66,58,52,47,42,37,27,16], [0.46]),
        ("nonlin2_bloom", [0.62,-0.58,0.66,-0.61], [(4999,7001),(9001,12007),(16001,19001),(23003,26003),(30011,34019),(38011,42013),(46021,50023),(54001,58013),(62003,66013)], [70,62,58,53,48,43,39,31,21], [0.62,-0.58,0.66,-0.61]),
        ("nonlin3_drum", [0.42], [(97,97),(163,163),(263,263),(397,397),(557,557),(751,751),(977,977),(1237,1237),(1531,1531)], [80,66,54,45,38,31,25,19,12], [0.42], True),
    ]
    records=[]
    for spec in specs:
        name, stages, taps, levels, coeffs, *rest = spec
        centered = bool(rest and rest[0])
        w, wl, wr, pcs, coeff_regs = assemble(name, stages, taps, levels, coeffs, centered)
        (ROOT/f"{name}_words.inc").write_text("\n".join(f"0x{x:012x}ULL," for x in w)+"\n")
        (ROOT/f"{name}.asm").write_text(
            f"# Original TROLLfx one-pass density program: {name}; not Ensoniq microcode.\n"
            f"# {len(w)} words; {len(stages)} serial input diffuser stage(s)/channel; nine weighted read taps.\n"
            f"# Store PCs {wl}/{wr}; tap read PCs {pcs}; tap levels GPR 150..158; diffuser GPR 128+; wet/dry GPR 180/181.\n"
            f"# Offsets at 44.1kHz: {taps}; all nine envelope gains are host control values,\n"
            f"# multiplied with delayed audio in the ES5510 MAC path. Mono-center={centered}.\n")
        records.append((name,len(w),wl,wr,pcs,coeff_regs,taps,levels))
        print(f"{name}: {len(w)} words; store PCs {wl}/{wr}; tap read PCs {pcs}")
    hdr="#pragma once\n#include <array>\n#include <cstddef>\n#include <cstdint>\n#include <utility>\nnamespace trollfx::esp::programs::nonlin_tap {\n"
    hdr+="enum class Id : std::uint8_t { NonLin1, NonLin2, NonLin3 };\n"
    hdr+="struct Definition { Id id; const char* name; const std::uint64_t* words; std::size_t wordCount; unsigned writeLeftPc, writeRightPc; std::array<unsigned,9> readLeftPc, readRightPc; std::array<std::uint32_t,9> tapLeft, tapRight; std::array<float,9> defaultLevels; std::array<std::pair<unsigned,float>,8> coefficients; std::size_t coefficientCount; };\n"
    for name,size,*_ in records:
        var=name.upper()
        hdr+=f"inline constexpr std::array<std::uint64_t,{size}> k{var} = {{\n#include \"{name}_words.inc\"\n}};\n"
    hdr+="inline constexpr std::array<Definition,3> kCatalog = {{\n"
    for idx,(name,size,wl,wr,pcs,coeff_regs,taps,levels) in enumerate(records):
        leftpcs=','.join(str(x[0]) for x in pcs); rightpcs=','.join(str(x[1]) for x in pcs)
        lefttap=','.join(str(x[0]) for x in taps); righttap=','.join(str(x[1]) for x in taps)
        levelstr=','.join(str(x/100.0) for x in levels)
        coeffstr=','.join('{%d,%s}'%(r,v) for r,v in coeff_regs)
        hdr+=f"{{Id::NonLin{idx+1},\"{name}\",k{name.upper()}.data(),k{name.upper()}.size(),{wl},{wr},{{{leftpcs}}},{{{rightpcs}}},{{{lefttap}}},{{{righttap}}},{{{levelstr}}},{{{{{coeffstr}}}}},{len(coeff_regs)}}},\n"
    hdr+="}};\nconstexpr const Definition* find(Id id) noexcept { for (const auto& p:kCatalog) if(p.id==id) return &p; return nullptr; }\n"
    hdr+="static_assert(kNONLIN1_TRANSIENT.size()<=160 && kNONLIN2_BLOOM.size()<=160 && kNONLIN3_DRUM.size()<=160,\"ES5510 program memory limit\");\n}\n"
    (ROOT/"NonLinTapPrograms.hpp").write_text(hdr)

if __name__ == "__main__": main()
