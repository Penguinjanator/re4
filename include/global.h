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
    u32 ofs_14;   // 0x14  core effect data (eff_sys: EspDataLoad owner 0)
    u32 ofs_18;   // 0x18  room texture data (room_tex)
    u32 ofs_1C;   // 0x1C  vibration pattern table (pl_dmg: VibSetData)
    u32 ofs_20;   // 0x20  obstacle model bin (obj20 SetObaModel)
    u32 ofs_24;   // 0x24  obstacle model tpl
    u32 ofs_28;   // 0x28  message tables (mes: MesData.ptr[0..2])
    u32 ofs_2C;   // 0x2C  core light data (game: cLightMgr::roomInit core cLit)
    u32 ofs_30;   // 0x30  core camera data (game: CameraControl::CoreDataRead)
    u32 ofs_34;   // 0x34
    u32 ofs_38;   // 0x38
    u32 ofs_3C;   // 0x3C  light path data (game: cLightMgr::initPath)
    u32 ofs_40;   // 0x40  global illumination texture (read: CoreDataRead -> GlobalIlmTexInit)
    u32 ofs_44;   // 0x44  specular data 2..4 (SpecularInit)
    u32 ofs_48;   // 0x48
    u32 ofs_4C;   // 0x4C
    u32 ofs_50;   // 0x50  debug effect data (eff_sys: EspDataLoad owner 0xD1)
    u32 ofs_54;   // 0x54  message table type 3 (mes: MesData.ptr[3])
    u32 ofs_58;   // 0x58  item examine light cuts 0..4 (examine ItemExamine::init)
    u32 ofs_5C;   // 0x5C
    u32 ofs_60;   // 0x60
    u32 ofs_64;   // 0x64
    u32 ofs_68;   // 0x68
    u32 ofs_6C;   // 0x6C  system message table (dvd: MesData.ptr[4])
    u32 ofs_70;   // 0x70  TV-mode message table (tv_mode)
    u32 ofs_74;   // 0x74  HUD id textures (cockpit: IdTexDataLoad(.., 4))
    u32 ofs_78;   // 0x78
    u32 ofs_7C;   // 0x7C  life meter id data (cockpit, type 0x21)
    u32 ofs_80;   // 0x80  action button id data (cockpit, type 0x20)
    u32 ofs_84;   // 0x84  count-down id data (cockpit, type 0x23)
    u32 ofs_88;   // 0x88  HUD id data type 0x30 (cockpit)
    u32 ofs_8C;   // 0x8C
    u32 ofs_90;   // 0x90
    u32 ofs_94;   // 0x94  message window id data (cockpit, type 0x2F)
    u32 ofs_98;   // 0x98  bullet icon id data (cockpit, type 0x32)
    u32 ofs_9C;   // 0x9C  sub-mission widget id data (stage)
};

// Player archive at pG->pPlArc: a table of byte offsets to the player's sub-files (models, textures,
// motions, faces...). The pl_* units index it directly; the pointer is `ofs + (u32) arc`.
struct PlArc {
    u32 ofs[0x100];   // pl_knife indexes up to 0x87
};
#define PL_ARC_PTR(arc, no) ((void*) ((arc)->ofs[no] + (u32) (arc)))

// Room archive at pG->pRoomArc: offsets to its sub-files (GetDataExt finds them by tag; ctrl14 indexes it).
struct RoomArc {
    u32 ofs[0x10];
};
#define ROOM_ARC_PTR(arc, no) ((void*) (((RoomArc*) (arc))->ofs[no] + (u32) (arc)))

// TEV stage / texture map / texture coord counters the model renderer allocates from (pG+0x184).
struct GxStageWork {
    s32 tevStage;  // 0x00
    s32 texMap;    // 0x04
    s32 texCoord;  // 0x08
};

// Item left in a room (pG->save_item[256], game/sce_at.cpp), 16 bytes.
struct ITEM_SAVE_WORK {
    u8 item_type;          // 0x00  0 item area, 1 item handed to an area
    u8 item_at;          // 0x01
    s8 item_eff;       // 0x02
    u8 pad_3;
    u16 room_no;         // 0x04  0 = free
    u16 item_id;           // 0x06
    u16 item_num;          // 0x08
    s16 pos[3];       // 0x0A  / 10
};

