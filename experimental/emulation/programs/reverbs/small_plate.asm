# Original TROLLfx ES5510 small plate prototype; not factory microcode.
# 41 words. 2 serial allpass stages per channel; cross-coupled stereo feedback, stereo input.
# Host init: GPR 64+ are signed Q22 allpass coefficients; GPR 160 feedback,
# 161 wet, 162 dry; GPR 0/1 read taps; write-bank offsets live at instruction
# offsets 30/31; GPR 245 mask.
# Suggested base delay samples at 44.1 kHz: L=1301, R=1499; feedback=0.68.
# Structure: allpass diffusion before two feedback-delay lanes.
