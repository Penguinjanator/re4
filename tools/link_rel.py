#!/usr/bin/env python3
"""Link a REL module's objects into a relocatable ELF with ProDG's ngcld -r.

usage: link_rel.py <linker> <wrapper> <sn_ngc_path> <ldscript> <out.elf> <rsp>

Split objects (anything under a .../obj/ directory) are first copied next to the output with their
R_PPC_REL14 relocations rewritten to be section-relative: dtk emits them against the containing
function symbol, and ngcld -r patches a relocation against a global symbol with A - P (it only adds
S for local symbols), which is out of the 16-bit range for anything but tiny modules. NgcAs expresses
every conditional branch as `.text + offset`, so the rewrite also makes the split objects look like
assembler output. Compiled objects are linked as they are.

After the link the ADDR32/ADDR16* fields of relocations against GLOBAL/WEAK symbols defined in the
module are reset to the addend: the original linker left those fields as the compiler wrote them (A;
the debug RELs show field 0 for every global in a displaced section, e.g. t_util's flag backups in
t_emlist/t_light) while ngcld 3.9.3 -r adds the symbol's input-section displacement (field = disp + A
for globals, S + A for locals). Local symbols already come out as S + A, like the original. Nothing
changes for a module whose globals all sit in the first input section of their kind (displacement 0),
which is every single-object module.
"""
import os
import shutil
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
import elffile  # noqa: E402


def fix_split_object(src: str, dst: str) -> None:
    elf = elffile.Elf(src)
    data = bytearray(open(src, 'rb').read())
    section_sym = {s.shndx: s.index for s in elf.symbols if s.type == elffile.STT_SECTION}
    for rela_sec in elf.sections:
        if rela_sec.type != elffile.SHT_RELA:
            continue
        for i in range(rela_sec.size // 12):
            pos = rela_sec.offset + 12 * i
            off, info, addend = struct.unpack('>IIi', data[pos:pos + 12])
            if info & 0xFF != 11:  # R_PPC_REL14
                continue
            sym = elf.symbols[info >> 8]
            if sym.type == elffile.STT_SECTION or sym.shndx != rela_sec.info:
                continue
            sec_sym = section_sym[sym.shndx]
            data[pos:pos + 12] = struct.pack('>IIi', off, (sec_sym << 8) | 11, sym.value + addend)
    with open(dst, 'wb') as f:
        f.write(data)


R_PPC_ADDR32, R_PPC_ADDR16, R_PPC_ADDR16_LO, R_PPC_ADDR16_HI, R_PPC_ADDR16_HA = 1, 3, 4, 5, 6


def reset_global_fields(path: str) -> int:
    """Rewrite the relocated field of every ADDR32/ADDR16* relocation against a defined GLOBAL/WEAK
    symbol to the plain addend (what the original link left there). Returns the number of fields changed."""
    elf = elffile.Elf(path)
    data = bytearray(open(path, 'rb').read())
    changed = 0
    for rela_sec in elf.sections:
        if rela_sec.type != elffile.SHT_RELA:
            continue
        target = elf.sections[rela_sec.info]
        if not (target.flags & 2) or target.type == elffile.SHT_NOBITS:  # SHF_ALLOC
            continue
        for i in range(rela_sec.size // 12):
            off, info, addend = struct.unpack('>IIi', rela_sec.data[12 * i:12 * i + 12])
            kind = info & 0xFF
            if kind not in (R_PPC_ADDR32, R_PPC_ADDR16, R_PPC_ADDR16_LO, R_PPC_ADDR16_HI, R_PPC_ADDR16_HA):
                continue
            sym = elf.symbols[info >> 8]
            if sym.bind not in (elffile.STB_GLOBAL, elffile.STB_WEAK) or sym.shndx in (elffile.SHN_UNDEF, elffile.SHN_COMMON, elffile.SHN_ABS):
                continue
            pos = target.offset + off
            if kind == R_PPC_ADDR32:
                want = struct.pack('>I', addend & 0xFFFFFFFF)
            elif kind == R_PPC_ADDR16_HA:
                want = struct.pack('>H', ((addend + 0x8000) >> 16) & 0xFFFF)
            elif kind == R_PPC_ADDR16_HI:
                want = struct.pack('>H', (addend >> 16) & 0xFFFF)
            else:
                want = struct.pack('>H', addend & 0xFFFF)
            if data[pos:pos + len(want)] != want:
                data[pos:pos + len(want)] = want
                changed += 1
    if changed:
        with open(path, 'wb') as f:
            f.write(data)
    return changed


def main() -> int:
    if len(sys.argv) != 7:
        print(__doc__, file=sys.stderr)
        return 2
    linker, wrapper, sn_ngc_path, ldscript, out_path, rsp_path = sys.argv[1:]
    with open(rsp_path, 'r', encoding='utf-8') as f:
        inputs = [line.strip() for line in f if line.strip()]

    fix_dir = os.path.join(os.path.dirname(out_path), 'objfix')
    linked = []
    for obj in inputs:
        parts = os.path.normpath(obj).split(os.sep)
        if 'obj' in parts:
            fixed = os.path.join(fix_dir, *parts[parts.index('obj') + 1:])
            os.makedirs(os.path.dirname(fixed), exist_ok=True)
            fix_split_object(obj, fixed)
            linked.append(fixed)
        else:
            linked.append(obj)

    cmd = [wrapper] if wrapper else []
    cmd += [linker, '-r', '-T', ldscript, '-o', out_path, *linked]
    env = os.environ.copy()
    env['SN_NGC_PATH'] = sn_ngc_path
    res = subprocess.run(cmd, env=env, check=False, capture_output=True, text=True)
    # ngcld prints its banner on every run; keep only diagnostics
    for stream in (res.stdout, res.stderr):
        for line in stream.splitlines():
            if line.startswith('ngcld ') and ' @ ' in line:
                continue
            print(line, file=sys.stderr)
    if res.returncode != 0 and os.path.exists(out_path):
        os.remove(out_path)
    if res.returncode == 0:
        reset_global_fields(out_path)
    return res.returncode


if __name__ == '__main__':
    raise SystemExit(main())
