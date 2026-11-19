#!/usr/bin/env python3
"""Convert a reconstructed private ASR-10 capture to the standalone renderer bundle.

This is an opt-in research replay path, not a claim that the program is factory
authentic or accurate. The capture and bundle must remain outside Git.
"""
import argparse, json, sys
from pathlib import Path

def inside(p: Path, root: Path) -> bool:
    try: p.resolve().relative_to(root.resolve()); return True
    except ValueError: return False

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("capture",type=Path); ap.add_argument("output",type=Path)
    ap.add_argument("--repo-root",type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument("--accept-candidate",action="store_true",help="explicitly permit replay of an unvalidated candidate")
    ap.add_argument("--sample-rate",type=int,default=44100)
    a=ap.parse_args()
    if inside(a.capture,a.repo_root) or inside(a.output,a.repo_root):
        print("ERROR: capture and bundle paths must both be outside the repository",file=sys.stderr); return 2
    try: c=json.loads(a.capture.read_text(encoding="utf-8"))
    except (OSError,json.JSONDecodeError) as e: print(f"ERROR: {e}",file=sys.stderr); return 2
    if (not a.accept_candidate or c.get("schema")!="trollfx.asr10-es5510-host-capture.v1"
        or c.get("capture_status")!="candidate_complete_needs_independent_review"
        or c.get("source_class")!="mame_runtime_firmware_write_observation"):
        print("ERROR: pass --accept-candidate and provide a recognized capture",file=sys.stderr); return 2
    ins=c.get("instructions",[]); regs=c.get("gprs",[])
    if not ins or len(ins)>160 or not c.get("candidate_end_indices"):
        print("ERROR: candidate lacks bounded instruction words or an END opcode",file=sys.stderr); return 2
    if [x.get("index") for x in ins] != list(range(len(ins))):
        print("ERROR: instruction indices are not contiguous from zero",file=sys.stderr); return 2
    lines=["ES5510_BUNDLE 1",f"sample_rate {a.sample_rate}","# Reconstructed from OpenASR/MAME host writes; authenticity unverified."]
    # Renderer bundle records are ordered. The capture JSON already contains
    # commit-ordered final state; no values are fabricated for missing GPRs.
    lines += [f"instruction 0x{x['word_hex']}" for x in ins]
    lines += [f"gpr {x['register']} 0x{x['value_hex']}" for x in regs]
    # Replay MAME host serial only when directly observed in host offset 0x18.
    serial=[x["data"] for x in c.get("commit_events",[]) if x.get("host_offset")==0x18]
    if serial: lines.insert(2,f"host_serial 0x{serial[-1]:02x}")
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text("\n".join(lines)+"\n",encoding="ascii")
    print(f"Wrote {len(ins)} instruction(s), {len(regs)} captured register(s); unverified candidate: {a.output}")
    return 0
if __name__=="__main__": raise SystemExit(main())
