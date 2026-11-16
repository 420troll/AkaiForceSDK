# Original TROLLfx mid/side width and position panner.
# Host updates GPR40..43 with the four Q22 matrix gains before each frame.
# GPR10/11 preserve stereo inputs; GPR12/13 form left; GPR14/15 form right.
# Output: (L+R)/2 +/- width*(L-R)/2, independently scaled by left/right pan.
# Every multiply and sum is executed by the ES5510 MAC pipeline.
MOV 10 239 0 0 0 6       # input L <- SER2L
MOV 11 238 0 0 0 6       # input R <- SER2R
MOV 12 10 0 0 0 6        # copy L for left output
MOV 13 11 0 0 0 6        # copy R for left output
MOV 14 10 0 0 0 6        # copy L for right output
MOV 15 11 0 0 0 6        # copy R for right output
MAC 12 40 0 0            # left L coefficient
MAC 13 41 1 0            # accumulate left R coefficient
MAC 14 42 0 0            # right L coefficient
MAC 15 43 1 0            # accumulate right R coefficient
NOP                       # retire right sum
MOV 241 13 0 0 0 6       # SER3L
MOV 240 15 0 0 0 6       # SER3R
NOP
NOP
NOP
END
