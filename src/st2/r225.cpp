#include "types.h"
#include "main_mem.h"
#include "st_room.h"
#include "atari.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "flag_rsf.h"
#include "global.h"
#include "main.h"
#include "sce.h"
#include "sce_sys.h"
#include "sce_at.h"
#include "scroll.h"
#include "obj.h"
#include "em.h"
#include "emhit.h"
#include "player.h"
#include "pl_sub.h"
#include "cam_ctrl.h"
#include "st_mgr_event.h"
#include "act_btn.h"
#include "item.h"
#include "sscrn.h"
#include "cSceObj.h"
#include "esp.h"
#include "est.h"
#include "snd.h"
#include "fade.h"
#include "math_sub.h"
#include "motion.h"
#include "rnd.h"
#include "db_log.h"

// Room 2-25 (D:/Bio4/Prog/r225.cpp): the graveyard / crank puzzle; a local copy of sce_com's
// SceElevator with the chapter end on the way out.

struct R225Work {
    cObj* crank;   // 0x00
    cObj* door;    // 0x04  the SetObjSmd dummy that rides along with the door
};

// sce_com.cpp SceElevatorData
struct SceElevatorData {
    s32 dir;
    u32 objId;
    Vec pos;
    Vec plPos;
    Vec plRot;
    s32 cut;
    u16 pad_30;
    u16 seStart;
    u16 pad_34;
    u16 seStop;
    Vec jumpPos;
    Vec jumpRot;
    u16 room;
};

static R225Work* r225_work;

// Stores through references (not MEM_IN_STRUCT_P): the static pointer / pPL reload after each one.
static inline void FSetP(f32& d, f32 v) { d = v; }
static inline void PSet(cObj*& d, cObj* v) { d = v; }

static SceElevatorData r225_elvArrive = {2, 0x15, {0.0f, 0.0f, 0.0f}, {80130.0f, 1500.0f, -22530.0f}, {0.0f, -1.6f, 0.0f}, -1, 0, 0xE, 0, 0xF, {-3300.0f, 5000.0f, 22200.0f}, {0.0f, 3.14f, 0.0f}, 0x226};
static SceElevatorData r225_elvLeave = {3, 0x15, {0.0f, 0.0f, 0.0f}, {80130.0f, 1500.0f, -22530.0f}, {0.0f, -1.6f, 0.0f}, 8, 0, 0xD, 0, 0xF, {-3300.0f, 5000.0f, 22200.0f}, {0.0f, 3.14f, 0.0f}, 0x226};

static void gnd_open();
static void r225_operateCrank();
void r225_open_door();
static void r225_DoorMes_exec();
static void r225_DoorMes();
static void r225_moveGrave(int dir);
static void r225_checkGrave();
static void first_cut_exit();
static void first_cut();
extern "C" void SceElevator_r225(SceElevatorData* d);

