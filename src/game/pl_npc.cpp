// game/pl_npc.cpp: cSubChar, the partner character (Ashley): routine tables, follow / escape /
// hide / action logic, damage, neck and face control. Owns the cSubChar vtable.

#include "atari.h"
#include "light.h"
#include "dmg.h"
#include "player.h"
#include "pl_npc.h"
#include "pl_sub.h"
#include "global.h"
#include "db_log.h"
#include "main.h"
#include "joy.h"
#include "snd.h"
#include "motion.h"
#include "math_sub.h"
#include "act_btn.h"
#include "emwindow.h"
#include "emdoor.h"
#include "sce_at.h"
#include "cam_ctrl.h"
#include "esp.h"
#include "est.h"
#include "rnd.h"
#include "em_sub.h"
#include "emhit.h"
#include "embarrel.h"
#include "at_mod.h"
#include "route_ck.h"
#include "cMotBase.h"
#include "eprintf.h"
#include "dbmodule.h"
#include "main_mem.h"
#include "new"

extern "C" {
double atan2(double y, double x);
f32 sinf(f32 x);
void* memset(void* dst, int c, unsigned int n);
void ShapeMove(void* p);
u16 MotionMoveF(cModel* m, int flag) asm("MotionMove");   // motion.h declares the one-argument form
int SubLadderClimbCk(cModel* m);
int SubLadderClimbCk2(cModel* m);
void pl_fall_ok0();
void pl_fall_ok();
void catchOn();
int getFallPos(cSubChar* pl, Vec* pos, Vec* rot);
void waterProc(cSubChar* pl);
}

#define VALID_PTR(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)

// Motion data `no` of the partner's motion archive.
#define SUB_MOT(pl, no) PL_ARC_PTR((pl)->subArc, no)
// The partner's cMotBase (em.h keeps the bytes under pl_sub's name).
#define SUB_MOTBASE(pl) ((cMotBase*) &(pl)->subFlags58C)
// MotionSetCore with the sequence table as the 4th argument (declared int in motion.h).
#define MOT_SET(m, w, data, seq, a, b, c) MotionSetCore(m, w, data, (int) (seq), a, b, c)

const Vec cSubChar::atckPos = { -100.0f, 0.0f, -500.0f };
const Vec cSubChar::atckPos2 = { 300.0f, 0.0f, -500.0f };

// Eye direction state (moveFace): a class with a constructor, so the file-scope static gets the
// dynamic initialiser the original has.
struct SubEyeDir {
    f32 x;
    f32 y;
    f32 z;
    SubEyeDir()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
};
static SubEyeDir eyeDir;

int cSubChar::mot_ck()
{
    return hp <= (s16) pG->sub_life_max / 2;
}

cSubChar::cSubChar()
{
    subFlags2 = 0;
    subFlags = 0;
    new (SUB_MOTBASE(this)) cMotBase;
    subSelf = this;
    subLight = 0;
    subFunc = 0;
    neckInit();
    eyeDir.x = 0.0f;
    eyeDir.z = 0.4f;
    eyeDir.y = 0.0f;
}

cSubChar::~cSubChar()
{
    if (subLight) {
        LightMgr.destroy(subLight);
    }
    pSUB = 0;
}

void cSubChar::init()
{
    initCloth();
    x12D = 1;
    {
        static const Vec lightOfs = { 0.0f, 0.0f, 0.0f };
        static const Vec lightSize = { 1000.0f, 1000.0f, 0.0f };
        lightInfo.init2(0, 1, &lightOfs, &lightSize, 0x40);
    }
    atari.init(1, 0x1000, 10, 0.0f, -200.0f, 0.0f, 300.0f, 200.0f, 400.0f, 900.0f);
    if (subLight == 0) {
        subLight = LightMgr.createBack(0, 2, 0, 0);
        subLight->setParent(this);
    }
    subSelf->lockParts = 4;
    subSelf->lockOfs.z = 0.0f;
    subSelf->lockOfs.x = 0.0f;
    subSelf->lockOfs.y = 0.0f;
    subSelf->setStatus(1);
    subFlags |= 0x40;
    subFlags &= 0xFFF4;
    subAux0 = 0;
    subAux1 = 0;
    sub580 = 0;
    sub550 = 0;
    be_flag |= 0x02200000;
    atari.flags |= 0x300;
    YarareInit(this, 0.0f, -30.0f, 0.0f, 140.0f, 100.0f, 2, 1);
    YarareAdd(this, &subHit[0], 0.0f, 0.0f, 0.0f, 150.0f, 130.0f, 3, 1);
    YarareAdd(this, &subHit[1], -20.0f, -300.0f, 0.0f, 120.0f, 300.0f, 0x13, 1);
    YarareAdd(this, &subHit[2], 20.0f, -300.0f, 0.0f, 120.0f, 300.0f, 0x17, 1);
    if (mot_ck()) {
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x12), 0, 0, 5, 0);
    } else {
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x6E), 0, 0, 5, 0);
    }
    motionMove();
    subBustBase[0] = getPartsPtr(0x1D)->pos;
    subBustBase[1] = getPartsPtr(0x1E)->pos;
    subBustBase[2] = getPartsPtr(0x1A)->pos;
#line 231 "D:/Bio4/Prog/pl_npc.cpp"
    p2A4 = (EmWork2A4*) MEM_ALLOC(0x98, 1, 13);
}

void cSubChar::move()
{
    static void (cSubChar::*NpcFuncTbl[])() = {
        &cSubChar::moveCore,
        &cSubChar::moveDamage,
        &cSubChar::moveDie,
        &cSubChar::moveBull,
        0,
        &cSubChar::moveEvent,
        &cSubChar::moveDijection,
    };
    cMotBase* mb;
    Vec water;

    if (subFlags & 1) {
        return;
    }
    hp = pG->sub_life;
    mb = SUB_MOTBASE(this);
    mb->adjust();
    BitOff(pG->flags_5010, 0x8000);
    BitOff(pG->flags_5014, 0x20000000);
    BitOff(pG->flags_5010, 0x8);
    subPlStatus = PlGetStatus();
    dmgCheck();
    damageCheck();
    if (xFC == 4) {
        subFunc();
    } else {
        (this->*NpcFuncTbl[xFC])();
    }
    if (pG->flags_68 & 0x10000) {
        int i;

        for (i = 0; i < PlKaiou + 1; i++) {
            if (xFC == 4) {
                subFunc();
            } else {
                (this->*NpcFuncTbl[xFC])();
            }
        }
    }
    if (sub580) {
        sub580--;
    }
    backCheckMove();
    neckCtrl();
    moveFace();
    mb->move();
    shadowCtrl();
    partsWorldCalc();
    EmAtCheck(this);
    SatMgr.check(this, 0);
    atari.move();
    PartsWorldPosCalc(this);
    seqSeCtrl();
    moveCloth();
    moveBust();
    if (subShape) {
        ShapeMove(subShape);
    }
    if (GetWaterHeight(&pos, &water.x) && water.x > pos.y) {
        waterProc(this);
    }
    debugMove();
}

void cSubChar::moveCore()
{
    static void (cSubChar::*funcTbl[])() = {
        &cSubChar::moveFootwork,
        &cSubChar::moveMove,
        &cSubChar::moveBehind,
        0,
        &cSubChar::moveKagamu,
        &cSubChar::moveBehind,
        &cSubChar::movePants,
        &cSubChar::moveDown,
        &cSubChar::moveFance,
        &cSubChar::moveFall,
        &cSubChar::moveAction,
        0,
        &cSubChar::moveLadder,
        0,
        &cSubChar::moveBack,
        &cSubChar::moveAux,
        &cSubChar::moveHide,
        &cSubChar::moveStoop,
        &cSubChar::moveFallWait,
        &cSubChar::moveLadderWait,
        &cSubChar::moveWindowWait,
    };

    analyze();
    (this->*funcTbl[xFD])();
    if ((s16) pG->pl_life <= 0) {
        switch (xFD) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 0xE:
            xFC = 6;
            xFF = 0;
            xFD = 0;
            xFE = 0;
            dmg.set(0, 0x80);
            break;
        }
    }
}

// Routine bytes written through an inline taking ints (pl_class PlRoutineSet): the stores come out
// in the original's order.
static inline void SubRoutineSet(cSubChar* pl, int r0, int r1, int r2, int r3)
{
    pl->xFC = r0;
    pl->xFD = r1;
    pl->xFE = r2;
    pl->xFF = r3;
}

