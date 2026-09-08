// game/pl_wep.cpp: player weapon control: weapon object release/load, hit checks, lock-on
// target search, aim control (PlWepLockCtrl), auto tracking, water shots.

#include "atari.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "player.h"
#include "pl_sub.h"
#include "global.h"
#include "db_log.h"
#include "main.h"
#include "joy.h"
#include "item.h"
#include "snd.h"
#include "esp.h"
#include "obj.h"
#include "cam_ctrl.h"
#include "rnd.h"
#include "math_sub.h"

// GetWepTargetList entry (em_sub.cpp).
struct WepTarget {
    cEm* em;
    EmHitInfo* part;
};

extern "C" {
void EspDataRelease(int a, int b, int c);                 // game/eff_sys.cpp
void EffectEspDelete(int a, int b, cModel* m, int c);     // game/est.cpp
void EffectEspgenDelete(int a, int b, cModel* m);
void EffectEfmDelete(int a, int b, cModel* m);
void ReadWepData(int no, int type);                       // game/read.cpp
u32 GetWepTargetListBomb(Vec* pos, WepTarget* list, u32 prio, int type, int flag, f32 len);  // game/em_sub.cpp
u32 GetWepTargetList2(Vec* p0, Vec* p1, WepTarget* list, u32 prio, Vec* hit, Vec* nrm, u32* attr, int type,
                      int flag, f32 len);
void EspSetEatEffect(Vec* pos, Vec* nrm, int type, u8 wep);  // game/est.cpp
void EspSetWaterHitmark(Vec* pos);
void GameAddPoint(int no);                                // game/game.cpp
f32 GetXZAngleLocal(Vec* a, Vec* b, f32 ang);             // game/sub2.cpp
int GetWaterCrossPos(Vec* pos, Vec* dir, Vec* out);       // game/Espgen42.cpp
void AddWaterPower(Vec* pos, f32 power);
f64 atan2(f64 y, f64 x);
f32 rangeDist(Vec* pos, cEm* em, f32 range);
int lockEmCk(cEm* em, Vec* pos);
cModel* searchLockEm(Vec* pos, cModel* skip, f32 range);
int cnCkSub(Vec* pos, Vec* nrm, Vec* outA, Vec* outB, f32 len);
void wepSetWaterShot(Vec* p0, Vec* p1, u8 type);
void setWaterShot(Vec* pos);
}
int Front_check(cModel* a, cModel* b, f32 ang);           // game/sub2.cpp

void (*WeaponInitFunc)(cModel*) = 0;
u8 lockCtr;
static f32 lockRandCtr;

// Stores through references: scalar MEMs, so pG is reloaded after each of them.
static inline void U8Set(u8& d, u8 v) { d = v; }
static inline void Inc32(u32& d) { d++; }

cPlWep::cPlWep()
{
    pObj = 0;
    pObj2 = 0;
    x20 = 0;
}

