#!/usr/bin/env python3
"""Normalize OpenASR T551ESP device-boundary logs for capture reconstruction.

The private input log may contain factory DSP bytes. Both input and outputs
must remain outside the repository. CPU-space Lua taps are not used as the
authoritative stream when direct ES5510 device events are present.
"""
from __future__ import annotations
import argparse, json, re, sys
from pathlib import Path

DEVICE = re.compile(r"T551ESP\t([A-Z0-9_]+)\t([^\t\r\n]+)(?:\t([^\t\r\n]+))?(?:\t([^\t\r\n]+))?(?:\t([^\t\r\n]+))?(?:\t([^\t\r\n]+))?")
SYN_BEGIN = re.compile(r"T551CAP_TEST_SYNTHETIC_BEGIN\tsec=([^\t]+)\taddress=([0-9A-Fa-f]+)\tvalue=([0-9A-Fa-f]+)")

SELECTORS = {
    0x80: (0x00FC3100, "read_select"),
    0xA0: (0x00FC3140, "gpr_commit"),
    0xC0: (0x00FC3180, "instr_commit"),
    0xE0: (0x00FC31C0, "combined_commit"),
}

def convert(text: str) -> tuple[str, dict]:
    rows=[]; details=[]; metadata=[]; pa=[]; seq=0; synthetic=False; synthetic_rows=0
    semantic_commits=[]
    for n,line in enumerate(text.splitlines(),1):
        if line.startswith("T551CAP_META\t"):
            metadata.append(line)
        start=SYN_BEGIN.search(line)
        if start:
            synthetic=True
            details.append({"line":n,"type":"synthetic_test_begin","time":start.group(1),
                            "address":start.group(2).upper(),"value":start.group(3).upper()})
            continue
        if "T551CAP_TEST_SYNTHETIC_END\t" in line:
            synthetic=False
            details.append({"line":n,"type":"synthetic_test_end"})
            continue
        m=DEVICE.search(line)
        if not m: continue
        op,t,a,b,c,d=m.groups()
        if op in ("W","R"):
            off=int(a,16); value=int(b,16)
            if off < 0x40:
                address=0x00FC3000+2*off
                kind="host_"+op.lower()
            elif off in SELECTORS:
                address,label=SELECTORS[off]
                kind=label+"_"+op.lower()
            else:
                # Keep out-of-map offsets observable, but do not treat them as
                # instruction/GPR commits by guessing a CPU-space address.
                address=0x00FC3000+2*off
                kind="device_host_"+op.lower()
            seq+=1
            rows.append({"seq":seq,"line":n,"time":t,"address":address,
                         "data":value,"kind":kind,"offset":off,
                         "synthetic":synthetic})
            if synthetic: synthetic_rows+=1
        elif op in ("COMBINED_COMMIT","INSTR_COMMIT","GPR_COMMIT"):
            fields={"line":n,"time":t,"type":op,"index":int(a,16)}
            if op=="COMBINED_COMMIT":
                fields.update(instruction_hex=b.lower(),gpr_hex=c.lower())
            elif op=="INSTR_COMMIT": fields["instruction_hex"]=b.lower()
            else: fields["gpr_hex"]=b.lower()
            semantic_commits.append(fields)
        elif op=="PA4":
            data=int(a,16); run,received,loaded=map(int,(b,c,d))
            pa.append({"line":n,"time":t,"data":data,"run":bool(run),
                       "received":bool(received),"loaded":bool(loaded)})
        else:
            detail={"line":n,"time":t,"type":op}
            for i,v in enumerate((a,b,c,d)):
                if v is not None: detail[f"field{i+1}"]=v
            details.append(detail)

    if not rows: raise ValueError("no T551ESP device-boundary host reads/writes found")
    normalized=["T551CAP_META\t1\tsource_class=openasr\tmachine=asr10booth\tOpenASR=instrumented\tMAME=instrumented\tobservation=es5510_device_boundary\thost_address_encoding=word_base_16bit_active_low_byte"]
    ordered=[]
    for row in rows:
        ordered.append((row["line"],0,f"T551CAP\tv1\t{row['seq']}\t{row['address']:08X}\t{row['data']:08X}\t000000FF\t00000000\t{row['kind']}\topenasr-device-boundary\tmame-instrumented\t{row['time']}"))
        if row["synthetic"]: ordered.append((row["line"],1,"T551CAP_SYNTHETIC\t"+str(row["seq"])))
    for event in pa:
        ordered.append((event["line"],0,f"T551_PA data={event['data']:04X} run={int(event['run'])} received={int(event['received'])} loaded={int(event['loaded'])}"))
    for detail in details:
        if detail["type"]=="synthetic_test_begin":
            record=f"T551CAP_TEST_SYNTHETIC_BEGIN\tsec={detail['time']}\taddress={detail['address']}\tvalue={detail['value']}"
            ordered.append((detail["line"],0,record))
        elif detail["type"]=="synthetic_test_end":
            ordered.append((detail["line"],0,"T551CAP_TEST_SYNTHETIC_END\tsec=0"))
    normalized.extend(x[2] for x in sorted(ordered,key=lambda x:(x[0],x[1])))
    audit={"schema":"trollfx.openasr-es5510-device-trace-audit.v1",
           "authoritative_source":"ES5510 host_r/host_w device boundary",
           "device_host_event_count":len(rows),"host_read_count":sum(r["kind"]=="host_r" for r in rows),
           "host_write_count":sum(r["kind"]=="host_w" for r in rows),
           "synthetic_test_event_count":synthetic_rows,"pa4_event_count":len(pa),
           "semantic_commits":semantic_commits,"details":details,"metadata":metadata,
           "events":rows,"pa4":pa}
    return "\n".join(normalized)+"\n",audit

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("trace",type=Path); ap.add_argument("normalized",type=Path); ap.add_argument("audit",type=Path)
    ap.add_argument("--repo-root",type=Path,default=Path(__file__).resolve().parents[1])
    a=ap.parse_args()
    for p in (a.trace,a.normalized,a.audit):
        try: p.resolve().relative_to(a.repo_root.resolve())
        except ValueError: pass
        else: print("ERROR: raw trace and generated capture files must be outside the repository",file=sys.stderr); return 2
    try:
        normalized,audit=convert(a.trace.read_text(encoding="utf-8"))
        a.normalized.write_text(normalized,encoding="utf-8")
        a.audit.write_text(json.dumps(audit,indent=2)+"\n",encoding="utf-8")
    except (OSError,ValueError) as e:
        print(f"ERROR: {e}",file=sys.stderr); return 2
    print(f"device_events={audit['device_host_event_count']} synthetic={audit['synthetic_test_event_count']} normalized={a.normalized}")
    return 0
if __name__=="__main__": raise SystemExit(main())
