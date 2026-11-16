Standalone ES5510 ARMv7 test kit

This is an experimental user-mode harness, not an Akai VST plugin. It has not
been installed on or run on Akai hardware. Use a compatible ARMv7-A Linux
hard-float environment with glibc 2.31 or newer.

The binaries contain an original MAME-semantics ES5510 adapter and no ROM,
firmware or factory effect program. The included bundles are:

  original_stereo_swap.esp
    Original 3-word MOV/END test; swaps dry input channels.
  mame_pump_summed_delay_test.esp
    Adaptation of MAME's BSD-3-Clause PUMP_REPLACE_ESP_PROGRAM fixture. It
    sums the three stereo serial buses, uses delay RAM and returns stereo.
    This is a synthetic emulator test, not Ensoniq factory microcode.

See THIRD_PARTY_NOTICES.md for MAME BSD-3-Clause terms.

Run the benchmark from this directory:

  ./es5510-bench 44100 89
  ./es5510-bench 44100 160

These process synthetic no-op microprograms for 44,100 audio frames and
report elapsed time, instructions/frame and host time per audio second.

To render a bundle and stereo 16-bit PCM WAV at 44.1 kHz:

  ./es5510-render original_stereo_swap.esp input.wav output.wav
  ./es5510-render mame_pump_summed_delay_test.esp input.wav output.wav

Provide stereo input WAV with matching 44.1 kHz sample rate. The second
program sums FX1, FX2 and dry stereo serial lanes in the MAME pump mapping,
then reads its result through the modeled delay RAM path. It is not a sound
quality or factory-effect compatibility claim.
