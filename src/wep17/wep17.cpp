// wep17 module: the VP70 (weapon number 0x11, cObjVp70 object id 0x34) and the weapon number 3
// variant of the Mauser (wep02's cObjMauser, id 0x27, imported from that module). The module has
// its own player routine: the handgun routine (wep/pl_handgun.cpp) with an aim stance (the
// pWep->knifeStance byte), a wall check / enemy check at the ready start (ready00, ckEmWep), the
// lock target switch (r2_next) and the wall-facing "out" routine (r2_out).

#include "wep_mod.h"
#include "light.h"
#include "item.h"
#include "esp.h"
#include "main.h"
#include "cam_ctrl.h"
#include "camera.h"
#include "motion.h"
#include "pad.h"
#include "snd.h"
#include "rnd.h"
#include "math_sub.h"
#include "em.h"

extern "C" {
f64 atan2(f64 y, f64 x);
// motion.h declares the one-argument MotionMove; the routines pass a second argument (pl_knife.cpp).
int MotionMoveI(cModel* m, int flag) asm("MotionMove");
}

#define VALID_PTR(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)
#define WEP_OBJ(pl) ((pl)->Wep->m_pWep)
// The weapon object's own cAtariInfo (the object's collision with enemies while it is held).
#define WEP_ATARI(pl) (&WEP_OBJ(pl)->sub2B4.atari)

void ObjMauser_init(cObj* obj);   // wep02/objMauser.cpp (module import)
void ObjVp70_init(cObj* obj);     // wep17/objVp70.cpp

void Wep17_move(cPlayer* pl);
int ckEmWep(cPlayer* pl);
void wepDown(cPlayer* pl);
cObjWep* equipWeapon(cPlayer* pl);
static void wep17_r2_ready(cPlayer* pl);
static void wep17_r3_ready00(cPlayer* pl);
static void wep17_r3_ready10(cPlayer* pl);
static void wep17_r3_ready20(cPlayer* pl);
static void wep17_r3_ready30(cPlayer* pl);
static void wep17_r2_set(cPlayer* pl);
static void wep17_r3_set00(cPlayer* pl);
static void wep17_r3_set10(cPlayer* pl);
static void wep17_r3_set20(cPlayer* pl);
static void wep17_r3_set30(cPlayer* pl);
static void wep17_r3_set40(cPlayer* pl);
static void wep17_r2_fire(cPlayer* pl);
static void wep17_r3_fire00(cPlayer* pl);
static void wep17_r3_fire10(cPlayer* pl);
static void wep17_r2_reload(cPlayer* pl);
static void wep17_r2_next(cPlayer* pl);
static void wep17_r2_out(cPlayer* pl);

// ready30 (the turn towards the lock target): positions the player is pulled to / turned to.
static Vec pos;
static Vec tgt;
// enemy found by ckEmWep (the out routine turns to it)
static cModel* pCkEm;

void Wep17_init(cModel* m)
{
    cPlayer* pl = (cPlayer*) m;
    cObjWep* obj = equipWeapon(pl);

    if (!VALID_PTR(obj)) {
        pLog->err(0, 0, "Wep17_init() wep model init failed.");
    } else {
        pl->Wep->m_pWep = obj;
        obj->setMotion(pl);
        if (pG->weapon_no == 3) {
            PSet(pl->pMotTbl[0x00], WEP_ARC_PTR(0x0A));
            PSet(pl->pMotTbl[0x01], (void*) 0);
            PSet(pl->pMotTbl[0x02], WEP_ARC_PTR(0x0D));
            PSet(pl->pMotTbl[0x03], WEP_ARC_PTR(0x1D));
            PSet(pl->pMotTbl[0x06], WEP_ARC_PTR(0x0F));
            PSet(pl->pMotTbl[0x07], WEP_ARC_PTR(0x1F));
            PSet(pl->pMotTbl[0x08], WEP_ARC_PTR(0x0E));
            PSet(pl->pMotTbl[0x09], WEP_ARC_PTR(0x1E));
            PSet(pl->pMotTbl[0x0B], WEP_ARC_PTR(0x10));
            PSet(pl->pMotTbl[0x0C], WEP_ARC_PTR(0x20));
            PSet(pl->pMotTbl[0x0D], WEP_ARC_PTR(0x0B));
            PSet(pl->pMotTbl[0x0E], WEP_ARC_PTR(0x1B));
            PSet(pl->pMotTbl[0x0F], WEP_ARC_PTR(0x0C));
            PSet(pl->pMotTbl[0x10], WEP_ARC_PTR(0x1C));
        }
        EspDataLoad((u32) WEP_ARC_PTR(0x4), 0x4B, 1);
        PlWepMot[0] = WEP_ARC_PTR(0x14);
        PlWepMot[1] = WEP_ARC_PTR(0x18);
        PlWepMot[2] = WEP_ARC_PTR(0x1A);
    }
}

