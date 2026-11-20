# DP/4 factory microprogram extraction feasibility

Branch: `issue11-dp4-rom-forensics`
Baseline requested: `c06ac8992f1e4ff3d20dab0f51f901e5637b543f`
Analysis date: 2026-11-19
ROMs and full analysis outputs: private under `/tmp/dp4-rom-forensics`; none are committed.

Current environment availability, Ghidra/JDK setup, and the resumed evidence
boundary as of 2026-11-20 are recorded in
[`DP4_SESSION_RESUME_2026-11-20.md`](DP4_SESSION_RESUME_2026-11-20.md). The
earlier static findings below were preserved; their private binaries were not
available for rehashing in that resumed session.

## Decision

**One CPU-to-U54 read path is now verified in software across both ROM revisions. No CPU-to-ES5510 instruction or GPR write has yet been identified, so no factory microprogram can yet be claimed as recovered.** Software-only extraction remains technically plausible: the base board, CPU family, U56 window, U54 bank wiring, and U54 revision-tag access are now established. It is not demonstrated end-to-end because the U56-to-ESP execution path depends on runtime pointers and an incompletely reduced ESP address decoder.

The bounded cross-version Ghidra pass has now resolved the bank-latch routines and per-slot bank table, but it still cannot connect U54 algorithm data to an ES5510 host transaction. A computed call into the U54 window is a runtime-object candidate, not a verified U54 fetch. **Stop further speculative software exploration and make a synchronized U54/ESP hardware capture the next milestone.** Do not build a full 6803 emulator from guessed addresses or pointer state.

## Evidence table