// Routine 0 / 0: standing still, looking around, turning towards the player.
void cSubChar::moveFootwork()
{
    switch (xFE) {
    case 0:
        if (xFF == 1) {
            void* m;
            void* seq;

            if (mot_ck()) {
                m = SUB_MOT(subSelf, 0x12);
                seq = SUB_MOT(subSelf, 0x4F);
            } else {
                m = SUB_MOT(subSelf, 0x6E);
                seq = 0;
            }
            MOT_SET(subSelf, MOTION(subSelf), m, seq, 0, 4, 0);
            subSelf->motionMove();
        }
        if (mot_ck()) {
            subFlags &= ~4;
        } else {
            subFlags |= 4;
        }
        subHideMode = 0;
        if (subFlags & 8) {
            if (subDist > 400.0f) {
                SubRoutineSet(this, 0, 1, 0, 0);
                return;
            }
            xFE = 0x32;
            break;
        }
        xFE = 1;
    case 1: {
        void* m;
        void* seq;
        void* back;

        if (mot_ck()) {
            m = SUB_MOT(subSelf, 0x12);
            seq = SUB_MOT(subSelf, 0x4F);
            back = SUB_MOT(subSelf, 0x18);
        } else {
            m = SUB_MOT(subSelf, 0x6E);
            seq = 0;
            back = 0;
        }
        MOT_SET(subSelf, MOTION(subSelf), m, seq, 0x14, 4, 0);
        backCheckSet(back);
        sub40A = Rnd();
        if (sub40A < 60) {
            sub40A += 60;
        }
        xFE = 2;
    }
    case 2:
        neckSet(&pPL->pos);
        sub40A--;
        if (sub40A != 0xFF) {
            break;
        }
        if (checkBackEm()) {
            break;
        }
        if (subAng > 2.617994f || subAng < -2.617994f) {
            sub40A = 60;
            xFE = 0x14;
            break;
        }
        if (subAng > 0.5235988f || subAng < -0.5235988f) {
            sub40A = 6;
            xFE = 0xA;
            break;
        }
        sub40A = Rnd();
        if (sub40A < 100) {
            sub40A += 100;
        }
        xFE = 0x1E;
        break;
    case 0xA:
        if (subAng < -0.19634955f) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x35), SUB_MOT(subSelf, 0x5D), 7, 5, 0);
            subSelf->xFE = 0xB;
        } else if (subAng > 0.19634955f) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x36), SUB_MOT(subSelf, 0x5E), 7, 5, 0);
            subSelf->xFE = 0xC;
        } else {
            subSelf->xFE = 1;
        }
        break;
    case 0xB:
        if (subAng >= -0.19634955f) {
            subSelf->xFE = 1;
        }
        break;
    case 0xC:
        if (subAng < 0.19634955f) {
            subSelf->xFE = 1;
        }
        break;
    case 0x14:
        if (mot_ck()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x1A), SUB_MOT(subSelf, 0x53), 7, 5, 0);
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x72), 0, 7, 5, 0);
        }
        subSelf->xFE = 0x1F;
    case 0x15:
        if (subSelf->frame >= (f32) (subSelf->frameMax - 1)) {
            subSelf->xFE = 1;
        }
        break;
    case 0x1E:
        setFace(2);
        if (!(subFlags & 4)) {
            subFlags |= 4;
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x19), 0, 7, 5, 0);
            xFE = 0x1F;
        } else {
            neckSet(&pPL->pos);
            xFF = Rnd();
            if (xFF < 60) {
                sub40A += 60;
            }
            xFE = 0x20;
        }
    case 0x1F:
        if (subSelf->frame >= (f32) (subSelf->frameMax - 1) || checkBackEm()) {
            setFace(4);
            subSelf->xFE = 1;
        }
        break;
    case 0x20:
        neckSet(&pPL->pos);
        xFF--;
        if (xFF == 0 || checkBackEm()) {
            xFF = 0;
            setFace(4);
            subSelf->xFE = 1;
        }
        break;
    case 0x28:
        if (subSelf->frame >= (f32) (subSelf->frameMax - 1)) {
            subSelf->xFE = 0;
        }
        break;
    case 0x32:
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x43), 0, 7, 1, 0);
        xFE = 0x33;
    case 0x33:
        if (subSelf->frame >= (f32) (subSelf->frameMax - 1)) {
            MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x44), 0, 7, 5, 0);
            xFE = 0x34;
        }
        break;
    }
    motionMove();
    if ((subFlags & 2) && (subFlags2 & 0x200) && !(subFlags2 & 2)) {
        subHideMode++;
        if (subHideMode > 150) {
            subHideMode = 0;
            subSndId = SndCall(8, 2, &subSelf->pParts->worldPos, id, 0, 0);
        }
    }
    backCheckCtrlFootwork();
    if (subFlags2 & 4) {
        SubRoutineSet(this, 0, 6, 0, 0);
        return;
    }
    if (subFlags2 & 0x10) {
        SubRoutineSet(this, 0, 4, 0, 0);
        return;
    }
    if (plDownCheck()) {
        SubRoutineSet(this, 0, 0x11, 0, 0);
        return;
    }
    if (!(subFlags2 & 2) && (subFlags2 & 1)) {
        SubRoutineSet(this, 0, 7, 0, 0);
        return;
    }
    if (subFlags & 2) {
        return;
    }
    if (subPlStatus & 0x40000) {
        return;
    }
    if (sub550) {
        sub550--;
        if (sub550 == 0) {
            checkAnotherRoute();
        }
        return;
    }
    if (subPlStatus & 0x400) {
        return;
    }
    if (subFlags2 & 0x100) {
        return;
    }
    if ((pG->flags_5010 & 0x8000) && (subFlags2 & 2)) {
        SubRoutineSet(this, 0, 7, 0, 0);
        return;
    }
    if ((pG->flags_5014 & 0x40000000) && (subFlags2 & 2)) {
        if ((u8) (xFE - 0x32) > 9) {
            xFE = 0x32;
        }
        return;
    }
    if ((subPlStatus & 0x20000) && subDist < 1000.0f) {
        return;
    }
    if (readyCheck()) {
        SubRoutineSet(this, 0, 1, 0, 0);
        return;
    }
    switch (actCheck()) {
    case 7:
        SubRoutineSet(this, 0, 2, 0, 0);
        break;
    case 8:
        SubRoutineSet(this, 0, 0xA, 0, 0);
        break;
    case 9:
        SubRoutineSet(this, 0, 0xC, 0, 0);
        break;
    case 5:
    case 6:
        break;
    default:
        if (subPlStatus & 0x41F) {
            cSubChar* s = subSelf;

            if ((s->subFlags & 8) && !(s->subFlags2 & 0x40)) {
                s->sub408 = 0;
                subSelf->sub409 = 5;
                SubRoutineSet(subSelf, 0, 1, 0, 0);
            } else if (subDist > 1100.0f || sub580) {
                s->sub408 = 0;
                subSelf->sub409 = 5;
                SubRoutineSet(subSelf, 0, 1, 0, 0);
            }
        }
        break;
    }
}

// Routine 0 / 1: walk or run after the player to subTarget.
void cSubChar::moveMove()
{
    switch (subSelf->xFE) {
    case 0:
        sub40A = 0;
        if (subAng > 2.617994f || subAng < -2.617994f) {
            if (!(subFlags & 0x10) && (ckPlRun() || subDist > 3000.0f)) {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x26), SUB_MOT(subSelf, 0x58), 7, 5, 0);
                subSelf->xFF = 3;
            } else {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x26), SUB_MOT(subSelf, 0x58), 7, 5, 0);
                subSelf->xFF = 2;
            }
        } else {
            if (!(subFlags & 0x10) && (ckPlRun() || subDist > 3000.0f)) {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x25), SUB_MOT(subSelf, 0x57), 7, 4, 0);
                subSelf->xFF = 1;
            } else {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x24), SUB_MOT(subSelf, 0x56), 7, 4, 0);
                subSelf->xFF = 0;
            }
        }
        checkAnotherRoute();
        subBackMot.blendRate = 0.0f;
        sub405 = 0;
        sub406 = 30;
        sub404 = 0;
        subSelf->xFE = 1;
    case 1:
        if (subSelf->xFF <= 1) {
            rot.y += Muku(&pos, &subTarget, rot.y, 0.20943952f);
        }
        if (!subSelf->motionMove()) {
            break;
        }
        subSelf->xFE = 2;
    case 2:
        switch (xFF) {
        case 2:
            xFF = 0;
        case 0:
            if (mot_ck()) {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x13), SUB_MOT(subSelf, 0x50), 7, 4, 0);
                backCheckSet(SUB_MOT(subSelf, 0x15));
            } else {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x6F), SUB_MOT(subSelf, 0x76), 7, 4, 0);
                backCheckSet(0);
            }
            break;
        case 3:
            subSelf->xFF = 1;
        case 1:
            if (mot_ck()) {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x16), SUB_MOT(subSelf, 0x52), 7, 4, 0);
                backCheckSet(SUB_MOT(subSelf, 0x17));
            } else {
                MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x71), SUB_MOT(subSelf, 0x78), 7, 4, 0);
                backCheckSet(0);
            }
            break;
        }
        subSelf->xFE = 3;
    case 3:
        motionMove();
        switch (subSelf->xFF) {
        case 0:
            if (!(subFlags & 0x10) && (ckPlRun() || subDist > 3000.0f)) {
                xFE = 2;
                sub409 = 3;
                xFF = 1;
                sub408 = (u8) (pPL->frame * 100.0f / (f32) (int) pPL->frameMax);
            }
            break;
        case 1:
            if ((subFlags & 0x10) || (!ckPlRun() && subDist < 550.0f)) {
                xFE = 2;
                sub409 = 3;
                xFF = 0;
                sub408 = (u8) (pPL->frame * 100.0f / (f32) (int) pPL->frameMax);
            }
            break;
        }
        if (subDist > 400.0f || (subFlags & 8) || sub580) {
            rot.y += Muku(&pos, &subTarget, rot.y, 0.20943952f);
        } else if (subPlStatus & 4) {
            rot.y += Muku2(rot.y, LIMIT_ANGLE(pPL->rot.y + 3.1415927f), 0.10471976f);
        } else {
            rot.y += Muku2(rot.y, pPL->rot.y, 0.10471976f);
        }
        if ((subFlags & 8) || sub580) {
            if (subDist <= 400.0f) {
                xFE = 4;
                motState &= ~1;
                subHideMode = 0;
            }
        } else if (subDist <= 400.0f) {
            if (pPL->xFC == 0 && (pPL->xFD == 0 || pPL->xFD == 4)) {
                xFF = 0;
                xFC = 0;
                subDist = 0.0f;
                xFD = 0;
                xFE = 0;
            }
        }
        break;
    case 4:
        motionMove();
        subHideMode++;
        if (subDist < 50.0f || subHideMode > 6) {
            setPos(&subTarget);
            if (subMoveTo[3] == 193.0f) {
                xFF = 0;
                subFlags2 |= 0x40;
                xFC = 0;
                xFD = 0;
                xFE = 0;
            } else {
                xFE = 0xA;
            }
        }
        break;
    case 0xA:
        if (Muku2(rot.y, subMoveTo[3], 3.1415927f) < 0.0f) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x35), SUB_MOT(subSelf, 0x5D), 4, 5, 0);
            xFE = 0xB;
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x36), SUB_MOT(subSelf, 0x5E), 4, 5, 0);
            xFE = 0xC;
        }
        motionMove();
        break;
    case 0xB:
        motionMove();
        if (Muku2(rot.y, subMoveTo[3], 3.1415927f) > 0.0f) {
            rot.y = subMoveTo[3];
            subFlags2 |= 0x40;
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    case 0xC:
        motionMove();
        if (Muku2(rot.y, subMoveTo[3], 3.1415927f) < 0.0f) {
            rot.y = subMoveTo[3];
            subFlags2 |= 0x40;
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
    backCheckCtrlMove();
    if ((subFlags & 8) || (subFlags & 0x10)) {
        Vec p;
        Vec r;

        MotionGetSpeed(this, MOTION(this), 0, &p, &r);
        MotionAddSpeed(this, MOTION(this), &p, &r);
    } else if (xFE == 4) {
        Vec d;

        PSVECSubtract(&subTarget, &pos, &d);
        PSVECScale(&d, &d, 0.4f);
        PSVECAdd(&pos, &d, &pos);
    } else if (xFF <= 1) {
        static f32 distMin = 310.0f;
        f32 spd;

        if (subSelf->xFF == 0) {
            if (subDist > 600.0f) {
                spd = 100.0f;
            } else if (subDist < distMin) {
                spd = 0.0f;
            } else {
                spd = (subDist - distMin) * 100.0f / (600.0f - distMin);
            }
        } else {
            if (subDist > 600.0f) {
                spd = 160.0f;
            } else if (subDist < distMin) {
                spd = 0.0f;
            } else {
                spd = (subDist - distMin) * 160.0f / (600.0f - distMin);
            }
        }
        movePos(&subTarget, spd);
    }
    if (sub40A <= 0xF9) {
        sub40A++;
    }
    if (subFlags2 & 4) {
        SubRoutineSet(this, 0, 6, 0, 0);
    } else if (subFlags2 & 0x10) {
        SubRoutineSet(this, 0, 4, 0, 0);
    } else if ((pG->flags_5014 & 0x40000000) && (subFlags2 & 2)) {
        SubRoutineSet(this, 0, 0, 0x32, 0);
    } else if (!(subFlags2 & 2) && (subFlags2 & 1)) {
        SubRoutineSet(this, 0, 7, 0, 0);
    } else if ((pG->flags_5010 & 0x8000) && (subFlags2 & 2)) {
        SubRoutineSet(this, 0, 7, 0, 0);
    } else if (subFlags & 2) {
        SubRoutineSet(this, 0, 0, 0, 0);
    } else if (subFlags2 & 0x100) {
        return;
    } else if (sub550) {
        SubRoutineSet(this, 0, 0, 0, 0);
    } else if ((subPlStatus & 0x400) && !(subFlags & 8)) {
        SubRoutineSet(this, 0, 0, 0, 0);
    } else if (subPlStatus & 0x40000) {
        SubRoutineSet(this, 0, 0, 0, 0);
    } else if ((subPlStatus & 0x20000) && subDist < 1000.0f && !(subFlags & 8)) {
        SubRoutineSet(this, 0, 0, 0, 0);
    } else {
        switch (actCheck()) {
        case 1:
            SubRoutineSet(this, 0, 8, 0, 0);
            break;
        case 2:
            SubRoutineSet(this, 0, 8, 0, 1);
            break;
        case 3:
            subFlags2 |= 0x80;
            SubRoutineSet(this, 0, 0x12, 0, 0);
            break;
        case 4:
            SubRoutineSet(this, 0, 0x14, 0, 0);
            break;
        case 5:
        case 6:
            break;
        case 7:
            SubRoutineSet(this, 0, 2, 0, 0);
            break;
        case 8:
            SubRoutineSet(this, 0, 0xA, 0, 0);
            break;
        case 9:
            SubRoutineSet(this, 0, 0xC, 0, 0);
            break;
        case 0xB:
            SubRoutineSet(this, 0, 0x13, 0, 0);
            break;
        }
    }
}

int cSubChar::readyOkCheck()
{
    if (!(subPlStatus & 0x10)) {
        return 0;
    }
    if (GetDistance(&pPL->pos, &pos) > 302500.0f) {
        return 0;
    }
    return SatMgr.hitCheck(&pPL->pParts->worldPos, &pParts->worldPos, 0, 0, 0, 0) == 0;
}

// Routine 0 / 2 (and 5): stand behind the aiming player, cover the ears.
void cSubChar::moveBehind()
{
    const Vec* ap;

    switch (PlGetWeaponNo()) {
    case 0xD:
    case 0x13:
    case 0x16:
    case 0x17:
    case 0x19:
    case 0x1F:
    case 0x20:
        ap = &atckPos2;
        break;
    default:
        ap = &atckPos;
        break;
    }
    if (xFE <= 0x13 && (subPlStatus & 0x10)) {
        subOfs = *ap;
        subOfs.z -= pPL->pWep->getAngle() * 0.31830987f * 300.0f;
        PSMTXMultVec(pPL->mat, &subOfs, &subOfs);
        pos.x = pos.x * 0.7f + subOfs.x * 0.3f;
        pos.z = pos.z * 0.7f + subOfs.z * 0.3f;
        rot.y += Muku2(rot.y, pPL->rot.y, 0.31415927f);
    }
    if (!(subPlStatus & 0x10)) {
        subHideMode = 1;
    }
    switch (xFE) {
    case 0:
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x1D), 0, 3, 5, 0);
        atari.flags &= 0xFCFF;
        subHideMode = 0;
        xFE = 1;
    case 1:
        if (!(subPlStatus & 0x10)) {
            xFE = 0x14;
        }
        if (motionMove()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x1E), 0, 3, 5, 0);
            xFE = 2;
        }
        break;
    case 2:
        motionMove();
        if (pG->flags_5010 & 0x20000) {
            pG->flags_5010 &= ~0x20000;
            if (!(subPlStatus & 0x2080)) {
                xFE = 0xA;
            }
        }
        if (subHideMode) {
            xFE = 0x14;
        }
        break;
    case 0xA:
        setHand(1);
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x20), 0, 7, 5, 0);
        xFE = 0xB;
    case 0xB:
        if ((subPlStatus & 0x2080) || motionMove()) {
            setHand(0);
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x1E), 0, 3, 5, 0);
            motionMove();
            xFE = 2;
        }
        break;
    case 0x14:
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x1F), 0, 3, 5, 0);
        pG->flags_5010 &= ~0x20000;
        atari.flags |= 0x300;
        inSat();
        atari.set(-10, 300.0f, 200.0f);
        SubRoutineSet(this, 0, 0, 0x28, 0);
        break;
    }
}