void R225Init()
{
#line 81 "D:/Bio4/Prog/r225.cpp"
    r225_work = (R225Work*) MEM_CALLOC(sizeof(R225Work), 1, 0xd);
    if (RsfCheck(G_ROOM_ID, 0) == 0) {
        SceAtDataSet_exec(4, 0x12, 0, (TaskFunc) r225_operateCrank, 0, 1);
        SceAtSetEnable(5, 0);
        SceAtSetEnable(6, 0);
        SceAtSetEnable(0xA, 1);
    } else {
        SceAtSetEnable(5, 1);
        SceAtSetEnable(6, 1);
        SceAtSetEnable(0xA, 0);
        SmdGetObjPtr(0x27)->be_flag |= 0x20;
        SmdGetObjPtr(0x27)->pos.x = 1393.0f;
    }
    if (RsfCheck(G_ROOM_ID, 1) == 0) {
        SceAtDataSet_exec(0xC, 0x12, 0, (TaskFunc) r225_DoorMes, 0, 1);
        SceExec(0x12, (TaskFunc) r225_DoorMes_exec, 0, 0, 2, 0);
        SceAtSetEnable(0xD, 1);
    } else {
        SmdGetObjPtr(0x25)->be_flag |= 0x20;
        SmdGetObjPtr(0x25)->pos.y = 4579.0f;
        SceAtSetEnable(0xD, 0);
    }
    if (pG->room_id_prev == 0x21B) {
        RsfSet(G_ROOM_ID, 2);
    }
    if (RsfCheck(G_ROOM_ID, 2)) {
        SmdGetObjPtr(0x28)->be_flag |= 0x20;
        SmdGetObjPtr(0x28)->pos.x = 71275.0f;
        SceAtSetEnable(9, 1);
        SceAtSetEnable(8, 1);
        SceAtSetEnable(0xB, 0);
    } else {
        SceAtSetEnable(9, 0);
        SceAtSetEnable(8, 0);
        SceAtSetEnable(0xB, 1);
    }
    SceAtDataSet_exec(1, 0x12, 0, (TaskFunc) SceElevator_r225, &r225_elvLeave, 1);
    if (pG->room_id_prev == 0x226) {
        if (!(pG->flags_54 & 0x80000)) {
            if (!(pG->flags_54 & 0x100)) {
                SceExec(0x12, (TaskFunc) SceElevator_r225, (int) &r225_elvArrive, 0, 2, 0);
            }
        }
    }
    SceAtDataSet_exec(0, 0x12, 0, (TaskFunc) r225_checkGrave, 0, 1);
    if (pG->room_id_prev == 0x21D) {
        if (!(pG->flags_54 & 0x80000)) {
            if (!(pG->flags_54 & 0x100)) {
                SceExec(0x12, (TaskFunc) r225_moveGrave, 0, 0, 2, 0);
            }
        }
    }
    if (RsfCheck(G_ROOM_ID, 3) == 0) {
        SceAtDataSet_exec(0x10, 0x12, 0, (TaskFunc) first_cut, 0, 1);
    }
    {
        Vec pos;
        Vec rot;
        cEmHit* hit;

        pos.x = 24048.0f;
        pos.y = 5550.0f;
        pos.z = -12503.0f;
        rot.x = 0.0f;
        rot.y = -1.08f;
        rot.z = 0.0f;
        hit = SetEmHit(ROOM_ARC_PTR(pG->pRoomArc, 0x35), ROOM_ARC_PTR(pG->pRoomArc, 0x36), &pos, &rot, 2);
        if (hit) {
            hit->setBeetle(ROOM_ARC_PTR(pG->pRoomArc, 0x37), ROOM_ARC_PTR(pG->pRoomArc, 0x39), ROOM_ARC_PTR(pG->pRoomArc, 0x38));
        }
    }
    pG->flags_51C4 |= 0x01000000;
}

void R225Main()
{
}

// The crank raised the ground plate: the door slides open.
static void gnd_open()
{
    SceEventStart(0);
    CamCtrl.CutCall(7);
    SndCall(6, 5, 0, 0, 0, 0);
    SmdGetObjPtr(0x27)->be_flag |= 0x20;
    while (SmdGetObjPtr(0x27)->pos.x < 1393.0f) {
        SceSleep(1);
        SmdGetObjPtr(0x27)->pos.x += 22.0f;
    }
    SmdGetObjPtr(0x27)->pos.x = 1393.0f;
    SceSleep(35);
    RsfSet(G_ROOM_ID, 0);
    SceAtSetEnable(5, 1);
    SceAtSetEnable(6, 1);
    CamCtrl.Comeback(0);
    SceEventEnd(0);
}