| Question | Evidence | Finding / confidence |
|---|---|---|
| Is U52 really a 6803-family host? | Base DP/4 Main Board schematic 4010015001 Rev J, sheet 2 labels U52 `6803`; the service manual independently calls out “6803 (U52)”. The DP/4+ 63803 is a different model and was not used for this conclusion. | **Verified.** Do not analyze this as a 68030 or assume the DP/4+ CPU. |
| Which 6803 execution mode is used? | Motorola's MC6801/03 manual says MC6803 supports modes 2 and 3. In mode 2, the internal register block is at `$0000–$001F`; in mode 3 it is at `$D000–$D01F`. Both U56 versions write the RAM Control Register at `$0014` and initialize port registers `$0000–$0002`. The startup sequence sets RAM-enable bit 6 (`0x40`). | **Mode 2 strongly supported, strap-level confirmation absent.** The low-register accesses match mode 2. The board mode pins have not been measured at reset, so retain the mode-3/external-alias possibility as a low-confidence alternative. |
| Is `$8000–$FFFF` really U56? | Rev J shows U56 is a 27256 with A0–A14 on the U52 bus and CE qualified by A15. The 32 KiB images' word at file offset `0x7FFE` is the reset vector `0x84E4`, and U52's external reset vector is `$FFFE–$FFFF`. | **Verified mapping.** CPU `$FFFE` maps to U56 file offset `0x7FFE`; reset target `$84E4` maps to file offset `0x04E4`. |
| Do all interrupt vectors fit the proposed map? | Motorola MC6801/03 vector table defines `$FFF0–$FFFF`; the table below decodes the private ROM images as big-endian target words. Every target lies in the schematic-backed U56 window. | **Verified mapping and internally consistent vector targets.** |
| Does boot control flow resemble a functioning 6803 program? | Ghidra 12.1.4 with MC6801 extension decodes 1,323 instructions/55 functions in v1.06 and 1,326/55 in v1.15. Both enter at `$84E4`, set SP to `$510C`, access the low internal register block, call coherent setup code, and follow direct call/return control flow. The 6800-only decoder reaches only 59 instructions/2 functions. | **Strongly consistent.** The external SRAM chip U58 is 32 KiB and its bus wiring supports external RAM, but its exact chip-enable range is still unresolved. `$510C` is outside the MCU's internal RAM and is used as the stack; exact physical SRAM select is inferred from the running firmware, not reduced from the schematic. |
| What did Ghidra call the `$8000–$FFFF` import block? | The earlier private report called it `MEMORY_BLOCK RAM 8000..ffff`. The updated `Dp4Report.java` labels such entries `GHIDRA_IMPORT_BLOCK` and explicitly says this is a loader mapping, not a board memory classification. | **Analysis label only.** Do not use the old generic `RAM` name as evidence of the hardware memory map. |
| Is there a real CPU-to-U54 relationship? | Both U56 images select bank 9 through the `$0600` bank/latch write, read the word at CPU `$2000`, and compare it to the revision-specific tag. U54 bank 9 begins at file offset `9 × 0x2000 = 0x12000`, where the two bytes match the expected value in each corresponding revision. | **Verified cross-revision tag access.** Confirms U54 bank/window mapping for this read, not instruction transfer. The private-input `verify_u54_tag.py` reproduces it and reports hashes, offsets, and tag only. |
| What is `$0600` doing? | U56 calls the bank helper immediately before the U54 tag read. The helper writes bank value 9 to `$0600`; schematic U57 outputs B0–B3 drive U54 A13–A16, and its upper outputs drive the four ESP HALT lines. | **Strong bank-latch interpretation.** The exact U57 latch edge and complete `$0600` decode are not yet traced as a Boolean/address equation or observed on a live bus. |
| How are runtime bank values selected? | The `$00B9` selector uses a four-entry U56 descriptor pointer table indexed by `$00B8 & 3`; `$00B8 & $20` chooses descriptor offset +4 or +5. Both revisions contain matching bank-value pairs `(0,0x0B)`, `(0,0x0B)`, `(1,0x0C)`, `(2,0x0D)`. Separate routines write the low nibble with the `$0092` high-nibble shadow to `$0600`. | **Verified bank-selection data flow.** Interpreting the slots as DSP A/B/C/D and the paired values as configuration/program banks is plausible but not demonstrated by a bus trace. See [banked access trace](DP4_BANKED_ACCESS_TRACE.md). |
| Do the selector's paired U54 banks contain related data? | Aligned byte comparisons in both ROMs find 3,670/8,192 equal positions for banks 0/`0x0B`, 3,894 for 1/`0x0C`, and 3,395 for 2/`0x0D`; hundreds of matching positions per pair are nonzero/non-`0xFF`. | **Repeated content confirmed, semantics unresolved.** This does not validate code, coefficient, or configuration layouts; see [banked access trace](DP4_BANKED_ACCESS_TRACE.md). |
| Is there an indirect U54-window execution candidate? | Ghidra reports a computed call edge at `$82C9` to `$30EC` in both revisions. The instruction is `JSR 0,X`; X is loaded from a runtime object's first word. The direct caller is in the MIDI/SysEx parser path. | **Candidate only.** Object origin and active U54 bank are unresolved; the edge does not prove a U54 instruction fetch or any ES5510 transaction. |
| Is `$1800/$1801` the ESP port? | U56 writes `$1801`, polls status `$1800`; Rev J maps these accesses to U59 6850 ACIA, whose status/data register behavior matches the firmware. U46 Y4 shares the low A9–A11 pattern at `$1800`, but its additional enables are inactive for the ACIA mapping. | **No. Verified ACIA access, not an ESP access.** This eliminates the earlier misleading candidate sender. |
| Are there four independent ESP paths? | Rev J shows four ES5510s and U46 Y4–Y7 as ESP1–ESP4 selects, with independent U57 HALT1–HALT4 lines. | **Four hardware host paths verified.** Their absolute CPU-address ranges still require full U46/U48 high-address enable equations. |
| Has instruction/GPR transfer been found? | Read-only Ghidra analysis found 433 data references in v1.06 and 435 in v1.15. The expanded pass resolves all `$0600` writes, the per-slot bank selectors, and the computed `$82C9 -> $30EC` edge, but no instruction/GPR operation is tied to a selected ESP host address. | **Not identified.** No six-byte instruction latch plus commit, GPR write, or verified equivalent has been tied to an ESP select. |
| Do U54 label records provide program directories? | The existing scanner verifies three stable label-bearing geometries (50 × 51, 50 × 87, 50 × 158 bytes); all 50 records in each region contain nonzero bytes after the name terminator, and the records are identical across v1.06/v1.15. The new bank-9 revision tag is at `0x12000`, outside those records. | **The records contain payload/configuration bytes beyond names, but no pointer, size, index, or program-boundary schema is accepted.** Simple fixed-offset pointer-to-record-start candidates are inconsistent across the three schemas; numeric values alone are ambiguous due to padding and data bytes. No rejected raw-48-bit packing hypothesis was rerun. |
| Does MAME provide the missing transfer? | The pinned MAME ES5510 source implements the device-local host port and instruction/GPR latches, but the MAME Ensoniq driver set has no complete DP/4 machine driver. | **No.** It can decode a future transaction; it does not execute this U56/U54 host system. |
| Are REA tools available in Codex now? | The active Codex `ALL_TOOLS` registry has no REA entries. Existing local REA registration/self-check is not repeated. Ghidra 12.1.4 and the MC6801 extension are available privately and were used. | **REA MCP unavailable to this active session; Ghidra available and used.** |

### Interrupt vectors

The vector table is big-endian. The parenthesized target is the corresponding U56 file offset (`CPU target - 0x8000`).

