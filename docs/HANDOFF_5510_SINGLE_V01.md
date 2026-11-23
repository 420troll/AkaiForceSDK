# TROLLfx 5510 Single V0.1 — hardware test handoff

## Release identity

- Product: **TROLLfx - VST - 5510 Single**, version 0.1.0
- Plugin UID: `T55S` (audited against checked-in plugin manifests; distinct from four-unit `T551`, ESP Lab `TfxE`, and ESP Lab V0.2/V0.3 UIDs)
- Development branch: `feature/5510-single-hwtest`
- Package: `plugins/TROLLfx5510Single/dist/5510-single-0.1.0-mpc.zip`
- Package SHA-256: `2acc0552d96e8b7be9836ac1add5f19873d7144f742c5d302229dacbe1279c29`
- ARMv7 binary SHA-256: `f47034935ba1f039820f9215f95654c4361b4af032fd4ba2b8f3de5865c63772`

The package is a local build artifact and is git-ignored; only source, metadata, UI, licensing, and this handoff are committed.

## DSP actually included

The plugin embeds one MAME-derived standalone ES5510 engine and its own 1,048,576-word external delay RAM. It exposes two **original TROLLfx diagnostic programs**, `TROLLfx Delay` and `TROLLfx Chorus`, with algorithm, time, feedback, rate, depth, wet, and bypass controls. Their audio path executes ES5510 microinstructions; the chorus LFO schedule is host-computed. The diagnostic programs use the existing MAME pump DOL/SER3 output route.

The private ASR-10 captures `44DDL-chorus-linux.esp` and `hall-os353.esp` were located and reviewed, but are not in the plugin or ZIP. They are runtime snapshots missing transient processor pipeline state and are not yet safe, fresh initialization bundles for this plugin. Therefore this release does **not** claim to load either Ensoniq factory effect or route a factory program through ASR SER1. Captures, firmware, effect bytes, and WAVs remain outside Git and the package. The selected diagnostic program is clearly labeled as TROLLfx in the host algorithm choices and README.

## Build and validation

Official Akai/InMusic SDK snapshot documented in `docs/SDK_COMPATIBILITY.md`: upstream commit `febd2ca5bd9eb16c4a412837196e145566bc981b` (MIT). The project builder is the checked-in SDK build/validation tooling.

Build command (Zig 0.17.1):

```sh
PYTHONPATH=/tmp/trollfx-zig \
ZIG_GLOBAL_CACHE_DIR=/tmp/trollfx-zig-cache \
ZIG_LOCAL_CACHE_DIR=/tmp/trollfx-zig-local-cache \
python3 tools/build.py plugins/TROLLfx5510Single --target all --toolchain zig --zig /usr/bin/python3
```

Results:

- Native Linux build: passed.
- ARMv7 hard-float/Cortex-A17 build: passed; 32-bit ARM EABI5 shared object, maximum GLIBC requirement 2.27; exports `VSTPluginMain`.
- ARM64 build: passed; maximum GLIBC requirement 2.29.
- Strict SDK validator: **0 errors, 0 warnings**; UID and binary identity match; one touch page and one Q-Link tab validate.
- Native `host_sim` at 44.1 kHz, 64-frame blocks: **20 passed, 0 warnings, 0 failures**. It rendered finite nonzero audio (peak -4.58 dBFS), tested 32/512-frame calls, parameter sweeps, state round trip, in-place processing, silence, no clipping, and teardown.
- Average native desktop processing cost: 1.83% of a 64-frame block; worst measured block 8.10%. This is not a Force CPU measurement.
- No hardware load, Force discovery, or audition has been performed.

## Force installation (not yet executed)

The target `/media/SSD250g/VST2 Plugins/` was unreachable during this session: both WSL SSH and Windows TCP checks failed to reach `192.168.50.47:22` (`No route to host` / `TcpTestSucceeded=False`). Consequently the Force architecture, free space, existing paths, host process state, and target-folder absence could not be confirmed remotely. No files on the Force were changed.

After restoring the existing `akai-force` SSH route, first recheck architecture, storage, plugin folders, and active MPC/plugin processes. Transfer the package to `/tmp` on the Force, then install only if the exact destination does not exist:

```sh
scp plugins/TROLLfx5510Single/dist/5510-single-0.1.0-mpc.zip akai-force:/tmp/5510-single-0.1.0-mpc.zip
ssh akai-force 'set -eu; test "$(uname -m)" = armv7l; df -h /media/SSD250g; target="/media/SSD250g/VST2 Plugins/TROLLfx - VST - 5510 Single"; test ! -e "$target"; stage="/tmp/5510-single-stage"; rm -rf "$stage"; mkdir -p "$stage"; unzip -q /tmp/5510-single-0.1.0-mpc.zip -d "$stage"; test -f "$stage/TROLLfx - VST - 5510 Single/VST/5510-single-arm32.so"; install -d "/media/SSD250g/VST2 Plugins"; cp -a "$stage/TROLLfx - VST - 5510 Single" "$target"; chmod 755 "$target/VST/5510-single-arm32.so"; sha256sum "$target/VST/5510-single-arm32.so"; ls -la "$target"'
```

Compare the installed binary SHA-256 with the ARMv7 hash above. If the Force filesystem reports `noexec`, do not change mount settings; plugin loading is performed by the host's loader. If discovery needs a host restart, save the project and ask before interrupting any active audio session. No restart is authorized by this handoff.

Rollback affects only this newly named plugin folder:

```sh
ssh akai-force 'rm -rf -- "/media/SSD250g/VST2 Plugins/TROLLfx - VST - 5510 Single"'
```

## Next milestone

Restore Force network reachability, verify the live target state, install this distinct package without replacing anything, and audition live stereo input with `TROLLfx Delay` and `TROLLfx Chorus`. Capture host discovery/load evidence and real Force CPU use. Continue factory capture initialization separately; do not put the current runtime snapshots into the callback or advertise them as clean Ensoniq programs.