// Area 4: Leon turns the crank; the plate rises with the crank speed.
static void r225_operateCrank()
{
    int spd = 0;
    int cur = 0;
    int acc = 0;
    KeyWork* key;

    pG->flags_174 |= 0x80000000;
    PSet(r225_work->crank, SmdGetObjPtr(0x16));
    BitOn(r225_work->crank->be_flag, 0x20);
    ((cUnitEventView*) pPL)->beginEvent(0);
    PlSetHand(1, 0);
    ((cUnitEventView*) r225_work->crank)->beginEvent(0);
    CamCtrl.CutCall(5);
    pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x1F), 3, 0, 5, (int) ROOM_ARC_PTR(pG->pRoomArc, 0x20));
    r225_work->crank->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x2A), 3, 0, 5, (int) ROOM_ARC_PTR(pG->pRoomArc, 0x2B));
    {
        Vec pos = {82071.0f, 1500.0f, -18900.0f};
        cPlayer* pl;
        Vec* rot;

        pos.y = pPL->pos.y;
        FSetP(pPL->rot.y, r225_work->crank->rot.y - 1.5707964f);
        pl = pPL;
        rot = &pl->rot;
        pl->setPos(&pos);
        pl->setAng(rot);
    }
    key = &Key;
    // `if (a && !b) {body} else break;` / `else { gnd_open(); break; }`: no direct `break` within
    // the first insns of the body, so stmt.c leaves the loop un-rotated.
    do {
        CamCtrl.CutCall(5);
        if (MotionCheckCrossFrame(&pPL->mot, 0.0f) == 1 || MotionCheckCrossFrame(&pPL->mot, 50.0f) == 1 ||
            MotionCheckCrossFrame(&pPL->mot, 100.0f) == 1) {
            SndCall(6, 0x35, &pPL->pos, 0, 0, 0);
            SndCall(6, 4, &pPL->pos, 0, 0, 0);
        }
        if ((PlGetStatus() & 0x20000) && !(key->trg & 0x40000000)) {
            int lvl;
            void* mot;
            void* mot2;

            acc++;
            if (acc > 8) {
                acc = 8;
                spd -= 5;
                if (spd < 0) {
                    spd = 0;
                }
            }
            lvl = spd / 20;
            if (lvl > 7) {
                lvl = 7;
            }
            if (lvl != cur) {
                u32 frame;
                u32 max;

                cur = lvl;
                switch (lvl) {
                default:
                case 0:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x20);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x2B);
                    break;
                case 1:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x21);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x2C);
                    break;
                case 2:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x22);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x2D);
                    break;
                case 3:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x23);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x2E);
                    break;
                case 4:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x24);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x2F);
                    break;
                case 5:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x25);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x30);
                    break;
                case 6:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x26);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x31);
                    break;
                case 7:
                    mot = ROOM_ARC_PTR(pG->pRoomArc, 0x27);
                    mot2 = ROOM_ARC_PTR(pG->pRoomArc, 0x32);
                    break;
                }
                max = *(u16*) mot;
                f32 ratio = pPL->frame / (f32) pPL->frameMax;
                frame = (u32) ((f32) max * ratio);
                frame++;
                if (frame >= max) {
                    frame = 0;
                }
                pPL->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x1F), 3, (u16) frame, 5, (int) mot);
                r225_work->crank->motionSet(ROOM_ARC_PTR(pG->pRoomArc, 0x2A), 3, (u16) frame, 5, (int) mot2);
            }
            SmdGetObjPtr(0x27)->be_flag |= 0x20;
            if (SmdGetObjPtr(0x27)->pos.x < 800.0f) {
                SmdGetObjPtr(0x27)->pos.x += (f32) (lvl + 1) * 1.2f;
                if (key->trg & 0x80000) {
                    spd += acc;
                    acc = 0;
                    if (spd > 159) {
                        spd = 159;
                    }
                }
                ActBtn.set(0x2A, 5, 0, 0, 2, 2, 0, 0);
                SceSleep(1);
            } else {
                gnd_open();
                break;
            }
        } else {
            break;
        }
    } while (1);
    r225_work->crank->motionPause();
    PlSetHand(0, 0);
    ((cUnitEventView*) pPL)->endEvent(0);
    ((cUnitEventView*) r225_work->crank)->endEvent(0);
    if (RsfCheck(G_ROOM_ID, 0) == 0) {
        SceAtSetEnable(4, 1);
    } else {
        SceAtSetEnable(4, 0);
    }
    CamCtrl.Comeback(0);
    pG->flags_174 &= ~0x80000000;
}

