# ASR-10 OS 3.53 ES5510 capture sprint

**State:** Ready for one user-driven effect-load capture; no factory program has
yet been recovered from this run.
**Worktree:** `feature/5510-asr10-factory-programs`
**Scope:** Capture tooling and documentation only. No Force plugin is changed.

## Preflight verified

The specified local installation is present:

- Executable: `/home/gamer/OpenASR-run/mame`, reports `0.288 (unknown)`;
  the upstream source commit is not embedded in the executable.
- ROM directory: `/home/gamer/OpenASR-run/roms/asr10booth/` with the two
  expected ASR-10 v1.5b ROM files.
- OS image: `/home/gamer/OpenASR-run/floppies/os353.img`, 1,638,400 bytes.
- `mame -verifyroms asr10booth -rompath /home/gamer/OpenASR-run/roms`
  reported `romset asr10booth is good`.

The installed binary contains the `T551_PA` callback diagnostic. With the
exact launcher flags (`-log -oslog`), a 12-second private preflight produced
119 Lua tap rows: 26 host-register writes, three combined instruction/GPR
commits, three GPR commits, and three PA4 callback records. A 4-second run
with the same options also captured the three PA4 records, host writes, and a
combined commit. This verifies that the full launcher records known boot-time
DSP initialization plus HALT/RUN callback events; it does **not** represent a
selected effect. Reconstruction correctly labels the boot-only state
`incomplete` because it lacks a contiguous program, effect-specific
initialization, and a post-upload RUN transition. A separate 12-second run
without `-log -oslog` saw more combined commits but no PA4 callback rows, so
that mode is not used for the user capture.

The launcher copies the OS image to a unique private run folder, checks the
copy hash, sets it read-only, and mounts only that copy. MAME configuration,
logs, captured effect objects, trace, reconstruction JSON, and disassembly all
stay under `/home/gamer/OpenASR-run/captures/`, outside Git. Original ROMs and
the OS image are only read. The recorder records the installed binary hash and
version; it accurately marks source commit as unknown.

## User capture command

From a WSL/Linux terminal with the graphical display available, run this one
command:

```sh
sh /mnt/c/Users/Gamer/research/trollfx-asr10/tools/capture_asr10_os353.sh
```

When OpenASR opens:

1. Load the standalone effect file `44DDL+CHORUS` from the OS 3.53 floppy's
   Effects directory.
2. Change one displayed effect parameter by one step. Note its name and
   before/after values in your reply.
3. Exit OpenASR normally so the launcher closes the log and reconstructs the
   captured state.

The launcher prints the private capture directory. Send back that directory
path and the parameter name/value change; do not copy ROMs, EFE files, trace
contents, or disassembly into Git or chat. The private directory itself stays
on this PC.

## Capture contents and validation

Each run produces:

- `manifest.txt`: executable, ROM, OS, and Lua hashes; MAME version; the
  explicitly unknown source revision; run-copy integrity and exit status.
- `openasr.log`: unfiltered stdout/stderr with all passive host taps and
  `T551_PA` HALT/RUN callback diagnostics.
- `os353-catalog.json` and `reference-effects/`: private OS disk inventory
  and extracted effect objects for comparison.
- `44DDL+CHORUS-candidate-metadata.json`: static candidate boundary and
  metadata for the existing object, not proof that the bytes executed.
- `capture.json`: reconstructed host transactions, final instruction/GPR
  state, trace order, PA4 transitions, and deterministic state hashes.
- `candidate-disassembly.txt`: readable instruction fields, private.

Reconstruction status must remain `incomplete` until the trace contains an
effect load, an ordered HALT → bounded instruction/GPR/memory setup → RUN
sequence, contiguous instruction memory with a defensible program boundary,
and required special-register state. The boot-time callback's `received=0`
and `loaded=0` flags are not effect-load evidence. Compare the captured words
against the private 44DDL+CHORUS `FxUcode` candidate; report exact overlap,
word count, register-state hashes, and differences without printing firmware
bytes.

Do not create a renderer bundle or claim authentic execution unless the capture
passes those checks. If `capture.json` remains incomplete, keep the raw logs
private and diagnose that specific missing event before any DSP replay.

## Replay status / next blocker

The only available run so far is the boot-time preflight. It does not meet the
replay gate, so no factory program was loaded into the standalone 5510 engine,
no deterministic factory audio render exists, and no MAME-vs-standalone audio
comparison is claimed. The concrete next dependency is the user-driven
44DDL+CHORUS selection and one-parameter edit in the GUI.

## Reproducible preflight command

The validated preflight used the same launcher command line with a temporary
wrapper appending `-seconds_to_run 4` and running under Xvfb. Interactive
captures must omit that timeout. Equivalent bounded manual command:

```sh
xvfb-run -a /home/gamer/OpenASR-run/mame asr10booth \
  -rompath /home/gamer/OpenASR-run/roms \
  -flop1 /private/run/os353-readonly-copy.img \
  -autoboot_script tools/asr10_es5510_trace.lua \
  -skip_gameinfo -noreadconfig -nowriteconfig -log -oslog \
  -seconds_to_run 4
```

Use the launcher for the real GUI session; it supplies isolated writable
configuration directories and a unique private log path.