void Wep17_move(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        wep17_r2_ready,
        wep17_r2_set,
        wep17_r2_fire,
        0,
        wep17_r2_reload,
        wep17_r2_next,
        wep17_r2_out,
    };

    func_tbl[pl->r_no_2](pl);
    pl->Wep->lockMove();
}

static void wep17_r2_ready(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        wep17_r3_ready00,
        wep17_r3_ready10,
        wep17_r3_ready20,
        wep17_r3_ready30,
    };

    pl->m_Work0 = 0;
    if (pl->r_no_3 == 0x64) {
        pl->r_no_3 = 0;
        pl->m_Work0 = 1;
    }
    if (Key.on & 1) {
        if (pl->Wep->knifeStance != 0) {
            pl->Wep->knifeStance = 0;
        }
    } else if (Key.on & 2) {
        if (pl->Wep->knifeStance != 2) {
            pl->Wep->knifeStance = 2;
        }
    } else {
        if (pl->Wep->knifeStance != 1) {
            pl->Wep->knifeStance = 1;
        }
    }
    func_tbl[pl->r_no_3](pl);
    if (joyKamae() == 0 && pl->r_no_3 != 3) {
        if (pl->flags_420 & 0x40) {
            pl->r_no_0 = 0;
            pl->r_no_2 = 0;
            pl->r_no_1 = 0x11;
            pl->r_no_3 = 0;
        } else {
            pl->r_no_0 = 0;
            pl->r_no_1 = 0;
            pl->r_no_2 = 0;
            pl->r_no_3 = 0;
            WEP_ATARI(pl)->clrFlag200();
        }
    }
    if (pl->keyReload() && WEP_OBJ(pl)->reloadable()) {
        pl->Wep->m_Flag |= 1;
        pl->r_no_0 = 0;
        pl->r_no_1 = 6;
        pl->r_no_2 = 4;
        pl->r_no_3 = 0;
        pl->m_Work0 = 1;
    } else if (pl->pLockEm) {
        CamCtrlShoulderSetAim(&pl->pLockEm->pos);
    } else {
        Vec aim = {0.0f, 1000.0f, 10000.0f};
        Vec hit;

        PSMTXMultVec(pl->mat, &aim, &aim);
        SatMgr.hitCheck(&pl->getPartsPtr(0)->world, &aim, &hit, 0, 0, 0);
        CamCtrlShoulderSetAim(&hit);
    }
}

// Alive enemy in front of the player (within a quarter turn) with a free line of sight: pCkEm.
int ckEmWep(cPlayer* pl)
{
    u32 i;

    pCkEm = 0;
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* em = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);

        if (!em->isAlive()) {
            continue;
        }
        if (em->checkThrow() == 0) {
            continue;
        }
        if (Front_check(pl, em, PI / 2.0f) == 0) {
            continue;
        }
        if (SatMgr.hitCheck(&pl->pParts->world, &em->pParts->world, 0, 0, 0, 0)) {
            continue;
        }
        pCkEm = em;
        return 1;
    }
    return 0;
}