void cPlayer::weaponRelease()
{
    cObj* obj;
    cObj* objCur;

    obj = ObjMgr.pAlive;
    while (obj) {
        objCur = obj;
        obj = (cObj*) obj->next;
        if (objCur->id == 0xA) {
            ObjMgr.destroy(objCur);
        }
    }
    if (pWep->pObj) {
        ObjMgr.destroyNow((cObj*) pWep->pObj);
        pWep->pObj = 0;
    }
    if (pWep->pObj2) {
        ObjMgr.destroyNow((cObj*) pWep->pObj2);
        pWep->pObj2 = 0;
    }
    endCamera();
    if ((flags_420 & 1) == 0) {
        switch (pG->x4F7C) {
        case 0:
            break;
        case 1:
            EspDataRelease(0x35, 1, 1);
            break;
        case 2:
        case 3:
        case 0x12:
            EspDataRelease(0x36, 1, 1);
            break;
        case 4:
            EspDataRelease(0x38, 1, 1);
            break;
        case 5:
            EspDataRelease(0x39, 1, 1);
            break;
        case 6:
            EspDataRelease(0x3A, 1, 1);
            break;
        case 7:
        case 0x21:
            EspDataRelease(0x3B, 1, 1);
            break;
        case 8:
            EspDataRelease(0x3C, 1, 1);
            break;
        case 9:
            EspDataRelease(0x3D, 1, 1);
            break;
        case 0xA:
            EspDataRelease(0x44, 1, 1);
            break;
        case 0xB:
        case 0x14:
            EspDataRelease(0x45, 1, 1);
            break;
        case 0xC:
            EspDataRelease(0x46, 1, 1);
            break;
        case 0xD:
            EspDataRelease(0x47, 1, 1);
            break;
        case 0xE:
            EspDataRelease(0x48, 1, 1);
            break;
        case 0xF:
            EspDataRelease(0x49, 1, 1);
            break;
        case 0x10:
            EspDataRelease(0x4A, 1, 1);
            break;
        case 0x11:
            EspDataRelease(0x4B, 1, 1);
            break;
        case 0x13:
        case 0x16:
        case 0x17:
        case 0x19:
        case 0x1F:
        case 0x20:
            EspDataRelease(0x4D, 1, 1);
            break;
        case 0x1C:
            EspDataRelease(0x50, 1, 1);
            break;
        }
    }
    flags_420 &= ~1;
    EffectEspDelete(0, 10, this, 0);
    EffectEspgenDelete(0, 10, this);
    EffectEfmDelete(0, 10, this);
}

void cPlayer::weaponLoad(int no, int type)
{
    U8Set(pG->wep_no, no);
    U8Set(pG->wep_type, type);
    ReadWepData(no, type);
}

void cPlayer::weaponInit()
{
    int i;

    for (i = 0; i < 0x5F; i++) {
        pMotTbl[i] = 0;
    }
    if (WeaponInitFunc) {
        WeaponInitFunc(this);
    }
    if (!(pG->flags_5010 & 0x200000) && !(flags_420 & 0x40)) {
        xFC = 0;
        xFD = 0;
        xFE = 0;
        xFF = 1;
        x4FD = 0;
        x4FC = 0;
    }
}

// Dead-stripped by the original linker (STRIP_UNUSED): its pool (-1, 0, 0.25) opens the unit's
// constants, right before PlWepHitCheck2's.
static f32 wepRate(cPlWep* w)
{
    if (w->x20) {
        return -1.0f;
    }
    if (w->pitch > 0.0f) {
        return 0.25f;
    }
    return w->pitch;
}

