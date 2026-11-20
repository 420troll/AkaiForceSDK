# ASR-10 EFE FxUcode and update-task trace

## Scope and confidence

This report records a new static disassembly of the private 3.53
`44DDL+CHORUS` effect object, plus the header relationships described by the
unofficial ESP1 programming guide. It is not a runtime trace. Capstone 5.0.6
decoded the 68k object-local code in big-endian 68000 mode. No private object
or full disassembly is committed.

| Finding | Evidence | Confidence |
|---|---|---|
| `FxUcode` is a header-relative field at object offset `0x26`; for `44DDL+CHORUS`, it is `0x0a3c`. | Header parser and guide's field map; the value points inside `FxSize=0x0c59`. | Verified static layout / strong candidate section |
| The header stores output GPRs `0x82`/`0x83` and `FxNumVoices=1` in all 12 extracted ASR effect objects. | Parsed fields following three NUL-terminated bus names, consistent with the unofficial guide's documented header. | Verified stored output assignment; not an initial GPR value |
| The bus-name fields are populated per object. | `44DDL+CHORUS` stores `DDL+CHORUS / DRY / DRY`; the three plate objects store `REVERB / REVERB / DRY`. | Verified metadata; actual host route semantics not independently traced |
| The effect update task starts at `0x0452`. | Header `FxUpdateTask=0x0452`, disassembles as reachable 68000 instructions. | Verified static |
| The task calls local routine `0x0564` twice and update routine `0x0468` twice, advancing its A2 state by eight bytes between the latter calls. | Direct branch/call targets and `LEA 8(A2),A2` in task entry. | Verified static |
| Routine `0x0564` computes A2 as object base A1 + `FxUcode` − `0x0e`. | `A2=A1`; `ADDA.W $38(A1),A2`; `LEA -$0e(A2),A2`. | Verified static |
| Routine `0x0468` reads relative offsets and parameter variable offsets, interpolates/limits a value, stores an updated effect variable, then calls absolute service `$801e`. | Local instructions `0x046c–0x055c`; task data remains to be fully typed. | Verified behavior at instruction level; service semantics unresolved |
| `$801e` writes an ES5510 GPR. | No body or host transaction was observed. | Unresolved; do not claim |
| The 14-byte area before `FxUcode` is a seven-word DSP metadata/GPR map. | The task uses the area as input, but the schema and complete consumption are not established. | Unresolved |

## Observed task flow

```text
FxUpdateTask 0x0452
  -> 0x0564: A2 = effect_base + FxUcode - 14
  -> 0x0468: resolve parameter-relative data; update/slew variable; call $801e
  -> 0x0564: reset A2 to the same pre-FxUcode area
  -> A2 += 8
  -> 0x0468: process another parameter/update record; call $801e
  -> return
```

`0x0468` reads the parameter variable offset at `4(A0)` and indexes the
current-variable base A5. It reads old/current and target-related values,
uses a lookup table at PC-relative `0x08a6`, and applies a bounded step before
writing a byte or word back through A6. For several parameter types it also
uses signed-byte semantics. It then advances A2 eight bytes and invokes
`$801e.w`. This demonstrates that the effect object contains executable
parameter-update logic and an eight-byte-stride descriptor area adjacent to
the candidate code. It does not reveal the callee's register write semantics.

The 14-byte area in `[FxUcode-14,FxUcode)` is observed task input because the
task deliberately points into that area and reads relative fields there. The
routine reads some of those fields; all seven 16-bit slots have not been shown
to be consumed by this path. Their exact schema and relationship to ESP
registers are unresolved. They must not be appended to the six-byte
instruction stream or treated as GPR initialization without further evidence.

## Other effect-local initialization routines

Static entry-point inspection found nonzero `FxInitCode` pointers for several
objects (for example `44DDL+CH+REV` at `0x0e36`, `44DLYLFO+REV` at `0x0b40`,
and `44EQ+DDL+CHO` at `0x0b16`). These routines contain 68k calls and accesses
to absolute addresses such as `$8000`/`$8030`. The local object alone does not
resolve those OS service addresses to ES5510 transactions. A zero
`FxInitCode` means the object has no object-local init entry; it does not mean
the DSP receives no generic initialization from the operating system.

## Loader evidence boundary

The public guide says the EFE structure has host-side init/task code and an
ESP code region pointed to by `FxUcode`. The private objects conform to that
structure. The previous OpenASR V3.50 report documents combined host commits
and readback but lacks the raw instruction/GPR arrays and is not tied to these
1.61/3.53 objects. No evidence presently joins this object-local routine to a
specific ES5510 commit index or proves which candidate bytes the ASR OS
uploads.

## Next observation

Run OpenASR with a matching ASR OS image and required boot ROM pair, select
this exact effect, and record all ES5510 host writes, commit indices, readback,
GPR values, host control, HALT/RUN, and serial configuration. In parallel,
resolve the `$801e` service target from a correctly mapped running OS image.
The hardware/object capture must associate the selected EFE object hash with
the recorded instruction sequence before claiming the `FxUcode` interval is
the executed program.

References: unofficial [ESP1 Microcode Programming Guide](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/) (EFE header and task model); private ASR-10 OS 3.53 effect payloads (not redistributed); OpenASR prior runtime reports at commit `241f9331477b07b072330ca6ebd2d7a2e2cc832e` (secondary runtime evidence, not reproduced here).
