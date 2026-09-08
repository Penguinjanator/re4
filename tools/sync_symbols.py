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
    if s.startswith("t"):  # template class: t<len>Name<nargs><args>  (`Z<type>` per type argument)
        m = re.match(r"t(\d+)", s); n = int(m.group(1)); s = s[m.end():]
        name, s = s[:n], s[n:]
        m = re.match(r"(\d)", s)
        if m and s[m.end():].startswith("Z"):
            args = []; rest = s[m.end():]
            for _ in range(int(m.group(1))):
                if not rest.startswith("Z"):
                    return name, s  # non-type argument: leave the arguments out
                cls, rest2 = read_class(rest[1:])
                if cls is None:
                    return name, s
                args.append(cls); rest = rest2
            return f"{name}<{', '.join(args)}>", rest
        return name, s
    return None, s

def demangle_v2(sym):
    """Return `Class::Method` / `func` for a GNU v2 mangled name, ignoring argument types."""
    if sym.startswith("_GLOBAL_"):
        return None
    # vtable: _vt.13Class  (Bio4.sym: "Class virtual table")
    m = re.match(r"^_vt\.(.+)$", sym)
    if m:
        cls, _ = read_class(m.group(1))
        return f"{cls} virtual table" if cls else None
    # destructor: _._13Class / _$_13Class
    m = re.match(r"^_[.$]_(.+)$", sym)
    if m:
        cls, _ = read_class(m.group(1))
        return f"{cls}::~{re.sub(r'<.*', '', cls)}" if cls else None
    # constructor: __13Class...
    m = re.match(r"^__(\d+|Q\d|t\d+)(.*)$", sym)
    if m and not sym.startswith("___"):
        cls, _ = read_class(sym[2:])
        if cls:
            return f"{cls}::{re.sub(r'<.*', '', cls)}"
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

def elf_symbols(obj, with_value=False):
    """Read the ELF32 (big-endian) symbol table: list of (name, bind, defined)
    (or (name, bind, defined, shndx, value) with with_value)."""
    import struct
    data = open(obj, "rb").read()
    shoff, = struct.unpack_from(">I", data, 0x20)
    shentsize, shnum = struct.unpack_from(">HH", data, 0x2E)
    shdrs = [struct.unpack_from(">IIIIIIIIII", data, shoff + i * shentsize) for i in range(shnum)]
    syms = []
    for sh in shdrs:
        if sh[1] != 2:  # SHT_SYMTAB
            continue
        strtab = shdrs[sh[6]]
        for off in range(sh[4], sh[4] + sh[5], 16):
            st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from(">IIIBBH", data, off)
            if st_name == 0 or (st_info & 0xF) == 3:  # unnamed / STT_SECTION
                continue
            end = data.index(b"\0", strtab[4] + st_name)
            name = data[strtab[4] + st_name:end].decode()
            if with_value:
                syms.append((name, st_info >> 4, st_shndx != 0, st_shndx, st_value))
            else:
                syms.append((name, st_info >> 4, st_shndx != 0))
    return syms

def other_source_references(name, unit):
    """True when a source file other than `unit` mentions `name` as a whole word."""
    pat = re.compile(r"\b" + re.escape(name) + r"\b")
    for dirpath, _, files in os.walk(os.path.join(ROOT, "src")):
        for fn in files:
            if not fn.endswith((".c", ".cpp", ".h", ".s")):
                continue
            path = os.path.join(dirpath, fn)
            if os.path.relpath(path, os.path.join(ROOT, "src")) == unit:
                continue
            try:
                if pat.search(open(path, errors="replace").read()):
                    return True
            except OSError:
                pass
    return False

