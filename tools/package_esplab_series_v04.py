#!/usr/bin/env python3
"""Create a deterministic separate ARM Force package for ESP Lab Series V0.4."""
import hashlib
import json
from pathlib import Path
import zipfile

FOLDER="TROLLfx - VST - ESP Lab Series V0.4"
ARCHIVE="trollfx-esp-lab-series-v04-akai-force-armv7.zip"
FIXED_TIME=(2026,1,1,0,0,0)

def main():
    repo=Path(__file__).resolve().parents[1]
    project=repo/"plugins/TROLLfxESPLabSeriesV04"
    manifest=json.loads((project/"plugin.json").read_text())
    if manifest.get("uid")!="TfxS": raise SystemExit("Unexpected series UID")
    owners={}
    for path in sorted((repo/"plugins").glob("*/plugin.json")):
        uid=json.loads(path.read_text()).get("uid")
        if isinstance(uid,str):owners.setdefault(uid,[]).append(path)
    dup={k:v for k,v in owners.items() if len(v)>1}
    if dup:raise SystemExit(f"Plugin UID collision: {dup}")
    built=project/"build"/FOLDER
    required=[manifest["file32"],manifest["file64"],"plugin.json","version.xml","Plugin Skins/TUI.json","Plugin Skins/Q-Links.json"]
    for item in required:
        if not (built/item).is_file():raise SystemExit(f"Missing build product: {item}")
    out=repo/"artifacts/esplab-series/v04"/ARCHIVE;out.parent.mkdir(parents=True,exist_ok=True)
    members=[(f"{FOLDER}/{p.relative_to(built).as_posix()}",p.read_bytes()) for p in sorted(built.rglob("*")) if p.is_file()]
    for name,path in (("README.md",project/"README.md"),("INSTALL.txt",project/"INSTALL.txt")):
        members.append((f"{FOLDER}/{name}",path.read_bytes()))
    with zipfile.ZipFile(out,"w",zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(members):
            item=zipfile.ZipInfo(name,FIXED_TIME);item.compress_type=zipfile.ZIP_DEFLATED;item.external_attr=0o100644<<16
            z.writestr(item,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
    print(out);print("SHA-256",hashlib.sha256(out.read_bytes()).hexdigest())
if __name__=="__main__":main()