// The key door slides open (with a dummy copy of the door model riding along).
void r225_open_door()
{
    f32 spd;
    f32 max;

    pG->door_flags_51CC |= 0x04000000;
    SceEventStart(0);
    CamCtrl.CutCall(9);
    SceSleep(15);
    {
        Vec pos = {78329.0f, 3145.0f, -22522.0f};
        Vec rot = {1.5707964f, 1.5707964f, 0.0f};
        r225_work->door = SetObjSmd(ROOM_ARC_PTR(pG->pRoomArc, 0x33), ROOM_ARC_PTR(pG->pRoomArc, 0x34), &pos, &rot, 0x10, 1);
    }
    EstSet(0, -1, 0, 0, 1, 0, 1, 2, 0, 0);
    SndCall(6, 3, 0, 0, 0, 0);
    SceSleep(30);
    spd = 0.0f;
    max = 50.0f;
    SndCall(6, 1, 0, 0, 0, 0);
    SmdGetObjPtr(0x25)->be_flag |= 0x20;
    while (SmdGetObjPtr(0x25)->pos.y < 4579.0f) {
        spd += (max - spd) * 0.1f;
        FAdd(SmdGetObjPtr(0x25)->pos.y, spd);
        if (r225_work->door) {
            r225_work->door->pos.y += spd;
        }
        SceSleep(1);
    }
    SndCall(6, 2, 0, 0, 0, 0);
    SceSleep(25);
    CamCtrl.Comeback(0);
    SceEventEnd(0);
    EffectEspDelete(0, 2, 0, 0);
    EffectEspgenDelete(0, 2, 0);
    EffectEfmDelete(0, 2, 0);
    SceAtSetEnable(0xD, 0);
    SceAtSetEnable(0xC, 0);
}

// Waits for the key item to be used, then opens the door.
static void r225_DoorMes_exec()
{
    while (ItemMgr.check(0x82) != 1) {
        SceSleep(1);
    }
    RsfSet(G_ROOM_ID, 1);
    r225_open_door();
}

// Area 12: the door message; the item screen when Leon has the key.
static void r225_DoorMes()
{
    SceUpCut(0, -1, 6, 0);
    if (ItemMgr.num(0x82) != 0) {
        SubScreenOpen(0x80, 1);
    }
}

// The grave slab (object 0x2E) sinks (dir 1) or rises back (dir 0) with Leon on it.
static void r225_moveGrave(int dir)
{
    cObj* obj = SmdGetObjPtr(0x2E);

    if (obj) {
        pPL->setNoSuspend(1);
        Vec d = {0.0f, -3000.0f, 0.0f};
        cSceObj grave;
        u32 i;

        grave.initMove1_pos(obj, 90, &d, 10.0f, 0.0f);
        grave.setVibration(10, 10, 2.0f, 1.0f, 4.0f);
        {
            cPlayer* pl = pPL;
            u32 n;

            if (pl) {
                for (n = 0; n < 4; n++) {
                    if (grave.sub[n] == NULL) {
                        grave.sub[n] = pl;
                        break;
                    }
                }
            }
        }
        SceEventStart(0);
        pG->flags_58 |= 0x02000000;
        if (dir == 1) {
            CamCtrl.CutCall(6);
            SndCall(6, 7, 0, 0, 0, 0);
            for (i = 0; i < 90; i++) {
                grave.move();
                if (i == 60) {
                    FadeSetW(1, 30, 0, 0);
                }
                SceSleep(1);
            }
        } else {
            CamCtrl.CutCall(0xA);
            grave.setReverse(1);
            SndCall(6, 7, 0, 0, 0, 0);
            for (i = 0; i < 90; i++) {
                grave.move();
                SceSleep(1);
            }
            SndCall(6, 8, 0, 0, 0, 0);
            SceSleep(15);
            CamCtrl.Comeback(0);
        }
        pG->flags_58 &= ~0x02000000;
        SceEventEnd(0);
    }
}