// Global game work (`pG`, game/main.cpp). Offsets come from the cam_ctrl unit; extend the
// pads as other units reveal more fields, never rewrite.
struct GlobalWork {
    s32 dev_mode;          // 0x00  1 = development hardware (main: OSGetConsoleType & 0xF0000000)
    u8 shooting_mode;      // 0x04  shooting range mode (title: shoot_mode[] name table; em10/em39: 9999 damage, marker lines)
    u8 save_no;            // 0x05  save file number last loaded/saved (card dataSelect)
    u8 pad_6[2];
    u32 CardStatus;                // 0x08  card flags (card: 4 loaded, 8/0x10/0x20/0x40/0x80 CardSave modes, bit 31 first check done; main: bit 31 saved into pRK->x3C)
    u8 pad_C[4];
    u64 card_serial;       // 0x10  serial of the card the save file came from (card)
    void* pFont;           // 0x18  ROM font header (dvd: RomFontSetting)
    s32 IsMessageInit;     // 0x1C  1 = the message system is usable (mes sets it; dvd error screen tests it)
    union {
        u32 mode32;        // 0x20  x20..x23 as one word (game: gameOption saves/restores it in Game.mode_bak)
        struct {
            u8 Rno0;        // 0x20  game task step (game_func_tbl index; main_sub: 3/4/6 allow the blur filter)
            u8 Rno1;        // 0x21  sub step (room_jmp roomJumpExit clears x21..x23 with x20 = 4)
            u8 Rno2;
            u8 Rno3;
        };
    };
    u32 vtx_buf_no;        // 0x24  double-buffer index into cModelInfo::pPosBuf/pNrmBuf (mirror)
    union {
        u16 next_room;     // 0x28  room id (stage << 8 | room) being entered (snd: room BGM / door tables)
        struct {
            u8 next_stage; // 0x28  (sce_at sceAtFunc_door stores the door destination byte by byte)
            u8 next_room_no;  // 0x29
        };
    };
    u8 next_point;         // 0x2A  spawn point in the next room (room_jmp CRoomInfo::setNextPos clears it)
    u8 pad_2B;
    Vec NextPos;          // 0x2C  player position in the next room (room_jmp)
    f32 NextY;        // 0x38
    void* pStFnt;      // 0x3C  stage/event font buffer (mes: MessageControl::stageInit)
    void* pRoom;        // 0x40  current room archive (GetDataExt(pG->pRoomArc, "STB", 0))
    void* pWep;         // 0x44  weapon data (read: ReadWepData)
    struct ArcFile* pArc;       // 0x48  current archive: offsets to its sub-files (room_tex, tv_mode)
    void* pOption;     // 0x4C  SS/<lang>/option.dat (read: OptionDataRead)
    struct PlArc* pPlayer;       // 0x50  player archive (pl_leon/pl_push: model, motion, face data offsets)
    u32 System_flg;          // 0x54
    u32 Disp_flg;          // 0x58
    u32 game_start_time;         // 0x5C  OSTicksToSeconds at the last InitGameTime/SetGameTime
    u32 Debug_flg[4];      // 0x60  debug option bits ([2] 0x04000000 / [3] 0x00200000 shown in the title debug page)
    f32 mot_speed;         // 0x70  motion frame step per game frame (MotionSequenceCtrl: speed * mot_speed)
    Camera Cam;            // 0x74 .. 0x16C  (Cam.param at 0x118)
    u8 pad_16C[4];
    u32 Stop_flg;          // 0x170  stop flags (debug tools save/restore it)
    u32 Room_flg[4];       // 0x174  per-room flag words: [0] room scripts (pl_sub joyFireOn 0x20000000 in room 11C), [1] objRobo WalkHitCk bit31 = the statue caught the player, [2]/[3] cleared by SceAtWorkLoopInit every frame
    GxStageWork gxStage;   // 0x184  TEV stage / texmap / texcoord counters of the model renderer (mirror)
    Mtx mtxPalette[0xF8];  // 0x190  skinning matrix palette (trans.cpp calcWeightMat / MakeWeightPalette)
    u8 pad_3010[0x4F10 - 0x3010];  // 0x3010  GXTexObj texObj[0xF8] (trans.cpp GxWork view of 0x184..0x4F14)
    s32 prim_base;         // 0x4F10  primitive buffer: first entry of the current frame (debug PrimitiveBuffDisp)
    f32 prim_rate;         // 0x4F14  worst free ratio of the primitive buffer seen so far
    s32 prim_cnt;          // 0x4F18  entries used so far this frame
    s32 nPrim;          // 0x4F1C  entries per frame (game: ConsGetRoomValue(8), 0x8000 while stopped)
    void* RoomMes;        // 0x4F20  room message table (mes: MesData.ptr[1])
    void* pCamCore;    // 0x4F24  core camera data ("B40x")
    void* pCamRoom;    // 0x4F28  room camera data ("B40x")
    void* Rtp;        // 0x4F2C  room "RTP" data (read: ReadAreaData)
    void* pEmi;        // 0x4F30  room "EMI" data
    void* pOsd;        // 0x4F34  room "OSD" data
    s8 AreaNo;            // 0x4F38  block trigger area the player stands in (block.cpp), -1 = none
    u8 pad_4F39[3];
    Vec bell_pos;          // 0x4F3C  floor point under the rung bell (obj14; flags_5010 bit29)
    u8 bell_stat;          // 0x4F48  2 = bell rung
    u8 pad_4F49[0x4F70 - 0x4F49];
    Vec quake_ofs;         // 0x4F70
    u8 weapon_no_old;
    u8 door_no;            // 0x4F7D  door used to enter the room (index into the DSE door SE table)
    u16 cdown_add_sec;     // 0x4F7E  seconds to add to the count-down (cockpit CountDown::move consumes it)
    u8 save_data_start_addr[4];  // 0x4F80  start of the save block (game: cGameSave copies 0x4F80..0x8678)
    s32 point;             // 0x4F84  difficulty point (game GameAddPoint, 0..0x2AF7)
    u8 Game_level;         // 0x4F88  adaptive difficulty rank 1..10 = point / 1000 (em2d/em10 branch on > 1/3/6/== 10)
    u8 x4F89;
    u8 chapter;            // 0x4F8A  chapters ended (sce_com SceChapterEnd: SceSys chapter + 1)
    u8 pad_4F8B;
    u16 save_cnt;          // 0x4F8C  times saved (card makeSaveData increments it)
    u16 game_cnt;          // 0x4F8E  games cleared: nonzero = new round (merchant full tables; 1 = Merchant2ndRoundInit on load)
    u16 r_continue_cnt;    // 0x4F90  continues in this room (GameContinue increments; room jump / scene change clear it)
    u8 snd_tbl_no;         // 0x4F92  room BGM/stream table row (0..4) selected by the game flow
    u8 language;           // 0x4F93  game language (main: pSys->language; title: language_tbl[])
    u32 play_time;         // 0x4F94  seconds (SetGameTime accumulates into it)
    u32 peseta;            // 0x4F98  money (ss_shop buy/sell, item pickups; PlSelect swaps it with peseta_bak)
    union {
        u16 room_id;       // 0x4F9C  stage << 8 | room as one halfword (obj14: room 004 test)
        struct {
            u8 stage_no;   // 0x4F9C
            u8 room_no;    // 0x4F9D
        };
    };
    u8 Part;       // 0x4F9E  spawn point in the current room (copied to Part_old / next_point)
    u8 JumpPoint;  // 0x4F9F  room jump point (title/room_jmp debug jump; room scripts branch on 1/2)
    union {
        u16 room_id_prev;  // 0x4FA0  room_id of the previous room (room_jmp CRoomInfo::setNextPos)
        struct {
            u8 stage_prev; // 0x4FA0  stage the current room data was loaded for (stage.cpp)
            u8 room_prev;  // 0x4FA1
        };
    };
    u8 Part_old;              // 0x4FA2  copy of x4F9E (room_jmp)
    s8 em_list_no;          // 0x4FA3  enemy list currently loaded (stage.cpp), -1 = none
    u16 pl_life;           // 0x4FA4  (compared as s16 by the debug tools)
    u16 pl_life_max;       // 0x4FA6
    u16 ashley_life;          // 0x4FA8  Ashley
    u16 ashley_life_max;      // 0x4FAA
    u8 pad_4FAC[4];
    u8 weapon_no;             // 0x4FB0  equipped weapon (cPlayer::weaponLoad(no, type))
    u8 weapon_type;           // 0x4FB1
    u8 bullet_type;        // 0x4FB2  equipped weapon slot num >> 13 (sscrn SubScreenExit re-arms when it changed)
    u8 weapon_lv_power;    // 0x4FB3  firepower tune level (em_dm_val: WeaponLevelTbl column, clamped to 7)
    u8 weapon_lv_speed;    // 0x4FB4  firing speed tune level (PlShotFrameTbl column; item cItemMgr::arm)
    u8 weapon_lv_blt;      // 0x4FB5  capacity tune level (item cItemMgr::arm)
    u8 pad_4FB6[2];
    u8 pl_type;      // 0x4FB8  player character: 0 Leon, 1 Ashley, 2 Ada, 3 HUNK, 4 Krauser, 5 Wesker, 6 Leon+Ashley
    u8 pl_costume;    // 0x4FB9  player costume (pl_leon: 2 = no cloth simulation)
    u8 weapon_lv_reload;      // 0x4FBA
    u8 game_costume;   // 0x4FBB  Ashley costume (pl_cloth: 1 = ribbon + lapels instead of skirt + sweater)
    u8 pad_4FBC[2];
    u16 pl_flag;        // 0x4FBE  bit0: player data changed (pl_sub PlSelect/PlSetCostume/PlChangeData)
    Vec sub_pos;           // 0x4FC0  sub character start position (sce_sys ScenarioRoomInit)
    f32 sub_angle;         // 0x4FCC
    u8 pad_4FD0[0x500C - 0x4FD0];
    u32 Status_flg[4];     // 0x500C  game status bits ([3] 0x10000000: main_sub letterbox scissor)
    u32 Em_flg[12][8];    // 0x501C  per enemy list (emlist_no): one bit per list entry, set when the enemy died (em_set)
    u32 Item_flg[8];     // 0x519C  "ITEM_SET" flag words (t_flag; merchant: [0] bit 0x10000000 = item 0x40 sold)
    // 0x51BC  scenario progress bits set by the room scripts; the flag editor's SCENARIO page
    // (game/t_flag.cpp scf_s) names them, index 0 being bit 31 of word 0. DoorFlagInit presets bits
    // of words 3 to 5.
    u32 Scenario_flg[8];
    u32 Key_flg[2];        // 0x51DC  one bit per locked door (t_flag KEY_LOCK; sce_at SceAtWork::lockFlag)
    u32 Frame_cnt;         // 0x51E4  frame counter (em: `& 3` vs emset_no staggers per-enemy work; tools blink on % 30)
    u32 save_free_work[64];      // 0x51E8  scenario free words (sce_com SetFree/GetFree)
    u8 Em_list[0x2000];     // 0x52E8  enemy list (ESL file) read by stage.cpp
    ITEM_SAVE_WORK item_save[0x100];  // 0x72E8  items left in rooms (sce_at SceAtSetSaveItem)
    u32 ope_x82E8;         // 0x82E8  sub screen "Ope" block (sscrn: memset(&pG->ope_x82E8, 0, 0x44) in SubScreenGameInit)
    u8 ope_ow_type;        // 0x82EC  (sscrn OpeOwTypeSet)
    u8 pad_82ED[3];
    u32 ope_mdt_bits[3];   // 0x82F0  one bit per mdt number (sscrn OpeSetMdtNo)
    u32 ope_x82FC;         // 0x82FC  (sscrn OpeOwTypeSet clears it)
    s32 ope_mdt_no;        // 0x8300  (sscrn OpeGetMdtNo / OpeSetMdtNo; SubScreenGameInit: 0x18)
    u8 pad_8304[0x832C - 0x8304];
    u32 peseta_bak;        // 0x832C  the other character's money (PlSelect swaps it with peseta; r206 adds it back)
    s16 shootingScore[4];  // 0x8330  shooting range scores (game clearGlobalSaveData keeps 0x8330..0x8338 across the clear)
    u16 c_continue_cnt;             // 0x8338  (sce_com SceChapterEnd clears it with the kill/shot counters)
    u16 g_continue_cnt;             // 0x833A  (option: result screen counter next to x8338)
    u32 c_kill_cnt;        // 0x833C  enemies killed (em_set EmSetDieCnt)
    u32 g_kill_cnt;       // 0x8340
    u32 c_hit_cnt;           // 0x8344  (pl_wep PlWepHitCheck2: shots that hit something)
    u32 g_hit_cnt;          // 0x8348
    u32 c_shot_cnt;         // 0x834C  shots fired
    u32 g_shot_cnt;        // 0x8350
    u8 game_mode;          // 0x8354  difficulty: 1 VERY_EASY, 3 EASY, 5 NORMAL, 6 HARD (title game_mode_tbl; main systemWorkInit: 5)
    u8 pad_8355[3];
    s32 SaveKind;          // 0x8358  save kind passed to cGameSave::save (stage: 3 = no enemy list reload; -1 on continue)
    u8 pad_835C[0x8678 - 0x835C];
    s8 debug_mode;         // 0x8678  debug page number (t_page), 0xF = camera rail debug draw
    s8 debug_disp;         // 0x8679  debug page shown by the game (0 = off); t_page/t_sc_shot edit it
    u8 pad_867A[0x8680 - 0x867A];  // sizeof == 0x8680 (main: memclr_asm(pG, sizeof(GlobalWork)))
};