// Routine 0 / 4: crouch (kagamu) while the player fires over her.
void cSubChar::moveKagamu()
{
    switch (subSelf->xFE) {
    case 0:
        if (mot_ck()) {
            subSelf->motionSet(SUB_MOT(subSelf, 0x12), 10, 0, 1, 0);
        } else {
            subSelf->motionSet(SUB_MOT(subSelf, 0x6E), 10, 0, 1, 0);
        }
        subSelf->xFF = 0;
        subSelf->xFE = 1;
    case 1:
        subSelf->motionMove();
        subSelf->xFF++;
        if (subSelf->xFF > 5) {
            subSelf->xFE = 2;
        }
        break;
    case 2:
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x43), 0, 1, 5, 0);
        subSelf->xFE = 3;
        sub424 = subSelf->pos.y;
        subSelf->atari.flags &= 0xFDFF;
    case 3:
        if (subSelf->frame > 7.7f && subSelf->frame < 8.3f) {
            subFlags2 |= 0x20;
        }
        if (subSelf->motionMove()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x44), 0, 3, 5, 0);
            subSelf->xFE = 4;
        }
        break;
    case 4:
        subSelf->motionMove();
        if (Key.on & 0x810) {
            subSelf->subHideMode = 0;
        } else {
            subSelf->subHideMode++;
            if (subSelf->subHideMode > 30) {
                MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x45), 0, 3, 5, 0);
                subSelf->atari.flags |= 0x200;
                subSelf->xFE = 5;
            }
        }
        break;
    case 5:
        if (subSelf->frame > 11.7f && subSelf->frame < 12.3f) {
            subFlags2 &= ~0x20;
        }
        if (subSelf->motionMove()) {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Routine 0 / 6: the "pants" reaction (the player looks up her skirt).
void cSubChar::movePants()
{
    switch (subSelf->xFE) {
    case 0:
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x21), 0, 7, 5, 0);
        subSndId = SndCall(8, 0x16, &subSelf->pParts->worldPos, id, 0, 0);
        subSelf->xFE = 1;
    case 1:
        if (MotionMove(subSelf)) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x22), 0, 7, 5, 0);
            subSelf->xFE = 2;
        }
        rot.y += Muku(&pos, &pPL->pos, rot.y, 0.31415927f);
        break;
    case 2:
        MotionMove(subSelf);
        rot.y += Muku(&pos, &pPL->pos, rot.y, 0.31415927f);
        if (!(subFlags2 & 4)) {
            subSelf->xFE = 3;
            subSelf->xFF = 0;
        }
        break;
    case 3:
        MotionMove(subSelf);
        rot.y += Muku(&pos, &pPL->pos, rot.y, 0.31415927f);
        if (subFlags2 & 4) {
            subSelf->xFE = 2;
        } else {
            subSelf->xFF++;
            if (subSelf->xFF > 30) {
                MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x23), 0, 7, 5, 0);
                subSelf->xFE = 4;
            }
        }
        break;
    case 4:
        if (MotionMove(subSelf)) {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Routine 0 / 7: crouch down (the player's "wait" command).
void cSubChar::moveDown()
{
    switch (xFE) {
    case 0:
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x43), 0, 7, 5, 0);
        subFlags2 |= 0x20;
        subSelf->xFE = 1;
    case 1:
        if (motionMove()) {
            subSelf->xFE = 2;
        }
        break;
    case 2:
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x44), 0, 7, 1, 0);
        xFE = 3;
    case 3:
        if ((subFlags2 & 1) || (pG->flags_5010 & 0x8000)) {
            xFF = 40;
        } else if (xFF) {
            xFF--;
        }
        if (xFF == 0) {
            MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x45), 0, 7, 5, 0);
            xFE = 4;
        }
        motionMove();
        break;
    case 4:
        if (motionMove()) {
            subFlags2 &= ~0x20;
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Point 300 behind the scenario wall (of attribute `attr`) 1000 ahead of the partner: position and
// facing angle for the fence / window actions. 1 when found.
int cSubChar::getScrActionPoint(Vec* opos, Vec* orot, u32 attr)
{
    Vec a = { 0.0f, 400.0f, 0.0f };
    Vec b = { 0.0f, 400.0f, 1000.0f };
    Vec nrm;
    Vec hit;
    u32 r;

    PSVECAdd(&a, &pos, &a);
    PSMTXMultVec(mat, &b, &b);
    r = SatMgr.hitCheck(&a, &b, 0, &nrm, 0, 0);
    if (!(r & 0x01000000) || !(r & attr)) {
        return 0;
    }
    PSVECScale(&nrm, &b, -1000.0f);
    PSVECAdd(&b, &pos, &b);
    b.y += 400.0f;
    r = SatMgr.hitCheck(&a, &b, &hit, &nrm, 0, 0);
    if (!(r & 0x01000000) || !(r & attr)) {
        return 0;
    }
    PSVECScale(&nrm, &b, 300.0f);
    PSVECAdd(&b, &hit, &b);
    b.y = pos.y;
    *opos = b;
    orot->x = 0.0f;
    orot->z = 0.0f;
    orot->y = atan2(-nrm.x, -nrm.z);
    return 1;
}

// Routine 0 / 8: climb over a fence.
void cSubChar::moveFance()
{
    switch (subSelf->xFE) {
    case 0: {
        Vec p;
        Vec r;

        if (subSelf->xFF == 0) {
            if (!getScrActionPoint(&p, &r, 0x20)) {
                SubRoutineSet(this, 0, 0, 0, 0);
                break;
            }
        } else {
            p = pos;
            r.y = sub52C;
            r.z = 0.0f;
            r.x = 0.0f;
        }
        SUB_MOTBASE(this)->set((cMotModel*) this, &p, &r, 10);
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x2C), SUB_MOT(subSelf, 0x67), 7, 5, 0);
        subSelf->atari.flags &= 0xFEFF;
        atari.setPriority(2);
        subSelf->dmg.set(0, 0x80);
        subSelf->cCoord::matUpdate();
        subSelf->xFE = 1;
    }
    case 1:
        pG->flags_5010 |= 8;
        if (subSelf->motionMove()) {
            subSelf->atari.flags |= 0x100;
            atari.setPriority(0);
            subSelf->dmg.clear();
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Floor under the partner (2000 up): 1 when found, pos.y moved onto it.
int cSubChar::landCheck()
{
    Vec a = { 0.0f, 2000.0f, 0.0f };
    Vec hit;
    Vec nrm;

    PSVECAdd(&a, &pos, &a);
    if (!SatMgr.hitCheck(&a, &pos, &hit, &nrm, 0, 0)) {
        return 0;
    }
    pos.y = hit.y;
    return 1;
}

// Player damage handlers while the partner drops down a ledge (SetPlDamage).
void pl_fall_ok0()
{
}

void pl_fall_ok()
{
    cPlayer* pl = pPL;
    PlArc* arc;

    arc = ((cEm*) pl->dmgType)->subArc;
    pl->subArc = arc;
    switch (pl->xFD) {
    case 0:
        MOT_SET(pl, MOTION(pl), PL_ARC_PTR(arc, 0x3E), PL_ARC_PTR(arc, 0x68), 0, 5, 0);
        pl->pWep->setTrans(0, 0);
        pl->xFD = 1;
    case 1:
        if (pl->motionMove()) {
            pl->pWep->setTrans(1, 0);
            pl->endAction(0);
            EndPlDamage();
        }
        break;
    }
    pl->subArc = pl->subArc2;
}

// Routine 0 / 9: the player lets her down a ledge.
void cSubChar::moveFall()
{
    switch (xFE) {
    case 0:
        getFallPos(this, &pPL->pos, &pPL->rot);
        FSet(pPL->rot.y, pPL->rot.y + 4.712389f);
        FSet(pPL->rot.y, LIMIT_ANGLE(pPL->rot.y));
        pPL->cCoord::matUpdate();
        SetPlDamage((int) this, (void (*)(cPlayer*)) pl_fall_ok0);
        pPL->dmg.set(0, 0x80);
        if (subFlags2 & 0x80) {
            motionSet(SUB_MOT(subSelf, 0x46), 3, 0, 0x201, 0);
        } else {
            motionSet(SUB_MOT(subSelf, 0x47), 3, 0, 0x201, 0);
        }
        subFlags2 &= ~0x80;
        atari.flags &= 0xFCFF;
        dmg.set(0, 0x80);
        sub580 = 0;
        subFlags |= 0x20;
        xFE = 1;
    case 1:
        motionMove();
        if (subSelf->frame >= 41.0f) {
            xFE = 4;
        }
        break;
    case 4:
        pPL->atari.flags &= 0xFCFF;
        atari.flags |= 0x200;
        atari.flags &= 0xFEFF;
        atari.setPriority(3);
        rot.y = pPL->rot.y - 4.712389f;
        rot.y = LIMIT_ANGLE(rot.y);
        {
            static const Vec v_ok = { -501.4f, 3200.0f, 106.0f };
            PSMTXMultVec(pPL->mat, &v_ok, &pos);
        }
        setPos(&pos);
        SetPlDamage((int) this, (void (*)(cPlayer*)) pl_fall_ok);
        pPL->dmg.set(0, 0x80);
        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x3F), SUB_MOT(subSelf, 0x5F), 7, 5, 0);
        atari.flags &= 0xFEFF;
        xFE = 5;
    case 5:
        if (motionMove()) {
            atari.flags |= 0x300;
            atari.setPriority(0);
            dmg.clear();
            subFlags &= ~0x20;
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Routine 0 / 0xA: one-shot actions selected by subHideMode (jump down, climb, registered motion).
void cSubChar::moveAction()
{
    void* m = 0;
    void* seq;

    switch (xFE) {
    case 0:
        switch (subHideMode) {
        default:
            pLog->err(0, 0, "ASHLEY UNKNOWN ACTION %d", subHideMode);
        case 0:
            m = SUB_MOT(subSelf, 0x1B);
            seq = SUB_MOT(subSelf, 0x54);
            break;
        case 1:
            m = SUB_MOT(subSelf, 0x1C);
            seq = SUB_MOT(subSelf, 0x55);
            break;
        case 2:
            m = subSelf->subMot0;
            seq = subSelf->subMot1;
            break;
        case 3:
            m = SUB_MOT(subSelf, 0x47);
            seq = SUB_MOT(subSelf, 0x7C);
            break;
        case 4:
            m = SUB_MOT(subSelf, 0x48);
            seq = SUB_MOT(subSelf, 0x5C);
            break;
        }
        switch (subHideMode) {
        case 3:
            xFE = 2;
            break;
        case 4:
            xFE = 4;
            sub52C = getJumpAdjY();
            break;
        default:
            xFE = 1;
            break;
        }
        if (subHideMode == 4) {
            subSndId = SndCall(8, 0x12, &subSelf->pParts->worldPos, id, 0, 0);
        }
        MOT_SET(subSelf, MOTION(subSelf), m, seq, 3, 0x101, 0);
        atari.flags &= 0xFCFF;
        dmg.set(0, 0x80);
    case 1:
        pG->flags_5010 |= 8;
        if (motionMove()) {
            atari.flags |= 0x300;
            dmg.clear();
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    case 2:
        pG->flags_5010 |= 8;
        motionMove();
        if (subSelf->frame >= 40.0f && landCheck()) {
            atari.flags |= 0x300;
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x2D), SUB_MOT(subSelf, 0x5A), 0, 0x101, 0);
            motionMove();
            xFE = 3;
        }
        break;
    case 3:
        pG->flags_5010 |= 8;
        if (motionMove()) {
            atari.flags |= 0x300;
            dmg.clear();
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    case 4:
        pG->flags_5010 |= 8;
        if (subSelf->frame <= 9.0f) {
            jumpAdjust();
        }
        if (subSelf->frame >= 35.0f && subSelf->frame <= 44.0f) {
            pos.y += sub52C * 0.1f;
        }
        if (motionMove()) {
            atari.flags |= 0x300;
            dmg.clear();
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Routine 0 / 0xC: climb a ladder (subHideMode: 0 up, 1 down; subX534 rungs).
void cSubChar::moveLadder()
{
    void* m;
    void* seq;

    pG->flags_5010 |= 8;
    switch (xFE) {
    case 0:
        if (subHideMode) {
            m = SUB_MOT(subSelf, 0x4C);
            seq = SUB_MOT(subSelf, 0x64);
        } else {
            m = SUB_MOT(subSelf, 0x49);
            seq = SUB_MOT(subSelf, 0x61);
        }
        MOT_SET(subSelf, MOTION(subSelf), m, seq, 3, 5, 0);
        xFE = 1;
        subFlags |= 0x20;
    case 1:
        if (motionMove()) {
            xFE = 2;
        }
        break;
    case 2:
        if (subHideMode) {
            m = SUB_MOT(subSelf, 0x4D);
            seq = SUB_MOT(subSelf, 0x65);
        } else {
            m = SUB_MOT(subSelf, 0x4A);
            seq = SUB_MOT(subSelf, 0x62);
        }
        MOT_SET(subSelf, MOTION(subSelf), m, seq, 3, 5, 0);
        xFE = 3;
    case 3:
        if (motionMove()) {
            subX534--;
            if (subX534 <= 0) {
                xFE = 4;
            }
        }
        break;
    case 4:
        if (subHideMode) {
            m = SUB_MOT(subSelf, 0x4E);
            seq = SUB_MOT(subSelf, 0x66);
        } else {
            m = SUB_MOT(subSelf, 0x4B);
            seq = SUB_MOT(subSelf, 0x63);
        }
        MOT_SET(subSelf, MOTION(subSelf), m, seq, 3, 5, 0);
        xFE = 5;
    case 5:
        if (motionMove()) {
            atari.flags |= 0x300;
            dmg.clear();
            subFlags &= ~0x20;
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Height difference to the floor 3800 behind the action wall (0 when more than 1500 away).
f32 cSubChar::getJumpAdjY()
{
    Vec v;
    f32 h;

    v.x = -sub448.x;
    v.y = sub448.y;
    v.z = -sub448.z;
    PSVECScale(&v, &v, 3800.0f);
    PSVECAdd(&v, &sub43C, &v);
    v.y += 1500.0f;
    h = SatMgr.getFloor(&v, 600.0f, 100000.0f, 0, 0);
    h -= pos.y;
    if (fabsf(h) > 1500.0f) {
        h = 0.0f;
    }
    return h;
}

// Slide the partner sideways off the posts while she jumps over a wall.
void cSubChar::jumpAdjust()
{
    Vec a;
    Vec b;

    a.x = 0.0f;
    a.y = 0.0f;
    a.z = 300.0f;
    PSMTXMultVec(mat, &a, &a);
    b.x = 0.0f;
    b.y = 0.0f;
    b.z = 800.0f;
    PSMTXMultVec(mat, &b, &b);
    if (!(SatMgr.hitCheck(&a, &b, 0, 0, 0, 0) & 0x80000)) {
        a.x = -35.0f;
        a.y = 300.0f;
        a.z = 300.0f;
        PSMTXMultVecSR(mat, &a, &a);
        PSVECAdd(&pos, &a, &pos);
    }
    a.x = -300.0f;
    a.y = 0.0f;
    a.z = 300.0f;
    PSMTXMultVec(mat, &a, &a);
    b.x = -300.0f;
    b.y = 0.0f;
    b.z = 800.0f;
    PSMTXMultVec(mat, &b, &b);
    if (!(SatMgr.hitCheck(&a, &b, 0, 0, 0, 0) & 0x80000)) {
        a.x = 35.0f;
        a.y = 300.0f;
        a.z = 300.0f;
        PSMTXMultVecSR(mat, &a, &a);
        PSVECAdd(&pos, &a, &pos);
    }
}

// Routine 0 / 0xE: turn back towards the player (the "come back" reaction).
void cSubChar::moveBack()
{
    switch (xFE) {
    case 0:
        if (mot_ck()) {
            motionSet(SUB_MOT(subSelf, 0x14), 8, 0, 1, 0);
        } else {
            motionSet(SUB_MOT(subSelf, 0x70), 8, 0, 1, 0);
        }
        xFF = 0;
        xFE = 1;
    case 1:
        motionMove();
        xFF++;
        if (xFF > 30) {
            xFE = 2;
        }
        rot.y += Muku(&pos, &pPL->pos, rot.y, 0.31415927f);
        break;
    case 2:
        if (mot_ck()) {
            motionSet(SUB_MOT(subSelf, 0x12), 8, 0, 1, 0);
        } else {
            motionSet(SUB_MOT(subSelf, 0x6E), 8, 0, 1, 0);
        }
        xFE = 3;
        break;
    case 3:
        motionMove();
        break;
    }
    if (!(pG->flags_5010 & 0x8000)) {
        SubRoutineSet(this, 0, 0, 0, 0);
    }
}

// Routine 0 / 0xF: run the scenario's own handler (SetSubAux).
void cSubChar::moveAux()
{
    ((void (*)()) subAux0)();
}

// Routine 0 / 0x10: run to the hide spot (SubCharCtrlHide) and duck / climb into it.
void cSubChar::moveHide()
{
    static Vec norm;
    static Vec vdz0 = { 1.0f, 0.0f, 0.0f };
    f32 ang;

    switch (xFE) {
    case 0: {
        void* m;
        void* seq;
        u16 hokan = 4;

        subFlags &= ~2;
        subHidePos.y += 300.0f;
        ang = Muku(&pos, &subHidePos, rot.y, 3.1415927f);
        if (fabsf(ang) > 2.0943952f) {
            if (mot_ck()) {
                m = SUB_MOT(subSelf, 0x1A);
                seq = SUB_MOT(subSelf, 0x53);
            } else {
                m = SUB_MOT(subSelf, 0x72);
                seq = 0;
            }
            xFE = 2;
        } else if (fabsf(ang) > 1.0471976f) {
            if (mot_ck()) {
                m = SUB_MOT(subSelf, 0x16);
                seq = SUB_MOT(subSelf, 0x52);
            } else {
                m = SUB_MOT(subSelf, 0x71);
                seq = 0;
            }
            xFE = 1;
        } else {
            if (mot_ck()) {
                m = SUB_MOT(subSelf, 0x16);
                seq = SUB_MOT(subSelf, 0x52);
            } else {
                m = SUB_MOT(subSelf, 0x71);
                seq = 0;
            }
            xFE = 5;
        }
        MOT_SET(this, MOTION(this), m, seq, 7, hokan, 0);
        motionMove();
        xFE = 1;
        break;
    }
    case 1:
        ang = Muku(&pos, &subHidePos, rot.y, 0.62831855f);
        subSelf->rot.y += ang;
        if (fabsf(ang) < 0.44879895f) {
            xFE = 5;
        }
        motionMove();
        break;
    case 2:
        if (motionMove()) {
            xFE = 5;
        }
        break;
    case 5:
        if (mot_ck()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x16), SUB_MOT(subSelf, 0x52), 7, 5, 0);
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x71), 0, 7, 5, 0);
        }
        xFE = 6;
        break;
    case 6: {
        Vec a;
        Vec b;
        Vec hit;
        u32 r;

        a.x = pos.x;
        a.y = subHidePos.y;
        a.z = pos.z;
        b = subHidePos;
        r = SatMgr.hitCheck(&a, &b, &hit, &norm, 0, 0);
        if ((r & 8) && GetDistance(a, hit) < 250000.0f) {
            subX534 = 0;
            subHidePos = hit;
            switch (subHideMode) {
            case 1:
            default:
                xFE = 0xA;
                break;
            case 2:
                xFE = 0x14;
                break;
            }
        }
        RouteCkToPos(this, &b, &a, (subHidePos.y > pos.y + 1000.0f) | 2, &subX5C8);
        subSelf->rot.y += Muku(&pos, &a, rot.y, 0.20943952f);
        motionMove();
        break;
    }
    case 0xA: {
        Vec v;

        MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x2C), SUB_MOT(subSelf, 0x67), 7, 5, 0);
        rot.y = atan2f(-norm.x, -norm.z);
        cCoord::matUpdate();
        PSVECScale(&norm, &v, 400.0f);
        PSVECAdd(&v, &subHidePos, &v);
        pos.z = v.z;
        pos.x = v.x;
        atari.flags |= 0x200;
        subSelf->atari.setPriority(3);
        atari.flags &= 0xFEFF;
        dmg.set(0, 0x80);
        sub52C = getAdjustX(8) * 0.1f;
        sub538 = 10;
        xFE = 0xB;
    }
    case 0xB:
        if (sub52C != 0.0f && sub538) {
            Vec v;

            PSMTXMultVecSR(subSelf->mat, &vdz0, &v);
            PSVECScale(&v, &v, sub52C);
            PSVECSubtract(&pos, &v, &pos);
            sub538--;
        }
        if (motionMove()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x43), 0, 7, 1, 0);
            pG->flags_500C |= 0x800;
            xFE = 0xC;
            sub538 = 6;
        }
        break;
    case 0xC:
        if (motionMove()) {
            xFE = 0xD;
            xFF = 10;
        }
        break;
    case 0xD:
        motionMove();
        xFF--;
        if (xFF == 0) {
            rot.y = LIMIT_ANGLE(rot.y + 3.1415927f);
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x44), 0, 7, 5, 0);
            xFE = 0xE;
        }
        break;
    case 0xE:
        motionMove();
        if (subX534) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x45), 0, 7, 5, 0);
            xFE = 0xF;
            subX534 = frameMax - 3;
        }
        break;
    case 0xF:
        if (subX534 > 0) {
            Vec v;

            v.y = 0.0f;
            v.x = 0.0f;
            v.z = 200.0f / (f32) (frameMax - 3);
            PSMTXMultVecSR(mat, &v, &v);
            PSVECAdd(&pos, &v, &pos);
            subX534--;
        }
        if (motionMove()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x2C), SUB_MOT(subSelf, 0x67), 7, 5, 0);
            xFE = 0x10;
        }
        break;
    case 0x10:
        if (motionMove()) {
            pG->flags_500C &= ~0x800;
            subSelf->atari.setPriority(0);
            atari.flags |= 0x300;
            dmg.clear();
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Routine 0 / 0x11: stoop while the player is down.
void cSubChar::moveStoop()
{
    if (!plDownCheck()) {
        subHideMode = 1;
    }
    switch (xFE) {
    case 0:
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x27), 0, 7, 5, 0);
        subHideMode = 0;
        xFE = 1;
    case 1:
        if (motionMove()) {
            MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x28), 0, 7, 5, 0);
            xFE = 2;
            subFlags2 |= 0x20;
        }
        break;
    case 2:
        if (subHideMode && !(subFlags & 2)) {
            xFE = 3;
        }
        motionMove();
        break;
    case 3:
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x29), 0, 7, 5, 0);
        xFE = 4;
        subFlags2 &= ~0x20;
        break;
    case 4:
        if (motionMove()) {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Routine 0 / 0x12: wait on the ledge for the player to catch her.
void cSubChar::moveFallWait()
{
    Vec v;
    f32 ang;

    switch (xFE) {
    case 0:
        sub540 = 0;
        sub53C = 0;
        if (mot_ck()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x13), SUB_MOT(subSelf, 0x50), 7, 5, 0);
            backCheckSet(SUB_MOT(subSelf, 0x15));
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x6F), 0, 7, 5, 0);
            backCheckSet(0);
        }
        subX534 = 0;
        if (sub580 == 0) {
            sub564 = atan2(sub448.x, sub448.z) + 3.1415927f;
            sub564 = LIMIT_ANGLE(sub564);
        }
        subHideMode = 5;
        xFE = 1;
    case 1:
        motionMove();
        ang = Muku2(subSelf->rot.y, sub564, 0.31415927f);
        subSelf->rot.y += ang;
        if (fabsf(ang) < 0.15707964f) {
            subHideMode--;
            if (subHideMode <= 0) {
                if ((subFlags2 & 0x80) || (checkSatAttr(500.0f) & 0x100010)) {
                    subHideMode = 0;
                    xFE = 2;
                } else {
                    xFE = 4;
                }
            }
        }
        break;
    case 2:
        if (mot_ck()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x12), SUB_MOT(subSelf, 0x4F), 7, 5, 0);
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x6E), 0, 7, 5, 0);
        }
        sub538 = 0;
        xFE = 3;
    case 3:
        motionMove();
        break;
    case 4:
        if (mot_ck()) {
            motionSet(SUB_MOT(subSelf, 0x14), 8, 0, 1, 0);
        } else {
            motionSet(SUB_MOT(subSelf, 0x70), 8, 0, 1, 0);
        }
        xFE = 5;
    case 5:
        if (motionMove()) {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 1300.0f;
    PSMTXMultVec(mat, &v, &v);
    v.y = SatMgr.getFloor(&v, 600.0f, 100000.0f, 0, 0) + 300.0f;
    if (GetDistance(&v, &pPL->pos) > 25000000.0f) {
        sub540++;
        if (sub540 == 300) {
            subSndId = SndCall(8, 0x14, &subSelf->pParts->worldPos, id, 0, 0);
            sub540 = 0;
        }
    } else if (sub540 <= 0x95) {
        sub540++;
    }
    if (subX534 == 0) {
        if (subHideMode == 0 && (pPL->stat & 0xFFFF0000) != 0x000E0000) {
            subX534 = 1;
        }
    }
    if (subX534) {
        if (GetDistance(&v, &pPL->pos) < 9000000.0f && !SatMgr.hitCheck(&v, &pPL->pParts->worldPos, 0, 0, 0, 0) &&
            !SatMgr.hitCheck(&pPL->pParts->worldPos, &v, 0, 0, 0, 0)) {
            ActBtn.set(0x1C, 6, (int) catchOn, 0, 0, 1, 0, 0);
            sub53C = 1;
        }
    }
    if (!SatMgr.hitCheck(&pParts->worldPos, &pPL->pParts->worldPos, 0, 0, 0, 0)) {
        subFlags2 &= ~0x80;
        SubRoutineSet(this, 0, 0, 0, 0);
    }
}

