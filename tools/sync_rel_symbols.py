#!/usr/bin/env python3
"""sync_symbols.py for REL module units: rename the placeholder names in the module symbol files to
the mangled names a compiled module object defines or references.

usage: sync_rel_symbols.py build/<ver>/src/<mod>/<unit>.o [more .o ...]

Defined symbols rename entries of the unit's own module (config/<ver>/modules/<mod>/symbols.txt, matched
through sym_map.tsv by unit + demangled name). Undefined symbols rename the placeholder of the
definition make_rel.py will pick: the DOL (config/<ver>/symbols.txt, placeholders only) first, then the
imported modules by ascending module id.
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from sync_symbols import atomic_write, demangle_v2, elf_symbols  # noqa: E402
from symnames import sanitize  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VER = os.environ.get("RE4_VERSION", "G4BE08")
CFG = os.path.join(ROOT, "config", VER)
MODULES = os.path.join(CFG, "modules")
SYM_RE = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-F]+);(.*)$")
KNOWN_SIZE = {}  # mangled name -> .text size, from every sym_map row already carrying that name


class SymbolFile:
    """A symbols.txt plus its sym_map.tsv; keys are (section, offset) for modules, (None, address) for the DOL."""

    def __init__(self, path, map_path, is_dol):
        self.path, self.map_path, self.is_dol = path, map_path, is_dol
        self.lines = open(path).read().splitlines()
        self.by_key = {}
        for i, l in enumerate(self.lines):
            m = SYM_RE.match(l)
            if m:
                self.by_key[(None if is_dol else m.group(2), int(m.group(3), 16))] = i
        self.by_dn = {}  # demangled -> [(key, unit)]
        self.size = {}  # key -> size
        self.map_rows = open(map_path).read().splitlines()
        for l in self.map_rows[1:]:
            f = l.split("\t")
            if is_dol:
                key, unit, dn = (None, int(f[0], 16)), f[3], f[6]
            else:
                key, unit, dn = (f[0], int(f[1], 16)), f[3], f[6]
            self.by_dn.setdefault(dn, []).append((key, unit))
            size = int(f[1] if is_dol else f[2], 16)
            self.size[key] = size
            # a row that already carries a mangled name tells that name's size (template instantiations
            # are the same code in the DOL and in every module), which tells overloads apart
            if f[5] != sanitize(dn) and not f[5].startswith(("fn_", "lbl_")) and f[5] != dn:
                KNOWN_SIZE.setdefault(f[5], size)
        self.changed = 0

    def is_placeholder(self, key, dn):
        """True when the entry still carries the generated name (sanitised demangled name, optionally
        with the `_<offset>` suffix of a duplicate, or fn_/lbl_)."""
        old = self.name(key)
        base = sanitize(dn)
        return old == base or old.startswith((base + "_", "fn_", "lbl_"))

    def name(self, key):
        return self.lines[self.by_key[key]].split(" = ")[0]

    def rename(self, key, name, make_global):
        i = self.by_key[key]
        old = self.name(key)
        if old != name:
            if any(l.startswith(name + " = ") for l in self.lines):
                print(f"  {old}: keeping ({name} already defined elsewhere in {os.path.relpath(self.path, ROOT)})")
                return
            self.lines[i] = name + self.lines[i][len(old):]
            print(f"  {os.path.relpath(self.path, ROOT)}: {old} -> {name}")
            self.changed += 1
        if make_global and "scope:local" in self.lines[i]:
            self.lines[i] = self.lines[i].replace("scope:local", "scope:global")
            print(f"  {name}: scope local -> global")
            self.changed += 1

    def save(self):
        if not self.changed:
            return
        atomic_write(self.path, "\n".join(self.lines) + "\n")
        cur = {}
        for l in self.lines:
            m = SYM_RE.match(l)
            if m:
                scope = re.search(r"scope:(\w+)", m.group(4))
                cur[(None if self.is_dol else m.group(2), int(m.group(3), 16))] = (m.group(1), scope.group(1) if scope else "global")
        out = [self.map_rows[0]]
        for l in self.map_rows[1:]:
            f = l.split("\t")
            key = (None, int(f[0], 16)) if self.is_dol else (f[0], int(f[1], 16))
            if key in cur:
                f[5], f[4] = cur[key]
            out.append("\t".join(f))
        atomic_write(self.map_path, "\n".join(out) + "\n")


def main():
    dol = SymbolFile(os.path.join(CFG, "symbols.txt"), os.path.join(CFG, "sym_map.tsv"), True)
    modules = {}
    for name in sorted(os.listdir(MODULES)):
        d = os.path.join(MODULES, name)
        if os.path.isfile(os.path.join(d, "sym_map.tsv")):
            modules[name] = SymbolFile(os.path.join(d, "symbols.txt"), os.path.join(d, "sym_map.tsv"), False)
            modules[name].info = json.load(open(os.path.join(d, "rel.json")))

    for obj in sys.argv[1:]:
        rel = os.path.relpath(obj, os.path.join(ROOT, "build", VER, "src"))
        mod, stem = rel.split(os.sep, 1)[0], rel.rsplit(".", 1)[0]
        if mod not in modules:
            sys.exit(f"{obj}: {mod} is not a REL module (config/{VER}/modules/{mod}/ missing)")
        own = modules[mod]
        # the unit name as in splits.txt (the source may be shared, e.g. st2_0/st2.cpp -> src/st2/st2.cpp)
        units = {u for lst in own.by_dn.values() for _, u in lst}
        unit = next((u for u in units if u.rsplit(".", 1)[0] == stem), stem + ".cpp")
        # resolution order for undefined names: the module's other units (ngcld -r), then make_rel.py's
        # order (DOL, imported modules by ascending id)
        order = [own, dol] + [modules[n] for n in sorted(own.info["links"], key=lambda n: modules[n].info["module_id"])]
        # names whose size is known first, so an overload with a known size claims the placeholder before
        # an unknown one could
        for name, bind, defined in sorted(elf_symbols(obj), key=lambda t: t[0] not in KNOWN_SIZE):
            if name.startswith((".", "@", "_GLOBAL_")):
                continue
            dn = demangle_v2(name)
            if dn is None:
                continue
            if defined:
                cands = [k for k, u in own.by_dn.get(dn, []) if u == unit]
                if len(cands) != 1:
                    if len(cands) > 1:
                        print(f"  ambiguous {name} -> {dn} in {unit}: {cands} (rename by hand)")
                    continue
                own.rename(cands[0], name, bind == 1)
                continue
            for sf in order:
                cands = [k for k, _ in sf.by_dn.get(dn, [])]
                if not cands:
                    continue
                if any(sf.name(k) == name for k in cands):
                    break  # already named
                if not sf.is_dol:
                    # overloads share one demangled name (the .sym has no parameter lists): a known size
                    # of the mangled name selects the candidate, or says the module has no copy of it
                    # (a linkonce duplicate this object dropped: then it is not referenced either)
                    if name in KNOWN_SIZE:
                        cands = [k for k in cands if sf.size.get(k) == KNOWN_SIZE[name]]
                        if not cands:
                            continue
                    cands = [k for k in cands if sf.is_placeholder(k, dn)]
                    if not cands:
                        print(f"  {name} -> {dn}: every candidate in {os.path.relpath(sf.path, ROOT)} already carries another mangled name (overload?), left alone")
                        break
                if len(cands) > 1:
                    print(f"  ambiguous reference {name} -> {dn}: {cands} (rename by hand)")
                    break
                old = sf.name(cands[0])
                if sf.is_dol and not (old == sanitize(dn) or old.startswith(("fn_", "lbl_"))):
                    if old != name:
                        print(f"  {old}: referenced as {name}; the DOL name is not a placeholder (declare it that way or fix the reference)")
                    break
                sf.rename(cands[0], name, True)
                break
    for sf in [dol, *modules.values()]:
        sf.save()
    print(f"{sum(sf.changed for sf in [dol, *modules.values()])} symbols changed; re-run `python3 configure.py && ninja`")


if __name__ == "__main__":
    main()
