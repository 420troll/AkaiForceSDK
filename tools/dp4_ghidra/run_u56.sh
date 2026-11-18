#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 U56_IMAGE PRIVATE_WORK_DIR PROJECT_NAME" >&2
  exit 2
fi
rom=$(realpath "$1")
work=$(realpath -m "$2")
project=$3
if [[ ! -f "$rom" || ! "$project" =~ ^[A-Za-z0-9._-]+$ ]]; then
  echo "ROM must exist and project name must be a simple filename" >&2
  exit 2
fi
: "${GHIDRA_INSTALL_DIR:?set GHIDRA_INSTALL_DIR to the isolated Ghidra installation}"
: "${JAVA_HOME:?set JAVA_HOME to a compatible JDK}"
headless="$GHIDRA_INSTALL_DIR/support/analyzeHeadless"
[[ -x "$headless" ]] || { echo "not executable: $headless" >&2; exit 2; }
mkdir -p "$work"
exec "$headless" "$work" "$project" \
  -import "$rom" -loader BinaryLoader -loader-baseAddr 0x8000 \
  -processor MC6801:BE:16:default \
  -scriptPath "$(dirname "$(realpath "$0")")" \
  -preScript Dp4MarkReset.java 0xfffe \
  -postScript Dp4Report.java "$work/$project-report.txt"
