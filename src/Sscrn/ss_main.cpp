// Sscrn/ss_main: sub screen DLL entry (D:/Bio4/Prog/ss_main.cpp): _prolog/_epilog/_unresolved, the
// SubScreenTask that runs the widget chains, the common id/model/light helpers, the exit and item
// examine widgets and the DLL's own model managers.
// NOT YET WRITTEN: SubScreenTask (0x1428), SsItemExamine::move (0x6E8), clearZbuffer (0x258),
// numDisp (0x280), weaponChangeTask (0x3E0).
#include "types.h"
#include "global.h"
#include "light.h"
#include "atari.h"
#include "map_obj.h"
#include "widget.h"
#include "dbg_button.h"
#include "item.h"
#include "cockpit.h"
#include "mes.h"
#include "id_sys.h"
#include "fade.h"
#include "dvd.h"
#include "main_mem.h"
#include "main.h"
#include "snd.h"
#include "db_log.h"
#include "view.h"
#include "trans.h"
#include "model.h"
#include "camera.h"
#include "sscrn.h"
#include "ss_main.h"

extern "C" void OSReport(const char* fmt, ...);

#define HALT()                                                    \
    {                                                             \
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);            \
        *(volatile u32*) 0x11111111 = 0;                          \
    }

// _ctors/_dtors: the linker script's labels on the .ctors/.dtors lists (null terminated)
extern void (*_ctors[])(void);
extern void (*_dtors[])(void);

class SsExitInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class SsExitMain : public Widget<SUB_SCREEN> {
public:
    virtual void move(SUB_SCREEN* wk);
};

extern "C" {
void SubScreenTask();
void sscrnCameraInit(SUB_SCREEN* wk, Camera* cam);
int sscrnKey2Game(SUB_SCREEN* wk);
void dispScrollBar(u32 top, u32 n, u32 num, IdUnit* bar, IdUnit* up, IdUnit* down);
void generalModelAlloc(SUB_SCREEN* wk);
int sscrnMainMenu(SUB_SCREEN* wk);
void idMainMenu(SUB_SCREEN* wk, int sw);
void idMainMenuFade(SUB_SCREEN* wk, int sw);
void sscrnModelFree(SUB_SCREEN* wk);
void weaponChangeRequest(u16 no, u16 type);
int weaponChangeReadCheck();
int weaponChangeMoveCheck();
void LightSetModel2(cModel* m);
}

cSsPartsMgr ssPartsMgr;
cSsModInfoMgr ssModInfoMgr;

extern "C" void _prolog()
{
    void (**p)(void);

    for (p = _ctors; *p; p++) {
        (*p)();
    }
    OSReport("prolog...\n");
    SubScreenTask();
}

extern "C" void _epilog()
{
    void (**p)(void);

    for (p = _dtors; *p; p++) {
        (*p)();
    }
    OSReport("epilog...\n");
}

extern "C" void _unresolved()
{
    OSReport("unresolved...\n");
#line 139 "D:/Bio4/Prog/ss_main.cpp"
    HALT();
}

void sscrnCameraInit(SUB_SCREEN* wk, Camera* cam)
{
    cam->param.pos.z = 5000.0f;
    cam->up.y = 1.0f;
    cam->up.z = 0.0f;
    cam->param.fovy = 20.0f;
    cam->param.at.x = 0.0f;
    cam->param.at.y = 0.0f;
    cam->param.at.z = 0.0f;
    cam->param.pos.x = 0.0f;
    cam->param.pos.y = 0.0f;
    cam->up.x = 0.0f;
    CameraSetOrientationUp(cam);
    C_MTXPerspective(cam->projMat, cam->param.fovy, 1.3333334f, ZNEAR, ZFAR);
    cam->dist = PSVECDistance(&cam->param.pos, &cam->param.at);
    C_MTXLookAt(cam->viewMat, &cam->param.pos, &cam->up, &cam->param.at);
}

