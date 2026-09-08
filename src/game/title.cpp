// game/title: title screen task — logos, demo movies, main menu, omake (Ada / Mercenaries) select and
// the debug start menu (D:/Bio4/Prog/title.cpp).
#include "types.h"
#include "global.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "card.h"
#include "atari.h"
#include "sofdec.h"
#include "id_sys.h"
#include "fade.h"
#include "dvd.h"
#include "main_mem.h"
#include "main_sub.h"
#include "main.h"
#include "pad.h"
#include "joy.h"
#include "scheduler.h"
#include "snd.h"
#include "mes.h"
#include "view.h"
#include "game.h"
#include "item.h"
#include "read.h"
#include "option.h"
#include "mercenaries.h"
#include "room_jmp.h"
#include "stage.h"
#include "sce.h"
#include "pl_sub.h"
#include "eprintf.h"
#include "title.h"

extern "C" void OSReport(const char* fmt, ...);

#define ID_TITLE 0x28
#define ID_MENU 0x29
#define ID_OMAKE 0x2A
#define ID_OMAKE_BG 0x2B
#define ID_OPTION 0x2C

#define KEY_A 0x80000000
#define KEY_B 0x40000000
#define KEY_START 0x1000
#define KEY_UP 0x01000000
#define KEY_DOWN 0x02000000
#define KEY_RIGHT 0x04000000
#define KEY_LEFT 0x08000000

// Reference setters: a store through a scalar reference is not a struct-member MEM, so the
// following pG load stays below it (see global.h FSet).
static inline void ISet(int& d, int v)
{
    d = v;
}
static inline void CSet(s8& d, s8 v)
{
    d = v;
}
static inline void BSet(u8& d, u8 v)
{
    d = v;
}

// Fade colours: word constants passed by address (see sscrn.cpp).
union FadeColor {
    GXColor c;
    u32 w;
};

// Read-error report of the original: the condition never holds, only the strings survive.
#define READ_ERROR(msg)                                           \
    if (0) {                                                      \
        OSReport(msg);                                            \
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);            \
    }

void titleInit(TitleWork* w);
void titleWait(TitleWork* w);
void titleNintendo(TitleWork* w);
void titleWarning(TitleWork* w);
void titleLogo(TitleWork* w);
void titleMain(TitleWork* w);
void titleSub(TitleWork* w);
void titleExit(TitleWork* w);
void titleSet(TitleWork* w, int time);
void titleMenuInit(TitleWork* w);
int titleMenuSelect(TitleWork* w);
void titleLevelInit(TitleWork* w);
int titleLevelSelect(TitleWork* w);
void titleLoop(TitleWork* w);
void id_color_copy(int src, int dst, u8 type);
void stageSelectInit(TitleWork* w);
int stageSelect(TitleWork* w);
void titleDebugMenu(TitleWork* w);

int TTL_CANCEL_WARNING = 45;
int TTL_CANCEL_CAPCOM = 120;
int TTL_CANCEL_CRI = 245;
static int TTL_CANCEL_DOLBY = 345;

void Title_task()
{
    static void (*titleFuncTbl[8])(TitleWork*) = {
        titleInit, titleWait, titleNintendo, titleWarning, titleLogo, titleMain, titleSub, titleExit,
    };
    TitleWork* w;

    pSaveData = GameSave.alloc();
    ItemMgr.init();
    CoreDataRead();
    OptionDataRead();
#line 114 "D:/Bio4/Prog/title.cpp"
    w = (TitleWork*) MEM_CALLOC(sizeof(TitleWork), 1, 13);
    for (;;) {
        titleFuncTbl[w->mode](w);
        TaskSleep(1);
    }
}

void titleInit(TitleWork* w)
{
    ScreenReSize(640, 448);
#line 147 "D:/Bio4/Prog/title.cpp"
    int req = DvdReadN("SS/cmn/title.snd", 0, 0, 0, 0, 0x8000, __FILE__, __LINE__);
    READ_ERROR("title.snd read error!!");
    // PERM_BEGIN_INIT
    ISet(w->req, req);
    w->mode = 1;
    w->xC = 0;
    w->step = 0;
    ISet(w->scroll, 0);
    FSet(w->speed, 1.5f);
    // PERM_END_INIT
    pG->flags_5014 |= 0x8000;
    IdAllocBuffer();
}

void titleSet(TitleWork* w, int time)
{
    IdSys.kill(0xFF, ID_TITLE);
    if (!(pSys->x4 & 0x40000000)) {
        IdSys.set(TITLE_ARC_PTR(w->pDat, 6), 0xFF, ID_TITLE, 0x13, 6, 0);
    } else {
        IdSys.set(TITLE_ARC_PTR(w->pDat, 7), 0xFF, ID_TITLE, 0x13, 6, 0);
    }
    IdSys.setTimeS(IdSys.unitPtr(0, ID_TITLE), (s16) time);
}

void titleWait(TitleWork* w)
{
    static char title_dat[] = "SS/___/title.dat";

    switch (w->step) {
    case 0:
        if (CardCheckDone() == 1) {
            int stat = Dvd.ReadCheck(w->req, 0, 0, 0);
            if (stat == 1) {
                FadeKill(0);
                systemVISetBlack(0);
                w->step = 1;
            }
        }
        break;
    case 1:
        setLangExt3(title_dat + 3);
#line 210 "D:/Bio4/Prog/title.cpp"
        w->req = DvdReadN(title_dat, 0, 0, 0, 0, 4, __FILE__, __LINE__);
        READ_ERROR("%s read error!!");
        w->step = 2;
        break;
    case 2: {
        int stat = Dvd.ReadCheck(w->req, 0, 0, (void**) &w->pDat);
        if (stat == 1) {
            pG->prim_max = 0x20000;
            primInit();
            IdTexRoomInit();
            IdSys.roomInit();
            IdTexDataLoad(TITLE_ARC_PTR(w->pDat, 4), 7);
            // PERM_BEGIN_WAIT
            CSet(w->mode, 2);
            ISet(w->sndFlag, 1);
            w->step = 0;
            ISet(w->cnt, 0);
            ISet(w->x48, 0);
            // PERM_END_WAIT
            if (pRK->x17 != 0) {
                w->mode = 5;
                w->cnt = 585;
                titleSet(w, 585);
                if ((s32) pG->flags_54 < 0 || (pG->flags_54 & 0x40000000)) {
                    w->saveStep = w->step;
                    w->saveSub = w->sub;
                    w->saveX3 = w->x3;
                    w->saveCnt = w->cnt;
                    w->mode = 6;
                    w->step = 0;
                }
            }
        }
        {
            Camera* cam = &pG->Cam;
            C_MTXPerspective(cam->projMat, cam->param.fovy, 4.0f / 3.0f, ZNEAR, ZFAR);
            cam->dist = PSVECDistance(&cam->param.pos, &cam->param.at);
            C_MTXLookAt(cam->viewMat, &cam->param.pos, &cam->up, &cam->param.at);
        }
        break;
    }
    }
}

void titleNintendo(TitleWork* w)
{
    static int wait_cnt = 0;
    FadeColor c0;
    FadeColor c1;

    if (PadCheckStatus(&Joy[1]) == 1) {
        FadeKill(0);
        w->mode = 5;
        w->step = 0;
        w->cnt = 585;
        titleSet(w, 585);
        return;
    }
    switch (w->step) {
    case 0:
        IdSys.set(TITLE_ARC_PTR(w->pDat, 0xB), 0xFF, ID_TITLE, 0x13, 6, 0);
        c0.w = 0x000000FF;
        c1.w = 0x00000000;
        FadeSet(0x80000000, &c0.c, &c1.c, 15, 0, 0);
        wait_cnt = 0;
        w->step++;
        break;
    case 1:
        if (wait_cnt++ > 60) {
            c0.w = 0x00000000;
            c1.w = 0x000000FF;
            FadeSet(0, &c0.c, &c1.c, 15, 0, 0);
            w->step++;
        }
        break;
    case 2:
        if ((Fade[0].flags & 1) == 0) {
            FadeKill(0);
            w->step = 0;
            w->mode = 3;
            titleSet(w, w->cnt);
        }
        break;
    }
}

