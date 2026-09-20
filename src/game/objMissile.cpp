// game/objMissile: object id 0x38, the helicopter missile (D:/Bio4/Prog/objMissile.cpp): mounted on
// a parts of the helicopter (R0 1 Parent), ignites (R0 2 FireWait), flies towards its target with
// increasing speed (R0 3 Fire) and explodes on the scenery or an enemy (objMissileBomb), clearing
// enemies with a large player-weapon hit sphere. Type 1 is the shootable variant with a hit box.
#include "atari.h"
#include "atari_init.h"
#include "light.h"
#include "dmg.h"
#include "map_obj.h"
#include "widget.h"
#include "obj.h"
#include "esp.h"
#include "emhit.h"
#include "global.h"
#include "math_sub.h"
#include "snd.h"
#include "pl_wep.h"
#include "motion.h"

// Helicopter missile: follows a parts of the helicopter (R0_Parent), waits (R0_FireWait), flies
// toward its target and explodes on the scenario / an enemy (R0_Fire, objMissileBomb).
class cObjMissile : public cObj {
public:
    virtual void move();
    virtual ~cObjMissile() {}

    void setParent(cModel* parent, int partsNo, int noNormalize);
    void setFire(Vec* target);
};

extern "C" {
void objMissile_R0_Set(cObjMissile* obj);
void objMissile_R0_Parent(cObjMissile* obj);
void objMissile_R0_FireWait(cObjMissile* obj);
void objMissile_R0_Fire(cObjMissile* obj);
void objMissile_R0_Lost(cObjMissile* obj);
void objMissileBomb(cObjMissile* obj, Vec* pos);
}

void (*ObjMissile_R0_move_tbl[5])(cObjMissile*) = {
    objMissile_R0_Set, objMissile_R0_Parent, objMissile_R0_FireWait, objMissile_R0_Fire, objMissile_R0_Lost,
};

// Creates a missile (id 0x38) of `type` (0 helicopter rocket; 1 a shootable missile with a cEmHit
// hit box that detonates it when shot) at pos/rot.
cObj* SetHeliMissile(void* bin, void* tpl, Vec* pos, Vec* rot, u8 type)
{
    cObj* obj;
    MissileWork* w;

    obj = ObjMgr.create(0x38);
    if (obj == 0) {
        return 0;
    }
    w = &obj->missile;
    if (obj->modelInit(bin, tpl) == 0) {
        pLog->err(0, 0, "SetLadder() failed.");
        ObjMgr.destroy(obj);
        return 0;
    }
    static const Vec p0 = { 0.0f, 0.0f, 0.0f };
    static const Vec p1 = { 5000.0f, 5000.0f, 5000.0f };

    obj->LightInfo.init2(0, 1, &p0, &p1, 2);
    AtariInit(&obj->sub2B4.atari, 0.0f, 1000.0f, -700.0f, 350.0f, 700.0f, 700.0f, 1000.0f, 0, 2, 0);
    obj->sub2B4.atari.throughOn();
    if (pos) {
        obj->pos = *pos;
    } else {
        obj->pos.x = 0.0f;
        obj->pos.y = 0.0f;
        obj->pos.z = 0.0f;
    }
    obj->pos_old = obj->pos;
    if (rot) {
        obj->ang = *rot;
    } else {
        obj->ang.x = 0.0f;
        obj->ang.y = 0.0f;
        obj->ang.z = 0.0f;
    }
    obj->type = type;
    w->pHit = 0;
    if (obj->type == 1) {
        w->pHit = SetEmHit((void*) (pG->pCore->ofs_20 + (u32) pG->pCore), (void*) (pG->pCore->ofs_24 + (u32) pG->pCore), &obj->pos, &obj->ang, 1);
        if (w->pHit) {
            w->pHit->hp = 0;
            YarareInitCube(w->pHit, 0.0f, -300.0f, -300.0f, 300.0f, 600.0f, 600.0f, 1, 5);
            w->pHit->setParent(obj, 0, 0);
        }
    }
    obj->r_no_0 = 0;
    obj->r_no_1 = 0;
    obj->r_no_2 = 0;
    obj->r_no_3 = 0;
    return obj;
}

// Per-frame: destroyed (with its hit box) when the launcher dies; runs the R0 routine.
void cObjMissile::move()
{
    MissileWork* w = &missile;

    if (w->parent) {
        if ((w->parent->be_flag & 0x201) != 1 || ((cEm*) w->parent)->hp <= 0) {
            if (w->pHit) {
                EmMgr.destroy(w->pHit);
                w->pHit = 0;
            }
            ObjMgr.destroy(this);
            return;
        }
    }
    ObjMissile_R0_move_tbl[r_no_0](this);
}

// Rno0 == 0: free: matrices only.
void objMissile_R0_Set(cObjMissile* obj)
{
    obj->matUpdate();
}