u32 PlWepHitCheck2(cModel* plm, Vec* p0, Vec* p1, int type, u32 flag, f32 len)
{
    cPlayer* pl = (cPlayer*) plm;
    WepTarget list[20];
    Vec hit;
    Vec nrm;
    u32 attr;
    f32 wh;
    u32 prio;
    int f4;
    u32 n;
    u32 i;

    switch (type) {
    case 1:
        if (pG->wep_lv > 6) {
            prio = 5;
        } else {
            prio = 2;
        }
        break;
    case 9:
    case 0xA:
        prio = 5;
        break;
    case 5:
    case 6:
        prio = 3;
        break;
    case 4:
    case 8:
    case 0xC:
        prio = 1;
        break;
    case 0xF:
        prio = 5;
        break;
    case 0x10:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x16:
    case 0x17:
    case 0x2D:
        prio = 0x14;
        break;
    default:
        prio = 1;
        break;
    }
    if (prio > 0x14) {
        prio = 0x14;
    }
    f4 = 0;
    if (flag & 4) {
        f4 = 1;
    }
    nrm.x = 0.0f;
    nrm.y = 0.0f;
    nrm.z = 0.0f;
    switch (type) {
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x16:
    case 0x17:
    case 0x29:
    case 0x2D:
        n = GetWepTargetListBomb(p0, list, prio, type, f4, len);
        break;
    default:
        n = GetWepTargetList2(p0, p1, list, prio, &hit, &nrm, &attr, type, f4, len);
        break;
    }
    for (i = 0; i < n; i++) {
        cEm* em = list[i].em;
        EmHitInfo* part = list[i].part;
        cDmgInfo* dmg = &em->dmg;

        switch (type) {
        case 0x14:
            if (em->id == 3) {
                continue;
            }
            break;
        case 9:
        case 0xA:
            if ((em->id == 3 || em->id == 4) && i != 0) {
                continue;
            }
            break;
        }
        if (!(dmg->stat & 1)) {
            dmg->set(0, 10, type, p0, part->rad, part);
            if (part->flags & 0x20) {
                dmg->stat |= 0x20;
            }
        }
        if (list[i].em->id == 0x38) {
            break;
        }
    }
    if (!(flag & 1)) {
        if (nrm.x != 0.0f || nrm.y != 0.0f || nrm.z != 0.0f) {
            if (GetWaterHeight(&hit, &wh) == 0 || hit.y > wh) {
                int et = EatGetEffectType(attr);

                EspSetEatEffect(&hit, &nrm, et, type);
                BitOn(pG->flags_5010, 0x20000000);
                pG->bell_pos = hit;
                pG->bell_stat = 0;
            }
        }
    }
    if (pl != 0 && !(flag & 1)) {
        if (pl->pWep->pObj != 0) {
            wepSetWaterShot(p0, p1, type);
            pG->bell_pos = pl->pWep->pObj->wep.marker;
            if (type == 0xD || (type >= 0x12 && type <= 0x13)) {
                pG->bell_stat = 1;
            } else {
                pG->bell_stat = 0;
            }
        }
    }
    if (!(flag & 2) && n == 0) {
        pG->flags_5014 |= 0x01000000;
        switch (type) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 0x11:
            GameAddPoint(4);
            break;
        case 7:
        case 8:
        case 0x21:
            GameAddPoint(5);
            break;
        case 9:
        case 0xA:
            GameAddPoint(7);
            break;
        case 0xB:
        case 0xC: {
            u16 rest = ItemMgr.bulletNumCurrent() % 5;

            if (rest == 0) {
                GameAddPoint(8);
            }
            break;
        }
        case 0xF:
        case 0x13:
            GameAddPoint(6);
            break;
        }
    }
    if (pl != 0 && pl->id == 0) {
        switch (PlGetWeaponNo()) {
        case 0:
        case 0xD:
        case 0xE:
        case 0x10:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x16:
        case 0x17:
        case 0x19:
        case 0x1A:
        case 0x1F:
        case 0x20:
            break;
        case 7:
        case 8:
        case 0x21:
            if (flag & 4) {
                break;
            }
        default:
            if (!(flag & 2)) {
                if (n != 0) {
                    Inc32(pG->shotHit);
                    Inc32(pG->shotHit2);
                }
                Inc32(pG->shotTotal);
                Inc32(pG->shotTotal2);
            }
            break;
        }
    }
    return n;
}

u32 PlWepHitCheck3(Vec* pos, int type, u32 prio, f32 len)
{
    WepTarget list[20];
    u32 n;
    u32 i;

    if (prio > 0x14) {
        prio = 0x14;
    }
    n = GetWepTargetListBomb(pos, list, prio, type, 0, len);
    for (i = 0; i < n; i++) {
        cEm* em = list[i].em;
        cDmgInfo* dmg = &em->dmg;

        switch (type) {
        case 0x14:
            if (em->id == 3) {
                continue;
            }
            break;
        case 9:
        case 0xA:
            if ((em->id == 3 || em->id == 4) && i != 0) {
                continue;
            }
            break;
        }
        EmHitInfo* part = list[i].part;
        if (!(dmg->stat & 1)) {
            dmg->set(0, 10, type, pos, part->rad, part);
        }
    }
    return n;
}

