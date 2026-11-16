TROLLfx ES5510 Phase 2 ARMv7 standalone test kit

This package is a command-line research harness, not an Akai VST plugin. It has
not been installed on or run by an Akai Force. Use ARMv7-A Linux hard-float with
glibc 2.31 or newer.

Contents include the stripped renderer and benchmark, original TROLLfx stereo
feedback-delay and modulated-chorus ES5510 bundles/source, synthetic impulse
and chord inputs, assembler source, and the MAME BSD-3-Clause notice. No MAME
framework, ROM, firmware, or Ensoniq factory microprogram is included.

Render the included 44.1 kHz stereo 16-bit PCM impulse:

  ./es5510-render original_stereo_feedback_delay.esp stereo_input_impulse.wav delay.wav
  ./es5510-render original_stereo_chorus.esp stereo_input_impulse.wav chorus.wav

Render the musical test input:

  ./es5510-render original_stereo_feedback_delay.esp original_test_chord_input.wav delay-chord.wav
  ./es5510-render original_stereo_chorus.esp original_test_chord_input.wav chorus-chord.wav

Run native-style workloads on this ARM environment:

  ./es5510-bench --rate 44100 --frames 256 --block 64 --program original_stereo_feedback_delay.esp
  ./es5510-bench --rate 44100 --frames 44100 --block 4096 --program original_stereo_chorus.esp
  ./es5510-bench --rate 48000 --frames 48000 --block 4096 --program original_stereo_feedback_delay_48k.esp
  ./es5510-bench --rate 48000 --frames 48000 --block 4096 --program original_stereo_chorus_48k.esp
  ./es5510-bench 44100 160

The ARM runtime has not been measured in the build environment. Record the
Force model, OS version, commands, elapsed times, and whether audio underruns
occur. Run from a temporary directory; do not install the binaries or modify
Force projects/system files.