def main():
    symmap = {}  # (unit, demangled) -> list of (address, current name)
    unit_of = {}  # address -> owning split unit
    rows = []
    with open(os.path.join(CFG, "sym_map.tsv")) as f:
        next(f)
        for line in f:
            addr, size, sec, unit, scope, name, dn = line.rstrip("\n").split("\t")
            symmap.setdefault((unit, dn), []).append(int(addr, 16))
            unit_of[int(addr, 16)] = unit
    symtxt_path = os.path.join(CFG, "symbols.txt")
    lines = open(symtxt_path).read().splitlines()
    by_addr = {}
    for i, l in enumerate(lines):
        m = re.match(r"^(\S+) = (\.\w+):0x([0-9A-F]+);", l)
        if m: by_addr[int(m.group(3), 16)] = i
    by_dn = {}  # demangled -> list of addresses (all units), for undefined references
    for (unit, dn), addrs in symmap.items():
        by_dn.setdefault(dn, []).extend(addrs)
    renamed = 0

    def rename(i, name):
        nonlocal renamed
        old = lines[i].split(" = ")[0]
        if old != name:
            # never create a duplicate: a global of that name defined elsewhere would make the split
            # objects' relocations resolve to the wrong address (this is a local of another unit)
            for j, l in enumerate(lines):
                if j != i and l.startswith(name + " = "):
                    print(f"  {old}: keeping (name {name} already defined at line {j+1}; this one is a local duplicate)")
                    if "scope:global" in lines[i]:
                        lines[i] = lines[i].replace("scope:global", "scope:local"); renamed += 1
                    return
            lines[i] = name + lines[i][len(old):]
            print(f"  {old} -> {name}")
            renamed += 1

    def make_global(i):
        # A symbol defined global by the compiler, or referenced from another unit, cannot be local.
        nonlocal renamed
        if "scope:local" in lines[i]:
            lines[i] = lines[i].replace("scope:local", "scope:global")
            print(f"  {lines[i].split(' = ')[0]}: scope local -> global")
            renamed += 1

    for obj in sys.argv[1:]:
        unit = os.path.relpath(obj, os.path.join(ROOT, "build", VER, "src")).rsplit(".", 1)[0]
        unit = unit + (".cpp" if os.path.exists(os.path.join(ROOT, "src", unit + ".cpp")) else ".c")
        if not any(u == unit for (u, _) in symmap) and any(u == unit[:-2] + ".cpp" for (u, _) in symmap):
            unit = unit[:-2] + ".cpp"  # .c source for a split unit named *.cpp
        # undefined names of the split object: disambiguates overloads (several sym_map entries share
        # one demangled name) — the overload this unit calls is the one the split object imports
        split_obj = os.path.join(ROOT, "build", VER, "obj", unit.rsplit(".", 1)[0] + ".o")
        split_und = {n for n, _, d in elf_symbols(split_obj) if not d} if os.path.exists(split_obj) else set()
        # overloads: several sym_map entries of this unit share one demangled name. The compiled
        # object's definitions with that demangled name are paired with them in address order
        # (source order is the original order), provided the counts agree.
        obj_defs = {}
        for name, bind, defined, shndx, value in elf_symbols(obj, with_value=True):
            if defined and not name.startswith(".") and not name.startswith("_GLOBAL_"):
                dn = demangle_v2(name)
                if dn is not None:
                    obj_defs.setdefault(dn, []).append((shndx, value, name, bind))
        overload = {}  # mangled name -> address
        for dn, defs in obj_defs.items():
            cands = symmap.get((unit, dn))
            if cands and len(cands) > 1 and len(cands) == len(defs):
                for (_, _, name, bind), addr in zip(sorted(defs), sorted(cands)):
                    overload[name] = addr
        for name, bind, defined in elf_symbols(obj):
            if name.startswith(".") or name.startswith("@") or name.startswith("_GLOBAL_"):
                continue
            dn = demangle_v2(name)
            if dn is None: continue
            if defined and name in overload:
                i = by_addr.get(overload[name])
                if i is not None:
                    rename(i, name)
                    if bind == 1:
                        make_global(i)
                continue
            if not defined:
                # undefined reference: the target (in whatever unit) must be global and carry this name
                cands = by_dn.get(dn, [])
                if len(cands) > 1:
                    narrowed = [a for a in cands if a in by_addr and lines[by_addr[a]].split(" = ")[0] in split_und]
                    if len(narrowed) == 1:
                        cands = narrowed
                if len(cands) == 1 and cands[0] in by_addr:
                    i = by_addr[cands[0]]
                    old = lines[i].split(" = ")[0]
                    if "__" in name and "__" not in old and old == dn.split("(")[0]:
                        # the target still carries its plain (map) name and this unit references the
                        # C++-mangled name. Renaming is safe only while no other source declares the
                        # plain name (an extern "C" caller would then fail to link: silent ngcld exit 99).
                        if other_source_references(old, unit):
                            print(f"  {old}: referenced as {name}; another source uses the plain name, declare it extern \"C\" (not renamed)")
                            continue
                    owner = unit_of.get(cands[0])
                    if owner and os.path.exists(os.path.join(ROOT, "src", owner)):
                        # the defining unit has source; its own sync is authoritative for the name
                        continue
                    sanitized = re.sub(r"\W+", "_", dn).strip("_")  # Bio4.sym placeholder: `cSatMgr_polySphereCk`
                    if not re.search(r"_[0-9A-F]{8}$", old) and not old.startswith(("fn_", "lbl_")) and old != sanitized:
                        # only placeholders may be renamed from a reference; a real name that differs
                        # from the reference means this unit declared the function with the wrong linkage/signature
                        if old != name:
                            print(f"  {old}: referenced as {name}; fix the declaration in the unit (not renamed)")
                        continue
                    rename(i, name)
                    make_global(i)
                continue
            cands = symmap.get((unit, dn))
            if not cands:
                continue
            if len(cands) > 1:
                print(f"  ambiguous {name} -> {dn}: {[hex(a) for a in cands]} (skipped; rename by hand)")
                continue
            addr = cands[0]
            i = by_addr.get(addr)
            if i is None: continue
            rename(i, name)
            if bind == 1:  # STB_GLOBAL
                make_global(i)
    if renamed:
        open(symtxt_path, "w").write("\n".join(lines) + "\n")
        # keep sym_map.tsv's name column in sync
        cur = {}
        for l in lines:
            m = re.match(r"^(\S+) = (\.\w+):0x([0-9A-F]+);.*scope:(\w+)", l)
            if m: cur[int(m.group(3), 16)] = (m.group(1), m.group(4))
        mp = os.path.join(CFG, "sym_map.tsv"); rows = open(mp).read().splitlines()
        out = [rows[0]]
        for l in rows[1:]:
            f = l.split("\t"); a = int(f[0], 16)
            if a in cur: f[5], f[4] = cur[a]
            out.append("\t".join(f))
        open(mp, "w").write("\n".join(out) + "\n")
    print(f"{renamed} symbols renamed; re-run `python3 configure.py && ninja`")

if __name__ == "__main__":
    main()