// ready00: aim start. Outside the first stage (and not coming back from the out routine) the player
// facing a wall turns away from it (routine 6, r2_out: x3E0 = the free side) and an enemy in front is
// faced instead. Forms that matter: `md` (an int holding 1) is what wep.mode and the left tail's x3E4
// share (r23); the x3E0 store is written first in both tails: the later use of the same register is
// the one the scheduler issues early (its REG_DEAD lowers the register weight), so the earlier store
// ends up last, before the call.
static void wep17_r3_ready00(cPlayer* pl)
{
    const f32 zero = 0.0f;
    cObjWep* obj;
    f32 pitch;
    void* m;
    int md;

    pl->m_Work1 = 0;
    pl->Wep->m_CenterY = zero;
    pitch = CamCtrl.getCameraPitch();
    if (pitch > zero) {
        pitch += pitch;
    }
    pl->Wep->pitch = pitch;
    m3r[2] = zero;
    pitch *= 2.0f / PI;
    m3r[1] = pitch;
    m3r[0] = pitch;
    pl->m_Fwork0 = zero;
    pl->Neck->init(0, 0, 0);
    SndCall(2, 9, &pl->getPartsPtr(0xA)->world, 0, 0, 0);
    obj = WEP_OBJ(pl);
    md = 1;
    obj->wep.mode = md;
    obj->wep.step = 0;
    AtariFlagsOr(WEP_ATARI(pl), 0x200);
    if (pG->stage_no > 1 && pl->m_Work0 == 0 && (pG->Status_flg[3] & 0x08000000)) {
        Vec nrm;
        Vec v0;
        Vec v1;

        v0.x = zero;
        v0.y = 1000.0f;
        v0.z = zero;
        PSVECAdd(&v0, &pl->pos, &v0);
        v1.x = zero;
        v1.y = zero;
        v1.z = 1000.0f;
        RotVector(&v1, &pl->ang, &v1);
        PSVECAdd(&v1, &v0, &v1);
        if (SatMgr.hitCheck(&v0, &v1, 0, &nrm, 0, 0)) {
            v1.x = zero;
            v1.y = 500.0f;
            v1.z = zero;
            PSVECAdd(&v1, &pl->pos, &v1);
            v0.x = -1000.0f;
            v0.y = zero;
            v0.z = zero;
            RotVector(&v0, &pl->ang, &v0);
            PSVECAdd(&v0, &v1, &v0);
            if (SatMgr.hitCheck(&v1, &v0, 0, 0, 0, 0) == 0) {
                v1.x = zero;
                v1.y = zero;
                v1.z = 2000.0f;
                RotVector(&v1, &pl->ang, &v1);
                PSVECAdd(&v1, &v0, &v1);
                if (SatMgr.hitCheck(&v0, &v1, 0, 0, 0, 0) == 0) {
                    pl->m_Work0 = 0;
                    pl->m_Work1 = 1;
                    pl->r_no_2 = 6;
                    pl->r_no_3 = 0;
                    v0.x = -nrm.x;
                    v0.y = zero;
                    v0.z = -nrm.z;
                    pl->ang.y += Muku3(&v0, pl->ang.y, PI);
                    return;
                }
            }
            v1.x = 0.0f;
            v1.y = 500.0f;
            v1.z = 0.0f;
            PSVECAdd(&v1, &pl->pos, &v1);
            v0.x = 1000.0f;
            v0.y = 0.0f;
            v0.z = 0.0f;
            RotVector(&v0, &pl->ang, &v0);
            PSVECAdd(&v0, &v1, &v0);
            if (SatMgr.hitCheck(&v1, &v0, 0, 0, 0, 0) == 0) {
                v1.x = 0.0f;
                v1.y = 0.0f;
                v1.z = 2000.0f;
                RotVector(&v1, &pl->ang, &v1);
                PSVECAdd(&v1, &v0, &v1);
                if (SatMgr.hitCheck(&v0, &v1, 0, 0, 0, 0) == 0) {
                    pl->m_Work0 = 1;
                    pl->m_Work1 = 1;
                    pl->r_no_2 = 6;
                    pl->r_no_3 = 0;
                    v0.x = -nrm.x;
                    v0.y = 0.0f;
                    v0.z = -nrm.z;
                    pl->ang.y += Muku3(&v0, pl->ang.y, PI);
                    return;
                }
            }
        }
        if (ckEmWep(pl)) {
            int side = ((Key.on >> 2) ^ 1) & 1;

            pl->m_Work1 = 0;
            pl->r_no_3 = 0;
            pl->r_no_2 = 6;
            pl->m_Work0 = side;
            return;
        }
    }
    pl->Wep->lockInit();
    m = WEP_ARC_PTR(0x11);
    mot3.set(pl, m, m, m, 0, 3, 0, 4, 0);
    mot3.move(m3r[0]);
    pl->r_no_3 = 1;
}

