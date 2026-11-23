#!/usr/bin/env python3
"""Apply 5510 Single's per-tab Q-Link banks and page tints after SDK UI generation."""
import json
import sys
from pathlib import Path

class JObject(list):
    pass

def get(obj, key):
    return next(value for name, value in obj if name == key)

def set_value(obj, key, value):
    for i, (name, _) in enumerate(obj):
        if name == key:
            obj[i] = (name, value)
            return
    raise KeyError(key)

def encode(value, level=0):
    pad = " " * level
    inner = " " * (level + 2)
    if isinstance(value, JObject):
        if not value: return "{}"
        return "{\n" + ",\n".join(inner + json.dumps(k) + ": " + encode(v, level + 2) for k, v in value) + "\n" + pad + "}"
    if isinstance(value, list):
        if not value: return "[]"
        return "[\n" + ",\n".join(inner + encode(v, level + 2) for v in value) + "\n" + pad + "]"
    return json.dumps(value, ensure_ascii=False)

def main(root: Path) -> None:
    skins = root / "Plugin Skins"
    tui_path = skins / "TUI.json"
    tui = json.loads(tui_path.read_text(encoding="utf-8"), object_pairs_hook=JObject)
    tabs = get(get(tui, "pageData"), "tabs")
    if len(tabs) != 2:
        raise SystemExit("expected exactly the DELAY and CHORUS tabs")
    for tab, colour in zip(tabs, ("ff21180e", "ff101d25")):
        data = get(get(tab, "componentDefinition"), "backgroundData")
        for side_name in ("focussed", "unfocussed"):
            side = get(data, side_name)
            set_value(side, "colour", colour)
    tui_path.write_text(encode(tui) + "\n", encoding="utf-8")

    q_path = skins / "Q-Links.json"
    q = json.loads(q_path.read_text(encoding="utf-8"))
    maps = q["Screen Mode Q-Links"]["map"]
    if len(maps) != 2:
        raise SystemExit("expected Q-Link maps for two tabs")
    # V0.2 keeps the SDK's Q-Link format and assigns the first eight physical
    # Q-Links explicitly per tab. All indices preserve V0.1's 0..6 order.
    assignments = [
        {1:1, 2:2, 3:5, 4:7, 5:8, 6:10, 7:6, 8:0, 9:9, 10:12},
        {1:3, 2:4, 3:5, 4:11, 5:1, 7:6, 8:0},
    ]
    for item, mapping in zip(maps, assignments):
        qmap = item["Q-Links"]
        for n in range(1, 17): qmap[f"Q-Link {n}"] = mapping.get(n, -1)
    q_path.write_text(json.dumps(q, indent=2) + "\n", encoding="utf-8")

if __name__ == "__main__":
    if len(sys.argv) != 2: raise SystemExit("usage: finish_ui.py <built plugin folder>")
    main(Path(sys.argv[1]))
