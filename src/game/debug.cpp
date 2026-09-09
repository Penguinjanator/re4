// game/debug: debug overlays, process timing bars, debug/config.txt (D:/Bio4/Prog/debug.cpp).
#include "types.h"
#include "global.h"
#include "atari.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "gx.h"
#include "main.h"
#include "main_mem.h"
#include "joy.h"
#include "libgpu.h"
#include "eprintf.h"
#include "datactrl.h"
#include "db_log.h"
#include "db_cam.h"
#include "dvd.h"
#include "snd.h"
#include "debug.h"

void bio4_GXSetCopyClear(GXColor color, u32 z);   // game/gx_sub.cpp
void DbMenuExitAfterCheck();                        // game/db_menu.cpp

extern "C" {
u32 OSGetTick();
void OSReport(const char* fmt, ...);
unsigned int strlen(const char* s);
unsigned int strcspn(const char* s, const char* reject);
int strncmp(const char* a, const char* b, unsigned int n);
long strtol(const char* s, char** end, int base);
void* memset(void* dst, int c, unsigned int n);
}
extern u8 PlMode;       // game/player.cpp
extern u8 PlFormMode;   // game/player.cpp

#define HALT(cond)                                                           \
    if (cond) {                                                              \
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);                        \
        *(volatile u32*) 0x11111111 = 0;                                     \
    }

// 0x20-byte tile primitive (TILE padded to the array stride used by the debug bars)
struct DbgTile {
    u32 tag;          // 0x00
    u32 code;         // 0x04
    GpuColor c0;      // 0x08
    s16 x0, y0;       // 0x0C
    s16 w, h;         // 0x10
    s16 z0;           // 0x14
    u8 pad_16[0xA];
};

static u32 proc_tick[32];
const char* proc_name[32];
u32 zero_tick;
int proc_tick_idx;
int proc_tick_idx_bak;
int g_proc_cnt;

#define OS_BUS_CLOCK (((OSClock*) 0x80000000)->busClock)
struct OSClock {
    u8 pad_0[0xF8];
    u32 busClock;   // 0xF8
};

#line 66 "D:/Bio4/Prog/debug.cpp"

void DebugControl()
{
    if ((pG->flags_68 & 0x40000000) && pG->debug_mode) {
        MemCheckUsedHeap();
        processBarDisp();
        PrimitiveBuffDisp();
        DC.dispDebug();
    }
    if ((pG->flags_68 & 0x8000) && pG->debug_mode) {
        debugPadInfoDisp();
    }
    HALT(Joy[2].on == 0x1600);
    DbMenuExitAfterCheck();
}

void debugPadInfoDisp()
{
    u32 i;
    u32 on = Joy[0].on;
    u32 trg = Joy[0].trg;
    int x = 10;
    int x2 = 10;

    for (i = 0; i < 32; i++) {
        if (on & (0x80000000 >> i)) {
            eprintf2(6, 12, x2, 2, 4, 1, "I");
        } else {
            eprintf2(6, 12, x, 2, 0, 1, "_");
        }
        if (trg & (0x80000000 >> i)) {
            eprintf2(6, 12, i * 6 + 10, 2, 6, 1, "o");
        }
        x += 6;
        x2 += 6;
    }
}

static inline s16 tickX(u32 tick, f32 total)
{
    return (s16) ((f32) tick / total * 400.0f);
}

// Progressive (60Hz) screen: the 400-line bars are squashed to 300 lines below y = 56.
#define PROG_Y(y) ((s16) ((f32) (s16) (y) / 1.3333334f + 56.0f))
#define PROG_H(h) ((s16) ((f32) (s16) (h) / 1.3333334f))
// Reference read of pSys: the load stays below the preceding tile stores.
static inline SystemWork* SysRef(SystemWork*& p) { return p; }
// Ticks -> 1/100 frame units (bus clock / 4 = tick rate, 60 frames per second)
#define TICK_100F(t) ((f32) (t) * 60.0f / (f32) (OS_BUS_CLOCK >> 2) * 100.0f)