// Routine 0 / 0x13: wait at the ladder until the player is in sight again.
void cSubChar::moveLadderWait()
{
    switch (xFE) {
    case 0:
        if (mot_ck()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x19), 0, 7, 5, 0);
            xFE = 2;
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x6E), 0, 7, 5, 0);
            xFE = 1;
        }
        break;
    case 1:
        motionMove();
        break;
    case 2:
        motionMove();
        break;
    }
    if (!SatMgr.hitCheck(&pParts->worldPos, &pPL->pParts->worldPos, 0, 0, 0, 0) &&
        !SatMgr.hitCheck(&pPL->pParts->worldPos, &pParts->worldPos, 0, 0, 0, 0)) {
        SubRoutineSet(this, 0, 0, 0, 0);
    }
}

// Routine 0 / 0x14: wait at the window until the player is in sight again.
void cSubChar::moveWindowWait()
{
    switch (xFE) {
    case 0:
        if (mot_ck()) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x19), 0, 7, 5, 0);
            xFE = 2;
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x6E), 0, 7, 5, 0);
            xFE = 1;
        }
        break;
    case 1:
        motionMove();
        break;
    case 2:
        motionMove();
        break;
    }
    if (!SatMgr.hitCheck(&pParts->worldPos, &pPL->pParts->worldPos, 0, 0, 0, 0) &&
        !SatMgr.hitCheck(&pPL->pParts->worldPos, &pParts->worldPos, 0, 0, 0, 0)) {
        SubRoutineSet(this, 0, 0, 0, 0);
    }
}

