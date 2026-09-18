// game/pl_knife.cpp: player routine 2 (knife): ready stance, set (idle/turn), fire (slash + hit check), down.

#include "atari.h"
#include "light.h"
#include "player.h"
#include "global.h"
#include "main.h"
#include "cam_ctrl.h"
#include "item.h"
#include "snd.h"
#include "esp.h"
#include "math_sub.h"

extern "C" {
int MotionMove(cModel* m, int flag);               // game/motion.cpp
int MotionCheckCrossFrame(void* work, f32 frame);  // game/motion.cpp
}

#define VALID_PTR(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)

void knife_r3_ready00(cPlayer* pl);
void knife_r3_ready10(cPlayer* pl);
void knife_r3_set00(cPlayer* pl);
void knife_r3_set10(cPlayer* pl);
void knife_r3_set20(cPlayer* pl);
void knife_r3_set30(cPlayer* pl);
static void knife_r3_set40(cPlayer* pl);
void hitCheck(cPlayer* pl, int no, u32 flag);
void knife_r3_fire00(cPlayer* pl);
void knife_r3_fire10(cPlayer* pl);
void knife_r3_down00(cPlayer* pl);
void knife_r3_down10(cPlayer* pl);

// face model info: the face blend weights (0x5C/0x70/0x84) are reset to `v`
#define FACE_SET(pl, v)                                 \
    do {                                                \
        cModelInfo* face = (pl)->Body->pFace;          \
        if (VALID_PTR(face)) {                          \
            face->x84 = v;                              \
            face->x70 = v;                              \
            face->x5C = v;                              \
        }                                               \
    } while (0)

void PlKnifeMove(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        knife_r2_ready,
        knife_r2_set,
        knife_r2_fire,
        knife_r2_down,
    };

    func_tbl[pl->r_no_2](pl);
    pl->Wep->lockMove();
    pl->checkXbutton();
}

void knife_r2_ready(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        knife_r3_ready00,
        knife_r3_ready10,
    };

    pl->m_Work0 = 0;
    if (pl->r_no_3 == 100) {
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
    if (joyLKamae() == 0 && pl->r_no_3 != 3) {
        setWepTrans(pl, 1);
        FACE_SET(pl, 0.0f);
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
        }
    } else {
        if (pl->pLockEm) {
            CamCtrlShoulderSetAim(&pl->pLockEm->pos);
        } else {
            Vec aim = {0.0f, 1000.0f, 10000.0f};
            Vec hit;

            PSMTXMultVec(pl->mat, &aim, &aim);
            SatMgr.hitCheck(&pl->getPartsPtr(0)->world, &aim, &hit, 0, 0, 0);
            CamCtrlShoulderSetAim(&hit);
        }
    }
}

void knife_r3_ready00(cPlayer* pl)
{
    f32 pitch;
    void* mot0;
    void* mot1;

    pl->Wep->m_CenterY = 0.0f;
    pitch = CamCtrl.getCameraPitch();
    if (pitch > 0.0f) {
        pitch += pitch;
    }
    pl->Wep->pitch = pitch;
    m3r[2] = 0.0f;
    pitch *= 2.0f / PI;
    m3r[1] = pitch;
    m3r[0] = pitch;
    pl->m_Fwork0 = 0.0f;
    pl->Neck->init(0, 0, 0);
    if ((G_WEP_ID & 0xFFFF0000) == 0x0D020000) {
        mot0 = pl->pMotTbl[0x59];
        mot1 = pl->pMotTbl[0x5A];
    } else if (ItemMgr.bulletNum() && (Key.on & 0x10) && pG->weapon_no == 0xD) {
        mot0 = pl->pMotTbl[0x55];
        mot1 = pl->pMotTbl[0x56];
    } else {
        mot0 = PL_ARC_PTR(pG->pPlayer, 0x23);
        mot1 = PL_ARC_PTR(pG->pPlayer, 0x24);
    }
    mot3.set(pl, mot0, mot0, mot0, (int) mot1, 3, 0, 4, 0);
    mot3.move(m3r[0]);
    pl->motionMove();
    lockCtr = 0;
    pl->r_no_3 = 1;
}