// Area 0: the grave goes down and the room changes.
static void r225_checkGrave()
{
    r225_moveGrave(1);
    SceAtExecute(0);
}

static void first_cut_exit()
{
    SceEventEnd(0);
    if (pG->flags_174 & 0x40000000) {
        SndRoomStrStop(0);
    }
}

// Area 16: the first look at the graveyard (cut 12).
static void first_cut()
{
    RsfSet(G_ROOM_ID, 3);
    SceEventStart(0);
    SndRoomStrStart(1, 0, 1);
    SceSetEventCancel(1, (TaskFunc) first_cut_exit, 0, 1, 1);
    CamCtrl.CutCall(0xC);
    while (CamCtrl.IsMotionEnd() == 0) {
        SceSleep(1);
    }
    SceSetEventCancel(0, 0, 0, -1, 1);
    first_cut_exit();
}

// sce_com's SceElevator without the flags_5014 bit and with the chapter end when leaving the
// first time (room save flag 4).
void SceElevator_r225(SceElevatorData* d)
{
    cPlayer* pl = pPL;
    cObj* obj;
    f32 accel;
    f32 maxSpd;
    f32 minSpd;
    f32 stopDist;
    f32 stopDist2;
    f32 spd;
    f32 step;
    int faded;
    int done;
    int i;
    u32 hSnd;

    obj = SmdGetObjPtr(d->objId);
    if (obj == 0) {
        return;
    }
    maxSpd = 100.0f;
    minSpd = 10.0f;
    accel = 2.0f;
    stopDist = CalcStopDist(maxSpd, accel);
    stopDist2 = stopDist + 4000.0f;
    SceEventStart(0);
    faded = 0;
    done = 0;
    obj->setNoSuspend(1);
    obj->setPos(&d->pos);
    pPL->setNoSuspend(1);
    ((cUnitEventView*) pPL)->beginEvent(0);
    pPL->setPos(&d->plPos);
    pPL->setAng(&d->plRot);
    pPL->be_flag &= ~0x10;
    if (d->cut != -1) {
        CamCtrl.CutCall((s8) d->cut);
    }
    if (d->dir == 1 || d->dir == 3) {
        SndCall(6, d->seStart, &obj->pos, 0, 0, 0);
        spd = accel;
        step = minSpd;
        for (i = 0; i < 10; i++) {
            obj->setPos(&d->pos);
            pPL->setPos(&d->plPos);
            {
                Vec v;

                v.x = obj->pos.x;
                v.y = fRand1_1() * step + obj->pos.y;
                v.z = obj->pos.z;
                obj->setPos(&v);
            }
            {
                Vec v;

                v.x = pPL->pos.x;
                v.y = fRand1_1() * step + pPL->pos.y;
                v.z = pPL->pos.z;
                pPL->setPos(&v);
            }
            SceSleep(1);
        }
        obj->setPos(&d->pos);
        pPL->setPos(&d->plPos);
        for (;;) {
            if (spd > maxSpd) {
                spd = maxSpd;
            }
            step = spd;
            if (d->dir == 1) {
                step = -spd;
            }
            {
                Vec v;

                v.x = obj->pos.x;
                v.y = obj->pos.y + step;
                v.z = obj->pos.z;
                obj->setPos(&v);
            }
            {
                Vec v;

                v.x = pPL->pos.x;
                v.y = pPL->pos.y + step;
                v.z = pPL->pos.z;
                pPL->setPos(&v);
            }
            if (faded == 0) {
                if (!(spd < maxSpd)) {
                    GXColor c0;
                    GXColor c1;
                    *(u32*) &c0 = 0;
                    *(u32*) &c1 = 0xFF;
                    FadeSet(2, &c0, &c1, 30, 0, 0);
                    faded = 1;
                }
            } else if (!(Fade[2].flags & 1)) {
                if (RsfCheck(G_ROOM_ID, 4) == 0) {
                    RsfSet(G_ROOM_ID, 4);
                    SceEventEnd(0);
                    SceSetChapterEnd(0xC, 1);
                    for (;;) {
                        SceSleep(1);
                    }
                }
                SceAtExecRoomJump(d->room, &d->jumpPos, &d->jumpRot, 0);
                break;
            }
            SceSleep(1);
            spd += accel;
        }
    }
    if (d->dir == 0 || d->dir == 2) {
        BitOff(pG->flags_5010, 0x10000000);
        spd = maxSpd;
        step = stopDist2;
        if (d->dir == 0) {
            step = -step;
        }
        {
            Vec v;

            v.x = obj->pos.x;
            v.y = obj->pos.y + step;
            v.z = obj->pos.z;
            obj->setPos(&v);
        }
        {
            Vec v;

            v.z = pl->pos.z;
            v.y = pPL->pos.y + step;
            v.x = pl->pos.x;
            pPL->setPos(&v);
        }
        CamCtrl.Comeback(0);
        {
            GXColor c0;
            GXColor c1;
            *(u32*) &c0 = 0xFF;
            *(u32*) &c1 = 0;
            FadeSet(0x80000002, &c0, &c1, 30, 0, 0);
        }
        hSnd = SndCall(6, d->seStart, &obj->pos, 0, 0, 0);
        do {
            f32 y = obj->pos.y;
            if (fabsf(d->pos.y - y) < stopDist) {
                spd -= accel;
                if (spd < minSpd) {
                    spd = minSpd;
                }
            }
            step = spd;
            if (d->dir != 0) {
                step = -step;
            }
            {
                Vec v;

                v.x = obj->pos.x;
                v.y = y + step;
                v.z = obj->pos.z;
                obj->setPos(&v);
            }
            {
                Vec v;

                v.x = pPL->pos.x;
                v.y = pPL->pos.y + step;
                v.z = pPL->pos.z;
                pPL->setPos(&v);
            }
            {
                Vec q = {0.0f, 0.0f, 0.0f};
                q.y = step;
                pG->quake_ofs = q;
            }
            done = 0;
            if (d->dir == 0) {
                if (!(obj->pos.y < d->pos.y)) {
                    done = 1;
                }
            }
            if (d->dir == 2) {
                if (!(obj->pos.y > d->pos.y)) {
                    done = 1;
                }
            }
            if (done == 0) {
                SceSleep(1);
            }
        } while (done == 0);
        if (hSnd) {
            SndStop(hSnd, 0);
        }
        SndCall(6, d->seStop, &obj->pos, 0, 0, 0);
        obj->setPos(&d->pos);
        pPL->setPos(&d->plPos);
        pPL->setAng(&d->plRot);
        step = 10.0f;
        for (i = 0; i < 10; i++) {
            obj->setPos(&d->pos);
            pPL->setPos(&d->plPos);
            {
                Vec v;

                v.x = obj->pos.x;
                v.y = fRand1_1() * step + obj->pos.y;
                v.z = obj->pos.z;
                obj->setPos(&v);
            }
            {
                Vec v;

                v.x = pPL->pos.x;
                v.y = fRand1_1() * step + pPL->pos.y;
                v.z = pPL->pos.z;
                pPL->setPos(&v);
            }
            SceSleep(1);
        }
        obj->setPos(&d->pos);
        pPL->setPos(&d->plPos);
    }
    pPL->be_flag |= 0x10;
    SceEventEnd(0);
    SceExit();
}
