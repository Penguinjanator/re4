# tools/motion — RE4 (GameCube) skeletal motion export

`tools/motion_export.py` reads the "FCV" motion entries of the character archives
(`files/em/plNN.drs` players, `emNN.drs` enemies, `wepNN.drs` weapons; the room `.das` archives are
yz2-compressed and not opened), plays them with the **game's own code** compiled for the host, and
writes glTF 2.0 (and BVH) animations on the model's parts hierarchy for Blender, with the whole
character (body, head, hair, eyes, hands ... as the game assembles it) skinned and textured from
the model `.bin` ("BIN") and texture palette ("TPL") entries, and the heads' face morphs as shape keys.

    python3 tools/motion_export.py list   files/em/pl00.drs            # or the disc .iso
    python3 tools/motion_export.py export files/em/pl00.drs --motion 46 -o leon_046.gltf --bvh leon_046.bvh   # full Leon
    python3 tools/motion_export.py export orig/G4BE08/re4_debug_disc1.iso --archive em10.drs --motion 46 -o ganado.gltf
    python3 tools/motion_export.py export files/em/pl00.drs --all -o out/          # every motion
    python3 tools/motion_export.py export ... --blender-check /tmp/mot/render [--strip]  # headless Blender import + renders
    python3 tools/motion_export.py export ... --blender-check --render-nice /tmp/mot/render   # + lit EEVEE renders
    python3 tools/motion_export.py verify orig/G4BE08/re4_debug_disc1.iso [--dump dump.bin | --dolphin]

`--motion N` is the archive entry index (the game's `PL_ARC` index is `N + 4`: the container body
starts with four header words). One command exports the whole character: the body `.bin` with its
palette and the attachment models the game's set-up code hangs on the same parts (the table in
`character.py`, below). `--body-only` leaves the attachments out; `--mesh [stem:]BIN[:[stem:]TPL]`
adds a model (or replaces the palette of a table entry; `stem:` names another archive of the
source: the disc, or a `.drs` next to the given one). An archive that is not in the table exports
the body only and says so. `--no-mesh` writes the skeleton with the old stick-figure placeholder.
Units: game millimetres × `--scale` (default 0.001 → metres), Y up, 30 fps.

## The character table (`character.py`)

Read off the players' `setModel` / `setRightHand` / `setLeftHand` (the state before the weapon
module runs) and the Ganado's `em10ModelInit`; `verify` checks that every entry loads from the disc.
`bin:tpl` are archive entry indices (PL_ARC − 4).

| archive | character | models (role bin:tpl) | source |
|---|---|---|---|
| pl00 | Leon | body 0:1, costume 6:1, face 9:1, head 4:3, hair 2:3, eyes 5:3, right hand 14:13, left hand 17:13 | `cPlLeon::setModel` (PL_ARC 4/5, 0xA/5, 0xD/5, 8/7, 6/7, 9/7), `setRightHand(0)` 0x12/0x11, `setLeftHand(1)` 0x15/0x11 |
| pl01 | Ashley | body 0:1, head 3:5, hair 2:7, skirt 4:1, accessory 6:1, right hand 13:1, left hand 16:1 | `cPlAshley::setModel` (4/5, 7/9, 6/0xB, 8/5, 0xA/5), `setRightHand(0)` 0x11/5, `setLeftHand(0)` 0x14/5 |
| pl0c | Ada (playable) | body 0:1, hair 2:3, head 4:3, eyes 5:6, right hand 13:1, left hand 14:1 | `cPlAda::setModel` (4/5, 6/7, 8/7, 9/0xA), `setRightHand(0)` 0x11/5, `setLeftHand(0)` 0x12/5 |
| pl02 | Ada (event, red dress) | body 0:1, hair 2:3, eyes/face 5:6 | same code; entry 13 is empty, 14 wants texture 6 of the 5-texture palette, and the "head" entry 4 is a 29-vertex stub without a shape table rigged to a 67-part skeleton whose parts 7 is the body's upper arm (it floats beside the shoulder in the game's own skinning) - left out; the body has the hands modelled in |
| pl0d | Wesker | body 0:1, costume 6:1, head 4:3, hair 2:3, right hand 14:13, left hand 16:13 | `cPlWesker::setModel`, `setRightHand(0)` 0x12/0x11, `setLeftHand(0)` 0x14/0x11 |
| pl06 | HUNK | body 0:1, mask 2:3, right hand 14:13, left hand 17:13 | `cPlHunk::setModel`, `setRightHand(0)`, `setLeftHand(1)` 0x15/0x11 |
| pl0a | Krauser | body 0:1, hair 2:3, head 4:5, face 10:5, accessory 11:12, right hand 14:13, left hand 16:13 | `cPlKlauser::setModel` (the be_flag &= ~8 models 0xA/0xB and the 0x18/0x19 glow are hidden: not exported) |
| em10 | Ganado (village, type 0) | body 440:441, head 442:441, right hand 444:441, left hand 449:441 | `em10ModelInit` + `Em10Set` type 0 (mot[1]/mot[0] = ARC 0x1BC/0x1BD), `em10HeadSet(0)` mot[2]/mot[5], `em10HandSet(0)` mot[6], mot[11] with mot[0] |

