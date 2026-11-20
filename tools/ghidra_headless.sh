#!/bin/sh
# Launch Ghidra headlessly with an explicit JDK to avoid the interactive JDK picker.
set -eu

: "${GHIDRA_HOME:?Set GHIDRA_HOME to the unpacked Ghidra directory}"

if [ -z "${JAVA_HOME:-}" ]; then
    javac_path=$(command -v javac || true)
    if [ -z "$javac_path" ]; then
        echo "JDK compiler javac not found; install a Ghidra-supported JDK or set JAVA_HOME" >&2
        exit 2
    fi
    javac_real=$(readlink -f "$javac_path")
    JAVA_HOME=$(dirname "$(dirname "$javac_real")")
fi

if [ ! -x "$JAVA_HOME/bin/java" ] || [ ! -x "$JAVA_HOME/bin/javac" ]; then
    echo "JAVA_HOME must point to a JDK containing bin/java and bin/javac: $JAVA_HOME" >&2
    exit 2
fi

export JAVA_HOME
PATH="$JAVA_HOME/bin:$PATH"
export PATH
exec "$GHIDRA_HOME/support/analyzeHeadless" "$@"