extern GlobalWork* pG;
extern GlobalWork Global;  // the instance pG points at (game/main.cpp); static initializers take its address

// System save block (game/main.cpp `SystemSave`, 0x38 bytes; layout partially known).
struct SystemSaveWork {
    u32 Config_flg;  // 0x00  CFG_* bits
    u32 Extra_flg;   // 0x04
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

// Flag bit indices, named as the flag editor (game/t_flag.cpp) names them; a bit the editor leaves
// blank is <category>_<index in hex>, the convention the named ones already use (DBG_5e, STA_1b). A
// word holds its first flag in bit 31, so index n of a word is 0x80000000 >> n, and the Chk macros
// below shift that bit to the sign and test it: one signed compare for index 0 of a word, a mask
// test for the rest.

// Debug_flg bits (t_flag DEBUG page)
enum DBG_FLAG {
    DBG_TEST_MODE = 0,
    DBG_SCR_TEST = 1,
    DBG_BACK_CLIP = 2,
    DBG_DBG_CAM = 3,
    DBG_SAT_DISP = 4,
    DBG_EAT_DISP = 5,
    DBG_EVENT_TOOL = 6,
    DBG_SLOW_ON = 7,
    DBG_SHADOW_POLYGON = 8,
    DBG_SCE_AT_DISP = 9,
    DBG_SCR2_TEST = 10,
    DBG_SHADOW_FRAME = 11,
    DBG_MIRROR_POLYGON = 12,
    DBG_GROUND_DISP = 13,
    DBG_SKELETON_DISP = 14,
    DBG_ESPTOOL_ONSCR = 15,
    DBG_CINESCO_OFF = 16,
    DBG_RTP_DISP = 17,
    DBG_ROOM_WIRE_DISP = 18,
    DBG_EM_YARARE_DISP = 19,
    DBG_CAM_AREA_OFF = 20,
    DBG_CLOTH_AT_DISP = 21,
    DBG_WIND_ON = 22,
    DBG_ESPTOOL_MEM_USE = 23,
    DBG_TEX_RENDER_ALL = 24,
    DBG_ESPTOOL_ONEM = 25,
    DBG_EMINFO_DISP = 26,
    DBG_LIGHT_TOOL = 27,
    DBG_1c = 28,
    DBG_1d = 29,
    DBG_1e = 30,
    DBG_1f = 31,
    DBG_COCKPIT_TOOL = 32,
    DBG_BOUNDING_DISP = 33,
    DBG_ADJUST_CAM = 34,
    DBG_FLAT_FLOOR = 35,
    DBG_OBJ_SKELETON_ = 36,
    DBG_DRAW_SH_TEX = 37,
    DBG_EM_NO_ATK = 38,
    DBG_NO_EST_CALL = 39,
    DBG_IN_ESP_TOOL = 40,
    DBG_TERM_TOOL = 41,
    DBG_WARN_LEVEL_LOW = 42,
    DBG_2b = 43,
    DBG_2c = 44,
    DBG_2d = 45,
    DBG_2e = 46,
    DGG_TIMER_STOP = 47,
    DBG_30 = 48,
    DBG_31 = 49,
    DBG_32 = 50,
    DBG_33 = 51,
    DBG_34 = 52,
    DBG_35 = 53,
    DBG_36 = 54,
    DBG_37 = 55,
    DBG_38 = 56,
    DBG_39 = 57,
    DBG_3a = 58,
    DBG_3b = 59,
    DBG_3c = 60,
    DBG_3d = 61,
    DBG_3e = 62,
    DBG_3f = 63,
    DBG_ROOMJMP = 64,
    DBG_PROC_BAR = 65,
    DBG_SCA_VIEW = 66,
    DBG_OBA_VIEW = 67,
    DBG_SLOW_MODE = 68,
    DBG_NO_SCE_EXE = 69,
    DBG_SINGLE_DISK = 70,
    DBG_BUGCHECK_MODE = 71,
    DBG_NO_DEATH = 72,
    DBG_INF_BULLET = 73,
    DBG_NO_ENEMY = 74,
    DBG_BGM_STOP = 75,
    DBG_SE_STOP = 76,
    DBG_PL_LOCK_FOLLOW = 77,
    DBG_EM_NO_DEATH = 78,
    DBG_KAIOUKEN = 79,
    DBG_PAD_INFO = 80,
    DBG_UNDER_CONST = 81,
    DBG_EM_WEAK = 82,
    DBG_EM_LIFE_DISP = 83,
    DBG_SHADOW_LIGHT = 84,
    DBG_CAPTION_OFF = 85,
    DBG_TEST_MODE_CK = 86,
    DBG_LIGHT_ERR_CHECK = 87,
    DBG_EST_CALL_CHK = 88,
    DBG_GX_WARN_ALL = 89,
    DBG_GX_WARN_MIDIUM = 90,
    DBG_GX_WARN_SEVERE = 91,
    DBG_PL_NOHIT = 92,
    DBG_SE_ERR_ALL = 93,
    DBG_5e = 94,
    DBG_AV_TEST = 95,
    DBG_INF_BULLET2 = 96,
    DBG_BATTLE_CAM = 97,
    DBG_62 = 98,
    DBG_63 = 99,
    DBG_SCISSOR_OFF = 100,
    DBG_LOG_OFF = 101,
    DBG_SCR_CHECK = 102,
    DBG_OBJ_SERVER = 103,
    DBG_START_ST2 = 104,
    DBG_ERRORL_CK = 105,
    DBG_APP_USE_DBMEM = 106,
    DBG_REFRACT_CK = 107,
    DBG_EM_NO_DIE_FLAG = 108,
    DBG_START_ST3 = 109,
    DBG_6e = 110,
    DBG_DOOR_SET_MODE = 111,
    DBG_EFF_NUM_DISP = 112,
    DBG_SET_HITMARK_ALL = 113,
    DBG_FOG_FAR_GREEN = 114,
    DBG_73 = 115,
    DBG_NO_ETC_SET = 116,
    DBG_NO_DEATH2 = 117,
    DBG_NO_PARASITE = 118,
    DBG_ESP_CHK = 119,
    DBG_NO_EVENT = 120,
    DBG_NO_LASER_LINE = 121,
    DBG_7a = 122,
    DBG_SHOP_FULL = 123,
    DBG_ADA_OMAKE_EV = 124,
    DBG_7d = 125,
    DBG_7e = 126,
    DBG_7f = 127,
};

// Status_flg bits (t_flag STATUS page)
enum STA_FLAG {
    STA_BG_OFF = 0,
    STA_PL_CHECK = 1,
    STA_PL_CHECK2 = 2,
    STA_MOVIE_ON = 3,
    STA_CUTCHG = 4,
    STA_MOVIE2_ON = 5,
    STA_SSCRN_ENABLE = 6,
    STA_CINESCO = 7,
    STA_PL_FIRE = 8,
    STA_09 = 9,
    STA_ACT_DONT_FIRE = 10,
    STA_DIEDEMO = 11,
    STA_BLUR = 12,
    STA_SUB_SCRN = 13,
    STA_CARD_ACCESS = 14,
    STA_PAD_SENSITIVE = 15,
    STA_10 = 16,
    STA_PL_ACTION = 17,
    STA_PL_INVISIBLE = 18,
    STA_EVENT = 19,
    STA_ASHLEY_HIDE = 20,
    STA_CAM_SHOULDER = 21,
    STA_WATER_ALIVE = 22,
    STA_CAMERA = 23,
    STA_BLACKOUT = 24,
    STA_SCOPE_CAMERA = 25,
    STA_RIDE_GONDOLA = 26,
    STA_1b = 27,
    STA_PL_JUMP_OFF = 28,
    STA_MIRROR = 29,
    STA_SAND_ALIVE = 30,
    STA_SELF_SHADOW = 31,
    STA_PL_SE_FOOT = 32,
    STA_PL_SE_WHISTLE = 33,
    STA_SE_BURST = 34,
    STA_SUSPEND = 35,
    STA_TEX_RENDER = 36,
    STA_THERMO_GRAPH = 37,
    STA_CAMERA_IN_ROOM = 38,
    STA_NO_LIGHTMASK = 39,
    STA_PL_SPEAR_SET = 40,
    STA_PL_SWIM = 41,
    STA_PL_BOAT = 42,
    STA_WATER_CAMERA = 43,
    STA_PL_SWIM_CAMERA = 44,
    STA_PL_LADDER = 45,
    STA_CRITICAL = 46,
    STA_TAKEAWAY = 47,
    STA_PL_CATCHED = 48,
    STA_SHADOW_EQCOL = 49,
    STA_PL_CATCHHOLD = 50,
    STA_NEARCLIP_TOUCH = 51,
    STA_CAMERA_SET_ROOM = 52,
    STA_ROOM_RAIN = 53,
    STA_USE_CAST_SHADOW = 54,
    STA_PROC_SHD_TEX = 55,
    STA_ALPHA_DRAW2 = 56,
    STA_SET_BG_COLOR = 57,
    STA_ESPGEN45_SET = 58,
    STA_EFFEM2D_TEXRND = 59,
    STA_SUB_LADDER = 60,
    STA_SUBCHAR_CTRL = 61,
    STA_ITEM_GET = 62,
    STA_LASERSITE_NOADD = 63,
    STA_PL_DONT_FIRE = 64,
    STA_PL_EM_ACTION = 65,
    STA_SUB_CATCHED = 66,
    STA_CUT_CHANGE = 67,
    STA_NO_FENCE = 68,
    STA_SSCRN_REQUEST = 69,
    STA_ESP_COMPULSION_NOSUSPEND = 70,
    STA_PL_MISS_SHOT = 71,
    STA_SUB_BULLDOZER = 72,
    STA_LIT_NO_UPDATE = 73,
    STA_MAP_DISABLE = 74,
    STA_USE_SHADOW_LIGHT = 75,
    STA_EVENT_SYSYTEM = 76,
    STA_INTO_SHOP = 77,
    STA_TIMER_NO_PAUSE = 78,
    STA_4f = 79,
    STA_50 = 80,
    STA_51 = 81,
    STA_52 = 82,
    STA_53 = 83,
    STA_54 = 84,
    STA_55 = 85,
    STA_56 = 86,
    STA_57 = 87,
    STA_58 = 88,
    STA_59 = 89,
    STA_5a = 90,
    STA_5b = 91,
    STA_5c = 92,
    STA_5d = 93,
    STA_5e = 94,
    STA_5f = 95,
    STA_SAVEDATA_NO_UPDATE = 96,
    STA_BEHIND_CAM = 97,
    STA_62 = 98,
    STA_SCISSOR = 99,
    STA_SLOW = 100,
    STA_SUB_ASHLEY = 101,
    STA_BIG_MARKER = 102,
    STA_67 = 103,
    STA_KLAUSER_TRANSFORM = 104,
    STA_69 = 105,
    STA_6a = 106,
    STA_6b = 107,
    STA_6c = 108,
    STA_6d = 109,
    STA_6e = 110,
    STA_6f = 111,
    STA_70 = 112,
    STA_71 = 113,
    STA_72 = 114,
    STA_73 = 115,
    STA_74 = 116,
    STA_75 = 117,
    STA_76 = 118,
    STA_77 = 119,
    STA_78 = 120,
    STA_79 = 121,
    STA_7a = 122,
    STA_7b = 123,
    STA_7c = 124,
    STA_7d = 125,
    STA_7e = 126,
    STA_7f = 127,
};

// System_flg bits (t_flag SYSTEM page)
enum SYS_FLAG {
    SYS_OMAKE_ADA_GAME = 0,
    SYS_OMAKE_ETC_GAME = 1,
    SYS_EXCEPTION = 2,
    SYS_RENDER_END = 3,
    SYS_SP_USED = 4,
    SYS_SOFT_RESET = 5,
    SYS_DATA_READ = 6,
    SYS_ROOMJUMP = 7,
    SYS_INVISIBLE = 8,
    SYS_DOOR_AFTER = 9,
    SYS_DOORDEMO = 10,
    SYS_TRANS_STOP = 11,
    SYS_CONTINUE = 12,
    SYS_SET_BLACK = 13,
    SYS_SN_PC_READ = 14,
    SYS_SN_PC_READ_TOOL = 15,
    SYS_HARD_RESET = 16,
    SYS_SCREEN_SHOT = 17,
    SYS_NEW_GAME = 18,
    SYS_TYPEWRITER = 19,
    SYS_SCISSOR_ON = 20,
    SYS_SCREEN_STOP = 21,
    SYS_CARD_ACCESS = 22,
    SYS_LOAD_GAME = 23,
    SYS_CONTINUE_AFTER = 24,
    SYS_START_EVT_SKIP = 25,
    SYS_HARD_MODE = 26,
    SYS_MESSAGE_INIT = 27,
    SYS_PUBLICITY_VER = 28,
    SYS_1d = 29,
    SYS_1e = 30,
    SYS_1f = 31,
};

// Stop_flg bits (t_flag STOP page)
enum SPF_FLAG {
    SPF_KEY = 0,
    SPF_CAMERA = 1,
    SPF_EM = 2,
    SPF_PL = 3,
    SPF_ESP = 4,
    SPF_OBJ = 5,
    SPF_CTRL = 6,
    SPF_LIGHT = 7,
    SPF_SCE = 8,
    SPF_SCE_AT = 9,
    SPF_CCHG = 10,
    SPF_PL_CCHG = 11,
    SPF_NOTSUBSCR = 12,
    SPF_WATER = 13,
    SPF_SPECULAR = 14,
    SPF_EARTHQUAKE = 15,
    SPF_VIBRATION = 16,
    SPF_CINESCO = 17,
    SPF_MIST = 18,
    SPF_SUBCHAR = 19,
    SPF_SE = 20,
    SPF_EVT = 21,
    SPF_BLOCK = 22,
    SPF_ACTBTN = 23,
    SPF_DATAREAD_AT = 24,
    SPF_ID_SYSTEM = 25,
    SPF_ESP_AREA = 26,
    SPF_1b = 27,
    SPF_1c = 28,
    SPF_1d = 29,
    SPF_1e = 30,
    SPF_1f = 31,
};

// Disp_flg bits (t_flag DISP page)
enum DPF_FLAG {
    DPF_EM = 0,
    DPF_PL = 1,
    DPF_SUBCHAR = 2,
    DPF_OBJ = 3,
    DPF_SCR = 4,
    DPF_ESP = 5,
    DPF_SHADOW = 6,
    DPF_WATER = 7,
    DPF_MIRROR = 8,
    DPF_CTRL = 9,
    DPF_CINESCO = 10,
    DPF_FILTER = 11,
    DPF_GLB_ILM = 12,
    DPF_CAST_SHADOW = 13,
    DPF_CLOTH = 14,
    DPF_COCKPIT = 15,
    DPF_SELF_SHADOW = 16,
    DPF_FOG = 17,
    DPF_ID_SYSTEM = 18,
    DPF_ACTBTN = 19,
    DPF_MESSAGE = 20,
    DPF_15 = 21,
    DPF_16 = 22,
    DPF_17 = 23,
    DPF_18 = 24,
    DPF_19 = 25,
    DPF_1a = 26,
    DPF_1b = 27,
    DPF_1c = 28,
    DPF_1d = 29,
    DPF_1e = 30,
    DPF_1f = 31,
};

// Scenario_flg bits (t_flag SCENARIO page)
enum SCF_FLAG {
    SCF_KEY_ID_A_GET = 0,
    SCF_KEY_ID_B_GET = 1,
    SCF_KEY_ID_C_GET = 2,
    SCF_HOOK_STALKING_R10A = 3,
    SCF_R10E_BATTLE_END = 4,
    SCF_R104_ACT_STATUE = 5,
    SCF_R01E_TEST = 6,
    SCF_R100_TEST00 = 7,
    SCF_R100_TEST01 = 8,
    SCF_R100_TEST02 = 9,
    SCF_R106_EVENT = 10,
    SCF_R117_ASHLEY_FIND = 11,
    SCF_R100_DOG_RUN = 12,
    SCF_ST1_SUB_MISSION = 13,
    SCF_R11C_BESIEGED_EVENT = 14,
    SCF_R201_EVENT00 = 15,
    SCF_R108_PUZZLE_CLEAR = 16,
    SCF_R100_KILL_GANADE_1ST = 17,
    SCF_R101_ENTER = 18,
    SCF_R103_ENTER = 19,
    SCF_R106_ENTER = 20,
    SCF_R106_CONFINEED_WITH_LUIS = 21,
    SCF_R108_CHECK_DOOR = 22,
    SCF_R10C_GET_CREST = 23,
    SCF_NO_ASHLEY_DIST_CK = 24,
    SCF_R11C_BESIEGED_END_EVENT = 25,
    SCF_R103_MANURE_RECEPTACLE = 26,
    SCF_R103_ITEM_IN_MANURE_RECEPTACLE = 27,
    SCF_R11B_END_SALAMANDER = 28,
    SCF_ST1_MAP_DAY = 29,
    SCF_ST1_MAP_NIGHT = 30,
    SCF_ST2_MAP = 31,
    SCF_ST3_MAP = 32,
    SCF_R217_PUZZLE_CLEAR = 33,
    SCF_22 = 34,
    SCF_R206_ASHLEY_RESCUE = 35,
    SCF_R101_IMPRISON = 36,
    SCF_R103_OPEN_COVER = 37,
    SCF_R20D_END_OF_ASHLEY_PLAY = 38,
    SCF_ST1_NIGHT = 39,
    SCF_ST2_IN = 40,
    SCF_STOCK_ST1_DAY = 41,
    SCF_STOCK_ST1_NIGHT = 42,
    SCF_STOCK_ST2 = 43,
    SCF_R108_OPERATOR = 44,
    SCF_R204_ASHLEY_SPLIT = 45,
    SCF_R11C_OPERATOR = 46,
    SCF_ST3_IN = 47,
    SCF_R119_DOOR_CLOSE = 48,
    SCF_R10C_TO_R10E = 49,
    SCF_ST1_NIGHT_LV_ADD = 50,
    SCF_R307_REGENERATER_APPEAR = 51,
    SCF_R316_TO_R30A_CUTBACK_EVENT = 52,
    SCF_R30D_ENTER = 53,
    SCF_36 = 54,
    SCF_37 = 55,
    SCF_38 = 56,
    SCF_39 = 57,
    SCF_R317_LEON_WOUND = 58,
    SCF_3b = 59,
    SCF_R22C_BONUS_1 = 60,
    SCF_R22C_BONUS_2 = 61,
    SCF_R22C_BONUS_3 = 62,
    SCF_R22C_BONUS_4 = 63,
    SCF_40 = 64,
    SCF_41 = 65,
    SCF_42 = 66,
    SCF_43 = 67,
    SCF_44 = 68,
    SCF_45 = 69,
    SCF_46 = 70,
    SCF_47 = 71,
    SCF_48 = 72,
    SCF_49 = 73,
    SCF_4a = 74,
    SCF_4b = 75,
    SCF_4c = 76,
    SCF_4d = 77,
    SCF_4e = 78,
    SCF_4f = 79,
    SCF_50 = 80,
    SCF_51 = 81,
    SCF_52 = 82,
    SCF_53 = 83,
    SCF_54 = 84,
    SCF_55 = 85,
    SCF_56 = 86,
    SCF_57 = 87,
    SCF_58 = 88,
    SCF_59 = 89,
    SCF_5a = 90,
    SCF_5b = 91,
    SCF_5c = 92,
    SCF_5d = 93,
    SCF_5e = 94,
    SCF_5f = 95,
    SCF_60 = 96,
    SCF_61 = 97,
    SCF_62 = 98,
    SCF_63 = 99,
    SCF_64 = 100,
    SCF_65 = 101,
    SCF_66 = 102,
    SCF_67 = 103,
    SCF_68 = 104,
    SCF_69 = 105,
    SCF_6a = 106,
    SCF_6b = 107,
    SCF_6c = 108,
    SCF_6d = 109,
    SCF_6e = 110,
    SCF_6f = 111,
    SCF_70 = 112,
    SCF_71 = 113,
    SCF_72 = 114,
    SCF_73 = 115,
    SCF_74 = 116,
    SCF_75 = 117,
    SCF_76 = 118,
    SCF_77 = 119,
    SCF_78 = 120,
    SCF_79 = 121,
    SCF_7a = 122,
    SCF_7b = 123,
    SCF_7c = 124,
    SCF_7d = 125,
    SCF_7e = 126,
    SCF_7f = 127,
    SCF_80 = 128,
    SCF_81 = 129,
    SCF_82 = 130,
    SCF_83 = 131,
    SCF_84 = 132,
    SCF_85 = 133,
    SCF_86 = 134,
    SCF_87 = 135,
    SCF_88 = 136,
    SCF_89 = 137,
    SCF_8a = 138,
    SCF_8b = 139,
    SCF_8c = 140,
    SCF_8d = 141,
    SCF_8e = 142,
    SCF_8f = 143,
    SCF_90 = 144,
    SCF_91 = 145,
    SCF_92 = 146,
    SCF_93 = 147,
    SCF_94 = 148,
    SCF_95 = 149,
    SCF_96 = 150,
    SCF_97 = 151,
    SCF_98 = 152,
    SCF_99 = 153,
    SCF_9a = 154,
    SCF_9b = 155,
    SCF_9c = 156,
    SCF_9d = 157,
    SCF_9e = 158,
    SCF_9f = 159,
    SCF_a0 = 160,
    SCF_a1 = 161,
    SCF_a2 = 162,
    SCF_a3 = 163,
    SCF_a4 = 164,
    SCF_a5 = 165,
    SCF_a6 = 166,
    SCF_a7 = 167,
    SCF_a8 = 168,
    SCF_a9 = 169,
    SCF_aa = 170,
    SCF_ab = 171,
    SCF_ac = 172,
    SCF_ad = 173,
    SCF_ae = 174,
    SCF_af = 175,
    SCF_b0 = 176,
    SCF_b1 = 177,
    SCF_b2 = 178,
    SCF_b3 = 179,
    SCF_b4 = 180,
    SCF_b5 = 181,
    SCF_b6 = 182,
    SCF_b7 = 183,
    SCF_b8 = 184,
    SCF_b9 = 185,
    SCF_ba = 186,
    SCF_bb = 187,
    SCF_bc = 188,
    SCF_bd = 189,
    SCF_be = 190,
    SCF_bf = 191,
    SCF_c0 = 192,
    SCF_c1 = 193,
    SCF_c2 = 194,
    SCF_c3 = 195,
    SCF_c4 = 196,
    SCF_c5 = 197,
    SCF_c6 = 198,
    SCF_c7 = 199,
    SCF_c8 = 200,
    SCF_c9 = 201,
    SCF_ca = 202,
    SCF_cb = 203,
    SCF_cc = 204,
    SCF_cd = 205,
    SCF_ce = 206,
    SCF_cf = 207,
    SCF_d0 = 208,
    SCF_d1 = 209,
    SCF_d2 = 210,
    SCF_d3 = 211,
    SCF_d4 = 212,
    SCF_d5 = 213,
    SCF_d6 = 214,
    SCF_d7 = 215,
    SCF_d8 = 216,
    SCF_d9 = 217,
    SCF_da = 218,
    SCF_db = 219,
    SCF_dc = 220,
    SCF_dd = 221,
    SCF_de = 222,
    SCF_df = 223,
    SCF_e0 = 224,
    SCF_e1 = 225,
    SCF_e2 = 226,
    SCF_e3 = 227,
    SCF_e4 = 228,
    SCF_e5 = 229,
    SCF_e6 = 230,
    SCF_e7 = 231,
    SCF_e8 = 232,
    SCF_e9 = 233,
    SCF_ea = 234,
    SCF_eb = 235,
    SCF_ec = 236,
    SCF_ed = 237,
    SCF_ee = 238,
    SCF_ef = 239,
    SCF_f0 = 240,
    SCF_f1 = 241,
    SCF_f2 = 242,
    SCF_f3 = 243,
    SCF_f4 = 244,
    SCF_f5 = 245,
    SCF_f6 = 246,
    SCF_f7 = 247,
    SCF_f8 = 248,
    SCF_f9 = 249,
    SCF_fa = 250,
    SCF_fb = 251,
    SCF_fc = 252,
    SCF_fd = 253,
    SCF_fe = 254,
    SCF_ff = 255,
};

// Item_flg bits (t_flag ITEM_SET page)
enum ITF_FLAG {
    ITF_DUMMY = 0,
    ITF_01 = 1,
    ITF_02 = 2,
    ITF_03 = 3,
    ITF_04 = 4,
    ITF_05 = 5,
    ITF_06 = 6,
    ITF_07 = 7,
    ITF_08 = 8,
    ITF_09 = 9,
    ITF_0a = 10,
    ITF_0b = 11,
    ITF_0c = 12,
    ITF_R101_IDCARD_A = 13,
    ITF_R10D_IDCARD_B = 14,
    ITF_R10B_IDCARD_C = 15,
    ITF_10 = 16,
    ITF_11 = 17,
    ITF_12 = 18,
    ITF_13 = 19,
    ITF_14 = 20,
    ITF_15 = 21,
    ITF_16 = 22,
    ITF_17 = 23,
    ITF_18 = 24,
    ITF_19 = 25,
    ITF_1a = 26,
    ITF_1b = 27,
    ITF_1c = 28,
    ITF_1d = 29,
    ITF_1e = 30,
    ITF_1f = 31,
    ITF_20 = 32,
    ITF_21 = 33,
    ITF_22 = 34,
    ITF_23 = 35,
    ITF_24 = 36,
    ITF_25 = 37,
    ITF_26 = 38,
    ITF_27 = 39,
    ITF_28 = 40,
    ITF_29 = 41,
    ITF_2a = 42,
    ITF_2b = 43,
    ITF_2c = 44,
    ITF_2d = 45,
    ITF_2e = 46,
    ITF_2f = 47,
    ITF_30 = 48,
    ITF_31 = 49,
    ITF_32 = 50,
    ITF_33 = 51,
    ITF_34 = 52,
    ITF_35 = 53,
    ITF_36 = 54,
    ITF_37 = 55,
    ITF_38 = 56,
    ITF_39 = 57,
    ITF_3a = 58,
    ITF_3b = 59,
    ITF_3c = 60,
    ITF_3d = 61,
    ITF_3e = 62,
    ITF_3f = 63,
    ITF_40 = 64,
    ITF_41 = 65,
    ITF_42 = 66,
    ITF_43 = 67,
    ITF_44 = 68,
    ITF_45 = 69,
    ITF_46 = 70,
    ITF_47 = 71,
    ITF_48 = 72,
    ITF_49 = 73,
    ITF_4a = 74,
    ITF_4b = 75,
    ITF_4c = 76,
    ITF_4d = 77,
    ITF_4e = 78,
    ITF_4f = 79,
    ITF_50 = 80,
    ITF_51 = 81,
    ITF_52 = 82,
    ITF_53 = 83,
    ITF_54 = 84,
    ITF_55 = 85,
    ITF_56 = 86,
    ITF_57 = 87,
    ITF_58 = 88,
    ITF_59 = 89,
    ITF_5a = 90,
    ITF_5b = 91,
    ITF_5c = 92,
    ITF_5d = 93,
    ITF_5e = 94,
    ITF_5f = 95,
    ITF_60 = 96,
    ITF_61 = 97,
    ITF_62 = 98,
    ITF_63 = 99,
    ITF_64 = 100,
    ITF_65 = 101,
    ITF_66 = 102,
    ITF_67 = 103,
    ITF_68 = 104,
    ITF_69 = 105,
    ITF_6a = 106,
    ITF_6b = 107,
    ITF_6c = 108,
    ITF_6d = 109,
    ITF_6e = 110,
    ITF_6f = 111,
    ITF_70 = 112,
    ITF_71 = 113,
    ITF_72 = 114,
    ITF_73 = 115,
    ITF_74 = 116,
    ITF_75 = 117,
    ITF_76 = 118,
    ITF_77 = 119,
    ITF_78 = 120,
    ITF_79 = 121,
    ITF_7a = 122,
    ITF_7b = 123,
    ITF_7c = 124,
    ITF_7d = 125,
    ITF_7e = 126,
    ITF_7f = 127,
};

// Test flag `no` in the word array at `base` (bit 31 - (no & 31) of word no >> 5).  The base is an
// address rather than a field so a check can read the flags through whichever pointer the caller
// holds: pG for most of the game, pGS where the code reaches them through the save block.
#define FlagChk(base, no) (*(u32*) ((((no) >> 5) << 2) + (base)) & (0x80000000 >> ((no) & 31)))

#define DbgFlagChk(n) ((s32) (pG->Debug_flg[(n) >> 5] << ((n) & 31)) < 0)
#define StaFlagChk(g, n) FlagChk((u32) &(g)->Status_flg[0], n)
#define SysFlagChk(n) ((s32) (pG->System_flg << (n)) < 0)
#define SpfFlagChk(n) ((s32) (pG->Stop_flg << (n)) < 0)
#define DpfFlagChk(n) ((s32) (pG->Disp_flg << (n)) < 0)
#define ScfFlagChk(g, n) FlagChk((u32) &(g)->Scenario_flg[0], n)
#define ItfFlagChk(n) ((s32) (pG->Item_flg[(n) >> 5] << ((n) & 31)) < 0)

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
