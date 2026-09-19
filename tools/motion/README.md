# tools/motion — RE4 (GameCube) skeletal motion export

`tools/motion_export.py` reads the "FCV" motion entries of the character archives
(`files/em/plNN.drs` players, `emNN.drs` enemies, `wepNN.drs` weapons; the room `.das` archives are
yz2-compressed and not opened), plays them with the **game's own code** compiled for the host, and
writes glTF 2.0 (and BVH) animations on the model's parts hierarchy for Blender.

    python3 tools/motion_export.py list   files/em/pl00.drs            # or the disc .iso
    python3 tools/motion_export.py export files/em/pl00.drs --motion 46 -o leon_046.gltf --bvh leon_046.bvh
    python3 tools/motion_export.py export files/em/pl00.drs --all -o out/          # every motion
    python3 tools/motion_export.py export ... --blender-check /tmp/mot/render     # headless Blender import + renders
    python3 tools/motion_export.py verify orig/G4BE08/re4_debug_disc1.iso [--dump dump.bin | --dolphin]

`--motion N` is the archive entry index (the game's `PL_ARC` index is `N + 4`: the container body
starts with four header words). The skeleton defaults to entry 0 of the same archive (the body
`.bin`). Units: game millimetres × `--scale` (default 0.001 → metres), Y up, 30 fps.

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
   helper's parts world positions at four frames, and renders four frames (workbench, solid).

## Files

    motion_export.py   CLI (tools/)
    fcv.py             MotionData / sequence table parse + byte-exact serialise (format notes in the docstring)
    modelbin.py        model .bin parts hierarchy, rest pose, blend table
    archive.py         .drs containers via tools/drs.py, disc extraction via dtk
    evalhost.py        ctypes front end: Player (hosted cModel), Pose
    gltf.py, bvh.py    writers
    dolphin.py         Dolphin harness, memory dump format, comparison
    blender_check.py   headless Blender import / compare / render
    host/              Makefile, prepare.py, motion_host.cpp, stub/ -> build/libmotion_host*.so
