"""Where the motions live, and how each file is read. Every way the game hands a MotionData
pointer to MotionSetCore (see README "Where the motions come from") resolves to one of these
containers on the discs:

  ハカセ container   files/em/*.drs (players, enemies, weapons; read.cpp SET_DRS_NAME), files/St*/rNNN.das
  (`hakase`)        (rooms, yz2-compressed body), files/etc/*.das (core, memcard), files/op/*.das (the
                    codec screen's per-stage data, ss_term.cpp). tools/drs.py; body decompressed by yz2.py.
                    The game addresses an entry as PL_ARC_PTR(arc, no) = arc->ofs[no] + arc where `arc`
                    is the body: ofs[0..3] are the body header words, so PL_ARC index = entry index + 4.
  bare body         files/ss/<lang>/*.dat, files/ss/cmn/*.dat (the sub screen: SS_ARC_PTR, sscrn.cpp),
  (`body`)          files/St1/r100_NN.dat (SMD sets): the container body alone, no record table.
  event             files/Evd/rNNNsMM.evd (EventMgr::SetEvd): "event" header, a table of named bins
  (`evd`)           (EvtBinEntry {char name[0x30]; u32 ofs, size}) the packets look up by name
                    (EventMgr::GetBin); the cutscene motions (`<room>/<cut>/<model>/*.fcv`, camera
                    motions under `cam/`, ShapeData under `face/`), models, TPLs, lights, effects.
  raw file          files/ss/<lang>/*.fcv (one MotionData; not referenced by any code on the disc),
  (`fcv`, `eff`)    files/etc/<lang>/*.eff (effect data: EspDataLoad).

Nested containers inside entries (`Archive.entries(deep=True)` walks them):
  ETM   room etc archive (EtcModel.cpp EtcArc: u32 num; files of {u32 size; ...; char name[0x20];
        data} from 0x20, 0x40-byte header): the ladders' / doors' / windows' .bin/.tpl/.fcv/.seq/.eff
        that GetEtcAddr(arc, "pl00017.fcv") fetches by name (Et06_init -> cObjLadder plays them on
        the player).
  EFF   effect data (eff_sys.cpp EffData, version 0xB): effect models (EffEfmEnt) with an optional
        motion table (EspEfmMotTbl {u32 num; u32 ofs[]}), played by esp_efm.cpp MotionSetCore.
  SMD   scroll model data (scroll.h cSmd): bin / tpl / motion offset tables (getMotPtr); the motion
        tables are empty on both discs.

Zero-padded variants: the event bins and the ETM files pad motions and sequences with 0x00 (fcv.py
`fill`), the event camera motions carry a zero size word.

A source is a container file or a disc image (.iso/.gcm): the disc's files are extracted with dtk
vfs into a cache directory that mirrors the disc tree; archives are parsed on first use (a room
decompresses in ~2 s).
"""
import os
import struct
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
EXTS = ('.drs', '.das', '.dat', '.evd', '.fcv', '.eff')

# file name extension -> entry tag, for the named files of events and ETM archives
EXT_TAG = {'.fcv': 'FCV', '.seq': 'SEQ', '.bin': 'BIN', '.tpl': 'TPL', '.eff': 'EFF', '.mdt': 'MDT',
           '.lit': 'LIT', '.fcs': 'FCS'}


def ext_tag(name):
    ext = os.path.splitext(name.strip())[1].lower()
    if ext not in EXT_TAG:
        raise ValueError(f'{name}: no tag for this file extension')
    return EXT_TAG[ext]


@dataclass
class Entry:
    archive: str      # file name (pl00.drs, r100s00.evd)
    index: int        # entry index in the container (bin index in an event)
    tag: str
    data: bytes
    label: str = None  # named entry: the event bin / ETM file name
    sub: str = None    # nested entry: 'ETM/pl00017.fcv', 'EFF/efm124/mot0', 'SMD/mot0'

    @property
    def arc_no(self):
        """The game's PL_ARC / ROOM_ARC / SS_ARC index; None for a nested or named entry."""
        return None if self.sub is not None or self.label is not None else self.index + ARC_INDEX_BASE

    @property
    def name(self):
        n = f'{os.path.splitext(self.archive)[0]}:{self.index}'
        if self.sub:
            n += '/' + self.sub
        elif self.label:
            n += f' {self.label}'
        return n

    @property
    def is_camera(self):
        """Event camera motion (cam/*.fcv: CameraControl::MotionSet, not a skeleton)."""
        return self.tag == 'FCV' and self.label is not None and '/cam/' in self.label

    @property
    def is_face(self):
        """Event face data (<model>/face/*.fcv: ShapeData for Event::ExePacket ShapeSet, the same
        key-table layout with shape indices for parts numbers, not a skeletal motion)."""
        return self.tag == 'FCV' and self.label is not None and '/face/' in self.label


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


