"""Where the motions live: the "FCV" entries of the ﾊｶｾ containers (files/em/plNN.drs players,
emNN.drs enemies, wepNN.drs weapons), read through tools/drs.py. The game addresses an entry as
PL_ARC_PTR(arc, no) = arc->ofs[no] + arc where `arc` is the container body: ofs[0..3] are the body
header words (count, rel_offset, 0, 0), so PL_ARC index no = entry index no - 4.

A source is a container file or a disc image (.iso/.gcm): the disc's files/em/*.drs are extracted
with dtk vfs into a cache directory. Room archives (St*/rNNN.das) are yz2-compressed (game/yz2code
+ yz2asm) and are not opened here.
"""
import os
import subprocess
import sys
from dataclasses import dataclass

TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS)
import drs  # noqa: E402

DTK = os.path.join(os.path.dirname(TOOLS), 'build', 'tools', 'dtk')
ARC_INDEX_BASE = 4


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


class Archive:
    def __init__(self, path):
        self.path = path
        self.name = os.path.basename(path)
        self.drs = drs.load(path)

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


def disc_archives(disc, cache_dir):
    """Extracts files/em/*.drs from the disc (once) and returns their paths."""
    os.makedirs(cache_dir, exist_ok=True)
    out = subprocess.run([DTK, 'vfs', 'ls', f'{disc}:/files/em'], check=True, capture_output=True, text=True).stdout
    names = [line.split('|')[1].strip() for line in out.splitlines() if '.drs' in line]
    paths = []
    for n in names:
        dst = os.path.join(cache_dir, n)
        if not os.path.exists(dst):
            subprocess.run([DTK, 'vfs', 'cp', f'{disc}:/files/em/{n}', dst], check=True, capture_output=True)
        paths.append(dst)
    return paths


def open_source(path, cache_dir='/tmp/mot/disc'):
    """[Archive] for a container file or every character archive of a disc image."""
    if is_disc(path):
        return [Archive(p) for p in disc_archives(path, cache_dir)]
    return [Archive(path)]


def resolve_ref(ref, default_archive=None):
    """'path:index' or 'index' (with default_archive) -> Entry."""
    if ':' in ref:
        path, idx = ref.rsplit(':', 1)
        return Archive(path).entry(int(idx, 0))
    if default_archive is None:
        raise ValueError(f'{ref}: need archive:index')
    return default_archive.entry(int(ref, 0))
