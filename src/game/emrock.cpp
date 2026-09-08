// game/emrock.cpp: rolling rock enemy (cEmRock): boulders that hang on a parent, fall, get
// thrown, roll after the player (with the escape event) or drop on him.
//
// PARTIAL: the camera / escape / hit-check routines (plemRockEscape*, emRock*CamMove,
// emRockRollHitCk, emRockAtkScrCk, emRockSetRoll*, emRockRunDownCk, emRockAtkCk, emRockPushCk,
// emRockDropHitCk*, plemDropFind/Escape/Die, subemDropDie, setFall, setThrow, setThrow2,
// setBreakR11E) are declared in emrock.h but not written yet.

#include "atari.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "dmg.h"
#include "ctrl.h"
#include "emrock.h"
#include "emhit.h"
#include "at_mod.h"
#include "esp.h"
#include "snd.h"
#include "quake.h"
#include "pad.h"
#include "act_btn.h"
#include "player.h"
#include "pl_sub.h"
#include "pl_wep.h"
#include "rnd.h"
#include "global.h"
#include "math_sub.h"
#include "db_log.h"

extern "C" {
int MotionMove(cModel* m, int a);
void EffectEspgenDelete(int a, int b, cModel* m);
}
void MotionSetCore(cModel* m, void* w, void* data, int seq, int hokan, int flags, int frame);   // motion.cpp (C++ linkage)

// lockParts = 0 through an int parameter: the zero becomes an SImode pseudo shared with the
// later `= 0` stores (emmine SetMine).
static inline void LockPartsSet(cEm* em, int no)
{
    em->lockParts = no;
}

// Struct-member view of pPL (the pGS trick): the load stays below the preceding atari flag store.
struct PlayerPtr {
    cPlayer* p;
};
#define PLS (((PlayerPtr*) &pPL)->p)

typedef void (*EmRockFunc)(cEmRock*);

extern "C" void emRock_R0_Move(cEmRock* em);
extern "C" void emRock_R1_Lost(cEmRock* em);

EmRockFunc EmRock_R0_move_tbl[4] = {
    emRock_R0_Init,
    emRock_R0_Move,
    0,
    0,
};

static EmRockFunc EmRock_R1_move_tbl[9] = {
    emRock_R1_Set,
    emRock_R1_Lost,
    emRock_R1_Parent,
    emRock_R1_Fall,
    emRock_R1_Throw,
    emRock_R1_Throw2,
    emRock_R1_Roll,
    emRock_R1_Drop,
    emRock_R1_Drop2,
};

