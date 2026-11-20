#!/usr/bin/env python3
"""Reconstruct ES5510 host state from passive OpenASR T551CAP tap output.

Outputs may contain copyrighted program bytes; paths inside the source tree are
refused. A TAP log is an observation, not by itself a validated factory image.
"""
from __future__ import annotations
import argparse, json, re, sys
from pathlib import Path

ROW = re.compile(r"^T551CAP\tv1\t(\d+)\t([0-9A-Fa-f]{8})\t([0-9A-Fa-f]{8})\t([0-9A-Fa-f]{8})\t([0-9A-Fa-f]{8})\t([^\t]+)\t([^\t]+)\t([^\t]+)\t([^\t]+)$")
MAX_WORDS = 160

class TraceError(ValueError): pass

def reconstruct(text: str) -> dict:
    events=[]; metadata=[]
    for n,line in enumerate(text.splitlines(),1):
        if line.startswith("T551CAP_META\t"):
            metadata.append(line); continue
        if not line.startswith("T551CAP\t"): continue
        m=ROW.fullmatch(line)
        if not m: raise TraceError(f"line {n}: malformed T551CAP row")
        seq,address,data,mask,pc,kind,openasr,mame,t=m.groups()
        events.append({"seq":int(seq),"address":int(address,16),"data":int(data,16),
                       "mask":int(mask,16),"pc":int(pc,16),"kind":kind,
                       "openasr_commit":openasr,"mame_commit":mame,"time":t})
    if not events: raise TraceError("no T551CAP events found")
    events.sort(key=lambda e:e["seq"])
    if len({e["seq"] for e in events}) != len(events): raise TraceError("duplicate sequence number")
    regs={}; instructions={}; commits=[]; latches={}; lifecycle=[]
    for e in events:
        a,d=e["address"],e["data"]
        if e["kind"]=="padat_w":
            lifecycle.append({"seq":e["seq"],"event":"padat_write","data":d,
                              "pa4_run_model":bool(d & 0x10),"pc":e["pc"]})
        if e["kind"]=="host_w" and 0x00FC3000 <= a <= 0x00FC303F:
            # MAME's 16-bit program-space tap reports the aligned word base.
            # The ASR host map is 8-bit on the active low lane, so consecutive
            # ES5510 latches appear at FC3000, FC3002, ... in the tap. Preserve
            # compatibility with older fixtures/logs that reported byte addresses.
            word_base=any("host_address_encoding=word_base_16bit_active_low_byte" in x for x in metadata)
            off=(a-0x00FC3000)//2 if word_base else a-0x00FC3000
            # OpenASR maps these host offsets to the MAME ES5510 latch interface.
            if off <= 0x02 or 0x03 <= off <= 0x08: latches[off]=d & 0xff
            elif off in (0x0f,0x10,0x11,0x12,0x14,0x18,0x1f):
                commits.append({"seq":e["seq"],"kind":"host_control_or_memory_port_write",
                                "host_offset":off,"data":d & 0xff,"pc":e["pc"]})
        if e["kind"] not in ("gpr_commit_w","instr_commit_w","combined_commit_w"): continue
        idx=d & 0xff
        item={"seq":e["seq"],"selector":e["kind"],"index":idx,"pc":e["pc"]}
        if e["kind"] in ("gpr_commit_w","combined_commit_w"):
            g=(latches.get(0,0)<<16)|(latches.get(1,0)<<8)|latches.get(2,0)
            if idx < 256: regs[idx]=g
            item["gpr_value_hex"]=f"{g:06x}"
        if e["kind"] in ("instr_commit_w","combined_commit_w") and idx < MAX_WORDS:
            w=0
            for off in range(3,9): w=(w<<8)|latches.get(off,0)
            instructions[idx]=w
            item["instruction_hex"]=f"{w:012x}"
        commits.append(item)
    # First END index is a candidate boundary only; code may include conditional
    # or unreachable END words, so this cannot establish a program boundary alone.
    end_indices=[i for i,w in sorted(instructions.items()) if ((w>>12)&15)==15]
    halts=[x for x in lifecycle if not x["pa4_run_model"]]
    runs=[x for x in lifecycle if x["pa4_run_model"]]
    seqs=sorted(instructions)
    contiguous=bool(seqs) and seqs==list(range(seqs[-1]+1))
    special={str(r):f"{regs[r]:06x}" for r in (244,245,246,247,248,249,250,251) if r in regs}
    # Strict program validation needs a known firmware image identity and an
    # observed HALT -> complete program/GPR setup -> RUN interval. Those inputs
    # are deliberately not inferred from a tap file's own text.
    upload_events=[x for x in commits if x.get("selector") in ("gpr_commit_w","instr_commit_w","combined_commit_w")]
    lifecycle_ok=bool(halts and runs and upload_events
      and any(h["seq"] < upload_events[0]["seq"] for h in halts)
      and any(r["seq"] > upload_events[-1]["seq"] for r in runs))
    valid_meta=any("machine=asr10booth" in x and "OpenASR=" in x and "MAME=" in x for x in metadata)
    fixture=any("fixture=synthetic" in x for x in metadata)
    validated=(valid_meta and not fixture and lifecycle_ok and contiguous and bool(end_indices)
               and bool(regs) and "244" in special and "245" in special)
    return {"schema":"trollfx.asr10-es5510-host-capture.v1",
      "source_class":"synthetic_test_fixture" if fixture else ("openasr_cpu_program_space_firmware_write_observation" if valid_meta else "unattributed_trace"),
      "capture_status":"candidate_complete_needs_independent_review" if validated else "incomplete",
      "validated_factory_program":False,
      "metadata_lines":metadata,"event_count":len(events),"events":events,
      "lifecycle":lifecycle,"commit_events":commits,
      "instructions":[{"index":i,"word_hex":f"{w:012x}"} for i,w in sorted(instructions.items())],
      "gprs":[{"register":i,"value_hex":f"{v:06x}"} for i,v in sorted(regs.items())],
      "special_registers":special,"candidate_end_indices":end_indices,
      "program_indices_contiguous_from_zero":contiguous,
      "halt_run_sequence_observed":lifecycle_ok,
      "limitations":["Tap evidence is CPU program-space traffic in OpenASR, not physical pin capture.",
        "For host_address_encoding=word_base_16bit_active_low_byte, direct host-port tap addresses are aligned word bases and map to byte registers at a two-byte stride.",
        "PA4/RUN-HALT is an OpenASR MC68302 callback and is not represented by a memory-space tap at $FC6822/$FC6823.",
        "OpenASR's ES5510 execution core differs from the pinned upstream MAME reference; CPU writes do not establish core conformance.",
        "Program boundaries and completeness require independent validation against firmware selection and ES5510 behavior.",
        "This parser does not infer omitted writes, delay RAM contents, or firmware identity.",
        "The PA4 interpretation reflects the OpenASR model and is not an electrical measurement."]}

def inside(path: Path, root: Path) -> bool:
    try: path.resolve().relative_to(root.resolve()); return True
    except ValueError: return False

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("trace",type=Path); ap.add_argument("output",type=Path)
    ap.add_argument("--repo-root",type=Path,default=Path(__file__).resolve().parents[1])
    a=ap.parse_args()
    if inside(a.trace,a.repo_root): print("ERROR: private trace input must be outside the repository",file=sys.stderr); return 2
    if inside(a.output,a.repo_root): print("ERROR: capture output must be outside the repository",file=sys.stderr); return 2
    try:
        result=reconstruct(a.trace.read_text(encoding="utf-8"))
        a.output.parent.mkdir(parents=True,exist_ok=True)
        a.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    except (OSError,TraceError) as e: print(f"ERROR: {e}",file=sys.stderr); return 2
    print(f"status={result['capture_status']} events={result['event_count']} output={a.output}")
    return 0
if __name__=="__main__": raise SystemExit(main())