int sscrnKey2Game(SUB_SCREEN* wk)
{
    if (wk->type == 1) {
        if (Key.trg & 0x100000) {
            return 1;
        }
    } else {
        if (Key.trg & 0x40100000) {
            return 1;
        }
    }
    return 0;
}

void dispScrollBar(u32 top, u32 n, u32 num, IdUnit* bar, IdUnit* up, IdUnit* down)
{
    if (n < num) {
        f32 h = up->scr.y - down->scr.y;
        f32 rate = (f32) n / (f32) num;
        bar->sizeY = rate * h;
        bar->flags |= 8;
        rate = (f32) top / (f32) num;
        bar->scr.y = up->pos.y - rate * (up->scr.y - down->scr.y);
    } else {
        bar->flags &= ~8;
    }
}

// Struct-member view of the cModel manager pointers (game/sscrn.cpp MGR_PTR).
struct MgrPtr {
    void* p;
};
#define MGR_PTR(g) (((MgrPtr*) &(g))->p)

void generalModelAlloc(SUB_SCREEN* wk)
{
    int i;

    wk->x38 |= 1;
    ssModInfoMgr.roomInit();
    ssModInfoMgr.arrayAlloc(0xA0);
    ssPartsMgr.roomInit();
    ssPartsMgr.arrayAlloc(0x100);
    MGR_PTR(cModel::mm) = &ssModInfoMgr;
    MGR_PTR(cModel::pm) = &ssPartsMgr;
    MapMgr.roomInit();
    MapMgr.arrayAlloc(0xA0);
    for (i = 0; i < 0xA0; i++) {
        MapMgr.create(0, i);
    }
}

void SsExitInit::init(SUB_SCREEN* wk)
{
    state = 0;
}

void SsExitInit::move(SUB_SCREEN* wk)
{
    switch (state) {
    case 0:
        FadeSetW(0, 3, 0, 0);
        state++;
    case 1:
        if (Fade[0].flags & 1) {
            break;
        }
        if (weaponChangeReadCheck() == 0) {
            break;
        }
        state++;
    case 2:
        sscrnModelFree(wk);
        sscrnLightClear(wk);
        IdTexRelease(8);
        IdSubErase();
        IdNumErase();
        IdFreeBuffer();
        IdSub.roomInit();
        IdNum.roomInit();
        state++;
    case 3:
        transit(0, wk);
        break;
    }
}

void SsExitMain::move(SUB_SCREEN* wk)
{
    wk->x28 = 0;
}

void SsItemExamine::init(SUB_SCREEN* wk)
{
    state = 0;
}

static void sscrnModelTrans(cModel* m)
{
    if (m->be_flag & 2) {
        ModelTrans(m);
    }
}

void sscrnMainMenuInit(SUB_SCREEN* wk, int no)
{
    idMainMenu(wk, no);
}

int sscrnMainMenu(SUB_SCREEN* wk)
{
    int ret = 0;
    s8 old = wk->x265;

    if (Key.trg & 0x100000) {
        wk->x265 = 4;
        wk->x264 = 4;
        ret = 1;
    } else if (Key.trg & 0x40000000) {
        if (old == 4) {
            wk->x265 = old;
            wk->x264 = old;
            ret = 1;
        } else {
            wk->x265 = 4;
        }
    } else if (Key.rep & 0x08000000) {
        wk->x265--;
    } else if (Key.rep & 0x04000000) {
        wk->x265++;
    } else if (Key.trg & 0x80000000) {
        wk->x266 = wk->x264;
        wk->x264 = wk->x265;
        ret = 1;
        if (old != 4) {
            wk->x34 = 0;
            SndCall(0, 4, 0, 0, 0, 0);
        }
    }
    wk->x265 = (s8) wk->x265 < 0 ? 4 : ((s8) wk->x265 > 4 ? 0 : wk->x265);
    if (old != (s8) wk->x265) {
        idMainMenu(wk, 1);
        SndCall(0, 0xA, 0, 0, 0, 0);
    }
    return ret;
}