void knife_r3_ready10(cPlayer* pl)
{
    if (MotionCheckCrossFrame(&pl->pMotion, 4.0f)) {
        setWepTrans(pl, 0);
        FACE_SET(pl, 1.0f);
    }
    if ((G_WEP_ID & 0xFFFF0000) == 0x0D020000) {
        if (MotionCheckCrossFrame(&pl->pMotion, 10.0f)) {
            ((cObjLauncher*) pl->Wep->m_pWep)->gripBack();
            pl->r_no_2 = 1;
            pl->r_no_3 = 4;
        }
    } else {
        if (pl->frame >= 10.0f) {
            pl->r_no_2 = 1;
            pl->r_no_3 = 4;
        }
    }
    mot3.move(m3r[0]);
    pl->motionMove();
}

void knife_r2_set(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        knife_r3_set00,
        knife_r3_set10,
        knife_r3_set20,
        knife_r3_set30,
        knife_r3_set40,
    };

    func_tbl[pl->r_no_3](pl);
    if (pl->r_no_3 != 4) {
        PlWepLockCtrl(pl);
    }
    if (joyLKamae() == 0) {
        if (pl->flags_420 & 0x40) {
            pl->r_no_0 = 0;
            pl->r_no_2 = 0;
            pl->r_no_1 = 0x11;
            pl->r_no_3 = 0;
        } else if (pG->weapon_no == 0xD && joyKamae()) {
            pl->r_no_0 = 0;
            pl->r_no_3 = 2;
            pl->r_no_2 = 0;
            pl->r_no_1 = 6;
        } else {
            pl->r_no_0 = 0;
            pl->r_no_1 = 0xB;
            pl->r_no_2 = 3;
            pl->r_no_3 = 0;
        }
    } else if (joyFireTrg() || joyFireOn()) {
        pl->r_no_2 = 2;
        pl->r_no_3 = 0;
    }
}

void knife_r3_set00(cPlayer* pl)
{
    PlArc* arc = pG->pPlayer;

    mot3.set(pl, PL_ARC_PTR(arc, 0x81), PL_ARC_PTR(arc, 0x83), PL_ARC_PTR(arc, 0x85), 0, 3, 0, 4, 0);
    mot3.move(m3r[0]);
    pl->motionMove();
    pl->r_no_3 = 1;
}

void knife_r3_set10(cPlayer* pl)
{
    pl->motionMove();
}

void knife_r3_set20(cPlayer* pl)
{
    if ((Key.on & 4) == 0) {
        pl->r_no_3 = 0;
    }
    MotionMove(pl, 0);
    if (pl->frame > 9.7f && pl->frame < 10.3f) {
        SndCall(5, 0, &pl->getPartsPtr(0x14)->world, 0, 0, 0);
    }
    if (pl->frame > 22.7f && pl->frame < 23.3f) {
        SndCall(5, 1, &pl->getPartsPtr(0x18)->world, 0, 0, 0);
    }
}

void knife_r3_set30(cPlayer* pl)
{
    if ((Key.on & 8) == 0) {
        pl->r_no_3 = 0;
    }
    MotionMove(pl, 0);
    if (pl->frame > 9.7f && pl->frame < 10.3f) {
        SndCall(5, 0, &pl->getPartsPtr(0x14)->world, 0, 0, 0);
    }
    if (pl->frame > 22.7f && pl->frame < 23.3f) {
        SndCall(5, 1, &pl->getPartsPtr(0x18)->world, 0, 0, 0);
    }
}

static void knife_r3_set40(cPlayer* pl)
{
    if (MotionMove(pl, 0) || (Key.on & 0x10F)) {
        pl->r_no_3 = 0;
    }
}

void knife_r2_fire(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        knife_r3_fire00,
        knife_r3_fire10,
    };

    func_tbl[pl->r_no_3](pl);
    PlWepLockCtrl(pl);
}

