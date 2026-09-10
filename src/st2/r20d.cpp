#include "types.h"
#include "main_mem.h"
#include "st_room.h"
#include "atari.h"
#include "light.h"
#include "map_obj.h"
#include "widget.h"
#include "flag_rsf.h"
#include "global.h"
#include "main.h"
#include "sce.h"
#include "sce_sys.h"
#include "sce_at.h"
#include "scroll.h"
#include "obj.h"
#include "obj01.h"
#include "em.h"
#include "emtorch.h"
#include "emswitch.h"
#include "em_wrap.h"
#include "etc_model.h"
#include "player.h"
#include "pl_sub.h"
#include "pl_wep.h"
#include "cam_ctrl.h"
#include "camera.h"
#include "motion.h"
#include "math_sub.h"
#include "act_btn.h"
#include "item.h"
#include "mes.h"
#include "sscrn.h"
#include "fade.h"
#include "game.h"
#include "snd.h"
#include "esp.h"
#include "st_mgr_event.h"

// Room 2-0D (D:/Bio4/Prog/r20d.cpp): the hall with the three crank-raised fences, the round switch
// of the picture puzzle, the lantern throwing and the ten pass-through spots.

// One fence: the scroll object, its collision piece and the raise direction.
class cFence {
public:
    cObj* obj;     // 0x00
    cSat* sat;     // 0x04
    Vec pos;       // 0x08  lowered position
    Vec dir;       // 0x14  raise vector
    f32 t;         // 0x20  0 lowered .. 1 raised

    void move(f32 t);
    void init(struct R20dFenceData* d);
};

struct R20dFenceData {
    int objNo;     // 0x00
    f32 w;         // 0x04  collision size x
    f32 d;         // 0x08  collision size z
    Vec dir;       // 0x0C
};

// One lantern the player can pick up and throw at an enemy.
class cLanternUnit {
public:
    cEm* em;         // 0x00  the torch enemy
    cEm* target;     // 0x04
    u32 state;       // 0x08  0 idle, 1 being thrown, 2 done
    int step;        // 0x0C
    int active;      // 0x10
    void* mot[6];    // 0x14  [0] the etc archive, [1..5] player motions

    void destroy();
    void check();
    cEm* getTargetPos(Vec* out);
    static void throwLantern(cLanternUnit* u);
    void setThrowLantern(Vec* target);
};

class cLantern {
public:
    cLanternUnit* units;   // 0x00
    int num;               // 0x04

    void initLantern(void* arc, void* m1, void* m2, void* m3, void* m4, void* m5);
    static void checkLantern(cLantern* p);
};

struct R20dWork {
    cLantern lantern;      // 0x00
    int nFence;            // 0x08
    cFence fence[3];       // 0x0C
    cObj* crank[3];        // 0x78
    cObj* roundSwitch;     // 0x84
    int rsFlip;            // 0x88
    int x8C;               // 0x8C
    int x90;               // 0x90
    f32 wallY;             // 0x94
};

// One pass-through spot: where the player lands, facing which way, and the camera cut.
struct R20dThroughData {
    Vec pos;       // 0x00
    f32 ang;       // 0x0C
    f32 dist;      // 0x10
    int cut;       // 0x14
};

// The work pointer is a struct member: every store through the work reloads it.
struct R20dWorkPtr {
    R20dWork* p;
};

static R20dWorkPtr r20d_work;

static R20dFenceData r20d_fenceData[3] = {
    {0x18, 3000.0f, 300.0f, {0.0f, 2000.0f, 0.0f}},
    {0x29, 300.0f, 3000.0f, {0.0f, 2000.0f, 0.0f}},
    {0x2A, 300.0f, 2100.0f, {0.0f, 0.0f, 2000.0f}},
};

static const R20dThroughData r20d_throughData[10] = {
    {{-4491.0f, 0.0f, 8557.0f}, -1.5707964f, 1500.0f, 0x3F},
    {{-6274.0f, 0.0f, 8557.0f}, 1.5707964f, 1500.0f, 0x38},
    {{-3539.0f, 0.0f, 27204.0f}, -1.5707964f, 1500.0f, -1},
    {{-5201.0f, 0.0f, 27447.0f}, 1.5707964f, 1500.0f, -1},
    {{-6273.0f, 0.0f, 13093.0f}, 1.5707964f, 1500.0f, 0x3E},
    {{-4436.0f, 0.0f, 13093.0f}, -1.5707964f, 1500.0f, 0x3D},
    {{-10338.0f, 0.0f, 13459.0f}, 0.0f, 1500.0f, -1},
    {{-10290.0f, 0.0f, 15281.0f}, 3.1415927f, 1500.0f, -1},
    {{-10165.0f, 0.0f, 10512.0f}, 0.0f, 1500.0f, -1},
    {{-10242.0f, 0.0f, 12142.0f}, 3.1415927f, 1500.0f, -1},
};

static inline void ObjPSet(cObj*& d, cObj* v) { d = v; }
struct PlPtr { cPlayer* p; };
#define pPLS (((PlPtr*) &pPL)->p)
static inline void AtariFlagsAnd(cAtariInfo* a, u16 mask) { a->flags &= mask; }
static inline void AtariFlagsOr(cAtariInfo* a, u16 bit) { a->flags |= bit; }