void titleWarning(TitleWork* w)
{
    IdUnit* u = IdSys.unitPtr(0, ID_TITLE);
    FadeColor c0;
    FadeColor c1;

    w->cnt++;
    if (PadCheckStatus(&Joy[1]) == 1) {
        w->cnt = 585;
        w->mode = 5;
        w->step = 0;
        IdSys.setTimeS(u, (s16) w->cnt);
        return;
    }
    switch (w->step) {
    case 0:
        if (w->cnt > 105) {
            w->step = 0;
            w->mode = 4;
        } else if (w->cnt > TTL_CANCEL_WARNING) {
            if (Key.trg & KEY_START) {
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 15, 0, 0);
                w->step = 1;
            }
        }
        break;
    case 1:
        if ((Fade[0].flags & 1) == 0) {
            FadeKill(0);
            w->cnt = 105;
            w->step = 0;
            w->mode = 4;
            IdSys.setTimeS(u, (s16) w->cnt);
        }
        break;
    }
}

void titleLogo(TitleWork* w)
{
    IdUnit* u = IdSys.unitPtr(0, ID_TITLE);
    static u32 LOGO_CALL_FRAME = TTL_CANCEL_DOLBY;
    FadeColor c0;
    FadeColor c1;

    w->cnt++;
    switch (w->step) {
    case 0:
        if (w->cnt > 230) {
            w->step = 2;
        } else if (w->cnt > TTL_CANCEL_CAPCOM) {
            if (Key.trg & KEY_START) {
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 15, 0, 0);
                w->step = 1;
            }
        }
        break;
    case 1:
        if ((Fade[0].flags & 1) == 0) {
            FadeKill(0);
            w->cnt = 230;
            w->step = 2;
            IdSys.setTimeS(u, (s16) w->cnt);
        }
        break;
    case 2:
        if (w->cnt > 330) {
            w->step = 4;
        } else if (w->cnt > TTL_CANCEL_CRI) {
            if (Key.trg & KEY_START) {
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 15, 0, 0);
                w->step = 3;
            }
        }
        break;
    case 3:
        if ((Fade[0].flags & 1) == 0) {
            FadeKill(0);
            w->cnt = 330;
            w->step = 4;
            IdSys.setTimeS(u, (s16) w->cnt);
        }
        break;
    case 4:
        if (w->cnt > 585) {
            w->step = 6;
        } else if (w->cnt > TTL_CANCEL_DOLBY) {
            if (Key.trg & KEY_START) {
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 15, 0, 0);
                w->step = 5;
            }
        }
        break;
    case 5:
        if ((Fade[0].flags & 1) == 0) {
            FadeKill(0);
            w->cnt = 585;
            w->step = 6;
            IdSys.setTimeS(u, (s16) w->cnt);
        }
        break;
    case 6:
        w->mode = 5;
        w->step = 0;
        break;
    }
    if ((u32) w->cnt > LOGO_CALL_FRAME && w->sndFlag == 1) {
        SndCall(6, 4, 0, 0, 0, 0);
        w->sndFlag = 0;
    }
}

void titleMenuInit(TitleWork* w)
{
    IdSys.kill(0xFF, ID_MENU);
    if (pSys->x4 & 0x40000000) {
        IdSys.set(TITLE_ARC_PTR(w->pDat, 9), 0xFF, ID_MENU, 0x13, 5, 0);
        w->menuNum = 5;
        w->menu[0] = IdSys.unitPtr(1, ID_MENU);
        w->menu[1] = IdSys.unitPtr(7, ID_MENU);
        w->menu[2] = IdSys.unitPtr(9, ID_MENU);
        w->menu[3] = IdSys.unitPtr(3, ID_MENU);
        w->menu[4] = IdSys.unitPtr(5, ID_MENU);
        w->cursor = 3;
    } else {
        IdSys.set(TITLE_ARC_PTR(w->pDat, 8), 0xFF, ID_MENU, 0x13, 5, 0);
        w->menuNum = 3;
        w->menu[0] = IdSys.unitPtr(1, ID_MENU);
        w->menu[1] = IdSys.unitPtr(5, ID_MENU);
        w->menu[2] = IdSys.unitPtr(3, ID_MENU);
        w->cursor = 1;
    }
    w->scroll = 0;
    w->speed = 1.5f;
}

// Cursor movement shared by the main menu and the level select.
#define TITLE_MENU_MOVE(w)                                                                 \
    {                                                                                      \
        int old = (w)->cursor;                                                             \
        int i;                                                                             \
        if (Key.trg & KEY_UP) {                                                            \
            (w)->cursor = old - 1;                                                         \
        }                                                                                  \
        if (Key.trg & KEY_DOWN) {                                                          \
            (w)->cursor++;                                                                 \
        }                                                                                  \
        (w)->cursor = (w)->cursor < 0 ? 0 : ((w)->cursor > (w)->menuNum - 1 ? (w)->menuNum - 1 : (w)->cursor); \
        if (old != (w)->cursor) {                                                          \
            SndCall(0, 10, 0, 0, 0, 0);                                                    \
        }                                                                                  \
        for (i = 0; i < (w)->menuNum; i++) {                                               \
            if (i == (w)->cursor) {                                                        \
                (w)->menu[i]->flags |= 8;                                                  \
            } else {                                                                       \
                (w)->menu[i]->flags &= ~8;                                                 \
            }                                                                              \
        }                                                                                  \
        if ((w)->menuNum > 1 && (Key.trg & (KEY_UP | KEY_DOWN))) {                        \
            for (i = 0; i < (w)->menuNum; i++) {                                           \
                IdUnit* u = (w)->menu[i];                                                  \
                u->timer[3] = 0;                                                           \
                u->timer[1] = 0;                                                           \
                u->timer[2] = 0;                                                           \
                u->timer[0] = 0;                                                           \
            }                                                                              \
        }                                                                                  \
    }

int titleMenuSelect(TitleWork* w)
{
    int ret = 0;

    if (Key.trg & KEY_A) {
        if (pSys->x4 & 0x40000000) {
            switch (w->cursor) {
            case 0:
                ret = 6;
                break;
            case 1:
                ret = 2;
                break;
            case 2:
                ret = 3;
                break;
            case 3:
                ret = 4;
                break;
            case 4:
                ret = 5;
                break;
            }
        } else {
            switch (w->cursor) {
            case 0:
                ret = 1;
                break;
            case 1:
                ret = 4;
                break;
            case 2:
                ret = 5;
                break;
            }
        }
        return ret;
    }
    TITLE_MENU_MOVE(w);
    return 0;
}

void titleLevelInit(TitleWork* w)
{
    IdSys.kill(0xFF, ID_MENU);
    IdSys.set(TITLE_ARC_PTR(w->pDat, 0xA), 0xFF, ID_MENU, 0x13, 5, 0);
    w->menu[0] = IdSys.unitPtr(1, ID_MENU);
    w->menu[1] = IdSys.unitPtr(3, ID_MENU);
    w->menu[2] = IdSys.unitPtr(5, ID_MENU);
    if (pSys->language == 1) {
        IdSys.unitPtr(5, ID_MENU)->flags &= ~8;
        IdSys.unitPtr(6, ID_MENU)->flags &= ~8;
        w->menuNum = 2;
    } else {
        w->menuNum = 3;
    }
    w->cursor = 1;
}