void processBarDisp()
{
    static DbgTile tile[6];
    static u8 cnt;
    static char xchr[5] = "-/l/";
    int vcnt = GetSystemVcnt();
    u32 frameTick = OS_BUS_CLOCK / 240 * vcnt;
    f32 total;
    DbgTile* t = tile;
    s16 x0;
    s16 x1;
    s16 x2;
    s16 x3;
    u32 i;

    total = (f32) frameTick;
    x0 = tickX(proc_tick[4], total);
    t->z0 = 0;
    t->code = 4;
    t->x0 = 6;
    t->y0 = 30;
    t->w = 5;
    t->c0.r = 0x80;
    t->c0.g = 0x80;
    t->c0.b = 0x20;
    t->c0.cd = 0xFF;
    t->h = x0;
    if (SysRef(pSys)->flags & 0x40000000) {
        t->y0 = PROG_Y(30);
        t->h = PROG_H(t->h);
    }
    AddPrim(&MainOt[1], (u32*) t);
    t++;

    x1 = tickX(proc_tick[1], total);
    t->code = 4;
    t->x0 = 6;
    t->y0 = x0 + 30;
    t->z0 = 0;
    t->w = 5;
    t->c0.r = 0x80;
    t->c0.g = 0x20;
    t->c0.b = 0x20;
    t->c0.cd = 0xFF;
    t->h = x1 - x0;
    if (SysRef(pSys)->flags & 0x40000000) {
        t->y0 = PROG_Y(t->y0);
        t->h = PROG_H(t->h);
    }
    AddPrim(&MainOt[1], (u32*) t);
    t++;

    x2 = tickX(proc_tick[2], total);
    t->y0 = x0 + 30;
    t->code = 4;
    t->x0 = 12;
    t->z0 = 0;
    t->w = 5;
    t->c0.r = 0x20;
    t->c0.g = 0x20;
    t->c0.b = 0x80;
    t->c0.cd = 0xFF;
    t->h = x2 - x0;
    if (SysRef(pSys)->flags & 0x40000000) {
        t->y0 = PROG_Y(t->y0);
        t->h = PROG_H(t->h);
    }
    AddPrim(&MainOt[1], (u32*) t);
    t++;

    x0 = x2;
    if (x1 > x2) {
        x0 = x1;
    }
    x3 = tickX(proc_tick[3], total);
    t->y0 = x0 + 30;
    t->c0.g = 0x80;
    t->code = 4;
    t->x0 = 6;
    t->z0 = 0;
    t->w = 5;
    t->c0.r = 0x20;
    t->c0.b = 0x20;
    t->c0.cd = 0xFF;
    t->h = x3 - x0;
    if (SysRef(pSys)->flags & 0x40000000) {
        t->y0 = PROG_Y(t->y0);
        t->h = PROG_H(t->h);
    }
    AddPrim(&MainOt[1], (u32*) t);
    t++;

    t->x0 = 6;
    t->code = 4;
    t->y0 = 30;
    t->z0 = 0;
    t->w = 5;
    t->h = 400;
    t->c0.r = 8;
    t->c0.g = 8;
    t->c0.b = 0x20;
    t->c0.cd = 0xFF;
    if (SysRef(pSys)->flags & 0x40000000) {
        t->y0 = 78;
        t->h = 300;
    }
    AddPrim(&MainOt[1], (u32*) t);
    t++;

    t->code = 4;
    t->x0 = 12;
    t->y0 = 30;
    t->z0 = 0;
    t->w = 5;
    t->h = 400;
    t->c0.g = 8;
    t->c0.b = 0x20;
    t->c0.cd = 0xFF;
    t->c0.r = 8;
    if (SysRef(pSys)->flags & 0x40000000) {
        t->y0 = 78;
        t->h = 300;
    }
    AddPrim(&MainOt[1], (u32*) t);

    if (proc_tick[2] > proc_tick[1]) {
        eprintf2(10, 16, 12, 400, 0, 1, "%4.0f", TICK_100F(proc_tick[3]));
        eprintf2(10, 16, 12, 416, 0, 1, "%4.0f", TICK_100F(proc_tick[1]));
    } else {
        eprintf2(10, 16, 12, 400, 0, 1, "%4.0f", TICK_100F(proc_tick[2]));
        eprintf2(10, 16, 12, 416, 0, 1, "%4.0f", TICK_100F(proc_tick[3]));
    }
    eprintf2(10, 16, 0, 16, 0, 13, "%4.0f", TICK_100F(proc_tick[3]));
    g_proc_cnt = (u32) TICK_100F(proc_tick[3]);
    eprintf2(10, 16, 42, 28, 0, 2, "1000/F");
    for (i = 5; i < proc_tick_idx_bak + 5; i++) {
        eprintf2(10, 16, 32, 50 + (i - 5) * 16, 0, 2, "%5.0f %s", TICK_100F(proc_tick[i] - proc_tick[i - 1]), proc_name[i]);
    }
    cnt = (cnt + 1) & 3;
    eprintf2(14, 14, 10, 18, 0, 0, "%c", xchr[cnt]);
}

