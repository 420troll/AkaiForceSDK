# Original TROLLfx feedback delay, all audio math in ES5510 microinstructions.
# GPR 0/1: left/right read offsets (updated by assembler from delay parameter).
# GPR 6/7: left/right delay-bank write offsets (PC6 and PC7).
# GPR 40: feedback multiplier in ES5510 MAC Q22 encoding.
# Each line: OP A B C D select ram. Fields not used by an op are zero.
# Delay RAM reads at PC0/PC1 feed DIL two instructions later.
MOV 10 239 0 0 0 0    # input SER2L -> dry L; read delay left at GPR[0]
MOV 11 238 0 0 0 0    # input SER2R -> dry R; read delay right at GPR[1]
MACDIL_MOV 12 14 40 12 6 # delayed L -> GPR12; delayed L * feedback -> GPR14
MACDIL_MOV 13 15 40 12 6 # delayed R -> GPR13; delayed R * feedback -> GPR15
ADD 10 14 0 0 0 6    # dry L + feedback return -> write signal L
ADD 11 15 0 0 0 6    # dry R + feedback return -> write signal R
MOV_DOL 0 10 0 0 5 1   # write left feedback signal at GPR[6] offset
MOV_DOL 0 11 0 0 5 1   # write right feedback signal at GPR[7] offset
MOV 241 10 0 0 0 6    # output SER3L
MOV 240 11 0 0 0 6    # output SER3R
END