static void r20d_checkBgmPlay();
void r20d_openShelf_main(int no, int opened);
static void r20d_openedShelf(int no);
static void r20d_openShelf(int no);
void r20d_openDrawer_main(int no, int opened);
static void r20d_openedDrawer(int no);
static void r20d_openDrawer(int no);
static void r20d_checkDoor();
static void r20d_setEm();
static void r20d_checkSwitch(int opened);
void r20d_initCrank();
static void r20d_operateCrank(int no);
static void r20d_execFlagOn();
void r20d_initFence();
static void r20d_execDeathTrap();
static void r20d_moveWall();
void r20d_checkPictureCombination();
static void r20d_checkSalazarCrestUse();
static void r20d_execRoundSwitch();
void r20d_initRoundSwitch();
static void r20d_execThrough(int no);

void R20dInit()
{
#line 55 "D:/Bio4/Prog/r20d.cpp"
    r20d_work.p = (R20dWork*) MEM_CALLOC(sizeof(R20dWork), 1, 0xd);
    if (pG->x4FB8 == 1) {
        r20d_work.p->lantern.initLantern(ROOM_ARC_PTR(pG->pRoomArc, 0xE), ROOM_ARC_PTR(pG->pRoomArc, 0x3D),
                                         ROOM_ARC_PTR(pG->pRoomArc, 0x1F), ROOM_ARC_PTR(pG->pRoomArc, 0x3E),
                                         ROOM_ARC_PTR(pG->pRoomArc, 0x3F), ROOM_ARC_PTR(pG->pRoomArc, 0x40));
        r20d_initFence();
        r20d_initCrank();
        SceExec(0x12, (TaskFunc) r20d_setEm, 0, 0, 2, 0);
        if (RsfCheck(G_ROOM_ID, 0) == 0) {
            SceExec(0x12, (TaskFunc) r20d_checkSwitch, 0, 0, 2, 0);
        } else {
            SceExec(0x12, (TaskFunc) r20d_checkSwitch, 1, 0, 2, 0);
        }
        SceAtDataSet_exec(6, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 0, 1);
        SceAtDataSet_exec(7, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 1, 1);
        SceAtDataSet_exec(8, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 2, 1);
        SceAtDataSet_exec(9, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 3, 1);
        SceAtDataSet_exec(2, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 4, 1);
        SceAtDataSet_exec(3, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 5, 1);
        SceAtDataSet_exec(4, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 6, 1);
        SceAtDataSet_exec(5, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 7, 1);
        SceAtDataSet_exec(0x1C, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 8, 1);
        SceAtDataSet_exec(0x1D, 0x12, 0, (TaskFunc) r20d_execThrough, (void*) 9, 1);
        SceAtDataSet_exec(0x10, 0x12, 0, (TaskFunc) r20d_checkDoor, 0, 1);
    } else {
        cObj* o = SmdGetObjPtr(0x18);

        if (o) {
            o->be_flag |= 0x20;
            o->pos.y += 2100.0f;
        }
        r20d_initFence();
        r20d_work.p->fence[0].move(1.0f);
        r20d_work.p->fence[1].move(1.0f);
        r20d_work.p->fence[2].move(1.0f);
    }
    r20d_initRoundSwitch();
    SceSetItemEvent(0x14, 0x81, 4, 6, r20d_openShelf, (void (*)()) r20d_openedShelf, 0, 0);
    SceSetItemEvent(0x15, 0x80, 5, 5, r20d_openDrawer, (void (*)()) r20d_openedDrawer, 0, 0);
    SceSetItemEvent(0x16, 0x83, 6, 7, r20d_openDrawer, (void (*)()) r20d_openedDrawer, 1, 0);
    SceSetItemEvent(0x17, 0x86, 7, 8, r20d_openDrawer, (void (*)()) r20d_openedDrawer, 2, 0);
    SceExec(0x12, (TaskFunc) r20d_checkBgmPlay, 0, 0, 2, 0);
    SceAtSetActColor(0x1B, 1);
}

void R20dMain()
{
}

static void r20d_checkBgmPlay()
{
    while (SceCkFindPL(0) != 1) {
        SceSleep(1);
    }
    SndRoomStrStart(1, 3, 1);
    while (SceCountEmAlive(0x10, 0x20) != 0) {
        SceSleep(1);
    }
    SndRoomStrStop(3);
}

// The double-door shelf opens (opened: already open, snap the doors).
void r20d_openShelf_main(int no, int opened)
{
    f32 ang = 0.0f;
    cObj* a = 0;
    cObj* b = 0;

    switch (no) {
    case 0:
        a = SmdGetObjPtr(0x21);
        b = SmdGetObjPtr(0x22);
        ang = -2.22f;
        break;
    default:
        SceExit();
        break;
    }
    if (a != 0 && b != 0) {
        a->be_flag |= 0x20;
        b->be_flag |= 0x20;
        if (opened == 1) {
            a->pParts->rot.y = ang;
            b->pParts->rot.y = -ang;
        } else {
            int i;

            ang /= 30.0f;
            SndCall(6, 0x1A, 0, 0, 0, 0);
            for (i = 30; i != 0; i--) {
                a->pParts->rot.y += ang;
                b->pParts->rot.y -= ang;
                SceSleep(1);
            }
        }
    }
}