Hands and the weapon module: the default handgun modules (wep01 FN57, wep02 Mauser) do not touch the
hands (`Wep01_init` creates the weapon object only), so the bare hands of the player archive are the
default state. The other modules replace the right hand with their grip hand:
`cPlBody::initWepHand(WEP_ARC_PTR(0xA))` + `setRightHand(1)` draws `wepNN.drs` entry 6 with the
player's hand palette (`PL_ARC_PTR(pG->pPlayer, 0x11)` = pl00:13) — a cross-archive pair, e.g.
`--mesh wep04:6:13 --mesh 20:13` for Leon holding the shotgun (`wep04.cpp`: `setLeftHand(4)` =
0x18). The eye / eyelid parts (0x1C, 0x20, 0x21) are skeleton parts the game turns by code
(`cPlayer::moveEyeNormal`), not by the motion, so the eye models are bound through the skin like
everything else and stay at rest here.

## The mesh export

`meshbin.py` parses the model `.bin` (`include/model.h` ModelData, the section offsets that
`calcModelAddr` in `game/model.cpp` relocates) and `gxtex.py` the TPL; `gltf.py` writes one glTF
mesh per `.bin`, one primitive per `ModelPart` (its GX display list), skinned to the armature.
What each array means comes from the game's draw path, `game/trans.cpp`:

- `commonModelTrans` sets the vertex descriptor: position / normal / texcoord (and colour when
  flags bit31) all `GX_INDEX16`; positions `GX_S16` with `frac = shift` (value / 2^shift mm,
  `dbmodule.cpp DrawObjWireframe` uses the same `1 / (1 << shift)`), normals `GX_S8` (flags bit29)
  or `GX_S16`, texcoords `GX_S16` frac 8, colours `GX_RGBA8`. A display-list vertex is therefore
  four u16 indices (position, normal, colour, texcoord); opcodes on the disc: 0x80 quads, 0x90
  triangles, 0x98 strips (vertex format 0 only), 0x00 NOPs pad the list to 32 bytes. Every model on
  the disc has the colour array; the game's lighting ignores it (`trans_lit.cpp`: material source
  = register unless flags bit30, never set), it is exported as `COLOR_0` anyway.
- Skinning (`commonScreenMatSub`): `calcWeightMat` builds `mtx[k]` = parts k world matrix × bind
  matrix (`cParts::lt_inv_mat`, the inverse of the rest-pose world translation) relative to parts 0;
  `MakeWeightPalette` blends them per `Weight {u8 id[3], num, wht[4] %}` entry (the last weight
  takes `1 - sum`); `CalcSk1_x` multiplies each vertex (`s16 x, y, z, s16 matrix index`, 8 bytes)
  by `palette[matrix index]`, the normals (`s8 x, y, z, u8 index`) likewise. So the vertices are
  rest-pose model-space positions and the export is plain glTF linear-blend skinning:
  `JOINTS_0 / WEIGHTS_0` = the Weight entry's (parts, weight) pairs (≤ 3), inverse bind matrices
  from the rest pose (the same skin the bones already used). Rigid attachment models
  (`weight_palette_num <= 1 && nParts == 1`, drawn from the original arrays on
  `getPartsPtr(pHead->partsNo)`) fall out as single-joint weights. `WeightExt` (u16 ids,
  `weight_ext_num > 0xFF`) does not occur on the disc and is rejected.
