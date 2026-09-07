#!/usr/bin/env python3
"""Show what a unit contains and how well it matches.

usage: unit_info.py <unit>            e.g. unit_info.py game/cam_ctrl   (or game/cam_ctrl.cpp)
       unit_info.py --list [prefix]   list units with size and match %
"""
import json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VER = os.environ.get("RE4_VERSION", "G4BE08")
REPORT = os.path.join(ROOT, "build", VER, "report.json")

def load_report():
    if not os.path.exists(REPORT):
        subprocess.run([os.path.join(ROOT, "build/tools/objdiff-cli"), "report", "generate", "-p", ROOT, "-o", REPORT], capture_output=True)
    return json.load(open(REPORT))

def main():
    r = load_report()
    if len(sys.argv) > 1 and sys.argv[1] == "--list":
        prefix = sys.argv[2] if len(sys.argv) > 2 else ""
        rows = []
        for u in r["units"]:
            name = u["name"].split("/", 1)[1]
            if not name.startswith(prefix): continue
            m = u["measures"]
            rows.append((int(m.get("total_code", 0)), name, m.get("fuzzy_match_percent", 0.0), m.get("matched_functions", 0), m.get("total_functions", 0)))
        for size, name, pct, mf, tf in sorted(rows):
            print(f"{size:7d} bytes  {pct:6.2f}%  {mf:3d}/{tf:<3d} funcs  {name}")
        return
    unit = re.sub(r"\.(cpp|c)$", "", sys.argv[1])
    symmap = {}
    with open(os.path.join(ROOT, "config", VER, "sym_map.tsv")) as f:
        next(f)
        for line in f:
            addr, size, sec, uname, scope, name, dn = line.rstrip("\n").split("\t")
            if re.sub(r"\.(cpp|c)$", "", uname) == unit:
                symmap[name] = (addr, size, sec, scope, dn)
    match = {}
    for u in r["units"]:
        if u["name"].split("/", 1)[1] == unit:
            for fn in u.get("functions", []):
                match[fn["name"]] = fn.get("fuzzy_match_percent", fn.get("measures", {}).get("fuzzy_match_percent", 0.0))
    print(f"unit {unit}: {len(symmap)} symbols")
    for sec in (".text", ".ctor", ".dtor", ".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2"):
        rows = [(v[0], k, v) for k, v in symmap.items() if v[2] == sec]
        if not rows: continue
        print(f"\n[{sec}]")
        for addr, name, (a, size, s, scope, dn) in sorted(rows):
            pct = match.get(name)
            pcts = f"{pct:6.2f}%" if pct is not None else "       "
            extra = f"  ({dn})" if dn != name and dn not in (".",) else ""
            print(f"  {addr} {size:>7} {scope[0]} {pcts} {name}{extra}")

if __name__ == "__main__":
    main()
