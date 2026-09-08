// game/emBarred.cpp: barred gate enemy (cEmBarred): iron gates that rise for the player when he
// stands near, drop back shut, and can be shot open (type 6).

#include "atari.h"
#include "light.h"
#include "dmg.h"
#include "emBarred.h"
#include "emhit.h"
#include "etc_model.h"
#include "at_mod.h"
#include "player.h"
#include "esp.h"
#include "snd.h"
#include "rnd.h"
#include "global.h"
#include "math_sub.h"
#include "db_log.h"

extern cModel* pSUB;   // game/em.cpp

typedef void (*EmBarredFunc)(cEmBarred*);

EmBarredFunc EmBarred_R1_move_tbl[4] = {
    emBarred_R1_Set,
    emBarred_R1_Open,
    emBarred_R1_Close,
    emBarred_R1_Break,
};

// Closes again when the player leaves (setNoClose clears this).
static inline int emBarredCanClose(EmBarredWork* w)
{
    return !(w->flags & 1);
}

cEmBarred* SetEmBarred(void* bin, void* tpl, Vec* pos, Vec* rot, int flagNo, int type)
{
    cEmBarred* em;
    EmBarredWork* w;
    u16* flg;
    cModel* parts;
    int i;

    flg = GetEtcFlgPtr(flagNo, pG->room_id);
    if (flg != 0 && (*flg & 1)) {
        return 0;
    }
    em = (cEmBarred*) EmMgr.create(0x4E);
    if (em == 0) {
        return 0;
    }
    w = EMBARRED_WK(em);
    w->flagNo = flagNo;
    em->type = type;
    if (type == 6) {
        em->x12F = 1;
    }
    if (em->modelInit(bin, tpl) == 0) {
        pLog->err(0, 0, "SetEmBarred() failed.");
        EmMgr.destroy(em);
        return 0;
    }
    {
        static const Vec ofs = { 0.0f, 0.0f, 0.0f };
        static const Vec size = { 3000.0f, 3000.0f, 3000.0f };

        em->lightInfo.init2(0, 1, &ofs, &size, 0x10);
    }
    switch (em->type) {
    case 2:
        em->atari.init(0, 2, 0, 0.0f, 1300.0f, 0.0f, 800.0f, 120.0f, 120.0f, 1300.0f);
        break;
    case 3:
        em->atari.init(0, 2, 0, 0.0f, 2400.0f, 0.0f, 4150.0f, 230.0f, 230.0f, 2400.0f);
        break;
    case 4:
        em->atari.init(0, 2, 0, 0.0f, 1500.0f, 0.0f, 1500.0f, 230.0f, 230.0f, 1500.0f);
        break;
    case 7:
        em->atari.init(0, 2, 0, 0.0f, 1750.0f, 0.0f, 1950.0f, 230.0f, 230.0f, 1750.0f);
        break;
    case 8:
        em->atari.init(0, 2, 0, 0.0f, 1300.0f, 0.0f, 850.0f, 120.0f, 120.0f, 1300.0f);
        break;
    case 9:
        em->atari.init(0, 2, 0, 0.0f, 1550.0f, 0.0f, 750.0f, 120.0f, 120.0f, 1550.0f);
        break;
    case 5:
    case 6:
    default:
        em->atari.init(0, 2, 0, 0.0f, 1200.0f, 0.0f, 800.0f, 120.0f, 120.0f, 1200.0f);
        break;
    }
    em->atari.clrFlag100();
    em->atari.setPriority(3);
    em->setNoSuspend(1);
    em->setStatus(1);
    em->setStatus(0xB);
    switch (em->type) {
    case 0:
    case 7:
        break;
    case 2:
        YarareInitCube(em, 0.0f, 0.0f, 0.0f, 800.0f, 2700.0f, 120.0f, 0, 0x41);
        break;
    case 3:
        YarareInitCube(em, 0.0f, 0.0f, 0.0f, 4150.0f, 4800.0f, 250.0f, 0, 0x41);
        break;
    case 4:
        YarareInitCube(em, 0.0f, 0.0f, 0.0f, 1650.0f, 3150.0f, 250.0f, 0, 0x41);
        break;
    case 6:
        YarareInitCube(em, 0.0f, 260.0f, 0.0f, 490.0f, 1880.0f, 140.0f, 0, 0x41);
        YarareAddCube(em, &w->hit[0], -490.0f, 0.0f, 0.0f, 80.0f, 2400.0f, 140.0f, 0, 0x41);
        YarareAddCube(em, &w->hit[1], 490.0f, 0.0f, 0.0f, 80.0f, 2400.0f, 140.0f, 0, 0x41);
        YarareAddCube(em, &w->hit[2], 0.0f, 0.0f, 0.0f, 650.0f, 260.0f, 140.0f, 0, 0x41);
        YarareAddCube(em, &w->hit[3], 0.0f, 2040.0f, 0.0f, 650.0f, 260.0f, 140.0f, 0, 0x41);
        break;
    case 8:
        YarareInitCube(em, 0.0f, 0.0f, 0.0f, 850.0f, 2600.0f, 140.0f, 0, 0x41);
        break;
    case 9:
        YarareInitCube(em, 0.0f, 0.0f, 0.0f, 750.0f, 3100.0f, 140.0f, 0, 0x41);
        break;
    case 1:
    case 5:
    default:
        YarareInitCube(em, 0.0f, 0.0f, 0.0f, 800.0f, 2400.0f, 140.0f, 0, 0x41);
        break;
    }
    em->hpMax = em->hp = 1000;
    if (pos) {
        em->pos = *pos;
    } else {
        em->pos.x = 0.0f;
        em->pos.y = 0.0f;
        em->pos.z = 0.0f;
    }
    em->oldPos = em->pos;
    if (rot) {
        em->rot = *rot;
    }
    w->pos0 = em->pos;
    switch (em->type) {
    case 2:
        w->openH = 2600.0f;
        break;
    case 3:
        w->openH = 3800.0f;
        break;
    case 4:
        w->openH = 3000.0f;
        break;
    case 7:
        w->openH = 3500.0f;
        break;
    case 8:
        w->openH = 2600.0f;
        break;
    case 9:
        w->openH = 3100.0f;
        break;
    case 5:
    case 6:
    default:
        w->openH = 2300.0f;
        break;
    }
    switch (em->type) {
    case 0:
        w->halfW = 675.0f;
        break;
    case 1:
        w->halfW = 675.0f;
        break;
    case 2:
        w->halfW = 750.0f;
        break;
    case 3:
        w->halfW = 4150.0f;
        break;
    case 4:
        w->halfW = 1500.0f;
        break;
    case 5:
        w->halfW = 675.0f;
        break;
    case 6:
        w->halfW = 675.0f;
        break;
    case 7:
        w->halfW = 2000.0f;
        break;
    case 8:
        w->halfW = 875.0f;
        break;
    case 9:
        w->halfW = 875.0f;
        break;
    default:
        w->halfW = 675.0f;
        break;
    }
    w->sat = 0;
    for (i = 0; i < 4; i++) {
        w->sub[i] = 0;
    }
    w->eff = 0xFF;
    if (flg && (*flg & 2)) {
        parts = em->getPartsPtr(1);
        em->hitInfo.flags &= ~1;
        parts->scale.x = 0.0f;
        parts->scale.y = 0.0f;
        parts->scale.z = 0.0f;
    }
    w->status = 1;
    w->lockMode = 0;
    w->open = 1;
    w->pDouble = 0;
    if (em->type == 5 || em->type == 6 || em->type == 8 || em->type == 9) {
        w->open = 0;
        w->status = 2;
    }
    em->xFC = 1;
    em->xFD = 0;
    em->xFE = 0;
    em->xFF = 0;
    emBarredEatSet(em);
    return em;
}

