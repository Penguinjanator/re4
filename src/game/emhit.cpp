// game/emhit.cpp: hit-only enemy (cEmHit): a damage receiver for objects, a parts follower
// (setParent) and the beetle that flies away when shot.

#include "atari.h"
#include "light.h"
#include "dmg.h"
#include "emhit.h"
#include "global.h"
#include "math_sub.h"
#include "db_log.h"
#include "motion.h"

extern cEm* pPL;   // game/em.cpp

extern "C" {
void EtcSetAddAmb(cModel* m, int kind);                                                         // EtcModel.cpp
}

typedef void (*EmHitFunc)(cEmHit*);

static EmHitFunc EmHit_R0_move_tbl[4] = {
    emHit_R0_Init,
    emHit_R0_Move,
    0,
    0,
};

EmHitFunc EmHit_R1_move_tbl[4] = {
    emHit_R1_Set,
    emHit_R1_Parent,
    emHit_R1_Break,
    emHit_R1_Beetle,
};

// Creates a hit-only enemy (id 0x4D) from a model / TPL at pos / rot: an unlockable, invisible-to-
// Ashley damage receiver with a 200 unit hit cylinder, 1000 hp and a 700 x 400 atari cylinder.
// `type` picks the damage reaction (0 break, 1 report only, 2 hp to 0). Starts in Rno0 1 /
// Rno1 0 (emHit_R1_Set). NULL when no work or the model fails.
cEmHit* SetEmHit(void* bin, void* tpl, Vec* pos, Vec* rot, int type)
{
    cEmHit* em;
    EmHitWork* w;

    em = (cEmHit*) EmMgr.create(0x4D);
    if (em == 0) {
        return 0;
    }
    w = EMHIT_WK(em);
    if (pos) {
        em->pos = *pos;
    }
    if (rot) {
        em->ang = *rot;
    }
    if (em->modelInit(bin, tpl) == 0) {
        pLog->err(0, 0, "SetEmHit() failed.");
        EmMgr.destroy(em);
        return 0;
    }
    em->type = type;
    EtcSetAddAmb(em, 5);
    cModel* parent = 0;
    w->x248 = 0xFF;
    w->size.x = 200.0f;
    w->size.y = 200.0f;
    w->size.z = 200.0f;
    em->atari.init(0.0f, 0.0f, 0.0f, 700.0f, 400.0f, 500.0f, 500.0f, 0, 2, 0);
    em->atari.setPriority(PRI_LV3);
    em->atari.throughOn();
    emHitYarareInit(em);
    em->hp_max = em->hp = 1000;
    {
        static const Vec ofs = { 0.0f, 0.0f, 0.0f };
        static const Vec size = { 1000.0f, 1000.0f, 0.0f };

        em->LightInfo.init2(0, 1, &ofs, &size, 0x10);
    }
    em->lockParts = 0;
    em->lockOfs.x = 0.0f;
    em->lockOfs.y = 0.0f;
    em->lockOfs.z = 0.0f;
    em->setStatus(EM_STATUS_LOCKOFF);
    em->setStatus(EM_STATUS_ASHLEY_NO_HELP);
    w->Be_flg = 0;
    em->be_flag &= ~0x01000000;
    em->be_flag &= ~0x10;
    w->Status = 0;
    w->pParent = parent;
    w->partsNo = 0;
    w->noNormalize = 0;
    em->r_no_0 = 1;
    em->r_no_1 = 0;
    em->r_no_2 = 0;
    em->r_no_3 = 0;
    return em;
}

