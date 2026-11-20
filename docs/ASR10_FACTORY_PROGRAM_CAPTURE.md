# ASR-10 factory program capture procedure

## Preparation

1. Build OpenASR at commit `241f9331477b07b072330ca6ebd2d7a2e2cc832e` with
   Lua support; record the actual binary build and MAME version. A local build
   and boot attempt were completed; current results are in
   `ASR10_BOOT_ROM_RUNTIME_CHECK.md`.
2. Obtain an OS floppy and system ROMs through lawful means. OpenASR's
   `floppies/README.md` and `roms/README.md` describe the expected local paths;
   neither is distributed. Record private SHA-256 hashes, but do not add
   payloads to Git or public CI.
3. Use a private working directory for stdout trace, reconstructed JSON,
   bundles, disassembly and WAV files.
4. Check OpenASR's ES5510 model/runtime configuration. The ASR machine calls
   `set_disable()` to remove the chip from normal scheduler execution, while
   `esqpump.cpp::sound_stream_update()` manually calls `run_once()` per sample
   when the DSP is released (unless fake processing is compiled in). Therefore
   a real upload plus HALT release can reach instruction execution in this
   model. Confirm the build flags, observed release, and pump path in the run
   log; this is not evidence of physical timing or routing fidelity. Do not
   silently change timing/pump behavior and call it original.

## Capture

Run `tools/asr10_es5510_trace.lua` passively through MAME's Lua autoboot script
facility. Boot the machine, select a simple effect, activate it, and vary one
parameter at a time. Preserve the complete stdout log. The script emits a
metadata row identifying source revisions and that ROM identity is not
verified. Observe:

- PA4/PADAT transitions around program transfer.
- Six instruction-latch bytes and three GPR-latch bytes before each commit.
- read-select/readback traffic and selectors.
- instruction, GPR and combined commit data/index.
- direct host control, DADR, RAM-select, serial and halt-enable accesses.
- later isolated GPR writes when adjusting a single effect parameter.

The Lua tap itself is passive and logs raw CPU events. It does not change
memory, halt the emulation, or write programs into the device. PA4 is not
available from the tapped program-space map; the observed callback transitions
came from a separate build-only log in the OpenASR model.

## Reconstruct and replay

```sh
python3 tools/asr10_capture_reconstruct.py \
  /private/asr10-es5510-tap.tsv /private/asr10-es5510-capture.json
python3 tools/asr10_capture_disassemble.py \
  /private/asr10-es5510-capture.json > /private/asr10-es5510-disassembly.txt
python3 tools/asr10_capture_to_bundle.py \
  /private/asr10-es5510-capture.json /private/asr10-candidate.esp \
  --accept-candidate --sample-rate 44100
```

The scripts refuse capture/bundle paths within the repository. Review the
program, associated initialization and special registers, first/last END
behavior, RAM configuration, readback, halt/run sequence, selected effect and
parameter update intervals. The renderer bundle is a reduced replay format,
not a full restoration of all host transactions. Before any authenticity
claim, run the same captured state in a pinned MAME ES5510 execution harness
and the standalone adapter and explain every mismatch.

## Acceptance checklist

The current parser intentionally sets `validated_factory_program` false. A
future human-validated capture should establish all of the following:

- identified and hashed firmware/media inputs (privately held);
- firmware CPU execution and specific effect-selection interval;
- HALT asserted before uploads and RUN release after initialization;
- contiguous, bounded instruction commits and a defensible program boundary;
- all GPRs and relevant special-register writes recorded, with readback where
  firmware performs it;
- delay RAM clear/configuration and required host serial/audio routing;
- reproducible trace and source/build metadata;
- differential execution and audio tests against the pinned MAME model.

No such complete capture is present in this branch. Synthetic traces used in
unit tests never meet capture acceptance.

OpenASR's committed V3.50 research notes preserve an important prior partial
result: the firmware reportedly uploaded ROM HALL and 44LUSH, verified host
readback, and yielded reproducible instruction-list hashes and END words.
However, those reports point to raw logs under `/private/tmp/` that are not
present now, and the complete instruction/GPR values are not printed in the
committed reports. Treat this as prior partial trace evidence, not a replayable
capture. Restore the original logs/media or reacquire them in a new run before
building a bundle.
