#!/usr/bin/env python3
"""Hash and classify local ASR-10 firmware/media without executing it.

Default input root is ignored `research_local/asr10_firmware`. JSON contains
only names, metadata, hashes and archive member names (never payload bytes).
"""
from __future__ import annotations
import argparse, hashlib, json, struct, sys, zipfile, zlib
from pathlib import Path

EXPECTED={
 "asr-648c-lo-1.5b.bin":{"size":131072,"crc32":"8e437843","sha1":"418f042acbc5323f5b59cbbd71fdc8b2d851f7d0"},
 "asr-65e0-hi-1.5b.bin":{"size":131072,"crc32":"b37cd3b6","sha1":"c4371848428a628b5e5a50e99be602d7abfc7904"},
}
EXPECTED_FILES={
 "asr-648c-lo-1.5b.bin":"ASR-10/88 lower boot ROM, MAME fingerprint reference",
 "asr-65e0-hi-1.5b.bin":"ASR-10/88 upper boot ROM, MAME fingerprint reference",
 "V161.img":"ASR OS 1.61 disk image (OpenASR expected name)",
 "V350.img":"OS 3.50 disk image (OpenASR expected name; model compatibility unresolved)",
 "V353.img":"ASR OS 3.53 image candidate",
}

def inspect(path: Path) -> dict:
    data=path.read_bytes(); crc=f"{zlib.crc32(data)&0xffffffff:08x}"
    row={"name":path.name,"size":len(data),"sha256":hashlib.sha256(data).hexdigest(),
         "sha1":hashlib.sha1(data).hexdigest(),"crc32":crc,"format":"unknown"}
    exp=EXPECTED.get(path.name.lower())
    if exp:
        row["format"]="ASR-10 1.5b boot ROM candidate"
        row["fingerprint_match"]=(len(data)==exp["size"] and crc==exp["crc32"] and row["sha1"]==exp["sha1"])
    if data.startswith(b"MZ"):
        row["format"]="DOS executable/container; not executed"
        if len(data)>=0x40:
            ne=struct.unpack_from("<I",data,0x3c)[0]
            if ne+0x40<=len(data) and data[ne:ne+2]==b"NE":
                seg_count=struct.unpack_from("<H",data,ne+0x1c)[0]
                seg_table=struct.unpack_from("<H",data,ne+0x22)[0]
                shift=struct.unpack_from("<H",data,ne+0x32)[0]
                row.update(format="Windows NE self-extracting installer (data inspected only)",
                           ne_header_offset=ne,segment_count=seg_count,
                           segment_table_offset=seg_table,alignment_shift=shift)
                for marker in (b"SETUPMN.ARV",b"SETUPMN.EXE",b"CTL3DV2.DLL",b"ASETUP.EXE",
                               b"asr161os.eda",b"asr353os.eda",b"ede.exe",b"remove.exe"):
                    if marker in data: row.setdefault("referenced_filename_strings",[]).append(marker.decode("ascii"))
                row["archive_members"]="not enumerated; ARV payload needs non-executing compatible extractor"
    elif data.startswith(b"PK\x03\x04") or zipfile.is_zipfile(path):
        row["format"]="ZIP archive"
        try:
            with zipfile.ZipFile(path) as z:
                row["archive_members"]=[{"name":i.filename,"size":i.file_size,"crc32":f"{i.CRC:08x}"} for i in z.infolist()]
        except (OSError,zipfile.BadZipFile) as e: row["archive_error"]=str(e)
    elif path.suffix.lower() in (".img",".eda",".hfe",".mfi",".dsk",".efe"):
        row["format"]={".img":"raw disk image candidate",".eda":"Ensoniq ASR disk-image candidate",".hfe":"HxC floppy image candidate",".mfi":"MAME floppy image candidate",".dsk":"disk-image candidate",".efe":"Ensoniq effect-file candidate"}[path.suffix.lower()]
    return row

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root",nargs="?",type=Path,default=Path(__file__).resolve().parents[1]/"research_local/asr10_firmware")
    ap.add_argument("--output",type=Path,help="optional JSON metadata file; never includes binary contents")
    a=ap.parse_args()
    rows=[]
    for p in sorted(x for x in a.root.rglob("*") if x.is_file()) if a.root.exists() else []:
        try: rows.append(inspect(p))
        except OSError as e: rows.append({"name":p.name,"error":str(e)})
    present={r["name"].lower() for r in rows}
    expected=[{"name":name,"description":desc,"present":name.lower() in present}
              for name,desc in EXPECTED_FILES.items()]
    result={"schema":"trollfx.asr10-firmware-inventory.v1","root":str(a.root),
            "expected_assets":expected,"files":rows}
    out=json.dumps(result,indent=2)+"\n"
    if a.output:
        a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_text(out,encoding="utf-8")
    print(out,end="")
    return 0
if __name__=="__main__": raise SystemExit(main())