def parse_body(body):
    """[(tag, bytes)] of a bare container body (u32 count, 0, 0, 0; u32 ofs[count]; char tag[count][4]):
    the ss/*.dat files, and the same layout the ハカセ body uses (drs.Drs checks it there)."""
    count, a, b, c = struct.unpack('>4I', body[:16])
    if (a, b, c) != (0, 0, 0) or not 1 <= count <= 0x1000:
        raise ValueError(f'not a container body: count {count:#x}, header words {a:#x} {b:#x} {c:#x}')
    ofs = struct.unpack(f'>{count}I', body[16:16 + 4 * count])
    tags = [body[16 + 4 * count + 4 * i:][:4] for i in range(count)]
    hdr_end = 16 + 8 * count
    if ofs[0] != drs.align(hdr_end) or body[hdr_end:ofs[0]] != b'\0' * (ofs[0] - hdr_end):
        raise ValueError(f'container body: first entry at {ofs[0]:#x}, header ends at {hdr_end:#x}')
    out = []
    for i in range(count):
        end = ofs[i + 1] if i + 1 < count else len(body)
        if ofs[i] % drs.ALIGN or end < ofs[i] or end > len(body):
            raise ValueError(f'container body: entry {i} spans {ofs[i]:#x}..{end:#x} of {len(body):#x}')
        out.append((drs.tag_name(tags[i]), body[ofs[i]:end]))
    return out


def is_body(d):
    return len(d) >= 0x20 and d[4:16] == b'\0' * 12 and 1 <= struct.unpack('>I', d[:4])[0] <= 0x1000


def is_hakase(d):
    return len(d) > drs.TABLE_SIZE and d[0x20:0x24] == b'\0' * 4 and struct.unpack('>I', d[0x2C:0x30])[0] == drs.TABLE_SIZE


def parse_event(d):
    """[(name, bytes)] of an event block's bins (EvtHeader nBin / binOfs; EvtBinEntry 0x40 bytes)."""
    if d[:5] != b'event':
        raise ValueError('not an event block')
    n_bin, bin_ofs = struct.unpack('>iI', d[0x48:0x50])
    out = []
    end = 0
    for i in range(n_bin):
        e = d[bin_ofs + 0x40 * i:bin_ofs + 0x40 * (i + 1)]
        name = e[:0x30].split(b'\0')[0].decode('ascii')
        ofs, size = struct.unpack('>2I', e[0x30:0x38])
        if e[0x38:] != b'\0' * 8 or ofs < end or ofs + size > len(d):
            raise ValueError(f'event bin {i} {name}: {ofs:#x}+{size:#x} of {len(d):#x}')
        end = ofs + size
        out.append((name, d[ofs:ofs + size]))
    if end != len(d):
        raise ValueError(f'event: bins end at {end:#x}, block is {len(d):#x}')
    return out


def parse_etm(d):
    """[(name, bytes)] of a room ETM entry (EtcArc): u32 num at 0, files from 0x20, each
    {u32 size (of the record); pad; char name[0x20] at 0x20; data at 0x40}."""
    num, = struct.unpack('>I', d[:4])
    p = 0x20
    out = []
    for _ in range(num):
        size, = struct.unpack('>I', d[p:p + 4])
        name = d[p + 0x20:p + 0x40].split(b'\0')[0].decode('ascii')
        if size < 0x40 or p + size > len(d) or size % drs.ALIGN:
            raise ValueError(f'ETM file {name}: record size {size:#x} at {p:#x} of {len(d):#x}')
        out.append((name, d[p + 0x40:p + size]))
        p += size
    if p != len(d):
        raise ValueError(f'ETM: files end at {p:#x}, entry is {len(d):#x}')
    return out