void emBarredDmCk(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);
    EmHitInfo* part;
    cModel* parts;
    u16* flg;
    Vec v;
    f32 ang;
    int near;
    u8 wep;

    if (em->dmHit == 0) {
        return;
    }
    part = em->dmPart;
    em->dmHit = 0;
    near = 0;
    if (part->rad < 36000000.0f) {
        near = 1;
    }
    wep = em->dmWep;
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
    em->dmType = 1;
    if (wep == 0x10) {
        em->dmType = 0x11;
    }
    if (em->type == 6 && part == &em->hitInfo && w->eff != 0xFF) {
        ang = Muku(&em->pos, &em->x328, em->rot.y, PI);
        if (fabsf(ang) < PI / 2) {
            ang = em->rot.y;
        } else {
            ang = em->rot.y + PI;
        }
        v.y = LIMIT_ANGLE(ang);
        v.x = 0.0f;
        v.z = 0.0f;
        parts = em->getPartsPtr(1);
        EstSet(0, -1, &em->pos, &v, w->eff, 3, 0, 0, 0, 0);
        part->flags &= ~1;
        parts->scale.x = 0.0f;
        parts->scale.y = 0.0f;
        parts->scale.z = 0.0f;
        SndCall(6, 0x3B, &em->pos, 0, 0, em);
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        if (flg) {
            *flg |= 2;
        }
    } else {
        switch (em->dmWep) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 0xB:
        case 0xC:
        case 0x10:
        case 0x11:
        case 0x1B:
        case 0x1D:
        case 0x26:
        case 0x27:
        case 0x2B:
            if (w->eff != 0xFF) {
                EmDmBloodSet2(em, w->eff, 0, 0, 0, 0);
            }
            break;
        case 7:
        case 8:
        case 0x21:
            if (w->eff != 0xFF) {
                if (near) {
                    EmDmBloodSet2(em, w->eff, 1, 0, 0, 0);
                } else {
                    EmDmBloodSet2(em, w->eff, 0, 0, 0, 0);
                }
            }
            break;
        case 5:
        case 6:
        case 9:
        case 0xA:
        case 0xD:
        case 0xE:
        case 0xF:
        case 0x12:
        case 0x13:
        case 0x28:
        case 0x29:
        case 0x2C:
        case 0x2D:
            if (w->eff != 0xFF) {
                EmDmBloodSet2(em, w->eff, 0, 0, 0, 0);
            }
            break;
        case 0x14:
        case 0x15:
        default:
            break;
        }
    }
}

