# Original TROLLfx dual-band rotary speaker proof of concept.
# 40 48-bit words. The first two cascaded one-pole sections make a low drum
# band; high = mono input - low band makes the horn band. Both bands are written
# to distinct ES5510 DBASE ring banks, then read at host-modulated offsets.
# ES5510 performs downmix, crossover filtering, DOL writes, DIL reads, gain MACs
# and serial outputs. Host supplies only crossover coefficient, delay offsets,
# modulation gains and slow/fast speed ramp trajectories.
# GPR[19] / GPR[20] are PC-addressed DRAM write offsets (high=0, low=bank size).
# GPR[21] / GPR[24] are PC-addressed moving read offsets (high, low bank+tap).
# GPR 54 is one-pole crossover coefficient Q22.
# GPR 50/51 are horn left/right gain; 52/53 drum left/right gain, Q22.