cEmRock* SetRock(void* bin, void* tpl, Vec* pos, Vec* rot, u8 type)
{
    cEmRock* em;
    EmRockWork* w;

    em = (cEmRock*) EmMgr.createBack(0x4A);
    if (em == 0) {
        return 0;
    }
    w = EMROCK_WK(em);
    if (pos) {
        em->pos = *pos;
    }
    if (rot) {
        em->rot = *rot;
    }
    if (em->modelInit(bin, tpl) == 0) {
        pLog->err(0, 0, "SetRock() ModelInit failed.");
        EmMgr.destroy(em);
        return 0;
    }
    em->type = type;
    switch (em->type) {
    case 0:
        break;
    case 1:
        em->scale.x = 4.2f;
        em->scale.y = 4.2f;
        em->scale.z = 4.2f;
        break;
    }
    if (em->type != 3) {
        em->atari.init(0, 0x2000, 10, 0.0f, -(em->scale.y * 1200.0f) * 0.5f, 0.0f, em->scale.x * 1200.0f * 0.5f,
                       em->scale.x * 1200.0f * 0.5f, em->scale.x * 1200.0f * 0.5f, em->scale.y * 1200.0f * 0.5f);
    } else {
        em->atari.init(0, 0x2000, 10, 0.0f, 2000.0f, 0.0f, 2700.0f, 2700.0f, 2700.0f, 2000.0f);
    }
    em->hp = 1000;
    em->hpMax = 1000;
    {
        static const Vec ofs = { 0.0f, 0.0f, 0.0f };
        static const Vec size = { 10000.0f, 10000.0f, 10000.0f };

        if (type != 1 && type != 3) {
            em->lightInfo.init2(0, 1, &ofs, &size, 0x10);
        } else {
            em->lightInfo.init2(0, 1, &ofs, &size, 8);
        }
    }
    LockPartsSet(em, 0);
    em->lockOfs.x = 0.0f;
    em->lockOfs.y = 0.0f;
    em->lockOfs.z = 0.0f;
    em->setStatus(1);
    em->setStatus(0xB);
    em->be_flag &= ~0x01000000;
    em->atari.setPriority(3);
    em->atari.flags &= ~0x100;
    em->be_flag &= ~0x10;
    w->alwaysWait = 4;
    w->sndId = 0;
    w->flags = 0;
    w->x24 = 0;
    w->pParent = 0;
    w->x30 = 0;
    w->pAtk = 0;
    w->xA1 = 0;
    w->se8C = 0;
    w->seFall[0] = 0xFF;
    w->seFall[1] = 0xFF;
    w->seFall[2] = 0;
    w->seFall[3] = 0;
    w->se8D[0] = 0xFF;
    w->se8D[1] = 0xFF;
    w->se8D[2] = 0;
    w->se97[0] = 0xFF;
    w->se97[1] = 0xFF;
    w->se97[2] = 0;
    w->se90[0] = 0xFF;
    w->se90[1] = 0xFF;
    w->se90[2] = 0;
    w->seAlways[0] = 0xFF;
    w->seAlways[1] = 0xFF;
    w->seAlways[2] = 0;
    w->effFall[0] = 0xFF;
    w->effFall[1] = 0xFF;
    w->eff9E[0] = 0xFF;
    w->eff9E[1] = 0xFF;
    w->eff9C[0] = 0xFF;
    w->eff9C[1] = 0xFF;
    if (em->type != 3) {
        w->radius = em->scale.x * 600.0f;
    } else {
        w->radius = 2000.0f;
    }
    w->started = 0;
    w->grav = 20.0f;
    w->rollWait = 0;
    em->pMotion = 0;
    w->plMot[2] = 0;
    w->plMot[3] = 0;
    w->plMot[4] = 0;
    w->plMot[5] = 0;
    w->plMot[6] = 0;
    w->plMot[7] = 0;
    w->plMot[8] = 0;
    w->plMot[9] = 0;
    w->plMot[10] = 0;
    w->plMot[11] = 0;
    w->mot1 = 0;
    w->mot0 = 0;
    w->mot2 = 0;
    w->mot3 = 0;
    w->pSat = 0;
    w->espKind = EspPullCoreKind();
    em->setStatus(5);
    em->flags_3C8 &= ~1;
    em->xFC = 1;
    em->xFD = 0;
    em->xFE = 0;
    em->xFF = 0;
    emRock_R0_Move(em);
    return em;
}

void cEmRock::beginEvent()
{
}

void emRockDmCk(cEmRock* em)
{
    if (em->dmHit == 0) {
        return;
    }
    em->dmHit = 0;
}

void cEmRock::move()
{
    EmRockWork* w = EMROCK_WK(this);

    emRockDmCk(this);
    EmRock_R0_move_tbl[xFC](this);
    if ((be_flag & 0x201) != 1) {
        return;
    }
    EmAtCheck(this);
    atari.move();
    if (w->pParent) {
        alpha = w->pParent->alpha;
        x158 = w->pParent->x158;
        if (w->pParent->be_flag & 2) {
            be_flag |= 2;
        } else {
            be_flag &= ~2;
        }
    }
    if (w->flags & 2) {
        be_flag &= ~2;
    }
    emRockSatSet(this);
}

void emRock_R0_Init(cEmRock* em)
{
    em->xFC = 1;
    em->xFD = 0;
    em->xFE = 0;
    em->xFF = 0;
}

void emRock_R0_Move(cEmRock* em)
{
    EmRock_R1_move_tbl[em->xFD](em);
}

void emRock_R1_Set(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);

    if (em->pMotion) {
        MotionMove(em, 0);
    } else {
        RotMatrix(em->mat, &em->rot);
        TransMatrix(em->mat, &em->pos);
        ScaleMatrix(em->mat, &em->scale);
        em->partsMatCalc();
    }
    em->partsWorldCalc();
    switch (em->xFE) {
    case 0:
        em->xFE++;
        break;
    case 1:
        if (w->started == 0 && em->type == 1) {
            if (emRockRollStartCk(em)) {
                w->started = 1;
                em->flags_3C8 |= 1;
                em->xFC = 1;
                em->xFD = 6;
                em->xFE = 0;
                em->xFF = 0;
            }
        }
        break;
    }
}

void emRock_R1_Lost(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);

    switch (em->xFE) {
    case 0:
        em->hp = 0;
        em->be_flag &= ~2;
        em->clearStatus(5);
        EffectEspgenDelete(0, w->espKind, em);
        em->xFE++;
        w->timer = 30;
    case 1:
        if (w->timer) {
            w->timer--;
        } else {
            EmMgr.destroy(em);
        }
        break;
    }
}

