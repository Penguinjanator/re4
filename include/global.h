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
    u32 ofs_1C;   // 0x1C  vibration pattern table (pl_dmg: VibSetData)
    u32 ofs_20;   // 0x20  obstacle model bin (obj20 SetObaModel)
    u32 ofs_24;   // 0x24  obstacle model tpl
    u32 ofs_28;   // 0x28  message tables (mes: MesData.ptr[0..2])
    u8 pad_2C[0x54 - 0x2C];
    u32 ofs_54;   // 0x54  message table type 3 (mes: MesData.ptr[3])
    u8 pad_58[0x6C - 0x58];
    u32 ofs_6C;   // 0x6C  system message table (dvd: MesData.ptr[4])
    u32 ofs_70;   // 0x70  TV-mode message table (tv_mode)
    u8 pad_74[0x9C - 0x74];
    u32 ofs_9C;   // 0x9C  sub-mission widget id data (stage)
};

// Player archive at pG->pPlArc: a table of byte offsets to the player's sub-files (models, textures,
// motions, faces...). The pl_* units index it directly; the pointer is `ofs + (u32) arc`.
struct PlArc {
    u32 ofs[0x100];   // pl_knife indexes up to 0x87
};
#define PL_ARC_PTR(arc, no) ((void*) ((arc)->ofs[no] + (u32) (arc)))

// Global game work (`pG`, game/main.cpp). Offsets come from the cam_ctrl unit; extend the
// pads as other units reveal more fields, never rewrite.
struct GlobalWork {
    u8 pad_0[4];
    u8 x4;                 // 0x04  (stage: sub-mission coin marker only while set)
    u8 pad_5[0x18 - 0x05];
    void* pFont;           // 0x18  ROM font header (dvd: RomFontSetting)
    s32 x1C;               // 0x1C  1 = the message system is usable (dvd error screen)
    u8 x20;                // 0x20  (main_sub: 3/4/6 allow the blur filter)
    u8 pad_21[0x28 - 0x21];
    u16 next_room;         // 0x28  room id (stage << 8 | room) being entered (snd: room BGM / door tables)
    u8 pad_2A[0x3C - 0x2A];
    void* pStageFont;      // 0x3C  stage/event font buffer (mes: MessageControl::stageInit)
    void* pRoomArc;        // 0x40  current room archive (GetDataExt(pG->pRoomArc, "STB", 0))
    u8 pad_44[4];
    struct ArcFile* pArc;       // 0x48  current archive: offsets to its sub-files (room_tex, tv_mode)
    u8 pad_4C[4];
    struct PlArc* pPlArc;       // 0x50  player archive (pl_leon/pl_push: model, motion, face data offsets)
    u32 flags_54;          // 0x54
    u32 flags_58;          // 0x58
    u32 time_base;         // 0x5C  OSTicksToSeconds at the last InitGameTime/SetGameTime
    u32 flags_60;          // 0x60
    u32 flags_64;          // 0x64
    u32 flags_68;          // 0x68
    u32 flags_6C;          // 0x6C
    u8 pad_70[4];
    Camera Cam;            // 0x74 .. 0x16C  (Cam.param at 0x118)
    u8 pad_16C[4];
    u32 flags_170;         // 0x170  stop flags (debug tools save/restore it)
    u8 pad_174[0x4F20 - 0x174];
    void* pRoomMes;        // 0x4F20  room message table (mes: MesData.ptr[1])
    void* pCoreCamData;    // 0x4F24  core camera data ("B40x")
    void* pRoomCamData;    // 0x4F28  room camera data ("B40x")
    u8 pad_4F2C[0x4F3C - 0x4F2C];
    Vec bell_pos;          // 0x4F3C  floor point under the rung bell (obj14; flags_5010 bit29)
    u8 bell_stat;          // 0x4F48  2 = bell rung
    u8 pad_4F49[0x4F70 - 0x4F49];
    Vec quake_ofs;         // 0x4F70
    u8 x4F7C;
    u8 door_no;            // 0x4F7D  door used to enter the room (index into the DSE door SE table)
    u8 pad_4F7E[0x4F92 - 0x4F7E];
    u8 snd_tbl_no;         // 0x4F92  room BGM/stream table row (0..4) selected by the game flow
    u8 x4F93;              // 0x4F93
    u32 play_time;         // 0x4F94  seconds (SetGameTime accumulates into it)
    u8 pad_4F98[4];
    union {
        u16 room_id;       // 0x4F9C  stage << 8 | room as one halfword (obj14: room 004 test)
        struct {
            u8 stage_no;   // 0x4F9C
            u8 room_no;    // 0x4F9D
        };
    };
    u8 pad_4F9E[2];
    u8 stage_prev;         // 0x4FA0  stage the current room data was loaded for (stage.cpp)
    u8 pad_4FA1[2];
    s8 emlist_no;          // 0x4FA3  enemy list currently loaded (stage.cpp), -1 = none
    u16 pl_life;           // 0x4FA4  (compared as s16 by the debug tools)
    u16 pl_life_max;       // 0x4FA6
    u16 sub_life;          // 0x4FA8  Ashley
    u16 sub_life_max;      // 0x4FAA
    u8 pad_4FAC[4];
    u8 wep_no;             // 0x4FB0  equipped weapon (cPlayer::weaponLoad(no, type))
    u8 wep_type;           // 0x4FB1
    u8 pad_4FB2[6];
    u8 x4FB8;              // 0x4FB8
    u8 costume;            // 0x4FB9  player costume (pl_leon: 2 = no cloth simulation)
    u8 x4FBA;              // 0x4FBA
    u8 costume2;           // 0x4FBB  Ashley costume (pl_cloth: 1 = ribbon + lapels instead of skirt + sweater)
    u8 pad_4FBC[0x500C - 0x4FBC];
    u32 flags_500C;        // 0x500C
    u32 flags_5010;        // 0x5010
    u32 flags_5014;        // 0x5014
    u32 flags_5018;        // 0x5018  (main_sub: 0x10000000 letterbox scissor)
    u8 pad_501C[0x51BC - 0x501C];
    u32 flags_51BC;        // 0x51BC  (stage: 0x4 stage-1 loaded, 0x40000 sub-mission 1 done)
    u32 flags_51C0;        // 0x51C0  (stage: route flags)
    u8 pad_51C4[0x51E4 - 0x51C4];
    u32 flags_51E4;        // 0x51E4  (db_cam: 0x10 show the tool banner, 0x18 show the offset headers)
    u8 pad_51E8[0x52E8 - 0x51E8];
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
