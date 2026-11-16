#!/usr/bin/env python3
"""Create a reproducible Force V0.1 package from the official SDK build output."""
import argparse
import json
from pathlib import Path
import zipfile

PLUGIN_DIR_NAME = "TROLLfx - VST - ES5510 Experimental"
ARCHIVE_NAME = "trollfx-es5510-experimental-v0.1-akai-force-armv7.zip"
FIXED_TIME = (2026, 1, 1, 0, 0, 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    repo = args.repo.resolve()
    project = repo / "plugins" / "TROLLfxES5510Experimental"
    built = project / "build" / PLUGIN_DIR_NAME
    manifest = json.loads((project / "plugin.json").read_text())
    if manifest.get("uid") != "TfxX" or manifest.get("name") != "ES5510 Experimental":
        raise SystemExit("Unexpected plugin identity; refusing to package")
    arm32 = built / manifest["file32"]
    if not arm32.is_file():
        raise SystemExit(f"Missing ARM32 plugin: {arm32}")
    for relative in ("plugin.json", "version.xml", "Plugin Skins/TUI.json", "Plugin Skins/Q-Links.json"):
        if not (built / relative).is_file():
            raise SystemExit(f"Missing built package file: {relative}")
    output = args.output or (repo / "artifacts" / "es5510-emulation" / "vst-v01" / ARCHIVE_NAME)
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    members = []
    for path in sorted(p for p in built.rglob("*") if p.is_file()):
        members.append((f"{PLUGIN_DIR_NAME}/{path.relative_to(built).as_posix()}", path.read_bytes()))
    extras = [
        ("INSTALL.txt", project / "INSTALL.txt"),
        ("README.md", project / "README.md"),
        ("THIRD_PARTY_NOTICES.md", repo / "experimental" / "emulation" / "THIRD_PARTY_NOTICES.md"),
    ]
    members.extend((f"{PLUGIN_DIR_NAME}/{name}", path.read_bytes()) for name, path in extras)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(members):
            info = zipfile.ZipInfo(name, FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    print(output)


if __name__ == "__main__":
    main()
