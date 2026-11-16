# Original TROLLfx ES5510 nonlin3 drum prototype; not factory microcode.
# 36 words. 1 serial allpass stages per channel; cross-coupled stereo feedback, centered mono drum feed.
# Host init: GPR 64+ are signed Q22 allpass coefficients; GPR 160 feedback,
# 161 wet, 162 dry; GPR 0/1 read taps; write-bank offsets live at instruction
# offsets 25/26; GPR 245 mask.
# Suggested base delay samples at 44.1 kHz: L=509, R=509; feedback=0.46.
# Structure: allpass diffusion before two feedback-delay lanes.