// Scenario attribute of the wall `len` ahead (300 up).
u32 cSubChar::checkSatAttr(f32 len)
{
    Vec a;
    Vec b;

    a = pos;
    b.z = len;
    a.y += 300.0f;
    b.x = 0.0f;
    b.y = 300.0f;
    PSMTXMultVec(mat, &b, &b);
    return SatMgr.hitCheck(&a, &b, 0, 0, 0, 0);
}

// Sideways offset (in tenths of `width`) to the first wall of attribute `attr` 1000 ahead,
// scanning right then left; 0 when none.
f32 cSubChar::getAdjustX(int attr)
{
    static f32 width = 550.0f;
    static f32 height = 300.0f;
    Vec a;
    Vec b;
    Vec a0;
    Vec b0;
    Vec step;
    int i;

    a.x = 0.0f;
    a.y = height;
    a.z = 0.0f;
    PSMTXMultVec(subSelf->mat, &a, &a);
    a0 = a;
    b.x = 0.0f;
    b.y = height;
    b.z = 1000.0f;
    PSMTXMultVec(subSelf->mat, &b, &b);
    b0 = b;
    step.x = width / 10.0f;
    step.y = 0.0f;
    step.z = 0.0f;
    PSMTXMultVecSR(subSelf->mat, &step, &step);
    for (i = 1; i <= 10; i++) {
        if (SatMgr.hitCheck(&a, &b, 0, 0, 0, 0) & attr) {
            return width - width * (f32) i / 10.0f;
        }
        PSVECAdd(&a, &step, &a);
        PSVECAdd(&b, &step, &b);
    }
    a = a0;
    b = b0;
    for (i = 1; i <= 10; i++) {
        if (SatMgr.hitCheck(&a, &b, 0, 0, 0, 0) & attr) {
            return -width - -width * (f32) i / 10.0f;
        }
        PSVECSubtract(&a, &step, &a);
        PSVECSubtract(&b, &step, &b);
    }
    return 0.0f;
}

// Routine 1: damage reaction (subHideMode selects the motion / effect).
void cSubChar::moveDamage()
{
    void* m = 0;

    if (subAux1) {
        ((void (*)()) subAux1)();
        return;
    }
    switch (xFD) {
    case 0:
        beginDamage();
        atari.flags |= 0x300;
        if (pG->flags_68 & 0x800000) {
            switch (subHideMode) {
            case 7:
                subHideMode = 8;
                break;
            case 9:
                subHideMode = 10;
                break;
            }
        }
        switch (subHideMode) {
        case 0:
        case 2:
        case 4:
            m = SUB_MOT(subSelf, 0x2E);
            break;
        case 1:
        case 3:
        case 5:
            m = SUB_MOT(subSelf, 0x2F);
            break;
        case 6:
            m = 0;
            break;
        case 7:
        case 8:
            m = SUB_MOT(subSelf, 0x32);
            EstSet((int) subSelf, -1, 0, 0, 4, ChkWaterEffectEnable(&pos) ? 6 : 5, 0, 0, (u32) subSelf, 0);
            break;
        case 9:
        case 10:
            m = SUB_MOT(subSelf, 0x31);
            EstSet((int) subSelf, -1, 0, 0, 4, ChkWaterEffectEnable(&pos) ? 8 : 7, 0, 0, (u32) subSelf, 0);
            break;
        case 11:
            atari.flags &= 0xFCFF;
            m = SUB_MOT(subSelf, 0x6A);
            break;
        }
        MOT_SET(subSelf, MOTION(subSelf), m, 0, 3, 1, 0);
        setFace(1);
        subSndId = SndCall(8, 9, &subSelf->pParts->worldPos, id, 0, 0);
        subSelf->xFD = 1;
    case 1:
        if (subSelf->frame >= 10.0f && subHideMode == 11 && landCheck()) {
            atari.flags |= 0x300;
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x6B), 0, 3, 1, 0);
            EstSet((int) subSelf, -1, 0, 0, 4, ChkWaterEffectEnable(&pos) ? 10 : 9, 0, 0, (u32) subSelf, 0);
            xFD = 10;
            if (pG->flags_68 & 0x800000) {
                subHideMode = 8;
                xFD = 5;
            }
        }
        if (subSelf->motionMove()) {
            switch (subHideMode) {
            case 6:
            case 7:
            case 9:
                xFD = 2;
                break;
            case 8:
            case 10:
                xFD = 5;
                break;
            case 11:
                xFD = 7;
                break;
            default:
                setFace(0);
                SubRoutineSet(this, 0, 0, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (MotionMove(subSelf)) {
            if (subHideMode == 7 || subHideMode == 9) {
                hp = 0;
                pG->sub_life = 0;
            }
        }
        break;
    case 5:
        if (subHideMode == 8) {
            m = SUB_MOT(subSelf, 0x33);
        } else {
            m = SUB_MOT(subSelf, 0x34);
        }
        MOT_SET(subSelf, MOTION(subSelf), m, 0, 3, 5, 0);
        xFD = 6;
    case 6:
        if (motionMove()) {
            endDamage();
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    case 7:
        motionMove();
        break;
    case 10:
        motionMove();
        break;
    }
}

// Routine 2: death.
void cSubChar::moveDie()
{
    switch (xFD) {
    case 0:
        CamCtrl.deleteAttachCamera((AttachCamera*) pPL->p2A4, pPL);
        if (subFlags2 & 0x20) {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x30), 0, 3, 1, 0x32);
        } else {
            MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x30), 0, 3, 1, 0);
            EstSet((int) this, -1, 0, 0, 4, ChkWaterEffectEnable(&pos) ? 4 : 3, 0, 0, (u32) this, 0);
        }
        SndCall(8, 0xD, &pParts->worldPos, id, 0, 0);
        atari.partsNo = 4;
        xFD = 1;
    case 1:
        if (motionMove()) {
            atari.flags &= 0xFCFF;
            xFD = 2;
        }
        break;
    default:
        motionMove();
        break;
    }
}