void ProcessTickGet(int no, const char* name)
{
    if ((u32) no <= 4) {
        proc_tick[no] = OSGetTick() - zero_tick;
        proc_name[no] = name;
    } else {
        u32 t = OSGetTick();

        proc_name[proc_tick_idx + 5] = name;
        proc_tick[proc_tick_idx + 5] = t - zero_tick;
        proc_tick_idx++;
    }
}

void ProcessTickInit()
{
    zero_tick = OSGetTick();
    proc_tick_idx_bak = proc_tick_idx;
    proc_tick_idx = 0;
}

// The primitive buffer statistics live in the big GX block at pG + 0x184 in the original
struct PrimBuffView {
    u8 pad_0[0x4F10 - 0x184];
    s32 base;   // pG->prim_base
    f32 rate;   // pG->prim_rate
};

void PrimitiveBuffDisp()
{
    static DbgTile tile[6];
    DbgTile* t;
    int max = pG->prim_max;
    PrimBuffView* pb = (PrimBuffView*) &pG->gxStage;
    f32 rate;

    if (max == 0) {
        return;
    }
    rate = 1.0f - (f32) (int) (pG->prim_cnt + max * (pG->vtx_buf_no + 1) - pb->base) / (f32) max;
    t = tile;
    if (pb->rate < rate) {
        pb->rate = rate;
    }
    if (rate > 0.9f && !(pG->flags_64 & 0x00200000)) {
        pLog->warn(0, 0, "PrimitiveBuff :  work remain under 1/10");
    }

    {
        s16 width = 200;

        t->code = 4;
        t->x0 = (s16) (pb->rate * (f32) width + (f32) width);
        t->y0 = 24;
        t->w = 4;
        t->h = 4;
        t->c0.r = 0x80;
        t->c0.g = 0x14;
        t->c0.b = 0x14;
        t->c0.cd = 0xFF;
        t->z0 = 0;
        int z = 0;
        if (SysRef(pSys)->flags & 0x40000000) {
            t->y0 = 74;
            t->h = 3;
        }
        AddPrim(&MainOt[1], (u32*) t);
        t++;

        t->w = (s16) (rate * (f32) width);
        t->c0.b = 0x80;
        t->code = 4;
        t->x0 = width;
        t->y0 = 24;
        t->z0 = z;
        t->h = 4;
        t->c0.r = 0x14;
        t->c0.g = 0x14;
        t->c0.cd = 0xFF;
        if (SysRef(pSys)->flags & 0x40000000) {
            t->y0 = 74;
            t->h = 3;
        }
        AddPrim(&MainOt[1], (u32*) t);
        t++;

        t->y0 = 24;
        t->z0 = z;
        t->w = width;
        t->h = 4;
        t->c0.b = 0x14;
        t->c0.cd = 0xFF;
        t->code = 4;
        t->x0 = width;
        t->c0.r = 0x14;
        t->c0.g = 0x14;
        if (SysRef(pSys)->flags & 0x40000000) {
            t->y0 = 74;
            t->h = 3;
        }
        AddPrim(&MainOt[1], (u32*) t);
    }
}