void emRock_R1_Parent(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);
    cEm* parent = w->pParent;
    Mtx m;
    Vec v0;
    Vec v1;
    Vec v2;

    RotMatrix(em->mat, &em->rot);
    TransMatrix(em->mat, &em->pos);
    ScaleMatrix(em->mat, &em->scale);
    if (parent && parent->pParts) {
        PSMTXConcat(parent->getPartsPtr(w->partsNo)->mat, em->mat, m);
        if (!(w->flags & 1)) {
            v0.x = m[0][0];
            v0.y = m[1][0];
            v0.z = m[2][0];
            v1.x = m[0][1];
            v1.y = m[1][1];
            v1.z = m[2][1];
            v2.x = m[0][2];
            v2.y = m[1][2];
            v2.z = m[2][2];
            if (v0.x == 0.0f && v0.y == 0.0f && v0.z == 0.0f) {
                v0.x = 1.0f;
            }
#line 547 "D:/Bio4/Prog/emrock.cpp"
            VECNormalize(&v0, &v0);
            if (v1.x == 0.0f && v1.y == 0.0f && v1.z == 0.0f) {
                v1.y = 1.0f;
            }
#line 549 "D:/Bio4/Prog/emrock.cpp"
            VECNormalize(&v1, &v1);
            if (v2.x == 0.0f && v2.y == 0.0f && v2.z == 0.0f) {
                v2.z = 1.0f;
            }
#line 551 "D:/Bio4/Prog/emrock.cpp"
            VECNormalize(&v2, &v2);
            m[0][0] = v0.x;
            m[1][0] = v0.y;
            m[2][0] = v0.z;
            m[0][1] = v1.x;
            m[1][1] = v1.y;
            m[2][1] = v1.z;
            m[0][2] = v2.x;
            m[1][2] = v2.y;
            m[2][2] = v2.z;
        }
        PSMTXCopy(m, em->mat);
    }
    if (em->pMotion) {
        em->motFlags2 |= 0x40000000;
        MotionMove(em, 0);
    } else {
        em->partsMatCalc();
    }
    em->partsWorldCalc();
}

void emRock_R1_Fall(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);
    Vec d;
    Vec nrm;
    f32 len;
    f32 ang;

    switch (em->xFE) {
    case 0:
        w->timer2 = 60;
        em->xFE++;
    case 1:
        if (w->timer2) {
            w->timer2--;
            break;
        }
        em->pos.x = em->mat[0][3];
        em->pos.y = em->mat[1][3];
        em->pos.z = em->mat[2][3];
        Matrix2AxisAngle(em->mat, &em->rot);
        em->xFC = 1;
        em->xFD = 1;
        em->xFE = 0;
        em->xFF = 0;
        EstSet(0, -1, &em->pos, 0, 1, 8, 0, 0, 0, 0);
        break;
    }
    w->spd.y -= w->grav;
    PSVECAdd(&em->pos, &w->spd, &em->pos);
    nrm.x = 0.0f;
    nrm.y = 0.0f;
    nrm.z = 0.0f;
    EatMgr.adjust(&nrm, &em->oldPos, &em->pos, w->radius, 0x2001, 0);
    if (nrm.x != 0.0f || nrm.y != 0.0f || nrm.z != 0.0f) {
        em->be_flag &= ~2;
        em->pos.x = em->mat[0][3];
        em->pos.y = em->mat[1][3];
        em->pos.z = em->mat[2][3];
        Matrix2AxisAngle(em->mat, &em->rot);
        em->xFC = 1;
        em->xFD = 1;
        em->xFE = 0;
        em->xFF = 0;
        EstSet(0, -1, &em->pos, 0, 1, 8, 0, 0, 0, 0);
        return;
    }
    if (w->pAtk) {
        emRockAtkCk(em, w->pAtk, 0, w->radius);
    }
    {
        Mtx m;
        Vec up;
        Vec axis;

        PSVECSubtract(&em->pos, &em->oldPos, &d);
        PSMTXRotRad(m, 'y', atan2f(d.x, d.z));
        len = SQRTF((em->pos.x - em->oldPos.x) * (em->pos.x - em->oldPos.x) +
                    (em->pos.y - em->oldPos.y) * (em->pos.y - em->oldPos.y) +
                    (em->pos.z - em->oldPos.z) * (em->pos.z - em->oldPos.z));
        if (len > 500.0f) {
            len = 500.0f;
        }
        ang = len * 0.002f * 0.62831855f;
        up.x = 0.0f;
        up.y = 1.0f;
        up.z = 0.0f;
        axis.x = 0.0f;
        axis.y = 0.0f;
        axis.z = 1.0f;
        PSMTXMultVecSR(m, &axis, &axis);
        if (axis.x == 0.0f) {
            axis.y = 0.0f;
        }
#line 654 "D:/Bio4/Prog/emrock.cpp"
        VECNormalize(&axis, &axis);
        len = acosf(PSVECDotProduct(&up, &axis));
        if (len > 0.01f && len < 3.1315927f) {
            PSVECCrossProduct(&up, &axis, &up);
            PSMTXRotAxisRad(m, &up, ang);
            PSMTXConcat(m, em->mat, em->mat);
        }
    }
    TransMatrix(em->mat, &em->pos);
    em->partsWorldCalc();
    emRockAtkScrCk(em);
}

