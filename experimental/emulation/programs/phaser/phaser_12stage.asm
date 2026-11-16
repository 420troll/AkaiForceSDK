# Original TROLLfx stereo phaser, assembled with phaser_asm.py.
# 12 first-order allpass sections per channel (24 sections across the stereo pair).
# The input and state/output equations for each stage are:
#   d = y[n-1] - x[n]
#   y[n] = x[n-1] + a*d
#   x[n-1] <- x[n]
# MAC multiplies a*d; ALU subtract/add and allpass state flow execute in ES5510.
# Host modulation changes the 12 signed Q22 coefficients per channel in GPRs.
# GPR 152 feedback in signed Q22 (-0.90..+0.90). Host performs final wet/dry blend.
# GPR 150/151 hold prior-frame final allpass output for feedback.
# GPR 32/33 hold the feedback-added input. Stage coefficients are GPR 64..75
# left and GPR 80..91 right. X state GPR 100..111 / 124..135; Y state
# GPR 112..123 / 136..147.
# The generated instruction image is phaser_12stage_words.inc.