def eff_motions(d):
    """[(effect model id, motion index, bytes)] of the effect models' motion tables in EFF data
    (eff_sys.cpp EspDataLoad: EffData -> EffOfsTbl of EffEfmEnt -> EspEfmMotTbl). A motion runs to
    the next table offset or the entry's end."""
    version, = struct.unpack('>I', d[:4])
    if version != 0xB:
        raise ValueError(f'EFF version {version:#x}, expected 0xb')
    ofs_efm_id, ofs_efm = struct.unpack('>I', d[0x14:0x18])[0], struct.unpack('>I', d[0x2C:0x30])[0]
    n, = struct.unpack('>I', d[ofs_efm_id:ofs_efm_id + 4])
    n2, = struct.unpack('>I', d[ofs_efm:ofs_efm + 4])
    if n2 < n:
        raise ValueError(f'EFF: {n} effect model ids, {n2} entries')
    out = []
    for i in range(n):
        ent = ofs_efm + struct.unpack('>I', d[ofs_efm + 4 + 4 * i:ofs_efm + 8 + 4 * i])[0]
        _, _, _, ofs_mot, _ = struct.unpack('>5I', d[ent:ent + 20])
        efm_id, = struct.unpack('>H', d[ofs_efm_id + 4 + 8 * i:ofs_efm_id + 6 + 8 * i])
        if not ofs_mot:
            continue
        tbl = ent + ofs_mot
        num, = struct.unpack('>I', d[tbl:tbl + 4])
        offs = struct.unpack(f'>{num}I', d[tbl + 4:tbl + 4 + 4 * num])
        for k in range(num):
            start = tbl + offs[k]
            end = tbl + offs[k + 1] if k + 1 < num else len(d)
            out.append((efm_id, k, d[start:end]))
    return out


def smd_motions(d):
    """[bytes] of an SMD entry's motion table (cSmd::getMotPtr: u32 offsets from MotTblOfs, the
    table runs up to its first entry). Empty on both discs."""
    _, _, _, _, _, mot_ofs = struct.unpack('>BBHIII', d[:16])
    if mot_ofs > len(d):
        raise ValueError(f'SMD: motion table at {mot_ofs:#x} beyond {len(d):#x}')
    if mot_ofs == len(d):
        return []
    first, = struct.unpack('>I', d[mot_ofs:mot_ofs + 4])
    num = first // 4
    offs = struct.unpack(f'>{num}I', d[mot_ofs:mot_ofs + 4 * num])
    return [d[mot_ofs + offs[k]:mot_ofs + offs[k + 1] if k + 1 < num else len(d)] for k in range(num)]


def kind_of(path):
    """Container kind from the file name: hakase / body / evd / fcv / eff."""
    ext = os.path.splitext(path)[1].lower()
    if ext in ('.drs', '.das'):
        return 'hakase'
    if ext == '.dat':
        return 'body'
    if ext == '.evd':
        return 'evd'
    if ext == '.fcv':
        return 'fcv'
    if ext == '.eff':
        return 'eff'
    raise ValueError(f'{path}: not an archive ({", ".join(EXTS)})')


class Archive:
    def __init__(self, path):
        self.path = path
        self.name = os.path.basename(path)
        self.stem, self.ext = os.path.splitext(self.name)
        self.ext = self.ext.lower()
        self.kind = kind_of(path)
        self._drs = None
        self._entries = None
        self.das = None     # DasInfo for a .das

    @property
    def drs(self):
        """The drs.Drs of a ハカセ container (kind 'hakase')."""
        if self._drs is None:
            if self.ext == '.das':
                self._drs, self.das = load_das(self.path)
            else:
                self._drs = drs.load(self.path)
        return self._drs

    def _load(self):
        """[(tag, data, label)] of the top-level entries."""
        if self._entries is not None:
            return self._entries
        if self.kind == 'hakase':
            self._entries = [(drs.tag_name(t), d, None) for t, d in self.drs.entries]
        elif self.kind == 'body':
            self._entries = [(t, d, None) for t, d in parse_body(open(self.path, 'rb').read())]
        elif self.kind == 'evd':
            self._entries = [(ext_tag(n), d, n) for n, d in parse_event(open(self.path, 'rb').read())]
        elif self.kind == 'fcv':
            self._entries = [('FCV', open(self.path, 'rb').read(), self.name)]
        else:
            self._entries = [('EFF', open(self.path, 'rb').read(), self.name)]
        return self._entries

    def __len__(self):
        return len(self._load())

    def entry(self, i):
        t, d, label = self._load()[i]
        return Entry(self.name, i, t, d, label)

    def find(self, label):
        """The named entry (event bin / raw file name, or its basename); None when absent."""
        for i, (t, d, l) in enumerate(self._load()):
            if l is not None and (l.strip() == label or os.path.basename(l.strip()) == label):
                return Entry(self.name, i, t, d, l)
        return None

    def entries(self, tag=None, deep=False):
        """The entries, with the ETM / EFF / SMD nested ones after their parent when `deep`."""
        for i, (t, d, label) in enumerate(self._load()):
            e = Entry(self.name, i, t, d, label)
            if tag is None or t == tag:
                yield e
            if deep:
                for sub in nested(e):
                    if tag is None or sub.tag == tag:
                        yield sub