f32 cPlWep::getAngle()
{
    cPlayer* pl = pPL;

    if (pPL->xFC != 0) {
        return 0.0f;
    }
    if (pl->xFD != 6 && pl->xFD != 0xB) {
        return 0.0f;
    }
    if (pl->xFE == 3) {
        return 0.0f;
    }
    return pitch;
}

f32 cPlWep::getPitch()
{
    cPlayer* pl = pPL;

    if (pPL->xFC != 0) {
        return 0.0f;
    }
    if (pl->xFD != 6 && pl->xFD != 0xB) {
        return 0.0f;
    }
    if (pl->xFE == 3) {
        return 0.0f;
    }
    return m3r[0];
}

void cPlWep::move()
{
    if (pObj) {
        pObj->matUpdate();
    }
}

int cPlWep::getMarkerPos(Vec* out)
{
    cPlayer* pl = pPL;

    if ((pl->stat & 0xFFFFFF00) != 0x00060100 || pl->xFF == 0) {
        return 0;
    }
    *out = pObj->wep.marker;
    return 1;
}

void cPlWep::setTrans(int on, int type)
{
    if (pObj == 0) {
        return;
    }
    switch (pG->wep_no) {
    default:
        pObj->setDisp(2, on);
        break;
    case 0xD:
        type = 1;
    case 0x13:
    case 0x16:
    case 0x17:
    case 0x19:
    case 0x1F:
    case 0x20:
        if (type & 1) {
            pObj->setDisp(2, on);
        }
        break;
    }
    if (pObj2) {
        pObj2->setDisp(2, on);
    }
}

cModel* cPlWep::lockInit()
{
    cPlayer* pl = pPL;
    cEm* em;

    em = (cEm*) SearchLockEm(&pl->getPartsPtr(3)->worldPos, 0);
    pl->pLockEm = em;
    if (em) {
        Vec v;
        f32 ang;

        PSMTXMultVec(em->getPartsPtr(em->lockParts)->mat, &((cEm*) pl->pLockEm)->lockOfs, &v);
        ang = GetXZAngleLocal(&pl->pos, &v, pl->rot.y);
        if (ang <= PI && ang >= -PI) {
            x40 = 10;
        } else {
            x40 = 0;
        }
    } else {
        x40 = 0;
    }
    return pl->pLockEm;
}

// Distance penalty by direction: inlined into rangeDist; its constants precede rangeDist's own.
static inline f32 rangeAdd(Vec* pos, Vec* v, f32 d)
{
    if (fabsf(GetXZAngleLocal(pos, v, pPL->rot.y)) < 0.87266463f) {
        if (d > 16000000.0f) {
            return d + 40000000000.0f;
        }
        return d;
    }
    if (d > 225000000.0f) {
        return d + 160000000000.0f;
    }
    if (d > 16000000.0f) {
        return d + 90000000000.0f;
    }
    return d + 10000000000.0f;
}

f32 rangeDist(Vec* pos, cEm* em, f32 range)
{
    Vec v;
    f32 d;

    PSMTXMultVec(em->getPartsPtr(em->lockParts)->mat, &em->lockOfs, &v);
    d = GetDistance(pos, &v);
    if (range == 0.0f) {
        range = 100000000.0f;
    }
    if (d > range) {
        return 1000000000000.0f;
    }
    if (d > 225000000.0f) {
        return d + 160000000000.0f;
    }
    return rangeAdd(pos, &v, d);
}

void cPlWep::lockMove()
{
    cPlayer* pl = pPL;

    if (Joy[0].on & 0xF0000) {
        x40 = 0;
    }
    if (pl->pLockEm && x40 != 0 && (pSys->flags & 0x20000000)) {
        PlWepAutoTrack(pl, 0, 1.0f);
    }
}

