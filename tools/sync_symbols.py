#!/usr/bin/env python3
"""Rename symbols in config/<ver>/symbols.txt to the mangled names produced by the compiler.

The debug build's Bio4.sym only carries demangled names (e.g. `CameraControl::Check`), and
config/symbols.txt initially holds sanitized placeholders (`CameraControl_Check`). objdiff and the
linker match symbols by name, so once a unit is compiled we read its ELF symbol table, demangle
each GNU v2 (gcc 2.95) name back to `Class::Method` and rename the placeholder at the matching
address.

usage: sync_symbols.py <compiled .o> [more .o ...]       (paths under build/<ver>/src/)
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VER = os.environ.get("RE4_VERSION", "G4BE08")
CFG = os.path.join(ROOT, "config", VER)
DTK = os.path.join(ROOT, "build", "tools", "dtk")

OPS = {"eq": "==", "ne": "!=", "lt": "<", "gt": ">", "le": "<=", "ge": ">=", "pl": "+", "mi": "-", "ml": "*", "dv": "/",
       "md": "%", "as": "=", "apl": "+=", "ami": "-=", "aml": "*=", "adv": "/=", "vc": "[]", "cl": "()", "nw": "new",
       "dl": "delete", "nt": "!", "ad": "&", "or": "|", "er": "^", "ls": "<<", "rs": ">>", "rf": "->", "pp": "++", "mm": "--",
       "aor": "|=", "aad": "&=", "aer": "^=", "als": "<<=", "ars": ">>=", "cm": ",", "oo": "||", "aa": "&&", "co": "~"}

def read_class(s):
    """Parse a class name: `13CameraControl` or `Q2<..>`; returns (name, rest)."""
    m = re.match(r"(\d+)", s)
    if m:
        n = int(m.group(1)); s = s[m.end():]
        return s[:n], s[n:]
    m = re.match(r"Q(\d)", s)
    if m:
        parts = []; s = s[m.end():]
        for _ in range(int(m.group(1))):
            p, s = read_class(s); parts.append(p)
        return "::".join(parts), s
    if s.startswith("t"):  # template class: t<len>Name<nargs>...  (args ignored)
        m = re.match(r"t(\d+)", s); n = int(m.group(1)); s = s[m.end():]
        return s[:n], s[n:]
    return None, s

def demangle_v2(sym):
    """Return `Class::Method` / `func` for a GNU v2 mangled name, ignoring argument types."""
    if sym.startswith("_GLOBAL_"):
        return None
    # destructor: _._13Class / _$_13Class
    m = re.match(r"^_[.$]_(.+)$", sym)
    if m:
        cls, _ = read_class(m.group(1))
        return f"{cls}::~{cls}" if cls else None
    # constructor: __13Class...
    m = re.match(r"^__(\d+|Q\d|t\d+)(.*)$", sym)
    if m and not sym.startswith("___"):
        cls, _ = read_class(sym[2:])
        if cls:
            return f"{cls}::{cls}"
    # operator: __eq__13Class or __eq__Fii
    m = re.match(r"^__([a-z]{2,3})__(.*)$", sym)
    if m and m.group(1) in OPS:
        rest = m.group(2)
        if rest.startswith("F"):
            return f"operator{OPS[m.group(1)]}"
        cls, _ = read_class(rest)
        return f"{cls}::operator{OPS[m.group(1)]}" if cls else None
    # method: Name__13Class<args>  /  function: Name__F<args>  /  static member: Name__13Class (no args)
    m = re.match(r"^(.+?)__(.*)$", sym)
    if m and not sym.startswith("__"):
        name, rest = m.group(1), m.group(2)
        if rest.startswith("F") or rest.startswith("H"):
            return name  # free function with args
        cls, _ = read_class(rest)
        if cls:
            return f"{cls}::{name}"
        return None
    return sym  # plain C symbol

def elf_symbols(obj):
    out = subprocess.run([DTK, "elf", "info", obj], capture_output=True, text=True).stdout
    syms = []
    in_syms = False
    for line in out.splitlines():
        if line.startswith("Symbols:"): in_syms = True; continue
        if in_syms:
            if not line.strip() or line.startswith("Relocations") or line.startswith("Metrowerks") or line.startswith("Split"):
                if line.strip() and not line.strip().startswith("Section"): in_syms = False
                continue
            parts = [p.strip() for p in line.split("|")]
            if len(parts) >= 4 and parts[0].startswith("."):
                syms.append((parts[0], int(parts[1], 16), int(parts[2], 16), parts[3]))
    return syms

def main():
    symmap = {}  # (unit, demangled) -> list of (address, current name)
    rows = []
    with open(os.path.join(CFG, "sym_map.tsv")) as f:
        next(f)
        for line in f:
            addr, size, sec, unit, scope, name, dn = line.rstrip("\n").split("\t")
            symmap.setdefault((unit, dn), []).append(int(addr, 16))
    symtxt_path = os.path.join(CFG, "symbols.txt")
    lines = open(symtxt_path).read().splitlines()
    by_addr = {}
    for i, l in enumerate(lines):
        m = re.match(r"^(\S+) = (\.\w+):0x([0-9A-F]+);", l)
        if m: by_addr[int(m.group(3), 16)] = i
    renamed = 0
    for obj in sys.argv[1:]:
        unit = os.path.relpath(obj, os.path.join(ROOT, "build", VER, "src")).rsplit(".", 1)[0]
        unit = unit + (".cpp" if os.path.exists(os.path.join(ROOT, "src", unit + ".cpp")) else ".c")
        for sec, off, size, name in elf_symbols(obj):
            if name.startswith(".") or name.startswith("@") or name.startswith("_GLOBAL_"):
                continue
            dn = demangle_v2(name)
            if dn is None: continue
            cands = symmap.get((unit, dn))
            if not cands:
                continue
            if len(cands) > 1:
                print(f"  ambiguous {name} -> {dn}: {[hex(a) for a in cands]} (skipped; rename by hand)")
                continue
            addr = cands[0]
            i = by_addr.get(addr)
            if i is None: continue
            old = lines[i].split(" = ")[0]
            if old != name:
                lines[i] = name + lines[i][len(old):]
                print(f"  {old} -> {name}")
                renamed += 1
    if renamed:
        open(symtxt_path, "w").write("\n".join(lines) + "\n")
    print(f"{renamed} symbols renamed; re-run `python3 configure.py && ninja`")

if __name__ == "__main__":
    main()