void hitCheck(cPlayer* pl, int no, u32 flag)
{
    static Vec ohpos;
    Vec p0;
    Vec p1;
    f32 len;
    cModel* parts;

    switch (pG->pl_type) {
    case 0:
    default:
        len = 750.0f;
        break;
    case 4:
        len = 1200.0f;
        break;
    }
    parts = pl->getPartsPtr(2);
    p0.x = 0.0f;
    p0.y = 0.0f;
    p0.z = 0.0f;
    PSMTXMultVec(parts->mat, &p0, &p0);
    parts = pl->getPartsPtr(9);
    p1.x = -len;
    p1.y = 0.0f;
    p1.z = 0.0f;
    PSMTXMultVec(parts->mat, &p1, &p1);
    if (!(flag & 8)) {
        Vec d;

        PSVECSubtract(&p1, &ohpos, &d);
        PSVECScale(&d, &d, 0.2f);
        PSVECAdd(&ohpos, &d, &ohpos);
        PlWepHitCheck2(pl, &p0, &ohpos, 0x10, flag | 1, 6000.0f);
        PSVECAdd(&ohpos, &d, &ohpos);
        PlWepHitCheck2(pl, &p0, &ohpos, 0x10, flag | 1, 6000.0f);
        PSVECAdd(&ohpos, &d, &ohpos);
        PlWepHitCheck2(pl, &p0, &ohpos, 0x10, flag | 1, 6000.0f);
        PSVECAdd(&ohpos, &d, &ohpos);
        PlWepHitCheck2(pl, &p0, &ohpos, 0x10, flag | 1, 6000.0f);
    }
    ohpos = p1;
    flag &= ~8;
    PlWepHitCheck2(pl, &p0, &p1, 0x10, flag | 1, 6000.0f);
    p0.y += 200.0f;
    p1.y += 400.0f;
    PlWepHitCheck2(pl, &p0, &p1, 0x10, flag | 1, 6000.0f);
    p0.y -= 300.0f;
    p1.y -= 600.0f;
    PlWepHitCheck2(pl, &p0, &p1, 0x10, flag | 1, 6000.0f);
}

void knife_r3_fire00(cPlayer* pl)
{
    PlArc* arc = pG->pPlayer;

    mot3.set(pl, PL_ARC_PTR(arc, 0x82), PL_ARC_PTR(arc, 0x84), PL_ARC_PTR(arc, 0x86), 0, 3, 0, 4, 0);
    m3r[0] = m3r[0] * m3r[2] + m3r[1] * (1.0f - m3r[2]);
    mot3.move(m3r[0]);
    MotionMove(pl, 0);
    pl->Waist->set(pl->m_Fwork0, 0.4f);
    EstSet((int) pl, -1, 0, 0, 0, 0x2B, 0, 0xA, 0, 0);
    pl->Body->waistMove();
    pl->partsWorldCalc();
    pl->r_no_3 = 1;
}

void knife_r3_fire10(cPlayer* pl)
{
    f32 st = 6.0f;   // unused: only order the constant pool (6.0, 10.0 before 3.0)
    f32 ed = 10.0f;
    f32 wh;

    if (MotionCheckCrossFrame(&pl->pMotion, 3.0f)) {
        SndCall(1, 3, &pl->getPartsPtr(4)->world, 0, 0, 0);
    }
    pl->motionMove();
    if (pl->frame >= 6.0f && pl->frame <= 10.0f) {
        u32 flag = 0;

        if (!(pl->frame >= 7.0f && pl->frame <= 8.0f)) {
            flag = 4;
        }
        if (pl->frame > 5.7f && pl->frame < 6.3f) {
            flag |= 8;
        }
        hitCheck(pl, (int) (pl->frame - 6.0f), flag);
    }
    if (MotionCheckCrossFrame(&pl->pMotion, 6.0f)) {
        Vec* pos = &pl->getPartsPtr(10)->world;

        if (GetWaterHeight(pos, &wh) && pos->y < wh + 100.0f) {
            if ((G_ROOM_ID32 & 0xFFFF0000) == 0x010A0000 || (G_ROOM_ID32 & 0xFFFF0000) == 0x011A0000) {
                EstSet((int) pl, -1, 0, 0, 1, 0x25, 0, 0, (u32) pl, 0);
            } else {
                EstSet((int) pl, -1, 0, 0, 3, 0, 0, 0, (u32) pl, 0);
            }
            SndCall(1, 0x51, &pl->getPartsPtr(10)->world, 0, 0, 0);
        }
    }
    if (pl->frame >= (f32) (pl->frameMax - 2)) {
        pl->r_no_2 = 1;
        pl->r_no_3 = 4;
    }
}