int lockEmCk(cEm* em, Vec* pos)
{
    Vec v;

    if ((em->be_flag & 0x201) != 1) {
        return 0;
    }
    if (!(em->be_flag & 0x20)) {
        return 0;
    }
    if (em->id <= 0xF) {
        return 0;
    }
    if (em->hp <= 0) {
        return 0;
    }
    if (em->checkStatus(1)) {
        return 0;
    }
    if (em->checkStatus(5) == 0) {
        return 0;
    }
    if (em->pParts == 0) {
        return 0;
    }
    if (em->id == 0x43 || em->id == 0x41) {
        if (Front_check(pPL, em, 1.0471976f) == 0) {
            return 0;
        }
    }
    PSMTXMultVec(em->getPartsPtr(em->lockParts)->mat, &em->lockOfs, &v);
    return EatMgr.hitCheck(pos, &v, 0, 0, 0x800, 0) == 0;
}

// Dead-stripped by the original linker (STRIP_UNUSED): pools -2pi, 0, 2pi and 0, pi between
// lockEmCk's and SearchLockEm's constants.
static f32 lockAngleWrap(f32 a)
{
    if (a < -2.0f * PI) {
        a = 0.0f;
    }
    if (a > 2.0f * PI) {
        a = 0.0f;
    }
    return a;
}

static int lockAngleFront(f32 a)
{
    if (a < 0.0f) {
        a = -a;
    }
    return a < PI;
}

cModel* cPlWep::lockNext()
{
    cPlayer* pl = pPL;

    if ((pl->pLockEm = SearchLockEm(&pl->getPartsPtr(3)->worldPos, pl->pLockEm)) != 0) {
        x40 = 10;
    }
    return pl->pLockEm;
}

cModel* SearchLockEm(Vec* pos, cModel* skip)
{
    if (pSys->flags & 0x20000000) {
        return searchLockEm(pos, skip, 0.0f);
    }
    return 0;
}

cModel* SearchTargetEm(Vec* pos, cModel* skip, f32 range)
{
    return searchLockEm(pos, skip, range);
}

cModel* searchLockEm(Vec* pos, cModel* skip, f32 range)
{
    cEm* best = 0;
    f32 bestD = 1000000000000.0f;
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* em = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        f32 d;

        if (em == skip) {
            continue;
        }
        if (lockEmCk(em, pos) == 0) {
            continue;
        }
        d = rangeDist(pos, em, range);
        if (d < bestD) {
            best = em;
            bestD = d;
        }
    }
    if (best == 0 && skip != 0) {
        if (lockEmCk((cEm*) skip, pos)) {
            best = (cEm*) skip;
        }
    }
    return best;
}

// Dead-stripped by the original linker (STRIP_UNUSED): only its `vecz` (.data vecz.1341, right
// before PlCornerCheck's) survives.
static int cornerCheckOld()
{
    static Vec vecz = {0.0f, 0.0f, 500.0f};
    Vec dir;
    Vec rot;

    dir.x = 0.0f;
    dir.y = pPL->rot.y;
    dir.z = 0.0f;
    RotVector(&vecz, &dir, &rot);
    PSVECAdd(&rot, &pPL->pos, &rot);
    return SatMgr.hitCheck(&pPL->pos, &rot, 0, 0, 0, 0);
}

int PlCornerCheck()
{
    static Vec vecz = {0.0f, 0.0f, 500.0f};
    cPlayer* pl = pPL;
    Vec* hand = &pl->getPartsPtr(3)->worldPos;
    Vec dir;
    Vec rot = {0.0f, 0.0f, 0.0f};
    Vec hit;
    Vec nrm;
    Vec a;
    Vec b;

    rot.y = pl->rot.y;
    dir = rot;
    if ((pl->stat & 0xFFFF0000) == 0x000D0000) {
        dir.y += PI;
        dir.y = LIMIT_ANGLE(dir.y);
    }
    RotVector(&vecz, &dir, &rot);
    PSVECAdd(&rot, hand, &rot);
    if (SatMgr.hitCheck(hand, &rot, &hit, &nrm, 0, 0)) {
        if (cnCkSub(hand, &nrm, &a, &b, 500.0f)) {
            return 1;
        }
        if (cnCkSub(hand, &nrm, &a, &b, -500.0f)) {
            return 2;
        }
        if (cnCkSub(hand, &nrm, &a, &b, 1000.0f)) {
            return 1;
        }
        if (cnCkSub(hand, &nrm, &a, &b, -1000.0f)) {
            return 2;
        }
    }
    return 0;
}

