#!/usr/bin/env python3
"""Print disassembly fields for a private reconstructed capture (stdout only)."""
import argparse, json, sys
from pathlib import Path
try:
    from .es5510_decode import decode_word
except ImportError:
    from es5510_decode import decode_word

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument("capture",type=Path); a=ap.parse_args()
    try: obj=json.loads(a.capture.read_text(encoding="utf-8"))
    except (OSError,json.JSONDecodeError) as e: print(f"ERROR: {e}",file=sys.stderr); return 2
    if obj.get("schema")!="trollfx.asr10-es5510-host-capture.v1": print("ERROR: unsupported capture schema",file=sys.stderr); return 2
    print("# Research candidate; not validated factory code")
    for row in obj.get("instructions",[]):
        word=int(row["word_hex"],16); d=decode_word(word)
        print(f"{row['index']:03d} {word:012X} A={d['a']:02X} B={d['b']:02X} C={d['c']:02X} D={d['d']:02X} ALU={d['alu_op']} SEL={d['operand_select']:X} SKIP={int(d['skip'])} MAC={int(d['mac_accumulate'])} RAM={d['ram_control']} RES={d['reserved_low_bits']:X}")
    return 0
if __name__=="__main__": raise SystemExit(main())
