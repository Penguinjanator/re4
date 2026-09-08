#!/usr/bin/env python3
"""Fold GCC 2.95 .gnu.linkonce.* sections of a compiled game object into what the original link produced.

The game's SN linker handled linkonce sections in a way ngcld (with our linker script) does not:
  * .gnu.linkonce.d.* / .r.*  (vtables of template / key-function-less classes): every unit's copy
    was kept, appended to that unit's .rodata in emission order; the weak symbol resolves to the
    first copy program-wide. We append them to .rodata here (symbols stay weak).
  * .gnu.linkonce.t.*  (template instantiations, out-of-line inline functions, implicit
    destructors): exactly one copy exists in the DOL, appended to the owning unit's .text in
    emission order. A linkonce function that sym_map.tsv lists for this unit is appended to
    .text here; every other copy is dropped and its symbols become weak undefined references
    (resolved to the owning unit's copy at link time).

usage: fold_linkonce.py --unit game/foo.cpp <object.o>     (rewrites the object in place)
"""
import argparse
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sync_symbols import demangle_v2  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VER = os.environ.get("RE4_VERSION", "G4BE08")

SHT_NULL = 0
SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4
SHT_NOBITS = 8
SHF_ALLOC = 2
STT_SECTION = 3
STB_WEAK = 2
SHN_UNDEF = 0


class Elf:
    def __init__(self, data):
        self.data = bytearray(data)
        (self.shoff,) = struct.unpack(">I", data[0x20:0x24])
        (self.shentsize, self.shnum, self.shstrndx) = struct.unpack(">HHH", data[0x2E:0x34])
        self.sections = []
        for i in range(self.shnum):
            off = self.shoff + i * self.shentsize
            self.sections.append(list(struct.unpack(">IIIIIIIIII", data[off : off + 40])))
        self.contents = [bytearray(self.sec_bytes(i)) for i in range(self.shnum)]
        self.names = [self.shstr_name(sh[0]) for sh in self.sections]

    def shstr_name(self, off):
        shstr = self.contents[self.shstrndx] if hasattr(self, "contents") else self.sec_bytes(self.shstrndx)
        end = shstr.index(b"\0", off)
        return shstr[off:end].decode()

    def sec_bytes(self, i):
        sh = self.sections[i]
        if sh[1] == SHT_NOBITS:
            return b"\0" * sh[5]
        return self.data[sh[4] : sh[4] + sh[5]]

    def add_section(self, name, shtype, flags, link=0, info=0, align=1, entsize=0):
        shstr = self.contents[self.shstrndx]
        name_off = len(shstr)
        shstr += name.encode() + b"\0"
        self.sections.append([name_off, shtype, flags, 0, 0, 0, link, info, align, entsize])
        self.contents.append(bytearray())
        self.names.append(name)
        self.shnum += 1
        return self.shnum - 1

    def write(self, path):
        out = bytearray(self.data[:0x34])
        pos = 0x34
        for i in range(1, self.shnum):
            sh = self.sections[i]
            body = self.contents[i]
            align = max(sh[8], 1)
            pos = (pos + align - 1) // align * align
            sh[4] = pos
            sh[5] = len(body)
            if sh[1] != SHT_NOBITS:
                out += b"\0" * (pos - len(out))
                out += body
                pos += len(body)
        pos = (pos + 3) // 4 * 4
        out += b"\0" * (pos - len(out))
        struct.pack_into(">I", out, 0x20, pos)
        struct.pack_into(">HH", out, 0x30, self.shnum, self.shstrndx)
        for sh in self.sections:
            out += struct.pack(">IIIIIIIIII", *sh)
        with open(path, "wb") as f:
            f.write(out)