- Materials: `materialSetup` binds `ModelPart.texId` on TEX0 (`GXSetTexCoordGen2(..., GX_TG_TEX0)`,
  identity matrix, `GX_REPEAT` from the TPL headers); `alphaSetup` (part flags bit2) multiplies the
  alpha of `alphaTex` in and alpha-compares `alpha > alphaRef` → glTF `MASK` with cutoff
  `(alphaRef + 1) / 255`, the alpha texture's alpha composed into the base PNG. Bump (flags bit0,
  `bumpTex`, indirect stage), specular / environment stages and texture blend tables are not
  exported (recorded in the primitive's `extras.re4`). Triangle winding: the GX order is
  counter-clockwise towards the vertex normals on every disc model checked (0 to 9 disagreeing
  faces per model), so it is kept as the glTF front face.
- Textures (`gxtex.py`): TPL descriptors → `GXInitTexObj` fields. Formats on the disc: CMPR (2159
  textures), IA8 (373), I4 (244); decoded to RGBA8 PNGs in `textures/` next to the output
  (`<archive>_<tpl entry>_<tex id>[_a<alpha id>].png`, shared by the motions of an archive). CMPR
  follows Dolphin's decoder (8×8 tiles of four DXT1 blocks, indices MSB first, 5/8-3/8 blend,
  transparent black fourth colour); only the base mip level is decoded (9 textures have mips).
