# ASR-10 ES5510 Capture Recorder Fix

## Status

This milestone adds a recorder at the OpenASR ES5510 device boundary and an
automated post-remap check. It is an observation of OpenASR's model, not a
physical bus capture and not, by itself, a recovered factory program.

## Diagnosis

The old capture ended after 158 CPU program-space tap records at emulated time
0.0014885 seconds. This did **not** mean the GUI session or emulation stopped:
the same raw log contains PA4 callback activity through roughly 307 emulated
seconds. The original Lua taps watched the MC68302 program address space.

At pinned OpenASR commit
`241f9331477b07b072330ca6ebd2d7a2e2cc832e`, the fixed ES5510 select/commit
handlers call `m_es5510_host->host_w(host_offset, data)` directly. These
transactions do not pass through the CPU program-space handler and are
therefore invisible to CPU address-space taps. The `$FC6000` MC68302 internal
window installed by `install_internal_window()` is separate from the static
ES5510 `$FC3000–$FC303F` map; the source alone does not prove that remapping
invalidates Lua taps. The new `T551MAP` trace plus a controlled post-remap
write tests that independently.

The previous `0x0000, 0x0101, 0x0202, ...` commit values were the 16-bit CPU
tap's active-low byte-lane representation, not the byte consumed by the
ES5510. OpenASR's `$FC30xx` mapping uses the low byte lane (`umask16(0x00ff)`).
The direct device log records the actual `uint8_t` value passed to `host_w`,
which should show `00, 01, 02, ...` for the same test sequence. `T551ESP`
semantic commit records independently report the committed 48-bit instruction
and 24-bit GPR value.

## Recorder and regression design

`tools/patches/openasr-es5510-device-trace.patch` instruments the pinned
OpenASR source at three points:

- `es5510_device::host_r/host_w`: actual byte reads/writes at the processor
  boundary, including all six instruction latch bytes and GPR/DOL/DADR ports.
- ES5510 commit handlers: committed instruction, GPR, combined instruction/GPR,
  external delay RAM write/clear events.
- ASR-10 PA4 callback and MC68302 BAR window install: modeled HALT/RUN and
  address-map relocation with emulated timestamps.

The patch is passive: it adds logs only and does not change the transfer
values or CPU/DSP state. `tools/apply_openasr_trace_patch.sh` checks the exact
OpenASR revision and a clean source tree before applying it. Build the patched
MAME/OpenASR executable, then point the GUI capture launcher at it with
`OPENASR_BIN`.

The Lua recorder retains its CPU-space taps as a cross-check and accepts an
opt-in synthetic write for regression only. The normalizer refuses CPU-only
logs, writes normalized output and raw transaction audit files only outside
the repository, and marks synthetic records. The reconstructor cannot mark a
capture as a factory program. Final semantic commit values are cross-checked
against byte-latch reconstruction; any mismatch disqualifies the capture.

## Validation

The automated ASR-10 tests cover direct-device event parsing, low-byte values,
PA4 events, synthetic labeling, CPU-only rejection, and the rule that a PA4
transition without a complete ordered upload is not effect-load evidence.
Shell syntax and `git diff --check` are also required.

The runtime regression should use the private OS 3.53 image and verified boot
ROMs, run past the BAR relocation, and inject one marked `$FC3030 = $5A` test
write after eight emulated seconds. Pass criteria are:

1. A `T551MAP` record reports the installed BAR/base/end and the ES5510 window
   remains outside it.
2. Exactly one direct `T551ESP W` record has device offset `0x18`, byte `0x5A`,
   and a timestamp at or after the injection time.
3. The normalized audit reports one synthetic event; it is excluded from
   effect-load qualification.
4. If the Lua CPU tap also observes that deliberate write, it appears exactly
   once. If it does not, the device-boundary event still proves the recorder
   remains active after remapping.
5. Device-boundary semantic commits equal the reconstructed final instruction
   and GPR state.

Runtime results, executable SHA-256 and the private regression directory are
recorded in the project handoff after the patched build is run. A complete
interactive effect-load capture is still needed after this regression passes.

## Existing capture

The 2026-11-23 capture at
`/home/gamer/OpenASR-run/captures/os353-es5510-20261123T030621Z` cannot be
recovered into a factory program. It contains CPU taps and PA4 diagnostics,
but lacks the device-boundary host transactions omitted by the old observer.
The original 158 events remain useful as boot/remap context only.

## Scope and limitations

The log observes the OpenASR host interface implementation, not electrical
timing or physical ES5510 pins. It does not prove the core's ES5510 arithmetic
is hardware-accurate. Program qualification still requires a coherent ordered
upload, valid instruction addresses and data, required GPR/special-register
state, HALT/upload/RUN ordering, a loaded-state indication, and independent
review. Proprietary ROMs, disk images, effect objects, raw traces, and factory
program bytes must remain private.
