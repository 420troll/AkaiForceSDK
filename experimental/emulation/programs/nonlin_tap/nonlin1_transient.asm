# Original TROLLfx one-pass density program: nonlin1_transient; not Ensoniq microcode.
# 81 words; 1 serial input diffuser stage(s)/channel; nine weighted read taps.
# Store PCs 14/15; tap read PCs [(16, 17), (20, 21), (24, 25), (28, 29), (32, 33), (36, 37), (40, 41), (44, 45), (48, 49)]; tap levels GPR 150..158; diffuser GPR 128+; wet/dry GPR 180/181.
# Offsets at 44.1kHz: [(173, 211), (293, 347), (467, 523), (719, 797), (1031, 1151), (1453, 1597), (2027, 2237), (11519, 12101), (21101, 22051)]; all nine envelope gains are host control values,
# multiplied with delayed audio in the ES5510 MAC path. Mono-center=False.