// Damage check at the top of every frame: consumes the registered hit (dmHit / dmg.m_Wep), ignores
// the knife, grenades and other non-bullet weapons, then by type: 0 dies (Rno1 2 Break), 1 only
// raises Status for the owner to read, 2 sets hp to 0.
void emHitDmCk(cEmHit* em)
{
    EmHitWork* w = EMHIT_WK(em);
    u8 wep;

    w->Status = 0;
    if (em->dmg.m_Flag == 0) {
        em->dmg.m_Wep = 0;
        return;
    }
    wep = em->dmg.m_Wep;
    em->dmg.m_Flag = 0;
    if (wep == 0x14) {
        return;
    }
    if (wep == 0x16) {
        return;
    }
    if (wep == 0x17) {
        return;
    }
    if (wep == 0x2A) {
        return;
    }
    if (wep == 0xE) {
        return;
    }
    em->dmg.m_Timer = 1;
    if (wep == 0x10) {
        em->dmg.m_Timer = 0x11;
    }
    switch (em->type) {
    case 0:
    default:
        em->hp = 0;
        w->Status = 1;
        em->r_no_0 = 1;
        em->r_no_1 = 2;
        em->r_no_2 = 0;
        em->r_no_3 = 0;
        break;
    case 1:
        w->Status = 1;
        break;
    case 2:
        em->hp = 0;
        break;
    }
}

// Per-frame: clears Status, runs the damage check and the Rno0 routine (0 Init, 1 Move).
void cEmHit::move()
{
    EmHitWork* w = EMHIT_WK(this);

    w->Status = 0;
    emHitDmCk(this);
    be_flag &= ~0x4000;
    EmHit_R0_move_tbl[r_no_0](this);
}

// Rno0 == 0: resets the routine numbers to the Set state.
void emHit_R0_Init(cEmHit* em)
{
    em->r_no_0 = 1;
    em->r_no_1 = 0;
    em->r_no_2 = 0;
    em->r_no_3 = 0;
}

// Rno0 == 1: dispatches on Rno1 (0 Set, 1 Parent, 2 Break, 3 Beetle).
void emHit_R0_Move(cEmHit* em)
{
    EmHit_R1_move_tbl[em->r_no_1](em);
}

// Rno1 == 0: a static hit target; builds the matrices once (Rno2 0 -> 1) and marks itself
// be_flag 0x4000 (hit box only, not drawn) every frame.
void emHit_R1_Set(cEmHit* em)
{
    if (em->r_no_2 == 0) {
        RotMatrix(em->mat, &em->ang);
        TransMatrix(em->mat, &em->pos);
        ScaleMatrix(em->mat, &em->scale);
        em->partsMatCalc();
        em->partsWorldCalc();
        em->r_no_2++;
    }
    em->be_flag |= 0x4000;
}

