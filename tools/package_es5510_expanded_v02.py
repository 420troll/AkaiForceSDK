#!/usr/bin/env python3
"""Reproducibly package the official SDK V0.2 build for Force ARMv7."""
import argparse
import json
from pathlib import Path
import zipfile

FOLDER = "TROLLfx - VST - ES5510 Expanded V0.2"
ARCHIVE = "trollfx-es5510-expanded-v0.2-akai-force-armv7.zip"
FIXED_TIME = (2026, 1, 1, 0, 0, 0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()
    repo = args.repo.resolve()
    project = repo / "plugins" / "TROLLfxES5510ExpandedV02"
    manifest = json.loads((project / "plugin.json").read_text())
    if manifest.get("uid") != "TfxV" or manifest.get("name") != "ES5510 Expanded V0.2":
        raise SystemExit("Unexpected V0.2 identity; refusing to package")
    built = project / "build" / FOLDER
    required = [manifest["file32"], "plugin.json", "version.xml",
                "Plugin Skins/TUI.json", "Plugin Skins/Q-Links.json"]
    for rel in required:
        if not (built / rel).is_file():
            raise SystemExit(f"Missing built package file: {built / rel}")
    output = args.output or repo / "artifacts/es5510-emulation/v02" / ARCHIVE
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    members = [(f"{FOLDER}/{p.relative_to(built).as_posix()}", p.read_bytes())
               for p in sorted(built.rglob("*")) if p.is_file()]
    for name, path in (("INSTALL.txt", project / "INSTALL.txt"),
                       ("README.md", project / "README.md"),
                       ("THIRD_PARTY_NOTICES.md", repo / "experimental/emulation/THIRD_PARTY_NOTICES.md")):
        members.append((f"{FOLDER}/{name}", path.read_bytes()))
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for name, data in sorted(members):
            info = zipfile.ZipInfo(name, FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            zf.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    print(output)


if __name__ == "__main__":
    main()
