# Original TROLLfx feedback delay with independent loop and output gains.
# GPR 0/1: left/right delay read offsets; GPR 8/9: left/right write offsets.
# GPR 40: feedback coefficient, GPR 41: wet output gain, GPR 42: dry gain.
# All coefficients use ES5510 MAC Q22 encoding, multiplier = value / 2^22.
# Every word runs a MAC in parallel; non-MAC ALU forms must set C/D=255.
MOV 10 239 0 0 0 0       # dry L <- SER2L; RAM reads L using GPR[0]
MOV 11 238 0 0 0 0       # dry R <- SER2R; RAM reads R using GPR[1]
MACDIL_MOV 12 14 40 12 6 # capture delayed L; feedback*DIL -> GPR14
MACDIL_MOV 13 15 40 12 6 # capture delayed R; feedback*DIL -> GPR15
MOV 16 10 0 0 0 6        # preserve dry L for feedback write
MOV 17 11 0 0 0 6        # preserve dry R for feedback write
ADD 16 14 0 0 0 6        # feedback write L = input L + feedback return
ADD 17 15 0 0 0 6        # feedback write R = input R + feedback return
MOV_DOL 0 16 0 0 5 1     # write feedback L at GPR[8]
MOV_DOL 0 17 0 0 5 1     # write feedback R at GPR[9]
MACREG_MOV 20 10 12 41 0 6 # wet L in-place; preserve dry L in GPR20
MACREG_MOV 21 11 13 41 0 6 # wet R in-place; preserve dry R in GPR21
MACREG_NOP 20 42 6        # dry gain L in-place
MACREG_NOP 21 42 6        # dry gain R in-place
ADD 20 12 0 0 0 6         # output L = dry gain + wet gain
ADD 21 13 0 0 0 6         # output R = dry gain + wet gain
MOV 241 20 0 0 0 6        # SER3L
MOV 240 21 0 0 0 6        # SER3R
END