void emRock_R1_Throw(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);
    Vec d;
    Vec nrm;
    f32 len;
    f32 ang;

    switch (em->xFE) {
    case 0:
        w->timer = 0;
        w->timer2 = 60;
        em->xFE++;
    case 1:
        if (w->timer) {
            w->timer--;
        } else {
            w->timer = w->alwaysWait;
            if (w->seAlways[0] != 0xFF && w->seAlways[1] != 0xFF) {
                w->sndId = SndCall(w->seAlways[0], w->seAlways[1], &em->pos, w->seAlways[2], 0, em);
            }
        }
        if (w->timer2) {
            w->timer2--;
            break;
        }
        em->pos.x = em->mat[0][3];
        em->pos.y = em->mat[1][3];
        em->pos.z = em->mat[2][3];
        Matrix2AxisAngle(em->mat, &em->rot);
        em->xFC = 1;
        em->xFD = 1;
        em->xFE = 0;
        em->xFF = 0;
        EstSet(0, -1, &em->pos, 0, 1, 8, 0, 0, 0, 0);
        break;
    }
    w->spd.y -= w->grav;
    PSVECAdd(&em->pos, &w->spd, &em->pos);
    nrm.x = 0.0f;
    nrm.y = 0.0f;
    nrm.z = 0.0f;
    EatMgr.adjust(&nrm, &em->oldPos, &em->pos, w->radius, 0x2001, 0);
    if (nrm.x != 0.0f || nrm.y != 0.0f || nrm.z != 0.0f) {
        f32 spd;

        spd = RootSumSquare3(&w->spd);
        C_VECReflect(&w->spd, &nrm, &d);
        PSVECScale(&d, &w->spd, spd * 0.99f);
        if (nrm.y < 0.5f) {
            em->be_flag &= ~2;
            em->pos.x = em->mat[0][3];
            em->pos.y = em->mat[1][3];
            em->pos.z = em->mat[2][3];
            Matrix2AxisAngle(em->mat, &em->rot);
            em->xFC = 1;
            em->xFD = 1;
            em->xFE = 0;
            em->xFF = 0;
            EstSet(0, -1, &em->pos, 0, 1, 8, 0, 0, 0, 0);
            return;
        }
        if (w->spd.y > 50.0f) {
            if (w->seFall[0] != 0xFF && w->seFall[1] != 0xFF) {
                SndCall(w->seFall[0], w->seFall[1], &em->pos, w->seFall[2], 0, em);
            }
            if (w->effFall[0] != 0xFF && w->effFall[1] != 0xFF) {
                Vec fp;

                fp = em->pos;
                fp.y = EatMgr.getFloor(&fp, 600.0f, 100000.0f, 0, 0);
                EstSet(0, -1, &fp, 0, w->effFall[0], w->effFall[1], 0, 0, 0, 0);
            }
            QuakeExec(0, 0, 5, 22.0f, 2);
        }
    }
    if (w->pAtk) {
        emRockAtkCk(em, w->pAtk, 1, w->radius);
    }
    {
        Mtx m;
        Vec up;
        Vec axis;

        PSVECSubtract(&em->pos, &em->oldPos, &d);
        PSMTXRotRad(m, 'y', atan2f(d.x, d.z));
        len = SQRTF((em->pos.x - em->oldPos.x) * (em->pos.x - em->oldPos.x) +
                    (em->pos.y - em->oldPos.y) * (em->pos.y - em->oldPos.y) +
                    (em->pos.z - em->oldPos.z) * (em->pos.z - em->oldPos.z));
        if (len > 500.0f) {
            len = 500.0f;
        }
        ang = len * 0.002f * 0.62831855f;
        up.x = 0.0f;
        up.y = 1.0f;
        up.z = 0.0f;
        axis.x = 0.0f;
        axis.y = 0.0f;
        axis.z = 1.0f;
        PSMTXMultVecSR(m, &axis, &axis);
        if (axis.x == 0.0f) {
            axis.y = 0.0f;
        }
#line 800 "D:/Bio4/Prog/emrock.cpp"
        VECNormalize(&axis, &axis);
        len = acosf(PSVECDotProduct(&up, &axis));
        if (len > 0.01f && len < 3.1315927f) {
            PSVECCrossProduct(&up, &axis, &up);
            PSMTXRotAxisRad(m, &up, ang);
            PSMTXConcat(m, em->mat, em->mat);
        }
    }
    TransMatrix(em->mat, &em->pos);
    em->partsWorldCalc();
    emRockAtkScrCk(em);
}