int cnCkSub(Vec* pos, Vec* nrm, Vec* outA, Vec* outB, f32 len)
{
    static Vec angR = {0.0f, PI / 2.0f, 0.0f};
    static Vec angB = {0.0f, PI, 0.0f};
    Vec v;
    Vec w;

    RotVector(nrm, &angR, &v);
    PSVECScale(&v, &v, len);
    PSVECAdd(&v, pos, &v);
    if (SatMgr.hitCheck(pos, &v, 0, 0, 0, 0)) {
        return 0;
    }
    RotVector(nrm, &angB, &w);
    PSVECScale(&w, &w, 1000.0f);
    PSVECAdd(&w, &v, &w);
    if (SatMgr.hitCheck(&v, &w, 0, 0, 0, 0)) {
        return 0;
    }
    *outA = v;
    *outB = w;
    return 1;
}

void PlWepLockCtrl(cModel* plm)
{
    static f32 repCtr = 0.0f;
    cPlayer* pl = (cPlayer*) plm;
    f32 lim;
    f32 spd;
    f32 spd2;
    f32 d;
    int moved;
    f32 tmp;

    switch (pG->wep_no) {
    case 4:
        lim = 0.062831853f;
        spd = 1.5f;
        spd2 = 1.2f;
        break;
    case 7:
        spd = 0.8f;
        lim = 0.10471976f;
        spd2 = spd;
        break;
    case 8:
        spd = 0.9f;
        lim = 0.052359879f;
        spd2 = spd;
        break;
    case 0x21:
        spd = 1.04f;
        lim = 0.10471976f;
        spd2 = spd;
        break;
    case 0xB:
    case 0xC:
    case 0x1B:
    case 0x1D:
    case 0x27:
        spd = 1.15f;
        lim = 0.0065449847f;
        spd2 = spd;
        break;
    case 0xE:
    case 0x13:
        if (pG->wep_type == 0) {
            spd = 1.0f;
            lim = 0.20943952f;
            spd2 = spd;
            break;
        }
    case 0x16:
    case 0x17:
    case 0x29:
        lim = 0.0065449847f;
        spd = 1.2f;
        spd2 = 1.5f;
        break;
    case 0x10:
        spd = 1.2f;
        lim = 0.10471976f;
        spd2 = spd;
        break;
    case 0xD:
        spd = 1.0f;
        lim = 0.0065449847f;
        spd2 = spd;
        break;
    default:
        spd = 1.0f;
        lim = 0.20943952f;
        spd2 = spd;
        break;
    }
    if (Joy[0].on & 0xFFFF0000) {
        if (repCtr < 7.0f) {
            repCtr = repCtr + 1.0f;
        }
    } else {
        repCtr = 0.0f;
    }
    moved = 0;
    if (joyKamae() || joyLKamae()) {
        if (pl->pLockEm && lockCtr != 0 && (pG->flags_68 & 0x40000)) {
            goto rand;
        }
        d = 0.0f;
        if ((s32) pSys->flags < 0) {
            if (Joy[0].on & 8) {
                d -= 0.035f;
            }
            if (Joy[0].on & 4) {
                d += 0.035f;
            }
            d -= spd2 * (f32) Joy[0].sy * repCtr * 0.15f / 200.0f / 10.0f;
        } else {
            if (Joy[0].on & 8) {
                d = 0.035f;
            }
            if (Joy[0].on & 4) {
                d -= 0.035f;
            }
            d += spd2 * (f32) Joy[0].sy * repCtr * 0.15f / 200.0f / 10.0f;
        }
        if (m3r[0] > 0.0f) {
            d *= 0.8f;
        }
        if (d != 0.0f) {
            moved = 1;
        }
        m3r[1] += d;
        if (m3r[2] == 0.0f) {
            m3r[0] = m3r[1];
        }
        if (m3r[1] < -1.0f) {
            m3r[1] = -1.0f;
        } else if (m3r[1] > 1.0f) {
            m3r[1] = 1.0f;
        }
        if (m3r[2] == 0.0f) {
            m3r[0] = m3r[1];
        }
        d = 0.0f;
        if (Joy[0].on & 2) {
            d -= 0.05f;
        }
        if (Joy[0].on & 1) {
            d += 0.05f;
        }
        d -= (f32) Joy[0].sx * repCtr * PI / 10.0f / 200.0f / 20.0f;
        if (d != 0.0f) {
            moved = 1;
        }
        pl->x400 += d;
        if (pl->x400 > lim) {
            pl->x400 = lim;
            if (Joy[0].on & 1) {
                pl->rot.y += 0.039269908f;
            } else {
                pl->rot.y -= spd * (f32) Joy[0].sx * repCtr * PI / 10.0f / 200.0f / 20.0f;
            }
        }
        if (pl->x400 < -lim * 0.8f) {
            pl->x400 = -lim * 0.8f;
            if (Joy[0].on & 2) {
                pl->rot.y -= 0.039269908f;
            } else {
                pl->rot.y -= spd * (f32) Joy[0].sx * repCtr * PI / 10.0f / 200.0f / 20.0f;
            }
        }
    }
rand:
    tmp = m3r[0];
    PlWepLockRand(pl, moved, &tmp, &pl->x400);
    m3r[1] = tmp;
    if (m3r[2] == 0.0f) {
        m3r[0] = tmp;
    }
    if ((pG->flags_68 & 0x40000) && lockCtr != 0) {
        PlWepAutoTrack(pl, 1, 0.03f);
    }
    m3r[0] = m3r[0] * m3r[2] + m3r[1] * (1.0f - m3r[2]);
    mot3.move(m3r[0]);
    pl->pWaist->set(pl->x400, 0.4f);
}

