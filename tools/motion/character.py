"""Which attachment models make up a character: the game's model set-up code, per archive.

The body `.bin` of a character archive is entry 0 (PL_ARC 4) with its palette in entry 1 (PL_ARC 5);
the head, hair, eyes, hands ... are separate `.bin`s that the character's set-up code hangs on the
same parts (ModInfoMgr.create(bin, tpl) + addModel). The table below is read off those functions:
the players' `setModel` / `setRightHand` / `setLeftHand` (what they load before the weapon module
runs: the default game state) and the Ganado's `em10ModelInit` (`em10_set.cpp` fills mot[] for the
model type, `em10HeadSet(0)` / `em10HandSet(0)` pick the normal head and the relaxed hands).
Entry indices are the archive's (the game's PL_ARC index minus 4: `archive.ARC_INDEX_BASE`).

A cross-archive reference is written 'stem:entry' (the weapon-grip hands of the weapon modules,
`cPlBody::initWepHand(WEP_ARC_PTR(0xA))` + `setRightHand(1)`, are a wepNN.drs `.bin` drawn with the
player's hand palette; the default handgun modules wep01 / wep02 do not touch the hands, so the
defaults below are all in-archive). The table is checked against the disc by `verify`.
"""
from dataclasses import dataclass


@dataclass(frozen=True)
class Attachment:
    bin: object      # entry index, or 'stem:entry' in another archive
    tpl: object
    role: str
    source: str      # the game function the pair comes from


def A(bin, tpl, role, source):
    return Attachment(bin, tpl, role, source)


# cPlLeon::setModel (game/pl_leon.cpp): body 4/5, costume extras 0xA/5 (costume 0..3), face 0xD/5
# (Body->pFace), head 8/7 (Body->pShape / pHeadData: the shape table), hair 6/7 (Body->pHair),
# eyes 9/7 (Body->pEye, be_flag 0x40), then setRightHand(0) = 0x12/0x11 and setLeftHand(1) = 0x15/0x11.
# The game poses the eyelid / eye parts (0x1C, 0x20, 0x21: cPlayer::moveEyeNormal) by code, not by
# the motion; they are skeleton parts, so the eye models are bound through the skin like the rest.
LEON = [
    A(0, 1, 'body', 'cPlLeon::setModel: modelInit(PL_ARC 4, 5)'),
    A(6, 1, 'costume', 'cPlLeon::setModel: create(PL_ARC 0xA, 5), pl_costume 0..3'),
    A(9, 1, 'face', 'cPlLeon::setModel: create(PL_ARC 0xD, 5) -> Body->pFace'),
    A(4, 3, 'head', 'cPlLeon::setModel: create(PL_ARC 8, 7) -> Body->pShape, pHeadData'),
    A(2, 3, 'hair', 'cPlLeon::setModel: create(PL_ARC 6, 7) -> Body->pHair'),
    A(5, 3, 'eyes', 'cPlLeon::setModel: create(PL_ARC 9, 7) -> Body->pEye'),
    A(14, 13, 'right hand', 'cPlLeon::setRightHand(0): create(PL_ARC 0x12, 0x11)'),
    A(17, 13, 'left hand', 'cPlLeon::setLeftHand(1): create(PL_ARC 0x15, 0x11)'),
]

# cPlAshley::setModel (game/pl_ashley.cpp): body 4/5, face 7/9 (Body->pShape / pHeadData), hair
# 6/0xB, skirt 8/5 (be_flag 0x40), 0xA/5, setRightHand(0) = 0x11/5, setLeftHand(0) = 0x14/5.
ASHLEY = [
    A(0, 1, 'body', 'cPlAshley::setModel: modelInit(PL_ARC 4, 5)'),
    A(3, 5, 'head', 'cPlAshley::setModel: create(PL_ARC 7, 9) -> Body->pShape, pHeadData'),
    A(2, 7, 'hair', 'cPlAshley::setModel: create(PL_ARC 6, 0xB)'),
    A(4, 1, 'skirt', 'cPlAshley::setModel: create(PL_ARC 8, 5), be_flag 0x40'),
    A(6, 1, 'accessory', 'cPlAshley::setModel: create(PL_ARC 0xA, 5)'),
    A(13, 1, 'right hand', 'cPlAshley::setRightHand(0): create(PL_ARC 0x11, 5)'),
    A(16, 1, 'left hand', 'cPlAshley::setLeftHand(0): create(PL_ARC 0x14, 5)'),
]