static void r20d_openedShelf(int no)
{
    r20d_openShelf_main(no, 1);
}

static void r20d_openShelf(int no)
{
    r20d_openShelf_main(no, 0);
}

// Drawer `no` slides out together with the item model in it.
void r20d_openDrawer_main(int no, int opened)
{
    Vec d = {0.0f, 0.0f, 0.0f};
    cObj* obj = 0;
    cModel* item = 0;

    switch ((u32) no) {
    case 0:
        obj = SmdGetObjPtr(0x23);
        item = SceAtItemModelPtr(0x80);
        d.x = 300.0f;
        break;
    case 1:
        obj = SmdGetObjPtr(0x25);
        item = SceAtItemModelPtr(0x83);
        d.x = -400.0f;
        break;
    case 2:
        obj = SmdGetObjPtr(0x28);
        item = SceAtItemModelPtr(0x86);
        d.z = 300.0f;
        break;
    default:
        SceExit();
        break;
    }
    if (obj != 0) {
        obj->be_flag |= 0x20;
        if (opened == 1) {
            PSVECAdd(&obj->pos, &d, &obj->pos);
            if (item != 0) {
                PSVECAdd(&item->pos, &d, &item->pos);
            }
        } else {
            Vec step;
            int i;

            PSVECScale(&d, &step, 0.033333335f);
            SndCall(6, 0x1B, 0, 0, 0, 0);
            for (i = 30; i != 0; i--) {
                PSVECAdd(&obj->pos, &step, &obj->pos);
                if (item != 0) {
                    PSVECAdd(&item->pos, &step, &item->pos);
                }
                SceSleep(1);
            }
        }
    }
}

static void r20d_openedDrawer(int no)
{
    r20d_openDrawer_main(no, 1);
}

static void r20d_openDrawer(int no)
{
    r20d_openDrawer_main(no, 0);
}

static void r20d_checkDoor()
{
    if (!(pG->door_unlock[0] & 0x00200000)) {
        SceAtExecute(0x10);
    } else {
        pG->flags_51C0 |= 0x02000000;
        PlSelect(0);
        SceAtExecute(0x10);
    }
}

static void r20d_setEm()
{
    SceSleep(1);
    if (pG->item_flags[0] & 0x4000) {
        SceDestroyEm(0x10, 0x20);
        SceExit();
    }
}

// The switch that raises / lowers fence 0.
static void r20d_checkSwitch(int opened)
{
    cEm* sw;
    int open;

    getRoomEtcSwitch(0xF, &sw, 1);
    if (sw == 0) {
        SceExit();
    }
    if (opened == 0) {
        open = 0;
        ((cEmSwitch*) sw)->setClosed();
    } else {
        open = 1;
        ((cEmSwitch*) sw)->setOpened();
    }
    for (;;) {
        f32 t = r20d_work.p->fence[0].t;

        if (open == 0) {
            if (t != 0.0f) {
                open = 1;
                ((cEmSwitch*) sw)->setOpen();
                if (pG->flags_174 & 0x40000000) {
                    continue;
                }
            }
            if (((cEmSwitch*) sw)->ckOpen() == 1) {
                SndCall(6, 0x24, 0, 0, 0, 0);
                open = 1;
                for (;;) {
                    t += 0.02f;
                    r20d_work.p->fence[0].move(t);
                    if (t >= 1.0f) {
                        SndCall(6, 0x25, 0, 0, 0, 0);
                        SceAtSetEnable(0, 0);
                        r20d_work.p->fence[0].move(1.0f);
                        goto sleep;
                    }
                    if (((cEmSwitch*) sw)->ckOpen() == 0) {
                        goto sleep;
                    }
                    SceSleep(1);
                }
            }
        } else {
            if (((cEmSwitch*) sw)->ckOpen() == 0) {
                f32 spd;

                SndCall(6, 0x26, 0, 0, 0, 0);
                open = 0;
                spd = 0.0f;
                SceAtSetEnable(0, 1);
                for (;;) {
                    r20d_work.p->fence[0].move(t);
                    t -= spd;
                    spd += 0.005f;
                    if (!(t < 0.0f)) {
                        if (((cEmSwitch*) sw)->ckOpen() == 1) {
                            goto sleep;
                        }
                        SceSleep(1);
                    } else {
                        break;
                    }
                }
                SndCall(6, 0x27, 0, 0, 0, 0);
                r20d_work.p->fence[0].move(0.0f);
            }
        }
    sleep:
        SceSleep(1);
    }
}