int titleLevelSelect(TitleWork* w)
{
    if (Key.trg & KEY_A) {
        if (pSys->language == 0) {
            switch (w->cursor) {
            case 0:
                pG->x8354 = 6;
                break;
            case 1:
            default:
                pG->x8354 = 5;
                break;
            case 2:
                pG->x8354 = 3;
                break;
            }
        } else if (pSys->language == 1) {
            if (w->cursor == 0) {
                pG->x8354 = 6;
            } else {
                pG->x8354 = 5;
            }
        } else {
            switch (w->cursor) {
            case 0:
                pG->x8354 = 6;
                break;
            case 1:
            default:
                pG->x8354 = 5;
                break;
            case 2:
                pG->x8354 = 3;
                break;
            }
        }
        return 1;
    }
    TITLE_MENU_MOVE(w);
    return 0;
}

void titleMain(TitleWork* w)
{
    static int demo_loop_cnt = 0;

    pRK->x17 = 1;
    if (!(pG->flags_54 & 0x100) && (pSys->x4 & 0x40000000)) {
        titleLoop(w);
    }
    switch (w->step) {
    case 0:
        titleMenuInit(w);
        w->step = 1;
        demo_loop_cnt = 600;
        break;
    case 1: {
        int sel = titleMenuSelect(w);
        BitOff(pG->flags_54, 0x80000000);
        BitOff(pG->flags_54, 0x40000000);
        switch (sel) {
        case 1:
            Snd.room_ok = 1;
            w->sndId = SndCall(6, pSys->language == 0 ? 0 : 2, 0, 0, 0, 0);
            VibSetData((VibDataTbl*) ((u8*) pG->pArc + pG->pArc->ofs_1C), 0x10, 1);
            {
                FadeColor c0;
                FadeColor c1;
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 90, 0, 0);
            }
            w->step = 3;
            break;
        case 2:
        case 3:
            if (sel == 2) {
                pG->flags_54 |= 0x80000000;
            } else if (sel == 3) {
                pG->flags_54 |= 0x40000000;
            }
            w->saveStep = w->step;
            w->saveSub = w->sub;
            w->saveX3 = w->x3;
            w->saveCnt = w->cnt;
            w->mode = 6;
            w->omkChar = 0;
            w->step = 0;
            SndCall(0, 4, 0, 0, 0, 0);
            break;
        case 4:
            w->step = 7;
            SndCall(0, 4, 0, 0, 0, 0);
            break;
        case 5:
            IdTexRelease(7);
            IdSys.kill(0xFF, ID_TITLE);
            IdSys.kill(0xFF, ID_MENU);
            MesData.ptr[2] = (u8*) pG->pArc + pG->pArc->ofs_28;
            OptScrn.init(1);
            IdTexDataLoad((u8*) pG->pArc + pG->pArc->ofs_74, 4);
            IdTexDataLoad(TITLE_ARC_PTR(w->pDat, 0xC), 6);
            IdSys.set(TITLE_ARC_PTR(w->pDat, 0xD), 0xFF, ID_OPTION, 0x13, 5, 0);
            w->saveCnt = w->cnt;
            w->step = 4;
            SndCall(0, 0x33, 0, 0, 0, 0);
            break;
        case 6:
            titleLevelInit(w);
            w->step = 2;
            SndCall(0, 4, 0, 0, 0, 0);
            break;
        }
        if (w->step == 1 && !(pG->flags_54 & 8)) {
            if (Key.trg & (KEY_UP | KEY_DOWN)) {
                demo_loop_cnt = 600;
            }
            if (demo_loop_cnt > 0) {
                demo_loop_cnt--;
                if (demo_loop_cnt == 0) {
                    FadeColor c0;
                    FadeColor c1;
                    c0.w = 0x00000000;
                    c1.w = 0x000000FF;
                    FadeSet(0, &c0.c, &c1.c, 15, 0, 0);
                    w->sub = 0;
                    w->step = 6;
                }
            } else {
                demo_loop_cnt = 600;
            }
        }
        break;
    }
    case 2:
        if (Key.trg & KEY_B) {
            titleMenuInit(w);
            w->step = 1;
        } else if (titleLevelSelect(w) != 0) {
            Snd.room_ok = 1;
            w->sndId = SndCall(6, pSys->language == 0 ? 0 : 2, 0, 0, 0, 0);
            VibSetData((VibDataTbl*) ((u8*) pG->pArc + pG->pArc->ofs_1C), 0x10, 1);
            {
                FadeColor c0;
                FadeColor c1;
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 90, 0, 0);
            }
            w->step = 3;
        }
        break;
    case 3:
        w->mode = 7;
        break;
    case 4:
        if (OptScrn.move() == 1) {
            IdTexRelease(4);
            IdTexRelease(6);
            IdSys.kill(0xFF, ID_OPTION);
            OptScrn.quit();
            IdTexDataLoad(TITLE_ARC_PTR(w->pDat, 4), 7);
            w->mode = 5;
            w->step = 0;
            w->cursor = 2;
            w->cnt = w->saveCnt;
            titleSet(w, w->saveCnt);
        }
        break;
    case 5:
        switch (w->sub) {
        case 0:
            if ((Fade[0].flags & 1) == 0) {
                w->sub++;
            }
            break;
        case 1:
            systemVISetBlack(1);
            FadeKill(0);
            ScreenReSize(512, 448);
            Sofdec.Initialize("movie/e3_jpn.sfd", 0);
            w->sub++;
            break;
        case 2:
            if (!Sofdec.isPlay()) {
                systemVISetBlack(1);
                ScreenReSize(640, 448);
                systemVISetBlack(0);
                {
                    FadeColor c0;
                    FadeColor c1;
                    c0.w = 0x000000FF;
                    c1.w = 0x00000000;
                    FadeSet(0x80000000, &c0.c, &c1.c, 15, 0, 0);
                }
                w->sub++;
            }
            break;
        case 3:
            if ((Fade[0].flags & 1) == 0) {
                w->sub = 0;
                w->step = 1;
            }
            break;
        }
        break;
    case 6:
        switch (w->sub) {
        case 0:
            if ((Fade[0].flags & 1) == 0) {
                w->sub++;
                w->demoNo = w->demoNo == 0;
            }
            break;
        case 1:
            systemVISetBlack(1);
            FadeKill(0);
            ScreenReSize(512, 448);
            if (w->demoNo) {
                Sofdec.Initialize("movie/demo0.sfd", 0);
            } else {
                Sofdec.Initialize("movie/demo1.sfd", 0);
            }
            w->sub++;
            break;
        case 2:
            if (!Sofdec.isPlay()) {
                systemVISetBlack(1);
                ScreenReSize(640, 448);
                systemVISetBlack(0);
                {
                    FadeColor c0;
                    FadeColor c1;
                    c0.w = 0x000000FF;
                    c1.w = 0x00000000;
                    FadeSet(0x80000000, &c0.c, &c1.c, 15, 0, 0);
                }
                w->sub++;
            }
            break;
        case 3:
            if ((Fade[0].flags & 1) == 0) {
                w->step = 1;
            }
            break;
        }
        break;
    case 7:
        if (CardLoad() == 1) {
            w->step = 3;
            pG->flags_54 |= 0x100;
        } else {
            pG->flags_54 |= 0x04000000;
            return;
        }
        break;
    case 8:
        switch (w->sub) {
        case 0: {
            IdUnit* u = IdSys.unitPtr(0, ID_TITLE);
            if (w->cnt <= 584) {
                w->cnt = 645;
            }
            IdSys.setTimeS(u, (s16) w->cnt);
            w->x48 = 1;
            w->sub++;
        }
        case 1:
            if (Joy[0].trg & 0x1100) {
                w->mode = 7;
                if ((s32) pG->flags_54 < 0 || (pG->flags_54 & 0x40000000)) {
                    w->saveSub = w->sub;
                    w->saveStep = w->step;
                    w->saveX3 = w->x3;
                    w->saveCnt = w->cnt;
                    w->mode = 6;
                    w->step = 0;
                } else {
                    FadeColor c0;
                    FadeColor c1;
                    Snd.room_ok = 1;
                    c0.w = 0x00000000;
                    c1.w = 0x000000FF;
                    FadeSet(0, &c0.c, &c1.c, 0, 0, 0);
                }
            }
            titleDebugMenu(w);
            break;
        }
        break;
    }
    if (w->cnt > 54000) {
        w->cnt = 54000;
    }
}