// Routine 3: the bulldozer scenario's own handler (SetSubBulldozer).
void cSubChar::moveBull()
{
    ((void (*)()) subAux0)();
}

// Routine 5: scenario event: walk to subHidePos along the route.
void cSubChar::moveEvent()
{
    f32 ang;

    switch (xFD) {
    case 0:
        MotionMoveF(this, 0);
        break;
    case 1:
        switch (xFE) {
        case 0:
            if (mot_ck()) {
                MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x16), SUB_MOT(subSelf, 0x52), 10, 5, 0);
            } else {
                MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x71), SUB_MOT(subSelf, 0x78), 10, 5, 0);
            }
            xFE = 1;
            motState &= ~1;
        case 1: {
            Vec out;

            RouteCkToPos(this, &subHidePos, &out, subHidePos.y > pos.y + 1000.0f, &subX5C8);
            ang = Muku(&pos, &out, rot.y, 0.44879895f);
            rot.y += ang;
            if (fabsf(ang) < 0.20943952f) {
                if (mot_ck()) {
                    MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x16), SUB_MOT(subSelf, 0x52), 10, 5, 0);
                } else {
                    MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x71), SUB_MOT(subSelf, 0x78), 10, 5, 0);
                }
                xFE = 2;
            }
            break;
        }
        case 2: {
            Vec out;

            RouteCkToPos(this, &subHidePos, &out, subHidePos.y > pos.y + 1000.0f, &subX5C8);
            rot.y += Muku(&pos, &out, rot.y, 0.31415927f);
            if (GetDistance(pos, subHidePos) < 250000.0f) {
                if (mot_ck()) {
                    MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x12), SUB_MOT(subSelf, 0x4F), 7, 5, 0);
                } else {
                    MOT_SET(subSelf, MOTION(subSelf), SUB_MOT(subSelf, 0x6E), 0, 7, 5, 0);
                }
                SubRoutineSet(this, 5, 0, 0, 0);
            }
            break;
        }
        }
        motionMove();
        break;
    }
    partsWorldCalc();
    EmAtCheck(this);
    SatMgr.check(this, 0);
    atari.move();
    PartsWorldPosCalc(this);
    seqSeCtrl();
    moveCloth();
}

// Routine 6: crouch and stay down (the player is dead).
void cSubChar::moveDijection()
{
    switch (xFD) {
    case 0:
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x43), 0, 7, 5, 0);
        xFD = 1;
        break;
    case 1:
        if (motionMove()) {
            xFD = 2;
        }
        break;
    case 2:
        MOT_SET(this, MOTION(this), SUB_MOT(subSelf, 0x44), 0, 7, 5, 0);
        xFE = 3;
        break;
    case 3:
        motionMove();
        break;
    }
}

// Step `spd` towards `target` in the XZ plane.
void cSubChar::movePos(Vec* target, f32 spd)
{
    f32 ang;

    ang = Muku(&pos, target, 0.0f, 3.1415927f);
    Vec v = { 0.0f, 0.0f, 0.0f };
    v.z = spd;
    Vec r = { 0.0f, 0.0f, 0.0f };
    r.y = ang;
    RotVector(&v, &r, &v);
    PSVECAdd(&pos, &v, &pos);
}

void cSubChar::neckInit()
{
    subNeckOn = 0;
    subNeckZ = 0.0f;
    subNeckX = 0.0f;
    subNeckAng = 0.0f;
}

// Turn the neck (parts 3) towards the player while neckSet() keeps asking for it.
void cSubChar::neckCtrl()
{
    cModel* p = getPartsPtr(3);
    int on = 1;
    f32 ang;

    MOTION_PARTS(p)->flags |= 0x40000000;
    if (!(pPL->flags_420 & 2)) {
        on = 0;
    }
    if (subNeckOn == 0 || (blendMot != 0 && blendMot->blendRate != 0.0f) || on) {
        subNeckAng += Muku2(subNeckAng, 0.0f, 0.15707964f);
    } else {
        if (subNeckOn > 0) {
            subNeckOn--;
        }
        ang = Muku(&pos, &pPL->pos, rot.y + subNeckAng, 0.10471976f);
        subNeckAng += ang;
        if (subNeckAng > 0.78539819f) {
            subNeckAng = 0.78539819f;
        } else if (subNeckAng < -0.78539819f) {
            subNeckAng = -0.78539819f;
        }
    }
    p->efmSpd.x = subNeckAng;
    subNeckOn = 0;
}

void cSubChar::neckSet(Vec* pos)
{
    subNeckPos = *pos;
    subNeckOn = 1;
}

// Which action the partner should take this frame (0 none; the routine 0 sub routine selector).
int cSubChar::actCheck()
{
    if (readyOkCheck()) {
        return 7;
    }
    if (fanceCheck()) {
        return 1;
    }
    switch (windowCheck()) {
    case 1:
        return 2;
    case 2:
        return 3;
    case 3:
        return 4;
    }
    if (SubLadderClimbCk(this)) {
        return 5;
    }
    if (doorCheck()) {
        return 6;
    }
    if (actionCheck()) {
        return 8;
    }
    if (ladder2Check()) {
        return 9;
    }
    if (fallLadderCheck()) {
        return 0xB;
    }
    return 0;
}

// subFlags2 bit4: the aiming player has her in front of him within 10000.
int cSubChar::cautionCheck()
{
    int ret;

    if (subFlags & 8) {
        ret = 0;
    } else if (!(subPlStatus & 0x10)) {
        ret = 0;
    } else if (fabsf(GetXZAngleLocal(&pPL->pos, &pos, pPL->rot.y)) > 0.78539819f) {
        ret = 0;
    } else if (GetDistance(pPL->pParts->worldPos, pParts->worldPos) > 100000000.0f) {
        ret = 0;
    } else {
        ret = 1;
    }
    if (ret == 1) {
        subFlags2 |= 0x10;
    } else {
        subFlags2 &= ~0x10;
    }
    return ret;
}

int cSubChar::plDownCheck()
{
    return (subPlStatus & 0x8000) != 0;
}