static void wep17_r3_ready10(cPlayer* pl)
{
    if (pl->frame > 1.7f && pl->frame < 2.3f) {
        int se;

        if (pl->m_Work2 == 1) {
            se = 0x29;
        } else {
            se = 0x28;
        }
        SndCall(1, se, &pl->getPartsPtr(0)->world, 0, 0, 0);
    }
    MotionMoveI(pl, 0);
    if (pl->frame >= 4.0f) {
        pl->r_no_0 = 0;
        pl->r_no_1 = 6;
        pl->r_no_2 = 1;
        pl->r_no_3 = 4;
    }
    m3r[0] = m3r[0] * m3r[2] + m3r[1] * (1.0f - m3r[2]);
    mot3.move(m3r[0]);
    pl->Waist->set(0.0f, 0.4f);
}

static void wep17_r3_ready20(cPlayer* pl)
{
    MotionMoveI(pl, 0);
    if (pl->frame >= 4.0f) {
        pl->r_no_0 = 0;
        pl->r_no_1 = 6;
        pl->r_no_2 = 1;
        pl->r_no_3 = 4;
    }
    m3r[0] = m3r[0] * m3r[2] + m3r[1] * (1.0f - m3r[2]);
    mot3.move(m3r[0]);
    pl->Waist->set(0.0f, 0.4f);
}

// ready30: turn / step towards the aim target while the motion plays (wep/pl_handgun.cpp).
static void wep17_r3_ready30(cPlayer* pl)
{
    f32 dist;
    f32 x;
    f64 a;
    f32* r;
    Vec* t;

    if (MotionMoveI(pl, 0)) {
        SndCall(5, 0, &pl->getPartsPtr(0x14)->world, 0, 0, 0);
        pl->r_no_0 = 0;
        pl->r_no_1 = 6;
        pl->r_no_2 = 1;
        pl->r_no_3 = 0;
    }
    pl->ang.y += Muku(&pl->pos, &tgt, pl->ang.y, PI / 8.0f);
    pl->pos.x = pl->pos.x * 0.6f + pos.x * 0.4f;
    pl->pos.z = pl->pos.z * 0.6f + pos.z * 0.4f;
    t = &tgt;
    dist = GetDistance3(&pos, t);
    a = atan2(t->y - pos.y, dist);
    r = m3r;
    x = a / (PI / 4.0f) - r[0];
    if (x > 0.05f) {
        x = 0.05f;
    }
    if (x < -0.05f) {
        x = -0.05f;
    }
    r[1] += x;
    if (r[2] == 0.0f) {
        m3r[0] = r[1];
    }
    {
        f32 lo = -1.0f;
        f32 hi = 1.0f;

        if (r[1] < lo) {
            r[1] = lo;
        } else if (r[1] > hi) {
            r[1] = hi;
        }
    }
    if (r[2] == 0.0f) {
        r[0] = r[1];
    }
    m3r[0] = m3r[0] * m3r[2] + m3r[1] * (1.0f - m3r[2]);
    mot3.move(m3r[0]);
    pl->Waist->set(0.0f, 0.4f);
}