void titleLoop(TitleWork* w)
{
    static f32 width = 900.0f;
    static f32 zoom_in_limit = 170.0f;
    IdUnit* u;

    IdSys.unitPtr(1, ID_TITLE)->scr.x = width * 0.0f;
    IdSys.unitPtr(2, ID_TITLE)->scr.x = width * 1.0f;
    IdSys.unitPtr(3, ID_TITLE)->scr.x = width * -1.0f;
    IdSys.unitPtr(4, ID_TITLE)->scr.x = width * -2.0f;
    IdSys.unitPtr(5, ID_TITLE)->scr.x = width * 2.0f;
    u = IdSys.unitPtr(6, ID_TITLE);
    if (w->scroll == 0) {
        u->scr.x -= w->speed;
        if (Key.on & (KEY_RIGHT | KEY_LEFT)) {
            w->scroll = 1;
        }
    } else {
        if ((f32) Key.sx != 0.0f) {
            f32 spd = (f32) Key.sx / 59.0f * 3.0f;
            u->scr.x -= spd;
            if (__builtin_fabsf((f32) Key.sx) > 3.0f) {
                w->speed = spd;
            }
        } else {
            u->scr.x -= w->speed;
        }
        if (Key.on & 0x00400000) {
            u->scr.z -= 5.0f;
        }
        if (Key.on & 0x00800000) {
            u->scr.z += 5.0f;
        }
        u->scr.z = u->scr.z < 0.0f ? 0.0f : (u->scr.z > zoom_in_limit ? zoom_in_limit : u->scr.z);
    }
    if (u->scr.x > width * 2.0f) {
        u->scr.x -= width * 3.0f;
    }
    if (u->scr.x < -width) {
        u->scr.x += width * 3.0f;
    }
}

void id_color_copy(int src, int dst, u8 type)
{
    IdUnit* s = IdSys.unitPtr(src, type);
    IdUnit* d = IdSys.unitPtr(dst, type);

    d->col0[0] = s->col0[0];
    d->col0[1] = s->col0[1];
    d->col0[2] = s->col0[2];
    d->col0[3] = s->col0[3];
    d->curve[2] = s->curve[2];
}