- Morph targets (`ModelData::shapeOfs`, `game/shape.cpp`): the head models carry a shape table,
  `u32 count` then `count × ShapeEntry {u32 ofs, s32 num}` (offsets from `shapeOfs + 4`, the lists
  back to back) and per shape `num × {s16 vertex index, s16 dx, dy, dz}` in the vertex units
  (`1 / (1 << shift)` mm). Every frame `commonScreenMatSub` copies `vtxOrig` into the vertex buffer
  (`ResetShape`) and `CalculateShape_new` adds `weight × delta` for each of the five shape slots
  (`weight` = the ShapeData channel's Hermite key in percent / 100, ×1.37 for `shapeFlags` bit 3),
  before the skinning; the normals are not touched. Each shape becomes a glTF POSITION morph
  target on every primitive of the head mesh (named `shape_NN` in `extras.targetNames`: the game has
  no names; Blender shows them as shape keys), so shape key 1.0 = the game's 100 %. Which key is
  which: `cPlLeon::setFace(1)` plays PL_ARC 0x62 (channel on shape 0, the pain face), `setFace(2)`
  0x63 (shape 1); Ada (pl0c) has one shape; Ashley's two are played by the events. On the disc 12
  of 1830 models have a shape table (22 keys: pl00/08/09/10 and em32 Leon heads, pl01/05/11/15/21
  Ashley, pl0b/0c Ada), deltas at most 19.7 mm (Ashley's open mouth).

Verified by:

1. **Byte round-trip** (`verify`): every `BIN` on disc 1 (1830 of 1830, players, enemies and
   weapons) parses into vertices, normals, colours, texcoords, weights, parts headers, display-list
   primitives, shape table, blend and flip tables and re-serialises to identical bytes, with the
   zero / 0xCD padding and the shape entries' offsets regenerated (not copied), the colour and
   texcoord counts taken from the display lists (the padding after them must be zero), and every
   vertex index (display lists and shape deltas) range-checked. Every TPL parses (1179 palettes,
   2776 textures decoded); the lossless formats re-encode byte-identically (617 of 617 I4 / IA8).
   CMPR is lossy: checked by eye on Leon's jacket / face textures. Every entry of the character
   table loads from the disc (45 of 45).
2. **Blender** (`--blender-check`): asserts the mesh objects are exactly the exporter's models
   (names) with its vertex and triangle counts, are skinned to the armature, have UVs, colours and
   node materials with the images; that the shape keys are exactly the head's morph targets, each
   moving some vertices and none by more than 60 mm; that the deformed mesh stays inside the
   bones' bounding box + 25 % at the sampled frames (an exploded vertex or a stretched limb fails
   it); renders textured (workbench, `color_type='TEXTURE'`); `--strip` renders every frame at the
   motion's fps into `<name>_strip.png` and `<name>.mp4` (ffmpeg). `--render-nice DIR` adds EEVEE
   renders with three area lights (key / fill / rim scaled to the character's height) and the
   camera framed on the skinned mesh's bounding box over the motion (`<name>_nice_fNNN.png`, first
   and middle frame), plus a head close-up per shape key at 1.0 and the neutral head
   (`<name>_nice_<mesh>_shape_NN.png`).

Known gaps: TEV materials (bump / specular / texture-blend stages, the indirect stage) and mip
levels are not exported, only the base texture with the alpha test; the room archives (`St*/rNNN.das`,
yz2-compressed) are not opened; 12 motions do not round-trip (below); the enemy table covers the
village Ganado (em10 type 0) only: the other Ganado types / enemies pair their heads and hands in
their `emNN_set.cpp` (`--mesh` for those); the game turns the eye parts by code, so exported eyes
look straight ahead; one duplicated face in pl00:4 is dropped (Blender removes it on import). The
host helper is built with `make` in `host/`; on Windows that needs a C++ toolchain (MSYS2 / MSVC).

## What is verified, and how

1. **Byte round-trip of the parse** (`verify`): every FCV and SEQ entry is parsed into keys
   (`fcv.py`) and re-serialised; the bytes must be identical, including the size word, the
   exporter's joint block order and the 0xCD padding. Disc 1: 9261 of 9273 motions and all 6351
   sequence tables. The 12 failures are named: 11 weapon motions (wep09/10/21/24/31/32/47) whose
   last attach-camera joints (channel 6) have key blocks in the padding, at the end, or on a
   duplicated offset, and pl14:8 (a 1-joint stub whose key block is padding); the game only
   evaluates channel-6 joints with an attach camera set.
2. **The evaluation is the game's code** (`host/`): `src/game/motion.cpp` (MotionSetCore,
   MotionMove, MotionMoveCore, HermiteInterpolation, Fcc_get_data_*), `src/game/ik.cpp`
   (IKInit, InverseKinematics), `cModel::partsMatCalc/partsWorldCalc/getPartsPtr/matBlend` from
   `model.cpp`, `hermite/RotMatrix/TransMatrix/ScaleMatrix/VecRadLimit/SetOrientation*/VecAngle`
   from `math_sub.cpp`, `GetDistance3` from `sub2.cpp`, the SDK's C `MTX/VEC/QUAT` routines
   (`src/lib`) under the `PS*` names, and the game's libm `sinf/cosf/acosf` (`src/lib` fdlibm).
   `host/prepare.py` writes host copies into `host/build/` (register pins `asm("rN")` stripped,
   `(u32)` pointer casts widened to `uintptr_t`, the `MOT_HIST` byte-offset macro turned into the
   member access; functions extracted verbatim by name). The game tree is not edited. Stub headers
   (`host/stub/`) supply the 32-bit types and a `cModel` with the members the code uses; offsets
   differ from the GameCube layout, so the offset macros of `motion.h` (`MOTION_PARTS`, `IK_PARTS`,
   `PARTS_BIND_MAT`) are redirected to members. Hand-written: the driver (`motion_host.cpp`), the
   log / camera / floor stubs, and C versions of the paired-single-only `SQRTF` (frsqrte + Newton
   in the game; correctly rounded sqrt here) and `LIMIT_ANGLE`.
   The motion image is handed to the helper little-endian (same layout, fields byte-swapped, from
   `fcv.serialise(m, '<')`) so the game's byte-wise readers produce the GameCube values; it lives in
   memory below 4 GB (`MAP_32BIT`) because MotionSetCore relocates the key offsets as 32-bit
   pointers. Python (`evalhost.py`) builds the cModel from the `.bin` parts records exactly like
   `cModel::initJoint` (rest positions, parent links, bind matrices, blend table), calls
   MotionSetCore(flags 0) and, per frame, MotionMove with the sequence frame forced (Mot_attr
   0x8000). So the poses include the IK (effector position keys: the feet/hands, no floor on the
   host) and the double-joint quaternion blends, as in the game.

   **For anyone writing their own importer:** the position keys on the IK end effectors (ankles,
   hands; chain roots have kind bit 0x10/0x20) are targets in model space for `ik.cpp`'s solver,
   not the joint's own translation. Reading them as bone translations gives stretched or folded
   limbs — the usual failure of RE4 animation importers. Either run the IK (this tool does) or
   drop those keys and accept approximate limbs. Details in `fcv.py`'s header.
   `make fma` builds a variant with `-mfma -ffp-contract=fast` (fused multiply-adds like the PowerPC
   `fmadds`); `--fma` selects it.