void cEmBarred::move()
{
    emBarredDmCk(this);
    EmBarred_R1_move_tbl[xFD](this);
    EmAtCheck(this);
    atari.move();
    emBarredEatSet(this);
}

void cEmBarred::setOpen(int a)
{
    EmBarredWork* w = EMBARRED_WK(this);

    if (w->status == 1) {
        return;
    }
    if (xFD == 1) {
        return;
    }
    if (xFD == 3) {
        return;
    }
    /*PERM*/
    w->status = 0;
    w->open = 1;
    xFC = 1;
    xFD = 1;
    xFE = 0;
    xFF = a;
    /*ENDPERM*/
}

void cEmBarred::setClose(int a)
{
    EmBarredWork* w = EMBARRED_WK(this);

    if (w->status == 2) {
        return;
    }
    if (xFD == 2) {
        return;
    }
    if (xFD == 3) {
        return;
    }
    /*PERM*/
    w->status = 0;
    w->open = 0;
    xFF = a;
    xFC = 1;
    xFD = 2;
    xFE = 0;
    /*ENDPERM*/
}

void cEmBarred::setOpened()
{
    EmBarredWork* w = EMBARRED_WK(this);

    if (xFD != 3) {
        w->status = 1;
        w->open = 1;
        pos.y = w->pos0.y + 2500.0f;
        SndStop(w->sndId, 0);
        xFC = 1;
        xFD = 0;
        xFE = 0;
        xFF = 0;
    }
}

void cEmBarred::setClosed()
{
    EmBarredWork* w = EMBARRED_WK(this);

    if (xFD != 3) {
        w->status = 2;
        w->open = 0;
        pos = w->pos0;
        SndStop(w->sndId, 0);
        xFC = 1;
        xFD = 0;
        xFE = 0;
        xFF = 0;
    }
}

void cEmBarred::setLockMode(u8 a)
{
    EMBARRED_WK(this)->lockMode = a;
}

int cEmBarred::ckStatus()
{
    return EMBARRED_WK(this)->status;
}

int cEmBarred::ckOpen()
{
    if (EMBARRED_WK(this)->open) {
        return 1;
    }
    return 0;
}

