# Factory microprogram research harness

This harness is an isolated renderer for external ES5510 test bundles. It does
not change or link into the working 5510 Single V0.2 plugin. No Force install
or deployment is part of this workflow.

## External bundle format

`tools/es5510_render.cpp` accepts `ES5510_BUNDLE 1` text files. Records are
applied in order; repeat a record to model repeated host writes. Use
`instruction_at ADDRESS WORD` for the 48-bit program image, `gpr REG VALUE`
for registers 0..191, `spr REG VALUE` for writable SPR addresses 234..251,
`host_serial VALUE`, `sample_rate HZ`, and `dram ADDRESS SIGNED16`. `instruction
WORD` is also accepted as a sequential convenience. Program capacity is 160
words; END is required and execution stops at the first END each frame. Invalid
records, ranges, sample-rate mismatches, WAV formats, and missing END fail
closed with a diagnostic.

For a private `ES5510C` capture, `tools/extract_es5510_program.py` extracts
only the instruction words through first END. It requires host serial to be
provided explicitly and deliberately omits captured GPR, SPR, control, and
DRAM snapshots. Add only independently verified *pristine* initialization
records to the generated external bundle. Never commit the resulting bundle,
microcode, private audio, or ROM/OS data.

The VFX-family pump mapping used by this adapter has three stereo input pairs
on serial ports 0..2 and processed stereo output on port 3. The WAV renderer
feeds stereo input on the dry pair (port 2) and writes port 3. Host serial bits
can override serial lanes as in the MAME pump. This is an adapter contract,
not a universal routing claim for every Ensoniq product.

## Build and execute

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/mame_es5510_standalone_test.cpp \
  -o /tmp/es5510-factory-test
/tmp/es5510-factory-test

g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp tools/es5510_render.cpp \
  -o /tmp/es5510-factory-render
python3 tools/extract_es5510_program.py /path/to/private.esp /tmp/44ddl.esp \
  --host-serial 0x48
/tmp/es5510-factory-render /tmp/44ddl.esp input.wav output.wav
```

For reproducible startup, create a fresh engine with `prepare()`, load the
program, apply its ordered initialization records, then process. `reset()`
clears all processor and DRAM state, so a reset run must reload the complete
bundle before processing.

## 44DDL+CHORUS accuracy report

**Status: playback path implemented; audio accuracy comparison unresolved.**

The private research workspace contains a 44DDL+CHORUS `ES5510C` capture
(SHA-256 `a9eca102ea11462eb9126b79e5659baa2f7b748148ed2d09f853d4e93c940c1c`)
and ASR-10 emulator recordings. The instruction section contains 160 slots,
with first END at slot 59. The matching Windows recording is 44.1 kHz stereo
16-bit, 1,103,686 frames, and was made with `ASR_TEST_SINE=440:0.25` while the
emulator navigated/loaded the effect. It is not a clean aligned input/output
pair for sample error, SNR, or correlation scoring.

The capture's GPR, SPR, and delay-RAM values are end-of-run snapshots. They are
not used as startup state. Full pristine delay RAM and the ordered pristine
initialization transaction stream are unavailable in the checked private
capture metadata, so a numerically meaningful 44DDL render cannot yet be
claimed. The extractor can run the captured instruction image with explicit
external pristine setup; cleared DRAM or post-run snapshots are not substitutes
for the missing state. No audio accuracy metric is reported until a clean
reference input/output pair and valid initial state are provided.

**Verified:** private capture header/section dimensions and first-END index;
the core's bounded frame execution and serial mapping are exercised by the
hand-authored stereo swap test. **Unresolved:** captured-program sound,
ASR-10 replay equivalence, exact factory association, and any sample-level
accuracy. The reference WAV and captured microcode stay outside this repo.
