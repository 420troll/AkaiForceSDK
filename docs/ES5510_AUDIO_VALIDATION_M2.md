# ES5510 audio validation — milestone 2

## Result

**Verified for the deterministic original TROLLfx 11-instruction stereo
feedback delay:** the ASR-10 emulator ES5510 core and the standalone
MAME-derived adapter produced identical PCM16 output for all five stimuli at
44.1 kHz for 8,192 frames and 16,384 frames. Across every engine/stimulus/run,
first divergent frame was none, maximum absolute difference was 0 samples,
RMS difference was 0.0, and best alignment lag was 0 frames. This validates
the tested program and state, not general ES5510 bit accuracy or factory audio.

At 8,192 frames, the input impulse is at frame 1,024 and only its direct tap is
inside the capture; the configured 11,025-frame echo is outside this window.
The 16,384-frame extension places the impulse at frame 2,048 and verifies both
the direct tap `(24000, 0)` and delayed left tap at frame 13,073, offset 11,025,
amplitude 13,199. Both engines produced those exact taps. No right-channel
crosstalk appeared for the left-only impulse (right peak 0). For equal-channel
sine and sweep, stereo-difference RMS was 0. Independent-channel noise and
stereo-phase inputs preserved nonzero stereo difference, identical between
engines: respectively 14,073.4271 and 23,050.7656 RMS samples.

## Comparison setup

The original engine was the ASR-10 emulator's `ES5510` implementation at source
commit `95106a6066fedb98dd7eddaba4d5a75e50734e7f`. The candidate was this repository's
`MameES5510` adapter. The comparison source is external to the repo and is
selected with `--reference-root`; no emulator source copy, factory payload,
private capture, or audio is included here.

Each engine was freshly constructed for each render. The original engine's
constructor reset zeroes its 1,048,576-word DRAM and processor registers. The
adapter's `prepare()` resets its registers/pipelines and zeroes the same-size
DRAM. Both then received the same 11 instruction words, host serial `0x20`,
and, for delay runs, these writes in the same order:

| Register | Value | Purpose |
|---|---:|---|
| 244 | `0x000000` | MEMSIZ = 0; one address unit per DRAM word |
| 245 | `0x0d754f` | delay length 881,999; ring length 882,000 |
| 248 | `0x000000` | DBASE |
| 0 / 1 | `0x002b11` / `0x06e5b9` | left/right read offsets |
| 6 / 7 | `0x000000` / `0x06baa8` | left/right write offsets |
| 40 | `0x233333` | feedback coefficient, Q22 (approximately 0.55) |

Both engines receive stereo input at SER2 (`SER2L/R`, register inputs 4/5)
and read processed stereo from SER3 (outputs 6/7). The delay instruction
sequence performs serial reads, delay RAM reads and writes, DIL/MAC feedback,
ALU summation, and serial output. At 44.1 kHz, the tested stereo delay is
11,025 frames.

The five deterministic inputs are an impulse, 997 Hz sine, logarithmic sweep,
seeded independent-channel noise, and stereo phase-offset 431 Hz tone. The
16,384-frame run ensures each stimulus includes post-delay output. The exact
reference implementation and adapter are not initialized by importing any
post-execution snapshots. The standalone WAV renderer was also run using
[`original_stereo_feedback_delay.esp`](../experimental/emulation/programs/original_stereo_feedback_delay.esp)
with a frame-zero stereo impulse `(24576, 8192)`; its output taps were
`(24576,8192)`, `(13516,4505)`, `(7433,2477)`, and `(4088,1362)` at frames
0, 11,025, 22,050, and 33,075 respectively. This verifies the checked-in
bundle's feedback path independently of the comparison driver.

## Reproduction

The ASR-10 emulator checkout must contain `src/es5510.cpp` and `src/es5510.h`.
Its source is not vendored into this repository.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/audio_validation/run.py \
  --reference-root /path/to/asr10-emulator \
  --out /tmp/es5510-audio-8192 --rate 44100 --frames 8192

PYTHONDONTWRITEBYTECODE=1 python3 tools/audio_validation/run.py \
  --reference-root /path/to/asr10-emulator \
  --out /tmp/es5510-audio-16384 --rate 44100 --frames 16384

PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s tests/audio_validation -v
```

Generated stimuli, per-engine WAV-sample CSVs and JSON summaries remain in the
selected temporary output directory. The test runner can report the first
divergent frame and sample metrics; no output divergence occurred, so there is
no first divergent instruction/frame to isolate. This run does not expose or
compare internal MAC/ALU registers instruction-by-instruction.

## Factory program gate

No `44DDL+CHORUS` execution was attempted. Available effect metadata and
post-execution ES5510 captures do not provide a reproducible pristine GPR/SPR,
MEMSIZ/DLENGTH/DBASE, delay-RAM, and host-serial initialization sequence.
Static EFE fields and a captured final state cannot replace those writes. The
private GPR-initialization audit likewise marks the factory register values
and external delay RAM missing. `tools/extract_es5510_program.py` can extract
only program words and intentionally omits runtime snapshots; it is not a
complete factory replay bundle. Factory playback and comparison remain
unresolved until a deterministic, independently reproducible init capture and
aligned emulator reference audio exist. Proprietary microcode and private WAVs
remain outside Git.