void emBarred_R1_Set(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);

    em->matUpdate();
    if (em->xFE == 0) {
        w->timer = 0;
        em->xFE++;
    }
    if (em->type == 5 || em->type == 6 || em->type == 8 || em->type == 9) {
        if (emBarredNearCk(em)) {
            w->timer = 0;
            if (w->status != 1 && w->lockMode == 0) {
                w->status = 0;
                w->open = 1;
                em->xFF = 0;
                em->xFD = 1;
                em->xFC = 1;
                em->xFE = 0;
                if (w->pDouble) {
                    w->pDouble->setOpen(1);
                }
            }
        } else if (emBarredCanClose(w)) {
            w->timer++;
            if (w->status == 1 && w->timer > 30 && w->lockMode == 0) {
                w->status = 0;
                w->open = 0;
                em->xFF = 0;
                em->xFC = 1;
                em->xFD = 2;
                em->xFE = 0;
                if (w->pDouble) {
                    w->pDouble->setClose(1);
                }
            }
        }
    }
}

void emBarred_R1_Open(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);
    Vec v;
    f32 d;

    w->status = 0;
    w->open = 1;
    switch (em->xFE) {
    case 0:
        SndStop(w->sndId, 0);
        if (em->xFF == 0) {
            switch (em->type) {
            case 5:
            case 6:
            case 8:
            case 9:
                w->sndId = SndCall(6, 0xF, &em->pos, 0, 0, em);
                break;
            default:
                w->sndId = SndCall(6, 0x24, &em->pos, 0, 0, em);
                break;
            }
        }
        em->xFE++;
    case 1:
        switch (em->type) {
        case 5:
        case 6:
            v.x = 15.0f;
            v.y = 0.0f;
            v.z = 0.0f;
            PSMTXMultVecSR(em->mat, &v, &v);
            PSVECAdd(&em->pos, &v, &em->pos);
            d = (em->pos.x - w->pos0.x) * (em->pos.x - w->pos0.x) + (em->pos.y - w->pos0.y) * (em->pos.y - w->pos0.y) + (em->pos.z - w->pos0.z) * (em->pos.z - w->pos0.z);
            if (d > 1690000.0f) {
                w->status = 1;
                em->xFC = 1;
                em->xFD = 0;
                em->xFE = 0;
                em->xFF = 0;
            }
            break;
        case 8:
            v.x = 15.0f;
            v.y = 0.0f;
            v.z = 0.0f;
            PSMTXMultVecSR(em->mat, &v, &v);
            PSVECAdd(&em->pos, &v, &em->pos);
            d = (em->pos.x - w->pos0.x) * (em->pos.x - w->pos0.x) + (em->pos.y - w->pos0.y) * (em->pos.y - w->pos0.y) + (em->pos.z - w->pos0.z) * (em->pos.z - w->pos0.z);
            if (d > 2890000.0f) {
                w->status = 1;
                em->xFC = 1;
                em->xFD = 0;
                em->xFE = 0;
                em->xFF = 0;
            }
            break;
        case 9:
            v.x = 15.0f;
            v.y = 0.0f;
            v.z = 0.0f;
            PSMTXMultVecSR(em->mat, &v, &v);
            PSVECAdd(&em->pos, &v, &em->pos);
            d = (em->pos.x - w->pos0.x) * (em->pos.x - w->pos0.x) + (em->pos.y - w->pos0.y) * (em->pos.y - w->pos0.y) + (em->pos.z - w->pos0.z) * (em->pos.z - w->pos0.z);
            if (d > 2890000.0f) {
                w->status = 1;
                em->xFC = 1;
                em->xFD = 0;
                em->xFE = 0;
                em->xFF = 0;
            }
            break;
        default:
            if (em->type == 4) {
                em->pos.y += 10.0f;
            } else {
                em->pos.y += 15.0f;
            }
            if (em->pos.y > w->pos0.y + w->openH) {
                em->pos.y = w->pos0.y + w->openH;
                em->xFE++;
            }
            break;
        }
        break;
    case 2:
        if (em->xFF == 0) {
            SndStop(w->sndId, 0);
            w->sndId = SndCall(6, 0x25, &em->pos, 0, 0, em);
        }
        w->timer = 5;
        em->xFE++;
    case 3:
        em->pos.x = fRand1_1() * 20.0f + w->pos0.x;
        em->pos.y = fRand0_1() * 20.0f + (w->pos0.y + w->openH);
        em->pos.z = fRand1_1() * 20.0f + w->pos0.z;
        if (w->timer != 0) {
            w->timer--;
        } else {
            em->pos = w->pos0;
            em->pos.y = w->pos0.y + w->openH;
            w->status = 1;
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    em->matUpdate();
}

void emBarred_R1_Close(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);
    Vec d;

    w->status = 0;
    w->open = 0;
    switch (em->xFE) {
    case 0:
        if (em->type == 4) {
            w->spd = -10.0f;
        } else {
            w->spd = -50.0f;
        }
        SndStop(w->sndId, 0);
        if (em->xFF == 0) {
            switch (em->type) {
            case 5:
            case 6:
            case 8:
            case 9:
                w->sndId = SndCall(6, 0xF, &em->pos, 0, 0, em);
                break;
            default:
                w->sndId = SndCall(6, 0x24, &em->pos, 0, 0, em);
                break;
            }
        }
        em->xFE++;
    case 1:
        switch (em->type) {
        case 5:
        case 6:
        case 8:
        case 9:
            PSVECSubtract(&w->pos0, &em->pos, &d);
            if (d.x * d.x + d.y * d.y + d.z * d.z <= 10000.0f) {
                em->pos = w->pos0;
                em->xFE++;
            } else {
#line 996 "D:/Bio4/Prog/emBarred.cpp"
                VECNormalize(&d, &d);
                PSVECScale(&d, &d, 100.0f);
                PSVECAdd(&em->pos, &d, &em->pos);
            }
            break;
        default:
            em->pos.y += w->spd;
            em->pos.y += w->spd;
            if (em->type == 4) {
                w->spd -= 10.0f;
            } else {
                w->spd -= 15.0f;
            }
            if (em->pos.y < w->pos0.y) {
                em->pos.y = w->pos0.y;
                em->xFE++;
            }
            break;
        }
        if (emBarredUnderCk(em)) {
            w->status = 0;
            w->open = 1;
            em->xFD = 1;
            em->xFF = 0;
            em->xFC = 1;
            em->xFE = 0;
            if (w->pDouble) {
                w->pDouble->setOpen(1);
            }
        }
        break;
    case 2:
        switch (em->type) {
        case 5:
        case 6:
        case 8:
        case 9:
            break;
        default:
            if (em->xFF == 0) {
                SndStop(w->sndId, 0);
                w->sndId = SndCall(6, 0x27, &em->pos, 0, 0, em);
            }
            break;
        }
        switch (em->type) {
        case 5:
        case 6:
        case 8:
        case 9:
            break;
        default:
            if (w->eff != 0xFF) {
                EstSet((int) em, -1, 0, 0, w->eff, 2, 0, 0, (u32) em, 0);
            }
            break;
        }
        w->timer = 5;
        em->xFE++;
    case 3:
        em->pos.x = fRand1_1() * 20.0f + w->pos0.x;
        em->pos.y = fRand0_1() * 20.0f + w->pos0.y;
        em->pos.z = fRand1_1() * 20.0f + w->pos0.z;
        if (w->timer != 0) {
            w->timer--;
        } else {
            em->pos = w->pos0;
            w->status = 2;
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    em->matUpdate();
    if (w->lockMode == 0) {
        if (em->type == 5 || em->type == 6 || em->type == 8 || em->type == 9) {
            if (emBarredNearCk(em)) {
                if (w->status != 1) {
                    w->status = 0;
                    w->open = 1;
                    em->xFF = 0;
                    em->xFD = 1;
                    em->xFC = 1;
                    em->xFE = 0;
                    if (w->pDouble) {
                        w->pDouble->setOpen(1);
                    }
                }
            }
        }
    }
}

void emBarred_R1_Break(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);
    u16* flg;

    if (em->xFE == 0) {
        em->hp = 0;
        em->be_flag &= ~2;
        em->clearStatus(5);
        em->atari.throughOn();
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        if (flg) {
            *flg |= 1;
        }
        em->xFE++;
    }
}

void emBarredEatSet(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);
    Vec poly[4];
    u16* flg;
    f32 hx;
    f32 hy;
    f32 hz;
    f32 h;
    int attr;

    if (em->hp <= 0) {
        em->atari.throughOn();
        if (w->sat) {
            w->sat->flags &= ~4;
        }
        if (w->sub[0]) {
            w->sub[0]->flags &= ~4;
        }
        if (w->sub[1]) {
            w->sub[1]->flags &= ~4;
        }
        if (w->sub[2]) {
            w->sub[2]->flags &= ~4;
        }
        if (w->sub[3]) {
            w->sub[3]->flags &= ~4;
        }
    }
    attr = 0;
    switch (em->type) {
    case 0:
        hx = 675.0f;
        hy = 60.0f;
        hz = 2350.0f;
        attr = 0x404000;
        break;
    case 2:
        hx = 750.0f;
        hy = 60.0f;
        hz = 2600.0f;
        break;
    case 3:
        hx = 4150.0f;
        hy = 125.0f;
        hz = 4800.0f;
        break;
    case 4:
        hx = 1500.0f;
        hy = 125.0f;
        hz = 3000.0f;
        break;
    case 6:
        hx = 675.0f;
        hy = 60.0f;
        hz = 2350.0f;
        break;
    case 7:
        hx = 2000.0f;
        hy = 115.0f;
        hz = 3500.0f;
        attr = 0x404000;
        break;
    case 8:
        hx = 875.0f;
        hy = 60.0f;
        hz = 2650.0f;
        break;
    case 9:
        hx = 875.0f;
        hy = 60.0f;
        hz = 2650.0f;
        break;
    case 1:
    case 5:
    default:
        hx = 675.0f;
        hy = 60.0f;
        hz = 2350.0f;
        break;
    }
    if (w->sat == 0) {
        h = hz;
        if (em->type == 6) {
            h = hz - 260.0f;
        }
        poly[0].x = -hx;
        poly[0].y = 0.0f;
        poly[0].z = -hy;
        poly[1].x = hx;
        poly[1].y = 0.0f;
        poly[1].z = -hy;
        poly[2].x = hx;
        poly[2].y = 0.0f;
        poly[2].z = hy;
        poly[3].x = -hx;
        poly[3].y = 0.0f;
        poly[3].z = hy;
        w->sat = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
    } else {
        w->sat->flags |= 4;
        w->sat->setCoord(&em->pos, &em->rot);
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        if (flg && (*flg & 2)) {
            w->sat->flags &= ~4;
        }
    }
    if (em->type == 6) {
        if (w->sub[0] == 0) {
            poly[0].x = -hx;
            poly[0].y = 0.0f;
            poly[0].z = -hy;
            poly[1].x = -hx + 160.0f;
            poly[1].y = 0.0f;
            poly[1].z = -hy;
            poly[2].x = -hx + 160.0f;
            poly[2].y = 0.0f;
            poly[2].z = hy;
            poly[3].x = -hx;
            poly[3].y = 0.0f;
            poly[3].z = hy;
            w->sub[0] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, hz);
        } else {
            w->sub[0]->flags |= 4;
            w->sub[0]->setCoord(&em->pos, &em->rot);
        }
        if (w->sub[1] == 0) {
            poly[0].x = hx - 160.0f;
            poly[0].y = 0.0f;
            poly[0].z = -hy;
            poly[1].x = hx;
            poly[1].y = 0.0f;
            poly[1].z = -hy;
            poly[2].x = hx;
            poly[2].y = 0.0f;
            poly[2].z = hy;
            poly[3].x = hx - 160.0f;
            poly[3].y = 0.0f;
            poly[3].z = hy;
            w->sub[1] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, hz);
        } else {
            w->sub[1]->flags |= 4;
            w->sub[1]->setCoord(&em->pos, &em->rot);
        }
        if (w->sub[2] == 0) {
            poly[0].x = -hx;
            poly[0].y = 0.0f;
            poly[0].z = -hy;
            poly[1].x = hx;
            poly[1].y = 0.0f;
            poly[1].z = -hy;
            poly[2].x = hx;
            poly[2].y = 0.0f;
            poly[2].z = hy;
            poly[3].x = -hx;
            poly[3].y = 0.0f;
            poly[3].z = hy;
            w->sub[2] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, 260.0f);
        } else {
            w->sub[2]->flags |= 4;
            w->sub[2]->setCoord(&em->pos, &em->rot);
        }
        if (w->sub[3] == 0) {
            poly[0].x = -hx;
            poly[0].y = hz - 260.0f;
            poly[0].z = -hy;
            poly[1].x = hx;
            poly[1].y = hz - 260.0f;
            poly[1].z = -hy;
            poly[2].x = hx;
            poly[2].y = hz - 260.0f;
            poly[2].z = hy;
            poly[3].x = -hx;
            poly[3].y = hz - 260.0f;
            poly[3].z = hy;
            w->sub[3] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, 260.0f);
        } else {
            w->sub[3]->flags |= 4;
            w->sub[3]->setCoord(&em->pos, &em->rot);
        }
    }
}

