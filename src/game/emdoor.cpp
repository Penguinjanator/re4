// game/emdoor.cpp: door enemy (cEmDoor): the room doors the player opens or kicks open, with
// locks, chains, breakable panes and the effect collision pieces around them.

#include "atari.h"
#include "atari_init.h"
#include "light.h"
#include "dmg.h"
#include "obj.h"
#include "emdoor.h"
#include "emhit.h"
#include "emwep.h"
#include "emrack.h"
#include "em_set.h"
#include "etc_model.h"
#include "at_mod.h"
#include "act_btn.h"
#include "esp.h"
#include "snd.h"
#include "rnd.h"
#include "main.h"
#include "player.h"
#include "pl_npc.h"
#include "pl_sub.h"
#include "pl_wep.h"
#include "global.h"
#include "math_sub.h"
#include "cmath.h"
#include "db_log.h"

extern "C" {
int MotionMove(cModel* m, int a);
void EtcSetAddAmb(cModel* m, int a);   // EtcModel.cpp
void Em_R0_Scenario(cEm* em);          // em_sub.cpp
}
void MotionSetCore(cModel* m, void* w, void* data, int seq, int hokan, int flags, int frame);   // motion.cpp (C++ linkage)

// Hanging object (game/obj12.cpp): the locks and the chain hang on the door as cObj12 models.
class cObj12 : public cObj {
public:
    void setFall(Vec* spd, u8 type);
    void setFallSe(u8 blk, u8 no, u8 id);
};
cObj* SetObj12(void* bin, void* tpl, Vec* pos, Vec* rot);

typedef void (*EmDoorFunc)(cEmDoor*);

static void emDoor_R1_Open2(cEmDoor* em);

// Parts index remap for the flipped motions (MotionWork::flip): identity.
static u16 emDoor_xflip_tbl[20] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
};

EmDoorFunc EmDoor_R0_move_tbl[5] = {
    emDoor_R0_Init,
    emDoor_R0_Move,
    0,
    0,
    (EmDoorFunc) Em_R0_Scenario,
};

static EmDoorFunc EmDoor_R1_move_tbl[10] = {
    emDoor_R1_Set,
    emDoor_R1_Open,
    emDoor_R1_Open2,
    emDoor_R1_Close,
    emDoor_R1_Break,
    emDoor_R1_Shock,
    emDoor_R1_OpenLock,
    emDoor_R1_CloseLock,
    emDoor_R1_Down,
    emDoor_R1_Downed,
};

// Local matrix from pos / rot / scale and the parts after it (the door has no motion most of the time).
static inline void emDoorMatUpdate(cEmDoor* em)
{
    RotMatrix(em->mat, &em->rot);
    TransMatrix(em->mat, &em->pos);
    ScaleMatrix(em->mat, &em->scale);
    em->partsMatCalc();
    em->partsWorldCalc();
}

// pG->door_unlock bit of key `no`.
static inline u32 emDoorKeyCk(u32 no)
{
    u32* tbl = pG->door_unlock;

    return tbl[no >> 5] & (0x80000000 >> (no & 0x1F));
}

cEmDoor* SetDoor(void* bin, void* tpl, Vec* pos, Vec* rot, int type, int flagNo)
{
    cEmDoor* em;
    EmDoorWork* w;
    u16* flg;
    Vec v;

    em = (cEmDoor*) EmMgr.create(0x41);
    if (em == 0) {
        return 0;
    }
    w = EMDOOR_WK(em);
    if (pos) {
        em->pos = *pos;
    }
    if (rot) {
        em->rot = *rot;
    }
    if (em->modelInit(bin, tpl) == 0) {
        pLog->err(0, 0, "SetDoor() failed.");
        EmMgr.destroy(em);
        return 0;
    }
    em->type = type;
    if (type != 6) {
        w->height = 2300.0f;
        w->width = 650.0f;
    } else {
        w->height = 4400.0f;
        w->width = 650.0f;
        em->type = 1;
    }
    em->motFlip = emDoor_xflip_tbl;
    EtcSetAddAmb(em, 2);
    w->eff = 0xFF;
    atariInitF(&em->atari, -w->width, w->height * 0.5f, 0.0f, w->width + 50.0f, 150.0f, 150.0f, w->height * 0.5f + 50.0f, 0, 2, 0);
    em->atari.setPriority(3);
    em->atari.clrFlag100();
    em->setStatus(5);
    w->sat[0] = 0;
    w->sat[1] = 0;
    w->sat[2] = 0;
    w->sat[3] = 0;
    w->sat[4] = 0;
    w->sat[5] = 0;
    emDoorYarareInit(em);
    em->hpMax = em->hp = 1000;
    {
        static const Vec ofs = { 0.0f, 0.0f, 0.0f };
        static const Vec size = { 3000.0f, 3000.0f, 3000.0f };

        em->lightInfo.init2(0, 1, &ofs, &size, 0x10);
    }
    em->lockParts = 0;
    em->lockOfs.x = 0.0f;
    em->lockOfs.y = 0.0f;
    em->lockOfs.z = 0.0f;
    em->setStatus(1);
    em->setStatus(0xB);
    em->be_flag &= ~0x01000000;
    em->be_flag &= ~0x10;
    w->bendL = 0.0f;
    w->bendR = 0.0f;
    w->bendChain = 0.0f;
    w->lockHpL = 0;
    w->lockHpR = 0;
    w->chainHp[0] = 0;
    w->chainHp[1] = 0;
    w->chainHp[2] = 0;
    w->pLockL = 0;
    w->pLockR = 0;
    w->pChain = 0;
    w->seCancel = 0;
    w->flags = 0;
    w->keyNo = 0x36;
    w->rnd = Rnd() % 5;
    w->dmg = (Rnd() & 1) + 1;
    w->x400 = 0;
    w->pDoor = 0;
    w->sndId = 0;
    w->rotY = em->rot.y;
    PSMTXRotRad(w->mat, 'y', em->rot.y);
    TransMatrix(w->mat, &em->pos);
    v.x = -w->width;
    v.y = 0.0f;
    v.z = 0.0f;
    PSMTXMultVec(w->mat, &v, &v);
    TransMatrix(w->mat, &v);
    PSMTXInverse(w->mat, w->inv);
    w->flagNo = flagNo;
    flg = GetEtcFlgPtr(flagNo, pG->room_id);
    if (flg && (*flg & 1)) {
        em->hp = 0;
    }
    if (em->hp <= 0) {
        em->xFC = 1;
        em->xFD = 4;
        em->xFE = 0;
        em->xFF = 0;
        em->clearStatus(5);
        if (flg && (*flg & 0xC0)) {
            if (*flg & 0x40) {
                w->dir = 0;
            } else {
                w->dir = 1;
            }
            em->xFC = 1;
            em->xFD = 9;
            em->xFE = 0;
            em->xFF = 0;
        }
    } else {
        em->xFC = 1;
        em->xFD = 0;
        em->xFE = 0;
        em->xFF = 0;
        em->setStatus(5);
    }
    if (em->hp > 0) {
        emDoorSatSet(em);
    }
    return em;
}

// Chain `no` of a wooden door was hit.
static inline void emDoorChainHit(cEmDoor* em, u32 no)
{
    SndCall(6, 0x15, &em->pos, 0, 0, em);
    EmDmBloodSet2(em, 0xCB, 0, 0, 0, 0);
    emDoorSetDmgChain(em, no);
}

