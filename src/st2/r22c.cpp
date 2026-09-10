#include "types.h"
class cObjWep;
#include "main_mem.h"
#include "st_room.h"
#include "atari.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "flag_rsf.h"
#include "global.h"
#include "main.h"
#include "game.h"
#include "read.h"
#include "sce.h"
#include "sce_sys.h"
#include "sce_at.h"
#include "scroll.h"
#include "obj.h"
#include "em.h"
#include "emdoor.h"
#include "em_set.h"
#include "em_wrap.h"
#include "etc_model.h"
#include "player.h"
#include "pl_sub.h"
#include "pl_npc.h"
#include "pl_wep.h"
#include "item.h"
#include "mes.h"
#include "rnd.h"
#include "snd.h"
#include "quake.h"
#include "esp.h"
#include "est.h"
#include "joy.h"
#include "sscrn.h"
#include "eprintf.h"
#include "db_log.h"

// Room 2-2C (D:/Bio4/Prog/r22c.cpp): the shooting range. The target tables below are the
// shooting game's level scripts: every record is `{time, flag, x, y, z, spd, moves...}` where a
// move is `{2, 0xF0}` (straight) or `{3, x, y, z, 0x32}` (way points) and `1` ends the record,
// `{time, 0xFE}` is a pause; a level is `{count, time, records...}`.
//
// STATUS: .data complete; the functions from itemSave on are not written yet (see the report).

class ResultScreen {
public:
    u8 work[0x78];
    void read();
    void reloadtime();
    void highscore();
    void init();
    void move();
    void quit();
};

struct R22cWork {
    u8 pad_00[0xC];
    int hits;            // 0x0C
    int score;           // 0x10
    u8 pad_14[8];
    int level;           // 0x1C  1..4
    int state;           // 0x20
    u8 pad_24[0x18];
    int wepSel;          // 0x3C
    u8 pad_40[4];
    u8 wepNo;            // 0x44
    u8 wepType;          // 0x45
    u8 pad_46[0xA];
    void* itemSaveBuf;   // 0x50
    int itemSel;         // 0x54
    int cap[24];         // 0x58
    u16 capId;           // 0xB8
    u8 pad_BA[2];
    cEm* door[2];        // 0xBC
    u8 pad_C4[4];
    cEm* wepMan;         // 0xC8
    u8 pad_CC[4];
    int itemNum[24];     // 0xD0
    ResultScreen result; // 0x130
};

static R22cWork* r22c_work;

extern u8 PlCapNum[25];   // game/pl_debug.cpp