// A fence (attribute 0x20) 600 ahead with a floor within 300 of the partner's height 1500 behind it.
int cSubChar::fanceCheck()
{
    Vec a = { 0.0f, 400.0f, -300.0f };
    Vec b = { 0.0f, 400.0f, 600.0f };
    Vec hit;
    Vec nrm;

    PSMTXMultVec(subSelf->mat, &a, &a);
    PSMTXMultVec(subSelf->mat, &b, &b);
    if (!(SatMgr.hitCheck(&a, &b, &hit, &nrm, 0, 0) & 0x20)) {
        return 0;
    }
    a.x = 400.0f;
    a.y = 300.0f;
    a.z = 1000.0f;
    b.x = -400.0f;
    b.y = 300.0f;
    b.z = 1000.0f;
    PSMTXMultVec(subSelf->mat, &a, &a);
    PSMTXMultVec(subSelf->mat, &b, &b);
    if (SatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    PSVECScale(&nrm, &a, -1500.0f);
    PSVECAdd(&a, &hit, &a);
    b.x = a.x;
    b.y = a.y - 10000.0f;
    b.z = a.z;
    SatMgr.hitCheck(&a, &b, &hit, 0, 0, 0);
    if (fabsf(hit.y - pos.y) > 300.0f) {
        return 0;
    }
    return 1;
}

// Window 600 ahead: 1 climb through (sub52C = facing angle), 2 break it first, 3 blocked, 0 none.
int cSubChar::windowCheck()
{
    Vec a = { 0.0f, 400.0f, 0.0f };
    Vec b = { 0.0f, 400.0f, 600.0f };
    Vec dir;
    Vec p;
    u16 status;
    cEmWindow* w;

    PSVECAdd(&a, &subSelf->pos, &a);
    PSMTXMultVec(subSelf->mat, &b, &b);
    if (!ChkWindow(this, &a, &b, 1, &status, &dir, &p, &w)) {
        return 0;
    }
    if (!(w->ChkStatus() & 1)) {
        return 3;
    }
    if (!EmRackCk(this, &pos, atan2f(-dir.x, -dir.z))) {
        return 3;
    }
    if (w->ChkBreakDir(&subSelf->pos) == 2) {
        subFlags2 |= 0x80;
        SubRoutineSet(this, 0, 0x12, 0, 0);
        return 2;
    }
    sub52C = atan2(-dir.x, -dir.z);
    return 1;
}

// A ladder to climb down below the target.
int cSubChar::fallLadderCheck()
{
    Vec d;

    if (!SubLadderClimbCk2(this)) {
        return 0;
    }
    PSVECSubtract(&subTarget, &pos, &d);
    if (VecElevation(&d) < 0.78539819f) {
        return 0;
    }
    return 1;
}

// A door in front to open (not while she is being told to wait).
int cSubChar::doorCheck()
{
    cEmDoor* d = DoorOpenCk(this);

    if (d == 0) {
        return 0;
    }
    if (subFlags2 & 2) {
        return 0;
    }
    SubOpenDoorSet(d);
    return 1;
}

int cSubChar::readyCheck()
{
    if (subFlags & 8) {
        return 0;
    }
    if (!(subPlStatus & 0x10)) {
        return 0;
    }
    return 1;
}

// Action button: the player catches the partner waiting on the ledge.
void catchOn()
{
    cSubChar* sub = pSUB;
    Vec p;
    Vec r;

    pPL->dmg.set(0, 0x80);
    sub->dmg.set(0, 0x80);
    SubRoutineSet(sub, 0, 9, 0, 0);
    if (sub->sub580) {
        p = sub->sub558;
        p.y = sub->pos.y;
        if (fabsf(sub->pos.y - sub->sub558.y) < 300.0f && GetDistance(&p, &sub->pos) < 25000000.0f) {
            sub->setPos(&sub->sub558);
            r.y = sub->sub564;
            r.z = 0.0f;
            r.x = 0.0f;
            sub->setAng(&r);
        }
    }
    sub->sub580 = 0;
}

// Wall in front (analyze's sub438 attribute): pick the action (jump down / climb / wait) for it.
int cSubChar::actionCheck()
{
    Vec a;
    Vec b;
    int ret = 0;

    a.x = 0.0f;
    a.y = 300.0f;
    a.z = -300.0f;
    PSMTXMultVec(mat, &a, &a);
    b = pPL->pos;
    b.y += 300.0f;
    if (!SatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    if (sub438 & 0x200000) {
        subSelf->subHideMode = 0;
        ret = 1;
    } else if (sub438 & 0x2000) {
        subSelf->subHideMode = 1;
        ret = 1;
    } else if (sub438 & 0x1000) {
        subSelf->subHideMode = 2;
        ret = 1;
    } else if (sub438 & 0x80000) {
        subSelf->subHideMode = 4;
        ret = 1;
    } else if (sub438 & 0x100010) {
        int up = pPL->pos.y > pos.y - 1000.0f;

        if (up) {
            if (getCliffHeight(atan2(-sub448.x, -sub448.z)) < 2900.0f) {
                subSelf->subHideMode = 3;
                ret = 1;
            } else if (sub580 == 0) {
                xFC = 0;
                xFD = 0x12;
                xFE = 0;
                xFF = 0;
            }
        }
    }
    if (ret == 1) {
        Vec d;

        d.x = -sub448.x;
        d.y = 0.0f;
        d.z = -sub448.z;
        rot.y += Muku3(&d, rot.y, 3.1415927f);
    }
    if (sub580 && subDist <= 300.0f && (pPL->stat & 0xFFFF0000) != 0x000E0000) {
        if (getCliffHeight(sub564) < 2900.0f) {
            sub580 = 0;
            pos = sub558;
            rot.y = sub564;
            subSelf->subHideMode = 3;
            ret = 1;
        } else {
            SubRoutineSet(this, 0, 0x12, 0, 0);
        }
    }
    return ret;
}

// A ladder at the partner's position while the player climbs one (routine 0x10): set her on it.
int cSubChar::ladder2Check()
{
    Vec p;
    u8 level;
    Vec r;
    f32 ang;
    int up;

    if (!SceAtCheckLadder(this, &p, &ang, &level)) {
        return 0;
    }
    if (fabsf(subTarget.y - pos.y) < 1000.0f) {
        return 0;
    }
    if ((pPL->stat & 0xFFFF0000) != 0x00100000) {
        return 0;
    }
    setPos(&p);
    r.y = ang;
    r.z = 0.0f;
    r.x = 0.0f;
    setAng(&r);
    up = 1;
    if ((s8) level <= 0) {
        up = 0;
    }
    subHideMode = up;
    subX534 = ((s8) level < 0 ? -(s8) level : (s8) level) - 2;
    atari.flags &= 0xFCFF;
    dmg.set(0, 0x80);
    return 1;
}

// Drop from the partner's height to the floor 1000 ahead in direction `ang` (100000 when < 800).
f32 cSubChar::getCliffHeight(f32 ang)
{
    Vec v;
    Vec r;
    f32 h;

    r.y = ang;
    v.y = 300.0f;
    r.z = 0.0f;
    v.x = 0.0f;
    r.x = 0.0f;
    v.z = 1000.0f;
    RotVector(&v, &r, &v);
    PSVECAdd(&pos, &v, &v);
    h = pos.y - SatMgr.getFloor(&v, 600.0f, 100000.0f, 0, 0);
    if (h < 800.0f) {
        h = 100000.0f;
    }
    return h;
}

// subFlags2 bit2: the player's head is close below her and looking up.
void cSubChar::pantsCheck()
{
    Vec d;
    Vec* hp;
    Vec* sp;

    subFlags2 &= ~4;
    if (pG->costume2 == 1) {
        return;
    }
    hp = &pPL->getPartsPtr(4)->worldPos;
    sp = &pParts->worldPos;
    if (GetDistance(hp, sp) > 25000000.0f) {
        return;
    }
    PSVECSubtract(sp, hp, &d);
    if (VecElevation(&d) < 0.78539819f) {
        return;
    }
    if (fabsf(Muku(hp, sp, pPL->rot.y, 6.2831855f)) > 0.78539819f) {
        return;
    }
    if (EatMgr.hitCheck(hp, sp, 0, 0, 0, 0)) {
        return;
    }
    subFlags2 |= 4;
}

int cSubChar::ckPlRun()
{
    return (subPlStatus & 8) != 0;
}

// Motion sequence sound (seNo) -> SndCall: footsteps by surface kind (seFlags28B), voices as is.
void cSubChar::seqSeCtrl()
{
    int no;
    int parts;
    int blk;

    if (seNo == 0) {
        return;
    }
    no = seNo - 1;
    parts = 0;
    blk = 5;
    switch (seFlags28B & 3) {
    case 0:
        switch (no) {
        case 0:
        case 2:
            parts = 0x14;
            blk = 5;
            no += 7;
            break;
        case 1:
        case 3:
            parts = 0x18;
            blk = 5;
            no += 7;
            break;
        case 4:
        case 5:
            no += 7;
            break;
        case 6:
            break;
        case 0x16:
            parts = 0x14;
            break;
        case 0x17:
            parts = 0x18;
            break;
        default:
            parts = 0;
            blk = 8;
            break;
        }
        break;
    case 1:
        blk = 8;
        break;
    case 2:
        blk = 6;
        break;
    case 3:
        blk = 1;
        break;
    }
    SndCall(blk, no, &getPartsPtr(parts)->worldPos, id, 0, 0);
    seNo = 0;
}

// Blend the look-back motion `mot` in (NULL: off).
void cSubChar::backCheckSet(void* mot)
{
    if (mot) {
        f32 rate = subBackMot.blendRate;

        MOT_SET(this, &subBackMot, mot, 0, 3, 4, 0);
        subBackMot.blendRate = rate;
        subBackMot.flags2 |= 0x80000000;
        blendMot = &subBackMot;
    } else {
        blendMot = 0;
        subBackMot.blendRate = 0.0f;
    }
}

// Fade the look-back blend in (sub404 == 2) or out (1).
void cSubChar::backCheckMove()
{
    MotionWorkSub* w = subSelf->blendMot;

    if (w == 0) {
        return;
    }
    switch (sub404) {
    case 1:
        if (w->blendRate > 0.0f) {
            w->blendRate -= 0.14f;
            if (subSelf->blendMot->blendRate < 0.0f) {
                subSelf->blendMot->blendRate = 0.0f;
                sub404 = 0;
            }
        }
        break;
    case 2:
        if (blendMot->blendRate < 1.0f) {
            blendMot->blendRate += 0.14f;
            if (blendMot->blendRate > 1.0f) {
                blendMot->blendRate = 1.0f;
                sub404 = 0;
            }
        }
        break;
    }
}

// Look back at an enemy behind her now and then (standing).
void cSubChar::backCheckCtrlFootwork()
{
    switch (sub405) {
    case 0:
        if (checkBackEm()) {
            sub404 = 2;
            sub406 = (u8) (Rnd() >> 2) + 30;
            sub405 = 1;
        }
        break;
    case 1:
        sub406--;
        if (sub406 & 0x8000) {
            if (!checkBackEm()) {
                sub404 = 1;
                sub405 = 0;
            }
            sub406 = (u8) (Rnd() >> 2) + 30;
        }
        break;
    }
}

// Look back at an enemy behind her now and then (walking).
void cSubChar::backCheckCtrlMove()
{
    switch (sub405) {
    case 0:
        sub406--;
        if (sub406 & 0x8000) {
            if (checkBackEm()) {
                sub404 = 2;
                sub406 = (u8) (Rnd() >> 2) + 30;
                sub405 = 1;
            }
        }
        break;
    case 1:
        sub406--;
        if (sub406 & 0x8000) {
            if (!checkBackEm()) {
                sub404 = 1;
                sub405 = 0;
            }
            sub406 = (u8) (Rnd() >> 2) + 30;
        }
        break;
    }
}

// An alive, non-battle enemy behind her (within 20000, in the back cone) with a clear line of sight.
int cSubChar::checkBackEm()
{
    int i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* em = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        f32 d;
        f32 ang;

        if (!em->isAlive()) {
            continue;
        }
        if (em->hp <= 0) {
            continue;
        }
        if (em == pPL) {
            continue;
        }
        if (em == pSUB) {
            continue;
        }
        if (em->checkStatus(1)) {
            continue;
        }
        d = GetDistance(&pos, &em->pos);
        if (d > 400000000.0f) {
            continue;
        }
        ang = GetXZAngleLocal(&pos, &em->pos, rot.y);
        if (d < 25000000.0f) {
            if (!(ang >= 2.3561945f) && ang > -2.3561945f) {
                continue;
            }
        } else {
            if (!(ang >= 2.617994f) && ang > -2.617994f) {
                continue;
            }
        }
        if (SatMgr.hitCheck(&pParts->worldPos, &em->pParts->worldPos, 0, 0, 0, 0)) {
            continue;
        }
        return 1;
    }
    return 0;
}

// The damage routine handler (routine 4): pl_sub's SetSubDamage passes it in r4.
void cSubChar::setEmFunc()
{
    register void (*func)() asm("r4");

    subFunc = func;
}

// Per-frame situation: player distance flags, nearby enemies, the route target (subTarget),
// distance / angle to it, and the action checks.
void cSubChar::analyze()
{
    static const Vec chasePosFwd = { 200.0f, 300.0f, 200.0f };
    static const Vec chasePosBck = { 200.0f, 300.0f, -200.0f };
    static int subNear2 = 0;
    static u8 delayMove = 0;
    static u8 npcCheck = 0;
    Vec d;
    int i;
    int up;
    int r;

    anaSatInfo();
    if (GetDistance(pos, pPL->pos) < 4000000.0f) {
        subFlags2 |= 2;
    } else {
        subFlags2 &= ~2;
    }
    subFlags2 &= ~0x201;
    if (!(subFlags & 8)) {
        for (i = 0; i < EmMgr.nArray; i++) {
            cEm* em = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);

            if (!em->isAlive()) {
                continue;
            }
            if (em->hp <= 0) {
                continue;
            }
            if (em->checkStatus(5)) {
                continue;
            }
            if (em == this) {
                continue;
            }
            if (em == pPL) {
                continue;
            }
            if (em->id > 0x3F) {
                continue;
            }
            if (em->checkStatus(0xB)) {
                continue;
            }
            if (GetDistance(pos, em->pos) < 16000000.0f) {
                subFlags2 |= 0x200;
                if (subNear2) {
                    Draw_pos(&em->pos, 1000);
                }
            }
            if (GetDistance(pos, em->pos) < 1000000.0f) {
                subFlags2 |= 1;
            }
        }
    }
    if (sub554 && moveAnotherRoute()) {
        sub554 = 0;
    }
    if (sub580 && !SatMgr.hitCheck(&pParts->worldPos, &pPL->pParts->worldPos, 0, 0, 0, 0)) {
        sub580 = 0;
    }
    up = 0;
    if (fabsf(subTarget.y - pos.y) > 1000.0f) {
        up = 1;
    }
    if (sub554) {
        if (fabsf(sub554->pos.y - pos.y) > 1000.0f) {
            up = 1;
        }
        r = RouteCkToPos(this, &sub554->pos, &subTarget, up, &subX5C8);
    } else if (subFlags & 8) {
        if (fabsf(subMoveTo[1] - pos.y) > 1000.0f) {
            up = 1;
        }
        r = RouteCkToPos(this, (Vec*) subMoveTo, &subTarget, up, &subX5C8);
    } else if (sub580) {
        if (fabsf(sub558.y - pos.y) > 1000.0f) {
            up = 1;
        }
        r = RouteCkToPos(this, &sub558, &subTarget, up, &subX5C8);
    } else {
        if (delayMove) {
            delayMove--;
        }
        if (subPlStatus & 0xE) {
            delayMove = 5;
        }
        if (delayMove == 0 && (GetDistance(&pos, &pPL->pos) > 16000000.0f || !(subPlStatus & 0xE))) {
            if (fabsf(pPL->pos.y - pos.y) > 1000.0f) {
                up = 1;
            }
            r = RouteCkToPos(this, &pPL->pos, &subTarget, up, &subX5C8);
        } else {
            f32 fl;

            if (xFD != 0 && xFD != 2) {
                if (subPlStatus & 4) {
                    subTarget = chasePosFwd;
                } else {
                    subTarget = chasePosBck;
                }
                PSMTXMultVec(pPL->mat, &subTarget, &subTarget);
            } else {
                subTarget = pPL->pos;
            }
            fl = SatMgr.getFloor(&subTarget, 1000.0f, 1000.0f, 0, 0);
            if (fl != -100000.0f) {
                subTarget.y = fl + 100.0f;
            }
            if (SatMgr.hitCheck(&pPL->pParts->worldPos, &subTarget, 0, 0, 0, 0)) {
                PSVECSubtract(&subTarget, &pPL->pParts->worldPos, &d);
#line 3781 "D:/Bio4/Prog/pl_npc.cpp"
                VECNormalize(&d, &d);
                PSVECScale(&d, &d, -200.0f);
                PSVECAdd(&subTarget, &d, &subTarget);
            }
            if (fabsf(subTarget.y - pos.y) > 1000.0f) {
                up = 1;
            }
            r = RouteCkToPos(this, &subTarget, &subTarget, up, &subX5C8);
        }
    }
    if (SatMgr.hitCheck(&pos, &subTarget, 0, 0, 0, 0)) {
        subDist = GetDistance3(&pos, &subTarget);
    } else {
        d.x = subTarget.x;
        d.y = pos.y;
        d.z = subTarget.z;
        subDist = GetDistance3(&pos, &d);
    }
    if (r == 0) {
        subDist += 10000.0f;
    }
    subAng = GetXZAngleLocal(&pos, &subTarget, rot.y);
    subAng = LIMIT_ANGLE(subAng);
    pantsCheck();
    cautionCheck();
    frontCheck();
    if (npcCheck) {
        int y = 0x18;

        for (i = 0; i <= 15; i++) {
            eprintf(y, 0x180, 0, 0, "%d", ((cFlag*) &subFlags)->check(i));
            y += 8;
        }
        Draw_pos(&subTarget, 1000);
    }
}

