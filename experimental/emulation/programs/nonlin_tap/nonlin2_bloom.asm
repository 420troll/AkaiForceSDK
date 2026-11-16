# Original TROLLfx one-pass density program: nonlin2_bloom; not Ensoniq microcode.
# 117 words; 4 serial input diffuser stage(s)/channel; nine weighted read taps.
# Store PCs 50/51; tap read PCs [(52, 53), (56, 57), (60, 61), (64, 65), (68, 69), (72, 73), (76, 77), (80, 81), (84, 85)]; tap levels GPR 150..158; diffuser GPR 128+; wet/dry GPR 180/181.
# Offsets at 44.1kHz: [(4999, 7001), (9001, 12007), (16001, 19001), (23003, 26003), (30011, 34019), (38011, 42013), (46021, 50023), (54001, 58013), (62003, 66013)]; all nine envelope gains are host control values,
# multiplied with delayed audio in the ES5510 MAC path. Mono-center=False.