static s32 r22c_d0[] = {0, 0, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d24[] = {300, 0, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d48[] = {600, 0, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d6C[] = {900, 0, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d90[] = {900, 0, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_dB4[] = {1200, 0, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_dD8[] = {1200, 0, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_dFC[] = {1500, 1, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d120[] = {1800, 0, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d144[] = {1800, 0, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d168[] = {2100, 254};
static s32 r22c_d170[] = {3000, 0, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d194[] = {3000, 0, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d1B8[] = {3300, 1, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d1DC[] = {3300, 0, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d200[] = {3600, 0, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d224[] = {3600, 0, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d248[] = {3600, 0, 0, 200, -18000, 0, 2, 240, 1};
static s32 r22c_d26C[] = {3900, 0, 2500, 200, -26000, 0, 3, -2500, 200, -26000, 50, 3, 2500, 200, -26000, 50, 3, -2500, 200, -26000, 50, 3, 2500, 200, -26000, 50, 1};
static s32 r22c_d2D8[] = {3900, 0, 2500, 200, -22000, 0, 3, -2500, 200, -22000, 50, 3, 2500, 200, -22000, 50, 3, -2500, 200, -22000, 50, 3, 2500, 200, -22000, 50, 1};
static s32 r22c_d344[] = {4200, 1, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d368[] = {4200, 0, 0, 200, -22000, 0, 2, 4200, 1};
static s32 r22c_d38C[] = {4200, 1, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d3B0[] = {4500, 1, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d3D4[] = {4500, 0, -2500, 200, -26000, 0, 3, -2500, 200, -22000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -22000, 100, 3, -2500, 200, -26000, 100, 1};
static s32 r22c_d440[] = {4500, 0, 2500, 200, -26000, 0, 3, 2500, 200, -22000, 100, 3, 2500, 200, -26000, 100, 3, 2500, 200, -22000, 100, 3, 2500, 200, -26000, 100, 1};
static s32* r22c_d4AC[] = {(s32*) 26, (s32*) 4800, r22c_d0, r22c_d24, r22c_d48, r22c_d6C, r22c_d90, r22c_dB4, r22c_dD8, r22c_dFC, r22c_d120, r22c_d144, r22c_d168, r22c_d170, r22c_d194, r22c_d1B8, r22c_d1DC, r22c_d200, r22c_d224, r22c_d248, r22c_d26C, r22c_d2D8, r22c_d344, r22c_d368, r22c_d38C, r22c_d3B0, r22c_d3D4, r22c_d440};
static s32 r22c_d51C[] = {0, 0, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d540[] = {300, 0, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d564[] = {600, 2, 0, 200, -22000, 0, 2, 600, 1};
static s32 r22c_d588[] = {630, 0, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d5AC[] = {630, 1, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d5D0[] = {900, 1, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d5F4[] = {900, 0, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d618[] = {300, 0, -800, 200, -22000, 0, 2, 120, 1};
static s32 r22c_d63C[] = {300, 0, 0, 200, -22000, 0, 2, 120, 1};
static s32 r22c_d660[] = {300, 0, 800, 200, -22000, 0, 2, 120, 1};
static s32 r22c_d684[] = {300, 0, 0, 200, -26000, 0, 2, 120, 1};
static s32 r22c_d6A8[] = {300, 0, 0, 200, -26000, 0, 2, 120, 1};
static s32 r22c_d6CC[] = {300, 0, 0, 200, -22000, 0, 2, 120, 1};
static s32 r22c_d6F0[] = {300, 0, 0, 200, -22000, 0, 2, 120, 1};
static s32 r22c_d714[] = {300, 2, 0, 200, -18000, 0, 2, 240, 1};
static s32 r22c_d738[] = {540, 1, 2500, 200, -22000, 0, 3, -2500, 200, -22000, 100, 3, 2500, 200, -22000, 100, 3, -2500, 200, -22000, 100, 3, 2500, 200, -22000, 100, 1};
static s32 r22c_d7A4[] = {540, 0, -2500, 200, -26000, 0, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 1};
static s32 r22c_d810[] = {300, 0, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d834[] = {300, 0, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d858[] = {300, 0, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d87C[] = {300, 2, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d8A0[] = {540, 0, 2500, 200, -26000, 0, 3, -2500, 200, -26000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, 2500, 200, -26000, 100, 1};
static s32 r22c_d90C[] = {540, 1, -2500, 200, -18000, 0, 3, 2500, 200, -18000, 100, 3, -2500, 200, -18000, 100, 3, 2500, 200, -18000, 100, 3, -2500, 200, -18000, 100, 1};
static s32 r22c_d978[] = {540, 0, -2500, 200, -26000, 0, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 1};
static s32 r22c_d9E4[] = {540, 0, -2500, 200, -26000, 0, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 1};
static s32* r22c_dA50[] = {(s32*) 22, (s32*) 1800, r22c_d51C, r22c_d540, r22c_d564, r22c_d588, r22c_d5AC, r22c_d5D0, r22c_d5F4, r22c_d618, r22c_d63C, r22c_d660, r22c_d684, r22c_d6A8, r22c_d6CC, r22c_d6F0, r22c_d714, r22c_d738, r22c_d7A4, r22c_d810, r22c_d834, r22c_d858, r22c_d87C, r22c_d8A0, r22c_d90C, r22c_d978, r22c_d9E4};
static s32 r22c_dABC[] = {0, 0, -2500, 200, -22000, 0, 2, 180, 1};
static s32 r22c_dAE0[] = {0, 0, 0, 200, -22000, 0, 2, 180, 1};
static s32 r22c_dB04[] = {180, 0, 2500, 200, -26000, 0, 2, 180, 1};
static s32 r22c_dB28[] = {180, 1, -2500, 200, -18000, 0, 2, 180, 1};
static s32 r22c_dB4C[] = {360, 2, 0, 200, -18000, 0, 2, 180, 1};
static s32 r22c_dB70[] = {360, 0, 0, 200, -26000, 0, 2, 180, 1};
static s32 r22c_dB94[] = {540, 0, 2500, 200, -18000, 0, 3, 2500, 200, -26000, 100, 3, 2500, 200, -18000, 100, 3, 2500, 200, -26000, 100, 3, 2500, 200, -18000, 100, 1};
static s32 r22c_dC00[] = {1050, 254};
static s32 r22c_dC08[] = {1200, 1, -2500, 200, -26000, 0, 3, -2500, 200, -18000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -18000, 100, 3, -2500, 200, -26000, 100, 1};
static s32 r22c_dC74[] = {1500, 0, -2500, 200, -22000, 0, 2, 90, 1};
static s32 r22c_dC98[] = {1500, 1, 2500, 200, -22000, 0, 2, 90, 1};
static s32 r22c_dCBC[] = {1620, 2, 0, 200, -22000, 0, 3, 2500, 200, -22000, 100, 3, 0, 200, -22000, 100, 3, 2500, 200, -22000, 100, 3, 0, 200, -22000, 100, 1};
static s32 r22c_dD28[] = {1620, 1, 0, 200, -26000, 0, 3, -2500, 200, -26000, 100, 3, 0, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, 0, 200, -26000, 100, 1};
static s32 r22c_dD94[] = {1620, 0, 0, 200, -18000, 0, 3, -2500, 200, -18000, 100, 3, 0, 200, -18000, 100, 3, -2500, 200, -18000, 100, 3, 0, 200, -18000, 100, 1};
static s32 r22c_dE00[] = {2100, 0, -2500, 200, -26000, 0, 2, 120, 1};
static s32 r22c_dE24[] = {2100, 0, -2500, 200, -22000, 0, 2, 120, 1};
static s32 r22c_dE48[] = {2100, 0, -2500, 200, -18000, 0, 2, 120, 1};
static s32 r22c_dE6C[] = {2250, 0, 2500, 200, -26000, 0, 2, 120, 1};
static s32 r22c_dE90[] = {2250, 2, 2500, 200, -22000, 0, 2, 120, 1};
static s32 r22c_dEB4[] = {2250, 0, 2500, 200, -18000, 0, 2, 120, 1};
static s32 r22c_dED8[] = {2400, 1, -2500, 200, -26000, 0, 2, 150, 1};
static s32 r22c_dEFC[] = {2400, 0, 0, 200, -26000, 0, 2, 150, 1};
static s32 r22c_dF20[] = {2400, 1, 2500, 200, -26000, 0, 2, 150, 1};
static s32 r22c_dF44[] = {2400, 2, -2500, 200, -22000, 0, 3, 2500, 200, -22000, 50, 3, -2500, 200, -22000, 50, 3, 2500, 200, -22000, 50, 3, -2500, 200, -22000, 50, 1};
static s32* r22c_dFB0[] = {(s32*) 24, (s32*) 2700, r22c_dABC, r22c_dAE0, r22c_dB04, r22c_dB28, r22c_dB4C, r22c_dB70, r22c_dB94, r22c_dC00, r22c_dC08, r22c_dC74, r22c_dC98, r22c_dCBC, r22c_dD28, r22c_dD94, r22c_dE00, r22c_dE24, r22c_dE48, r22c_dE6C, r22c_dE90, r22c_dEB4, r22c_dED8, r22c_dEFC, r22c_dF20, r22c_dF44};
static s32 r22c_d1018[] = {0, 0, -2500, 200, -31000, 0, 3, 0, 200, -31000, 80, 3, -2500, 200, -31000, 80, 3, 0, 200, -31000, 80, 3, -2500, 200, -31000, 80, 1};
static s32 r22c_d1084[] = {0, 0, 2500, 200, -26000, 0, 3, 0, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, 0, 200, -26000, 80, 3, 2500, 200, -26000, 80, 1};
static s32 r22c_d10F0[] = {0, 0, -2500, 200, -22000, 0, 3, 0, 200, -22000, 80, 3, -2500, 200, -22000, 80, 3, 0, 200, -22000, 80, 3, -2500, 200, -22000, 80, 1};
static s32 r22c_d115C[] = {2100, 0, -2500, 200, -26000, 0, 2, 180, 1};
static s32 r22c_d1180[] = {2100, 0, 0, 200, -26000, 0, 2, 180, 1};
static s32 r22c_d11A4[] = {2100, 0, 2500, 200, -26000, 0, 2, 180, 1};
static s32 r22c_d11C8[] = {2100, 0, -2500, 200, -22000, 0, 2, 180, 1};
static s32 r22c_d11EC[] = {2100, 0, 0, 200, -22000, 0, 2, 180, 1};
static s32 r22c_d1210[] = {2100, 0, 2500, 200, -22000, 0, 2, 180, 1};
static s32 r22c_d1234[] = {2100, 0, -2500, 200, -18000, 0, 2, 180, 1};
static s32 r22c_d1258[] = {2100, 0, 0, 200, -18000, 0, 2, 180, 1};
static s32 r22c_d127C[] = {2100, 0, 2500, 200, -18000, 0, 2, 180, 1};
static s32 r22c_d12A0[] = {2400, 0, 2500, 200, -36000, 0, 3, 2500, 200, -26000, 80, 3, 2500, 200, -36000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -36000, 80, 1};
static s32 r22c_d130C[] = {2400, 0, 0, 200, -31000, 0, 3, 0, 200, -22000, 80, 3, 0, 200, -31000, 80, 3, 0, 200, -22000, 80, 3, 0, 200, -31000, 80, 1};
static s32 r22c_d1378[] = {2400, 0, -2500, 200, -26000, 0, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 1};
static s32 r22c_d13E4[] = {2700, 0, -2500, 200, -31000, 0, 2, 180, 1};
static s32 r22c_d1408[] = {2700, 0, 2500, 200, -31000, 0, 2, 180, 1};
static s32 r22c_d142C[] = {2700, 6, 0, 200, -26000, 0, 2, 180, 1};
static s32 r22c_d1450[] = {2700, 0, -2500, 200, -22000, 0, 2, 180, 1};
static s32 r22c_d1474[] = {2700, 0, 2500, 200, -22000, 0, 2, 180, 1};
static s32* r22c_d1498[] = {(s32*) 20, (s32*) 3000, r22c_d1018, r22c_d1084, r22c_d10F0, r22c_d115C, r22c_d1180, r22c_d11A4, r22c_d11C8, r22c_d11EC, r22c_d1210, r22c_d1234, r22c_d1258, r22c_d127C, r22c_d12A0, r22c_d130C, r22c_d1378, r22c_d13E4, r22c_d1408, r22c_d142C, r22c_d1450, r22c_d1474};
static s32 r22c_d14F0[] = {0, 0, -2500, 200, -22000, 0, 2, 180, 1};
static s32 r22c_d1514[] = {0, 0, 2500, 200, -22000, 0, 2, 180, 1};
static s32 r22c_d1538[] = {300, 0, -2500, 200, -26000, 0, 2, 180, 1};
static s32 r22c_d155C[] = {300, 0, 2500, 200, -26000, 0, 2, 180, 1};
static s32 r22c_d1580[] = {600, 1, 2500, 200, -18000, 0, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, -2500, 200, -18000, 80, 3, 2500, 200, -18000, 80, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, -2500, 200, -18000, 80, 3, 2500, 200, -18000, 80, 1};
static s32 r22c_d163C[] = {600, 0, -2500, 200, -26000, 0, 3, 2500, 200, -26000, 80, 3, 2500, 200, -18000, 80, 3, 2500, 200, -26000, 80, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -18000, 80, 3, 2500, 200, -26000, 80, 3, -2500, 200, -26000, 80, 1};
static s32 r22c_d16F8[] = {1500, 2, 0, 200, -18000, 0, 2, 300, 1};
static s32 r22c_d171C[] = {1500, 1, -2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d1740[] = {1500, 0, 2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d1764[] = {1500, 1, 0, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d1788[] = {2100, 254};
static s32 r22c_d1790[] = {2400, 0, -2500, 200, -26000, 0, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 1};
static s32 r22c_d17FC[] = {2400, 1, 0, 200, -22000, 0, 3, 0, 200, -18000, 80, 3, 0, 200, -26000, 80, 3, 0, 200, -18000, 80, 3, 0, 200, -26000, 80, 3, 0, 200, -22000, 80, 1};
static s32 r22c_d187C[] = {2400, 0, 2500, 200, -18000, 0, 3, 2500, 200, -26000, 80, 3, 2500, 200, -18000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -18000, 80, 1};
static s32 r22c_d18E8[] = {3000, 1, 2500, 200, -26000, 0, 3, 2500, 200, -18000, 80, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -18000, 80, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 1};
static s32 r22c_d19A4[] = {3000, 1, -2500, 200, -18000, 0, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -18000, 80, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -18000, 80, 3, -2500, 200, -18000, 80, 1};
static s32 r22c_d1A60[] = {3600, 0, 0, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d1A84[] = {3600, 1, -2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d1AA8[] = {3600, 2, 0, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d1ACC[] = {3600, 1, 2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d1AF0[] = {3600, 0, 0, 200, -18000, 0, 2, 300, 1};
static s32* r22c_d1B14[] = {(s32*) 21, (s32*) 4500, r22c_d14F0, r22c_d1514, r22c_d1538, r22c_d155C, r22c_d1580, r22c_d163C, r22c_d16F8, r22c_d171C, r22c_d1740, r22c_d1764, r22c_d1788, r22c_d1790, r22c_d17FC, r22c_d187C, r22c_d18E8, r22c_d19A4, r22c_d1A60, r22c_d1A84, r22c_d1AA8, r22c_d1ACC, r22c_d1AF0};
static s32 r22c_d1B70[] = {0, 0, -2500, 200, -26000, 0, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 3, -2500, 200, -18000, 80, 3, -2500, 200, -26000, 80, 1};
static s32 r22c_d1BDC[] = {0, 0, 2500, 200, -26000, 0, 3, 2500, 200, -36000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -36000, 80, 3, 2500, 200, -26000, 80, 1};
static s32 r22c_d1C48[] = {600, 1, 0, 200, -31000, 0, 3, 2500, 200, -31000, 80, 3, -2500, 200, -31000, 80, 3, 2500, 200, -31000, 80, 3, -2500, 200, -31000, 80, 3, 0, 200, -31000, 80, 1};
static s32 r22c_d1CC8[] = {600, 2, 0, 200, -22000, 0, 3, -2500, 200, -22000, 80, 3, 2500, 200, -22000, 80, 3, -2500, 200, -22000, 80, 3, 2500, 200, -22000, 80, 3, 0, 200, -22000, 80, 1};
static s32 r22c_d1D48[] = {1200, 1, 2500, 200, -36000, 0, 3, 2500, 200, -26000, 80, 3, -2500, 200, -26000, 80, 3, -2500, 200, -18000, 80, 1};
static s32 r22c_d1DA0[] = {1800, 254};
static s32 r22c_d1DA8[] = {2100, 2, 0, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d1DCC[] = {2100, 2, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d1DF0[] = {2220, 0, -2500, 200, -36000, 0, 3, 2500, 200, -36000, 80, 2, 300, 1};
static s32 r22c_d1E28[] = {2220, 0, -2500, 200, -26000, 0, 3, 2500, 200, -26000, 80, 2, 300, 1};
static s32 r22c_d1E60[] = {2220, 1, -2500, 200, -18000, 0, 3, 2500, 200, -18000, 80, 2, 300, 1};
static s32 r22c_d1E98[] = {2340, 0, -2500, 200, -36000, 0, 2, 240, 1};
static s32 r22c_d1EBC[] = {2340, 0, 2500, 200, -36000, 0, 2, 240, 1};
static s32 r22c_d1EE0[] = {2340, 1, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d1F04[] = {2340, 0, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d1F28[] = {2340, 0, -2500, 200, -18000, 0, 2, 240, 1};
static s32 r22c_d1F4C[] = {2340, 1, 2500, 200, -18000, 0, 2, 240, 1};
static s32 r22c_d1F70[] = {2460, 0, -2500, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d1F94[] = {2460, 0, 2500, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d1FB8[] = {2460, 1, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d1FDC[] = {2460, 0, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d2000[] = {2460, 0, 2500, 200, -22000, 0, 2, 240, 1};
static s32* r22c_d2024[] = {(s32*) 22, (s32*) 3000, r22c_d1B70, r22c_d1BDC, r22c_d1C48, r22c_d1CC8, r22c_d1D48, r22c_d1DA0, r22c_d1DA8, r22c_d1DCC, r22c_d1DF0, r22c_d1E28, r22c_d1E60, r22c_d1E98, r22c_d1EBC, r22c_d1EE0, r22c_d1F04, r22c_d1F28, r22c_d1F4C, r22c_d1F70, r22c_d1F94, r22c_d1FB8, r22c_d1FDC, r22c_d2000};
static s32 r22c_d2084[] = {0, 1, -2500, 200, -36000, 0, 3, -2500, 200, -22000, 80, 3, -2500, 200, -36000, 80, 3, -2500, 200, -22000, 80, 3, -2500, 200, -36000, 80, 1};
static s32 r22c_d20F0[] = {0, 1, 0, 200, -22000, 0, 3, 0, 200, -36000, 80, 3, 0, 200, -22000, 80, 3, 0, 200, -36000, 80, 3, 0, 200, -22000, 80, 1};
static s32 r22c_d215C[] = {0, 1, 2500, 200, -36000, 0, 3, 2500, 200, -22000, 80, 3, 2500, 200, -36000, 80, 3, 2500, 200, -22000, 80, 3, 2500, 200, -36000, 80, 1};
static s32 r22c_d21C8[] = {900, 2, 0, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d21EC[] = {900, 0, 2500, 200, -22000, 0, 3, -2500, 200, -22000, 80, 3, -2500, 200, -26000, 80, 3, -2500, 200, -18000, 80, 1};
static s32 r22c_d2244[] = {900, 0, -2500, 200, -31000, 0, 3, 2500, 200, -31000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -36000, 80, 1};
static s32 r22c_d229C[] = {1500, 2, -2500, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d22C0[] = {1500, 2, 2500, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d22E4[] = {1500, 2, 0, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d2308[] = {1500, 2, -2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d232C[] = {1500, 2, 2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2350[] = {1530, 0, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2374[] = {1530, 0, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2398[] = {1800, 2, -2500, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d23BC[] = {1800, 2, 0, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d23E0[] = {1800, 2, 2500, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d2404[] = {1800, 2, 0, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2428[] = {1830, 1, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d244C[] = {2100, 2, 0, 200, -36000, 0, 2, 300, 1};
static s32 r22c_d2470[] = {2100, 2, -2500, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d2494[] = {2100, 2, 2500, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d24B8[] = {2100, 2, 0, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d24DC[] = {2130, 1, 0, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d2500[] = {2400, 254};
static s32 r22c_d2508[] = {2700, 2, 0, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d252C[] = {2700, 2, 0, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2550[] = {2730, 0, 0, 200, -26000, 0, 3, 2500, 200, -26000, 80, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, -2500, 200, -26000, 80, 3, 0, 200, -26000, 80, 1};
static s32 r22c_d25F8[] = {2730, 0, 0, 200, -36000, 0, 3, -2500, 200, -36000, 80, 3, 2500, 200, -36000, 80, 3, -2500, 200, -36000, 80, 3, 2500, 200, -36000, 80, 3, -2500, 200, -36000, 80, 3, 2500, 200, -36000, 80, 3, 0, 200, -36000, 80, 1};
static s32 r22c_d26A0[] = {3000, 2, 0, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d26C4[] = {3030, 0, -2500, 200, -36000, 0, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 1};
static s32 r22c_d27D0[] = {3030, 0, 2500, 200, -26000, 0, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 1};
static s32 r22c_d28DC[] = {4500, 0, -2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2900[] = {4500, 0, 0, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2924[] = {4500, 0, 2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2948[] = {4530, 0, -2500, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d296C[] = {4530, 6, 0, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d2990[] = {4530, 0, 2500, 200, -26000, 0, 2, 300, 1};
static s32 r22c_d29B4[] = {4560, 0, -2500, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d29D8[] = {4560, 0, 0, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d29FC[] = {4560, 0, 2500, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d2A20[] = {6000, 2, -2500, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d2A44[] = {6000, 2, 0, 200, -31000, 0, 2, 300, 1};
static s32 r22c_d2A68[] = {6000, 2, 0, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2A8C[] = {6000, 2, 2500, 200, -22000, 0, 2, 300, 1};
static s32 r22c_d2AB0[] = {6030, 1, -2500, 200, -26000, 0, 3, 2500, 200, -26000, 80, 3, 2500, 200, -36000, 80, 3, -2500, 200, -36000, 80, 1};
static s32 r22c_d2B08[] = {6030, 0, -2500, 200, -22000, 0, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -36000, 80, 3, 0, 200, -36000, 80, 1};
static s32 r22c_d2B74[] = {6030, 0, -2500, 200, -18000, 0, 3, -2500, 200, -26000, 80, 3, 2500, 200, -26000, 80, 3, 2500, 200, -36000, 80, 1};
static s32* r22c_d2BCC[] = {(s32*) 47, (s32*) 6300, r22c_d2084, r22c_d20F0, r22c_d215C, r22c_d21C8, r22c_d21EC, r22c_d2244, r22c_d229C, r22c_d22C0, r22c_d22E4, r22c_d2308, r22c_d232C, r22c_d2350, r22c_d2374, r22c_d2398, r22c_d23BC, r22c_d23E0, r22c_d2404, r22c_d2428, r22c_d244C, r22c_d2470, r22c_d2494, r22c_d24B8, r22c_d24DC, r22c_d2500, r22c_d2508, r22c_d252C, r22c_d2550, r22c_d25F8, r22c_d26A0, r22c_d26C4, r22c_d27D0, r22c_d28DC, r22c_d2900, r22c_d2924, r22c_d2948, r22c_d296C, r22c_d2990, r22c_d29B4, r22c_d29D8, r22c_d29FC, r22c_d2A20, r22c_d2A44, r22c_d2A68, r22c_d2A8C, r22c_d2AB0, r22c_d2B08, r22c_d2B74};
static s32 r22c_d2C90[] = {0, 2, -2500, 200, -31000, 0, 2, 270, 1};
static s32 r22c_d2CB4[] = {0, 2, 2500, 200, -31000, 0, 2, 270, 1};
static s32 r22c_d2CD8[] = {0, 2, 0, 200, -26000, 0, 2, 270, 1};
static s32 r22c_d2CFC[] = {0, 2, -2500, 200, -22000, 0, 2, 270, 1};
static s32 r22c_d2D20[] = {0, 2, 2500, 200, -22000, 0, 2, 270, 1};
static s32 r22c_d2D44[] = {60, 0, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2D68[] = {60, 0, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2D8C[] = {120, 0, 0, 200, -22000, 0, 2, 180, 1};
static s32 r22c_d2DB0[] = {120, 0, 0, 200, -31000, 0, 2, 180, 1};
static s32 r22c_d2DD4[] = {300, 2, -2500, 200, -31000, 0, 3, 2500, 200, -31000, 50, 1};
static s32 r22c_d2E04[] = {300, 2, 2500, 200, -22000, 0, 3, -2500, 200, -22000, 50, 1};
static s32 r22c_d2E34[] = {330, 0, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2E58[] = {330, 0, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2E7C[] = {330, 0, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2EA0[] = {600, 2, -2500, 200, -26000, 0, 2, 270, 1};
static s32 r22c_d2EC4[] = {600, 2, 0, 200, -26000, 0, 2, 270, 1};
static s32 r22c_d2EE8[] = {600, 2, 2500, 200, -26000, 0, 2, 270, 1};
static s32 r22c_d2F0C[] = {630, 0, -1250, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2F30[] = {630, 0, 1250, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d2F54[] = {900, 0, -2500, 200, -36000, 0, 3, 2500, 200, -36000, 200, 3, 2500, 200, -18000, 200, 3, -2500, 200, -18000, 200, 3, -2500, 200, -36000, 200, 1};
static s32 r22c_d2FC0[] = {900, 0, 2500, 200, -18000, 0, 3, -2500, 200, -18000, 200, 3, -2500, 200, -36000, 200, 3, 2500, 200, -36000, 200, 3, 2500, 200, -18000, 200, 1};
static s32 r22c_d302C[] = {1500, 254};
static s32 r22c_d3034[] = {3000, 2, -2500, 200, -36000, 0, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -18000, 100, 3, 2500, 200, -18000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 1};
static s32 r22c_d30F0[] = {3030, 1, -2500, 200, -36000, 0, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -18000, 100, 3, 2500, 200, -18000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 1};
static s32 r22c_d31AC[] = {3060, 2, -2500, 200, -36000, 0, 3, 2500, 200, -36000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -18000, 100, 3, 2500, 200, -18000, 100, 3, 2500, 200, -26000, 100, 3, -2500, 200, -26000, 100, 3, -2500, 200, -36000, 100, 1};
static s32 r22c_d3268[] = {3300, 2, 2500, 200, -26000, 0, 2, 90, 1};
static s32 r22c_d328C[] = {3360, 0, 2500, 200, -31000, 0, 2, 90, 1};
static s32 r22c_d32B0[] = {3450, 2, -2500, 200, -22000, 0, 2, 90, 1};
static s32 r22c_d32D4[] = {3510, 1, -2500, 200, -26000, 0, 2, 90, 1};
static s32 r22c_d32F8[] = {3600, 2, 0, 200, -26000, 0, 2, 90, 1};
static s32 r22c_d331C[] = {3660, 0, -2500, 200, -26000, 0, 2, 90, 1};
static s32 r22c_d3340[] = {3660, 0, 0, 200, -31000, 0, 2, 90, 1};
static s32 r22c_d3364[] = {3660, 0, 2500, 200, -26000, 0, 2, 90, 1};
static s32 r22c_d3388[] = {3660, 0, 0, 200, -22000, 0, 2, 90, 1};
static s32 r22c_d33AC[] = {3900, 1, 0, 200, -26000, 0, 2, 90, 1};
static s32 r22c_d33D0[] = {3900, 0, 0, 200, -26000, 0, 2, 90, 1};
static s32 r22c_d33F4[] = {3960, 2, -2500, 200, -26000, 0, 2, 150, 1};
static s32 r22c_d3418[] = {3960, 2, 2500, 200, -26000, 0, 2, 150, 1};
static s32 r22c_d343C[] = {3960, 2, -2500, 200, -22000, 0, 2, 150, 1};
static s32 r22c_d3460[] = {3960, 2, 0, 200, -22000, 0, 2, 150, 1};
static s32 r22c_d3484[] = {3960, 2, 2500, 200, -22000, 0, 2, 150, 1};
static s32 r22c_d34A8[] = {3960, 2, -2500, 200, -31000, 0, 2, 150, 1};
static s32 r22c_d34CC[] = {3960, 2, 0, 200, -31000, 0, 2, 150, 1};
static s32 r22c_d34F0[] = {3960, 2, 2500, 200, -31000, 0, 2, 150, 1};
static s32 r22c_d3514[] = {4500, 0, -2500, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d3538[] = {4500, 1, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d355C[] = {4530, 2, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d3580[] = {4530, 2, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d35A4[] = {4530, 2, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d35C8[] = {4530, 2, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d35EC[] = {4530, 2, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d3610[] = {4530, 2, 0, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d3634[] = {4530, 2, 2500, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d3658[] = {4800, 1, -2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d367C[] = {4800, 0, 2500, 200, -36000, 0, 2, 240, 1};
static s32 r22c_d36A0[] = {4830, 2, 0, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d36C4[] = {4830, 2, 2500, 200, -26000, 0, 2, 240, 1};
static s32 r22c_d36E8[] = {4830, 2, -2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d370C[] = {4830, 2, 0, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d3730[] = {4830, 2, 2500, 200, -22000, 0, 2, 240, 1};
static s32 r22c_d3754[] = {4830, 2, -2500, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d3778[] = {4830, 2, 0, 200, -31000, 0, 2, 240, 1};
static s32 r22c_d379C[] = {4830, 2, 2500, 200, -31000, 0, 2, 240, 1};
static s32* r22c_d37C0[] = {(s32*) 63, (s32*) 6000, r22c_d2C90, r22c_d2CB4, r22c_d2CD8, r22c_d2CFC, r22c_d2D20, r22c_d2D44, r22c_d2D68, r22c_d2D8C, r22c_d2DB0, r22c_d2DD4, r22c_d2E04, r22c_d2E34, r22c_d2E58, r22c_d2E7C, r22c_d2EA0, r22c_d2EC4, r22c_d2EE8, r22c_d2F0C, r22c_d2F30, r22c_d2F54, r22c_d2FC0, r22c_d302C, r22c_d3034, r22c_d30F0, r22c_d31AC, r22c_d3268, r22c_d328C, r22c_d32B0, r22c_d32D4, r22c_d32F8, r22c_d331C, r22c_d3340, r22c_d3364, r22c_d3388, r22c_d33AC, r22c_d33D0, r22c_d33F4, r22c_d3418, r22c_d343C, r22c_d3460, r22c_d3484, r22c_d34A8, r22c_d34CC, r22c_d34F0, r22c_d3514, r22c_d3538, r22c_d355C, r22c_d3580, r22c_d35A4, r22c_d35C8, r22c_d35EC, r22c_d3610, r22c_d3634, r22c_d3658, r22c_d367C, r22c_d36A0, r22c_d36C4, r22c_d36E8, r22c_d370C, r22c_d3730, r22c_d3754, r22c_d3778, r22c_d379C};

static const char* r22c_levelName[5] = {"-", "A", "B", "C", "D"};

void r22c_exitDoor();
cObj* getCap(int a, int b, int c, int d, int e);
void checkBottleCap();
void getBottleCap();
int weaponSelect(int sel);
void itemSave();
void getBonus();
void gameEnd();
static void r22c_startShootingGame();
static void shootInit();
static void shootReady();
static void shootMain();
static void shootResult();
static void shootEnd();
int isWepmanAlive();
static void r22cSetWepMan();
static void r22cGateCtrl();
static void r22c_checkShootingScore();
static void r22c_checkExitDoor();
static void r22c_AshleyCtrl();
void ScoreInit();
void ScoreSet(int pt, Vec* pos);

void R22cInit()
{
#line 1978 "D:/Bio4/Prog/r22c.cpp"
    r22c_work = (R22cWork*) MEM_CALLOC(sizeof(R22cWork), 1, 0xd);
    r22c_work->result.read();
    ScoreInit();
    EmReadSearch(0x3E, 0, 0);
    switch (pG->room_id_prev) {
    case 0x204:
    default:
        r22c_work->level = 1;
        break;
    case 0x211:
        r22c_work->level = 2;
        break;
    case 0x220:
        r22c_work->level = 3;
        break;
    case 0x305:
    case 0x31D:
        r22c_work->level = 4;
        break;
    }
    switch (pG->x4F9F) {
    case 1:
        r22c_work->level = 2;
        break;
    case 2:
        r22c_work->level = 3;
        break;
    }
    if (Joy[0].on & 0x40) {
        r22c_work->level = 4;
    }
    SceExec(0x12, (TaskFunc) r22cSetWepMan, 0, 0, 2, 0);
    SceExec(0x12, (TaskFunc) r22cGateCtrl, 0, 0, 2, 0);
    SceAtDataSet_exec(0, 0x12, 0, (TaskFunc) r22c_startShootingGame, 0, 1);
    SceAtDataSet_exec(7, 0x12, 0, (TaskFunc) r22c_checkShootingScore, 0, 1);
    SceAtDataSet_exec(2, 0x12, 0, (TaskFunc) r22c_checkExitDoor, 0, 1);
    SceAtSetActColor(2, 1);
    if (getRoomEtcDoor(0, &r22c_work->door[0], 1) && getRoomEtcDoor(1, &r22c_work->door[1], 1)) {
        ((cEmDoor*) r22c_work->door[0])->setDoor((cEmDoor*) r22c_work->door[1]);
        ((cEmDoor*) r22c_work->door[0])->setCloseLock(0);
        ((cEmDoor*) r22c_work->door[1])->setCloseLock(0);
    }
    SmdGetObjPtr(0)->be_flag &= ~2;
    SmdGetObjPtr(1)->be_flag &= ~2;
    SmdGetObjPtr(2)->be_flag &= ~2;
    SmdGetObjPtr(3)->be_flag &= ~2;
    SceExec(0x12, (TaskFunc) r22c_AshleyCtrl, 0, 0, 2, 0);
    LightMgr.onKind(1);
    LightMgr.offKind(2);
    {
        int i;

        for (i = 0; i < 24; i++) {
            r22c_work->itemNum[i] = ItemMgr.num((u16) (i + 0xDC));
        }
    }
}

void R22cMain()
{
    eprintf(0x130, 0x1A4, 0, 0, "LEVEL:%s-%d", r22c_levelName[r22c_work->level], r22c_work->state);
}

static void r22c_BirdsFly()
{
    EstSet(0, -1, 0, 0, 1, 3, 0, 0x3F, 0, 0);
    SndCall(6, 0xE, 0, 0, 0, 0);
    SceSleep(480);
    SndCall(6, 0xF, 0, 0, 0, 0);
}

static void r22c_BeeFly()
{
    EstSet(0, -1, 0, 0, 1, 1, 0, 0x3F, 0, 0);
    SndCall(6, 0x10, 0, 0, 0, 0);
    SceSleep(1200);
    SndCall(6, 0x11, 0, 0, 0, 0);
}

static void r22c_FireWorks()
{
    Vec pos = {1000.0f, 3000.0f, -25200.0f};

    ScoreSet(300, &pos);
    EstSet(0, -1, 0, 0, 1, 5, 0, 0x3F, 0, 0);
    SndCall(6, 0x12, 0, 0, 0, 0);
    SceSleep(1200);
    SndCall(6, 0x13, 0, 0, 0, 0);
}

static void r22c_ShootingStar()
{
    Vec pos;

    EstSet(0, -1, 0, 0, 1, 4, 0, 0x3F, 0, 0);
    SndCall(6, 0x14, 0, 0, 0, 0);
    SceSleep(90);
    SndCall(6, 0x15, 0, 0, 0, 0);
    SceSleep(90);
    cObj* o = ObjMgr.create(2);
    pos.x = -3951.0f;
    pos.y = 9000.0f;
    pos.z = -25337.0f;
    o->setPos(&pos);
    o->modelInit(ROOM_ARC_PTR(pG->pRoomArc, 0x1F), ROOM_ARC_PTR(pG->pRoomArc, 0x20));
    EstSet((int) o, -1, 0, 0, 1, 6, 0, 0, (u32) o, 0);
    SceSleep(10);
    pos.x = -3951.0f;
    pos.y = 3000.0f;
    pos.z = -25337.0f;
    o->setPos(&pos);
    SceSleep(1);
    pos.x = -2999.0f;
    pos.y = 2000.0f;
    pos.z = -22657.0f;
    o->setPos(&pos);
    SceSleep(1);
    pos.x = -2055.0f;
    pos.y = 1000.0f;
    pos.z = -19877.0f;
    o->setPos(&pos);
    SceSleep(1);
    pos.x = -1113.0f;
    pos.y = 400.0f;
    pos.z = -17100.0f;
    o->setPos(&pos);
    EffectEspgenDelete(0, 0x3F, (int) o);
    EstSet(0, -1, &o->pos, 0, 1, 7, 0, 0, 0, 0);
    QuakeExec(0, 0, 5, 22.0f, 2);
    SndCall(6, 0x16, 0, 0, 0, 0);
    PlWepHitCheck2(0, &o->pos, &o->pos, 0x13, 0, 6000.0f);
    SceSleep(1);
    ObjMgr.destroy(o);
}

// Task: Ashley waits by the range.
static void r22c_AshleyCtrl()
{
    Vec v;

    SceSleep(1);
    if (pSUB) {
        SubCharCtrl(7, 1);
        cEm* sub = pSUB;
        v.x = -1600.0f;
        v.y = 0.0f;
        v.z = -270.0f;
        sub->setPos(&v);
        v.y = 3.0f;
        v.z = 0.0f;
        v.x = 0.0f;
        sub->setAng(&v);
        pSUB->atari.setPriority(1);
    }
}

// Task: the exit door.
static void r22c_checkExitDoor()
{
    if ((int) pG->flags_174 < 0) {
        SceAtSetEnable(2, 0);
        r22c_exitDoor();
        if (isWepmanAlive() == 0) {
            SceAtSetEnable(1, 0);
        }
        SceAtSetEnable(2, 1);
    } else {
        switch (pG->room_id_prev) {
        case 0x204:
        default:
            SceAtExecute(3);
            break;
        case 0x211:
            SceAtExecute(4);
            break;
        case 0x220:
            SceAtExecute(5);
            break;
        case 0x305:
            SceAtExecute(6);
            break;
        case 0x31D:
            SceAtExecute(8);
            break;
        }
    }
}

// Task: talking to the range keeper.
static void r22c_talkWepMan()
{
    SceAtSetEnable(1, 0);
    SndCall(8, 9, &r22c_work->wepMan->pos, r22c_work->wepMan->id, 0, 0);
    if ((int) pG->flags_174 < 0) {
        weaponSelect(0);
    } else {
        SceMesSet(6, 0, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
        switch (SceMesGetSelection()) {
        case 1:
            SndCall(0, 4, 0, 0, 0, 0);
            gameEnd();
            break;
        case 2:
            SndCall(0, 4, 0, 0, 0, 0);
            weaponSelect(0);
            break;
        case 3:
            SndCall(0, 5, 0, 0, 0, 0);
            break;
        }
    }
    SceAtSetEnable(1, 1);
}

void r22c_exitDoor()
{
    SceMesSet(0xD, 0, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
    switch (SceMesGetSelection()) {
    case 1:
        SndCall(0, 4, 0, 0, 0, 0);
        gameEnd();
        break;
    case 2:
        SndCall(0, 4, 0, 0, 0, 0);
        break;
    }
}

// The first bottle cap of the five the player does not own yet, starting at a random one.
cObj* getCap(int a, int b, int c, int d, int e)
{
    int tbl[5];
    u8 r;
    int i;

    tbl[0] = a;
    tbl[1] = b;
    tbl[2] = c;
    tbl[3] = d;
    tbl[4] = e;
    r = Rnd() % 5;
    for (i = 0; i < 5; i++) {
        if (ItemMgr.num((u16) (tbl[(r + i) % 5] + 0xDB)) == 0) {
            return (cObj*) tbl[(r + i) % 5];
        }
    }
    return 0;
}

void checkBottleCap()
{
    int cap = 0;

    switch (r22c_work->level) {
    case 1:
    default:
        if (r22c_work->state == 2 && r22c_work->score > 3999) {
            cap = 0xF0;
        } else if (r22c_work->score > 2999) {
            cap = (int) getCap(1, 2, 3, 4, 5);
        }
        break;
    case 2:
        if (r22c_work->state == 2 && r22c_work->score > 3999) {
            cap = 0xF1;
        } else if (r22c_work->score > 2999) {
            cap = (int) getCap(6, 7, 8, 9, 0xA);
        }
        break;
    case 3:
        if (r22c_work->state == 2 && r22c_work->hits > 0x18) {
            cap = 0xF2;
        } else if (r22c_work->score > 2999) {
            cap = (int) getCap(0xB, 0xC, 0xD, 0xE, 0xF);
        }
        break;
    case 4:
        if (r22c_work->state == 2 && r22c_work->hits > 0x18) {
            cap = 0xF3;
        } else if (r22c_work->score > 2999) {
            cap = (int) getCap(0x10, 0x11, 0x12, 0x13, 0x14);
        }
        break;
    }
    r22c_work->capId = -1;
    if (cap) {
        r22c_work->cap[cap - 1]++;
        r22c_work->capId = cap + 0xDB;
        PlCapNum[cap - 1]++;
    }
}

void getBottleCap()
{
    int i;
    int total = 0;

    if ((Joy[0].on & 0x640) == 0x640) {
        for (i = 0; i < 24; i++) {
            r22c_work->cap[i] = 1;
        }
    }
    for (i = 0; i < 24; i++) {
        if (r22c_work->cap[i]) {
            ItemMgr.get((u16) (i + 0xDC), *(u16*) ((u8*) &r22c_work->cap[i] + 2));
            r22c_work->itemNum[i] += r22c_work->cap[i];
            total += r22c_work->cap[i];
            r22c_work->cap[i] = 0;
        }
    }
    if (total > 0) {
        cMes.getWork()->setNumber(total, 0);
        SceMesSet(5, 0, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
        SndCall(0, 0x13, &r22c_work->wepMan->pos, 0, 0, 0);
        if (ItemMgr.num(0xA2) == 0) {
            ItemMgr.get(0xA2, 0);
        }
    }
    r22c_work->score = 0;
}

int weaponSelect(int sel)
{
    cPlayer* pl = pPL;
    int ask = sel == 0;
    u8 wep;

    if (ask) {
        SceMesSet(4, 0, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
        sel = SceMesGetSelection();
        if (sel != 3) {
            SndCall(0, 4, 0, 0, 0, 0);
        } else {
            SndCall(0, 5, 0, 0, 0, 0);
        }
    }
    wep = pG->wep_no;
    switch (sel) {
    case 1:
        if ((int) pG->flags_174 >= 0) {
            itemSave();
        }
        ItemMgr.clear();
        SubScreenWk.x2AE = SubScreenWk.x2AF = ItemMgr.set_range(0);
        pl->weaponRelease();
        if (wep != 7 && wep != 0xB && wep != 0x13) {
            wep = 7;
        }
        pl->weaponLoad(wep, 0);
        pl->weaponInit();
        ItemMgr.arm(ItemMgr.search(WeaponNo2WeaponId(wep, 0)));
        if (ask) {
            SubScreenOpen(1, 0);
        }
        break;
    case 2:
        if ((int) pG->flags_174 >= 0) {
            itemSave();
        }
        ItemMgr.clear();
        SubScreenWk.x2AE = SubScreenWk.x2AF = ItemMgr.set_range(1);
        pl->weaponRelease();
        if (wep != 2 && wep != 9 && wep != 0x13) {
            wep = 2;
        }
        pl->weaponLoad(wep, 0);
        pl->weaponInit();
        ItemMgr.arm(ItemMgr.search(WeaponNo2WeaponId(wep, 0)));
        if (ask) {
            SubScreenOpen(1, 0);
        }
        break;
    }
    r22c_work->wepSel = sel;
    return sel;
}

void itemSave()
{
    r22c_work->wepNo = pG->wep_no;
    r22c_work->wepType = pG->wep_type;
    pG->flags_174 |= 0x80000000;
    SceAtSetEnable(9, 0);
    ((cEmDoor*) r22c_work->door[0])->setNormal();
    ((cEmDoor*) r22c_work->door[1])->setNormal();
#line 2565 "D:/Bio4/Prog/r22c.cpp"
    r22c_work->itemSaveBuf = MEM_ALLOC(ItemMgr.saveDataSize(), 1, 0xd);
    if (r22c_work->itemSaveBuf == 0) {
        pLog->err(0, 0, "ITEM BACKUP FAILED.");
    } else {
        ItemMgr.save(r22c_work->itemSaveBuf);
        r22c_work->itemSel = (s8) SubScreenWk.x2AE;
    }
}

// --- not written yet: getBonus, gameEnd, r22c_checkGameLevel, r22c_checkGame, r22c_startShootingGame,
// shootInit, shootReady, shootMain, shootResult, shootEnd, countMark, R22cHitMark, funcUfo,
// deleteAllMark, r22cSetWepMan, scoreRegist, r22cGateCtrl, isWepmanAlive, setWepmanKilled,
// R22cHitEffect, ResultScreen::*, Score*, r22c_checkShootingScore. The stubs below only keep the
// data tables' relocation targets defined; they must be replaced by the real bodies.
static void r22c_startShootingGame() {}
static void r22cSetWepMan() {}
static void r22cGateCtrl() {}
static void r22c_checkShootingScore() {}
static void shootInit() {}
static void shootReady() {}
static void shootMain() {}
static void shootResult() {}
static void shootEnd() {}

static void (*r22c_shootFunc[5])() = {shootInit, shootReady, shootMain, shootResult, shootEnd};
static const char* r22c_startMsg = "START";
static s32 r22c_d38F0[] = {0, 3, 2200, 3500, -37500, 0, 3, -2200, 3500, -37500, 30, 1};
static int r22c_d3920 = 4;
// The language directory is patched into the path at run time.
static char r22c_fname[] = "SS/___/id22c.dat";

asm(".section .data\n\t.balign 8\n\t.text");
