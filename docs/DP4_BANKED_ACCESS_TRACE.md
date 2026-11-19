# DP/4 banked U54 access trace

Branch: `issue11-dp4-rom-forensics`
Analysis base: `c0397c6b046e63e5adb781ebd16f00034118a2bd`
ROMs, Ghidra projects, and detailed reports: private under `/tmp/dp4-rom-forensics`.

## Result and stop decision

This pass resolved the firmware's bank-latch routines and the runtime bank-selection table across U56 v1.06 and v1.15. It confirms that U57's bank nibble is selected from per-slot data and that U54 is visible through the banked CPU window. It did **not** demonstrate that a particular U54 algorithm record or byte range is uploaded to an ES5510 instruction/GPR register.

Ghidra also reports a computed call at U56 CPU `$82C9` with a candidate target `$30EC`, inside the U54 CPU window. The call is `JSR 0,X`; X is loaded from a runtime object. Its only statically resolved caller in the examined images is in a MIDI/SysEx setup path. Neither the object field's origin nor the active U54 bank at that call was resolved. Treat `$30EC` as an unresolved banked-code candidate, not proof of execution from U54.

The requested stop condition is met: no static source-to-destination data flow connects U54 algorithm data to a selected ES5510 host transaction. Stop further speculative software emulation and make the next milestone a synchronous physical bus capture. A full DP/4 emulator is not justified by current evidence.

## Bank-selection routines

The base-board schematic connects U57 outputs B0–B3 to U54 A13–A16. U56's code writes the bank/latch at CPU `$0600`; the low nibble selects a U54 bank. The firmware also maintains a high-nibble shadow at `$0092` and updates CPU port register `$0002` as part of the latch operation. The exact electrical latch edge and full address decoder equation are not established by static code alone.

| Operation | v1.06 U56 CPU address | v1.15 U56 CPU address | Evidence |
|---|---:|---:|---|
| Reset bank helper entry | `$8702` | `$870A` | Each has one direct caller in reset/startup: `$856F` / `$8572`. |
| Literal bank setup | `LDAA #$09`; `ANDA #$0F` | Same | Stores the low bank nibble in `$0094`. |
| Latch write | `$870E: STAA $0600` | `$8716: STAA $0600` | Combines the bank nibble with `$0092 & $F0`; writes the result back to `$0092`. |
| Version-tag read | `$8572: LDX $2000` | `$8575: LDX $2000` | Compares against U54 bank 9 tag `0106` / `010F`; verified in the preceding handoff. |
| Return to bank zero | `$8581: STAA $0600` | `$8584: STAA $0600` | The accumulator is `$F0`, so the low bank nibble is zero after a successful tag check. |

The reset bank-9 helper has no other direct caller in either static call graph. Runtime bank changes use separate routines:

| Runtime source | v1.06 | v1.15 | Role supported by code |
|---|---|---|---|
| Bank from `$00B9` | `$BF2D` writes `$0600` at `$BF3C` | `$BF47` writes `$0600` at `$BF56` | Combines `($0092 & $F0)` with `$00B9`; also modifies low bits of port register `$0002`. |
| Bank from `$0093` / `$0094` | `$BF40` writes `$0600` at `$BF4F`, using `$0093` | `$BF5A` writes `$0600` at `$BF69`, using `$0094` | Combines the high-nibble shadow with the selected low bank value and updates port register `$0002`. |

All direct `$0600` writes found by the read-only Ghidra reference audit:

- v1.06: `$8581`, `$870E`, `$BF3C`, `$BF4F`.
- v1.15: `$84E4` (startup clear), `$8584`, `$8716`, `$BF56`, `$BF69`.

The dynamic `$00B9`-bank writers are called from a shared byte-processing loop (`$BD79` in v1.06; `$BD93` in v1.15). The loop reads two byte values through callback/timer wrappers, joins their nibbles, selects the bank, stores a byte through a runtime destination pointer, then advances that pointer. Static callers of the loop are `$BD4E` and `$BD6C` in v1.06, and `$BD68` and `$BD86` in v1.15. This path manipulates bank state but does not itself contain a direct read from `$2000–$3FFF` or an ES5510 host-register write.

## Four-slot descriptor table

Two related routines load `$00B9` from a table of four descriptor pointers. The index is `($00B8 & 3)`. If bit `$20` of `$00B8` is clear, the routine takes descriptor byte `+4`; if set, it takes byte `+5`.

