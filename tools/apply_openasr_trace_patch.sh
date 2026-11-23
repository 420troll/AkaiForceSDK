#!/bin/sh
# Apply the reproducible ES5510/MAP trace patch to the pinned OpenASR checkout.
set -eu
ROOT=${1:?usage: sh tools/apply_openasr_trace_patch.sh /path/to/OpenASR-checkout}
EXPECTED=241f9331477b07b072330ca6ebd2d7a2e2cc832e
REPO=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ACTUAL=$(git -C "$ROOT" rev-parse HEAD)
if [ "$ACTUAL" != "$EXPECTED" ]; then
  echo "Refusing OpenASR revision $ACTUAL; expected $EXPECTED" >&2
  exit 2
fi
PATCH="$REPO/tools/patches/openasr-es5510-device-trace.patch"
if ! git -C "$ROOT" diff --quiet; then
  echo "Refusing checkout with existing tracked modifications" >&2
  exit 2
fi
if ! git -C "$ROOT" diff --cached --quiet; then
  echo "Refusing checkout with staged changes" >&2
  exit 2
fi
patch -d "$ROOT" -p1 --dry-run < "$PATCH"
patch -d "$ROOT" -p1 < "$PATCH"
echo "Applied trace patch to OpenASR $EXPECTED"
