#!/usr/bin/env python3
"""Extract the REL embedded in a .drs archive (em/*.drs, ...).

usage: extract_drs_rel.py <file.drs> <out.rel> [--module-id N]
       extract_drs_rel.py --scan <files dir>          # list every REL found in the archives

The DRS container format is not reversed yet; the RELs inside are stored uncompressed, so this
finds the REL header by its fixed fields (next/prev = 0, section table at 0x4C, version 3) and copies
the file up to the end of its last relocation list.
"""
import glob
import os
import re
import struct
import sys

HEADER_RE = re.compile(rb'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00[\x10-\x30]\x00\x00\x00\x4c\x00\x00', re.S)


def find_rels(d):
    for m in HEADER_RE.finditer(d):
        o = m.start() - 4
        if o < 0:
            continue
        mid, nxt, prv, nsec, secoff, nameoff, namesz, ver, bsssz, reloff, impoff, impsz = struct.unpack('>12I', d[o:o + 0x30])
        if ver != 3 or nxt or prv or secoff != 0x4C or impsz % 8 or impsz == 0:
            continue
        end = 0
        ok = True
        for i in range(impsz // 8):
            _, ro = struct.unpack('>2I', d[o + impoff + 8 * i:][:8])
            p = o + ro
            while p + 8 <= len(d) and d[p + 2] != 203:
                p += 8
            if p + 8 > len(d):
                ok = False
                break
            end = max(end, p + 8 - o)
        if ok:
            yield o, mid, end


def main():
    if sys.argv[1] == '--scan':
        for p in sorted(glob.glob(os.path.join(sys.argv[2], '**', '*.drs'), recursive=True)):
            for o, mid, size in find_rels(open(p, 'rb').read()):
                print(f'{os.path.relpath(p, sys.argv[2])}: module {mid} at {o:#x}, {size:#x} bytes')
        return
    src, dst = sys.argv[1:3]
    want = int(sys.argv[sys.argv.index('--module-id') + 1]) if '--module-id' in sys.argv else None
    d = open(src, 'rb').read()
    hits = [(o, mid, size) for o, mid, size in find_rels(d) if want is None or mid == want]
    if len(hits) != 1:
        sys.exit(f'{src}: found {len(hits)} REL(s): {[(hex(o), mid) for o, mid, _ in hits]}')
    o, mid, size = hits[0]
    with open(dst, 'wb') as f:
        f.write(d[o:o + size])
    print(f'{dst}: module {mid}, {size:#x} bytes from {src}+{o:#x}')


if __name__ == '__main__':
    main()
