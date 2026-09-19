# tools/motion — RE4 (GameCube) skeletal motion export

`tools/motion_export.py` reads the "FCV" motion entries of the character archives
(`files/em/plNN.drs` players, `emNN.drs` enemies, `wepNN.drs` weapons; the room `.das` archives are
yz2-compressed and not opened), plays them with the **game's own code** compiled for the host, and
writes glTF 2.0 (and BVH) animations on the model's parts hierarchy for Blender, with the
character's skinned, textured mesh from the model `.bin` ("BIN") and texture palette ("TPL") entries.

    python3 tools/motion_export.py list   files/em/pl00.drs            # or the disc .iso
    python3 tools/motion_export.py export files/em/pl00.drs --motion 46 -o leon_046.gltf --bvh leon_046.bvh
    python3 tools/motion_export.py export files/em/pl00.drs --motion 33 -o leon_033.gltf \
        --mesh 6:1 --mesh 9:1 --mesh 4:3 --mesh 2:3 --mesh 5:3 --mesh 14:13 --mesh 17:13   # full Leon
    python3 tools/motion_export.py export files/em/pl00.drs --all -o out/          # every motion
    python3 tools/motion_export.py export ... --blender-check /tmp/mot/render [--strip]  # headless Blender import + renders
    python3 tools/motion_export.py verify orig/G4BE08/re4_debug_disc1.iso [--dump dump.bin | --dolphin]

`--motion N` is the archive entry index (the game's `PL_ARC` index is `N + 4`: the container body
starts with four header words). The skeleton defaults to entry 0 of the same archive (the body
`.bin`), its textures to the entry after it (`--tpl N`). `--mesh BIN[:TPL]` adds attachment models
of the archive that the game hangs on the same parts (`cPlLeon::setModel`: costume 6:1, face 9:1,
head 4:3, hair 2:3, eyes 5:3, right hand 14:13, left hand 17:13; Ada pl02: hair 2:3, dress 5:6;
Ganado em10: body 440:441). `--no-mesh` writes the skeleton with the old stick-figure placeholder.
Units: game millimetres × `--scale` (default 0.001 → metres), Y up, 30 fps.

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

Verified by:

1. **Byte round-trip** (`verify`): every `BIN` on disc 1 (1830 of 1830, players, enemies and
   weapons) parses into vertices, normals, colours, texcoords, weights, parts headers, display-list
   primitives, blend and flip tables and re-serialises to identical bytes, with the zero / 0xCD
   padding regenerated (not copied), the colour and texcoord counts taken from the display lists
   (the padding after them must be zero), and every vertex index range-checked. Every TPL parses
   (1179 palettes, 2776 textures decoded); the lossless formats re-encode byte-identically (617 of
   617 I4 / IA8). CMPR is lossy: checked by eye on Leon's jacket / face textures.
2. **Blender** (`--blender-check`): asserts the mesh objects hold the exporter's vertex and
   triangle counts, are skinned to the armature, have UVs, colours and node materials with the
   images; that the deformed mesh stays inside the bones' bounding box + 25 % at the sampled frames
   (an exploded vertex or a stretched limb fails it); renders textured (workbench,
   `color_type='TEXTURE'`); `--strip` renders every frame at the motion's fps into
   `<name>_strip.png` and `<name>.mp4` (ffmpeg).

Known gaps: the shape (morph target) table of the head models (`shapeOfs`, `game/shape.cpp`) is
carried as raw bytes, not decoded; bump / specular / texture-blend stages and mip levels are not
exported; enemy heads and hands live in separate `.bin`s whose pairing with the body is in the
enemy code (pass them with `--mesh`); Ada's hand models (pl02:14/15) want 7 textures, more than any
TPL of pl02, so they are not exported; one duplicated face in pl00:4 is dropped (Blender removes it
on import).

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
    meshbin.py         model .bin mesh data: arrays, weights, display lists; byte-exact serialise (format notes in the docstring)
    gxtex.py           TPL parse, CMPR / IA8 / I4 decode, I4 / IA8 encode, PNG writer
    archive.py         .drs containers via tools/drs.py, disc extraction via dtk
    evalhost.py        ctypes front end: Player (hosted cModel), Pose
    gltf.py, bvh.py    writers (gltf: skin, meshes, materials, animation)
    dolphin.py         Dolphin harness, memory dump format, comparison
    blender_check.py   headless Blender import / compare / render
    host/              Makefile, prepare.py, motion_host.cpp, stub/ -> build/libmotion_host*.so