void knife_r2_down(cPlayer* pl)
{
    static void (*func_tbl[])(cPlayer*) = {
        knife_r3_down00,
        knife_r3_down10,
    };

    func_tbl[pl->r_no_3](pl);
    FSet(pl->ang.y, pl->ang.y - pl->Waist->set(0.0f, 0.4f));
    BitOn(pG->Status_flg[0], 0x2000000);
    pl->checkCtrl();
}

void knife_r3_down00(cPlayer* pl)
{
    void* mot0;
    void* mot1;

    if (joyKamae()) {
        cObjWep* obj = pl->Wep->m_pWep;

        int on = 1;

        obj->wep.mode = on;
        obj->wep.step = 0;
        mot0 = pl->pMotTbl[0x57];
        mot1 = pl->pMotTbl[0x58];
        pl->m_Work0 = on;
    } else {
        if (dmMotCk()) {
            mot0 = pl->pMotTbl[0x5B];
            mot1 = pl->pMotTbl[0x5C];
        } else {
            mot0 = PL_ARC_PTR(pG->pPlayer, 0x87);
            mot1 = 0;
        }
        pl->m_Work0 = 0;
    }
    if (mot0 == 0) {
        FACE_SET(pl, 0.0f);
        setWepTrans(pl, 1);
        pl->r_no_0 = 0;
        pl->r_no_1 = 0;
        pl->r_no_2 = 0;
        pl->r_no_3 = 0;
    } else {
        pl->motionSet(mot0, 5, 0, ((G_WEP_ID & 0xFFFF0000) == 0x0E000000) ? 0x100 : 0, (int) mot1);
        pl->motionMove();
        pl->r_no_3 = 1;
    }
}

// back to routine 0 (both exits of down10 share this tail)
#define KNIFE_RESET(pl)   \
    do {                  \
        (pl)->r_no_0 = 0;    \
        (pl)->r_no_1 = 0;    \
        (pl)->r_no_2 = 0;    \
        (pl)->r_no_3 = 0;    \
    } while (0)

void knife_r3_down10(cPlayer* pl)
{
    if (MotionCheckCrossFrame(&pl->pMotion, 4.0f)) {
        FACE_SET(pl, 0.0f);
        setWepTrans(pl, 1);
    }
    if (MotionCheckCrossFrame(&pl->pMotion, 15.0f)) {
        if ((G_WEP_ID & 0xFFFF0000) == 0x0D020000) {
            ((cObjLauncher*) pl->Wep->m_pWep)->grip(0);
        }
    }
    if (pl->motionMove()) {
        if (pl->m_Work0 == 0) {
            KNIFE_RESET(pl);
        } else {
            pl->r_no_0 = 0;
            pl->r_no_1 = 6;
            pl->r_no_2 = 1;
            pl->r_no_3 = 0;
            pl->Wep->pitch = 0.0f;
            m3r[1] = 0.0f;
            m3r[0] = 0.0f;
        }
    } else if ((Key.on & 0x10F) || (pl->m_Work0 != 0 && joyKamae() == 0) || (pl->m_Work0 == 0 && joyKamae() != 0)) {
        FACE_SET(pl, 0.0f);
        setWepTrans(pl, 1);
        if ((G_WEP_ID & 0xFFFF0000) == 0x0D020000) {
            ((cObjLauncher*) pl->Wep->m_pWep)->grip(0);
        }
        KNIFE_RESET(pl);
    }
}

void setWepTrans(cPlayer* pl, int on)
{
    switch (pG->weapon_no) {
    default:
        pl->Wep->m_pWep->setDisp(1, on);
        break;
    case 0x13:
    case 0x16:
    case 0x17:
    case 0x19:
    case 0x1F:
    case 0x20:
        pl->Wep->pObj2->setDisp(1, on);
        break;
    case 0xD:
        break;
    }
}