static void wep17_r2_set(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        wep17_r3_set00,
        wep17_r3_set10,
        wep17_r3_set20,
        wep17_r3_set30,
        wep17_r3_set40,
    };
    int fire;

    func_tbl[pl->r_no_3](pl);
    if (pl->r_no_3 != 4) {
        PlWepLockCtrl(pl);
    }
    pl->setLaserSight(1, 0);
    if (joyKamae() == 0) {
        if ((pl->flags_420 & 0x40) == 0) {
            wepDown(pl);
            return;
        }
        pl->r_no_0 = 0;
        pl->r_no_2 = 0;
        pl->r_no_1 = 0x11;
        pl->r_no_3 = 0;
    }
    fire = joyFireTrg();
    if (fire) {
        fire = WEP_OBJ(pl)->bulletNum();
        if (fire) {
            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = 2;
            pl->r_no_3 = 0;
            return;
        }
        if (WEP_OBJ(pl)->reloadable()) {
            pl->Wep->m_Flag |= 1;
            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = 4;
            pl->r_no_3 = 0;
            pl->m_Work0 = fire;
            return;
        }
        SndCall(2, 0x17, &pl->getPartsPtr(4)->world, 0, 0, 0);
    } else if (joyFireOn() && WEP_OBJ(pl)->bulletNum()) {
        pl->r_no_0 = 0;
        pl->r_no_1 = 6;
        pl->r_no_2 = 2;
        pl->r_no_3 = 0;
        return;
    }
    if (pl->keyReload() && WEP_OBJ(pl)->reloadable()) {
        pl->Wep->m_Flag |= 1;
        pl->r_no_0 = 0;
        pl->r_no_1 = 6;
        pl->r_no_2 = 4;
        pl->r_no_3 = 0;
        pl->m_Work0 = 1;
    }
}

static void wep17_r3_set00(cPlayer* pl)
{
    PlArc* arc = (PlArc*) pG->pWep;

    mot3.set(pl, PL_ARC_PTR(arc, 0x12), PL_ARC_PTR(arc, 0x17), PL_ARC_PTR(arc, 0x19), 0, 3, 0, 4, 0);
    mot3.move(m3r[0]);
    pl->motionMove();
    pl->m_Work4 = 0;
    pl->r_no_3 = 1;
}

static void wep17_r3_set10(cPlayer* pl)
{
    MotionMoveI(pl, 0);
}

static void wep17_r3_set20(cPlayer* pl)
{
    if ((Key.on & 4) == 0) {
        pl->r_no_3 = 0;
    }
    MotionMoveI(pl, 0);
    if (pl->frame > 9.7f && pl->frame < 10.3f) {
        SndCall(5, 0, &pl->getPartsPtr(0x14)->world, 0, 0, 0);
    }
    if (pl->frame > 22.7f && pl->frame < 23.3f) {
        SndCall(5, 1, &pl->getPartsPtr(0x18)->world, 0, 0, 0);
    }
}

static void wep17_r3_set30(cPlayer* pl)
{
    if ((Key.on & 8) == 0) {
        pl->r_no_3 = 0;
    }
    MotionMoveI(pl, 0);
    if (pl->frame > 9.7f && pl->frame < 10.3f) {
        SndCall(5, 0, &pl->getPartsPtr(0x14)->world, 0, 0, 0);
    }
    if (pl->frame > 22.7f && pl->frame < 23.3f) {
        SndCall(5, 1, &pl->getPartsPtr(0x18)->world, 0, 0, 0);
    }
}

static void wep17_r3_set40(cPlayer* pl)
{
    if (pl->motionMove() || (Key.on & 0x10F)) {
        pl->r_no_3 = 0;
    }
}

static void wep17_r2_fire(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        wep17_r3_fire00,
        wep17_r3_fire10,
    };

    func_tbl[pl->r_no_3](pl);
    PlWepLockCtrl(pl);
}

