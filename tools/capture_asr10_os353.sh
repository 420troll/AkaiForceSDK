#!/bin/sh
# Start the instrumented OpenASR runtime with passive CPU taps plus direct
# ES5510 host-device/PA capture. All generated output is outside Git.
# All generated output is outside Git. The source OS image is never opened by
# MAME: a hash-checked, read-only copy is mounted for the run.
set -eu

REPO=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MAME=${OPENASR_BIN:-/home/gamer/OpenASR-run/mame-es5510-trace}
ROMDIR=${OPENASR_ROMDIR:-/home/gamer/OpenASR-run/roms}
OS353=${OPENASR_OS353:-/home/gamer/OpenASR-run/floppies/os353.img}
CAPTURE_ROOT=${T551_CAPTURE_ROOT:-/home/gamer/OpenASR-run/captures}

for f in "$MAME" "$OS353" \
  "$ROMDIR/asr10booth/asr-648c-lo-1.5b.bin" \
  "$ROMDIR/asr10booth/asr-65e0-hi-1.5b.bin" \
  "$REPO/tools/asr10_es5510_trace.lua"; do
  if [ ! -r "$f" ]; then echo "Missing required readable file: $f" >&2; exit 2; fi
done

STAMP=$(date -u +%Y%m%dT%H%M%SZ)
RUN="$CAPTURE_ROOT/os353-es5510-$STAMP"
if [ -e "$RUN" ]; then echo "Capture directory already exists: $RUN" >&2; exit 2; fi
mkdir -p "$RUN" "$RUN/cfg" "$RUN/nvram" "$RUN/state" "$RUN/snap" \
  "$RUN/diff" "$RUN/input"
cp "$OS353" "$RUN/os353-working-copy.img"
chmod 0444 "$RUN/os353-working-copy.img"

MAME_VERSION=$($MAME -version 2>&1 | head -1)
MAME_SHA=$(sha256sum "$MAME" | awk '{print $1}')
TRACE_SHA=$(sha256sum "$REPO/tools/asr10_es5510_trace.lua" | awk '{print $1}')
PATCH_SHA=$(sha256sum "$REPO/tools/patches/openasr-es5510-device-trace.patch" | awk '{print $1}')
OS_SHA=$(sha256sum "$OS353" | awk '{print $1}')
ROM_LO_SHA=$(sha256sum "$ROMDIR/asr10booth/asr-648c-lo-1.5b.bin" | awk '{print $1}')
ROM_HI_SHA=$(sha256sum "$ROMDIR/asr10booth/asr-65e0-hi-1.5b.bin" | awk '{print $1}')
COPY_SHA=$(sha256sum "$RUN/os353-working-copy.img" | awk '{print $1}')
if [ "$OS_SHA" != "$COPY_SHA" ]; then echo "OS working copy hash mismatch" >&2; exit 2; fi

python3 "$REPO/tools/asr10_disk_catalog.py" "$OS353" \
  --extract-dir "$RUN/reference-effects" > "$RUN/os353-catalog.json"
python3 "$REPO/tools/asr10_efe_inspect.py" \
  "$RUN/reference-effects/01_44DDL+CHORUS.payload.bin" > "$RUN/44DDL+CHORUS-candidate-metadata.json"

cat > "$RUN/manifest.txt" <<EOF
Capture started UTC: $STAMP
Runtime executable: $MAME
Runtime version: $MAME_VERSION
Runtime executable SHA-256: $MAME_SHA
MAME/OpenASR source commit: not embedded/identified by this installed binary
ASR-10 low boot ROM SHA-256: $ROM_LO_SHA
ASR-10 high boot ROM SHA-256: $ROM_HI_SHA
OS 3.53 source image SHA-256: $OS_SHA
OS 3.53 read-only run copy SHA-256: $COPY_SHA
Lua recorder SHA-256: $TRACE_SHA
Mounted floppy: read-only private copy of OS 3.53 at floppy 1
Trace: openasr.log (raw stdout/stderr, Lua CPU-space taps, T551ESP device-boundary records, and T551MAP BAR records)
OpenASR source revision: 241f9331477b07b072330ca6ebd2d7a2e2cc832e
Instrumentation patch SHA-256: $PATCH_SHA
The original ROM and OS files are opened read-only and never written by this script.
EOF