void r20d_initCrank()
{
    Vec rot = {0.0f, 1.5707964f, 0.0f};
    Vec p0 = {-3147.0f, 1000.0f, 9699.0f};
    Vec p1 = {-1630.0f, 1000.0f, 18360.0f};
    Vec p2 = {-1630.0f, 1000.0f, 24914.0f};

    ObjPSet(r20d_work.p->crank[0], SetObjSmd(ROOM_ARC_PTR(pG->pRoomArc, 0x29), ROOM_ARC_PTR(pG->pRoomArc, 0x2A), &p0, &rot, 0x10, 1));
    ObjPSet(r20d_work.p->crank[1], SetObjSmd(ROOM_ARC_PTR(pG->pRoomArc, 0x29), ROOM_ARC_PTR(pG->pRoomArc, 0x2A), &p1, &rot, 0x10, 1));
    ObjPSet(r20d_work.p->crank[2], SetObjSmd(ROOM_ARC_PTR(pG->pRoomArc, 0x29), ROOM_ARC_PTR(pG->pRoomArc, 0x2A), &p2, &rot, 0x10, 1));
    if (RsfCheck(G_ROOM_ID, 0) == 0) {
        SceAtDataSet_exec(0, 0x12, 0, (TaskFunc) r20d_operateCrank, (void*) 0, 1);
    } else {
        r20d_work.p->fence[0].move(1.0f);
    }
    if (RsfCheck(G_ROOM_ID, 1) == 0) {
        SceAtDataSet_exec(1, 0x12, 0, (TaskFunc) r20d_operateCrank, (void*) 1, 1);
    } else {
        r20d_work.p->fence[1].move(1.0f);
    }
    if (RsfCheck(G_ROOM_ID, 8) == 0) {
        SceAtDataSet_exec(0xE, 0x12, 0, (TaskFunc) r20d_operateCrank, (void*) 2, 1);
    } else {
        r20d_work.p->fence[2].move(1.0f);
    }
}

// The player turns crank `no`: fence `no` rises while the button is held.
static void r20d_operateCrank(int no)
{
    cObj* crank = 0;
    int idx = 0;
    f32 t;
    int spd = 0;
    int lastMot = 0;
    int accel = 0;
    u32 seId = 0;

    pG->flags_174 |= 0x40000000;
    switch ((u32) no) {
    case 0:
        SceAtSetEnable(0, 0);
        crank = r20d_work.p->crank[0];
        CamCtrl.CutCall(2);
        break;
    case 1:
        SceAtSetEnable(1, 0);
        idx = 1;
        crank = r20d_work.p->crank[1];
        CamCtrl.CutCall(3);
        break;
    case 2:
        SceAtSetEnable(0xE, 0);
        crank = r20d_work.p->crank[2];
        idx = 2;
        CamCtrl.CutCall(0xE);
        break;
    }
    t = r20d_work.p->fence[idx].t;
    ((cUnitEventView*) pPL)->beginEvent(0);
    ((cUnitEventView*) crank)->beginEvent(0);
    pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x34), 3, 0, 5, (int) ROOM_ARC_PTR(pG->pRoomArc, 0x35));
    crank->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x2B), 3, 0, 5, (int) ROOM_ARC_PTR(pG->pRoomArc, 0x2C));
    {
        Vec v = {427.81f, 0.0f, -563.42f};
        cPlayer* pl;
        Vec* pr;

        PSMTXMultVec(crank->mat, &v, &v);
        v.y = pPL->pos.y;
        FSet(pPL->rot.y, crank->rot.y - 1.5707964f);
        pl = pPL;
        pr = &pl->rot;
        pl->setPos(&v);
        pl->setAng(pr);
    }
    while ((PlGetStatus() & 0x00020000) && !(Key.trg & 0x40000000)) {
        int mot;

        accel++;
        if (accel > 8) {
            accel = 8;
            spd -= 5;
            if (spd < 0) {
                spd = 0;
            }
        }
        mot = spd / 20;
        if (mot > 7) {
            mot = 7;
        }
        if (mot != lastMot) {
            void* m0;
            void* m1;
            u32 n;
            u32 frame;
            f32 rate;

            lastMot = mot;
            switch (mot) {
            default:
            case 0:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x35);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x2C);
                break;
            case 1:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x36);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x2D);
                break;
            case 2:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x37);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x2E);
                break;
            case 3:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x38);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x2F);
                break;
            case 4:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x39);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x30);
                break;
            case 5:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x3A);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x31);
                break;
            case 6:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x3B);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x32);
                break;
            case 7:
                m0 = ROOM_ARC_PTR(pG->pRoomArc, 0x3C);
                m1 = ROOM_ARC_PTR(pG->pRoomArc, 0x33);
                break;
            }
            n = *(u16*) m0;
            rate = pPL->frame / (f32) pPL->frameMax;
            frame = (u32) ((f32) n * rate);
            frame++;
            if (frame >= n) {
                frame = 0;
            }
            pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x34), 3, (u16) frame, 5, (int) m0);
            crank->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x2B), 3, (u16) frame, 5, (int) m1);
        }
        if (MotionCheckCrossFrame(&pPL->mot, 0.0f) == 1) {
            SndCall(6, 0x35, 0, 0, 0, 0);
            seId = SndCall(6, 2, &r20d_work.p->fence[idx].obj->pos, 0, 0, 0);
        }
        if (MotionCheckCrossFrame(&pPL->mot, 50.0f) == 1) {
            SndCall(6, 0x35, 0, 0, 0, 0);
            seId = SndCall(6, 2, &r20d_work.p->fence[idx].obj->pos, 0, 0, 0);
        }
        if (MotionCheckCrossFrame(&pPL->mot, 100.0f) == 1) {
            SndCall(6, 0x35, 0, 0, 0, 0);
            seId = SndCall(6, 2, &r20d_work.p->fence[idx].obj->pos, 0, 0, 0);
        }
        t += (f32) (mot + 1) * 0.0005f;
        r20d_work.p->fence[idx].move(t);
        if (!(t < 1.0f)) {
            t = 1.0f;
            r20d_work.p->fence[idx].move(t);
            break;
        }
        if (Key.trg & 0x00080000) {
            spd += accel;
            accel = 0;
            if (spd > 159) {
                spd = 159;
            }
        }
        ActBtn.set(0x2A, 5, 0, 0, 2, 2, 0, 0);
        SceSleep(1);
    }
    FadeSetW(0x80000001, 5, 0, 0);
    ((cUnitEventView*) pPL)->endEvent(0);
    crank->motionPause();
    ((cUnitEventView*) crank)->endEvent(0);
    if (!(t < 1.0f)) {
        if (seId != 0) {
            SndStop(seId, 0);
        }
        SndCall(6, 3, &r20d_work.p->fence[idx].obj->pos, 0, 0, 0);
    } else {
        switch ((u32) no) {
        case 0:
            SceAtSetEnable(0, 1);
            break;
        case 1:
            SceAtSetEnable(1, 1);
            break;
        case 2:
            SceAtSetEnable(0xE, 1);
            break;
        }
    }
    pG->flags_174 &= ~0x40000000;
    CamCtrl.Comeback(0);
}