def unit_text_functions(unit):
    """Demangled names of the .text functions the DOL has in this unit."""
    names = set()
    with open(os.path.join(ROOT, "config", VER, "sym_map.tsv")) as f:
        next(f)
        for line in f:
            addr, size, sec, u, scope, name, dn = line.rstrip("\n").split("\t")
            if u == unit and sec == ".text":
                dn = dn if dn and dn != "." else name
                names.add(dn)
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--unit", required=True)
    ap.add_argument("object")
    args = ap.parse_args()
    path = args.object
    elf = Elf(open(path, "rb").read())
    if not any(n.startswith(".gnu.linkonce.") for n in elf.names):
        return
    owned = unit_text_functions(args.unit)

    symtab = elf.names.index(".symtab")
    syms = [list(struct.unpack(">IIIBBH", elf.contents[symtab][o : o + 16])) for o in range(0, len(elf.contents[symtab]), 16)]
    strtab = elf.sections[symtab][6]

    def sym_name(s):
        strs = elf.contents[strtab]
        return strs[s[0] : strs.index(b"\0", s[0])].decode()

    def rela_for(shndx):
        for i, sh in enumerate(elf.sections):
            if sh[1] == SHT_RELA and sh[7] == shndx:
                return i
        return None

    def section_symbol(shndx):
        for i, s in enumerate(syms):
            if (s[3] & 0xF) == STT_SECTION and s[5] == shndx:
                return i
        return None

    rodata = elf.names.index(".rodata") if ".rodata" in elf.names else None
    dead = set()

    for i, name in enumerate(elf.names):
        if not name.startswith(".gnu.linkonce."):
            continue
        kind = name.split(".")[3]
        if kind in ("d", "r"):
            if rodata is None:
                rodata = elf.add_section(".rodata", SHT_PROGBITS, SHF_ALLOC, align=8)
            body = elf.contents[rodata]
            align = max(elf.sections[i][8], 1)
            base = (len(body) + align - 1) // align * align
            body += b"\0" * (base - len(body))
            body += elf.contents[i]
            elf.sections[rodata][8] = max(elf.sections[rodata][8], align)
            for s in syms:
                if s[5] == i and (s[3] & 0xF) != STT_SECTION:
                    s[5] = rodata
                    s[1] += base
            rela = rela_for(i)
            if rela is not None:
                dst = rela_for(rodata)
                if dst is None:
                    dst = elf.add_section(".rela.rodata", SHT_RELA, 0, link=symtab, info=rodata, align=4, entsize=12)
                out = bytearray()
                for o in range(0, len(elf.contents[rela]), 12):
                    r_off, r_info, r_add = struct.unpack(">IIi", elf.contents[rela][o : o + 12])
                    sym, rtype = r_info >> 8, r_info & 0xFF
                    if (syms[sym][3] & 0xF) == STT_SECTION and syms[sym][5] == i:
                        # section-relative reference into the moved section
                        rsym = section_symbol(rodata)
                        if rsym is None:
                            sys.exit(f"fold_linkonce: {path}: no section symbol for .rodata")
                        sym = rsym
                        r_add += base
                    out += struct.pack(">IIi", r_off + base, (sym << 8) | rtype, r_add)
                elf.contents[dst] += out
                dead.add(rela)
            dead.add(i)
        elif kind == "t":
            funcs = [s for s in syms if s[5] == i and (s[3] & 0xF) != STT_SECTION]
            if any((demangle_v2(sym_name(s)) or sym_name(s)) in owned for s in funcs):
                # this unit owns the only copy: append to .text
                text = elf.names.index(".text")
                body = elf.contents[text]
                align = max(elf.sections[i][8], 4)
                base = (len(body) + align - 1) // align * align
                body += b"\0" * (base - len(body))
                body += elf.contents[i]
                for s in funcs:
                    s[5] = text
                    s[1] += base
                rela = rela_for(i)
                if rela is not None:
                    dst = rela_for(text)
                    if dst is None:
                        dst = elf.add_section(".rela.text", SHT_RELA, 0, link=symtab, info=text, align=4, entsize=12)
                    out = bytearray()
                    for o in range(0, len(elf.contents[rela]), 12):
                        r_off, r_info, r_add = struct.unpack(">IIi", elf.contents[rela][o : o + 12])
                        sym, rtype = r_info >> 8, r_info & 0xFF
                        if (syms[sym][3] & 0xF) == STT_SECTION and syms[sym][5] == i:
                            sym = section_symbol(text)
                            r_add += base
                        out += struct.pack(">IIi", r_off + base, (sym << 8) | rtype, r_add)
                    elf.contents[dst] += out
                    dead.add(rela)
                dead.add(i)
                continue
            rela = rela_for(i)
            if rela is not None:
                dead.add(rela)
            for s in funcs:
                s[5] = SHN_UNDEF
                s[1] = 0
                s[2] = 0
                s[3] = (STB_WEAK << 4) | (s[3] & 0xF)
            dead.add(i)
        else:
            sys.exit(f"fold_linkonce: {path}: unhandled section {name}")

    # References from kept sections to section symbols of dropped text sections are an error.
    for i, sh in enumerate(elf.sections):
        if sh[1] != SHT_RELA or i in dead:
            continue
        for o in range(0, len(elf.contents[i]), 12):
            _, r_info, _ = struct.unpack(">IIi", elf.contents[i][o : o + 12])
            s = syms[r_info >> 8]
            if (s[3] & 0xF) == STT_SECTION and s[5] in dead:
                sys.exit(f"fold_linkonce: {path}: {elf.names[sh[7]]} references dropped section {elf.names[s[5]]}")

    # Drop dead sections and their section symbols; renumber everything.
    keep = [i for i in range(elf.shnum) if i not in dead]
    remap_sec = {old: new for new, old in enumerate(keep)}
    new_syms = []
    remap_sym = {}
    for i, s in enumerate(syms):
        if (s[3] & 0xF) == STT_SECTION and s[5] in dead:
            continue
        if 0 < s[5] < 0xFF00:
            s[5] = remap_sec[s[5]]
        remap_sym[i] = len(new_syms)
        new_syms.append(s)
    # ELF wants locals first; weak-undef conversions keep their position, which may violate
    # that if a local symbol lived in a dropped section (none expected). Recompute sh_info.
    first_global = next((k for k, s in enumerate(new_syms) if (s[3] >> 4) != 0), len(new_syms))
    elf.contents[symtab] = bytearray().join(struct.pack(">IIIBBH", *s) for s in new_syms)
    elf.sections[symtab][7] = first_global

    for i, sh in enumerate(elf.sections):
        if i in dead or sh[1] != SHT_RELA:
            continue
        out = bytearray()
        for o in range(0, len(elf.contents[i]), 12):
            r_off, r_info, r_add = struct.unpack(">IIi", elf.contents[i][o : o + 12])
            out += struct.pack(">IIi", r_off, (remap_sym[r_info >> 8] << 8) | (r_info & 0xFF), r_add)
        elf.contents[i] = out

    elf.sections = [elf.sections[i] for i in keep]
    elf.contents = [elf.contents[i] for i in keep]
    elf.names = [elf.names[i] for i in keep]
    for sh in elf.sections:
        if sh[1] in (SHT_RELA, SHT_SYMTAB):
            sh[6] = remap_sec[sh[6]]
        if sh[1] == SHT_RELA:
            sh[7] = remap_sec[sh[7]]
    elf.shstrndx = remap_sec[elf.shstrndx]
    elf.shnum = len(elf.sections)
    elf.write(path)


if __name__ == "__main__":
    main()
