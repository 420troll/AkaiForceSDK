# Permanent development instructions

## Scope and target

- Build Ensoniq-inspired instruments and effects for the original Akai Force:
  ARMv7-A, hard-float, 32-bit Linux. Preserve native desktop testing and avoid
  assumptions that prevent an ARM64 build.
- Use the official `inmusic-software/mpc-plugin-builder-sdk` as the host/API
  reference. Pin and record its commit whenever build behavior is cited.
- Test on the development computer before proposing a hardware test. Never
  install, copy, restart, or modify anything on a Force without explicit user
  authorization for that deployment.

## Evidence discipline

- Label statements as **verified**, **inferred/hypothesis**, or **unresolved**.
- A product manual can verify exposed routing and parameter behavior, but it
  does not reveal the underlying microcode. Do not turn block diagrams or
  parameter names into claims of exact topology.
- Do not claim that an original Ensoniq algorithm has been recovered unless a
  reproducible artifact and comparison support the claim. Record the artifact
  provenance, legal status, hashes, test method, hardware/firmware version, and
  error metrics.
- Treat `ES5100` as unverified until a primary source or photographed part
  marking establishes it. Do not silently substitute it for ES5510.
- Record source URL, title, publisher/author, access date, relevant page or
  source lines, and repository commit hash. Prefer primary specifications,
  manuals, schematics, and upstream source.
- Never commit proprietary ROMs, firmware, factory effect binaries, disk
  images, or copyrighted manuals. Hashes and public source links are allowed.

## Separation and licensing

- `research/`: observations, measurements, citations, and test fixtures that
  contain no proprietary payload.
- `src/original/`: independently written production DSP. It may be inspired by
  documented behavior, but must not include emulator code unless the license
  and design decision are explicitly documented.
- `experimental/emulation/`: instruction-level or hardware emulation
  experiments. Keep this out of production targets by default.
- MAME's ES5510 core is BSD-3-Clause. Retain attribution and all three license
  conditions if code is adapted. A factual reading of the code is not a license
  to copy other projects whose repositories lack a clear license.
- Review every dependency for source and binary redistribution obligations.

## Real-time implementation rules

- No allocation, locks, file I/O, logging, or blocking on the audio thread.
- Prepare delay buffers and tables in `prepare()`. Smooth audible continuous
  parameters. Handle in-place input/output and every block length up to the
  announced maximum.
- Use bounded memory, deterministic code, finite-value checks, denormal
  handling, and stable feedback limits. Prefer contiguous buffers and simple
  C++17 suitable for Cortex-A17/NEON.
- Treat 44.1 kHz as the current Akai host contract, but keep DSP sample-rate
  aware unless a deliberate emulation mode requires a fixed rate.
- Stable parameter IDs are persistent state. Never rename them casually.

## Required verification per algorithm

1. Native compile with warnings enabled.
2. Unit tests for parameter mapping, bounds, reset, and state restore.
3. Host-simulator tests at 32, 64, 96, 128, 192, 256, and 512 frames where
   practical; include in-place processing, silence, finite output, and sweeps.
4. Offline impulse, sweep, modulation, and feedback-stability renders.
5. ARMv7 cross-compile and SDK validation. ARM64 cross-compile when practical.
6. Record exact commands, tool versions, results, failures, and commit hashes.
7. Hardware deployment only after explicit authorization; preserve existing
   plugin folders and use a new uniquely named folder.

## Working sequence

The first production milestone is a behaviorally documented, original
modulated stereo delay/chorus core. Reuse its delay interpolation, smoothing,
LFO, filtering, and routing primitives before attempting diffusion reverb or
pitch shifting. Instruction-level ES5510 work remains an independent research
track, not a prerequisite for useful Force effects.