void titleSub(TitleWork* w)
{
    static char omake_dat[] = "SS/___/omk_tX.dat";
    static void* omk_addr = 0;
    static u32 snd_id;
    static int title_snd_wait = 7;
    int charBit[5] = {4, 4, 6, 5, 7};

    switch (w->step) {
    case 0:
        if ((s32) pG->flags_54 < 0) {
            omake_dat[12] = '0';
        } else if (pG->flags_54 & 0x40000000) {
            omake_dat[12] = '1';
        }
        setLangExt3(omake_dat + 3);
        omk_addr = 0;
#line 1157 "D:/Bio4/Prog/title.cpp"
        w->req = DvdReadN(omake_dat, 0, 0, 0, 0, 4, __FILE__, __LINE__);
        if (w->req == 0) {
            break;
        }
        {
            FadeColor c0;
            FadeColor c1;
            c0.w = 0x00000000;
            c1.w = 0x000000FF;
            FadeSet(0, &c0.c, &c1.c, 5, 0, 0);
        }
        w->step++;
        if ((s32) pG->flags_54 < 0) {
            snd_id = SndStrReq(0, 60, 0x80000003, 0, 0, 0.0f);
        } else if (pG->flags_54 & 0x40000000) {
            snd_id = SndStrReq(0, 55, 0x80000003, 0, 0, 0.0f);
        }
        break;
    case 1:
        if (Dvd.ReadCheck(w->req, &w->omkSize, 0, (void**) &w->pOmk) != 0) {
            omk_addr = w->pOmk;
            w->step++;
        }
        break;
    case 2:
        if ((Fade[0].flags & 1) == 0) {
            w->step++;
        }
        break;
    case 3: {
        TitleArc* omk;
        {
            FadeColor c0;
            FadeColor c1;
            c0.w = 0x000000FF;
            c1.w = 0x00000000;
            FadeSet(0x80000000, &c0.c, &c1.c, 5, 0, 0);
        }
        omk = (TitleArc*) omk_addr;
        IdTexDataLoad(TITLE_ARC_PTR(omk, 4), 6);
        IdSys.kill(0xFF, ID_TITLE);
        IdSys.kill(0xFF, ID_MENU);
        omk = (TitleArc*) omk_addr;
        IdSys.set(TITLE_ARC_PTR(omk, 5), 0xFF, ID_OMAKE_BG, 0x13, 5, 0);
        omk = (TitleArc*) omk_addr;
        IdSys.set(TITLE_ARC_PTR(omk, 6), 0xFF, ID_OMAKE, 0x13, 4, 0);
        if (pG->flags_54 & 0x40000000) {
            if (!(pSys->x4 & 0x08000000)) {
                IdSys.unitPtr(4, ID_OMAKE_BG)->flags &= ~8;
            }
            if (!(pSys->x4 & 0x02000000)) {
                IdSys.unitPtr(1, ID_OMAKE_BG)->flags &= ~8;
            }
            if (!(pSys->x4 & 0x04000000)) {
                IdSys.unitPtr(2, ID_OMAKE_BG)->flags &= ~8;
            }
            if (!(pSys->x4 & 0x01000000)) {
                IdSys.unitPtr(3, ID_OMAKE_BG)->flags &= ~8;
            }
        }
        w->omkCursor = 0;
        w->step++;
        break;
    }
    case 4:
        if (Key.trg & KEY_B) {
            FadeColor c0;
            FadeColor c1;
            c0.w = 0x00000000;
            c1.w = 0x000000FF;
            FadeSet(0, &c0.c, &c1.c, 5, 0, 0);
            w->step++;
            SndCall(0, 5, 0, 0, 0, 0);
            SndStrReq(snd_id, 4, 200, 0);
        } else if (Key.trg & KEY_A) {
            if (w->omkCursor != 0) {
                FadeColor c0;
                FadeColor c1;
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 5, 0, 0);
                w->step++;
                SndCall(0, 5, 0, 0, 0, 0);
                SndStrReq(snd_id, 4, 200, 0);
            } else if ((s32) pG->flags_54 < 0) {
                FadeColor c0;
                FadeColor c1;
                w->mode = 7;
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 90, 0, 0);
                pG->x4FB8 = 2;
                pG->costume = 1;
                w->sndId = SndCall(6, 6, 0, 0, 0, 0);
                SndStrReq(snd_id, 4, 200, 0);
            } else if (pG->flags_54 & 0x40000000) {
                FadeColor c0;
                FadeColor c1;
                if (!(pSys->x4 & 0x00400000)) {
                    int i;
                    pSys->x4 |= 0x00400000;
                    for (i = 0; i < 4; i++) {
                        *(u32*) ((u8*) pSys + 0x10 + i * 4) = 0;
                    }
                    for (i = 0; i < 2; i++) {
                        *(u32*) ((u8*) pSys + 0x20 + i * 4) = 0;
                    }
                }
                if (DebugTrg(1)) {
                    BitOff(pSys->x4, 0x08000000);
                    BitOff(pSys->x4, 0x02000000);
                    BitOff(pSys->x4, 0x04000000);
                    BitOff(pSys->x4, 0x01000000);
                }
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 5, 0, 0);
                w->omkChar = 0;
                w->step = 6;
                SndCall(0, 60, 0, 0, 0, 0);
            }
        } else if (Key.trg & (KEY_UP | KEY_DOWN)) {
            s8 old = w->omkCursor;
            if (Key.trg & KEY_UP) {
                w->omkCursor = 0;
            } else {
                w->omkCursor = 1;
            }
            if (old != w->omkCursor) {
                SndCall(0, 10, 0, 0, 0, 0);
            }
        }
        if (w->omkCursor == 0) {
            IdSys.unitPtr(0, ID_OMAKE)->flags |= 8;
            IdSys.unitPtr(1, ID_OMAKE)->flags &= ~8;
        } else {
            IdSys.unitPtr(0, ID_OMAKE)->flags &= ~8;
            IdSys.unitPtr(1, ID_OMAKE)->flags |= 8;
        }
        break;
    case 5:
        if ((Fade[0].flags & 1) == 0) {
            IdTexRelease(6);
            IdSys.kill(0xFF, ID_OMAKE_BG);
            IdSys.kill(0xFF, ID_OMAKE);
            Mem_free(w->pOmk);
            {
                FadeColor c0;
                FadeColor c1;
                c0.w = 0x000000FF;
                c1.w = 0x00000000;
                FadeSet(0x80000000, &c0.c, &c1.c, 5, 0, 0);
            }
            w->mode = 5;
            w->step = w->saveStep;
            w->sub = w->saveSub;
            w->x3 = w->saveX3;
            w->cnt = w->saveCnt;
            titleSet(w, w->saveCnt);
            titleMenuInit(w);
        }
        break;
    case 6:
        if ((Fade[0].flags & 1) == 0) {
            IdTexRelease(6);
            IdSys.kill(0xFF, ID_OMAKE_BG);
            IdSys.kill(0xFF, ID_OMAKE);
            w->step++;
        }
        break;
    case 7: {
        TitleArc* omk;
        int i;
        {
            FadeColor c0;
            FadeColor c1;
            c0.w = 0x000000FF;
            c1.w = 0x00000000;
            FadeSet(0x80000000, &c0.c, &c1.c, 5, 0, 0);
        }
        omk = (TitleArc*) omk_addr;
        IdTexDataLoad(TITLE_ARC_PTR(omk, 4), 6);
        omk = (TitleArc*) omk_addr;
        IdSys.set(TITLE_ARC_PTR(omk, 7), 0xFF, ID_OMAKE, 0x13, 4, 0);
        for (i = 0; i < 5; i++) {
            IdUnit* u = IdSys.unitPtr(i, ID_OMAKE);
            u->no = i;
            u->flags_7F |= 2;
        }
        w->step++;
        break;
    }
    case 8: {
        if (!(pSys->x4 & 0x08000000)) {
            id_color_copy(0xFD, 1, ID_OMAKE);
        } else {
            id_color_copy(0xFC, 1, ID_OMAKE);
        }
        if (!(pSys->x4 & 0x02000000)) {
            id_color_copy(0xFD, 2, ID_OMAKE);
        } else {
            id_color_copy(0xFC, 2, ID_OMAKE);
        }
        if (!(pSys->x4 & 0x04000000)) {
            id_color_copy(0xFD, 3, ID_OMAKE);
        } else {
            id_color_copy(0xFC, 3, ID_OMAKE);
        }
        if (!(pSys->x4 & 0x01000000)) {
            id_color_copy(0xFD, 4, ID_OMAKE);
        } else {
            id_color_copy(0xFC, 4, ID_OMAKE);
        }
        if ((Key.on & 0x20000) && w->omkChar != 0) {
            int bit = charBit[w->omkChar];
            u32* tbl = &pSys->x4;
            BitOn(tbl[bit >> 5], 0x80000000 >> (bit & 0x1F));
        }
        if (Key.trg & KEY_B) {
            FadeColor c0;
            FadeColor c1;
            c0.w = 0x00000000;
            c1.w = 0x000000FF;
            FadeSet(0, &c0.c, &c1.c, 5, 0, 0);
            w->step++;
            SndCall(0, 5, 0, 0, 0, 0);
        } else if (Key.trg & KEY_A) {
            if (w->omkChar != 0) {
                int bit = charBit[w->omkChar];
                u32* tbl = &pSys->x4;
                if (!(tbl[bit >> 5] & (0x80000000 >> (bit & 0x1F)))) {
                    SndCall(0, 5, 0, 0, 0, 0);
                    break;
                }
            }
            switch (w->omkChar) {
            case 0:
                pG->x4FB8 = 0;
                pG->costume = 1;
                break;
            case 1:
                pG->x4FB8 = 2;
                pG->costume = 0;
                break;
            case 2:
                pG->x4FB8 = 4;
                pG->costume = 0;
                break;
            case 3:
                pG->x4FB8 = 3;
                pG->costume = 0;
                break;
            case 4:
                pG->x4FB8 = 5;
                pG->costume = 0;
                break;
            }
            w->step = 10;
            w->omkStage = 0;
            {
                FadeColor c0;
                FadeColor c1;
                c0.w = 0x00000000;
                c1.w = 0x000000FF;
                FadeSet(0, &c0.c, &c1.c, 15, 0, 0);
            }
            SndCall(0, 0x3F, 0, 0, 0, 0);
        } else if (Key.rep & (KEY_RIGHT | KEY_LEFT)) {
            s8 old = w->omkChar;
            if (Key.rep & KEY_LEFT) {
                w->omkChar = old - 1;
            } else {
                w->omkChar = old + 1;
            }
            w->omkChar = w->omkChar < 0 ? 4 : (w->omkChar > 4 ? 0 : w->omkChar);
            if (old != w->omkChar) {
                SndCall(0, 0x3E, 0, 0, 0, 0);
            }
        }
        {
            s8 sel = w->omkChar;
            IdUnit* a = IdSys.unitPtr(sel, ID_OMAKE);
            IdUnit* b = IdSys.unitPtr(0xFE, ID_OMAKE);
            IdUnit* u;
            b->scr = a->scr;
            u = IdSys.unitPtr(5, ID_OMAKE);
            u->no = sel;
            u->flags_7F |= 2;
            u = IdSys.unitPtr(6, ID_OMAKE);
            u->no = sel;
            u->flags_7F |= 2;
        }
        if (w->omkChar != 0) {
            int bit = charBit[w->omkChar];
            u32* tbl = &pSys->x4;
            if (!(tbl[bit >> 5] & (0x80000000 >> (bit & 0x1F)))) {
                id_color_copy(0xFD, 5, ID_OMAKE);
                IdSys.unitPtr(6, ID_OMAKE)->no = 5;
                break;
            }
        }
        id_color_copy(0xFC, 5, ID_OMAKE);
        break;
    }
    case 9:
        if ((Fade[0].flags & 1) == 0) {
            IdTexRelease(6);
            IdSys.kill(0xFF, ID_OMAKE_BG);
            IdSys.kill(0xFF, ID_OMAKE);
            w->step = 3;
        }
        break;
    case 10:
        if ((Fade[0].flags & 1) == 0) {
            IdSys.kill(0xFF, ID_OMAKE_BG);
            IdSys.kill(0xFF, ID_OMAKE);
            w->step = 11;
        }
        break;
    case 11: {
        FadeColor c0;
        FadeColor c1;
        c0.w = 0x000000FF;
        c1.w = 0x00000000;
        FadeSet(0x80000000, &c0.c, &c1.c, 15, 0, 0);
        w->step = 12;
        stageSelectInit(w);
        break;
    }
    case 12:
        if (Key.trg & KEY_B) {
            w->step = 6;
            SndCall(0, 5, 0, 0, 0, 0);
        } else if (stageSelect(w) != 0) {
            w->sub = 0;
            w->step = 13;
        }
        break;
    case 13:
        w->sub++;
        if (w->sub == title_snd_wait) {
            w->sndId = SndCall(6, 8, 0, 0, 0, 0);
        }
        if (w->sub > 30) {
            FadeColor c0;
            FadeColor c1;
            w->mode = 7;
            c0.w = 0x00000000;
            c1.w = 0x000000FF;
            FadeSet(0, &c0.c, &c1.c, 60, 0, 0);
            SndStrReq(snd_id, 4, 200, 0);
        }
        break;
    }
}

