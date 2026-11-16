# Original TROLLfx ES5510 nonlin1 transient prototype; not factory microcode.
# 31 words. 1 serial allpass stages per channel; independent stereo feedback, stereo input.
# Host init: GPR 64+ are signed Q22 allpass coefficients; GPR 160 feedback,
# 161 wet, 162 dry; GPR 0/1 read taps; write-bank offsets live at instruction
# offsets 20/21; GPR 245 mask.
# Suggested base delay samples at 44.1 kHz: L=317, R=431; feedback=0.24.
# Structure: allpass diffusion before two feedback-delay lanes.