static void wep17_r3_fire00(cPlayer* pl)
{
    cModel* parts;
    cObjWep* obj;
    void* m0;
    void* m1;
    void* m2;
    Vec p0;
    Vec p1;
    f32 pitch;

    WEP_OBJ(pl)->trigger();
    if (pG->weapon_no == 0x11) {
        m0 = WEP_ARC_PTR(0x3D);
        m1 = WEP_ARC_PTR(0x3E);
        m2 = WEP_ARC_PTR(0x3F);
    } else if (pG->weapon_type == 2) {
        m0 = WEP_ARC_PTR(0x14);
        m1 = WEP_ARC_PTR(0x18);
        m2 = WEP_ARC_PTR(0x1A);
    } else {
        m0 = WEP_ARC_PTR(0x21);
        m1 = WEP_ARC_PTR(0x22);
        m2 = WEP_ARC_PTR(0x23);
    }
    mot3.set(pl, m0, m1, m2, 0, 0, 0, 4, 0);
    mot3.move(m3r[0]);
    MotionMoveI(pl, 0);
    m3r[0] = m3r[0] * m3r[2] + m3r[1] * (1.0f - m3r[2]);
    mot3.move(m3r[0]);
    pl->Waist->set(pl->m_Fwork0, 0.4f);
    WEP_ATARI(pl)->clrFlag200();
    pl->Body->waistMove();
    pl->partsWorldCalc();
    parts = pl->getPartsPtr(10);
    p0.x = 234.5f;
    p0.y = -24.0f;
    p0.z = 38.33f;
    PSMTXMultVec(parts->mat, &p0, &p0);
    p1.x = -50000.0f;
    p1.y = fRand1_1() * 200.0f;
    p1.z = fRand1_1() * 200.0f;
    PSMTXMultVecSR(parts->mat, &p1, &p1);
    PSVECAdd(&p0, &p1, &p1);
    PlWepHitCheck2(pl, &p0, &p1, pG->weapon_no, 0, 6000.0f);
    pl->m_Work4++;
    obj = WEP_OBJ(pl);
    obj->wep.mode = 2;
    obj->wep.step = 0;
    pitch = m3r[0];
    PlWepLockRand(pl, 2, &pitch, &pl->m_Fwork0);
    m3r[1] = pitch;
    if (m3r[2] == 0.0f) {
        m3r[0] = pitch;
    }
    pl->r_no_3 = 1;
}

// fire10: the VP70 fires up to three shots on the held trigger (x3F0 counts them).
static void wep17_r3_fire10(cPlayer* pl)
{
    pl->motionMove();
    if (joyKamae() && joyFireOn() && pG->weapon_no == 0x11 && MotionCheckCrossFrame(&pl->Motion, 3.0f)
        && pl->m_Work4 <= 2 && WEP_OBJ(pl)->bulletNum()) {
        pl->r_no_3 = 0;
    }
    if (MotionCheckCrossFrame(&pl->Motion, 11.0f)) {
        pl->m_Work4 = 0;
        WEP_ATARI(pl)->setFlag200();
        pl->r_no_0 = 0;
        pl->r_no_1 = 6;
        pl->r_no_2 = 1;
        pl->r_no_3 = 4;
    }
}

void wepDown(cPlayer* pl)
{
    cObjWep* obj;

    if (dmMotCk()) {
        MotionSetCore(pl, &pl->Motion, WEP_ARC_PTR(0x15), 0, 3, 5, 0);
        pl->r_no_0 = 0;
        pl->r_no_1 = 0;
        pl->r_no_2 = 2;
        pl->r_no_3 = 0;
    } else {
        pl->r_no_3 = 1;
        pl->x4FD = 0xF;
        pl->r_no_0 = 0;
        pl->r_no_1 = 0;
        pl->r_no_2 = 0;
        pl->x4FC = 0;
    }
    pl->motionMove();
    obj = WEP_OBJ(pl);
    obj->wep.mode = 3;
    obj->wep.step = 0;
    WEP_ATARI(pl)->clrFlag200();
    FSet(pl->ang.y, pl->ang.y - pl->Waist->set(0.0f, 0.4f));
}

static void wep17_r2_reload(cPlayer* pl)
{
    u8 step = pl->r_no_3;
    cObjWep* obj;
    void* mot;

    switch (step) {
    case 0:
        if (pG->weapon_no == 0x11) {
            switch (pG->weapon_lv_reload) {
            default:
                mot = WEP_ARC_PTR(0x40);
                break;
            case 1:
                mot = WEP_ARC_PTR(0x27);
                break;
            case 2:
                mot = WEP_ARC_PTR(0x2A);
                break;
            }
        } else if (pG->weapon_type == 2) {
            mot = WEP_ARC_PTR(0x16);
        } else {
            mot = WEP_ARC_PTR(0x24);
        }
        MotionSetCore(pl, &pl->Motion, mot, 0, 3, 5, 0);
        MotionMoveI(pl, 0);
        pl->Wep->knifeStance = 1;
        pl->r_no_3 = 1;
        obj = WEP_OBJ(pl);
        obj->wep.mode = 4;
        obj->wep.step = 0;
        break;
    case 1:
        if (MotionMoveI(pl, 0)) {
            if (joyKamae()) {
                pl->r_no_0 = 0;
                pl->r_no_1 = 6;
                pl->r_no_2 = 1;
                pl->r_no_3 = 0;
            } else if (pl->flags_420 & 0x40) {
                pl->r_no_0 = 0;
                pl->r_no_2 = 0;
                pl->r_no_1 = 0x11;
                pl->r_no_3 = 0;
            } else {
                wepDown(pl);
            }
        }
        break;
    }
}