export T551_OPENASR_ID="241f9331477b07b072330ca6ebd2d7a2e2cc832e+trace-patch:$PATCH_SHA"
export T551_MAME_ID="$MAME_VERSION OpenASR-241f9331477b07b072330ca6ebd2d7a2e2cc832e"
export T551_ES5510_SOURCE_ID="openasr-es5510-device-trace-patch-sha256:$PATCH_SHA"
echo "Starting instrumented OpenASR capture. Exit the emulator normally to finalize private files."
echo "Private capture directory: $RUN"
set +e
(
  cd "$RUN"
  "$MAME" asr10booth \
    -rompath "$ROMDIR" \
    -flop1 "$RUN/os353-working-copy.img" \
    -autoboot_script "$REPO/tools/asr10_es5510_trace.lua" \
    -skip_gameinfo -noreadconfig -nowriteconfig -log -oslog \
    -window -nomaximize -nomouse -mouse_device none -ui_mouse \
    -cfg_directory "$RUN/cfg" -nvram_directory "$RUN/nvram" \
    -state_directory "$RUN/state" -snapshot_directory "$RUN/snap" \
    -diff_directory "$RUN/diff" -input_directory "$RUN/input" \
    "$@" \
    > "$RUN/openasr.log" 2>&1
)
STATUS=$?
set -e
echo "OpenASR exited (status $STATUS). Capture directory: $RUN"
printf 'Emulator exit status: %s\n' "$STATUS" >> "$RUN/manifest.txt"
printf 'Capture ended UTC: %s\n' "$(date -u +%Y%m%dT%H%M%SZ)" >> "$RUN/manifest.txt"

python3 "$REPO/tools/asr10_openasr_trace_normalize.py" \
  "$RUN/openasr.log" "$RUN/device-normalized.tap" "$RUN/device-audit.json"
python3 "$REPO/tools/asr10_capture_reconstruct.py" \
  "$RUN/device-normalized.tap" "$RUN/capture.json"
python3 - "$RUN/capture.json" "$RUN/device-audit.json" <<'PY'
import json, sys
capture_path, audit_path = sys.argv[1:]
capture = json.load(open(capture_path, encoding="utf-8"))
audit = json.load(open(audit_path, encoding="utf-8"))
instructions = {x["index"]: x["word_hex"] for x in capture.get("instructions", [])}
gprs = {x["register"]: x["value_hex"] for x in capture.get("gprs", [])}
last_instruction = {}
last_gpr = {}
for commit in audit.get("semantic_commits", []):
    if "instruction_hex" in commit: last_instruction[commit["index"]] = commit["instruction_hex"]
    if "gpr_hex" in commit: last_gpr[commit["index"]] = commit["gpr_hex"]
instruction_mismatches = [i for i, v in last_instruction.items() if instructions.get(i) != v]
gpr_mismatches = [i for i, v in last_gpr.items() if gprs.get(i) != v]
capture["device_boundary_event_count"] = audit["device_host_event_count"]
capture["device_boundary_host_read_count"] = audit["host_read_count"]
capture["device_boundary_host_write_count"] = audit["host_write_count"]
capture["device_boundary_semantic_commits"] = len(audit["semantic_commits"])
capture["device_boundary_synthetic_test_event_count"] = audit["synthetic_test_event_count"]
capture["device_boundary_final_state_crosscheck"] = {
    "instruction_indices_compared": len(last_instruction),
    "gpr_indices_compared": len(last_gpr),
    "instruction_mismatch_indices": instruction_mismatches,
    "gpr_mismatch_indices": gpr_mismatches,
    "pass": not instruction_mismatches and not gpr_mismatches,
}
if instruction_mismatches or gpr_mismatches:
    capture["capture_status"] = "incomplete"
    capture["effect_load_evidence"] = False
    capture["validated_factory_program"] = False
    capture.setdefault("limitations", []).append(
        "Direct ES5510 semantic commit records did not match reconstructed byte-latch state; capture is disqualified."
    )
with open(capture_path, "w", encoding="utf-8") as f:
    json.dump(capture, f, indent=2)
    f.write("\n")
PY
python3 "$REPO/tools/asr10_capture_disassemble.py" \
  "$RUN/capture.json" > "$RUN/candidate-disassembly.txt" \
  2> "$RUN/disassembly.err" || true
echo "Private capture finished: $RUN"
echo "Review manifest.txt, openasr.log, capture.json, and candidate-disassembly.txt."
exit "$STATUS"
