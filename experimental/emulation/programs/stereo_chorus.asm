# Original TROLLfx host-modulated stereo chorus, all delay/audio mixing in ESP.
# A host LFO updates GPR[0] (left read offset) and GPR[1] (right read offset)
# once per frame. Offsets are in raw sample offset units (MEMSIZ 0).
# The reads occur at PC0 and PC1; right phase should be left phase + pi.
# GPR 4/5: left/right write offsets, GPR 40: wet gain in MAC Q22 encoding.
MOV 10 239 0 0 0 0    # input SER2L -> dry L; read modulated delay L
MOV 11 238 0 0 0 0    # input SER2R -> dry R; read modulated delay R
MACDIL_MOV 12 14 40 12 6 # delayed L -> GPR12; delayed L * wet -> GPR14
MACDIL_MOV 13 15 40 12 6 # delayed R -> GPR13; delayed R * wet -> GPR15
MOV_DOL 0 10 0 0 5 1   # write dry input L (no feedback)
MOV_DOL 0 11 0 0 5 1   # write dry input R (no feedback)
ADD 10 14 0 0 0 6    # dry L + wet delayed L
ADD 11 15 0 0 0 6    # dry R + wet delayed R
MOV 241 10 0 0 0 6    # output SER3L
MOV 240 11 0 0 0 6    # output SER3R
END
