"""Where the motions live: the "FCV" entries of the ﾊｶｾ containers (files/em/plNN.drs players,
emNN.drs enemies, wepNN.drs weapons; files/St*/rNNN.das rooms, files/etc/*.das), read through
tools/drs.py. The game addresses an entry as PL_ARC_PTR(arc, no) = arc->ofs[no] + arc where `arc`
is the container body: ofs[0..3] are the body header words (count, rel_offset, 0, 0), so PL_ARC
index no = entry index no - 4.

`.das` is the same container whose body may be a yz2 stream (game/yz2code + yz2asm, ported in
yz2.py): the rooms (ReadAreaData -> decodeData) are, etc/core.das is stored raw. The body is
decompressed and the container re-assembled, so `.das` and `.drs` entries read alike. (The player
and enemy files are named `em/plNN.das` in the game's FileTbl but read as `.drs`: read.cpp
SET_DRS_NAME; the disc has no em/*.das.)

A source is a container file or a disc image (.iso/.gcm): the disc's files/em/*.drs, files/St*/*.das
and files/etc/*.das are extracted with dtk vfs into a cache directory; archives are parsed on first
use (a room decompresses in ~2 s).
"""
import os
import subprocess
import sys
from dataclasses import dataclass

TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drs  # noqa: E402
import yz2  # noqa: E402

DTK = os.path.join(os.path.dirname(TOOLS), 'build', 'tools', 'dtk')
ARC_INDEX_BASE = 4
EXTS = ('.drs', '.das')


@dataclass
class Entry:
    archive: str      # file name (pl00.drs)
    index: int        # entry index in the body
    tag: str
    data: bytes

    @property
    def arc_no(self):
        return self.index + ARC_INDEX_BASE

    @property
    def name(self):
        return f'{os.path.splitext(self.archive)[0]}:{self.index}'


@dataclass
class DasInfo:
    """What the .das body was: yz2 header sizes and the bytes the coder read."""
    compressed: bool
    packed: int
    unpacked: int
    used: int


def load_das(path):
    """(drs.Drs, DasInfo): a .das container with its body decompressed when it is a yz2 stream."""
    d = open(path, 'rb').read()
    sig, recs = drs.read_records(d)
    body_rec, snd_rec = recs
    assert body_rec[0] == 0 and body_rec[3] == drs.TABLE_SIZE, body_rec
    body_end = drs.TABLE_SIZE + body_rec[1]
    if yz2.is_yz2(d, drs.TABLE_SIZE):
        body, packed, used = yz2.decode(d, drs.TABLE_SIZE)
        info = DasInfo(True, packed, len(body), used)
        if packed + 0x20 > body_rec[1]:
            raise ValueError(f'{path}: yz2 packed size {packed:#x} exceeds the body record {body_rec[1]:#x}')
    else:
        body = d[drs.TABLE_SIZE:body_end]
        info = DasInfo(False, len(body), len(body), len(body))
    body += b'\0' * (drs.align(len(body)) - len(body))
    if snd_rec[0] == drs.EMPTY:
        snd = b''
        snd_rec = (drs.EMPTY, 0, 0, drs.TABLE_SIZE + len(body), 0, 0, 0, 0)
    else:
        snd = d[snd_rec[3]:snd_rec[3] + snd_rec[1]]
        snd_rec = (snd_rec[0], snd_rec[1], 0, drs.TABLE_SIZE + len(body), snd_rec[4], snd_rec[5], 0, 0)
    rebuilt = bytes(drs.write_records(sig, [(0, len(body), 0, drs.TABLE_SIZE, 0, 0, 0, 0), snd_rec]) + body + snd)
    return drs.Drs(rebuilt), info


class Archive:
    def __init__(self, path):
        self.path = path
        self.name = os.path.basename(path)
        self.stem, self.ext = os.path.splitext(self.name)
        self.ext = self.ext.lower()
        self._drs = None
        self.das = None     # DasInfo for a .das

    @property
    def drs(self):
        if self._drs is None:
            if self.ext == '.das':
                self._drs, self.das = load_das(self.path)
            else:
                self._drs = drs.load(self.path)
        return self._drs

    def entries(self, tag=None):
        for i, (t, d) in enumerate(self.drs.entries):
            tn = drs.tag_name(t)
            if tag is None or tn == tag:
                yield Entry(self.name, i, tn, d)

    def entry(self, i):
        t, d = self.drs.entries[i]
        return Entry(self.name, i, drs.tag_name(t), d)


def is_disc(path):
    return os.path.splitext(path)[1].lower() in ('.iso', '.gcm')


def _vfs_ls(disc, sub):
    out = subprocess.run([DTK, 'vfs', 'ls', f'{disc}:/{sub}'], check=True, capture_output=True, text=True).stdout
    return [line.split('|')[1].strip() for line in out.splitlines() if '|' in line]


def disc_files(disc):
    """[(dir, name)] of the archives on a disc: files/em/*.drs, files/St*/*.das, files/etc/*.das."""
    found = []
    for d in _vfs_ls(disc, 'files'):
        d = d.rstrip('/')
        if d == 'em':
            found += [(d, n) for n in _vfs_ls(disc, 'files/em') if n.lower().endswith('.drs')]
        elif d == 'etc' or d.lower().startswith('st'):
            found += [(d, n) for n in _vfs_ls(disc, f'files/{d}') if n.lower().endswith('.das')]
    return found


def disc_archives(disc, cache_dir):
    """Extracts the disc's archives (once) and returns their paths, em/*.drs first."""
    os.makedirs(cache_dir, exist_ok=True)
    paths = []
    for d, n in disc_files(disc):
        dst = os.path.join(cache_dir, n)
        if not os.path.exists(dst):
            subprocess.run([DTK, 'vfs', 'cp', f'{disc}:/files/{d}/{n}', dst], check=True, capture_output=True)
        paths.append(dst)
    return paths


def open_source(path, cache_dir='/tmp/mot/disc'):
    """[Archive] for a container file or every archive of a disc image (parsed lazily)."""
    if is_disc(path):
        return [Archive(p) for p in disc_archives(path, cache_dir)]
    return [Archive(path)]
