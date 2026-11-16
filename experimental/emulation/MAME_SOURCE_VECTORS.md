# MAME-source-derived ES5510 vectors

`mame_source_vector_test.cpp` provides deterministic arithmetic/control-flow
checks for the standalone execution adapter. The expected values were derived
from the pinned MAME implementation (`8be57cdc5f44ab830dff44c06dd39f24190742d4`),
including `alu_operation`, `execute_run`, register IDs and pipeline staging in
`src/devices/cpu/es5510/es5510.cpp` / `.h`.

These are **not** an execution-time comparison against the MAME device. The
available local MAME checkout is sparse: it includes only the ES5510, pump and
Ensoniq machine-driver paths (about 5.2 MB), but omits `src/emu`, build files,
and a MAME executable. `es5510_device` derives from MAME's `cpu_device`; its
public `run_once()` method still requires MAME's device lifecycle. Thus this
test can catch regressions in the adapter against independently recorded
source-derived outcomes, but cannot demonstrate bit-for-bit conformance to an
executing reference core.

Covered vectors:

- signed saturating ADD (`0x7fffff + 1`): result `0x7fffff`, CCR `0x20`;
- wrapping ADD (`0x7fffff + 1`): result `0x800000`, CCR `0xa0`;
- 24 x 24 signed MAC pipeline using the default x4 scale: quarter-scale inputs
  yield `0x400000` in the high MAC/GPR result after the next instruction;
- true CCR/CMR condition with skip bit: suppresses the selected MOV.

Build and run from repository root:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  experimental/emulation/mame_es5510_standalone.cpp \
  experimental/emulation/mame_source_vector_test.cpp \
  -o /tmp/es5510-source-vectors
/tmp/es5510-source-vectors
```

A real differential milestone requires a full buildable MAME source tree at
the pinned commit and a test driver that instantiates `es5510_device`, loads
these exact original instruction words, and exports register/DOL/DRAM/audio
traces. Until then describe the result as source-derived vectors, not a MAME
differential pass.