void stageSelectInit(TitleWork* w)
{
    TitleArc* omk = w->pOmk;

    IdSys.kill(0xFF, ID_OMAKE);
    IdSys.set(TITLE_ARC_PTR(omk, 8), 0xFF, ID_OMAKE, 0x13, 4, 0);
    w->omkStage = 0;
}

int stageSelect(TitleWork* w)
{
    int ret = 0;
    MercSaveWork save;
    int i;

    if (Key.trg & KEY_A) {
        ret = 1;
    } else if (Key.rep & (KEY_UP | KEY_DOWN | KEY_RIGHT | KEY_LEFT)) {
        s8 old = w->omkStage;
        if (Key.rep & KEY_UP) {
            if (old > 1) {
                w->omkStage = old - 2;
            }
        } else if (Key.rep & KEY_DOWN) {
            if (old <= 1) {
                w->omkStage = old + 2;
            }
        } else if (Key.rep & KEY_LEFT) {
            if (old & 1) {
                w->omkStage = old - 1;
            }
        } else if (Key.rep & KEY_RIGHT) {
            if (!(old & 1)) {
                w->omkStage = old + 1;
            }
        }
        w->omkStage = w->omkStage < 0 ? 3 : (w->omkStage > 3 ? 0 : w->omkStage);
        if (old != w->omkStage) {
            SndCall(0, 0x3E, 0, 0, 0, 0);
        }
    }
    for (i = 0; i < 4; i++) {
        IdUnit* u = IdSys.unitPtr(i + 0x11, ID_OMAKE);
        u->no = i;
        u->flags_7F |= 2;
    }
    for (i = 0; i < 4; i++) {
        IdUnit* u = IdSys.unitPtr(i + 0x21, ID_OMAKE);
        if (w->omkStage == i) {
            u->flags &= ~8;
        } else {
            u->flags |= 8;
        }
    }
    IdSys.unitPtr(0x31, ID_OMAKE)->flags &= ~8;
    IdSys.unitPtr(0x32, ID_OMAKE)->flags &= ~8;
    IdSys.unitPtr(0x33, ID_OMAKE)->flags &= ~8;
    IdSys.unitPtr(0x34, ID_OMAKE)->flags &= ~8;
    if (ret == 1) {
        IdUnit* u = IdSys.unitPtr(w->omkStage + 0x31, ID_OMAKE);
        u->flags |= 8;
        IdSys.setTime(u, 0);
    }
    {
        IdUnit* u;
        u = IdSys.unitPtr(1, ID_OMAKE);
        if (pSys->x4 & 0x08000000) {
            u->flags &= ~8;
        } else {
            u->flags |= 8;
        }
        u->no = 0;
        u->flags_7F |= 2;
        u = IdSys.unitPtr(2, ID_OMAKE);
        if (pSys->x4 & 0x02000000) {
            u->flags &= ~8;
        } else {
            u->flags |= 8;
        }
        u->no = 1;
        u->flags_7F |= 2;
        u = IdSys.unitPtr(3, ID_OMAKE);
        if (pSys->x4 & 0x04000000) {
            u->flags &= ~8;
        } else {
            u->flags |= 8;
        }
        u->no = 2;
        u->flags_7F |= 2;
        u = IdSys.unitPtr(4, ID_OMAKE);
        if (pSys->x4 & 0x01000000) {
            u->flags &= ~8;
        } else {
            u->flags |= 8;
        }
        u->no = 3;
        u->flags_7F |= 2;
    }
    MercSysGetSaveWork(&save);
    {
        int mode = 0;
        if (pG->x4FB8 == 2) {
            mode = 1;
        }
        if (pG->x4FB8 == 4) {
            mode = 2;
        }
        if (pG->x4FB8 == 3) {
            mode = 3;
        }
        if (pG->x4FB8 == 5) {
            mode = 4;
        }
        for (i = 0; i < 4; i++) {
            int rank = save.rank[mode][i];
            IdUnit* u = IdSys.unitPtr(i * 16 + 0x40, ID_OMAKE);
            int j;
            if (rank == 0) {
                u->flags &= ~8;
            } else {
                u->flags |= 8;
            }
            for (j = 0; j < 5; j++) {
                u = IdSys.unitPtr(i * 16 + 0x41 + j, ID_OMAKE);
                if (j < rank) {
                    u->flags |= 8;
                } else {
                    u->flags &= ~8;
                }
            }
            if (save.stage[i].score == 0) {
                for (j = 0; j < 8; j++) {
                    IdSys.unitPtr(i * 16 + 0x80 + j, ID_OMAKE)->flags &= ~8;
                }
            } else {
                u = IdSys.unitPtr(i * 16 + 0x88, ID_OMAKE);
                u->flags |= 8;
                u->flags_7F |= 2;
                IdSetNum(&IdSys, i * 16 + 0x81, ID_OMAKE, save.stage[i].score, 9999999, 7, 0);
            }
        }
    }
    return ret;
}

static cRoomJmp* pRj;

