#!/usr/bin/env python3
"""Generate five original ES5510 reverb prototypes under the 160-word limit."""
from pathlib import Path

OUT = Path(__file__).resolve().parent

def enc(a=255,b=255,c=255,d=255,op=4,sel=0,ram=6):
    return ((d&255)<<40)|((c&255)<<32)|((b&255)<<24)|((a&255)<<16)|((op&15)<<12)|((sel&15)<<8)|((ram&7)<<3)
def mov(dst,src): return enc(dst,src,op=9)
def alu(dst,src,op): return enc(dst,src,op=op)
def nop(): return enc()
def mac(dst,coef): return enc(255,255,dst,coef,op=4)
def end(): return enc(op=15,ram=0)

def one_pole(signal,temp,state,coefficient):
    # y = state + alpha * (x - state); state and MAC execute in ES5510.
    return [mov(temp,signal),alu(temp,state,3),nop(),mac(temp,coefficient),
            alu(temp,state,2),mov(state,temp),mov(signal,temp)]

# Baseline independent-stereo feedback delay, with the feedback writes replaced
# by cross-coupling for plate networks. All values are 24-bit/Q22 GPR controls.
BASE=[
  0xffffef0a9000,0xffffee0b9000,0xa00e000c9c30,0xa00f000d9c30,
  0xffff0a109030,0xffff0b119030,0xffff0e100030,0xffff0f110030,
  0xffff10009508,0xffff11009508,0xa10c0ab49030,0xa10d0bb59030,
  0xa2b4ffff4030,0xa2b5ffff4030,0xffff0cb40030,0xffff0db50030,
  0xffffb4f19030,0xffffb5f09030,0xfffffffff000]

def allpass_prefix(stages):
    words=[]
    coeffs=[]
    for channel, inp0 in ((0,10),(1,11)):
        inp=inp0
        for stage,coef_value in enumerate(stages):
            xreg=100+channel*16+stage
            yreg=112+channel*16+stage
            creg=64+channel*16+stage
            words += [alu(yreg,inp,3),nop(),mac(yreg,creg),alu(yreg,xreg,2),mov(xreg,inp)]
            coeffs.append((creg,coef_value))
            inp=yreg
        words.append(mov(inp0,inp))
    return words,coeffs

def build(name, stages, delays, feedback, cross=False, center=False):
    prefix,coeffs=allpass_prefix(stages)
    # Keep the two input RAM reads and two delayed-return captures in their
    # original first four slots. Their DIL timing depends on that schedule.
    prefix = BASE[:4]+prefix
    # Insert a controllable one-pole damping stage after diffusion and inside
    # the feedback path. Coefficient GPR163 is the control-rate alpha.
    for channel,signal in enumerate((10,11)):
        prefix += one_pole(signal,172+channel,174+channel,163)
    tail=BASE[4:]
    if cross:
        # Feedback input L gets delayed R; input R gets delayed L.
        tail[2]=alu(16,15,2)
        tail[3]=alu(17,14,2)
    if center:
        # After the two AP chains, collapse the stereo signal to a centered
        # mono drum send, while the base feedback network still returns stereo.
        # The prefix already preserved input channels in 10/11.
        prefix += [mov(30,10),alu(30,11,0),alu(30,30,14),mov(10,30),mov(11,30)]
    words=prefix+tail
    if len(words)>160: raise ValueError((name,len(words)))
    write_left=4+len(prefix)-4+4
    (OUT/f'{name}.asm').write_text(
        f'# Original TROLLfx ES5510 {name.replace("_"," ")} prototype; not factory microcode.\n'
        f'# {len(words)} words. {len(stages)} serial allpass stages per channel; '
        f'{"cross-coupled stereo" if cross else "independent stereo"} feedback, '
        f'{"centered mono drum feed" if center else "stereo input"}.\n'
        f'# Host init: GPR 64+ are signed Q22 allpass coefficients; GPR 160 feedback,\n'
        f'# 161 wet, 162 dry; GPR 0/1 read taps; write-bank offsets live at instruction\n'
        f'# offsets {write_left}/{write_left+1}; GPR 245 mask.\n'
        f'# Suggested base delay samples at 44.1 kHz: L={delays[0]}, R={delays[1]}; '
        f'feedback={feedback}.\n'
        f'# Structure: allpass diffusion before two feedback-delay lanes.\n')
    (OUT/f'{name}_words.inc').write_text('\n'.join(f'0x{w:012x}ULL,' for w in words)+'\n')
    return len(words),coeffs

if __name__=='__main__':
    designs=[
      # Transient: one short diffuser per channel and low feedback.
      ('nonlin1_transient',[0.48],[317,431],0.24,False,False),
      # Bloom: four serial diffusers with long returns and stronger regeneration.
      ('nonlin2_bloom',[0.62,-0.58,0.66,-0.61],[1877,2347],0.79,True,False),
      # Drum: short return plus centered send and one diffuser per channel.
      ('nonlin3_drum',[0.42],[509,509],0.46,True,True),
      # Small plate: 2 allpass stages and moderate cross-coupled decay.
      ('small_plate',[0.58,-0.52],[1301,1499],0.68,True,False),
      # Large plate: 3 diffusers, long cross-coupled paths and higher feedback.
      ('large_plate',[0.63,-0.59,0.67],[2801,3163],0.82,True,False),
    ]
    for item in designs:
        size,_=build(*item);print(f'{item[0]}: {size} words')
