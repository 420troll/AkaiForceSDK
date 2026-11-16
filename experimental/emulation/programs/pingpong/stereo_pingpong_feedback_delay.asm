# Original TROLLfx ping-pong variant of the 19-word stereo feedback delay.
# GPR 0/1 delay reads; GPR 8/9 writes; GPR40 feedback; GPR41 wet; GPR42 dry.
# Each channel's delayed return feeds the opposite write bank, producing
# alternating stereo echoes. All loop, delay and output processing is ESP code.
MOV 10 239 0 0 0 0       # dry L <- SER2L
MOV 11 238 0 0 0 0       # dry R <- SER2R
MACDIL_MOV 12 14 40 12 6 # delayed L; feedback*DIL -> 14
MACDIL_MOV 13 15 40 12 6 # delayed R; feedback*DIL -> 15
MOV 16 10 0 0 0 6
MOV 17 11 0 0 0 6
ADD 16 15 0 0 0 6       # write L = input L + delayed R feedback
ADD 17 14 0 0 0 6       # write R = input R + delayed L feedback
MOV_DOL 0 16 0 0 5 1
MOV_DOL 0 17 0 0 5 1
MACREG_MOV 20 10 12 41 0 6
MACREG_MOV 21 11 13 41 0 6
MACREG_NOP 20 42 6
MACREG_NOP 21 42 6
ADD 20 12 0 0 0 6
ADD 21 13 0 0 0 6
MOV 241 20 0 0 0 6
MOV 240 21 0 0 0 6
END
