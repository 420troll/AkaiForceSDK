# Open questions

## Highest priority

1. **Original EPS effects hardware:** Does every original EPS/EPS-M board
   revision contain an ES5510, or is MAME sharing an architecture model that
   obscures a product difference? Obtain primary schematic/board evidence.
2. **ES5100:** Is this only a transposition/typographical error for ES5510, or
   does a real Ensoniq part/revision exist? Require a primary document or clear
   chip photograph before using the name.
3. **ES5510 DOL conversion:** Does hardware truncate negative 24-bit values
   toward negative infinity, toward zero, or vary by revision/mode? MAME's
   2026 history explicitly records conflicting observations.
4. **ES5510 memory population:** What DRAM sizes, widths and MEMSIZ modes are
   used by EPS-16 Plus, ASR-10, DP/2, DP/4/4+, VFX/SD and other products?
5. **ESP revisions:** Which ESPR5/ESPR6/Rev-7 markings correspond to functional
   changes, and are factory programs revision-dependent?
6. **ESP2 identity:** Confirm ES5511 part markings and memory/clock topology in
   ASR-X, MR and DP/Pro with primary schematics or board photographs.

## Algorithm evidence

7. What are the exact DP/4 and DP/Pro delay-time quantization curves behind
   displayed parameter values?
8. Which interpolation method and parameter smoothing do original modulation
   and pitch algorithms use?
9. What are the exact window shape, overlap, crossfade schedule and latency of
   each pitch-shift family?
10. Do public impulse-response measurements support specific reverb matrices,
    delay lengths or diffusion counts beyond the manual block diagrams?
11. How do feedback, diffusion, damping and modulation interact at parameter
    extremes? Establish safe hardware-capture procedures before measurement.
12. Which ASR/EPS algorithms genuinely share microcode/topology with DP-series
    names, and which only share marketing/parameter vocabulary?
13. Verify DP/Pro algorithm-list entries 17 and 32 visually from a clean manual
    page; do not fill extraction gaps by guesswork.

## Emulation and licensing

14. Can MAME's ES5510 model be factored into a small conformance library while
    preserving BSD notices and without pulling MAME framework dependencies?
15. Are there upstream MAME tests or private hardware vectors not found in the
    inspected tree that can validate every opcode and hazard?
16. What license, if any, covers the original code in
    `mardlib/Ensoniq-EPS-16-Plus`? Dependency notices alone do not grant reuse.
17. Is there any public, licensed ESP2/ES5511 emulator? None was found in this
    pass.
18. Can hand-authored microprograms cover the ES5510 without handling any
    copyrighted factory program image?

## Akai/Force

19. Confirm the target Force's MPC OS version and whether External Plugin
    Support is enabled, without changing the unit.
20. Confirm actual CPU/ABI/GLIBC on the target unit against the SDK validator's
    assumptions before the first authorized deployment.
21. Validate Force-specific touchscreen scaling and Q-Link bank behavior; the
    SDK's documented 1280x800 skin canvas is shared across products.
22. Establish CPU and memory budgets for one, two and four plugin instances on
    original Force hardware.
23. Decide whether releases will ship ARM32 only or both ARM32/ARM64. The
    inspected builder's `--target arm32` validation fails when `file64` remains
    declared; document the packaging policy or upstream a fix.
24. Pin a known-good Zig release. The inspected SDK installed latest
    `ziglang` (0.17.1), while the repository did not itself pin that package in
    the observed bootstrap path.

## Repository/access

25. Does `420troll/TROLLfx-DP4-Recovery` exist outside the connected GitHub
    installation, under another owner, or in offline storage?
26. Should `TROLLfx-Force-SDK` be public or private when created on GitHub?
    This handoff did not invent that policy.
27. GitHub repository creation is unavailable through the connected tool and
    local `gh` authentication is invalid. Re-authentication or user-created
    repository access is required before push.