// Rno0 == 1: mounted on parts partsNo of the launcher (axes normalised unless noNormalize).
void objMissile_R0_Parent(cObjMissile* obj)
{
    MissileWork* w = &obj->missile;
    Mtx m;
    Vec v0;
    Vec v1;
    Vec v2;
    cModel* parent = w->parent;

    RotMatrix(obj->mat, &obj->ang);
    TransMatrix(obj->mat, &obj->pos);
    ScaleMatrix(obj->mat, &obj->scale);
    if (parent && parent->pParts) {
        PSMTXConcat(parent->getPartsPtr(w->partsNo)->mat, obj->mat, m);
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
#line 249 "D:/Bio4/Prog/objMissile.cpp"
            VECNormalize(&v0, &v0);
            if (v1.x == 0.0f && v1.y == 0.0f && v1.z == 0.0f) {
                v1.y = 1.0f;
            }
#line 251 "D:/Bio4/Prog/objMissile.cpp"
            VECNormalize(&v1, &v1);
            if (v2.x == 0.0f && v2.y == 0.0f && v2.z == 0.0f) {
                v2.z = 1.0f;
            }
#line 253 "D:/Bio4/Prog/objMissile.cpp"
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
        PSMTXCopy(m, obj->mat);
    }
    if (obj->pMotion) {
        obj->motFlags2 |= 0x40000000;
        MotionMove(obj, 0);
    } else {
        obj->partsMatCalc();
    }
    obj->partsWorldCalc();
}

// Rno0 == 2 (setFire): 15 frames on the mount with the ignition effect (type 0: est 0x32/4),
// then Fire.
void objMissile_R0_FireWait(cObjMissile* obj)
{
    MissileWork* w = &obj->missile;
    Mtx m;
    Vec v0;
    Vec v1;
    Vec v2;
    cModel* parent = w->parent;

    switch (obj->r_no_2) {
    case 0:
        w->Timer = 15;
        switch (obj->type) {
        case 0:
        default:
            EstSet(obj, -1, 0, 0, 0x32, 4, 0, 0, obj, 0);
            break;
        case 1:
            break;
        }
        obj->r_no_2++;
    case 1:
        if (w->Timer) {
            w->Timer--;
        } else {
            obj->r_no_0 = 3;
            obj->r_no_1 = 0;
            obj->r_no_2 = 0;
            obj->r_no_3 = 0;
        }
        break;
    }
    RotMatrix(obj->mat, &obj->ang);
    TransMatrix(obj->mat, &obj->pos);
    ScaleMatrix(obj->mat, &obj->scale);
    if (parent && parent->pParts) {
        PSMTXConcat(parent->getPartsPtr(w->partsNo)->mat, obj->mat, m);
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
#line 341 "D:/Bio4/Prog/objMissile.cpp"
            VECNormalize(&v0, &v0);
            if (v1.x == 0.0f && v1.y == 0.0f && v1.z == 0.0f) {
                v1.y = 1.0f;
            }
#line 343 "D:/Bio4/Prog/objMissile.cpp"
            VECNormalize(&v1, &v1);
            if (v2.x == 0.0f && v2.y == 0.0f && v2.z == 0.0f) {
                v2.z = 1.0f;
            }
#line 345 "D:/Bio4/Prog/objMissile.cpp"
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
        PSMTXCopy(m, obj->mat);
    }
    if (obj->pMotion) {
        obj->motFlags2 |= 0x40000000;
        MotionMove(obj, 0);
    } else {
        obj->partsMatCalc();
    }
    obj->partsWorldCalc();
}