| Vector CPU address | Meaning (Motorola table) | v1.06 target | v1.15 target |
|---|---|---|---|
| `$FFF0` | SCI | `$B903` (`0x3903`) | `$B91D` (`0x391D`) |
| `$FFF2` | Timer overflow | `$84E4` (`0x04E4`) | `$84E4` (`0x04E4`) |
| `$FFF4` | Output compare | `$838A` (`0x038A`) | `$838A` (`0x038A`) |
| `$FFF6` | Input capture | `$ADC7` (`0x2DC7`) | `$ADE5` (`0x2DE5`) |
| `$FFF8` | IRQ1 | `$8693` (`0x0693`) | `$8699` (`0x0699`) |
| `$FFFA` | SWI | `$84CD` (`0x04CD`) | `$84CD` (`0x04CD`) |
| `$FFFC` | NMI | `$84E4` (`0x04E4`) | `$84E4` (`0x04E4`) |
| `$FFFE` | Reset | `$84E4` (`0x04E4`) | `$84E4` (`0x04E4`) |

Both ROM revisions place all vector words within U56 and share the same reset target. This consistency is independent of the U54 tag check.

## U54 tag correlation

The Ghidra cross-reference and private verifier establish these exact relationships:

| Revision | U56 file offset | U56 CPU location | CPU read | Expected tag | U54 bank | U54 file offset | Match |
|---|---:|---:|---:|---:|---:|---:|---|
| v1.06 | `0x0572` | `$8572` | `$2000` | `0x0106` | 9 | `0x12000` | yes |
| v1.15 | `0x0575` | `$8575` | `$2000` | `0x010F` | 9 | `0x12000` | yes |

The tag check follows the bank helper in each reset flow. U54 wiring connects the four U57 bank bits to ROM A13–A16, while U54 A0–A12 follow CPU A0–A12. Therefore bank 9 plus CPU `$2000`'s zero low 13 bits selects physical U54 offset `0x12000`. The tag changes with the OS revision and its U54 counterpart changes at the same physical location. This is not a coincidental algorithm name or a raw-stream guess; it is direct evidence that U56 accesses a specific banked U54 word.

The 50-record label structures have fixed geometry and nonzero record tails after their visible strings, so they are not just padded name lists. However, candidate 16-bit fixed-offset pointers to other record starts do not yield one consistent field across the algorithm-name, effect/preset-label, and configuration-label regions. Low numeric values are common in binary fields and padding, so they are not accepted as indexes without a firmware cross-reference. The only confirmed U54 structure-to-firmware correlation found in this pass is the bank-9 revision tag. The firmware selection path still loads function pointers from runtime objects; no label-table record is yet connected to one of the four ESP write paths.

The verifier command, using private images, is:

```sh
python3 tools/dp4_ghidra/verify_u54_tag.py \
  /private/DP4_V106_os.bin /private/DP4_V106_ucode.bin
python3 tools/dp4_ghidra/verify_u54_tag.py \
  /private/DP4_V115_os.bin /private/DP4_V115_ucode.bin
```

It emits only SHA-256 hashes, U56 file/CPU offsets, U54 bank/offset, a two-byte version tag, and match status. Keep that output private or publish only the non-proprietary result table above.

The read-only Ghidra direct-reference audit is reproducible against each existing private project (ROMs and projects remain outside the repository):

```sh
export GHIDRA_INSTALL_DIR=/private/ghidra_12.1.4_PUBLIC
export JAVA_HOME=/private/jdk-21
"$GHIDRA_INSTALL_DIR/support/analyzeHeadless" \
  /private/ghidra-projects/schematic-confirmed v115-u56 \
  -process DP4_V115_os.bin -readOnly \
  -scriptPath "$PWD/tools/dp4_ghidra" \
  -postScript Dp4MemoryAudit.java /private/v115-memory-audit.txt
```

Use project `v106-u56`, the matching program name, and a separate output file for v1.06. The audit emits direct source/target references only. Review its output locally and publish aggregate counts and conclusions, not raw reports or disassembly excerpts.

## Approaches and feasibility