# cPlAda::setModel (pl02/pl02.cpp): body 4/5, hair 6/7 (Body->pHair), head 8/7 (Body->pShape /
# pHeadData), eyes 9/0xA (be_flag 0x40), setRightHand(0) = 0x11/5, setLeftHand(0) = 0x12/5.
# pl0c.drs is the archive laid out for this code (entry 4: 1104-vertex head with a shape table,
# entries 13 / 14: hands using texture 6 of the 7-texture body palette). pl02.drs is the event Ada
# (red dress; events attach parts by name from their scripts, Event::ExePacket_SetParts): her body
# .bin has the hands modelled in, entry 13 is empty, entry 14 wants texture 6 of a 5-texture
# palette, and entry 4 is a 29-vertex stub without a shape table, rigged to a 67-part skeleton that
# is not the body's (its one weight names parts 7, the body's left upper arm, so it floats beside
# the shoulder in the game's own skinning). pl02's default is therefore body, hair and eyes/face.
ADA = [
    A(0, 1, 'body', 'cPlAda::setModel: modelInit(PL_ARC 4, 5)'),
    A(2, 3, 'hair', 'cPlAda::setModel: create(PL_ARC 6, 7) -> Body->pHair'),
    A(4, 3, 'head', 'cPlAda::setModel: create(PL_ARC 8, 7) -> Body->pShape, pHeadData'),
    A(5, 6, 'eyes', 'cPlAda::setModel: create(PL_ARC 9, 0xA), be_flag 0x40'),
    A(13, 1, 'right hand', 'cPlAda::setRightHand(0): create(PL_ARC 0x11, 5)'),
    A(14, 1, 'left hand', 'cPlAda::setLeftHand(0): create(PL_ARC 0x12, 5)'),
]
ADA_EVENT = [a for a in ADA if a.role in ('body', 'hair', 'eyes')]

# cPlWesker::setModel (pl0d/pl_wesker.cpp): body 4/5, extra 0xA/5, head 8/7 (Body->pShape), hair
# 6/7 (Body->pHair), setRightHand(0) = 0x12/0x11, setLeftHand(0) = 0x14/0x11.
WESKER = [
    A(0, 1, 'body', 'cPlWesker::setModel: modelInit(PL_ARC 4, 5)'),
    A(6, 1, 'costume', 'cPlWesker::setModel: create(PL_ARC 0xA, 5)'),
    A(4, 3, 'head', 'cPlWesker::setModel: create(PL_ARC 8, 7) -> Body->pShape, pHeadData'),
    A(2, 3, 'hair', 'cPlWesker::setModel: create(PL_ARC 6, 7) -> Body->pHair'),
    A(14, 13, 'right hand', 'cPlWesker::setRightHand(0): create(PL_ARC 0x12, 0x11)'),
    A(16, 13, 'left hand', 'cPlWesker::setLeftHand(0): create(PL_ARC 0x14, 0x11)'),
]

# cPlHunk::setModel (pl06/pl06.cpp): body 4/5, mask 6/7 (Body->pHair), setRightHand(0) = 0x12/0x11,
# setLeftHand(1) = 0x15/0x11.
HUNK = [
    A(0, 1, 'body', 'cPlHunk::setModel: modelInit(PL_ARC 4, 5)'),
    A(2, 3, 'mask', 'cPlHunk::setModel: create(PL_ARC 6, 7) -> Body->pHair'),
    A(14, 13, 'right hand', 'cPlHunk::setRightHand(0): create(PL_ARC 0x12, 0x11)'),
    A(17, 13, 'left hand', 'cPlHunk::setLeftHand(1): create(PL_ARC 0x15, 0x11)'),
]