// Fence at `t` of its way up.
void cFence::move(f32 t)
{
    Vec d = dir;

    PSVECScale(&d, &d, t);
    PSVECAdd(&pos, &d, &d);
    obj->pos = d;
    this->t = t;
    Vec rot = {0.0f, 0.0f, 0.0f};
    sat->setCoord(&obj->pos, &rot);
}

void cFence::init(R20dFenceData* d)
{
    f32 hz;
    f32 hx;

    obj = SmdGetObjPtr(d->objNo);
    obj->be_flag |= 0x20;
    pos = obj->pos;
    dir.x = d->dir.x;
    dir.y = d->dir.y;
    dir.z = d->dir.z;
    t = 0.0f;
    hx = d->w * 0.5f + 100.0f;
    hz = d->d * 0.5f + 100.0f;
    Vec rot = {0.0f, 0.0f, 0.0f};
    Vec v[4] = {{-hx, -1100.0f, -hz}, {hx, -1100.0f, -hz}, {hx, -1100.0f, hz}, {-hx, -1100.0f, hz}};
    sat = SatMgr.create(&obj->pos, &rot, v, 0x40, 0, 4100.0f);
}

static void r20d_execFlagOn()
{
    RsfSet(G_ROOM_ID, 0);
    RsfSet(G_ROOM_ID, 1);
    RsfSet(G_ROOM_ID, 8);
}

void r20d_initFence()
{
    u32 i;

    r20d_work.p->nFence = 3;
    for (i = 0; i < r20d_work.p->nFence; i++) {
        r20d_work.p->fence[i].init(&r20d_fenceData[i]);
    }
    SceAtDataSet_exec(0x19, 0x12, 0, (TaskFunc) r20d_execFlagOn, 0, 1);
}

static void r20d_execDeathTrap()
{
    DiedemoExec(0, 0);
}

// The wall of the picture puzzle rises.
static void r20d_moveWall()
{
    Vec p;
    cPlayer* pl;
    cObj* wall;
    u32 i = 0;
    f32 spd;

    SceEventStart(0);
    pPL->setNoSuspend(1);
    p.x = 4161.0f;
    p.y = -1000.0f;
    p.z = 13197.0f;
    pl = pPL;
    f32 ry = -1.388f;
    pl->setPos(&p);
    p.x = 0.0f;
    p.y = ry;
    p.z = 0.0f;
    pl->setAng(&p);
    CamCtrl.CutCall(0xC);
    wall = SmdGetObjPtr(0x17);
    wall->be_flag |= 0x20;
    const f32 n = 90.0f;
    spd = (4000.0f - wall->pos.y) / n;
    SndCall(6, 8, 0, 0, 0, 0);
    do {
        wall->pos.y += spd;
        i++;
        SceSleep(1);
    } while ((f32) i < n);
    SndCall(6, 9, 0, 0, 0, 0);
    wall->pos.y = 4000.0f;
    SceAtSetEnable(0x11, 0);
    SceAtSetEnable(0x18, 0);
    CamCtrl.Comeback(0);
    pPL->setNoSuspend(0);
    SceEventEnd(0);
}

// Never-called debug helper of the original object: the original REL link dead-stripped its body and
// kept its constant pool (1.0, the signed int->float double, 120, PI/180, 2000, -1, PI/135) right after
// r20d_moveWall's pool; the unit is in modules.py STRIP_UNUSED so ours drops the body too.
static void r20d_dbgWall(int frame)
{
    f32 rate = 1.0f;
    f32 t = (f32) frame;
    f32 ang = t / 120.0f * 0.017453292f;
    f32 y = 2000.0f * ang;

    if (y < -1.0f) {
        rate = 0.023271058f;
    }
    pPL->pos.y = y * rate;
}