def nested(e):
    """The entries a container entry holds: ETM files, EFF effect-model motions, SMD motions."""
    if e.tag == 'ETM':
        for name, d in parse_etm(e.data):
            yield Entry(e.archive, e.index, ext_tag(name), d, name, f'ETM/{name}')
    elif e.tag == 'EFF':
        for efm_id, k, d in eff_motions(e.data):
            yield Entry(e.archive, e.index, 'FCV', d, None, f'EFF/efm{efm_id}/mot{k}')
    elif e.tag == 'SMD':
        for k, d in enumerate(smd_motions(e.data)):
            yield Entry(e.archive, e.index, 'FCV', d, None, f'SMD/mot{k}')


def is_disc(path):
    return os.path.splitext(path)[1].lower() in ('.iso', '.gcm')


def _vfs_ls(disc, sub):
    out = subprocess.run([DTK, 'vfs', 'ls', f'{disc}:/{sub}'], check=True, capture_output=True, text=True).stdout
    return [line.split('|')[1].strip() for line in out.splitlines() if '|' in line]


def disc_files(disc):
    """[(dir, name)] of every file on a disc that holds motions: files/em/*.drs, files/St*/*.das and
    *.dat, files/etc/*.das, files/op/*.das, files/ss/*/*.dat and *.fcv, files/Evd/*.evd, files/etc/*/*.eff."""
    found = []
    for d in _vfs_ls(disc, 'files'):
        d = d.rstrip('/')
        low = d.lower()
        if low == 'em':
            found += [(d, n) for n in _vfs_ls(disc, f'files/{d}') if n.lower().endswith('.drs')]
        elif low.startswith('st'):
            found += [(d, n) for n in _vfs_ls(disc, f'files/{d}') if n.lower().endswith(('.das', '.dat'))]
        elif low in ('etc', 'op'):
            names = _vfs_ls(disc, f'files/{d}')
            found += [(d, n) for n in names if n.lower().endswith('.das')]
            for sub in [n.rstrip('/') for n in names if n.endswith('/')]:
                found += [(f'{d}/{sub}', n) for n in _vfs_ls(disc, f'files/{d}/{sub}') if n.lower().endswith('.eff')]
        elif low == 'evd':
            found += [(d, n) for n in _vfs_ls(disc, f'files/{d}') if n.lower().endswith('.evd')]
        elif low == 'ss':
            for sub in [n.rstrip('/') for n in _vfs_ls(disc, f'files/{d}') if n.endswith('/')]:
                found += [(f'{d}/{sub}', n) for n in _vfs_ls(disc, f'files/{d}/{sub}') if n.lower().endswith(('.dat', '.fcv'))]
    return found


def disc_archives(disc, cache_dir):
    """Extracts the disc's archives (once) into cache_dir/<disc stem>/<dir>/ and returns their
    paths, em/*.drs first (the order of disc_files)."""
    root = os.path.join(cache_dir, os.path.splitext(os.path.basename(disc))[0])
    paths = []
    for d, n in disc_files(disc):
        dst = os.path.join(root, d, n)
        if not os.path.exists(dst):
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            subprocess.run([DTK, 'vfs', 'cp', f'{disc}:/files/{d}/{n}', dst], check=True, capture_output=True)
        paths.append(dst)
    return paths


def open_source(path, cache_dir='/tmp/mot/disc'):
    """[Archive] for a container file or every archive of a disc image (parsed lazily)."""
    if is_disc(path):
        return [Archive(p) for p in disc_archives(path, cache_dir)]
    return [Archive(path)]