// Rno0 == 3: launch from the mount aimed at Target (type 0: 300 units/frame with the smoke trail
// and sound, type 1: 150), speed x1.1 per frame, 90-frame limit; explodes (objMissileBomb) on the
// scenario 300 units back along the path, on a character (type 1), or when its hit box is shot.
void objMissile_R0_Fire(cObjMissile* obj)
{
    MissileWork* w = &obj->missile;

    if (obj->r_no_2 == 0) {
        Vec d;

        obj->pos.x = obj->mat[0][3];
        obj->pos.y = obj->mat[1][3];
        obj->pos.z = obj->mat[2][3];
        obj->pos_old = obj->pos;
        Matrix2AxisAngle(obj->mat, &obj->ang);
        if (w->pHit) {
            w->pHit->hp = 1;
        }
        if (w->Target_ok) {
            f32 len;

            PSVECSubtract(&w->Target, &obj->pos, &d);
            len = SQRTF(d.x * d.x + d.z * d.z);
            obj->ang.x = -atan2f(d.y, len);
            obj->ang.y = atan2f(d.x, d.z);
            obj->ang.z = 0.0f;
            RotMatrix(obj->mat, &obj->ang);
            TransMatrix(obj->mat, &obj->pos);
        }
        w->Timer = 90;
        w->hitWait = 3;
        switch (obj->type) {
        case 0:
        default:
            EstSet(obj, -1, 0, 0, 0x32, 5, 0, 0, obj, 0);
            SndCall(6, 2, &obj->pos, 0, 0, obj);
            w->Spd.x = 0.0f;
            w->Spd.y = 0.0f;
            w->Spd.z = 300.0f;
            break;
        case 1:
            w->Spd.x = 0.0f;
            w->Spd.y = 0.0f;
            w->Spd.z = 150.0f;
            break;
        }
        PSMTXMultVecSR(obj->mat, &w->Spd, &w->Spd);
        w->parent = 0;
        obj->r_no_2++;
    }
    Vec hit;
    Vec nrm;

    PSVECAdd(&obj->pos, &w->Spd, &obj->pos);
    PSVECScale(&w->Spd, &w->Spd, 1.1f);
    if (w->hitWait) {
        w->hitWait--;
    } else {
        if (EatMgr.hitCheck(&obj->pos_old, &obj->pos, &hit, 0, 0, 0)) {
            PSVECSubtract(&obj->pos_old, &obj->pos, &nrm);
            if (nrm.x == 0.0f && nrm.y == 0.0f && nrm.z == 0.0f) {
                objMissileBomb(obj, &hit);
                return;
            }
#line 450 "D:/Bio4/Prog/objMissile.cpp"
            VECNormalize(&nrm, &nrm);
            PSVECScale(&nrm, &nrm, 300.0f);
            PSVECAdd(&hit, &nrm, &hit);
            objMissileBomb(obj, &hit);
            return;
        }
    }
    if (obj->type == 1) {
        Vec nrm2;

        if (EmAtkLineHitCk(&obj->pos_old, &obj->pos, &hit, &nrm2, 0)) {
            objMissileBomb(obj, &hit);
            return;
        }
        if (w->pHit) {
            if (w->pHit->ckDmgWeapon()) {
                objMissileBomb(obj, &obj->pos);
                return;
            }
        }
    }
    RotMatrix(obj->mat, &obj->ang);
    TransMatrix(obj->mat, &obj->pos);
    ScaleMatrix(obj->mat, &obj->scale);
    if (obj->pMotion) {
        obj->motFlags2 |= 0x40000000;
        MotionMove(obj, 0);
    } else {
        obj->partsMatCalc();
    }
    obj->partsWorldCalc();
    if (w->Timer) {
        w->Timer--;
    } else {
        obj->r_no_0 = 4;
        obj->r_no_1 = 0;
        obj->r_no_2 = 0;
        obj->r_no_3 = 0;
    }
}

// Rno0 == 4: removes the missile and its hit box.
void objMissile_R0_Lost(cObjMissile* obj)
{
    MissileWork* w = &obj->missile;

    obj->be_flag &= ~2;
    if (w->pHit) {
        EmMgr.destroy(w->pHit);
        w->pHit = 0;
    }
    ObjMgr.destroy(obj);
}

// Mounts the missile on parts partsNo of `parent` -> Parent.
void cObjMissile::setParent(cModel* parent, int partsNo, int noNormalize)
{
    MissileWork* w = &missile;

    w->parent = parent;
    w->partsNo = partsNo;
    w->noNormalize = noNormalize;
    r_no_0 = 1;
    r_no_1 = 0;
    r_no_2 = 0;
    r_no_3 = 0;
}

// Fires the missile at `target` (NULL = straight ahead) -> FireWait.
void cObjMissile::setFire(Vec* target)
{
    MissileWork* w = &missile;

    w->Target_ok = 0;
    if (target) {
        w->Target = *target;
        w->Target_ok = 1;
    }
    r_no_0 = 2;
    r_no_1 = 0;
    r_no_2 = 0;
    r_no_3 = 0;
}

// Explosion at pos: type 0 est 0x32/7, sound and a player weapon hit sphere (kind 0x12, radius
// 8000) that destroys enemies; type 1 est 2/5 with radius 2000 (kind 0x13). Kills the hit box, in
// r320 raises Room_flg[0] 0x80000000, -> Lost.
void objMissileBomb(cObjMissile* obj, Vec* pos)
{
    MissileWork* w = &obj->missile;

    switch (obj->type) {
    case 0:
    default:
        EstSet(0, -1, pos, 0, 0x32, 7, 0, 0, 0, 0);
        SndCall(6, 3, &obj->pos, 0, 0, obj);
        PlWepHitCheck2(0, &obj->pos_old, &obj->pos_old, 0x12, 3, 8000.0f);
        break;
    case 1:
        EstSet(0, -1, pos, 0, 2, 5, 0, 0, 0, 0);
        PlWepHitCheck2(0, &obj->pos_old, &obj->pos_old, 0x13, 3, 2000.0f);
        break;
    }
    if (w->pHit) {
        EmMgr.destroy(w->pHit);
        w->pHit = 0;
    }
    if (G_ROOM_ID == 0x320) {
        pG->Room_flg[0] |= 0x80000000;
    }
    obj->r_no_0 = 4;
    obj->r_no_1 = 0;
    obj->r_no_2 = 0;
    obj->r_no_3 = 0;
}