void emRock_R1_Throw2(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);
    cAtariInfo* at;
    Vec d;
    Vec nrm;
    f32 len;
    f32 ang;

    switch (em->xFE) {
    case 0:
        w->timer = 0;
        w->timer2 = 180;
        at = &em->atari;
        at->flags &= ~0x300;
        SndCall(6, 0x49, &em->pos, 0, 0, em);
        em->xFE++;
    case 1:
        if (w->timer) {
            w->timer--;
        } else {
            w->timer = w->alwaysWait;
            if (w->seAlways[0] != 0xFF && w->seAlways[1] != 0xFF) {
                w->sndId = SndCall(w->seAlways[0], w->seAlways[1], &em->pos, w->seAlways[2], 0, em);
            }
        }
        if (w->timer2) {
            w->timer2--;
            break;
        }
        em->pos.x = em->mat[0][3];
        em->pos.y = em->mat[1][3];
        em->pos.z = em->mat[2][3];
        Matrix2AxisAngle(em->mat, &em->rot);
        em->xFC = 1;
        em->xFD = 1;
        em->xFE = 0;
        em->xFF = 0;
        break;
    }
    w->spd.y -= w->grav;
    PSVECAdd(&em->pos, &w->spd, &em->pos);
    nrm.x = 0.0f;
    nrm.y = 0.0f;
    nrm.z = 0.0f;
    EatMgr.adjust(&nrm, &em->oldPos, &em->pos, w->radius, 0x2001, 0);
    if (nrm.x != 0.0f || nrm.y != 0.0f || nrm.z != 0.0f) {
        Vec p;

        RootSumSquare3(&w->spd);
        C_VECReflect(&w->spd, &nrm, &d);
        if ((s16) pG->pl_life > 500) {
            w->pAtk->x0A |= 4;
        } else {
            w->pAtk->x0A &= ~4;
        }
        p = em->pos;
        p.y += 1000.0f;
        PlWepHitCheck2(0, &p, &p, 0x12, 3, 5000.0f);
        EffectEspgenDelete(0, w->espKind, em);
        if (nrm.y > 0.7f) {
            EstSet(0, -1, &em->pos, 0, 1, 1, 0, 0, 0, 0);
        } else {
            EstSet(0, -1, &em->pos, 0, 1, 2, 0, 0, 0, 0);
        }
        SndCall(6, 0x4A, &em->pos, 0, 0, em);
        em->be_flag &= ~2;
        em->xFC = 1;
        em->xFD = 1;
        em->xFE = 0;
        em->xFF = 0;
        return;
    }
    {
        Mtx m;
        Vec up;
        Vec axis;

        PSVECSubtract(&em->pos, &em->oldPos, &d);
        PSMTXRotRad(m, 'y', atan2f(d.x, d.z));
        len = SQRTF((em->pos.x - em->oldPos.x) * (em->pos.x - em->oldPos.x) +
                    (em->pos.y - em->oldPos.y) * (em->pos.y - em->oldPos.y) +
                    (em->pos.z - em->oldPos.z) * (em->pos.z - em->oldPos.z));
        if (len > 500.0f) {
            len = 500.0f;
        }
        ang = len * 0.002f * 0.62831855f;
        up.x = 0.0f;
        up.y = 1.0f;
        up.z = 0.0f;
        axis.x = 0.0f;
        axis.y = 0.0f;
        axis.z = 1.0f;
        PSMTXMultVecSR(m, &axis, &axis);
        if (axis.x == 0.0f) {
            axis.y = 0.0f;
        }
#line 936 "D:/Bio4/Prog/emrock.cpp"
        VECNormalize(&axis, &axis);
        len = acosf(PSVECDotProduct(&up, &axis));
        if (len > 0.01f && len < 3.1315927f) {
            PSVECCrossProduct(&up, &axis, &up);
            PSMTXRotAxisRad(m, &up, ang);
            PSMTXConcat(m, em->mat, em->mat);
        }
    }
    TransMatrix(em->mat, &em->pos);
    em->partsWorldCalc();
    emRockAtkScrCk(em);
}