int emBarredNearCk(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);
    f32 r2;
    u32 i;

    switch (em->type) {
    case 5:
    case 6:
    case 8:
    case 9:
        break;
    default:
        return 0;
    }
    if (w->lockMode != 0) {
        return 0;
    }
    if (w->status == 1) {
        r2 = 12250000.0f;
    } else {
        r2 = 6250000.0f;
    }
    if ((w->pos0.x - pPL->pos.x) * (w->pos0.x - pPL->pos.x) + (w->pos0.y - pPL->pos.y) * (w->pos0.y - pPL->pos.y) + (w->pos0.z - pPL->pos.z) * (w->pos0.z - pPL->pos.z) < r2) {
        return 1;
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);

        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id > 0x3F) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if ((w->pos0.x - e->pos.x) * (w->pos0.x - e->pos.x) + (w->pos0.y - e->pos.y) * (w->pos0.y - e->pos.y) + (w->pos0.z - e->pos.z) * (w->pos0.z - e->pos.z) < r2) {
            return 1;
        }
    }
    return 0;
}

void cEmBarred::setEff(u8 eff)
{
    EMBARRED_WK(this)->eff = eff;
}

void cEmBarred::setBreak(Vec* target)
{
    EmBarredWork* w = EMBARRED_WK(this);
    Vec v;
    f32 ang;

    if (hp > 0) {
        ang = Muku(&pos, target, rot.y, PI);
        if (fabsf(ang) < PI / 2) {
            ang = rot.y;
        } else {
            ang = rot.y + PI;
        }
        v.y = LIMIT_ANGLE(ang);
        v.x = 0.0f;
        v.z = 0.0f;
        EstSet(0, -1, &pos, &v, w->eff, 3, 0, 0, 0, 0);
        atari.throughOn();
        hp = 0;
        xFC = 1;
        xFD = 3;
        xFE = 0;
        xFF = 0;
    }
}