// Rno1 == 1: follows parts `partsNo` of pParent (setParent): mat = parent parts matrix * own
// matrix, with the rotation columns re-normalised unless noNormalize; plays its own motion when
// it has one.
void emHit_R1_Parent(cEmHit* em)
{
    Mtx m;
    Vec v0;
    Vec v1;
    Vec v2;
    EmHitWork* w = EMHIT_WK(em);
    cModel* parent = w->pParent;

    RotMatrix(em->mat, &em->ang);
    TransMatrix(em->mat, &em->pos);
    ScaleMatrix(em->mat, &em->scale);
    if (parent && parent->pParts) {
        PSMTXConcat(parent->getPartsPtr(w->partsNo)->mat, em->mat, m);
        if (w->noNormalize == 0) {
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
#line 340 "D:/Bio4/Prog/emhit.cpp"
            VECNormalize(&v0, &v0);
            if (v1.x == 0.0f && v1.y == 0.0f && v1.z == 0.0f) {
                v1.y = 1.0f;
            }
#line 342 "D:/Bio4/Prog/emhit.cpp"
            VECNormalize(&v1, &v1);
            if (v2.x == 0.0f && v2.y == 0.0f && v2.z == 0.0f) {
                v2.z = 1.0f;
            }
#line 344 "D:/Bio4/Prog/emhit.cpp"
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

// Rno1 == 2: the object was shot (type 0): hides the model (be_flag bit1 off), hp 0, Status 1 on
// the first frame, then stays as a hit-box-only work.
void emHit_R1_Break(cEmHit* em)
{
    EmHitWork* w = EMHIT_WK(em);

    switch (em->r_no_2) {
    case 0:
        em->hp = 0;
        em->be_flag &= ~2;
        w->Status = 1;
        em->r_no_2++;
    case 1:
        em->be_flag |= 0x4000;
        break;
    }
}

// Rno1 == 3: the beetle (setBeetle): Rno2 0/1 idle motion until shot or until the player faces
// it within 1500 units, 2/3 startle motion, 4/5 flies away (fly motion, speed decaying toward
// 4 up / 4 forward) for 300 frames then fades out and hides.
void emHit_R1_Beetle(cEmHit* em)
{
    EmHitWork* w = EMHIT_WK(em);

    switch (em->r_no_2) {
    case 0:
        MotionSetCore(em, &em->pMotion, w->mot0, 0, 0, 5, 0);
        em->r_no_2++;
    case 1:
        MotionMove(em, 0);
        if (em->hp <= 0) {
            em->r_no_2++;
        } else if (fabsf(Muku(&pPL->pos, &em->pos, em->ang.y, 3.1415927f)) < 0.5235988f) {
            if (em->plDist2 < 2250000.0f) {
                em->hp = 0;
                em->r_no_2++;
            }
        }
        break;
    case 2:
        MotionSetCore(em, &em->pMotion, w->mot1, 0, 0, 1, 0x1F);
        em->r_no_2++;
    case 3:
        if (MotionMove(em, 0)) {
            em->r_no_2++;
        }
        break;
    case 4:
        MotionSetCore(em, &em->pMotion, w->mot2, 0, 3, 5, 0);
        w->spd.x = 0.0f;
        w->spd.y = 10.0f;
        w->spd.z = 10.0f;
        PSMTXMultVecSR(em->mat, &w->spd, &w->spd);
        w->Timer = 300;
        em->r_no_2++;
    case 5:
        PSVECAdd(&em->pos, &w->spd, &em->pos);
        w->spd.y = w->spd.y * 0.9f + 4.0f;
        w->spd.z = w->spd.z * 0.9f + 4.0f;
        MotionMove(em, 0);
        if (w->Timer) {
            w->Timer--;
        } else {
            em->invisible_factor -= 0.1f;
            if (em->invisible_factor <= 0.0f) {
                em->be_flag &= ~2;
                em->invisible_factor = 0.0f;
                em->r_no_2++;
            }
        }
        break;
    }
    em->partsWorldCalc();
}

// Default hit box: a cube of the work's size (x/z half + 50, y full) at the origin.
void emHitYarareInit(cEmHit* em)
{
    EmHitWork* w = EMHIT_WK(em);

    YarareInitCube(em, 0.0f, 0.0f, 0.0f, w->size.x * 0.5f + 50.0f, w->size.y, w->size.z * 0.5f + 50.0f, 0, 1);
}

// 1 during the frame the target was hit (Status).
int cEmHit::ckStatus()
{
    return EMHIT_WK(this)->Status;
}

// The weapon id that hit the target this frame; 0 when none.
int cEmHit::ckDmgWeapon()
{
    if (EMHIT_WK(this)->Status == 0) {
        return 0;
    }
    return dmg.m_Wep;
}

// Attaches the hit target to parts `partsNo` of `parent` (Rno1 1) and disables the parent's own
// atari flag 0x200 so the hit target takes the shots.
void cEmHit::setParent(cModel* parent, int partsNo, int noNormalize)
{
    EmHitWork* w = EMHIT_WK(this);

    w->pParent = parent;
    w->partsNo = partsNo;
    w->noNormalize = noNormalize;
    r_no_0 = 1;
    r_no_1 = 1;
    r_no_2 = 0;
    r_no_3 = 0;
    ((cEm*) parent)->atari.m_flag &= ~0x200;
}

// Turns the target into the beetle: idle / startle / fly motions, a 50 x 100 hit box, hp 1 and
// Rno1 3. Ignored when any motion is missing.
void cEmHit::setBeetle(void* mot0, void* mot1, void* mot2)
{
    EmHitWork* w = EMHIT_WK(this);

    w->mot0 = mot0;
    w->mot1 = mot1;
    w->mot2 = mot2;
    if (mot0 && mot1 && mot2) {
        YarareInitCube(this, 0.0f, 0.0f, 0.0f, 50.0f, 100.0f, 100.0f, 1, 1);
        hp = 1;
        r_no_0 = 1;
        r_no_1 = 3;
        r_no_2 = 0;
        r_no_3 = 0;
    }
}