void emRock_R1_Roll(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);
    Vec d;
    f32 len;
    f32 ang;

    switch (em->xFE) {
    case 0:
        KeyStop(0xEFCF0000ULL);
        if (emRockSetRollRoute(em) == 0) {
            em->pos.x = em->mat[0][3];
            em->pos.y = em->mat[1][3];
            em->pos.z = em->mat[2][3];
            Matrix2AxisAngle(em->mat, &em->rot);
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
            return;
        }
        switch (pG->room_no) {
        case 4:
            SndStrReq(1, 0x3E, 0x80000003, 0, 0, 0.0f);
            break;
        case 6:
            SndStrReq(1, 0x3C, 0x80000003, 0, 0, 0.0f);
            break;
        case 0xA:
            SndStrReq(1, 0x3D, 0x80000003, 0, 0, 0.0f);
            break;
        }
        emRockPushCk(em, 0);
        em->atari.flags &= ~0x200;
        PLS->rot.y = em->rot.y;
        SetPlDamage((int) em, (void (*)(cPlayer*)) plemRockEscape);
        w->timer3 = 75;
        w->spd.x = 0.0f;
        w->spd.y = 0.0f;
        w->spd.z = 0.0f;
        if ((pG->room_id32 & 0xFFFF0000) == 0x01040000) {
            w->xAD = 1;
            w->rollWait = 0;
        } else {
            w->xAD = 0;
            w->rollWait = 25;
        }
        em->xFE++;
    case 1:
        if (w->timer3) {
            w->timer3--;
            if (w->timer3 == 0) {
                w->sndId2 = SndCall(6, 5, &em->pos, 0, 0, em);
            }
            return;
        }
        if (emRockSetRollSpd(em)) {
            SndStop(w->sndId2, 0);
            SndCall(6, 6, &em->pos, 0, 0, em);
            EstSet(0, -1, &em->pos, 0, 1, 0x1F, 0, 0, 0, 0);
            em->be_flag &= ~2;
            em->xFC = 1;
            em->xFD = 1;
            em->xFE = 0;
            em->xFF = 0;
            return;
        }
    default:
        w->spd.y -= 10.0f;
        PSVECAdd(&em->pos, &w->spd, &em->pos);
        if (w->rollWait) {
            w->rollWait--;
        } else {
            f32 floor;

            floor = EatMgr.getFloor(&em->pos, 600.0f, 100000.0f, 0, 0) + w->radius;
            if (em->pos.y < floor) {
                em->pos.y = floor;
                w->spd.y *= -0.5f;
                if (w->spd.y > 50.0f) {
                    Vec fp;

                    fp = em->pos;
                    fp.y -= w->radius;
                    EstSet(0, -1, &fp, 0, 0xC8, 0, 0, 0, 0, 0);
                    SndCall(6, 7, &em->pos, 0, 0, em);
                    if (w->xAD == 0) {
                        w->xAD = 1;
                        w->spd.x = 0.0f;
                        w->spd.z = 0.0f;
                    }
                }
            }
        }
        emRockRollHitCk(em);
        emRockRunDownCk(em);
        {
            Mtx m;
            Vec up;
            Vec axis;

            PSVECSubtract(&em->pos, &em->oldPos, &d);
            PSMTXRotRad(m, 'y', atan2f(d.x, d.z));
            len = SQRTF((em->pos.x - em->oldPos.x) * (em->pos.x - em->oldPos.x) +
                        (em->pos.y - em->oldPos.y) * (em->pos.y - em->oldPos.y) +
                        (em->pos.z - em->oldPos.z) * (em->pos.z - em->oldPos.z));
            if (len > 500.0f) {
                len = 500.0f;
            }
            ang = len * 0.002f * 0.31415927f;
            up.x = 0.0f;
            up.y = 1.0f;
            up.z = 0.0f;
            axis.x = 0.0f;
            axis.y = 0.0f;
            axis.z = 1.0f;
            PSMTXMultVecSR(m, &axis, &axis);
            if (axis.x == 0.0f) {
                axis.y = 0.0f;
            }
#line 1093 "D:/Bio4/Prog/emrock.cpp"
            VECNormalize(&axis, &axis);
            len = acosf(PSVECDotProduct(&up, &axis));
            if (len > 0.01f && len < 3.1315927f) {
                PSVECCrossProduct(&up, &axis, &up);
                PSMTXRotAxisRad(m, &up, ang);
                PSMTXConcat(m, em->mat, em->mat);
            }
        }
        TransMatrix(em->mat, &em->pos);
        em->partsWorldCalc();
        emRockAtkScrCk(em);
        break;
    }
}