void r20d_checkPictureCombination()
{
    SceExec(0x12, (TaskFunc) r20d_moveWall, 0, 0, 2, 0);
    RsfSet(G_ROOM_ID, 2);
    pG->door_flags_51C8 |= 4;
    SceAtSetEnable(0xD, 0);
}

static void r20d_checkSalazarCrestUse()
{
    while (ItemMgr.check(0xF) != 1) {
        SceSleep(1);
    }
    SceEventStart(0);
    RsfSet(G_ROOM_ID, 3);
    CamCtrl.CutCall(0x3C);
    SceSleep(20);
    SndCall(6, 4, 0, 0, 0, 0);
    SceAtSetEnable(0x85, 1);
    SceMesSet(2, 0, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
    SceEventEnd(0);
}

// The player turns the round switch: the two pictures swap.
static void r20d_execRoundSwitch()
{
    if (RsfCheck(G_ROOM_ID, 3) == 0) {
        SceUpCut(1, 0x3C, -1, 4);
        if (ItemMgr.num(0xF) == 0) {
            CamCtrl.Comeback(0);
        } else {
            SubScreenOpen(0x80, 1);
        }
        SceExit();
    }
    SceMesSet(0, 0x200, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
    switch (SceMesGetSelection()) {
    case -1:
    case 0:
    case 2:
        break;
    case 1:
        goto yes;
    default:
        goto yes;
    }
    return;
yes:
    {
        f32 ang;
        cPlayer* pl;

        SceEventStart(0);
        pPL->setNoSuspend(1);
        Vec d = {287.49002f, 0.0f, -544.75f};
        SndCall(6, 5, 0, 0, 0, 0);
        if (r20d_work.p->rsFlip == 0) {
            ang = -1.5707964f;
        } else {
            d.x = -d.x;
            d.z = -d.z;
            ang = 1.5707964f;
        }
        pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x26), 0, 0, 1, 0);
        r20d_work.p->roundSwitch->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x25), 0, 0, 1, 0);
        r20d_work.p->rsFlip ^= 1;
        CamCtrl.CutCall(4);
        FSet(pPL->pos.x, r20d_work.p->roundSwitch->pos.x + d.x);
        FSet(pPL->pos.z, r20d_work.p->roundSwitch->pos.z + d.z);
        FSet(pPL->rot.y, ang);
        FSet(r20d_work.p->roundSwitch->rot.y, ang);
        pl = pPL;
        pl->setPos(&pl->pos);
        pl->setAng(&pl->rot);
        SceSleep(30);
        while (MotionGetState(pPL) != 4) {
            SceSleep(1);
        }
        pPL->setNoSuspend(0);
        CamCtrl.Comeback(0);
        SceEventEnd(0);
        r20d_checkPictureCombination();
    }
}

void r20d_initRoundSwitch()
{
    Vec pos = {3926.0f, 0.0f, 13987.0f};
    Vec rot = {0.0f, 0.0f, 0.0f};
    cModel* m;

    r20d_work.p->roundSwitch = SetObjSmd(ROOM_ARC_PTR(pG->pRoomArc, 0x23), ROOM_ARC_PTR(pG->pRoomArc, 0x24), &pos, &rot, 0x10, 1);
    r20d_work.p->rsFlip = 0;
    r20d_work.p->x8C = 0;
    r20d_work.p->x90 = 0;
    r20d_work.p->wallY = SmdGetObjPtr(0x19)->pos.y;
    SceAtSetEnable(0x12, 0);
    SceAtSetEnable(0x85, 1);
    m = SceAtItemModelPtr(0x85);
    if (m) {
        m->setNoSuspend(1);
        m->lightInfo.x50 = (m->lightInfo.x50 & ~0x20) | 0x10;
    }
    if (RsfCheck(G_ROOM_ID, 3) == 0) {
        SceAtSetEnable(0x85, 0);
    }
    if (pG->x4FB8 == 1) {
        if (RsfCheck(G_ROOM_ID, 2) == 0) {
            SceAtDataSet_exec(0xD, 0x12, 0, (TaskFunc) r20d_execRoundSwitch, 0, 1);
            SceExec(0x12, (TaskFunc) r20d_checkSalazarCrestUse, 0, 0, 2, 0);
            SceAtDataSet_exec(0x13, 0x12, 0, (TaskFunc) r20d_execDeathTrap, 0, 1);
            SceAtSetEnable(0x13, 0);
        } else {
            SmdGetObjPtr(0x17)->pos.y = 4000.0f;
            SmdGetObjPtr(0x17)->be_flag |= 0x20;
            SceAtSetEnable(0x11, 0);
            SceAtSetEnable(0x18, 0);
        }
    } else {
        SmdGetObjPtr(0x17)->pos.y = 4000.0f;
        SmdGetObjPtr(0x17)->be_flag |= 0x20;
        SceAtSetEnable(0x11, 0);
    }
}