// next: turn to the next lock target (pLockEm); a target far round the back is turned to through
// the waist first (step 2).
static void wep17_r2_next(cPlayer* pl)
{
    u8 step = pl->r_no_3;
    cModel* em = pl->pLockEm;
    f32 ang;

    switch (step) {
    case 0:
        MotionMoveI(pl, 0);
        pl->m_Work0 = 0;
        pl->m_Work1 = 0;
        if (em) {
            ang = Muku(&pl->pos, &em->pos, pl->ang.y, PI);
        } else {
            ang = 0.0f;
        }
        if (ang < cPlWaist::ROT_LIMIT && ang > -cPlWaist::ROT_LIMIT) {
            pl->m_Fwork0 = ang;
            pl->r_no_3 = 2;
        } else {
            if (ang > 3.0f * PI / 4.0f || ang < -3.0f * PI / 4.0f) {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 1, 0);
            } else {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 1, 0);
            }
            pl->m_Work1 = 1;
            pl->r_no_3 = 1;
        }
        break;
    case 1:
        if (GetDistance3(&pl->pos, &em->pos) > 200.0f) {
            pl->ang.y += Muku(&pl->pos, &em->pos, pl->ang.y, 0.31415927f);
            pl->ang.y = LIMIT_ANGLE(pl->ang.y);
        }
        pl->Waist->set(0.0f, 0.4f);
        if (pl->m_Work1 == 0) {
            pl->partsFixMemory(0x19);
        }
        MotionMoveI(pl, 0);
        if ((int) pl->m_Work0 > 9) {
            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = 1;
            pl->r_no_3 = 0;
        }
        pl->m_Work0++;
        break;
    case 2:
        MotionMoveI(pl, 0);
        if (fabsf(pl->Waist->set(pl->m_Fwork0, 0.4f)) < 0.01f) {
            pl->Waist->m_Ang.y = pl->m_Fwork0;
            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = 1;
            pl->r_no_3 = 1;
        }
        break;
    }
    if (Key.trg & 0x20) {
        Vec p = pl->getPartsPtr(3)->world;

        em = SearchLockEm(&p, pl->pLockEm);
        if (em) {
            pl->pLockEm = em;
            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = 5;
            pl->r_no_3 = 0;
        } else {
            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = 1;
            pl->r_no_3 = 0;
        }
    } else if (joyKamae() == 0) {
        if (pl->flags_420 & 0x40) {
            pl->r_no_0 = 0;
            pl->r_no_2 = 0;
            pl->r_no_1 = 0x11;
            pl->r_no_3 = 0;
        } else {
            int md = 1;

            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = md;
            pl->r_no_3 = 0;
        }
    }
}