void titleExit(TitleWork* w)
{
    if ((s32) pG->flags_54 >= 0 && !(pG->flags_54 & 0x40000000)) {
        pG->x4FB8 = 0;
        pG->costume2 = 0;
    }
    if (!(pG->flags_54 & 0x100) && (s32) pG->flags_54 >= 0 && !(pG->flags_54 & 0x40000000)) {
        pRj = new cRoomJmp(roomInfoAddr);
        w->dbgStage = pG->stage_no;
        w->dbgRoom[w->dbgStage] = pRj->getRoomIdx(pG->stage_no, pG->room_no);
        w->dbgPoint = pG->x4F9F;
        w->dbgEmList = checkEmListNo(pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage])->room_id);
        w->dbgX = 340;
        w->dbgY = 60;
        w->loadNo = 1;
        w->sndId = 0;
        while (!(Joy[0].trg & 0x1100)) {
            titleDebugMenu(w);
            TaskSleep(1);
        }
        G_ROOM_ID = pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage] + w->dbgPoint)->room_id;
        pG->x4F9F = w->dbgPoint;
        pG->emlist_no = w->dbgEmList;
        if (pG->emlist_no > 3) {
            pG->flags_51C0 |= 0x10000000;
        } else if (pG->emlist_no > 2) {
            pG->flags_51C0 |= 0x40000;
        }
        switch (w->dbgCursor) {
        case 1:
            if (CardLoad() == 1) {
                pG->flags_54 |= 0x100;
            } else {
                pG->flags_54 |= 0x04000000;
            }
            return;
        case 2:
            G_ROOM_ID = 0x120;
            pG->x4F9F = 0;
            pG->x4FB8 = 0;
            break;
        case 0x12:
            pSys->x4 |= 0x40000000;
            pG->flags_54 |= 0x04000000;
            return;
        }
        if ((s32) pG->flags_54 >= 0 && !(pG->flags_54 & 0x40000000)) {
            PlSetCostume();
        }
        if (!(pG->x4FB8_32 & 0xFF0000FF)) {
            u32 room = pG->room_id32 & 0xFFFF0000;
            if (room == 0x01200000 || room == 0x01000000 || room == 0x01010000 || room == 0x01030000 || room == 0x01060000) {
                pG->costume = 0;
            } else {
                pG->costume = 1;
                pG->flags_51BC |= 0x00200000;
            }
        }
        if (pG->stage_no == 2) {
            BitOn(pG->flags_6C, 0x00800000);
            if (pG->room_id != 0x200) {
                BitOn(pG->flags_51C0, 0x00800000);
            }
        } else if (pG->stage_no == 3) {
            BitOn(pG->flags_6C, 0x40000);
            BitOn(pG->flags_51C0, 0x10000);
            if (pG->room_id == 0x333) {
                BitOn(pG->flags_6C, 0x20000);
            }
        }
        switch (pG->stage_no) {
        case 0:
            break;
        case 1:
            BitOn(pG->flags_51BC, 4);
            break;
        case 2:
            BitOn(pG->flags_51BC, 4);
            BitOn(pG->flags_51BC, 2);
            if ((pG->room_id32 & 0xFFFF00FF) != 0x02000000) {
                BitOn(pG->flags_51C0, 0x00800000);
            }
            break;
        }
        pRj->getRoomInfo(pG->stage_no, pG->x4F9F + pRj->getRoomIdx(pG->stage_no, pG->room_no))->setNextPos();
        delete pRj;
        if (pG->x4FB8 == 6) {
            pG->x4FB8 = 0;
            BitOn(pG->flags_5018, 0x04000000);
        }
    }
    BitOff(pG->flags_6C, 0x00200000);
    if (!(pG->flags_54 & 0x100)) {
        pG->flags_54 |= 0x2000;
    }
    pG->room_id_prev = 0xFFF;
    pG->emlist_no = -1;
    if ((s32) pG->flags_54 < 0) {
        G_ROOM_ID = 0x405;
        pG->x4F9F = 0;
        pG->x4F9E = 0;
        FSet(pG->sub_pos.x, 28450.0f);
        FSet(pG->sub_pos.y, -16798.0f);
        FSet(pG->sub_pos.z, -40000.0f);
        FSet(pG->sub_angle, -2.49f);
    } else if (pG->flags_54 & 0x40000000) {
        switch (w->omkStage) {
        case 0:
            G_ROOM_ID = 0x400;
            pG->x4F9F = 0;
            pG->x4F9E = 0;
            FSet(pG->sub_pos.x, -12400.0f);
            FSet(pG->sub_pos.y, 2576.0f);
            FSet(pG->sub_pos.z, 31080.0f);
            FSet(pG->sub_angle, 2.4235f);
            break;
        case 1:
            G_ROOM_ID = 0x402;
            pG->x4F9F = 0;
            pG->x4F9E = 0;
            FSet(pG->sub_pos.x, 21035.0f);
            FSet(pG->sub_pos.y, 3065.0f);
            FSet(pG->sub_pos.z, -26370.0f);
            FSet(pG->sub_angle, -1.53f);
            break;
        case 2:
            G_ROOM_ID = 0x403;
            pG->x4F9F = 0;
            pG->x4F9E = 0;
            FSet(pG->sub_pos.x, 31558.0f);
            FSet(pG->sub_pos.y, 8314.0f);
            FSet(pG->sub_pos.z, 38823.0f);
            FSet(pG->sub_angle, 2.345f);
            break;
        case 3:
            G_ROOM_ID = 0x404;
            pG->x4F9F = 0;
            pG->x4F9E = 0;
            FSet(pG->sub_pos.x, -640.0f);
            FSet(pG->sub_pos.y, 0.0f);
            FSet(pG->sub_pos.z, -15890.0f);
            FSet(pG->sub_angle, 3.13f);
            break;
        }
    } else {
        pG->sub_pos = pG->next_pos;
        FSet(pG->sub_angle, pG->next_angle);
        G_ROOM_ID = pG->next_room;
        pG->x4F9E = pG->next_point;
    }
    if (w->sndId != 0) {
        while (SndEndCheck(w->sndId) == 0) {
            TaskSleep(1);
        }
    }
    IdTexRelease(7);
    IdTexRelease(6);
    IdSys.roomInit();
    primFree();
    if (w->pDat) {
        Mem_free(w->pDat);
        w->pDat = 0;
    }
    if ((s32) pG->flags_54 < 0 || (pG->flags_54 & 0x40000000)) {
        Mem_free(w->pOmk);
    }
    pG->flags_5014 &= ~0x8000;
    IdFreeBuffer();
    Mem_free(w);
    systemVISetBlack(1);
    ScreenReSize(512, 448);
    TaskChain(GameTask, 0);
}