// The player squeezes through spot `no`: walk to the spot, the pass-through motion, walk on.
static void r20d_execThrough(int no)
{
    cPlayer* pl = pPL;
    const R20dThroughData* d;
    Vec step;
    Vec a;
    f32 da;
    u32 i;
    f32 dist;
    const f32 frame = 10.0f;   // pool order: 10 before 0.1/PI/0.0

    pl->beginAction();
    AtariFlagsAnd(&pPL->atari, 0xFEFF);
    pPL->atari.setPriority(1);
    pPL->dmg.set(0, 0x80);
    d = &r20d_throughData[no];
    if (d->cut >= 0) {
        CamCtrl.CutCall((s8) d->cut);
    }
    pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x20), 0xA, 0, 0x201, 0);
    PSVECSubtract(&d->pos, &pPL->pos, &step);
    PSVECScale(&step, &step, 0.1f);
    da = Muku2(pPL->rot.y, d->ang, 3.1415927f) * 0.1f;
    for (i = 0; i < 10; i++) {
        cPlayer* p;

        PSVECAdd(&pPL->pos, &step, &pPL->pos);
        pPL->rot.y += da;
        p = pPL;
        a.y = p->rot.y;
        p->setPos(&p->pos);
        a.x = 0.0f;
        a.z = 0.0f;
        p->setAng(&a);
        SceSleep(1);
    }
    {
        cPlayer* p = pPL;

        a.y = d->ang;
        p->setPos((Vec*) &d->pos);
        a.x = 0.0f;
        a.z = 0.0f;
        p->setAng(&a);
    }
    while (MotionGetState(pPL) != 4) {
        SceSleep(1);
    }
    pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x21), 5, 0, 0x205, 0);
    dist = d->dist * d->dist;
    for (;;) {
        if (MotionCheckCrossFrame(&pPL->mot, 0.0f) == 1) {
            SndCall(6, 0xE, 0, 0, 0, 0);
        }
        if (MotionCheckCrossFrame(&pPL->mot, frame) == 1) {
            SndCall(6, 0xD, 0, 0, 0, 0);
        }
        if (PSVECSquareDistance(&d->pos, &pPL->pos) > dist) {
            break;
        }
        SceSleep(1);
    }
    pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x22), 0xA, 0, 0x201, 0);
    while (MotionGetState(pPL) != 4) {
        SceSleep(1);
    }
    CamCtrl.Comeback(0);
    pl->endAction(8);
    pPL->dmg.clear();
    AtariFlagsOr(&pPL->atari, 0x100);
    pPL->atari.setPriority(0);
}

// Every torch / lamp of the room (etc types 0xB and 0x10) becomes a lantern unit.
void cLantern::initLantern(void* arc, void* m1, void* m2, void* m3, void* m4, void* m5)
{
    cEm* em;
    u32 i;

    num = 0;
    for (i = 0; i < 64; i++) {
        if (getRoomEtc(i, 0xB, &em, 0) == 1 || getRoomEtc(i, 0x10, &em, 0) == 1) {
            num++;
        }
    }
#line 1270 "D:/Bio4/Prog/r20d.cpp"
    units = (cLanternUnit*) MEM_ALLOC(num * sizeof(cLanternUnit), 1, 0xd);
    num = 0;
    for (i = 0; i < 64; i++) {
        if (getRoomEtc(i, 0xB, &em, 0) == 1 || getRoomEtc(i, 0x10, &em, 0) == 1) {
            units[num].em = em;
            units[num].active = 1;
            units[num].state = 0;
            units[num].step = 0;
            units[num].mot[0] = arc;
            units[num].mot[1] = m1;
            units[num].mot[2] = m2;
            units[num].mot[3] = m3;
            units[num].mot[4] = m4;
            units[num].mot[5] = m5;
            num++;
        }
    }
    SceExec(0x12, (TaskFunc) cLantern::checkLantern, (int) this, 0, 2, 0);
}

void cLantern::checkLantern(cLantern* p)
{
    for (;;) {
        u32 i;

        for (i = 0; i < p->num; i++) {
            cLanternUnit* u = (cLanternUnit*) (i * sizeof(cLanternUnit) + (u32) p->units);

            if (u->active != 0) {
                switch (u->state) {
                case 0:
                    u->check();
                    break;
                case 1:
                    break;
                case 2:
                    u->destroy();
                    break;
                }
            }
        }
        SceSleep(1);
    }
}

void cLanternUnit::destroy()
{
    if (em) {
        EmMgr.destroy(em);
    }
    em = 0;
    active = 0;
}

// Prompt the throw when the player stands next to the lantern.
void cLanternUnit::check()
{
    Vec a = em->pos;
    Vec b = pPL->pos;
    f32 d;

    a.y += 200.0f;
    if (b.y >= a.y) {
        return;
    }
    if (b.y < a.y - 2000.0f) {
        return;
    }
    const f32 lim = 1000000.0f;   // declared before the call: its `lis` is hoisted above it (callee-saved r30)
    b.y = a.y;
    d = PSVECSquareDistance(&b, &a);
    if (!(d < lim)) {
        return;
    }
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0) != 0) {
        return;
    }
    ActBtn.set(0x17, 5, (int) cLanternUnit::throwLantern, (int) this, 0, 1, 1, 0);
}