void PlWepLockRandInit()
{
    lockRandCtr = 0.2f;
}

void PlWepLockRand(cModel* plm, int flag, f32* pitch, f32* yaw)
{
    cPlayer* pl = (cPlayer*) plm;
    cPlWep* wep = pl->pWep;
    f32 rP;
    f32 rY;
    f32 sP;
    f32 sY;

    *pitch *= PI / 2.0f;
    rP = wep->pObj->wep.lockRandPitch;
    rY = wep->pObj->wep.lockRandYaw;
    sP = wep->pObj->wep.lockRandPitchStep;
    sY = wep->pObj->wep.lockRandYawStep;
    if (flag & 1) {
        wep->pitch = *pitch;
        wep->x2C = *yaw;
    } else if (flag & 2) {
        *pitch = fRand1_1() * rP * lockRandCtr + wep->pitch;
        *yaw = fRand1_1() * rY * lockRandCtr + wep->x2C;
    } else {
        *pitch = sP * fRand1_1() + *pitch;
        *yaw = sY * fRand1_1() + *yaw;
        if (*pitch > wep->pitch + rP) {
            *pitch = wep->pitch + rP;
        } else if (*pitch < wep->pitch - rP) {
            *pitch = wep->pitch - rP;
        }
        if (*yaw > wep->x2C + rP) {
            *yaw = wep->x2C + rP;
        } else if (*yaw < wep->x2C - rP) {
            *yaw = wep->x2C - rP;
        }
    }
    *pitch *= 2.0f / PI;
}