// subFlags2 bit8: a damage area 1500 ahead on the way to subTarget.
void cSubChar::frontCheck()
{
    Vec d;

    subFlags2 &= ~0x100;
    PSVECSubtract(&subTarget, &pos, &d);
    if (d.x == 0.0f && d.z == 0.0f) {
        return;
    }
    d.y = 0.0f;
#line 3850 "D:/Bio4/Prog/pl_npc.cpp"
    VECNormalize(&d, &d);
    PSVECScale(&d, &d, 1500.0f);
    PSVECAdd(&d, &pos, &d);
    if (DmgMgr.hitCheck(&d, 0)) {
        subFlags2 |= 0x100;
    }
}

// Scenario wall between her and subTarget (400 up): attribute -> sub438, hit / normal, flag bit3.
void cSubChar::anaSatInfo()
{
    Vec a = { 0.0f, 400.0f, 0.0f };
    Vec b;
    u32 r;

    PSVECAdd(&a, &subSelf->pos, &a);
    sub438 = 0;
    if (fabsf(Muku(&pos, &subTarget, rot.y, 3.1415927f)) > 0.5235988f) {
        return;
    }
    b.x = subTarget.x;
    b.y = a.y;
    b.z = subTarget.z;
    r = SatMgr.hitCheck(&a, &b, &sub43C, &sub448, 0, 0);
    if (!(r & 0x01000000)) {
        return;
    }
    if (GetDistance(a, sub43C) > 360000.0f) {
        return;
    }
    sub438 = r;
    subFlags2 |= 8;
}

void cSubChar::beginEvent()
{
    interrupt();
    atari.flags &= 0xFCFF;
}

void cSubChar::endEvent()
{
    be_flag |= 0x200000;
    atari.flags |= 0x300;
}

// pl_sub SubCharCtrl modes: 0 stop, 1 wait here, 2 follow, 3 warp to the player and follow,
// 4 move to subMoveTo, 5 re-init, 6 wait (only from routine 0).
void cSubChar::control(int mode)
{
    if (hp <= 0) {
        return;
    }
    subFlags &= ~0x1C;
    subFlags |= 0x40;
    switch (mode) {
    case 0:
        subFlags |= 1;
        break;
    case 1:
        if (xFC == 5) {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        if ((stat & 0xFFFF0000) != 0x00100000) {
            subFlags &= ~1;
            subFlags |= 2;
            atari.flags |= 0x300;
        }
        break;
    case 3:
        if (xFC == 5) {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        if ((stat & 0xFFFF0000) != 0x00100000) {
            setPos(&pPL->pos);
        }
        xFE = 0;
        xFF = 1;
        xFC = 0;
        xFD = 0;
        atari.flags |= 0x300;
    case 2:
        if (xFC == 5) {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        if ((stat & 0xFFFF0000) != 0x00100000) {
            subFlags &= ~3;
            atari.flags |= 0x300;
        }
        break;
    case 4:
        subFlags &= ~2;
        subFlags |= 8;
        if (subMoveTo[0] != 193.0f) {
            xFD = 1;
            xFF = 0;
            xFC = 0;
            xFE = 0;
            atari.flags |= 0x300;
        } else if (subMoveTo[3] != 193.0f) {
            *(Vec*) subMoveTo = pos;
            xFD = 1;
            xFF = 0;
            xFC = 0;
            xFE = 0;
            atari.flags |= 0x300;
        } else {
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    case 5:
        init();
        SubRoutineSet(this, 0, 0, 0, 0);
        break;
    case 6:
        if (xFC == 0 && (subFlags & 0x40)) {
            subFlags |= 2;
            subFlags &= ~0x60;
            SubRoutineSet(this, 0, 0, 0, 0);
        }
        break;
    }
}

// Ledge in front of the partner: point 800 out from the wall below it and the facing angle.
int getFallPos(cSubChar* pl, Vec* opos, Vec* orot)
{
    Vec a;
    Vec b;
    Vec hit;
    Vec nrm;
    Vec d;

    a.x = 0.0f;
    a.y = 300.0f;
    a.z = 0.0f;
    PSVECAdd(&a, &pl->pos, &a);
    b.y = 300.0f;
    b.z = 1000.0f;
    b.x = 0.0f;
    PSMTXMultVec(pl->mat, &b, &b);
    SatMgr.hitCheck(&a, &b, &hit, &nrm, 0, 0);
    d.x = -nrm.x;
    d.y = 0.0f;
    d.z = -nrm.z;
#line 4080 "D:/Bio4/Prog/pl_npc.cpp"
    VECNormalize(&d, &d);
    PSVECScale(&d, &d, 800.0f);
    PSVECAdd(&hit, &d, &b);
    orot->x = 0.0f;
    orot->y = Muku3(&d, 0.0f, 3.1415927f);
    orot->z = 0.0f;
    opos->x = b.x;
    opos->y = SatMgr.getFloor(&b, 600.0f, 100000.0f, 0, 0);
    opos->z = b.z;
    return 1;
}

// Routine bits of the partner for the camera / scenario.
u32 SubCharGetStatus()
{
    cSubChar* sub = pSUB;
    u32 ret;

    if (sub == 0) {
        return 0;
    }
    if (sub->id != 3) {
        return 0;
    }
    ret = 0;
    switch (sub->xFC) {
    case 0:
        switch (sub->xFD) {
        case 0:
            ret = 1;
            break;
        case 1:
            ret = 2;
            break;
        case 0xA:
            switch (sub->subHideMode) {
            case 0:
            case 3:
                ret |= 0x100;
                break;
            case 1:
                ret = 0x200;
                break;
            case 4:
                ret = 0x10000;
                break;
            }
            break;
        case 0xF:
            ret = 0x01000000;
            break;
        case 0x10:
            if (sub->xFE <= 0xC) {
                ret = 0x04000000;
            } else if (sub->xFE <= 0xD) {
                ret = 0x08000000;
            } else {
                ret = 0x10000000;
            }
            break;
        }
        break;
    case 1:
    case 2:
    case 3:
        ret = 0x80;
        break;
    case 4:
        ret = 0x02000000;
        break;
    case 5:
        switch (sub->xFD) {
        case 0:
            ret = 1;
            break;
        case 1:
            ret = 8;
            break;
        }
        break;
    }
    if ((sub->subFlags & 2) || (sub->subFlags & 1) || (sub->stat & 0xFFFF0000) == 0x00100000) {
        ret |= 0x40000000;
    } else {
        ret |= 0x20000000;
    }
    if ((sub->subFlags & 8) && (sub->subFlags2 & 0x40)) {
        ret |= 0x00800000;
    }
    return ret;
}

// Find an EMI "another route" (type 0xB) start near the partner (kind 0: she is above the player;
// kind 1: a kind 2 entry is near the player) and its first step; 1 when sub554 was set.
int cSubChar::checkAnotherRoute()
{
    EmiData* emi;
    EmiEntry* e;
    int found = -1;
    u8 id = 0;
    int i;
    int j;

    sub554 = 0;
    emi = (EmiData*) pG->pRoomEmi;
    if (emi == 0) {
        return 0;
    }
    if (emi->n == 0) {
        return 0;
    }
    for (i = 0; i < emi->n; i++) {
        e = &emi->entry[i];
        if (e->type != 0xB) {
            continue;
        }
        if (e->state != 0) {
            continue;
        }
        if (e->pad_3 > 1) {
            continue;
        }
        if ((pos.x - e->pos.x) * (pos.x - e->pos.x) + (pos.y - e->pos.y) * (pos.y - e->pos.y) +
                (pos.z - e->pos.z) * (pos.z - e->pos.z) >
            4000000.0f) {
            continue;
        }
        if (e->pad_3 == 1) {
            int ok = 0;

            for (j = 0; j < emi->n; j++) {
                EmiEntry* f = &((EmiData*) pG->pRoomEmi)->entry[j];

                if (f->type != 0xB) {
                    continue;
                }
                if (f->state != 0) {
                    continue;
                }
                if (f->pad_3 != 2) {
                    continue;
                }
                if ((pPL->pos.x - f->pos.x) * (pPL->pos.x - f->pos.x) + (pPL->pos.z - f->pos.z) * (pPL->pos.z - f->pos.z) >
                    16000000.0f) {
                    continue;
                }
                if (fabsf(pPL->pos.y - f->pos.y) > 500.0f) {
                    continue;
                }
                ok = 1;
                break;
            }
            if (!ok) {
                continue;
            }
        } else {
            if (pos.y < pPL->pos.y + 500.0f) {
                continue;
            }
        }
        found = i;
        id = e->sub;
        break;
    }
    if (found == -1) {
        return 0;
    }
    found = -1;
    for (i = 0; i < ((EmiData*) pG->pRoomEmi)->n; i++) {
        e = &((EmiData*) pG->pRoomEmi)->entry[i];
        if (e->type != 0xB) {
            continue;
        }
        if (e->state != 1) {
            continue;
        }
        if (e->sub != id) {
            continue;
        }
        found = i;
        break;
    }
    if (found == -1) {
        return 0;
    }
    sub554 = e;
    return 1;
}

// Step along the "another route": 1 when it ends (near the player past the last step), 0 while
// walking (sub554 advances to the next step once the current one is reached).
int cSubChar::moveAnotherRoute()
{
    EmiData* emi = (EmiData*) pG->pRoomEmi;
    EmiEntry* e;
    EmiEntry* f;
    int bad;
    int next;
    int i;

    bad = emi == 0;
    if (emi->n == 0) {
        bad = 1;
    }
    e = sub554;
    if (e == 0) {
        bad = 1;
    }
    if (bad) {
        sub554 = 0;
        return 1;
    }
    if (e->state > 1) {
        if ((pos.x - pPL->pos.x) * (pos.x - pPL->pos.x) + (pos.y - pPL->pos.y) * (pos.y - pPL->pos.y) +
                (pos.z - pPL->pos.z) * (pos.z - pPL->pos.z) <
            4000000.0f) {
            return 1;
        }
    }
    if ((pos.x - e->pos.x) * (pos.x - e->pos.x) + (pos.y - e->pos.y) * (pos.y - e->pos.y) +
            (pos.z - e->pos.z) * (pos.z - e->pos.z) >
        1000000.0f) {
        return 0;
    }
    next = -1;
    for (i = 0; i < emi->n; i++) {
        f = &((EmiData*) pG->pRoomEmi)->entry[i];
        if (f->type != 0xB) {
            continue;
        }
        if (f->sub != e->sub) {
            continue;
        }
        if (f->state == e->state + 1) {
            next = i;
            break;
        }
    }
    if (next == -1) {
        return 1;
    }
    sub554 = f;
    return 0;
}
