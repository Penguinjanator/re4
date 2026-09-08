#ifndef GLOBAL_H
#define GLOBAL_H

#include "types.h"
#include "vec.h"
#include "camera.h"

// Archive header at pG->pArc: a table of file offsets to the sub-files. Only the entries that
// matched units use are named.
struct ArcFile {
    u8 pad_0[0x18];
    u32 ofs_18;   // 0x18  room texture data (room_tex)
    u8 pad_1C[0x70 - 0x1C];
    u32 ofs_70;   // 0x70  TV-mode message table (tv_mode)
    u8 pad_74[0x9C - 0x74];
    u32 ofs_9C;   // 0x9C  sub-mission widget id data (stage)
};

// Global game work (`pG`, game/main.cpp). Offsets come from the cam_ctrl unit; extend the
// pads as other units reveal more fields, never rewrite.
struct GlobalWork {
    u8 pad_0[4];
    u8 x4;                 // 0x04  (stage: sub-mission coin marker only while set)
    u8 pad_5[0x48 - 0x05];
    struct ArcFile* pArc;       // 0x48  current archive: offsets to its sub-files (room_tex, tv_mode)
    u8 pad_4C[8];
    u32 flags_54;          // 0x54
    u32 flags_58;          // 0x58
    u8 pad_5C[4];
    u32 flags_60;          // 0x60
    u32 flags_64;          // 0x64
    u32 flags_68;          // 0x68
    u32 flags_6C;          // 0x6C
    u8 pad_70[4];
    Camera Cam;            // 0x74 .. 0x16C  (Cam.param at 0x118)
    u8 pad_16C[4];
    u32 flags_170;         // 0x170  stop flags (debug tools save/restore it)
    u8 pad_174[0x4F24 - 0x174];
    void* pCoreCamData;    // 0x4F24  core camera data ("B40x")
    void* pRoomCamData;    // 0x4F28  room camera data ("B40x")
    u8 pad_4F2C[0x4F70 - 0x4F2C];
    Vec quake_ofs;         // 0x4F70
    u8 pad_4F7C[0x4F93 - 0x4F7C];
    u8 x4F93;              // 0x4F93
    u8 pad_4F94[8];
    u8 stage_no;           // 0x4F9C
    u8 room_no;            // 0x4F9D
    u8 pad_4F9E[2];
    u8 stage_prev;         // 0x4FA0  stage the current room data was loaded for (stage.cpp)
    u8 pad_4FA1[2];
    s8 emlist_no;          // 0x4FA3  enemy list currently loaded (stage.cpp), -1 = none
    u16 pl_life;           // 0x4FA4  (compared as s16 by the debug tools)
    u16 pl_life_max;       // 0x4FA6
    u16 sub_life;          // 0x4FA8  Ashley
    u16 sub_life_max;      // 0x4FAA
    u8 pad_4FAC[0x4FB8 - 0x4FAC];
    u8 x4FB8;              // 0x4FB8
    u8 pad_4FB9[0x500C - 0x4FB9];
    u32 flags_500C;        // 0x500C
    u32 flags_5010;        // 0x5010
    u32 flags_5014;        // 0x5014
    u8 pad_5018[0x51BC - 0x5018];
    u32 flags_51BC;        // 0x51BC  (stage: 0x4 stage-1 loaded, 0x40000 sub-mission 1 done)
    u32 flags_51C0;        // 0x51C0  (stage: route flags)
    u8 pad_51C4[0x52E8 - 0x51C4];
    u8 emlist[0x2000];     // 0x52E8  enemy list (ESL file) read by stage.cpp
    u8 pad_72E8[0x8358 - 0x72E8];
    s32 game_mode;         // 0x8358  (stage: 3 = no enemy list reload)
    u8 pad_835C[0x8678 - 0x835C];
    s8 debug_mode;         // 0x8678  debug page number (t_page), 0xF = camera rail debug draw
    s8 debug_disp;         // 0x8679  debug page shown by the game (0 = off); t_page/t_sc_shot edit it
};

extern GlobalWork* pG;
extern GlobalWork Global;  // the instance pG points at (game/main.cpp); static initializers take its address

// System save block (game/main.cpp `SystemSave`, 0x38 bytes; layout partially known).
struct SystemSaveWork {
    u32 config_flg;  // 0x00  CFG_* bits
    u32 extra_flg;   // 0x04
    u8 pad_8[0x38 - 0x08];
};
extern SystemSaveWork SystemSave;

// stage_no/room_no read as one u16 (stage << 8 | room), as cRoomData::getRoomSavePtr wants it.
#define G_ROOM_ID (*(u16*) &pG->stage_no)

// Flag helpers. The original sets/clears bits through an inline helper taking a reference: the
// store is then a plain scalar access, so GCC 2.95 assumes it may clobber `pG` and reloads it
// (and does not merge consecutive updates). A direct `pG->flags |= x` compiles differently.
static inline void BitOn(u32& f, u32 b) { f |= b; }
static inline void BitOff(u32& f, u32 b) { f &= ~b; }

// Same mechanism for a plain float store. The original compiler never hoisted a load of a
// global (pG, a static float) above a store made through `this`/a member pointer; ProDG 3.9.3
// does, unless the store goes through a scalar reference. Use where the target asm shows the
// global load after such a store (esp10, esp15, esp17 ...).
static inline void FSet(f32& d, f32 v) { d = v; }
static inline void BitOn16(u16& f, u16 b) { f |= b; }
// Plain store through the same kind of reference (debug tools restoring saved flag words).
static inline void BitSet(u32& f, u32 v) { f = v; }

// Struct-member view of pG (the pLog trick, db_log.h): a load through it is not hoisted above a
// preceding struct-member store (esp15 SetFreeWork: `w->floorY = ...; if (pGS->flags ...)`), where
// FSet would fold the address into `this` and a plain `pG` load moves above the store.
struct GlobalWorkPtr {
    GlobalWork* p;
};
#define pGS (((GlobalWorkPtr*) &pG)->p)

// Same effect for the room camera data pointer store in CameraControl::RoomDataRead.
#define G_ROOM_CAM_DATA (*(void**) ((u8*) pG + 0x4F28))

#endif