void PlWepAutoTrack(cModel* plm, int mode, f32 rate)
{
    cPlayer* pl = (cPlayer*) plm;
    Vec* hand;
    Vec tgt;
    f32 dist;
    f32 d;
    f32 e;

    if (pl->pLockEm == 0) {
        return;
    }
    hand = &pl->getPartsPtr(10)->worldPos;
    PSMTXMultVec(pl->pLockEm->getPartsPtr(((cEm*) pl->pLockEm)->lockParts & 7)->mat, &((cEm*) pl->pLockEm)->lockOfs,
                 &tgt);
    dist = GetDistance3(hand, &tgt);
    if (dist > 400.0f) {
        d = Muku(hand, &tgt, pl->rot.y + pl->x400, rate * PI);
        if (d > 0.52359879f) {
            d = 0.52359879f;
        }
        if (d < -0.52359879f) {
            d = -0.52359879f;
        }
        if (mode != 0) {
            f32 na = pl->x400 + d;

            if (na <= 0.20943952f && na >= -0.20943952f) {
                pl->x400 = na;
            } else if (pl->x400 + d > 0.20943952f) {
                pl->rot.y += 0.052359879f;
            } else {
                pl->rot.y -= 0.052359879f;
            }
        } else {
            pl->rot.y += d;
            pl->rot.y = LIMIT_ANGLE(pl->rot.y);
        }
    }
    e = atan2(tgt.y - hand->y, dist) / (PI / 4.0f) - m3r[0];
    if (e > 0.05f) {
        e = 0.05f;
    }
    if (e < -0.05f) {
        e = -0.05f;
    }
    m3r[1] += e;
    if (m3r[2] == 0.0f) {
        m3r[0] = m3r[1];
    }
    if (m3r[1] < -1.0f) {
        m3r[1] = -1.0f;
    } else if (m3r[1] > 1.0f) {
        m3r[1] = 1.0f;
    }
    if (m3r[2] == 0.0f) {
        m3r[0] = m3r[1];
    }
}

void wepSetWaterShot(Vec* p0, Vec* p1, u8 type)
{
    Vec d;
    Vec cross;
    Vec r;
    int i;

    PSVECSubtract(p1, p0, &d);
    if (GetWaterCrossPos(p0, &d, &cross)) {
        if (EatMgr.hitCheck(p0, &cross, 0, 0, 0x800, 0) == 0) {
            setWaterShot(&cross);
        }
    }
    switch (type) {
    case 5:
    case 6:
    case 0xF:
    case 0x2C:
        for (i = 3; i != 0; i--) {
            r.x = fRand1_1() * 2000.0f + p1->x;
            r.y = fRand1_1() * 2000.0f + p1->y;
            r.z = fRand1_1() * 2000.0f + p1->z;
            wepSetWaterShot(p0, &r, 2);
        }
        break;
    }
}

void setWaterShot(Vec* pos)
{
    EspSetWaterHitmark(pos);
    AddWaterPower(pos, 0.6f);
    SndCall(2, 0xB, pos, 0, 0, 0);
}

void PlSetLockPitch(cModel* plm)
{
    cPlayer* pl = (cPlayer*) plm;
    f32 p;

    if (pSys->flags & 0x20000000) {
        if (pl->pLockEm) {
            Vec d;

            PSVECSubtract(&pl->pLockEm->getPartsPtr(((cEm*) pl->pLockEm)->lockParts)->worldPos, &pl->pParts->worldPos,
                          &d);
            p = VecElevation(&d);
        } else {
            p = 0.0f;
        }
    } else {
        p = CamCtrl.getCameraPitch();
        if (p > 0.0f) {
            p += p;
        }
    }
    pl->pWep->pitch = p;
    m3r[1] = p * (2.0f / PI);
    m3r[2] = 0.0f;
    m3r[0] = m3r[1] * m3r[2] + m3r[1];
}

int GetWepSizeGroup(int no)
{
    switch (no) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 0xB:
    case 0xC:
    case 0x11:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1F:
    case 0x20:
    case 0x26:
    case 0x27:
    case 0x2A:
    case 0x2B:
        return 0;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 0xA:
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x12:
    case 0x13:
    case 0x15:
    case 0x16:
    case 0x1E:
    case 0x21:
    case 0x28:
    case 0x29:
    case 0x2C:
        return 1;
    case 0x10:
    case 0x1A:
        return 2;
    case 0:
    case 0x14:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    default:
        return 3;
    }
}