3. **Check against the real game** (`dolphin.py`): `verify --dolphin` boots the debug disc in
   headless `dolphin-emu-nogui` (one instance under `timeout`, no window, no audio), drives the pad
   through Dolphin's Pipe input until the player model plays a motion, and dumps the player's
   `MotionWork` and parts (`pPL` = `*0x803159F4`, `cModel::Motion` at +0x1D8, parts chain at +0xF4)
   from the emulated MEM1 (read through `/proc/<pid>/mem`, the 32 MiB shared mapping). The motion
   playing is identified from `MotionWork::pMot - PL_DATA_ADDR (0x807EC000)` = the body offset of
   the FCV entry. `verify --dump file.bin` compares an existing dump (layout in `dolphin.py`
   `DUMP_LAYOUT`) with the helper's pose at the same frame and prints the max abs error of
   `ang/pos/scale`, `l_mat`, `mat` per parts class (motion keys only / IK chains + blend-table
   joints / parts no motion joint drives). Result on the opening event motion (pl00 entry 117,
   frames 12..70, `--fma-too`): key-driven angles differ by <= 6e-8 rad (0 with the FMA build),
   positions by <= 6.1e-5 mm (1 ULP at 1000 mm), world matrices by 1-2 ULP of the world
   coordinates (~1e5 mm); IK / blend parts by <= 2.3e-2 mm (frsqrte-based normalisation in the
   game); the head's eye/face parts differ because pl_leon poses them outside the motion.
4. **Blender** (`export --blender-check [dir]`): `blender -b --python blender_check.py` imports the
   glTF, asserts the bone count and the action frame range, compares bone world positions with the
   helper's parts world positions at four frames, checks the mesh (above), and renders four frames
   (workbench, textured).

## Files

    motion_export.py   CLI (tools/)
    fcv.py             MotionData / sequence table parse + byte-exact serialise (format notes in the docstring)
    modelbin.py        model .bin parts hierarchy, rest pose, blend table
    meshbin.py         model .bin mesh data: arrays, weights, display lists, shape table; byte-exact serialise (format notes in the docstring)
    character.py       archive -> attachment models (bin, tpl, role) read off the game's setModel code
    gxtex.py           TPL parse, CMPR / IA8 / I4 decode, I4 / IA8 encode, PNG writer
    archive.py         .drs containers via tools/drs.py, disc extraction via dtk
    evalhost.py        ctypes front end: Player (hosted cModel), Pose
    gltf.py, bvh.py    writers (gltf: skin, meshes, morph targets, materials, animation)
    dolphin.py         Dolphin harness, memory dump format, comparison
    blender_check.py   headless Blender import / compare / render
    host/              Makefile, prepare.py, motion_host.cpp, stub/ -> build/libmotion_host*.so
