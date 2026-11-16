# Original TROLLfx one-pass density program: nonlin3_drum; not Ensoniq microcode.
# 86 words; 1 serial input diffuser stage(s)/channel; nine weighted read taps.
# Store PCs 19/20; tap read PCs [(21, 22), (25, 26), (29, 30), (33, 34), (37, 38), (41, 42), (45, 46), (49, 50), (53, 54)]; tap levels GPR 150..158; diffuser GPR 128+; wet/dry GPR 180/181.
# Offsets at 44.1kHz: [(97, 97), (163, 163), (263, 263), (397, 397), (557, 557), (751, 751), (977, 977), (1237, 1237), (1531, 1531)]; all nine envelope gains are host control values,
# multiplied with delayed audio in the ES5510 MAC path. Mono-center=True.