void titleDebugMenu(TitleWork* w)
{
    static char* title_debug_tbl[21] = {
        "DEBUG GAME", "LOAD GAME", "NEW GAME", "STAGE", "ROOM", "JUMP_POINT", "EM_LIST", "PAGE",
        "ENEMY SET", "ETC SET", "SOUND MODE", "SCENARIO", "USE DBMEM", "SHOOT MODE", "PL COSTUME",
        "LANGUAGE", "EFF_COUNTRY", "GAME_COUNTRY", "OMAKE GAME", "GAME LEVEL", "CAPTION",
    };
    static char* pl_type_tbl[7] = {"LEON", "ASHLEY", "ADA", "HUNK", "KLAUSER", "WESKER", "LEON+ASHLEY"};
    static char* sound_mode[3] = {"MONO", "STEREO", "DPL2"};
    static char* shoot_mode[3] = {"OFF", "PHOTO", "VIDEO"};
    static char* language_tbl[8] = {"JPN", "USA", "ENG", "GER", "FRA", "ESP", "ITA", "KOR"};
    static char* game_mode_tbl[7] = {"DEFAULT", "VERY_EASY", "2", "EASY", "3", "NORMAL", "HARD"};
    CRoomInfo* info;
    s16 x;
    s16 y;
    s16 yy;
    int i;

    if (Joy[0].on & 0x00200000) {
        w->dbgX += 4;
    }
    if (Joy[0].on & 0x00100000) {
        w->dbgX -= 4;
    }
    if (Joy[0].on & 0x00400000) {
        w->dbgY += 4;
    }
    if (Joy[0].on & 0x00800000) {
        w->dbgY -= 4;
    }
    x = w->dbgX;
    y = w->dbgY;
    info = pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage] + w->dbgPoint);
    if (pRj->checkRoomNo(w->dbgStage, w->dbgRoom[w->dbgStage]) == w->dbgRoom[w->dbgStage]) {
        eprintf(x, y - 16, 4, 0, "%s", info->name);
    }
    yy = y - 16;
    for (i = 0; i < 21; i++) {
        int col = 0;
        if (i == w->dbgCursor) {
            col = 6;
        }
        eprintf(x, y + i * 16, col, 0, "%s", title_debug_tbl[i]);
    }
    eprintf(x - 8, y + w->dbgCursor * 16, 0, 0, ">");
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", pl_type_tbl[pG->x4FB8]);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "");
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "");
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%x", w->dbgStage);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%02x", info->room);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%x", w->dbgPoint);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", getEmListDbgName(w->dbgEmList));
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%d", pG->debug_mode);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", (pG->flags_68 & 0x00200000) ? "OFF" : "ON");
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", (pG->flags_6C & 0x800) ? "OFF" : "ON");
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", sound_mode[pSys->sound_mode]);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", (pG->flags_68 & 0x04000000) ? "OFF" : "ON");
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", (pG->flags_6C & 0x00200000) ? "ON" : "OFF");
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", shoot_mode[(s8) pG->x4]);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%d", pG->costume2);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", language_tbl[pSys->language]);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", language_tbl[pSys->region]);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", language_tbl[pG->x4F93]);
    if ((s32) pG->flags_54 < 0) {
        yy += 16;
        eprintf(x + 96, yy, 4, 0, "ADA GAME");
    } else if (pG->flags_54 & 0x40000000) {
        yy += 16;
        eprintf(x + 96, yy, 4, 0, "ETC GAME");
    } else {
        yy += 16;
    }
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", game_mode_tbl[pG->x8354]);
    yy += 16;
    eprintf(x + 96, yy, 4, 0, "%s", (pG->flags_68 & 0x400) ? "OFF" : "ON");

    if (Joy[0].rep & 0x00080008) {
        w->dbgCursor--;
    }
    if (Joy[0].rep & 0x00040004) {
        w->dbgCursor++;
    }
    w->dbgCursor = w->dbgCursor < 0 ? 20 : (w->dbgCursor > 20 ? 0 : w->dbgCursor);
    switch (w->dbgCursor) {
    case 0:
        if (Joy[0].trg & 0x00020002) {
            pG->x4FB8 = (pG->x4FB8 + 1) % 7;
        }
        if (Joy[0].trg & 0x00010001) {
            pG->x4FB8 = (pG->x4FB8 + 6) % 7;
        }
        break;
    case 1:
        if (Joy[0].rep & 0x00020002) {
            w->loadNo++;
        }
        if (Joy[0].trg & 0x00010001) {
            w->loadNo--;
        }
        w->loadNo = w->loadNo == 0 ? 10 : (w->loadNo > 10 ? 1 : w->loadNo);
        break;
    case 2:
        break;
    case 3:
        if (Joy[0].rep2 & 0x00020002) {
            w->dbgStage = pRj->getNextStageNo(w->dbgStage, 1);
            w->dbgPoint = 0;
            w->dbgEmList = checkEmListNo(pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage])->room_id);
        }
        if (Joy[0].rep2 & 0x00010001) {
            w->dbgStage = pRj->getNextStageNo(w->dbgStage, -1);
            w->dbgPoint = 0;
            w->dbgEmList = checkEmListNo(pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage])->room_id);
        }
        {
            int no = pRj->checkRoomNo(w->dbgStage, w->dbgRoom[w->dbgStage]);
            if (no >= 0) {
                w->dbgRoom[w->dbgStage] = no;
            }
        }
        break;
    case 4:
        if (Joy[0].rep2 & 0x00020002) {
            w->dbgRoom[w->dbgStage] = pRj->getNextRoomNo(w->dbgStage, w->dbgRoom[w->dbgStage], 1);
            w->dbgPoint = 0;
            w->dbgEmList = checkEmListNo(pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage])->room_id);
        }
        if (Joy[0].rep2 & 0x00010001) {
            w->dbgRoom[w->dbgStage] = pRj->getNextRoomNo(w->dbgStage, w->dbgRoom[w->dbgStage], -1);
            w->dbgPoint = 0;
            w->dbgEmList = checkEmListNo(pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage])->room_id);
        }
        break;
    case 5: {
        s8 room = pRj->getRoomInfo(w->dbgStage, w->dbgRoom[w->dbgStage])->room;
        if (Joy[0].rep2 & 0x00020002) {
            if (pRj->getPointNum(w->dbgStage, room) - 1 == w->dbgPoint) {
                w->dbgPoint = 0;
            } else {
                w->dbgPoint = pRj->getNextPointNo(w->dbgStage, room, w->dbgPoint, 1);
            }
        }
        if (Joy[0].rep2 & 0x00010001) {
            if (w->dbgPoint == 0) {
                w->dbgPoint = pRj->getPointNum(w->dbgStage, room) - 1;
            } else {
                w->dbgPoint = pRj->getNextPointNo(w->dbgStage, room, w->dbgPoint, -1);
            }
        }
        w->dbgPoint = w->dbgPoint < 0 ? 0 : (w->dbgPoint > pRj->getPointNum(w->dbgStage, room) - 1 ? pRj->getPointNum(w->dbgStage, room) - 1 : w->dbgPoint);
        break;
    }
    case 6: {
        int no = w->dbgEmList;
        if (Joy[0].rep & 0x00020002) {
            no++;
        }
        if (Joy[0].rep & 0x00010001) {
            no--;
        }
        w->dbgEmList = no < 0 ? 0 : (no > getEmListNum() - 1 ? getEmListNum() - 1 : no);
        break;
    }
    case 7:
        if (Joy[0].rep & 0x00020002) {
            pG->debug_mode++;
        }
        if (Joy[0].rep & 0x00010001) {
            pG->debug_mode--;
        }
        pG->debug_mode = pG->debug_mode < 0 ? 24 : (pG->debug_mode > 24 ? 0 : pG->debug_mode);
        break;
    case 8:
        if (Joy[0].trg & 0x00030003) {
            pG->flags_68 ^= 0x00200000;
        }
        break;
    case 9:
        if (Joy[0].trg & 0x00030003) {
            pG->flags_6C ^= 0x800;
        }
        break;
    case 10: {
        int old = pSys->sound_mode;
        int m = old;
        if (Joy[0].trg & 0x00020002) {
            m++;
        }
        if (Joy[0].trg & 0x00010001) {
            m--;
        }
        m = m < 0 ? 0 : (m > 2 ? 2 : m);
        if (m != old) {
            pSys->sound_mode = m;
            SndSetOutputMode(pSys->sound_mode, 0);
        }
        break;
    }
    case 11:
        if (Joy[0].trg & 0x00030003) {
            pG->flags_68 ^= 0x04000000;
        }
        break;
    case 12:
        break;
    case 13: {
        int m = (s8) pG->x4;
        if (Joy[0].trg & 0x00020002) {
            m++;
        }
        if (Joy[0].trg & 0x00010001) {
            m--;
        }
        pG->x4 = m < 0 ? 0 : (m > 2 ? 2 : m);
        break;
    }
    case 14: {
        int m = pG->costume2;
        if (Joy[0].trg & 0x00020002) {
            m++;
        }
        if (Joy[0].trg & 0x00010001) {
            m--;
        }
        pG->costume2 = m < 0 ? 0 : (m > 1 ? 1 : m);
        break;
    }
    case 15: {
        int m = pSys->language;
        if (Joy[0].trg & 0x00020002) {
            m++;
        }
        if (Joy[0].trg & 0x00010001) {
            m--;
        }
        pSys->language = m < 0 ? 0 : (m > 7 ? 7 : m);
        break;
    }
    case 16: {
        int m = pSys->region;
        if (Joy[0].trg & 0x00020002) {
            m++;
        }
        if (Joy[0].trg & 0x00010001) {
            m--;
        }
        pSys->region = m < 0 ? 0 : (m > 7 ? 7 : m);
        break;
    }
    case 17: {
        int m = pG->x4F93;
        if (Joy[0].trg & 0x00020002) {
            m++;
        }
        if (Joy[0].trg & 0x00010001) {
            m--;
        }
        pG->x4F93 = m < 0 ? 0 : (m > 7 ? 7 : m);
        break;
    }
    case 18:
        break;
    case 19: {
        int m = pG->x8354;
        if (Joy[0].trg & 0x00020002) {
            m++;
        }
        if (Joy[0].trg & 0x00010001) {
            m--;
        }
        pG->x8354 = m <= 0 ? 1 : (m > 6 ? 6 : m);
        break;
    }
    case 20:
        if (Joy[0].trg & 0x00030003) {
            pG->flags_68 ^= 0x400;
        }
        break;
    }
}
