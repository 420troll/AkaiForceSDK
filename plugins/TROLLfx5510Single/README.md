# TROLLfx 5510 Single V0.2

A separate one-engine ES5510 instruction-processing insert for Akai Force. Plugin UID `T55S` is retained from V0.1, and the original seven parameter IDs and indices remain unchanged; six controls are appended. A V0.1 state migration test is included.

## Algorithms

- **Delay:** original TROLLfx ES5510 feedback-delay microprogram, free time up to 10 seconds or host-tempo delay. Tempo Sync uses VST `audioMasterGetTime` tempo when valid (including stopped transport), otherwise Fallback BPM. Straight, dotted, and triplet note divisions run from 1/32 through 1/1 plus 2 bars. Fine Offset adjusts synced time. The selected duration is checked against the actual 10-second delay limit; out-of-range choices produce a dry safety path and `Sync Range` reports the condition instead of silently shortening the delay. Tempo changes glide the delay read target. Feedback remains bipolar from -99% to +99%.
- **Chorus:** original TROLLfx ES5510 modulated-delay microprogram. Rate, depth, wet, center delay, and stereo spread are active. Stereo spread defaults to 100%, preserving V0.1's opposite-phase modulation. Chorus tempo sync is not implemented.

Both programs continue to execute ES5510 microinstructions. One engine owns 1,048,576 words of delay RAM; delay and wet/bypass transitions are smoothed, and the audio callback performs no file I/O, allocation, or blocking synchronization. Factory ASR-10 captures are not embedded.

## Interface

The Force skin has separate **DELAY** and **CHORUS** tabs. Algorithm-specific controls use `visibleWhen` and update when the algorithm choice changes. The Force SDK skin format provides static tabs and per-tab Q-Link maps, but no verified parameter-driven tab-switch action; switching the selected algorithm does not automatically change tabs. Select the corresponding tab manually. The pages have separate amber and cyan-tinted backplates; controls use the SDK's shared component style.

Q-Link banks are explicitly generated for Q1–Q8 per tab. See `ui.json` and `tools/finish_ui.py` for the mappings.

## Build and package

```sh
python3 tools/build.py plugins/TROLLfx5510Single --target all --toolchain zig --zig /path/to/zig
python3 plugins/TROLLfx5510Single/tools/finish_ui.py \
  'plugins/TROLLfx5510Single/build/TROLLfx - VST - 5510 Single'
python3 tools/validate.py \
  'plugins/TROLLfx5510Single/build/TROLLfx - VST - 5510 Single' --strict
python3 tools/package.py plugins/TROLLfx5510Single
```

This release is staged as a V0.2 package. Do not extract it over the installed V0.1 folder until a backup has been made and replacement/restart is authorized.