| Approach | Known prerequisites | Missing information | Expected evidence | Main risk | Next experiment |
|---|---|---|---|---|---|
| **A. Static firmware/U54 analysis** | Correct 6803-family decoder; confirmed U56 window; U54 bank wiring; validated bank-9 tag check; bank-latch and four-slot selector audit. | Complete U54 CE/alias equation; U46/U48 ESP ranges; runtime object/function-pointer values; validated instruction/GPR/table transfer; U54 program schema. | Identified loader routine, U54 source interval, exact bank sequence, and destination register writes from code/data flow. | The remaining control/data flow is indirect/runtime state; further static mining risks substituting assumptions for observations. | **Bounded pass complete. Stop here and capture synchronized U54/ESP buses.** |
| **B. Minimal 6803 execution/emulation** | U56 image/mapping; likely mode 2; internal register semantics; U54 bank latch path; U59 ACIA identified. | Complete SRAM/I/O/ESP address decode; reset strap certainty; clock/timer/input behavior; U57 edge semantics; runtime user/peripheral state needed to reach load; exact ES interface read/write timing. | Executed CPU cycles showing U54 reads correlated with ES5510 host writes; optionally a full serialized microprogram captured in software. | A guessed peripheral map or guessed pointer state could create plausible but false traces; full-system boot includes unrelated UI/timer behavior. | **Defer.** Reconsider only after a hardware capture establishes the actual selected ESP transaction and loader entry/state. |
| **C. Physical DP/4 bus capture** | Base Rev J schematic; U52/U54/U57 and ES5510 components; tagged v1.06/v1.15 reference points; known host-port semantics from ES5510 spec/MAME. | A safe known-good base DP/4 board, exact board/chip revisions, buffered probes and sufficient synchronized channels; live signal pinout must be checked. | Direct U54 address/data/bank read sequence time-correlated with selected ESP address/data/AS/R/W/CS/HALT transactions at boot and algorithm change. | Probe loading, electrical damage, clock alignment/channel count, and capture interpreted using wrong silicon revision. | **Most decisive fallback.** Capture reset/boot and select a simple one-unit delay, then adjust one parameter. Capture U54 bus + U57 bank + one selected ESP host bus with common trigger. |

## Recommended next engineering milestone

**C: synchronized hardware capture.** The bounded cross-version static pass has resolved the bank selector and compared its data in both ROM revisions, but it stops at runtime object state and the unreduced ESP address decode. Do not spend another cycle on speculative emulation or additional U54 packing scans. Capture U54 address/data/bank and one or more selected ES5510 host buses on a common timebase. The exact capture checklist is below and in [DP4_BANKED_ACCESS_TRACE.md](DP4_BANKED_ACCESS_TRACE.md).

The shortest conclusive hardware capture must include, on a shared timebase:

- U54 A0–A12, D0–D7, CE and OE, plus U57 B0–B3 (so each byte maps to physical U54 offset).
- U52 multiplexed A/D0–A7, AS, E and R/W, plus the selected ESPn CS and HALTn and the ESP host bus/strobe signals (confirm pinout for the actual ES5510 revision).
- Reset and, where available, CPU A8–A15 and all four ESP selects/HALT lines.

A trace of only U54 reads cannot prove a DSP upload. A trace of only ESP writes cannot establish their U54 source. Synchronous source/destination observation is the minimum decisive bridge.

## Limitations

- The `$8000–$FFFF` U56 map is schematic-confirmed. The exact U58 SRAM select, full U54 CE equation, and absolute ESP CPU windows remain incomplete.
- The inferred 6803 mode 2 has strong firmware evidence but no reset-time mode-pin measurement.
- Ghidra's generic imported-memory block label `RAM` is not a hardware classification.
- The REA MCP server is not exposed in this active Codex registry. No REA installation/configuration was repeated. Ghidra headless analysis was used directly.
- No factory algorithm instruction word, GPR image, table, or waveform has been extracted or executed. The verified U54 tag is not a microprogram.
- ROM images, extracted bytes, full disassemblies, detailed Ghidra project reports, and schematic page images remain outside Git.
- No plugin, Force, or production DSP code was modified.

## Sources

- Ensoniq, *DP-4 PCB Schematic, Main Board*, drawing 4010015001 Rev J, sheet 2; [R-Massive technical archive](https://zine.r-massive.com/ensoniq-technical-documents-and-schematics/).
- Ensoniq, *DP/4 Service Manual*, printed pp. 6–7, 13, 19: [PDF](https://www.synfo.nl/servicemanuals/Ensoniq/ENSONIQ_DP-4_SERVICE_MANUAL.pdf).
- Motorola, *MC6801/MC6803 Technical Data*, memory maps, mode limits, internal register map, RAM Control Register, and vector table: [PDF](https://datasheets.chipdb.org/Motorola/mc6801_3.pdf).
- Ensoniq, *ESP ES5510 Specification Rev. 2.4*, host bus and HALT behavior: [PDF](https://gjcp.net/pdf/es5510.pdf).
- MAME ES5510 host-interface model at `8be57cdc5f44ab830dff44c06dd39f24190742d4`: [pinned source](https://github.com/mamedev/mame/blob/8be57cdc5f44ab830dff44c06dd39f24190742d4/src/devices/cpu/es5510/es5510.cpp).
