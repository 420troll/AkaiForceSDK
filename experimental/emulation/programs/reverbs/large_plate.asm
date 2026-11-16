# Original TROLLfx ES5510 large plate prototype; not factory microcode.
# 51 words. 3 serial allpass stages per channel; cross-coupled stereo feedback, stereo input.
# Host init: GPR 64+ are signed Q22 allpass coefficients; GPR 160 feedback,
# 161 wet, 162 dry; GPR 0/1 read taps; write-bank offsets live at instruction
# offsets 40/41; GPR 245 mask.
# Suggested base delay samples at 44.1 kHz: L=2801, R=3163; feedback=0.82.
# Structure: allpass diffusion before two feedback-delay lanes.