void idMainMenu(SUB_SCREEN* wk, int sw)
{
    IdUnit* u;
    int i;

    for (i = 0; i < 5; i++) {
        u = IdSub.unitPtr(i, 0);
        u->flags &= ~8;
    }
    if (sw) {
        u = IdSub.unitPtr(wk->x265, 0);
        u->flags |= 8;
        IdSub.setTime(u, 0);
    }
}

void idMainMenuFade(SUB_SCREEN* wk, int sw)
{
    if (wk->type != 0x10) {
        IdUnit* u = IdSub.unitPtr(7, 0);
        if (sw) {
            u->dir &= ~0xF;
        } else {
            u->dir |= 0xF;
        }
    }
}

void IdSubErase()
{
    IdSub.kill(0xFF, 0x1C);
    IdSub.kill(0xFF, 0x1D);
    IdSub.kill(0xFF, 0x1E);
    IdSub.kill(0xFF, 0x1F);
    IdSub.kill(0xFF, 0x14);
    IdSub.kill(0xFF, 0x15);
    IdSub.kill(0xFF, 0x16);
    IdSub.kill(0xFF, 0x10);
    IdSub.kill(0xFF, 0x11);
    IdSub.kill(0xFF, 0x12);
    IdSub.kill(0xFF, 0x18);
    IdSub.kill(0xFF, 0x19);
    IdSub.kill(0xFF, 0x1A);
    IdSub.kill(0xFF, 0x80);
    IdSub.kill(0xFF, 0x81);
    IdSub.kill(0xFF, 0x82);
    IdSub.kill(0xFF, 0x83);
    IdSub.kill(0xFF, 0x84);
    IdTexRelease(9);
}

void IdNumErase()
{
    int i;

    for (i = 0; i < 0x3E; i++) {
        IdNum.killI(0xFF, 0x40 + i);
    }
    IdNum.kill(0xFF, 0x10);
    IdNum.kill(0xFF, 0x11);
    IdNum.kill(0xFF, 0x12);
    IdNum.kill(0xFF, 0x14);
    IdNum.kill(0xFF, 0x15);
    IdNum.kill(0xFF, 0x16);
}

void sscrnModelClear(SUB_SCREEN* wk)
{
    u32 i;

    for (i = 3; i < 0xA0; i++) {
        MapMgr.getWork(i)->be_flag &= ~2;
    }
}

void sscrnModelFree(SUB_SCREEN* wk)
{
    int off = !(wk->x38 & 1);

    if (off) {
        return;
    }
    {
        MGR_PTR(cModel::mm) = &ModInfoMgr;
        MGR_PTR(cModel::pm) = &PartsMgr;
        MapMgr.destroyAll();
        ssModInfoMgr.arrayFree();
        ssPartsMgr.arrayFree();
        MapMgr.arrayFree();
        wk->x38 &= ~1;
    }
}

void sscrnLightClear(SUB_SCREEN* wk)
{
    int i;

    for (i = 0; i < 8; i++) {
        LightMgr.destroy(wk->x21C[i]);
        wk->x21C[i] = 0;
    }
}

void sscrnLightCreate(SUB_SCREEN* wk, cLit* lit)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (wk->x21C[i] == 0) {
            wk->x21C[i] = LightMgr.create(lit, 0, i, 0);
        }
    }
}

void weaponChangeRequest(u16 no, u16 type)
{
    SUB_SCREEN* wk = &SubScreenWk;

    switch (pG->x4FB8) {
    case 1:
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        wk->wepChange[wk->x251].req = 1;
        wk->wepChange[wk->x251].no = no;
        wk->wepChange[wk->x251].type = type;
        break;
    }
}

int weaponChangeReadCheck()
{
    SUB_SCREEN* wk = &SubScreenWk;

    if (wk->wepChange[0].req == 0 && wk->wepChange[1].req == 0) {
        return 1;
    }
    return 0;
}

int weaponChangeMoveCheck()
{
    return SubScreenWk.x250 == 3 || SubScreenWk.x250 == 4;
}

void LightSetModel2(cModel* m)
{
    LightMgr.setModel2(m);
}
