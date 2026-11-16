#!/usr/bin/env python3
"""Create a reproducible ARMv7 Force package for the consolidated V0.3 plugin."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

FOLDER = "TROLLfx - VST - ES5510 NonLin Reverb V0.3"
ARCHIVE = "trollfx-es5510-v03-nonlinear-reverb-akai-force-armv7.zip"
FIXED_TIME = (2026, 1, 1, 0, 0, 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    repo = args.repo.resolve()
    project = repo / "plugins/TROLLfxES5510ReverbV03"
    manifest = json.loads((project / "plugin.json").read_text())
    if manifest.get("uid") != "Tfx3" or manifest.get("name") != "ES5510 NonLin Reverb V0.3":
        raise SystemExit("Unexpected V0.3 identity; refusing to package")
    built = project / "build" / FOLDER
    required = [manifest["file32"], "plugin.json", "version.xml",
                "Plugin Skins/TUI.json", "Plugin Skins/Q-Links.json"]
    for rel in required:
        if not (built / rel).is_file():
            raise SystemExit(f"Missing built package file: {built / rel}")
    output = args.output or repo / "artifacts/es5510-emulation/v03" / ARCHIVE
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    members = [(f"{FOLDER}/{p.relative_to(built).as_posix()}", p.read_bytes())
               for p in sorted(built.rglob("*")) if p.is_file()]
    for name, path in (("INSTALL.txt", project / "INSTALL.txt"),
                       ("README.md", project / "README.md"),
                       ("THIRD_PARTY_NOTICES.md", repo / "experimental/emulation/THIRD_PARTY_NOTICES.md")):
        members.append((f"{FOLDER}/{name}", path.read_bytes()))
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(members):
            info = zipfile.ZipInfo(name, FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    print(f"{output}\nSHA-256 {hashlib.sha256(output.read_bytes()).hexdigest()}")


if __name__ == "__main__":
    main()
