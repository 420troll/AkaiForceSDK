#!/bin/sh
set -eu
PLUGIN_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
REPO_ROOT=$(git -C "$PLUGIN_DIR" rev-parse --show-toplevel)
BASELINE=40d4e8ed4bf5ca522137478d1576675ae6b6cf70
WORKTREE="${TMPDIR:-/tmp}/5510-v01-state-test-$$"
OLD_SO="${TMPDIR:-/tmp}/5510-v01-state-test-$$.so"
NEW_SO="$PLUGIN_DIR/build/native/5510-single.so"
TEST_BIN="${TMPDIR:-/tmp}/5510-state-compat-test-$$"
cleanup() {
    git -C "$REPO_ROOT" worktree remove --force "$WORKTREE" >/dev/null 2>&1 || true
    rm -f "$OLD_SO" "$TEST_BIN"
}
trap cleanup EXIT HUP INT TERM
git -C "$REPO_ROOT" worktree add --quiet --detach "$WORKTREE" "$BASELINE"
g++ -std=c++17 -O2 -fPIC -shared -fvisibility=hidden -fvisibility-inlines-hidden \
  -I"$WORKTREE/framework" -I"$WORKTREE/plugins/TROLLfx5510Single/src" \
  -o "$OLD_SO" "$WORKTREE/plugins/TROLLfx5510Single/src/5510_single.cpp"
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic \
  -o "$TEST_BIN" "$PLUGIN_DIR/tests/state_compat_test.cpp" -ldl
"$TEST_BIN" "$OLD_SO" "$NEW_SO"
