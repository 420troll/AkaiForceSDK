#!/bin/sh
# Start the installed OpenASR 0.288 runtime with passive ES5510 bus/PA capture.
# All generated output is outside Git. The source OS image is never opened by
# MAME: a hash-checked, read-only copy is mounted for the run.
set -eu

REPO=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MAME=${OPENASR_BIN:-/home/gamer/OpenASR-run/mame}
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
Trace: openasr.log (raw stdout/stderr, Lua bus taps, and T551_PA callback records)
The original ROM and OS files are opened read-only and never written by this script.
EOF

export T551_OPENASR_ID="installed-binary-sha256:$MAME_SHA"
export T551_MAME_ID="$MAME_VERSION-source-unpinned"
export T551_ES5510_SOURCE_ID="source-unpinned-binary-sha256:$MAME_SHA"
echo "Starting OpenASR capture. Exit the emulator normally to finalize private files."
echo "Private capture directory: $RUN"
set +e
(
  cd "$RUN"
  "$MAME" asr10booth \
    -rompath "$ROMDIR" \
    -flop1 "$RUN/os353-working-copy.img" \
    -autoboot_script "$REPO/tools/asr10_es5510_trace.lua" \
    -skip_gameinfo -noreadconfig -nowriteconfig -log -oslog \
    -cfg_directory "$RUN/cfg" -nvram_directory "$RUN/nvram" \
    -state_directory "$RUN/state" -snapshot_directory "$RUN/snap" \
    -diff_directory "$RUN/diff" -input_directory "$RUN/input" \
    > "$RUN/openasr.log" 2>&1
)
STATUS=$?
set -e
printf 'Emulator exit status: %s\n' "$STATUS" >> "$RUN/manifest.txt"
printf 'Capture ended UTC: %s\n' "$(date -u +%Y%m%dT%H%M%SZ)" >> "$RUN/manifest.txt"

python3 "$REPO/tools/asr10_capture_reconstruct.py" \
  "$RUN/openasr.log" "$RUN/capture.json" || true
python3 "$REPO/tools/asr10_capture_disassemble.py" \
  "$RUN/capture.json" > "$RUN/candidate-disassembly.txt" \
  2> "$RUN/disassembly.err" || true
echo "Private capture finished: $RUN"
echo "Review manifest.txt, openasr.log, capture.json, and candidate-disassembly.txt."
exit "$STATUS"
