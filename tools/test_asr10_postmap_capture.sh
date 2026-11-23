#!/bin/sh
# End-to-end recorder regression. Requires private OpenASR ROMs/OS and Xvfb.
set -eu
REPO=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ROOT=${T551_CAPTURE_ROOT:-/home/gamer/OpenASR-run/captures/recorder-regression}
mkdir -p "$ROOT"
export OPENASR_BIN=${OPENASR_BIN:-/home/gamer/OpenASR-run/mame-es5510-trace}
export T551_CAPTURE_ROOT="$ROOT"
export T551_TEST_SYNTHETIC_SEC=8

xvfb-run -a sh "$REPO/tools/capture_asr10_os353.sh" \
  -video none -sound none -nothrottle -seconds_to_run 12

RUN=$(find "$ROOT" -mindepth 1 -maxdepth 1 -type d -name 'os353-es5510-*' -printf '%T@ %p\n' | sort -nr | head -1 | cut -d' ' -f2-)
python3 - "$RUN/device-audit.json" "$RUN/openasr.log" <<'PY'
import json, re, sys
audit_path, log_path = sys.argv[1:]
audit = json.load(open(audit_path, encoding="utf-8"))
synthetic = [e for e in audit["events"] if e["synthetic"]]
assert len(synthetic) == 1, f"expected exactly one marked device write, got {len(synthetic)}"
event = synthetic[0]
assert event["offset"] == 0x18 and event["data"] == 0x5a, event
assert float(event["time"]) >= 8.0, event
text = open(log_path, encoding="utf-8", errors="replace").read()
maps = re.findall(r"T551MAP\t[^\n]*BAR=([0-9A-Fa-f]+)\tBASE=([0-9A-Fa-f]+)\tEND=([0-9A-Fa-f]+)", text)
assert maps, "no MC68302 BAR mapping record"
assert all(int(base, 16) + 0xfff < 0xfc3000 or int(base, 16) > 0xfc303f
           for _, base, _ in maps), f"BAR window overlaps ES5510 host map: {maps}"
assert "T551CAP_TEST_SYNTHETIC_BEGIN" in text and "T551CAP_TEST_SYNTHETIC_END" in text
cpu_tap = re.findall(r"T551CAP\tv1\t\d+\t00FC3030\t[^\t]*\t[^\t]*\t[^\t]*\thost_w\t[^\n]*", text)
tap_result = "observed" if any("0000005A" in row for row in cpu_tap) else "not observed"
print(f"PASS: one post-remap synthetic ES5510 write at t={event['time']}s; BAR maps={len(maps)}; Lua CPU tap {tap_result}")
PY
echo "Private regression capture: $RUN"