# cPlKlauser::setModel (pl0a/pl_klauser.cpp): body 4/5, 6/7, 8/9 (be_flag 0x20), 0xA/0xB (hidden:
# be_flag &= ~8), 0xE/9, 0xF/0x10, glow 0x18/0x19 (hidden, tex-render), setRightHand(0) = 0x12/0x11,
# setLeftHand(1) = 0x14/0x11. The two hidden models are not part of the visible default.
KRAUSER = [
    A(0, 1, 'body', 'cPlKlauser::setModel: modelInit(PL_ARC 4, 5)'),
    A(2, 3, 'hair', 'cPlKlauser::setModel: create(PL_ARC 6, 7)'),
    A(4, 5, 'head', 'cPlKlauser::setModel: create(PL_ARC 8, 9), be_flag 0x20'),
    A(10, 5, 'face', 'cPlKlauser::setModel: create(PL_ARC 0xE, 9)'),
    A(11, 12, 'accessory', 'cPlKlauser::setModel: create(PL_ARC 0xF, 0x10)'),
    A(14, 13, 'right hand', 'cPlKlauser::setRightHand(0): create(PL_ARC 0x12, 0x11)'),
    A(16, 13, 'left hand', 'cPlKlauser::setLeftHand(1): create(PL_ARC 0x14, 0x11)'),
]

# em10ModelInit (em10/em10.cpp) with Em10Set model type 0 (em10/em10_set.cpp, the male villager):
# modelInit(mot[1] = ARC 0x1BC, mot[0] = ARC 0x1BD); em10HeadSet(0): create(mot[2] = ARC 0x1BE,
# mot[5] = ARC 0x1BD); em10HandSet(0): right create(mot[6] = ARC 0x1C0, mot[0]), left
# create(mot[11] = ARC 0x1C5, mot[0]). Types 1 / 3 / 4 swap the body and head (ARC 0x1D8.., 0x1DC..,
# 0x1E0..) and keep the hands; accessories (hats, sacks, the weapon) depend on the cEm flags.
GANADO_VILLAGE = [
    A(440, 441, 'body', 'em10ModelInit: modelInit(mot[1] = ARC 0x1BC, mot[0] = ARC 0x1BD), Em10Set type 0'),
    A(442, 441, 'head', 'em10HeadSet(0): create(mot[2] = ARC 0x1BE, mot[5] = ARC 0x1BD)'),
    A(444, 441, 'right hand', 'em10HandSet(0): create(mot[6] = ARC 0x1C0, mot[0] = ARC 0x1BD)'),
    A(449, 441, 'left hand', 'em10HandSet(0): create(mot[11] = ARC 0x1C5, mot[0] = ARC 0x1BD)'),
]

TABLE = {
    'pl00': LEON,
    'pl01': ASHLEY,
    'pl02': ADA_EVENT,
    'pl0c': ADA,
    'pl06': HUNK,
    'pl0a': KRAUSER,
    'pl0d': WESKER,
    'em10': GANADO_VILLAGE,
}

# What plays the head's shape keys: the ShapeData entries (tagged 'FCV' like the motions: u16 nFrame,
# u8 num channels, u16 flags[num], u8 shape index[num], Hermite key tables) that setFace hands to
# ShapeSet. archive -> {shape table index: the call}. The game has no names for the shapes;
# Ashley's (pl01, two shapes) are played by the events (Event::ExePacket ShapeSet) - cPlAshley::setFace
# is empty - and Wesker's setFace names 0x62 / 0x63 but his head has no shape table.
FACES = {
    'pl00': {0: 'cPlLeon::setFace(1): PL_ARC 0x62 (pain)', 1: 'cPlLeon::setFace(2): PL_ARC 0x63'},
    'pl0c': {1: 'cPlAda::setFace(1): PL_ARC 0x62', 0: 'cPlAda::setFace(2): PL_ARC 0x63'},
}


def attachments(stem):
    """The character's model list for an archive stem ('pl00'), body first; None when unknown."""
    return TABLE.get(stem)


def parse_ref(text, default_stem):
    """'12' -> (default_stem, 12); 'wep04:6' -> ('wep04', 6)."""
    if ':' in text:
        stem, idx = text.rsplit(':', 1)
        return stem, int(idx, 0)
    return default_stem, int(text, 0)


def ref(value, default_stem):
    """Table entry (int or 'stem:entry') -> (stem, entry)."""
    if isinstance(value, int):
        return default_stem, value
    return parse_ref(value, default_stem)