void emDoorDmCkWood(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    EmHitInfo* part;
    u8 wep;
    Vec v;
    Vec out;
    int kind;

    if (em->hp > 0) {
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(w->mat, &v, &v);
        kind = DmgMgr.hitCheck(&v, &out);
        switch (kind) {
        case 1:
        case 4:
        case 5:
        case 7:
            em->setBreak(&out);
            return;
        }
    }
    if (em->dmHit == 0) {
        return;
    }
    wep = em->dmWep;
    em->dmHit = 0;
    part = em->dmPart;
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
    if (w->flags & 1) {
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        return;
    }
    if (em->dmWep == 0x10) {
        if (part != &w->hit[12] && part != &w->hit[11] && part != &w->hit[13] && part != &w->hit[14] && part != &w->hit[15]) {
            em->dmType = 0;
            return;
        }
    }
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
    case 0x14:
    case 0x15:
    case 0x1B:
    case 0x1D:
    case 0x26:
    case 0x27:
    case 0x2B:
        if (part == &w->hit[12]) {
            emDoorSetDmgLock_L(em, 0);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorSetDmgLock_R(em, 0);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (part->partsNo != 0) {
            if (em->plDist2 < 9000000.0f && Rnd() % 5 == 0) {
                w->dmg = 0.0f;
            } else {
                w->dmg -= 1.0f;
            }
        }
        emDoorSetDmgDoor(em);
        break;
    case 5:
    case 6:
    case 9:
    case 0xA:
    case 0xF:
    case 0x28:
    case 0x2C:
        if (part == &w->hit[12]) {
            if (w->flags & 2) {
                w->lockHpL -= 4;
                emDoorSetDmgLock_L(em, 0);
            } else {
                emDoorSetDmgLock_L(em, 1);
            }
            return;
        }
        if (part == &w->hit[11]) {
            if (w->flags & 2) {
                w->lockHpR -= 4;
                emDoorSetDmgLock_R(em, 0);
            } else {
                emDoorSetDmgLock_R(em, 1);
            }
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (part->partsNo != 0) {
            w->dmg = 0.0f;
        }
        emDoorSetDmgDoor(em);
        break;
    case 7:
    case 8:
    case 0x21:
        if (part == &w->hit[12]) {
            if (em->plDist2 < 25000000.0f) {
                if (w->flags & 2) {
                    w->lockHpL -= 4;
                } else {
                    w->lockHpL = 0;
                }
            }
            emDoorSetDmgLock_L(em, 0);
            return;
        }
        if (part == &w->hit[11]) {
            if (em->plDist2 < 25000000.0f) {
                if (w->flags & 2) {
                    w->lockHpR -= 4;
                } else {
                    w->lockHpR = 0;
                }
            }
            emDoorSetDmgLock_R(em, 0);
            return;
        }
        if (part->partsNo != 0 && part->rad < 64000000.0f) {
            w->dmg = 0.0f;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        emDoorSetDmgDoor(em);
        break;
    case 0xE:
        break;
    case 0xD:
    case 0x29:
    default:
        emDoorSetDmgLock_L(em, 1);
        emDoorSetDmgLock_R(em, 1);
        emDoorSetDmgChain(em, 0);
        emDoorSetDmgChain(em, 1);
        emDoorSetDmgChain(em, 2);
        emDoorSetBrkDoor(em, &em->x328);
        break;
    }
    if (emDoorBrkCk(em)) {
        emDoorSetDmgLock_L(em, 1);
        emDoorSetDmgLock_R(em, 1);
        emDoorSetDmgChain(em, 0);
        emDoorSetDmgChain(em, 1);
        emDoorSetDmgChain(em, 2);
        EstSet((int) em, -1, 0, 0, w->eff, 6, 0, 0, (u32) em, 0);
        SndCall(6, 0x37, &em->pos, 0, 0, em);
        em->xFC = 1;
        em->xFD = 4;
        em->xFE = 0;
        em->xFF = 0;
    }
}

// The hit part is a lock or a chain box.
static inline int emDoorLockChainCk(cEmDoor* em, EmHitInfo* part)
{
    if (part == &EMDOOR_WK(em)->hit[12]) {
        return 1;
    }
    if (part == &EMDOOR_WK(em)->hit[11]) {
        return 1;
    }
    if (part == &EMDOOR_WK(em)->hit[13]) {
        return 1;
    }
    if (part == &EMDOOR_WK(em)->hit[14]) {
        return 1;
    }
    if (part == &EMDOOR_WK(em)->hit[15]) {
        return 1;
    }
    return 0;
}

// Lock hit by a strong weapon: the lock hp drops by 4 in the strong mode, otherwise it breaks.
static inline void emDoorLockHitL(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    if (w->flags & 2) {
        w->lockHpL -= 4;
        emDoorSetDmgLock_L(em, 0);
    } else {
        emDoorSetDmgLock_L(em, 1);
    }
}

static inline void emDoorLockHitR(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    if (w->flags & 2) {
        w->lockHpR -= 4;
        emDoorSetDmgLock_R(em, 0);
    } else {
        emDoorSetDmgLock_R(em, 1);
    }
}

// Lock hit by a shotgun: only from close by.
static inline void emDoorLockHitNearL(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    if (em->plDist2 < 25000000.0f) {
        if (w->flags & 2) {
            w->lockHpL -= 4;
        } else {
            w->lockHpL = 0;
        }
    }
    emDoorSetDmgLock_L(em, 0);
}

static inline void emDoorLockHitNearR(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    if (em->plDist2 < 25000000.0f) {
        if (w->flags & 2) {
            w->lockHpR -= 4;
        } else {
            w->lockHpR = 0;
        }
    }
    emDoorSetDmgLock_R(em, 0);
}

// Breaks the locks and the chain (or the one the shot came from).
static inline void emDoorBreakLocks(cEmDoor* em)
{
    f32 ang;

    if (em->type == 3 || em->type == 7) {
        emDoorSetDmgLock_L(em, 1);
        emDoorSetDmgLock_R(em, 1);
    } else {
        ang = Muku(&em->pos, &em->x328, em->rot.y, PI);
        if (fabsf(ang) < PI / 2) {
            emDoorSetDmgLock_L(em, 1);
        } else {
            emDoorSetDmgLock_R(em, 1);
        }
    }
    emDoorSetDmgChain(em, 0);
    emDoorSetDmgChain(em, 1);
    emDoorSetDmgChain(em, 2);
}

void emDoorDmCkIron(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    EmHitInfo* part;
    u8 wep;

    if (em->dmHit == 0) {
        return;
    }
    wep = em->dmWep;
    em->dmHit = 0;
    part = em->dmPart;
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
        if (emDoorLockChainCk(em, part) == 0) {
            em->dmType = 0;
            return;
        }
    }
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
    case 0x14:
    case 0x15:
    case 0x1B:
    case 0x1D:
    case 0x26:
    case 0x27:
    case 0x2B:
        if (part == &w->hit[12]) {
            emDoorSetDmgLock_L(em, 0);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorSetDmgLock_R(em, 0);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        return;
    case 9:
    case 0xA:
    case 0x28:
        if (part == &w->hit[12]) {
            emDoorLockHitL(em);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorLockHitR(em);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        return;
    case 5:
    case 6:
    case 7:
    case 8:
    case 0xF:
    case 0x21:
    case 0x2C:
        if (part == &w->hit[12]) {
            emDoorLockHitNearL(em);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorLockHitNearR(em);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 3, 0, 0, 0);
        return;
    case 0xE:
        return;
    case 0xD:
    case 0x12:
    case 0x13:
    default:
        emDoorBreakLocks(em);
        if (w->eff != 0xFF) {
            EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        }
        break;
    }
}

void emDoorDmCkIron2(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    EmHitInfo* part;
    u8 wep;

    if (em->dmHit == 0) {
        return;
    }
    wep = em->dmWep;
    em->dmHit = 0;
    part = em->dmPart;
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
        if (emDoorLockChainCk(em, part) == 0) {
            em->dmType = 0;
            return;
        }
    }
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
    case 0x14:
    case 0x15:
    case 0x1B:
    case 0x1D:
    case 0x26:
    case 0x27:
    case 0x2B:
        if (part == &w->hit[12]) {
            emDoorSetDmgLock_L(em, 0);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorSetDmgLock_R(em, 0);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (part->partsNo != 0) {
            w->dmg = 0.0f;
            emDoorSetDmgDoor(em);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        return;
    case 9:
    case 0xA:
    case 0x28:
        if (part == &w->hit[12]) {
            emDoorLockHitL(em);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorLockHitR(em);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (part->partsNo != 0) {
            w->dmg = 0.0f;
            emDoorSetDmgDoor(em);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        return;
    case 5:
    case 6:
    case 7:
    case 8:
    case 0xF:
    case 0x21:
    case 0x2C:
        if (part == &w->hit[12]) {
            emDoorLockHitNearL(em);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorLockHitNearR(em);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (part->partsNo != 0) {
            w->dmg = 0.0f;
            emDoorSetDmgDoor(em);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 3, 0, 0, 0);
        return;
    case 0xE:
        return;
    case 0xD:
    case 0x12:
    case 0x13:
    default:
        emDoorBreakLocks(em);
        if (part->partsNo != 0) {
            w->dmg = 0.0f;
            emDoorSetDmgDoor(em);
            return;
        }
        if (w->eff != 0xFF) {
            EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        }
        break;
    }
}

void emDoorDmCkIronDown(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    EmHitInfo* part;
    u8 wep;

    if (em->dmHit == 0) {
        return;
    }
    wep = em->dmWep;
    em->dmHit = 0;
    part = em->dmPart;
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
        if (emDoorLockChainCk(em, part) == 0) {
            em->dmType = 0;
            return;
        }
    }
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
    case 0x14:
    case 0x15:
    case 0x1B:
    case 0x1D:
    case 0x26:
    case 0x27:
    case 0x2B:
        if (part == &w->hit[12]) {
            emDoorSetDmgLock_L(em, 0);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorSetDmgLock_R(em, 0);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        return;
    case 9:
    case 0xA:
    case 0x28:
        if (part == &w->hit[12]) {
            emDoorLockHitL(em);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorLockHitR(em);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        return;
    case 5:
    case 6:
    case 7:
    case 8:
    case 0xF:
    case 0x21:
    case 0x2C:
        if (part == &w->hit[12]) {
            emDoorLockHitNearL(em);
            return;
        }
        if (part == &w->hit[11]) {
            emDoorLockHitNearR(em);
            return;
        }
        if (part == &w->hit[13]) {
            emDoorChainHit(em, 0);
            return;
        }
        if (part == &w->hit[14]) {
            emDoorChainHit(em, 1);
            return;
        }
        if (part == &w->hit[15]) {
            emDoorChainHit(em, 2);
            return;
        }
        if (w->eff == 0xFF) {
            return;
        }
        EmDmBloodSet2(em, w->eff, 3, 0, 0, 0);
        return;
    case 0xE:
        return;
    case 0xD:
    case 0x12:
    case 0x13:
    default:
        emDoorBreakLocks(em);
        if (w->eff != 0xFF) {
            EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
        }
        break;
    }
}

// The lock falls off: the lock object drops as a rope and the door / list flags forget it.
static inline void emDoorLockFall(cEmDoor* em, cObj12* lock)
{
    Vec v;

    SndCall(6, 0x53, &em->pos, 0, 0, em);
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 20.0f;
    PSMTXMultVecSR(lock->mat, &v, &v);
    lock->setFall(&v, 4);
    lock->setFallSe(6, 0x3C, 0);
}

// Lock hit effect: the hit sound, blood and the effect on the lock object.
static inline void emDoorLockHitEff(cEmDoor* em, cObj12* lock)
{
    SndCall(6, 0x52, &em->pos, 0, 0, em);
    EmDmBloodSet2(em, 0xC9, 0, 0, 0, 0);
    EstSet((int) lock, -1, 0, 0, 0xC9, 1, 0, 0, (u32) lock, 0);
}

void emDoorSetDmgLock_L(cEmDoor* em, int mode)
{
    EmDoorWork* w = EMDOOR_WK(em);
    EmListData* d = EM_LIST(em->emsetNo);
    EmHitInfo* hit;
    u16* flg;

    if (w->pLockL == 0) {
        return;
    }
    hit = &w->hit[12];
    switch (mode) {
    case 0:
    default:
        if (w->flags & 2) {
            w->lockHpL -= 1;
        } else {
            w->lockHpL = 0;
        }
        emDoorLockHitEff(em, w->pLockL);
        if (w->lockHpL <= 0) {
            emDoorLockFall(em, w->pLockL);
            d->flags4 &= ~0x80000000;
            em->flags_3C8 &= ~0x80000000;
            em->setStatus(1);
            w->pLockL = 0;
            hit->flags &= ~1;
        }
        w->bendL = PI / 4;
        break;
    case 1:
        w->lockHpL = 0;
        emDoorLockFall(em, w->pLockL);
        emDoorLockHitEff(em, w->pLockL);
        d->flags4 &= ~0x80000000;
        em->flags_3C8 &= ~0x80000000;
        em->setStatus(1);
        w->pLockL = 0;
        hit->flags &= ~1;
        break;
    }
    if (w->lockHpL <= 0) {
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        if (flg) {
            *flg |= 4;
        }
    }
}

void emDoorSetDmgLock_R(cEmDoor* em, int mode)
{
    EmDoorWork* w = EMDOOR_WK(em);
    EmListData* d = EM_LIST(em->emsetNo);
    EmHitInfo* hit;
    u16* flg;

    if (w->pLockR == 0) {
        return;
    }
    hit = &w->hit[11];
    switch (mode) {
    case 0:
    default:
        if (w->flags & 2) {
            w->lockHpR -= 1;
        } else {
            w->lockHpR = 0;
        }
        emDoorLockHitEff(em, w->pLockR);
        if (w->lockHpR <= 0) {
            emDoorLockFall(em, w->pLockR);
            d->flags4 &= ~0x40000000;
            em->flags_3C8 &= ~0x40000000;
            em->setStatus(1);
            flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
            if (flg) {
                *flg |= 4;
            }
            w->pLockR = 0;
            hit->flags &= ~1;
        }
        w->bendR = PI / 4;
        break;
    case 1:
        w->lockHpR = 0;
        emDoorLockFall(em, w->pLockR);
        emDoorLockHitEff(em, w->pLockR);
        d->flags4 &= ~0x40000000;
        em->flags_3C8 &= ~0x40000000;
        em->setStatus(1);
        w->pLockR = 0;
        hit->flags &= ~1;
        break;
    }
    if (w->lockHpR <= 0) {
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        if (flg) {
            *flg |= 2;
        }
    }
}

void emDoorSetDmgChain(cEmDoor* em, u32 no)
{
    EmDoorWork* w = EMDOOR_WK(em);
    cModel* parts;
    u16* flg;

    if (w->pChain == 0) {
        return;
    }
    if (w->chainHp[no] <= 0) {
        return;
    }
    flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
    switch (no) {
    case 0:
        w->hit[13].flags &= ~1;
        w->chainHp[no] = 0;
        parts = w->pChain->getPartsPtr(1);
        EstSet(0, -1, &parts->worldPos, &w->pChain->rot, 0xCB, 1, 0, 0, 0, 0);
        parts->scale.x = 0.0f;
        parts->scale.y = 0.0f;
        parts->scale.z = 0.0f;
        SndCall(6, 0x16, &em->pos, 0, 0, em);
        if (flg) {
            *flg |= 8;
        }
        break;
    case 1:
        w->hit[14].flags &= ~1;
        w->chainHp[1] = 0;
        parts = w->pChain->getPartsPtr(2);
        EstSet(0, -1, &parts->worldPos, &w->pChain->rot, 0xCB, 2, 0, 0, 0, 0);
        parts->scale.x = 0.0f;
        parts->scale.y = 0.0f;
        parts->scale.z = 0.0f;
        SndCall(6, 0x16, &em->pos, 0, 0, em);
        if (flg) {
            *flg |= 0x10;
        }
        break;
    case 2:
        w->hit[15].flags &= ~1;
        w->chainHp[2] = 0;
        parts = w->pChain->getPartsPtr(3);
        EstSet(0, -1, &parts->worldPos, &w->pChain->rot, 0xCB, 3, 0, 0, 0, 0);
        parts->scale.x = 0.0f;
        parts->scale.y = 0.0f;
        parts->scale.z = 0.0f;
        SndCall(6, 0x16, &em->pos, 0, 0, em);
        if (flg) {
            *flg |= 0x20;
        }
        break;
    }
}

void emDoorSetDmgDoor(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    EmHitInfo* part = em->dmPart;
    EmListData* d = EM_LIST(em->emsetNo);
    cModel* parts;
    u16* flg;
    Vec v;
    Vec rot;
    f32 ang;
    f32 dif;
    u32 bit;
    int i;

    if (part->partsNo != 0) {
        parts = em->getPartsPtr(part->partsNo - 1);
        if (w->dmg <= 0.0f) {
            w->dmg = (Rnd() & 1) + 1;
            parts->scale.x = 0.0f;
            parts->scale.y = 0.0f;
            parts->scale.z = 0.0f;
            part->flags &= ~1;
            if (em->type == 4) {
                if (w->eff != 0xFF) {
                    parts = em->getPartsPtr(part->partsNo - 1);
                    v.x = 0.0f;
                    v.y = 0.0f;
                    v.z = 1.0f;
                    PSMTXMultVecSR(em->mat, &v, &v);
                    rot.x = 0.0f;
                    rot.y = atan2f(v.x, v.z);
                    rot.z = 0.0f;
                    ang = GetXZAngle(&parts->worldPos, &em->x328);
                    dif = Muku2(rot.y, ang, PI);
                    if (fabsf(dif) > PI / 2) {
                        rot.y += PI;
                        rot.y = LIMIT_ANGLE(rot.y);
                    }
                    EstSet(0, -1, &parts->worldPos, &rot, w->eff, 0, 0, 0, 0, 0);
                    SndCall(6, 0x3B, &em->pos, 0, 0, em);
                    flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
                    if (flg) {
                        *flg |= 8;
                    }
                }
            } else {
                if (w->eff != 0xFF) {
                    EmDmBloodSet2(em, w->eff, 0, 0, 0, 0);
                }
                SndCall(6, 0x36, &em->pos, 0, 0, em);
            }
            bit = 0x8000;
            for (i = 3; i <= 10; i++) {
                if (part == &w->hit[i]) {
                    em->flags_3C8 |= bit;
                    d->flags4 |= bit;
                    break;
                }
                bit >>= 1;
            }
            return;
        }
    }
    if (w->eff != 0xFF) {
        switch (em->dmWep) {
        case 7:
        case 8:
        case 0x21:
            EmDmBloodSet2(em, w->eff, 3, 0, 0, 0);
            break;
        default:
            EmDmBloodSet2(em, w->eff, 2, 0, 0, 0);
            break;
        }
    }
}

void emDoorSetBrkDoor(cEmDoor* em, Vec* pos)
{
    EmDoorWork* w = EMDOOR_WK(em);
    f32 ang;

    if (em->hp <= 0) {
        return;
    }
    if (em->type == 2 || em->type == 3) {
        em->setOpen(pos, 0, 0, 0);
        return;
    }
    if (w->eff != 0xFF) {
        ang = Muku(&em->pos, pos, em->rot.y, PI);
        if (fabsf(ang) < PI / 2) {
            EstSet((int) em, -1, 0, 0, w->eff, 5, 0, 0, (u32) em, 0);
        } else {
            EstSet((int) em, -1, 0, 0, w->eff, 4, 0, 0, (u32) em, 0);
        }
    }
    SndCall(6, 0x37, &em->pos, 0, 0, em);
    em->xFC = 1;
    em->xFD = 4;
    em->xFE = 0;
    em->xFF = 0;
}

int emDoorBrkCk(cEmDoor* em)
{
    u32 bit;
    int cnt;
    int i;

    if (em->hp <= 0) {
        return 0;
    }
    bit = 0x8000;
    cnt = 0;
    for (i = 0; i < 8; i++) {
        if (em->flags_3C8 & bit) {
            cnt++;
        }
        bit >>= 1;
    }
    return (u32) cnt > 3;
}

void cEmDoor::move()
{
    EmDoorWork* w = EMDOOR_WK(this);

    switch (type) {
    case 4:
        emDoorDmCkIron2(this);
        break;
    case 1:
    case 5:
        emDoorDmCkIron(this);
        break;
    case 2:
        emDoorDmCkIronDown(this);
        break;
    case 0:
    case 3:
    default:
        emDoorDmCkWood(this);
        break;
    case 7:
        emDoorDmCkIron(this);
        break;
    }
    if (w->pDoor && w->pDoor->hp <= 0) {
        w->pDoor = 0;
    }
    emDoorActEvtCk(this);
    be_flag &= ~0x4000;
    EmDoor_R0_move_tbl[xFC](this);
    EmAtCheck(this);
    atari.move();
    if (hp > 0) {
        emDoorLockBendMove(this);
        emDoorLockMove(this);
    }
    emDoorSatSet(this);
}

void emDoor_R0_Init(cEmDoor* em)
{
    em->xFC = 1;
    em->xFD = 0;
    em->xFE = 0;
    em->xFF = 0;
}

void emDoor_R0_Move(cEmDoor* em)
{
    EmDoor_R1_move_tbl[em->xFD](em);
}

void emDoor_R1_Set(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    switch (em->xFE) {
    case 0:
        emDoorMatUpdate(em);
        em->xFE++;
        SndStop(w->sndId, 0);
        break;
    case 1:
        if (em->flags_3C8 & 0xFFC0) {
            emDoorMatUpdate(em);
        }
        break;
    }
    if (emDoorDoorAutoCloseCk(em) == 0) {
        if (!(em->flags_3C8 & 0xFFC0)) {
            em->be_flag |= 0x4000;
        }
    }
}

// The opening door hits whoever stands in its way (PlWepHitCheck3 with the partner's hp saved).
static inline void emDoorKickHit(cEmDoor* em, Vec* v, int type, Vec* sndPos, cUnit* sndObj)
{
    s16 hp;

    hp = 0;
    if (pSUB) {
        hp = pSUB->hp;
        pSUB->hp = 0;
    }
    if (PlWepHitCheck3(v, type, 10, 400.0f)) {
        SndCall(1, 0xF, sndPos, 0, 0, sndObj);
    }
    if (pSUB) {
        pSUB->hp = hp;
    }
}

void emDoor_R1_Open(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    f32 ang;
    f32 tgt;
    f32 lim;
    f32 d;
    f32 r;
    Vec v;

    switch (em->xFE) {
    case 0:
        em->flags_3C8 |= 0x20000000;
        w->kickCnt = 1;
        w->timer = 0x14;
        w->x400 = 0x96;
        emDoorSetDmgLock_L(em, 1);
        emDoorSetDmgLock_R(em, 1);
        emDoorSetDmgChain(em, 0);
        emDoorSetDmgChain(em, 1);
        emDoorSetDmgChain(em, 2);
        em->hp = 1000;
        ang = Muku(&em->pos, &w->target, w->rotY, PI);
        if (fabsf(ang) > PI / 2) {
            w->dir = 0;
        } else {
            w->dir = 1;
        }
        emDoorDropWeapon(em);
        em->xFE++;
    case 1:
        if (w->dir) {
            tgt = LIMIT_ANGLE(w->rotY - PI / 2);
            lim = PI / 6;
        } else {
            tgt = LIMIT_ANGLE(w->rotY + PI / 2);
            lim = PI / 7;
        }
        d = Muku2(em->rot.y, tgt, lim);
        em->rot.y += d;
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (fabsf(d) < PI / 1024) {
            em->rot.y = tgt;
            em->xFE++;
        }
        if (w->kickCnt != 0) {
            w->kickCnt--;
            em->dmType = 2;
            if (w->dir) {
                v.x = 0.0f;
                v.y = w->width;
                v.z = -600.0f;
            } else {
                v.x = 0.0f;
                v.y = w->width;
                v.z = 600.0f;
            }
            PSMTXMultVec(w->mat, &v, &v);
            if (em->xFF != 0) {
                emDoorKickHit(em, &v, 0x18, &v, em);
            }
        }
        break;
    case 2:
        w->timer = 2;
        em->xFE++;
    case 3:
        if (w->dir) {
            tgt = w->rotY - PI / 2;
        } else {
            tgt = w->rotY + PI / 2;
        }
        if (w->timer != 0) {
            w->timer--;
            r = fRand0_1() * (PI / 128) + PI / 128;
            if (pG->flags_51E4 & 1) {
                r = -r;
            }
            em->rot.y = tgt + r;
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        } else {
            em->rot.y = tgt;
            em->flags_3C8 |= 0x10000000;
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    emDoorMatUpdate(em);
}

static void emDoor_R1_Open2(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    switch (em->xFE) {
    case 0:
        em->flags_3C8 |= 0x20000000;
        emDoorSetDmgLock_L(em, 1);
        emDoorSetDmgLock_R(em, 1);
        emDoorSetDmgChain(em, 0);
        emDoorSetDmgChain(em, 1);
        emDoorSetDmgChain(em, 2);
        em->hp = 1000;
        switch (em->xFF) {
        case 1:
            MotionSetCore(em, &em->pMotion, PL_ARC_PTR(pG->pPlArc, 0x1F), 0, 0, 0x41, 0);
            break;
        case 0:
        case 2:
        default:
            MotionSetCore(em, &em->pMotion, PL_ARC_PTR(pG->pPlArc, 0x1F), 0, 0, 1, 0);
            break;
        case 3:
            MotionSetCore(em, &em->pMotion, PL_ARC_PTR(pG->pPlArc, 0x1F), 0, 0, 0x41, 0);
            break;
        }
        emDoorDropWeapon(em);
        em->xFE++;
    case 1:
        if (em->frame > 14.7f && em->frame < 15.3f) {
            if (w->seCancel == 0) {
                switch (em->type) {
                case 1:
                case 2:
                case 4:
                case 5:
                case 7:
                    w->sndId = SndCall(1, 0x41, &em->pos, 0, 0, em);
                    break;
                default:
                    w->sndId = SndCall(1, 0x42, &em->pos, 0, 0, em);
                    break;
                }
            }
            w->seCancel = 0;
        }
        if (MotionMove(em, 0)) {
            em->flags_3C8 |= 0x30000000;
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    em->partsWorldCalc();
}

void emDoor_R1_Down(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    u16* flg;
    f32 ang;
    f32 h;
    f32 r;
    int water;
    Vec v;

    switch (em->xFE) {
    case 0:
        em->flags_3C8 |= 0x20000000;
        w->kickCnt = 1;
        w->timer = 0x14;
        w->x400 = 0x96;
        emDoorSetDmgLock_L(em, 1);
        emDoorSetDmgLock_R(em, 1);
        emDoorSetDmgChain(em, 0);
        emDoorSetDmgChain(em, 1);
        emDoorSetDmgChain(em, 2);
        ang = Muku(&em->pos, &w->target, w->rotY, PI);
        if (fabsf(ang) > PI / 2) {
            w->dir = 0;
        } else {
            w->dir = 1;
        }
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        if (flg) {
            *flg |= 1;
            if (w->dir == 0) {
                *flg |= 0x40;
            } else {
                *flg |= 0x80;
            }
        }
        water = 0;
        if (GetWaterHeight(&em->pos, &h) && em->pos.y < h) {
            water = 1;
        }
        if (em->type == 2 || em->type == 3) {
            if (water) {
                EstSet((int) em, -1, 0, 0, w->eff, 0xA, 0, 0, (u32) em, 0);
                SndCall(6, 3, &em->pos, 0, 0, em);
            } else {
                EstSet((int) em, -1, 0, 0, w->eff, 7, 0, 0, (u32) em, 0);
            }
        }
        w->spd = 0.0f;
        emDoorDropWeapon(em);
        em->hp = 0;
        if (w->dir) {
            w->spd = -0.122173f;
        } else {
            w->spd = 0.122173f;
        }
        em->pos.y += 40.0f;
        em->xFE++;
    case 1:
        if (w->dir) {
            w->spd -= 0.0174533f;
            em->rot.x += w->spd;
            if (em->rot.x < -PI / 2) {
                em->rot.x = -PI / 2;
                em->xFE++;
            }
        } else {
            w->spd += 0.0174533f;
            em->rot.x += w->spd;
            if (em->rot.x > PI / 2) {
                em->rot.x = PI / 2;
                em->xFE++;
            }
        }
        if (w->kickCnt != 0) {
            w->kickCnt--;
            em->dmType = 2;
            if (w->dir) {
                v.x = 0.0f;
                v.y = w->width;
                v.z = -600.0f;
            } else {
                v.x = 0.0f;
                v.y = w->width;
                v.z = 600.0f;
            }
            PSMTXMultVec(w->mat, &v, &v);
            if (em->xFF != 0) {
                emDoorKickHit(em, &v, 0x14, &pPL->pos, pPL);
            }
        }
        break;
    case 2:
        w->timer = 2;
        water = 0;
        if (GetWaterHeight(&em->pos, &h) && em->pos.y < h) {
            water = 1;
        }
        if (em->type == 2 || em->type == 3) {
            if (water) {
                if (w->dir) {
                    EstSet((int) em, -1, 0, 0, w->eff, 0xC, 0, 0, (u32) em, 0);
                } else {
                    EstSet((int) em, -1, 0, 0, w->eff, 0xB, 0, 0, (u32) em, 0);
                }
                SndCall(6, 3, &em->pos, 0, 0, em);
            } else {
                if (w->dir) {
                    EstSet((int) em, -1, 0, 0, w->eff, 9, 0, 0, (u32) em, 0);
                } else {
                    EstSet((int) em, -1, 0, 0, w->eff, 8, 0, 0, (u32) em, 0);
                }
            }
        }
        em->xFE++;
    case 3:
        if (w->dir) {
            em->rot.x = -PI / 2;
        } else {
            em->rot.x = PI / 2;
        }
        if (w->timer != 0) {
            w->timer--;
            r = fRand0_1() * (PI / 128) + PI / 128;
            if (pG->flags_51E4 & 1) {
                r = -r;
            }
            em->rot.x += r;
        } else {
            em->flags_3C8 |= 0x10000000;
            em->xFE++;
        }
        break;
    }
    emDoorMatUpdate(em);
}

void emDoor_R1_Downed(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    if (em->xFE == 0) {
        em->hp = 0;
        if (w->dir) {
            em->rot.x = -PI / 2;
        } else {
            em->rot.x = PI / 2;
        }
        em->pos.y += 40.0f;
        em->flags_3C8 |= 0x30000000;
        em->xFE++;
    }
    emDoorMatUpdate(em);
}

void emDoor_R1_Close(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    switch (em->xFE) {
    case 0:
        em->flags_3C8 &= ~0x30000000;
        w->timer = 0x14;
        em->xFE++;
    case 1:
        em->rot.y += Muku2(em->rot.y, w->rotY, PI) * 0.3f;
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (w->timer != 0) {
            w->timer--;
        } else {
            em->rot.y = w->rotY;
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    emDoorMatUpdate(em);
}

void emDoor_R1_Break(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    u16* flg;

    if (em->xFE == 0) {
        em->hp = 0;
        em->be_flag &= ~2;
        em->clearStatus(5);
        emDoorSatClear(em);
        em->flags_3C8 |= 0x30000000;
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        if (flg) {
            *flg |= 1;
        }
        if (w->keyNo != 0x36) {
            u32* tbl = pG->door_unlock;

            tbl[w->keyNo >> 5] |= 0x80000000 >> (w->keyNo & 0x1F);
        }
        em->xFE++;
    }
}

void emDoor_R1_Shock(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    f32 r;

    switch (em->xFE) {
    case 0:
        w->timer = 7;
        if (em->ckObj() == 0) {
            em->hp -= 25;
        } else {
            em->hp -= 50;
        }
        if (em->hp <= 0) {
            em->hp = 1;
        }
        em->xFE++;
    case 1:
        if (w->timer != 0) {
            w->timer--;
            r = fRand0_1() * (PI / 128) + PI / 128;
            if (pG->flags_51E4 & 1) {
                r = -r;
            }
            em->rot.y = w->rotY + r;
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        } else {
            em->rot.y = w->rotY;
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    emDoorMatUpdate(em);
}

void emDoor_R1_OpenLock(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    int unlock;

    switch (em->xFE) {
    case 0:
        em->flags_3C8 |= 0x30000000;
        w->timer = 0x14;
        if (em->xFF) {
            w->dir = 1;
            em->rot.y = w->rotY - PI / 2;
        } else {
            w->dir = 0;
            em->rot.y = w->rotY + PI / 2;
        }
        em->xFE++;
    case 1:
        unlock = !(w->flags & 1);
        if (unlock) {
            em->xFC = 1;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    emDoorMatUpdate(em);
}

void emDoor_R1_CloseLock(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    int unlock;

    switch (em->xFE) {
    case 0:
        em->flags_3C8 &= ~0x30000000;
        w->timer = 0x14;
        em->xFE++;
    case 1:
        em->rot.y += Muku2(em->rot.y, w->rotY, PI) * 0.3f;
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (w->timer != 0) {
            w->timer--;
        } else {
            em->rot.y = w->rotY;
            unlock = !(w->flags & 1);
            if (unlock) {
                em->xFC = 1;
                em->xFD = 0;
                em->xFE = 0;
                em->xFF = 0;
            }
        }
        break;
    }
    emDoorMatUpdate(em);
}

void emDoorLockBendMove(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    cModel* parts;

    if (w->pLockL) {
        parts = w->pLockL->getPartsPtr(1);
        parts->rot.x = w->bendL;
        w->bendL *= 0.7f;
    }
    if (w->pLockR) {
        parts = w->pLockR->getPartsPtr(1);
        parts->rot.x = w->bendR;
        w->bendR *= 0.7f;
    }
    if (w->pChain) {
        parts = w->pChain->getPartsPtr(1);
        parts->rot.x = w->bendChain;
        parts = w->pChain->getPartsPtr(2);
        parts->rot.x = w->bendChain;
        parts = w->pChain->getPartsPtr(3);
        parts->rot.x = w->bendChain;
        w->bendChain *= 0.7f;
    }
}

// Four corners of an effect collision panel: x0..x1 along the door, at height y, 50 deep.
static inline void emDoorSatPoly(Vec* poly, f32 x0, f32 x1, f32 y)
{
    poly[0].x = x0;
    poly[0].y = y;
    poly[0].z = -50.0f;
    poly[1].x = x1;
    poly[1].y = y;
    poly[1].z = -50.0f;
    poly[2].x = x1;
    poly[2].y = y;
    poly[2].z = 50.0f;
    poly[3].x = x0;
    poly[3].y = y;
    poly[3].z = 50.0f;
}

void emDoorSatSet(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    Vec poly[4];
    f32 h;
    int attr;

    emDoorSatClear(em);
    if (em->hp <= 0) {
        return;
    }
    em->atari.setFlag200();
    switch (em->type) {
    case 1:
    case 4:
    case 5:
        attr = 0;
        break;
    case 2:
        attr = 0x404000;
        break;
    case 7:
        attr = 0x400000;
        break;
    default:
        attr = 0x400000;
        break;
    }
    if (w->sat[1] == 0) {
        switch (em->type) {
        case 4:
            emDoorSatPoly(poly, -w->width * 2.0f, 0.0f, 0.0f);
            h = 1300.0f;
            break;
        case 5:
            emDoorSatPoly(poly, -w->width * 2.0f, 0.0f, 0.0f);
            h = 1350.0f;
            break;
        default:
            emDoorSatPoly(poly, -w->width * 2.0f, 0.0f, 0.0f);
            h = 800.0f;
            break;
        }
        w->sat[1] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
    } else {
        w->sat[1]->flags |= 4;
        w->sat[1]->setCoord(&em->pos, &em->rot);
    }
    if (!(em->flags_3C8 & 0x7500)) {
        if (w->sat[2] == 0) {
            switch (em->type) {
            case 4:
                emDoorSatPoly(poly, -w->width * 2.0f, 0.0f, 1880.0f);
                h = w->height - 1880.0f;
                break;
            case 5:
                emDoorSatPoly(poly, -w->width * 2.0f, 0.0f, 1800.0f);
                h = w->height - 1800.0f;
                break;
            default:
                emDoorSatPoly(poly, -w->width * 2.0f, 0.0f, 800.0f);
                h = 600.0f;
                break;
            }
            w->sat[2] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
        } else {
            w->sat[2]->flags |= 4;
            w->sat[2]->setCoord(&em->pos, &em->rot);
        }
    }
    if (!(em->flags_3C8 & 0x8AC0)) {
        if (w->sat[3] == 0) {
            switch (em->type) {
            case 4:
                emDoorSatPoly(poly, -w->width * 2.0f + 320.0f, -320.0f, 1300.0f);
                h = 580.0f;
                w->sat[3] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
                break;
            case 5:
                emDoorSatPoly(poly, -w->width * 2.0f + 150.0f, -150.0f, 1350.0f);
                h = 500.0f;
                w->sat[3] = EatMgr.create(&em->pos, &em->rot, poly, 0x404000, 0, h);
                break;
            default:
                emDoorSatPoly(poly, -w->width * 2.0f, 0.0f, 1400.0f);
                h = w->height - 1400.0f;
                w->sat[3] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
                break;
            }
        } else {
            w->sat[3]->flags |= 4;
            w->sat[3]->setCoord(&em->pos, &em->rot);
        }
    }
    if (w->sat[4] == 0) {
        switch (em->type) {
        case 4:
            emDoorSatPoly(poly, -w->width * 2.0f, -w->width * 2.0f + 320.0f, 1300.0f);
            h = 580.0f;
            w->sat[4] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
            break;
        case 5:
            emDoorSatPoly(poly, -w->width * 2.0f, -w->width * 2.0f + 150.0f, 1300.0f);
            h = 580.0f;
            w->sat[4] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
            break;
        }
    } else {
        w->sat[4]->flags |= 4;
        w->sat[4]->setCoord(&em->pos, &em->rot);
    }
    if (w->sat[5] == 0) {
        switch (em->type) {
        case 4:
            emDoorSatPoly(poly, -320.0f, 0.0f, 1300.0f);
            h = 580.0f;
            w->sat[5] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
            break;
        case 5:
            emDoorSatPoly(poly, -150.0f, 0.0f, 1300.0f);
            h = 580.0f;
            w->sat[5] = EatMgr.create(&em->pos, &em->rot, poly, attr, 0, h);
            break;
        }
    } else {
        w->sat[5]->flags |= 4;
        w->sat[5]->setCoord(&em->pos, &em->rot);
    }
}

void emDoorSatClear(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);

    em->atari.clrFlag200();
    if (w->sat[1]) {
        w->sat[1]->flags &= ~4;
    }
    if (w->sat[2]) {
        w->sat[2]->flags &= ~4;
    }
    if (w->sat[3]) {
        w->sat[3]->flags &= ~4;
    }
    if (w->sat[4]) {
        w->sat[4]->flags &= ~4;
    }
    if (w->sat[5]) {
        w->sat[5]->flags &= ~4;
    }
}

void emDoorLockMove(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    Vec v;
    Vec rot;

    if (w->pLockL) {
        v.x = -1160.0f;
        v.y = 1050.0f;
        v.z = 80.0f;
        PSMTXMultVec(em->mat, &v, &v);
        w->pLockL->pos = v;
        w->pLockL->rot = em->rot;
    }
    if (w->pLockR) {
        rot = em->rot;
        rot.y += PI;
        rot.y = LIMIT_ANGLE(rot.y);
        v.x = -1160.0f;
        v.y = 1050.0f;
        v.z = -80.0f;
        PSMTXMultVec(em->mat, &v, &v);
        w->pLockR->pos = v;
        w->pLockR->rot = rot;
    }
    if (w->pChain) {
        rot = em->rot;
        v.x = -650.0f;
        v.y = 1550.0f;
        v.z = 30.0f;
        PSMTXMultVec(em->mat, &v, &v);
        w->pChain->pos = v;
        w->pChain->rot = rot;
    }
}

// `v` (in the closed door's space) is inside the door's passage box.
static inline int emDoorNearCk(EmDoorWork* w, Vec* v)
{
    int ok = 1;
    f32 lim;

    if (v->z > 1500.0f) {
        ok = 0;
    }
    if (v->z < -1500.0f) {
        ok = 0;
    }
    if (v->y > 500.0f) {
        ok = 0;
    }
    if (v->y < -500.0f) {
        ok = 0;
    }
    if (v->x > w->width + 150.0f) {
        ok = 0;
    }
    if (w->pDoor) {
        lim = -(w->width * 3.0f + 150.0f);
    } else {
        lim = -(w->width + 150.0f);
    }
    if (v->x < lim) {
        ok = 0;
    }
    return ok;
}

int emDoorDoorAutoCloseCk(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    Vec v;
    u32 i;

    if (!(em->flags_3C8 & 0x20000000)) {
        return 0;
    }
    PSMTXMultVec(w->inv, &pPL->pos, &v);
    if (emDoorNearCk(w, &v)) {
        return 0;
    }
    if (pSUB) {
        PSMTXMultVec(w->inv, &pSUB->pos, &v);
        if (emDoorNearCk(w, &v)) {
            return 0;
        }
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);

        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->checkStatus(5) == 0) {
            continue;
        }
        if (e->id >= 0x40 && e->id <= 0x4A) {
            continue;
        }
        if (e == em) {
            continue;
        }
        PSMTXMultVec(w->inv, &e->pos, &v);
        if (emDoorNearCk(w, &v)) {
            return 0;
        }
    }
    if (em->ckObj() == 0) {
        return 0;
    }
    em->flags_3C8 &= ~0x30000000;
    em->xFC = 1;
    em->xFD = 3;
    em->xFE = 0;
    em->xFF = 0;
    return 1;
}

void emDoorYarareInit(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    u16 flags;

    if (em->type == 2) {
        return;
    }
    if (em->type == 7) {
        return;
    }
    flags = 0x41;
    if (em->type == 0) {
        flags = 0x21;
    }
    switch (em->type) {
    default:
        YarareInitCube(em, -w->width, 0.0f, 0.0f, w->width, w->height, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[0], -w->width, w->height - 300.0f, 0.0f, w->width, 300.0f, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[1], -100.0f, 0.0f, 0.0f, 100.0f, w->height, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[2], -(w->width * 2.0f - 100.0f), 0.0f, 0.0f, 100.0f, w->height, 55.0f, 0, flags);
        break;
    case 4:
        YarareInitCube(em, -w->width, 0.0f, 0.0f, w->width, w->height - 1000.0f, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[0], -w->width, w->height - 370.0f, 0.0f, w->width, 370.0f, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[1], -170.0f, 0.0f, 0.0f, 170.0f, w->height, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[2], -(w->width * 2.0f - 170.0f), 0.0f, 0.0f, 170.0f, w->height, 55.0f, 0, flags);
        break;
    case 5:
        YarareInitCube(em, -w->width, 0.0f, 0.0f, w->width, 1350.0f, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[0], -w->width, w->height - 600.0f, 0.0f, w->width, 600.0f, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[1], -75.0f, 0.0f, 0.0f, 75.0f, w->height, 55.0f, 0, flags);
        YarareAddCube(em, &w->hit[2], -(w->width * 2.0f - 75.0f), 0.0f, 0.0f, 75.0f, w->height, 55.0f, 0, flags);
        break;
    }
}

void cEmDoor::setLock(void* bin, void* tpl, int side, int strong)
{
    EmDoorWork* w = EMDOOR_WK(this);
    u16* flg;
    Mtx m;
    Vec v;
    Vec r;

    flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
    if (hp <= 0) {
        return;
    }
    if (side) {
        if (flg && (*flg & 4)) {
            return;
        }
        lockOfs.x = -1160.0f;
        lockOfs.y = 1050.0f;
        lockOfs.z = 150.0f;
        flags_3C8 |= 0x80000000;
        lockParts = 0;
        clearStatus(1);
        v.x = -1160.0f;
        v.y = 1050.0f;
        v.z = 80.0f;
        RotMatrix(m, &rot);
        TransMatrix(m, &pos);
        ScaleMatrix(m, &scale);
        PSMTXMultVec(m, &v, &v);
        w->pLockL = (cObj12*) SetObj12(bin, tpl, &v, &rot);
        if (w->pLockL) {
            w->pLockL->lightInfo.x50 = 0x10;
            w->pLockL->setNoSuspend(1);
        }
        YarareAddCube(this, &w->hit[12], -1150.0f, 800.0f, 60.0f, 150.0f, 350.0f, 100.0f, 0, 1);
        w->lockHpL = (Rnd() & 1) + 3;
        if (strong) {
            w->lockHpL = 0xF;
            w->flags |= 2;
        }
    } else {
        if (flg && (*flg & 2)) {
            return;
        }
        lockOfs.x = -1160.0f;
        lockOfs.y = 1050.0f;
        lockOfs.z = -150.0f;
        flags_3C8 |= 0x40000000;
        lockParts = 0;
        clearStatus(1);
        v.x = -1160.0f;
        v.y = 1050.0f;
        v.z = -80.0f;
        RotMatrix(m, &rot);
        TransMatrix(m, &pos);
        ScaleMatrix(m, &scale);
        PSMTXMultVec(m, &v, &v);
        r = rot;
        r.y += PI;
        r.y = LIMIT_ANGLE(r.y);
        w->pLockR = (cObj12*) SetObj12(bin, tpl, &v, &r);
        if (w->pLockR) {
            w->pLockR->lightInfo.x50 = 4;
            w->pLockR->setNoSuspend(1);
        }
        YarareAddCube(this, &w->hit[11], -1150.0f, 800.0f, -60.0f, 150.0f, 350.0f, 100.0f, 0, 1);
        w->lockHpR = (Rnd() & 1) + 3;
        if (strong) {
            w->lockHpR = 0xF;
            w->flags |= 2;
        }
    }
}

int cEmDoor::ckLock()
{
    EmDoorWork* w = EMDOOR_WK(this);

    if (w->pLockL) {
        return 1;
    }
    if (w->pLockR) {
        return 1;
    }
    return 0;
}

void cEmDoor::setChain(void* bin, void* tpl)
{
    EmDoorWork* w = EMDOOR_WK(this);
    u16* flg;
    Mtx m;
    Vec v;

    if (hp <= 0) {
        return;
    }
    flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
    v.x = -650.0f;
    v.y = 1550.0f;
    v.z = 30.0f;
    RotMatrix(m, &rot);
    TransMatrix(m, &pos);
    ScaleMatrix(m, &scale);
    PSMTXMultVec(m, &v, &v);
    w->pChain = (cObj12*) SetObj12(bin, tpl, &v, &rot);
    if (w->pChain == 0) {
        return;
    }
    w->pChain->lightInfo.x50 = 0x10;
    w->pChain->setNoSuspend(1);
    if (flg) {
        if (!(*flg & 8)) {
            YarareAddCube(this, &w->hit[13], -650.0f, 1350.0f, 50.0f, 650.0f, 350.0f, 100.0f, 0, 1);
            w->chainHp[0] = 2;
        } else {
            cModel* parts = w->pChain->getPartsPtr(1);

            parts->scale.x = 0.0f;
            parts->scale.y = 0.0f;
            parts->scale.z = 0.0f;
        }
        if (!(*flg & 0x10)) {
            YarareAddCube(this, &w->hit[14], -650.0f, 1000.0f, 50.0f, 650.0f, 350.0f, 100.0f, 0, 1);
            w->chainHp[1] = 2;
        } else {
            cModel* parts = w->pChain->getPartsPtr(2);

            parts->scale.x = 0.0f;
            parts->scale.y = 0.0f;
            parts->scale.z = 0.0f;
        }
        if (!(*flg & 0x20)) {
            YarareAddCube(this, &w->hit[15], -650.0f, 700.0f, 50.0f, 650.0f, 300.0f, 100.0f, 0, 1);
            w->chainHp[2] = 2;
        } else {
            cModel* parts = w->pChain->getPartsPtr(3);

            parts->scale.x = 0.0f;
            parts->scale.y = 0.0f;
            parts->scale.z = 0.0f;
        }
    }
}

void cEmDoor::setEff(u8 eff)
{
    EMDOOR_WK(this)->eff = eff;
}

// Pane `no` (parts no + 1, hit box hit[no + 1]) of a wooden door: a hit box while whole, hidden when broken.
static inline void emDoorPaneSet(cEmDoor* em, u32 bit, int no, u16 flags)
{
    EmDoorWork* w = EMDOOR_WK(em);

    if (!(em->flags_3C8 & bit)) {
        YarareAddCube(em, &w->hit[no + 1], 0.0f, -300.0f, 0.0f, 300.0f, 600.0f, 65.0f, no, flags);
    } else {
        cModel* parts = em->getPartsPtr(no - 1);

        parts->scale.x = 0.0f;
        parts->scale.y = 0.0f;
        parts->scale.z = 0.0f;
    }
}

void cEmDoor::setYarare()
{
    EmDoorWork* w = EMDOOR_WK(this);
    u16 flags;
    u32 bit;
    u16* flg;
    cModel* parts;

    if (type == 0 || type == 4) {
        flags = 0x21;
    } else {
        flags = 0x41;
    }
    if (type != 4) {
        hitInfo.height = 500.0f;
        bit = 0x8000;
        emDoorPaneSet(this, bit, 2, flags);
        bit >>= 1;
        emDoorPaneSet(this, bit, 3, flags);
        bit >>= 1;
        emDoorPaneSet(this, bit, 4, flags);
        bit >>= 1;
        emDoorPaneSet(this, bit, 5, flags);
        bit >>= 1;
        emDoorPaneSet(this, bit, 6, flags);
        bit >>= 1;
        emDoorPaneSet(this, bit, 7, flags);
        bit >>= 1;
        emDoorPaneSet(this, bit, 8, flags);
        bit >>= 1;
        emDoorPaneSet(this, bit, 9, flags);
    } else {
        flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
        parts = getPartsPtr(1);
        YarareAddCube(this, &w->hit[3], 0.0f, -350.0f, 0.0f, 350.0f, 700.0f, 65.0f, 2, flags);
        parts->scale.x = 1.0f;
        parts->scale.y = 1.0f;
        parts->scale.z = 1.0f;
        if (flg && (*flg & 8)) {
            w->hit[3].flags &= ~1;
            parts->scale.x = 0.0f;
            parts->scale.y = 0.0f;
            parts->scale.z = 0.0f;
            flags_3C8 |= 0xFFC0;
        }
    }
}

u32 cEmDoor::ckOpen()
{
    EmDoorWork* w = EMDOOR_WK(this);

    if (hp <= 0) {
        return 1;
    }
    if (w->flags & 1) {
        return 3;
    }
    if (flags_3C8 & 0x20000000) {
        return 1;
    }
    if (w->lockHpL > 0) {
        return 3;
    }
    if (w->lockHpR > 0) {
        return 3;
    }
    if (w->chainHp[0] > 0) {
        return 3;
    }
    if (w->chainHp[1] > 0) {
        return 3;
    }
    if (w->chainHp[2] > 0) {
        return 3;
    }
    if (ckObj() == 0) {
        return 2;
    }
    if (w->keyNo != 0x36 && emDoorKeyCk(w->keyNo) == 0) {
        return 3;
    }
    return 0;
}

int cEmDoor::ckKick(Vec* pos)
{
    EmDoorWork* w = EMDOOR_WK(this);
    Mtx m;
    Vec v;

    if (w->keyNo != 0x36 && emDoorKeyCk(w->keyNo) == 0) {
        return 0;
    }
    if (w->lockHpL > 1) {
        return 0;
    }
    if (w->lockHpR > 1) {
        return 0;
    }
    if (w->chainHp[0] + w->chainHp[1] + w->chainHp[2] > 1) {
        return 0;
    }
    PSMTXRotRad(m, 'y', w->rotY);
    TransMatrix(m, &this->pos);
    v.x = -1000.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    PSMTXMultVec(m, &v, &v);
    return 1;
}

void cEmDoor::setOpen(Vec* pos, int a, int b, int c)
{
    EmDoorWork* w = EMDOOR_WK(this);

    if (hp <= 0) {
        return;
    }
    w->target = *pos;
    if (type == 2 || type == 3) {
        if (a) {
            xFC = 1;
            xFD = 8;
            xFE = 0;
            xFF = 1;
        } else {
            xFC = 1;
            xFD = 8;
            xFE = 0;
            xFF = 0;
        }
    } else if (c) {
        if (a) {
            xFC = 1;
            xFD = 8;
            xFE = 0;
            xFF = 1;
        } else {
            xFC = 1;
            xFD = 8;
            xFE = 0;
            xFF = 0;
        }
    } else {
        if (a) {
            xFC = 1;
            xFD = 1;
            xFE = 0;
            xFF = 1;
        } else {
            xFC = 1;
            xFD = 1;
            xFE = 0;
            xFF = 0;
        }
    }
    flags_3C8 |= 0x20000000;
    emDoorSetDmgLock_L(this, 1);
    emDoorSetDmgLock_R(this, 1);
    emDoorSetDmgChain(this, 0);
    emDoorSetDmgChain(this, 1);
    emDoorSetDmgChain(this, 2);
    if (b == 0) {
        switch (type) {
        default:
            if (a) {
                SndCall(1, 0x1E, &this->pos, 0, 0, this);
            } else {
                SndCall(1, 0x19, &this->pos, 0, 0, this);
            }
            break;
        case 1:
        case 4:
        case 5:
        case 7:
            if (a) {
                SndCall(1, 0x20, &this->pos, 0, 0, this);
            } else {
                SndCall(1, 0x1C, &this->pos, 0, 0, this);
            }
            break;
        case 2:
            if (a) {
                SndCall(6, 0x17, &this->pos, 0, 0, this);
            } else {
                SndCall(1, 0x1C, &this->pos, 0, 0, this);
            }
            break;
        case 3:
            if (a) {
                SndCall(6, 0x2C, &this->pos, 0, 0, this);
            } else {
                SndCall(6, 0x2C, &this->pos, 0, 0, this);
            }
            break;
        }
    }
}

void cEmDoor::setOpen2(int a)
{
    int near;

    if (hp <= 0) {
        return;
    }
    near = 0;
    if (pSUB && pSUB->plDist2 < 25000000.0f) {
        near = 1;
    }
    if (near) {
        if (a) {
            xFC = 1;
            xFD = 2;
            xFE = 0;
            xFF = 3;
        } else {
            xFC = 1;
            xFD = 2;
            xFE = 0;
            xFF = 2;
        }
    } else {
        if (a) {
            xFC = 1;
            xFD = 2;
            xFE = 0;
            xFF = 1;
        } else {
            xFC = 1;
            xFD = 2;
            xFE = 0;
            xFF = 0;
        }
    }
    flags_3C8 |= 0x20000000;
    emDoorSetDmgLock_L(this, 1);
    emDoorSetDmgLock_R(this, 1);
    emDoorSetDmgChain(this, 0);
    emDoorSetDmgChain(this, 1);
    emDoorSetDmgChain(this, 2);
}

void cEmDoor::setShock(int a, Vec* pos, int b)
{
    EmDoorWork* w = EMDOOR_WK(this);
    f32 ang;
    u32 i;

    if (hp <= 0) {
        return;
    }
    ang = fabsf(Muku(&this->pos, pos, w->rotY, PI));
    if (ang < PI / 2) {
        if (w->pLockL) {
            EstSet((int) w->pLockL, -1, 0, 0, 0xC9, 1, 0, 0, (u32) w->pLockL, 0);
            w->bendL = -PI / 2;
        }
    } else {
        if (w->pLockR) {
            EstSet((int) w->pLockR, -1, 0, 0, 0xC9, 1, 0, 0, (u32) w->pLockR, 0);
            w->bendR = -PI / 2;
        }
    }
    if (w->pChain) {
        for (i = 0; i <= 2; i++) {
            if (w->chainHp[i] > 0) {
                switch (i) {
                case 0:
                    EstSet((int) w->pChain, -1, 0, 0, 0xCB, 4, 0, 0, (u32) w->pChain, 0);
                    break;
                case 1:
                    EstSet((int) w->pChain, -1, 0, 0, 0xCB, 5, 0, 0, (u32) w->pChain, 0);
                    break;
                case 2:
                    EstSet((int) w->pChain, -1, 0, 0, 0xCB, 6, 0, 0, (u32) w->pChain, 0);
                    break;
                }
                break;
            }
        }
        w->bendChain = -PI / 2;
        if (a == 2) {
            return;
        }
        SndCall(6, 0x15, &this->pos, 0, 0, this);
    }
    if (a == 2) {
        return;
    }
    if (ang < PI / 2) {
        if (w->pLockL) {
            if (w->lockHpL > 1) {
                w->lockHpL--;
                if ((Rnd() & 1) && !(w->flags & 2) && w->lockHpL > 1) {
                    w->lockHpL--;
                }
            }
            EstSet((int) w->pLockL, -1, 0, 0, 0xC9, 1, 0, 0, (u32) w->pLockL, 0);
            w->bendL = -PI / 2;
        }
    } else {
        if (w->pLockR) {
            if (w->lockHpR > 1) {
                w->lockHpR--;
                if ((Rnd() & 1) && !(w->flags & 2) && w->lockHpR > 1) {
                    w->lockHpR--;
                }
            }
            EstSet((int) w->pLockR, -1, 0, 0, 0xC9, 1, 0, 0, (u32) w->pLockR, 0);
            w->bendR = -PI / 2;
        }
    }
    if (w->pChain) {
        for (i = 0; i <= 2; i++) {
            if (w->chainHp[i] > 0) {
                w->chainHp[i]--;
                if (w->chainHp[i] <= 0) {
                    w->chainHp[i] = 1;
                    emDoorSetDmgChain(this, i);
                }
                break;
            }
        }
    }
    if (b == 0) {
        switch (type) {
        case 0:
        default:
            if (a) {
                SndCall(1, 0x1D, &this->pos, 0, 0, this);
            } else {
                SndCall(1, 0x18, &this->pos, 0, 0, this);
            }
            break;
        case 1:
        case 2:
        case 4:
        case 5:
        case 7:
            if (a) {
                SndCall(1, 0x1F, &this->pos, 0, 0, this);
            } else {
                SndCall(1, 0x1B, &this->pos, 0, 0, this);
            }
            break;
        case 3:
            if (a) {
                SndCall(6, 0x2B, &this->pos, 0, 0, this);
            } else {
                SndCall(6, 0x2B, &this->pos, 0, 0, this);
            }
            break;
        }
        if (w->pLockR || w->pLockL) {
            SndCall(6, 0x5D, &this->pos, 0, 0, this);
        }
    }
    xFC = 1;
    xFD = 5;
    xFE = 0;
    xFF = 0;
}

void cEmDoor::setBreak(Vec* pos)
{
    EmDoorWork* w = EMDOOR_WK(this);
    int zero;

    if (hp <= 0) {
        return;
    }
    emDoorSetDmgLock_L(this, 1);
    emDoorSetDmgLock_R(this, 1);
    emDoorSetDmgChain(this, 0);
    emDoorSetDmgChain(this, 1);
    emDoorSetDmgChain(this, 2);
    if (type == 2 || type == 3) {
        setOpen(pos, 0, 0, 0);
        return;
    }
    zero = 0;
    EstSet((int) this, -1, 0, 0, w->eff, 6, 0, 0, (u32) this, (void*) zero);
    SndCall(6, 0x37, &this->pos, 0, 0, this);
    hp = zero;
    xFC = 1;
    xFD = 4;
    xFE = 0;
    xFF = 0;
}

void emDoorActEvtCk(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    cEmDoor* pDoor;
    f32 ang;
    f32 lim;
    Vec v;

    if (em->flags_3C8 & 0x20000000) {
        return;
    }
    if (em->hp <= 0) {
        return;
    }
    if (w->flags & 1) {
        return;
    }
    if (w->keyNo != 0x36 && emDoorKeyCk(w->keyNo) == 0) {
        return;
    }
    ang = fabsf(Muku2(w->rotY, pPL->rot.y, PI));
    if (ang > PI / 4) {
        if (ang < PI * 3 / 4) {
            return;
        }
    }
    PSMTXMultVec(w->inv, &pPL->pos, &v);
    if (ang < PI / 2) {
        if (v.z > 0.0f) {
            return;
        }
        if (v.z < -800.0f) {
            return;
        }
        if (v.x > w->width) {
            return;
        }
        pDoor = w->pDoor;
        if (pDoor) {
            lim = -(w->width + 250.0f);
        } else {
            lim = -w->width;
        }
    } else {
        if (v.z < 0.0f) {
            return;
        }
        if (v.z > 800.0f) {
            return;
        }
        pDoor = w->pDoor;
        if (pDoor) {
            if (v.x > w->width + 250.0f) {
                return;
            }
        } else {
            if (v.x > w->width) {
                return;
            }
        }
        lim = -w->width;
    }
    if (v.x < lim) {
        return;
    }
    if (v.y > 500.0f) {
        return;
    }
    if (v.y < -500.0f) {
        return;
    }
    if (pDoor && v.x < -250.0f && pDoor->ckOpen() == 0) {
        ActBtn.set(0x10, 5, (int) emDoorAction2, (int) em, 0, 1, 0, 0);
    } else {
        ActBtn.set(0x10, 5, (int) emDoorAction, (int) em, 0, 1, 0, 0);
    }
}

void emDoorAction(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    int kick;

    kick = 0;
    if (em->ckOpen()) {
        kick = 1;
    }
    if (w->pDoor && w->pDoor->ckOpen() > 1) {
        kick = 1;
    }
    if (em->type == 2) {
        kick = 1;
    }
    if (em->type == 3) {
        kick = 1;
    }
    if (kick) {
        SetPlDamage((int) em, plemDoorKick);
    } else {
        SetPlDamage((int) em, plemDoorOpen);
    }
}

void emDoorAction2(cEmDoor* em)
{
    EmDoorWork* w = EMDOOR_WK(em);
    int kick;

    kick = 0;
    if (em->ckOpen()) {
        kick = 1;
    }
    if (w->pDoor && w->pDoor->ckOpen() > 1) {
        kick = 1;
    }
    if (em->type == 2) {
        kick = 1;
    }
    if (em->type == 3) {
        kick = 1;
    }
    if (kick) {
        SetPlDamage((int) em, plemDoorKick);
    } else {
        SetPlDamage((int) em, plemDoorOpen);
    }
    pPL->xFF = 1;
}

// The bell position marks where the door was kicked / opened (pG->bell_pos).
static inline void emDoorBellSet(Vec* pos)
{
    BitOn(pG->flags_5010, 0x20000000);
    memcpy((u8*) pG + ((u32) &((GlobalWork*) 0)->bell_pos), pos, sizeof(Vec));
    pG->bell_stat = 0;
}

void plemDoorKick(cPlayer* pl)
{
    cEmDoor* door = (cEmDoor*) pl->dmgType;
    cEmDoor* door2 = (cEmDoor*) pPL->dmgType;
    EmDoorWork* w = EMDOOR_WK(door2);
    int frame;
    Vec v;

    pl->x378 = door2->x378;
    if (pl->xFE == 0 || pl->xFE == 4) {
        if (door->ckKick(&pPL->pos) && (w->pDoor == 0 || w->pDoor->ckKick(&pPL->pos))) {
            if (pl->xFE == 0) {
                pl->xFE = 2;
            }
            if (pl->xFE == 4) {
                pl->xFE = 6;
            }
        }
    }
    frame = 0;
    if (pl->xFE == 4) {
        pl->xFE = 0;
        frame = 6;
    }
    if (pl->xFE == 6) {
        pl->xFE = 2;
        frame = 6;
    }
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, &pl->pMotion, PL_ARC_PTR(pG->pPlArc, 0x1D), 0, 5, 1, frame);
        pl->xFE++;
    case 1:
        if (pl->frame > 13.7f && pl->frame < 14.3f) {
            door->setShock(1, &pl->pos, 0);
            if (pl->xFF && w->pDoor && !(w->pDoor->flags_3C8 & 0x10000000)) {
                w->pDoor->setShock(1, &pl->pos, 1);
            }
            emDoorBellSet(&pl->pos);
        }
        if (pl->frame > 16.7f && pl->frame < 17.3f) {
            door->setShock(2, &pl->pos, 0);
            if (pl->xFF && w->pDoor && !(w->pDoor->flags_3C8 & 0x10000000)) {
                w->pDoor->setShock(2, &pl->pos, 1);
            }
        }
        if (MotionMove(pl, 0)) {
            EndPlDamage();
        }
        break;
    case 2:
        MotionSetCore(pl, &pl->pMotion, PL_ARC_PTR(pG->pPlArc, 0x1C), 0, 5, 1, frame);
        pl->xFE++;
    case 3:
        if (pl->frame > 13.7f && pl->frame < 14.3f) {
            v.x = 0.0f;
            v.y = 0.0f;
            v.z = -500.0f;
            PSMTXMultVec(pl->mat, &v, &v);
            door->setOpen(&v, 1, 0, 0);
            if (pl->xFF && w->pDoor && !(w->pDoor->flags_3C8 & 0x10000000)) {
                w->pDoor->setOpen(&v, 1, 0, 0);
                w->pDoor->setSeCancel();
            }
            emDoorBellSet(&pl->pos);
        }
        if (MotionMove(pl, 0)) {
            EndPlDamage();
        }
        break;
    }
    pl->x378 = pl->x37C;
}

void plemDoorOpen(cPlayer* pl)
{
    cEmDoor* door = (cEmDoor*) pl->dmgType;
    cEmDoor* door2 = (cEmDoor*) pPL->dmgType;
    EmDoorWork* w = EMDOOR_WK(door2);
    f32 d;
    u8 flag;
    Vec v;

    pl->x378 = door2->x378;
    switch (pl->xFE) {
    case 0:
        d = Muku2(pl->rot.y, door->rot.y, PI);
        if (fabsf(d) > PI / 2) {
            v.x = -638.54f;
            v.y = 0.0f;
            v.z = 440.4f;
            PSMTXMultVec(((cEmDoor*) pl->dmgType)->mat, &v, &v);
            PSVECSubtract(&v, &pPL->pos, &pl->evTarget);
            pl->evTarget.y = 0.0f;
            pl->x400 = ((cEmDoor*) pl->dmgType)->rot.y + PI;
            pl->x400 = LIMIT_ANGLE(pl->x400);
            MotionSetCore(pl, &pl->pMotion, PL_ARC_PTR(pG->pPlArc, 0x1E), 0, 5, 1, 0);
            door->setOpen2(0);
            if (pl->xFF && w->pDoor && !(w->pDoor->flags_3C8 & 0x10000000)) {
                w->pDoor->setOpen2(1);
                w->pDoor->setSeCancel();
            }
        } else {
            v.x = -638.54f;
            v.y = 0.0f;
            v.z = -440.4f;
            PSMTXMultVec(((cEmDoor*) pl->dmgType)->mat, &v, &v);
            PSVECSubtract(&v, &pPL->pos, &pl->evTarget);
            pl->evTarget.y = 0.0f;
            pl->x400 = ((cEmDoor*) pl->dmgType)->rot.y;
            MotionSetCore(pl, &pl->pMotion, PL_ARC_PTR(pG->pPlArc, 0x1E), 0, 5, 1, 0);
            door->setOpen2(1);
            if (pl->xFF && w->pDoor && !(w->pDoor->flags_3C8 & 0x10000000)) {
                w->pDoor->setOpen2(0);
                w->pDoor->setSeCancel();
            }
        }
        pl->dmg.set(0, 0x26);
        pl->atari.clrFlag200();
        pl->x3E0 = 0x2D;
        pl->xFE++;
    case 1:
        pl->rot.y += Muku2(pl->rot.y, pl->x400, PI / 16);
        pl->rot.y = LIMIT_ANGLE(pl->rot.y);
        if (pl->xFF == 0) {
            PSVECScale(&pl->evTarget, &v, 0.1f);
            PSVECAdd(&pl->pos, &v, &pl->pos);
            PSVECSubtract(&pl->evTarget, &v, &pl->evTarget);
        }
        if (MotionMove(pl, 0)) {
            pl->atari.setFlag200();
            EndPlDamage();
            pl->dmg.set(0, 10);
        } else if (pl->x3E0 != 0) {
            pl->x3E0--;
            if (Key.trg & 0x400) {
                flag = pl->xFF;
                SetPlDamage((int) door, plemDoorKick);
                pl->xFF = flag;
                pl->xFE = 4;
                door->xFC = 1;
                door->xFD = 0;
                door->xFE = 0;
                door->xFF = 0;
                if (pl->xFF && w->pDoor) {
                    w->pDoor->xFC = 1;
                    w->pDoor->xFD = 0;
                    w->pDoor->xFE = 0;
                    w->pDoor->xFF = 0;
                }
            }
        }
        break;
    }
    pl->x378 = pl->x37C;
}

// Corner `v` of an object (in the closed door's space) is inside the door's swing box.
static inline int emDoorObjBoxCk(Vec* v, f32 width)
{
    if (v->x < width && v->x > -width && v->z < 600.0f && v->z > -600.0f && v->y < 1000.0f && v->y > -1000.0f) {
        return 1;
    }
    return 0;
}

int cEmDoor::ckObj()
{
    EmDoorWork* w = EMDOOR_WK(this);
    f32 width = w->width;
    Vec v;
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        f32 x;
        f32 z;

        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id != 0x45) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        {
            f32 dx = pos.x - e->pos.x;
            f32 dz = pos.z - e->pos.z;

            if (dx * dx + dz * dz > 16000000.0f) {
                continue;
            }
        }
        x = EMRACK_WK(e)->size.x;
        z = EMRACK_WK(e)->size.z;
        v.x = x;
        v.y = 0.0f;
        v.z = z;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
        v.x = x;
        v.y = 0.0f;
        v.z = -z;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
        v.x = -x;
        v.y = 0.0f;
        v.z = z;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
        v.x = -x;
        v.y = 0.0f;
        v.z = -z;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = z;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = -z;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
        v.x = x;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
        v.x = -x;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(e->mat, &v, &v);
        PSMTXMultVec(w->inv, &v, &v);
        if (emDoorObjBoxCk(&v, width)) {
            return 0;
        }
    }
    return 1;
}

void cEmDoor::setOpenLock(int a)
{
    EmDoorWork* w = EMDOOR_WK(this);

    flags_3C8 |= 0x20000000;
    w->flags |= 1;
    xFC = 1;
    xFD = 6;
    xFE = 0;
    xFF = a;
}

void cEmDoor::setCloseLock(int a)
{
    EmDoorWork* w = EMDOOR_WK(this);

    flags_3C8 &= ~0x30000000;
    w->flags |= 1;
    xFC = 1;
    xFD = 7;
    xFE = 0;
    xFF = 0;
}

void cEmDoor::setClose()
{
    EmDoorWork* w = EMDOOR_WK(this);

    rot.y = w->rotY;
    xFC = 1;
    xFD = 0;
    xFE = 0;
    xFF = 0;
    emDoorMatUpdate(this);
}

void cEmDoor::setDowned(int dir)
{
    EmDoorWork* w = EMDOOR_WK(this);
    u16* flg;

    w->dir = dir;
    flg = GetEtcFlgPtr(w->flagNo, pG->room_id);
    if (flg) {
        *flg |= 1;
        if (w->dir == 0) {
            *flg |= 0x40;
        } else {
            *flg |= 0x80;
        }
    }
    xFC = 1;
    xFD = 9;
    xFE = 0;
    xFF = 0;
}

void cEmDoor::setNormal()
{
    EMDOOR_WK(this)->flags &= ~1;
}

void cEmDoor::setKey(int no)
{
    EMDOOR_WK(this)->keyNo = no;
}

cEmDoor* DoorOpenCk(cModel* m)
{
    Vec v;
    f32 ang;
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEmDoor* em = (cEmDoor*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        EmDoorWork* w;

        if ((em->be_flag & 0x201) != 1) {
            continue;
        }
        if (em->id != 0x41) {
            continue;
        }
        if (em->hp <= 0) {
            continue;
        }
        {
            f32 dx = m->pos.x - em->pos.x;
            f32 dy = m->pos.y - em->pos.y;
            f32 dz = m->pos.z - em->pos.z;

            if (dx * dx + dy * dy + dz * dz > 4000000.0f) {
                continue;
            }
        }
        w = EMDOOR_WK(em);
        ang = fabsf(Muku2(w->rotY, m->rot.y, PI));
        if (ang > PI / 4) {
            if (ang < PI * 3 / 4) {
                continue;
            }
        }
        PSMTXMultVec(w->inv, &m->pos, &v);
        if (ang < PI / 2) {
            if (v.z > 0.0f) {
                continue;
            }
            if (v.z < -800.0f) {
                continue;
            }
        } else {
            if (v.z < 0.0f) {
                continue;
            }
            if (v.z > 800.0f) {
                continue;
            }
        }
        if (v.x > w->width) {
            continue;
        }
        if (v.x < -w->width) {
            continue;
        }
        if (v.y > 500.0f) {
            continue;
        }
        if (v.y < -500.0f) {
            continue;
        }
        switch (em->ckOpen()) {
        case 0:
            return em;
        case 1:
            break;
        case 2:
            return em;
        case 3:
            return em;
        default:
            return em;
        }
    }
    return 0;
}

void SubOpenDoorSet(cEmDoor* door)
{
    SetSubDamage((int) door, (void*) subDoorKick);
}

void subDoorKick()
{
    cSubChar* sub = pSUB;
    cEmDoor* door = (cEmDoor*) sub->dmgType;

    if (sub->xFE == 0) {
        if (door->ckKick(&sub->pos)) {
            sub->xFE = 2;
        }
    }
    switch (sub->xFE) {
    case 0:
        MotionSetCore(sub, &sub->pMotion, PL_ARC_PTR((PlArc*) sub->x378, 0x2B), 0, 5, 1, 0);
        sub->subHideMode = 0xE;
        sub->xFE++;
    case 1:
        if (sub->subHideMode != 0) {
            sub->subHideMode--;
            if (sub->subHideMode == 0) {
                door->setShock(1, &sub->pos, 0);
                emDoorBellSet(&sub->pos);
            }
        }
        if (MotionMove(sub, 0)) {
            EndSubDamage();
            sub->dmg.set(0, 0x1E);
        }
        break;
    case 2:
        MotionSetCore(sub, &sub->pMotion, PL_ARC_PTR((PlArc*) sub->x378, 0x2A), 0, 5, 1, 0);
        sub->subHideMode = 0xE;
        sub->xFE++;
    case 3:
        if (sub->subHideMode != 0) {
            sub->subHideMode--;
            if (sub->subHideMode == 0) {
                door->setOpen(&sub->pos, 1, 0, 0);
                emDoorBellSet(&sub->pos);
            }
        }
        if (MotionMove(sub, 0)) {
            EndSubDamage();
        }
        break;
    }
}

void emDoorDropWeapon(cEmDoor* em)
{
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEmWep* e = (cEmWep*) ((u8*) EmMgr.pArray + EmMgr.size * i);

        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id != 0x42) {
            continue;
        }
        {
            f32 dx = e->pos.x - em->pos.x;
            f32 dy = e->pos.y - em->pos.y;
            f32 dz = e->pos.z - em->pos.z;

            if (dx * dx + dy * dy + dz * dz > 25000000.0f) {
                continue;
            }
        }
        e->setWaitDrop();
    }
}

void cEmDoor::setDoor(cEmDoor* other)
{
    EmDoorWork* w = EMDOOR_WK(this);

    if (other) {
        w->pDoor = other;
        EMDOOR_WK(other)->pDoor = this;
    }
}

void cEmDoor::setSeCancel()
{
    EMDOOR_WK(this)->seCancel = 1;
}
