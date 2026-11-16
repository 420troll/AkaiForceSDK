#!/usr/bin/env python3
"""Local TeleBrain CSV extractor. Read-only; generated JSON stays local."""
import argparse, csv, hashlib, io, json, re, zipfile
from pathlib import Path
from collections import defaultdict, Counter

MOD = {24:"mod1_source",25:"mod1_destination",26:"mod1_min",27:"mod1_max",28:"mod2_source",29:"mod2_destination",30:"mod2_min",31:"mod2_max"}
def number(s):
    return int(s) if re.fullmatch(r"\d+",s.strip()) else None

def extract(archive):
    result = {}
    with zipfile.ZipFile(archive) as z:
        for path in z.namelist():
            if path.startswith("__MACOSX/") or not re.search(r"/\d+\.csv$",path):
                continue
            raw=z.read(path)
            lines=list(csv.reader(io.StringIO(raw.decode("utf-8-sig","replace")),delimiter=";"))
            if not lines: continue
            key=Path(path).stem
            rows=[]; warnings=[]; positions=defaultdict(list); offsets=defaultdict(list)
            for line,values in enumerate(lines[1:],start=2):
                if not any(v.strip() for v in values): continue
                name,disp,off,kind,rawrange,shown=(values+[""]*6)[:6]
                disp,off,kind=disp.strip(),off.strip(),kind.strip()
                d,o=number(disp),number(off)
                if d is None and disp: warnings.append({"code":"non_numeric_display_order","line":line,"raw":disp})
                if d is not None: positions[d].append(line)
                if o is not None: offsets[o].append(line)
                rows.append({"line":line,"name_raw":name.strip(),"display_order_raw":disp,"display_order":d,"offset_raw":off,"offset":o,"type_raw":kind,"internal_range_raw":rawrange.strip(),"display_range_raw":shown.strip(),"mod_role":MOD.get(o) if kind.lower()!="function" else None,"editable":kind.lower()!="function"})
            for d,loc in positions.items():
                if len(loc)>1:warnings.append({"code":"duplicate_display_order","value":d,"lines":loc})
            for o,loc in offsets.items():
                if len(loc)>1:warnings.append({"code":"duplicate_internal_offset","value":o,"lines":loc})
            missing=sorted(MOD.keys()-offsets.keys())
            if missing:warnings.append({"code":"missing_modulation_offsets","values":missing})
            result[key]={"source_filename":Path(path).name,"source_sha256":hashlib.sha256(raw).hexdigest(),"header_raw":lines[0],"rows":rows,"warnings":warnings}
    return {"format":"trollfx.telebrain.local-catalog.v1","source_note":"user-supplied purchased editor; do not redistribute extracted catalog","table_count":len(result),"tables":dict(sorted(result.items(),key=lambda kv:int(kv[0])))}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("archive",type=Path)
    p.add_argument("--output",type=Path,default=Path("build/telebrain_catalog.local.json"))
    args=p.parse_args()
    data=extract(args.archive)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(data,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print("CSV tables:",data["table_count"],"rows:",sum(len(t["rows"]) for t in data["tables"].values()))
    print("Warnings:",dict(Counter(w["code"] for t in data["tables"].values() for w in t["warnings"])))
    print("Saved:",args.output)
if __name__=="__main__":main()