| Revision | Pointer table CPU / file offset | Slot | Descriptor CPU / file offset | Byte `+4` | Byte `+5` |
|---|---:|---:|---:|---:|---:|
| v1.06 | `$CC93` / `0x4C93` | 0 | `$CCB6` / `0x4CB6` | `0` | `0x0B` |
| v1.06 | `$CC93` / `0x4C93` | 1 | `$CCAD` / `0x4CAD` | `0` | `0x0B` |
| v1.06 | `$CC93` / `0x4C93` | 2 | `$CCA4` / `0x4CA4` | `1` | `0x0C` |
| v1.06 | `$CC93` / `0x4C93` | 3 | `$CC9B` / `0x4C9B` | `2` | `0x0D` |
| v1.15 | `$CCAF` / `0x4CAF` | 0 | `$CCD2` / `0x4CD2` | `0` | `0x0B` |
| v1.15 | `$CCAF` / `0x4CAF` | 1 | `$CCC9` / `0x4CC9` | `0` | `0x0B` |
| v1.15 | `$CCAF` / `0x4CAF` | 2 | `$CCC0` / `0x4CC0` | `1` | `0x0C` |
| v1.15 | `$CCAF` / `0x4CAF` | 3 | `$CCB7` / `0x4CB7` | `2` | `0x0D` |

The values fit the four-bit bank field and are stable across revisions. Given the four ESPs and the `& 3` index, interpreting these as four DSP slots is **strongly suggestive, not proven**. Likewise, the two values per slot appear to select two bank sets, but their algorithm/configuration meaning is not established. Do not rename these as algorithm pointers.

The table is in U56, not one of the 50 U54 algorithm-name record tables. No field in the 50 U54 records has been linked by a U56 code reference or dynamic data flow to these selector values.

## U54-window references and computed call

The audit queried all known Ghidra references, including computed-flow edges, into the candidate `$2000–$3FFF` U54 window in both analyzed programs. It found:

| Reference | v1.06 | v1.15 | Interpretation |
|---|---|---|---|
| Absolute read `$2000` | `$8572` | `$8575` | Verified bank-9 revision-tag check. |
| Computed call destination `$30EC` | `$82C9` | `$82C9` | Ghidra's computed-flow reference from `JSR 0,X`; runtime pointer/bank unresolved. |

The `$82C9` routine reads a function pointer from the first word of an object and calls it. Its caller is in the `$BBxx–$BCxx` input parser in both revisions. That caller allocates/updates a runtime object before entering the callback manager. Static analysis does not establish that this object's first word came from U54; it also does not establish which bank is selected when the call occurs. The call could enter a U54-mapped address, but the Ghidra edge alone is not an observed CPU fetch or verified ROM mapping.

If `$30EC` is interpreted through the 8 KiB bank arithmetic, its low address within a bank is `0x10EC`. Candidate physical U54 offsets under the table's two bank choices include `0x010EC`, `0x170EC`, `0x030EC`, `0x190EC`, `0x050EC`, and `0x1B0EC`. The bank-0 candidate falls within the effect/preset label-bearing region and the bank-1 candidate falls within the configuration-label region; the paired high banks and bank 2 are not structurally decoded at this address. Since the bank at call time is unknown, none of these offsets can be selected as the callback's source. No instruction stream is claimed.

## Revision comparison of U54 banks

The U54 images are both `0x20000` bytes and are divided into sixteen `0x2000`-byte banks by U57 B0–B3. The following table reports only changed-byte counts; the ROM images and byte-level comparisons remain private.

| Bank | File start | v1.06 vs v1.15 changed bytes |
|---:|---:|---:|
| 0–4 | `0x00000–0x08000` | 0 in each bank |
| 5 | `0x0A000` | 2,239 |
| 6–8 | `0x0C000–0x10000` | 0 in each bank |
| 9 | `0x12000` | 1,106 |
| 10 | `0x14000` | 0 |
| 11 | `0x16000` | 5 |
| 12 | `0x18000` | 1 |
| 13–15 | `0x1A000–0x1E000` | 0 in each bank |

The bank-9 differences include the verified revision tag. The bank-5 cluster and remaining changed bytes are not yet assigned to algorithms. The 50 algorithm-name records in bank 0 and previously cataloged label-bearing tables are stable across the two U54 versions. Existing pointer-layout checks still fail to identify a consistent executable-program directory; the six rejected uniform raw 48-bit interpretations are not repeated here.