#define CFG_ON(p) (strncmp(p, "ON", 2) == 0)
#define CFG_OFF3(p) (strncmp(p, "OFF", 3) == 0)

#line 817 "D:/Bio4/Prog/debug.cpp"
void ConfigSet()
{
    int size = 0;
    char* buf;
    char* end;
    char* p;
    int req;
    int ret;

    BitOn(pG->flags_68, 0x40000000);
    BitOn(pG->flags_68, 0x8000);
    BitOff(pG->flags_6C, 0x08000000);
    PlMode = 0;
    PlFormMode = 0;
    pG->x4F9F = 0;
    buf = (char*) MEM_CALLOC(0x1000, 1, 13);
    req = DvdReadN("debug/config.txt", buf, 0, 0, 0, 0x11, __FILE__, __LINE__);
    if (Dvd.ReadCheck(req, &size, 0, 0) < 0) {
        Mem_free(buf);
        return;
    }
    p = buf;
    end = buf + size;
    while (p < end) {
        p = space_skip(p);
        if (*p++ != '[') {
            continue;
        }
        ret = symbol_check(&p, "USER");
        if (ret) {
            int i = 0;
            while (*p != '\r') {
                pUser_name[i++] = *p;
                p++;
            }
        } else if ((ret = symbol_check(&p, "BRIGHTNESS")) != 0) {
            if (pRK->brightness == 0) {
                pRK->brightness = num_get(&p);
            }
        } else if ((ret = symbol_check(&p, "STAGE")) != 0) {
            pG->stage_no = num_get(&p);
        } else if ((ret = symbol_check(&p, "ROOM")) != 0) {
            pG->room_no = num_get(&p);
        } else if ((ret = symbol_check(&p, "JUMP_POINT")) != 0) {
            pG->x4F9F = num_get(&p);
        } else if ((ret = symbol_check(&p, "PRINT_PAGE")) != 0) {
            pG->debug_mode = num_get(&p);
            pG->debug_disp = -1;
        } else if ((ret = symbol_check(&p, "PLAYER")) != 0) {
            pG->x4FB8 = num_get(&p);
        } else if ((ret = symbol_check(&p, "BGM")) != 0) {
            if (CFG_OFF3(p)) {
                BitOn(pG->flags_68, 0x00100000);
            } else {
                BitOff(pG->flags_68, 0x00100000);
            }
        } else if ((ret = symbol_check(&p, "SE")) != 0) {
            if (CFG_OFF3(p)) {
                BitOn(pG->flags_68, 0x00080000);
            } else {
                BitOff(pG->flags_68, 0x00080000);
            }
        } else if ((ret = symbol_check(&p, "SCENARIO")) != 0) {
            if (CFG_OFF3(p)) {
                BitOn(pG->flags_68, 0x04000000);
            } else {
                BitOff(pG->flags_68, 0x04000000);
            }
        } else if ((ret = symbol_check(&p, "NO_ENEMY")) != 0) {
            if (CFG_OFF3(p)) {
                BitOff(pG->flags_68, 0x00200000);
            } else {
                BitOn(pG->flags_68, 0x00200000);
            }
        } else if ((ret = symbol_check(&p, "ENEMY_SET")) != 0) {
            if (CFG_OFF3(p)) {
                BitOn(pG->flags_68, 0x00200000);
            } else {
                BitOff(pG->flags_68, 0x00200000);
            }
        } else if ((ret = symbol_check(&p, "ETC_SET")) != 0) {
            if (CFG_OFF3(p)) {
                BitOn(pG->flags_6C, 0x800);
            } else {
                BitOff(pG->flags_6C, 0x800);
            }
        } else if ((ret = symbol_check(&p, "PROCESS_BAR")) != 0) {
            if (CFG_OFF3(p)) {
                BitOff(pG->flags_68, 0x40000000);
                BitOff(pG->flags_68, 0x8000);
            } else {
                BitOn(pG->flags_68, 0x40000000);
                BitOn(pG->flags_68, 0x8000);
            }
        } else if ((ret = symbol_check(&p, "VIBRATION")) != 0) {
            if (symbol_check(&p, "OFF")) {
                BitOff(pSys->flags, 0x08000000);
            } else {
                BitOn(pSys->flags, 0x08000000);
            }
        } else if ((ret = symbol_check(&p, "BG_BLACK")) != 0) {
            if (symbol_check(&p, "ON")) {
                GXColor c;
                c.r = c.g = c.b = c.a = 0;
                bio4_GXSetCopyClear(c, 0xFFFFFF);
            }
        } else if ((ret = symbol_check(&p, "DBG_CAM_KEY")) != 0) {
            if (symbol_check(&p, "DFLT")) {
                CamDbg.key_type = 0;
            } else if ((ret = symbol_check(&p, "SCR")) != 0) {
                CamDbg.key_type = 1;
            }
        } else if ((ret = symbol_check(&p, "DBG_ESP_DISP")) != 0) {
            if (symbol_check(&p, "ON")) {
                BitOn(pG->flags_6C, 0x8000);
            }
        } else if ((ret = symbol_check(&p, "GAME_MODE")) != 0) {
            if ((u32) ((u8) *p - '0') <= 9) {
                pG->x8354 = num_get(&p);
                if (pG->x8354 > 6) {
                    pG->x8354 = 6;
                }
            } else {
                if (symbol_check(&p, "VERY_EASY")) {
                    pG->x8354 = 1;
                }
                if (symbol_check(&p, "EASY")) {
                    pG->x8354 = 3;
                }
                if (symbol_check(&p, "NORMAL")) {
                    pG->x8354 = 5;
                }
                if (symbol_check(&p, "HARD")) {
                    pG->x8354 = 6;
                }
            }
        } else if ((ret = symbol_check(&p, "SOUND_MODE")) != 0) {
            int mode = 1;
            if ((u32) ((u8) *p - '0') <= 9) {
                mode = num_get(&p);
                if (mode < 0) {
                    mode = 0;
                } else if (mode > 2) {
                    mode = 2;
                }
            }
            if (symbol_check(&p, "MONO")) {
                mode = 0;
            }
            if (symbol_check(&p, "STEREO")) {
                mode = 1;
            }
            if (symbol_check(&p, "DPL2")) {
                mode = 2;
            }
            if (mode != pSys->sound_mode) {
                SndSetOutputMode(mode, 0);
            }
        } else if ((ret = symbol_check(&p, "WARNING_LOG")) != 0) {
            if (CFG_ON(p)) {
                BitOff(pG->flags_6C, 0x04000000);
            } else {
                BitOn(pG->flags_6C, 0x04000000);
            }
        } else if ((ret = symbol_check(&p, "PLAYER_MODE")) != 0) {
            PlMode = num_get(&p);
        } else if ((ret = symbol_check(&p, "SN_PC_READ")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_54, 0x00020000);
            } else {
                BitOff(pG->flags_54, 0x00020000);
            }
        } else if ((ret = symbol_check(&p, "SN_PC_READ_TOOL")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_54, 0x00010000);
            } else {
                BitOff(pG->flags_54, 0x00010000);
            }
        } else if ((ret = symbol_check(&p, "NO_DEATH")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_68, 0x00800000);
            } else {
                BitOff(pG->flags_68, 0x00800000);
            }
        } else if ((ret = symbol_check(&p, "OBJ_SERVER")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_6C, 0x01000000);
            } else {
                BitOff(pG->flags_6C, 0x01000000);
            }
        } else if ((ret = symbol_check(&p, "LANGUAGE")) != 0) {
            if (symbol_check(&p, "JPN")) {
                pSys->language = 0;
            }
            if (symbol_check(&p, "USA")) {
                pSys->language = 1;
            }
            if (symbol_check(&p, "ENG")) {
                pSys->language = 2;
            }
            if (symbol_check(&p, "GER")) {
                pSys->language = 3;
            }
            if (symbol_check(&p, "FRA")) {
                pSys->language = 4;
            }
            if (symbol_check(&p, "ESP")) {
                pSys->language = 5;
            }
            if (symbol_check(&p, "ITA")) {
                pSys->language = 6;
            }
            if (symbol_check(&p, "KOR")) {
                pSys->language = 7;
            }
        } else if ((ret = symbol_check(&p, "PUBLICITY_VER")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_54, 8);
            } else {
                BitOff(pG->flags_54, 8);
            }
        } else if ((ret = symbol_check(&p, "AIM_REVERSE")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pSys->flags, 0x80000000);
            } else {
                BitOff(pSys->flags, 0x80000000);
            }
        } else if ((ret = symbol_check(&p, "WIDE_MODE")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pSys->flags, 0x40000000);
            } else {
                BitOff(pSys->flags, 0x40000000);
            }
        } else if ((ret = symbol_check(&p, "AUTO_LOCK_ON")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pSys->flags, 0x20000000);
            } else {
                BitOff(pSys->flags, 0x20000000);
            }
        } else if ((ret = symbol_check(&p, "SINGLE_DISK")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_68, 0x02000000);
            } else {
                BitOff(pG->flags_68, 0x02000000);
            }
        } else if ((ret = symbol_check(&p, "BUGCHECK_MODE")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_68, 0x01000000);
            } else {
                BitOff(pG->flags_68, 0x01000000);
            }
        } else if ((ret = symbol_check(&p, "LIGHT_CHECK")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_68, 0x100);
            } else {
                BitOff(pG->flags_68, 0x100);
            }
        } else if ((ret = symbol_check(&p, "TITLE_CHECK")) != 0) {
            if (CFG_ON(p)) {
                BitOn(pG->flags_64, 0x00080000);
            } else {
                BitOff(pG->flags_64, 0x00080000);
            }
        }
    }
    Mem_free(buf);
    PlMode = 3;
}

int symbol_check(char** p, const char* sym)
{
    int len = strlen(sym);

    if (len == strcspn(*p, "] \t\n\r")) {
        if (strncmp(*p, sym, len) == 0) {
            *p += len;
            if (**p == ']') {
                (*p)++;
            }
            *p = space_skip(*p);
            return 1;
        }
    }
    return 0;
}

char* space_skip(char* p)
{
    do {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }
    } while (comment_check(&p));
    return p;
}

int comment_check(char** pp)
{
    char* p = *pp;

    if (strncmp(p, "[[", 2) == 0) {
        p += 2;
        while (strncmp(p, "]]", 2) != 0) {
            p++;
        }
        p += 2;
    } else if (strncmp(p, "/*", 2) == 0) {
        p += 2;
        while (strncmp(p, "*/", 2) != 0) {
            p++;
        }
        p += 2;
    } else if (strncmp(p, "//", 2) == 0) {
        while (*p != '\n') {
            p++;
        }
        p++;
    } else {
        return 0;
    }
    *pp = p;
    return -1;
}

int num_get(char** p)
{
    *p = space_skip(*p);
    if (strncmp(*p, "0x", 2) == 0) {
        return strtol(*p, p, 16);
    }
    return strtol(*p, p, 10);
}