// out: the player with his back to a wall (ready00) turns round / steps out before aiming; the
// motions of the steps are not in the archive (null motion pointers).
static void wep17_r2_out(cPlayer* pl)
{
    switch (pl->r_no_3) {
    case 0:
        if (pl->m_Work0 == 0) {
            MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
        } else {
            MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
        }
        pl->r_no_3 = 1;
    case 1:
        if ((int) pl->frameMax > (int) pl->frameMax - 7 && pCkEm) {
            pl->ang.y += Muku(&pl->pos, &pCkEm->pos, pl->ang.y, 0.31415927f);
        }
        if (MotionMoveI(pl, 0)) {
            if (pl->m_Work0 == 0) {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
            } else {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
            }
            pl->r_no_3 = 2;
            pl->m_Fwork0 = pl->ang.y;
        }
        break;
    case 2:
        if (Key.on & 4) {
            pl->ang.y -= 0.05235988f;
        }
        if (Key.on & 8) {
            pl->ang.y += 0.05235988f;
        }
        MotionMoveI(pl, 0);
        if (Key.trg & 0x40000000) {
            pl->m_Work1 = 0;
        }
        if (joyKamae() == 0) {
            if (pl->m_Work1 == 1) {
                if (pl->m_Work0 == 0) {
                    MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 5);
                } else {
                    MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 5);
                }
                pl->r_no_3 = 6;
                pl->ang.y = pl->m_Fwork0;
            } else {
                if (pl->m_Work0 == 0) {
                    MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
                } else {
                    MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
                }
                pl->r_no_3 = 5;
            }
        } else if (joyFireOn()) {
            cModel* parts;
            Vec p0;
            Vec p1;
            int zero;

            zero = 0;
            BitOn(pG->Status_flg[0], 0x00800000);
            SndCall(2, 0, &pl->getPartsPtr(4)->world, 0, 0, 0);
            VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 0, 1);
            EstSet((int) WEP_OBJ(pl), -1, 0, 0, 0x4B, 0, 0, 0xA, zero, 0);
            parts = pl->getPartsPtr(0xA);
            p0.x = 234.5f;
            p0.y = -24.0f;
            p0.z = 38.33f;
            PSMTXMultVec(parts->mat, &p0, &p0);
            p1.x = -50000.0f;
            p1.y = fRand1_1() * 200.0f;
            p1.z = fRand1_1() * 200.0f;
            PSMTXMultVecSR(parts->mat, &p1, &p1);
            PSVECAdd(&p0, &p1, &p1);
            PlWepHitCheck2(pl, &p0, &p1, pG->weapon_no, 0, 6000.0f);
            if (pl->m_Work0 == 0) {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
            } else {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
            }
            pl->r_no_3 = 3;
        }
        break;
    case 3: {
        int end;

        if (pl->frame >= 5.0f) {
            if (Key.on & 4) {
                pl->ang.y -= 0.05235988f;
            }
            if (Key.on & 8) {
                pl->ang.y += 0.05235988f;
            }
        }
        end = MotionMoveI(pl, 0);
        if (pl->frame > (f32) (pl->frameMax - 5) && joyFireOn()) {
            end |= 1;
        }
        if (end) {
            if (pl->m_Work0 == 0) {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
            } else {
                MotionSetCore(pl, &pl->Motion, 0, 0, 3, 5, 0);
            }
            pl->r_no_3 = 2;
        }
        break;
    }
    case 5:
        CamCtrl.resetCameraAngle();
        if ((Key.on & 0x10F) || MotionMoveI(pl, 0)) {
            pl->r_no_0 = 0;
            pl->r_no_1 = 0;
            pl->r_no_2 = 0;
            pl->r_no_3 = 0;
            WEP_ATARI(pl)->clrFlag200();
        }
        break;
    case 6:
        if ((int) pl->frameMax > (int) pl->frameMax - 7 && pCkEm) {
            pl->ang.y += Muku(&pl->pos, &pCkEm->pos, pl->ang.y, 0.31415927f);
        }
        if (MotionMoveI(pl, 0)) {
            pl->r_no_0 = 0;
            pl->r_no_1 = 0;
            pl->r_no_2 = 0;
            pl->r_no_3 = 0;
            WEP_ATARI(pl)->clrFlag200();
        }
        break;
    }
}

cObjWep* equipWeapon(cPlayer* pl)
{
    cObjWep* obj;
    int id;

    switch (pG->weapon_no) {
    case 3:
    default:
        id = 0x27;
        break;
    case 0x11:
        id = 0x34;
        break;
    }
    obj = (cObjWep*) ObjMgr.createBack(id);
    if (obj == 0) {
        pLog->err(0, 0, "Wep17_init() cObjWep CREATE FAILED");
        return 0;
    }
    obj->init(pl);
    return obj;
}

extern "C" void _prolog()
{
    WeaponInitFunc = Wep17_init;
    WeaponMoveFunc = Wep17_move;
    ObjInitFunc[0x27] = ObjMauser_init;
    ObjInitFunc[0x34] = ObjVp70_init;
    OSReport("Wep17 VP70 prolog Ok\n");
}

extern "C" void _epilog()
{
    ObjInitFunc[0x27] = 0;
    ObjInitFunc[0x34] = 0;
}

extern "C" void _unresolved()
{
}