The selector's paired bank choices were also compared at aligned offsets. In **both** ROM revisions, banks 0/`0x0B`, 1/`0x0C`, and 2/`0x0D` have respectively 3,670, 3,894, and 3,395 byte positions equal out of 8,192; 964, 1,186, and 1,046 of those equal positions are non-`0x00`/non-`0xFF` bytes. These metrics show repeated content between each pair, but they do not establish which bytes are code, coefficients, templates, or padding, nor prove the selector uses the pairs as executable/configuration banks.

## What the pass did and did not establish

| Claim | Status |
|---|---|
| The reset helper selects bank 9 and checks the matching U54 revision tag | Verified in v1.06 and v1.15. |
| U56 has a four-slot selector table that supplies alternate low-nibble bank values | Verified from code references and table contents in both revisions. |
| The four slots are DSP A/B/C/D | Plausible from `B8 & 3` and the four-ESP hardware, but not verified by named code or a bus trace. |
| The alternate banks correspond to DSP microprograms | Plausible, but not verified. |
| A callback can target logical address `$30EC` in the U54 window | Ghidra reports this computed-call edge; runtime object contents and active bank are unknown. |
| A U54 algorithm record is read, unpacked, and written to an ES5510 instruction/GPR register | Not demonstrated. |
| A selected ESP host transaction is identified in U56 | Not demonstrated. The ESP address decode and runtime access path remain unresolved. |

## Required synchronized hardware capture

This is the next engineering milestone. Capture a real base DP/4 board (verify PCB revision, U52 and ES5510 package revisions first) during cold boot and when selecting/activating one simple algorithm on one unit. Use high-impedance/buffered probes appropriate for the board; do not attach probes directly until signal levels and package pins are checked.

Record on a common timebase:

1. **U54 source side:** U54 A0–A12, D0–D7, active-low CE and OE, and U57 B0–B3. This yields physical U54 offsets for each sampled read.
2. **U52 host bus:** A8–A15, multiplexed A/D0–A7, AS, E, and R/W. Decode the address phase before interpreting the data phase.
3. **All DSP destinations:** ESP1–ESP4 active-low chip selects and HALT1–HALT4; on at least the selected ES5510, the host data/address bus and its AS, R/W, and host-cycle strobe pins as wired on the actual board.
4. **Timing anchors:** CPU reset and the relevant U57 latch transition; trigger on reset release and retain the interval through initial DSP setup and algorithm selection.

The minimal decisive evidence is a single trace showing a U54 read (address, bank, byte) followed in the same transaction sequence by the selected ES5510 host writes. Capture the six instruction-byte writes and program commit/index operation, then GPR/configuration writes and HALT/run transitions. A U54-only trace cannot prove transfer; an ESP-only trace cannot prove source. If the instrument cannot capture every channel at once, use a common trigger and a validated split-bus analyzer arrangement that preserves cycle alignment. Do not infer absent channels from separate, unsynchronized power cycles.

## Reproduction and privacy

The checked-in Ghidra scripts `Dp4BankPathAudit.java` and `Dp4IndirectTargetAudit.java` accept existing private U56 Ghidra projects and write only to caller-selected output paths. The latter can emit disassembly/decompiler output; keep all such output private. REA MCP tools were not available in this Codex tool registry, so this pass used the already-installed Ghidra 12.1.4/MC6801 extension directly in read-only mode.

For example, with the private project already created and the ROM imported at `$8000`:

```sh
export GHIDRA_INSTALL_DIR=/private/ghidra_12.1.4_PUBLIC
export JAVA_HOME=/private/jdk-21
"$GHIDRA_INSTALL_DIR/support/analyzeHeadless" /private/ghidra-projects/schematic-confirmed v106-u56 \
  -process DP4_V106_os.bin -readOnly -scriptPath "$PWD/tools/dp4_ghidra" \
  -postScript Dp4BankPathAudit.java /private/v106-bank-path.txt 8702 870e
"$GHIDRA_INSTALL_DIR/support/analyzeHeadless" /private/ghidra-projects/schematic-confirmed v115-u56 \
  -process DP4_V115_os.bin -readOnly -scriptPath "$PWD/tools/dp4_ghidra" \
  -postScript Dp4BankPathAudit.java /private/v115-bank-path.txt 870a 8716
```

For the computed call and caller context, run `Dp4IndirectTargetAudit.java /private/indirect.txt 82c9` against each corresponding project. Never write those reports into the repository.

ROMs, extracted code, full reports, Ghidra projects, and schematic scans are not included in Git. Synthetic repository tests do not validate private firmware semantics; the two-revision Ghidra executions and private bank comparison are the evidence for this report.