// The enemy the lantern flies at (NULL: 10000 units in front of the player, out = that point).
cEm* cLanternUnit::getTargetPos(Vec* out)
{
    Vec p = pPL->pos;
    cModel* t;

    p.y += 1800.0f;
    t = SearchTargetEm(&p, 0, 400000000.0f);
    if (t != 0) {
        *out = t->pos;
        return (cEm*) t;
    }
    {
        Vec rot = {0.0f, 0.0f, 0.0f};
        Vec fwd = {0.0f, 0.0f, 10000.0f};
        Mtx m;

        rot.y = LIMIT_ANGLE(pPL->rot.y + GetXZAngleLocal(&pPL->pos, &em->pos, pPL->rot.y) + 3.1415927f);
        low_RotMatrix(m, &rot);
        TransMatrix(m, &pPL->pos);
        PSMTXMultVec(m, &fwd, out);
    }
    return 0;
}

// The throw action: turn to the lantern, pick it up, turn to the target, throw.
void cLanternUnit::throwLantern(cLanternUnit* u)
{
    // Unreferenced: the five pool words {PI, PI/4, PI/2, 7PI/8, PI/8} between getTargetPos's and this
    // function's constants (an unused function-local static const array is emitted before the pool).
    static const f32 angTbl[5] = {3.1415927f, 0.7853982f, 1.5707964f, 2.7488935f, 0.3926991f};
    Vec pos;
    int st = 0;
    int cnt = 0;
    f32 zero;
    f32 turn;

    u->step = st;
    u->state = 1;
    ((cUnitEventView*) pPL)->beginEvent(0);
    AtariFlagsOr(&pPL->atari, 0x100);
    pPL->dmg.set(0, 0x80);
    u->target = u->getTargetPos(&pos);
    pPL->motionSet(u->mot[st + 1], 0xA, 0, 1, 0);
    u->em->be_flag |= 0x20;
    zero = 0.0f;
    turn = zero;
    Vec pos2 = pos;
    while ((PlGetStatus() & 0x00020000) && MotionGetState(pPL) != 4) {
        if (u->target) {
            pos = u->target->pos;
        }
        switch ((u32) u->step) {   // unsigned: `cmplwi 2; bgt`; `case 4` balances the tree at root 2
        case 0:
            turn = Muku(&pPL->pos, &u->em->pos, pPL->rot.y, 3.1415927f);
            pPL->rot.y = LIMIT_ANGLE(pPL->rot.y + turn);
            u->step++;
        case 1:
            if (MotionCheckCrossFrame(&pPL->mot, 10.0f) == 1) {
                Vec ofs = {-132.59999f, -245.22f, -174.37f};
                Vec rot = {1.5118269f, -0.35282877f, -1.6762177f};

                u->em->pos = ofs;
                u->em->rot = rot;
                ((cEmTorch*) u->em)->setParent(pPLS, 0xA, 0);
                u->step++;
                cnt = 0;
                f32 a = LIMIT_ANGLE(pPL->rot.y + zero);
                turn = Muku(&pPL->pos, &pos, a, 3.1415927f) / 15.0f;
                pos2 = pos;
            }
            break;
        case 2:
            pPL->rot.y = LIMIT_ANGLE(pPL->rot.y + turn);
            if (cnt == 15) {
                u->step++;
            }
            cnt++;
            break;
        case 3:
            if (MotionCheckCrossFrame(&pPL->mot, 33.0f) == 1) {
                u->setThrowLantern(&pos);
                SndCall(1, 0xF, 0, 0, 0, 0);
                SndCall(1, 0x11, 0, 0, 0, 0);
                u->state = 2;
                u->step++;
            }
            break;
        case 4:
            break;
        }
        if (u->em) {
            u->em->matUpdate();
        }
        SceSleep(1);
    }
    pPL->dmg.clear();
    ((cUnitEventView*) pPL)->endEvent(2);
}

// The lantern leaves the hand: an obj01 flies to the target along a parabola.
void cLanternUnit::setThrowLantern(Vec* target)
{
    Vec from;
    Vec rot;
    Vec spd;
    void* tpl;
    void* bin;
    cObj* obj;
    void* zero = NULL;
    const f32 spd0 = 20.0f;   // pool order (20 first) and `lis r25` at the top

    from.x = em->pParts->mat[0][3];
    from.y = em->pParts->mat[1][3];
    from.z = em->pParts->mat[2][3];
    Matrix2AxisAngle(em->pParts->mat, &rot);
    bin = GetEtcAddr(mot[0], "et1000.bin");
    EspGetEfmTplAddr(0xF, &tpl);
    CalcParabolaVector(&spd, &from, target, PSVECDistance(&from, target) / 10.0f + 1.0f);
    obj = SetObj01(bin, tpl, &from, &rot, &spd, spd0, 50.0f, 0xD2, 5);
    Obj01SetEst(obj, 0, 0x10, 3, 1, 1, 0, 0x14, (int) zero, (int) zero);
    EstSet((int) obj, -1, 0, 0, 1, 0, 0, 0, (u32) obj, zero);
}