void emRock_R1_Drop(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);

    switch (em->xFE) {
    case 0:
        em->atari.flags &= ~0x200;
        MotionSetCore(em, &em->pMotion, w->mot0, 0, 0, 1, 0);
        em->xFE++;
    case 1:
        MotionMove(em, 0);
        if (!(em->flags_3C8 & 1)) {
            break;
        }
        em->xFE++;
    case 2:
        MotionSetCore(em, &em->pMotion, w->mot1, 0, 0, 1, 0);
        EstSet((int) em, -1, 0, 0, 1, 4, 0, w->espKind, (u32) em, 0);
        SndCall(6, 8, &em->pos, 0, 0, em);
        w->timer = 37;
        em->xFE++;
    case 3:
        if (w->timer) {
            w->timer--;
            emRockDropHitCk(em);
            emRockDropHitCkSub(em);
            if (emRockDropHitCkEm2b(em)) {
                em->be_flag &= ~2;
                em->atari.flags &= ~0x200;
                em->hp = 0;
                em->xFC = 1;
                em->xFD = 1;
                em->xFE = 0;
                em->xFF = 0;
                EffectEspgenDelete(0, w->espKind, em);
                EstSet(0, -1, &em->getPartsPtr(0)->worldPos, 0, 1, 1, 0, 0, 0, 0);
                SndCall(6, 7, &em->pos, 0, 0, em);
                break;
            }
            if (w->timer == 0) {
                SndCall(6, 7, &em->pos, 0, 0, em);
                em->atari.flags |= 0x200;
            }
        }
        if (MotionMove(em, 0)) {
            em->xFE++;
        }
        break;
    case 4:
        break;
    }
    em->partsWorldCalc();
}

void emRock_R1_Drop2(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);

    switch (em->xFE) {
    case 0:
        em->xFE++;
        em->atari.flags &= ~0x200;
    case 1:
        MotionSetCore(em, &em->pMotion, w->mot1, 0, 0, 1, 0);
        MotionMove(em, 0);
        if (!(em->flags_3C8 & 1)) {
            break;
        }
        if ((s16) pG->pl_life <= 0) {
            break;
        }
        KeyStop(0xEFCF0000ULL);
        em->xFE++;
    case 2:
        w->timer = 25;
        emRockPushCk(em, 50);
        FSet(pPL->rot.y, -0.1f);
        FSet(pPL->pos.x, -4924.0f);
        FSet(pPL->pos.y, -11950.0f);
        FSet(pPL->pos.z, -14770.0f);
        pPL->setPos(&pPL->pos);
        pPL->st.x325 = 2;
        SetPlDamage((int) em, (void (*)(cPlayer*)) plemDropFind);
        em->xFE++;
    case 3:
        MotionSetCore(em, &em->pMotion, w->mot1, 0, 0, 1, 0);
        MotionMove(em, 0);
        if (w->timer) {
            w->timer--;
            break;
        }
        em->xFE++;
        break;
    case 4:
        MotionSetCore(em, &em->pMotion, w->mot1, 0, 0, 1, 0);
        EstSet(0, -1, 0, 0, 1, 5, 0, 0, 0, 0);
        SndCall(6, 4, &em->pos, 0, 0, em);
        w->timer = 31;
        w->rnd = Rnd() & 1;
        w->xAE = 0;
        em->xFE++;
    case 5:
        if (MotionMove(em, 0)) {
            SndCall(6, 5, &em->pos, 0, 0, em);
            em->be_flag &= ~2;
            em->xFC = 1;
            em->xFD = 1;
            em->xFE = 0;
            em->xFF = 0;
            break;
        }
        if (w->timer) {
            w->timer--;
            if (w->timer == 0) {
                if (w->xAE != 0) {
                    break;
                }
                if ((s16) pG->pl_life > 0) {
                    w->xAE = 1;
                    SetPlDamage((int) em, (void (*)(cPlayer*)) plemDropDie);
                    pPL->xFF = 1;
                    break;
                }
            }
            if (w->xAE == 0) {
                switch (w->rnd) {
                case 0:
                default:
                    ActBtn.set(0x25, 5, (int) plemDropEscAction, (int) em, 0x42, 3, 0, 0);
                    break;
                case 1:
                    ActBtn.set(0x25, 5, (int) plemDropEscAction, (int) em, 0x42, 4, 0, 0);
                    break;
                }
            }
        }
        break;
    }
    em->partsWorldCalc();
}

void plemDropEscAction(cEmRock* em)
{
    EMROCK_WK(em)->xAE = 1;
    pPL->st.x325 = 2;
    SetPlDamage((int) em, (void (*)(cPlayer*)) plemDropEscape);
}