void cEmBarred::setNoClose()
{
    EMBARRED_WK(this)->flags |= 1;
}

void cEmBarred::setDouble(cEmBarred* other)
{
    if (other) {
        EMBARRED_WK(this)->pDouble = other;
        EMBARRED_WK(other)->pDouble = this;
    }
}

void cEmBarred::setUnderCk()
{
    EMBARRED_WK(this)->flags |= 2;
}

int emBarredUnderCk(cEmBarred* em)
{
    EmBarredWork* w = EMBARRED_WK(em);
    Mtx m;
    Vec v;
    u32 i;

    if (!(w->flags & 2)) {
        return 0;
    }
    switch (em->type) {
    case 5:
    case 6:
    case 8:
    case 9:
        return 0;
    }
    if (em->pos.y - w->pos0.y > 2200.0f) {
        return 0;
    }
    PSMTXRotRad(m, 'y', em->rot.y);
    TransMatrix(m, &w->pos0);
    PSMTXInverse(m, m);
    PSMTXMultVec(m, &pPL->pos, &v);
    if (v.x > -w->halfW && v.x < w->halfW && v.y > -100.0f && v.y < 100.0f && v.z > -200.0f && v.z < 200.0f) {
        return 1;
    }
    if (pSUB) {
        PSMTXMultVec(m, &pSUB->pos, &v);
        if (v.x > -w->halfW && v.x < w->halfW && v.y > -100.0f && v.y < 100.0f && v.z > -200.0f && v.z < 2100.0f) {
            return 1;
        }
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);

        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id > 0x3F) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!(e->be_flag & 2)) {
            continue;
        }
        PSMTXMultVec(m, &e->pos, &v);
        if (v.x > -w->halfW && v.x < w->halfW && v.y > -100.0f && v.y < 100.0f && v.z > -200.0f && v.z < 200.0f) {
            return 1;
        }
    }
    return 0;
}
