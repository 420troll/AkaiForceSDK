# Original TROLLfx ES5510 nonlin2 bloom prototype; not factory microcode.
# 61 words. 4 serial allpass stages per channel; cross-coupled stereo feedback, stereo input.
# Host init: GPR 64+ are signed Q22 allpass coefficients; GPR 160 feedback,
# 161 wet, 162 dry; GPR 0/1 read taps; write-bank offsets live at instruction
# offsets 50/51; GPR 245 mask.
# Suggested base delay samples at 44.1 kHz: L=1877, R=2347; feedback=0.79.
# Structure: allpass diffusion before two feedback-delay lanes.