void cEmRock::setParent(cEm* parent, int partsNo_, int flag)
{
    EmRockWork* w = EMROCK_WK(this);

    w->partsNo = partsNo_;
    w->pParent = parent;
    if (flag) {
        w->flags |= 1;
    } else {
        w->flags &= ~1;
    }
    xFC = 1;
    xFD = 2;
    xFE = 0;
    xFF = 0;
    parent->atari.flags &= ~0x200;
}

void cEmRock::setSeFall(u8 blk, u8 no, u8 vol)
{
    EmRockWork* w = EMROCK_WK(this);

    w->seFall[0] = blk;
    w->seFall[1] = no;
    w->seFall[2] = vol;
    w->seFall[3] = 0;
}

void cEmRock::setEffFall(u8 id, u8 type)
{
    EmRockWork* w = EMROCK_WK(this);

    w->effFall[0] = id;
    w->effFall[1] = type;
}

void cEmRock::setEffAlways(int id, int type)
{
    EstSet((int) this, -1, 0, 0, id, type, 0, EMROCK_WK(this)->espKind, (u32) this, 0);
}

void cEmRock::setYarareCube(Vec* size, f32 x, f32 y, f32 z)
{
    if (size) {
        YarareInitCube(this, size->x, size->y, size->z, x, y, z, 0, 1);
    } else {
        YarareInitCube(this, 0.0f, -400.0f, 0.0f, x, y, z, 0, 1);
    }
    hp = 1;
}

void cEmRock::setTransMode(int on)
{
    EmRockWork* w = EMROCK_WK(this);

    if (on) {
        w->flags &= ~2;
    } else {
        w->flags |= 2;
    }
}

void plemRockEscAction(cEmRock* em)
{
    EMROCK_WK(em)->xAE = 1;
    em->xFE++;
}

void cEmRock::setPlMotion(void** mot)
{
    EmRockWork* w = EMROCK_WK(this);

    w->plMot[0] = *mot++;
    w->plMot[1] = *mot++;
    w->plMot[2] = *mot++;
    w->plMot[3] = *mot++;
    w->plMot[4] = *mot++;
    w->plMot[5] = *mot++;
    w->plMot[6] = *mot++;
    w->plMot[7] = *mot++;
    w->plMot[8] = *mot++;
    w->plMot[9] = *mot++;
    w->plMot[10] = *mot++;
    w->plMot[11] = *mot++;
    w->plMot[12] = *mot++;
    w->plMot[13] = *mot++;
    w->plMot[14] = *mot++;
    w->plMot[15] = *mot++;
}

void cEmRock::setScale(f32 s)
{
    scale.z = s;
    scale.y = s;
    scale.x = s;
    EMROCK_WK(this)->radius = s * 600.0f;
}

void cEmRock::setDropMot(void* a, void* b, void* c, void* d)
{
    EmRockWork* w = EMROCK_WK(this);

    w->mot0 = a;
    w->mot1 = b;
    w->mot2 = c;
    w->mot3 = d;
    xFC = 1;
    xFD = 7;
    xFE = 0;
    xFF = 0;
}

void cEmRock::setDropMot2(void* a, void* b, void* c, void* d, void* e, void* f, void* g)
{
    EmRockWork* w = EMROCK_WK(this);

    w->mot1 = a;
    w->mot2 = b;
    w->mot4 = c;
    w->mot5 = d;
    w->plMot[13] = e;
    w->plMot[14] = f;
    w->plMot[15] = g;
    xFC = 1;
    xFD = 8;
    xFE = 0;
    xFF = 0;
}

void emRockSatClear(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);

    if (pG->room_id != 0x11E) {
        return;
    }
    if (em->type != 3) {
        return;
    }
    if (w->pSat == 0) {
        return;
    }
    w->pSat->flags &= ~4;
}

void emRockSatSet(cEmRock* em)
{
    EmRockWork* w = EMROCK_WK(em);
    Vec pos;
    Vec rot;

    if (pG->room_id != 0x11E) {
        return;
    }
    if (em->type != 3) {
        return;
    }
    emRockSatClear(em);
    if (!(em->be_flag & 2)) {
        return;
    }
    pos = em->getPartsPtr(0)->worldPos;
    pos.y -= 400.0f;
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 0.0f;
    if (w->pSat) {
        w->pSat->flags |= 4;
        w->pSat->setCoord(&pos, &rot);
    } else {
        w->pSat = EatMgr.create((void*) (((u32*) pG->pRoomArc)[5] + (u32) pG->pRoomArc), 0, &pos, &rot, 1);
    }
}
