#ifndef GLOBAL_H
#define GLOBAL_H

#include "types.h"
#include "vec.h"
#include "camera.h"

// Archive header at pG->pArc: a table of file offsets to the sub-files. Only the entries that
// matched units use are named.
struct ArcFile {
    u8 pad_0[0x10];
    u32 ofs_10;   // 0x10  specular data (read: CoreDataRead -> SpecularInit)
    u8 pad_14[4];
    u32 ofs_18;   // 0x18  room texture data (room_tex)
    u32 ofs_1C;   // 0x1C  vibration pattern table (pl_dmg: VibSetData)
    u32 ofs_20;   // 0x20  obstacle model bin (obj20 SetObaModel)
    u32 ofs_24;   // 0x24  obstacle model tpl
    u32 ofs_28;   // 0x28  message tables (mes: MesData.ptr[0..2])
    u8 pad_2C[0x40 - 0x2C];
    u32 ofs_40;   // 0x40  global illumination texture (read: CoreDataRead -> GlobalIlmTexInit)
    u32 ofs_44;   // 0x44  specular data 2..4 (SpecularInit)
    u32 ofs_48;   // 0x48
    u32 ofs_4C;   // 0x4C
    u8 pad_50[4];
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

// TEV stage / texture map / texture coord counters the model renderer allocates from (pG+0x184).
struct GxStageWork {
    s32 tevStage;  // 0x00
    s32 texMap;    // 0x04
    s32 texCoord;  // 0x08
};

// Global game work (`pG`, game/main.cpp). Offsets come from the cam_ctrl unit; extend the
// pads as other units reveal more fields, never rewrite.
struct GlobalWork {
    s32 dev_mode;          // 0x00  1 = development hardware (main: OSGetConsoleType & 0xF0000000)
    u8 x4;                 // 0x04  (stage: sub-mission coin marker only while set)
    u8 pad_5[3];
    u32 x8;                // 0x08  (main: bit 31 saved into pRK->x3C)
    u8 pad_C[0x18 - 0x0C];
    void* pFont;           // 0x18  ROM font header (dvd: RomFontSetting)
    s32 x1C;               // 0x1C  1 = the message system is usable (dvd error screen)
    u8 x20;                // 0x20  (main_sub: 3/4/6 allow the blur filter)
    u8 pad_21[0x24 - 0x21];
    u32 vtx_buf_no;        // 0x24  double-buffer index into cModelInfo::pPosBuf/pNrmBuf (mirror)
    u16 next_room;         // 0x28  room id (stage << 8 | room) being entered (snd: room BGM / door tables)
    u8 pad_2A[0x3C - 0x2A];
    void* pStageFont;      // 0x3C  stage/event font buffer (mes: MessageControl::stageInit)
    void* pRoomArc;        // 0x40  current room archive (GetDataExt(pG->pRoomArc, "STB", 0))
    void* pWepArc;         // 0x44  weapon data (read: ReadWepData)
    struct ArcFile* pArc;       // 0x48  current archive: offsets to its sub-files (room_tex, tv_mode)
    void* pOptionData;     // 0x4C  SS/<lang>/option.dat (read: OptionDataRead)
    struct PlArc* pPlArc;       // 0x50  player archive (pl_leon/pl_push: model, motion, face data offsets)
    u32 flags_54;          // 0x54
    u32 flags_58;          // 0x58
    u32 time_base;         // 0x5C  OSTicksToSeconds at the last InitGameTime/SetGameTime
    u32 flags_60;          // 0x60
    u32 flags_64;          // 0x64
    u32 flags_68;          // 0x68
    u32 flags_6C;          // 0x6C
    f32 mot_speed;         // 0x70  motion frame step per game frame (MotionSequenceCtrl: speed * mot_speed)
    Camera Cam;            // 0x74 .. 0x16C  (Cam.param at 0x118)
    u8 pad_16C[4];
    u32 flags_170;         // 0x170  stop flags (debug tools save/restore it)
    u32 flags_174;         // 0x174  (pl_sub joyFireOn: 0x20000000 in room 11C while flags_5014 bit31 is set)
    u8 pad_178[0x184 - 0x178];
    GxStageWork gxStage;   // 0x184  TEV stage / texmap / texcoord counters of the model renderer (mirror)
    u8 pad_190[0x4F20 - 0x190];
    void* pRoomMes;        // 0x4F20  room message table (mes: MesData.ptr[1])
    void* pCoreCamData;    // 0x4F24  core camera data ("B40x")
    void* pRoomCamData;    // 0x4F28  room camera data ("B40x")
    void* pRoomRtp;        // 0x4F2C  room "RTP" data (read: ReadAreaData)
    void* pRoomEmi;        // 0x4F30  room "EMI" data
    void* pRoomOsd;        // 0x4F34  room "OSD" data
    u8 pad_4F38[4];
    Vec bell_pos;          // 0x4F3C  floor point under the rung bell (obj14; flags_5010 bit29)
    u8 bell_stat;          // 0x4F48  2 = bell rung
    u8 pad_4F49[0x4F70 - 0x4F49];
    Vec quake_ofs;         // 0x4F70
    u8 x4F7C;
    u8 door_no;            // 0x4F7D  door used to enter the room (index into the DSE door SE table)
    u8 pad_4F7E[0x4F88 - 0x4F7E];
    u8 x4F88;              // 0x4F88  (pl_sub PlGachaGet: > 2 keeps the raw button count)
    u8 pad_4F89[0x4F92 - 0x4F89];
    u8 snd_tbl_no;         // 0x4F92  room BGM/stream table row (0..4) selected by the game flow
    u8 x4F93;              // 0x4F93
    u32 play_time;         // 0x4F94  seconds (SetGameTime accumulates into it)
    u32 x4F98;             // 0x4F98  (pl_sub PlSelect swaps it with x832C)
    union {
        u32 room_id32;     // 0x4F9C  stage/room and the two bytes after them as one word (em_set EmSetDie: `& 0xFFFF0000`)
        u16 room_id;       // 0x4F9C  stage << 8 | room as one halfword (obj14: room 004 test)
        struct {
            u8 stage_no;   // 0x4F9C
            u8 room_no;    // 0x4F9D
            u8 x4F9E;
            u8 x4F9F;      // 0x4F9F  (main: cleared with the room id on flags_54 bit 3)
        };
    };
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
    u8 pad_4FB2;
    u8 wep_lv;             // 0x4FB3  weapon upgrade level (em_dm_val: WeaponLevelTbl column, clamped to 7)
    u8 pad_4FB4[4];
    u8 x4FB8;              // 0x4FB8
    u8 costume;            // 0x4FB9  player costume (pl_leon: 2 = no cloth simulation)
    u8 x4FBA;              // 0x4FBA
    u8 costume2;           // 0x4FBB  Ashley costume (pl_cloth: 1 = ribbon + lapels instead of skirt + sweater)
    u8 pad_4FBC[2];
    u16 flags_4FBE;        // 0x4FBE  bit0: player data changed (pl_sub PlSelect/PlSetCostume/PlChangeData)
    u8 pad_4FC0[0x500C - 0x4FC0];
    u32 flags_500C;        // 0x500C
    u32 flags_5010;        // 0x5010
    u32 flags_5014;        // 0x5014
    u32 flags_5018;        // 0x5018  (main_sub: 0x10000000 letterbox scissor)
    u32 em_dead[13][8];    // 0x501C  per enemy list (emlist_no): one bit per list entry, set when the enemy died (em_set)
    u32 flags_51BC;        // 0x51BC  (stage: 0x4 stage-1 loaded, 0x40000 sub-mission 1 done)
    u32 flags_51C0;        // 0x51C0  (stage: route flags)
    u8 pad_51C4[0x51E4 - 0x51C4];
    u32 flags_51E4;        // 0x51E4  (db_cam: 0x10 show the tool banner, 0x18 show the offset headers)
    u8 pad_51E8[0x52E8 - 0x51E8];
    u8 emlist[0x2000];     // 0x52E8  enemy list (ESL file) read by stage.cpp
    u8 pad_72E8[0x832C - 0x72E8];
    u32 x832C;             // 0x832C  (pl_sub PlSelect swaps it with x4F98 when the player changes)
    u8 pad_8330[0x833C - 0x8330];
    u32 em_die_cnt;        // 0x833C  enemies killed (em_set EmSetDieCnt)
    u32 em_die_cnt2;       // 0x8340
    u32 shotHit;           // 0x8344  (pl_wep PlWepHitCheck2: shots that hit something)
    u32 shotHit2;          // 0x8348
    u32 shotTotal;         // 0x834C  shots fired
    u32 shotTotal2;        // 0x8350
    u8 x8354;              // 0x8354  (main systemWorkInit: 5)
    u8 pad_8355[3];
    s32 game_mode;         // 0x8358  (stage: 3 = no enemy list reload)
    u8 pad_835C[0x8678 - 0x835C];
    s8 debug_mode;         // 0x8678  debug page number (t_page), 0xF = camera rail debug draw
    s8 debug_disp;         // 0x8679  debug page shown by the game (0 = off); t_page/t_sc_shot edit it
    u8 pad_867A[0x8680 - 0x867A];  // sizeof == 0x8680 (main: memclr_asm(pG, sizeof(GlobalWork)))
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
// `f &= ~b` with b a parameter keeps the 32-bit mask: `rlwinm` instead of the folded `andi.` (pl_sub).
static inline void BitOff16(u16& f, u16 b) { f &= ~b; }
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
