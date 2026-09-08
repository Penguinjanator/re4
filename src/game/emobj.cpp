// game/emobj.cpp: generic object enemy base (cEmObj): matrix update, scenario / effect
// collision quad registration, yarare setup.

#include "atari.h"
#include "emobj.h"
#include "math_sub.h"

extern "C" {
int MotionMove(cModel* m, int a);
void YarareInit(cEm* em, s16 no, u16 flag, f32 x, f32 y, f32 z, f32 w, f32 h);            // at_mod.cpp
void YarareInitCube(cEm* em, s16 no, u16 flag, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rad);
}

void cEmObj::EmObjInit()
{
    EmObjWork* w = EMOBJ_WK(this);

    x3E0 = 0;
    w->pSat = 0;
    w->pEat = 0;
    w->eff = 0xFF;
    w->etc = 0xFF;
}

void cEmObj::EmObjMove()
{
    EmObjWork* w = EMOBJ_WK(this);

    RotMatrix(mat, &rot);
    TransMatrix(mat, &pos);
    ScaleMatrix(mat, &scale);
    if (x3E0 & 1) {
        MotionMove(this, 0);
    } else {
        partsMatCalc();
        motState = 0;
    }
    partsWorldCalc();
    if (w->flags & 2) {
        setSatMain();
    }
    if (w->flags & 4) {
        setEatMain();
    }
}

void cEmObj::setSat(Vec* pos, int n, int flag, f32 sx, f32 sy, f32 sz)
{
    EmObjWork* w = EMOBJ_WK(this);

    if (pos) {
        w->satPos = *pos;
    }
    w->satSize.x = sx;
    w->satSize.y = sy;
    w->satSize.z = sz;
    w->satN = n;
    w->satFlag = flag;
    x3E0 |= 2;
    setSatMain();
}

void cEmObj::setSatMain()
{
    EmObjWork* w = EMOBJ_WK(this);
    Vec poly[4];

    if (w->pSat) {
        w->pSat->flags &= ~4;
    }
    poly[0].x = w->satPos.x - w->satSize.x;
    poly[0].y = w->satPos.y;
    poly[0].z = w->satPos.z - w->satSize.z;
    poly[1].x = w->satPos.x + w->satSize.x;
    poly[1].y = w->satPos.y;
    poly[1].z = w->satPos.z - w->satSize.z;
    poly[2].x = w->satPos.x + w->satSize.x;
    poly[2].y = w->satPos.y;
    poly[2].z = w->satPos.z + w->satSize.z;
    poly[3].x = w->satPos.x - w->satSize.x;
    poly[3].y = w->satPos.y;
    poly[3].z = w->satPos.z + w->satSize.z;
    if (w->pSat == 0) {
        f32 h = w->satSize.y;

        w->pSat = SatMgr.create(&pos, &rot, poly, w->satN, w->satFlag, h);
    } else {
        w->pSat->flags |= 4;
        w->pSat->setCoord(&pos, &rot);
    }
}

void cEmObj::clrSat()
{
    EmObjWork* w = EMOBJ_WK(this);

    if (w->pSat) {
        w->pSat->flags &= ~4;
    }
    x3E0 &= ~2;
}

void cEmObj::setEat(Vec* pos, int n, int flag, f32 sx, f32 sy, f32 sz)
{
    EmObjWork* w = EMOBJ_WK(this);

    if (pos) {
        w->eatPos = *pos;
    }
    w->eatSize.x = sx;
    w->eatSize.y = sy;
    w->eatSize.z = sz;
    w->eatN = n;
    w->eatFlag = flag;
    x3E0 |= 4;
    setEatMain();
}

void cEmObj::setEatMain()
{
    EmObjWork* w = EMOBJ_WK(this);
    Vec poly[4];

    if (w->pEat) {
        w->pEat->flags &= ~4;
    }
    poly[0].x = w->eatPos.x - w->eatSize.x;
    poly[0].y = w->eatPos.y;
    poly[0].z = w->eatPos.z - w->eatSize.z;
    poly[1].x = w->eatPos.x + w->eatSize.x;
    poly[1].y = w->eatPos.y;
    poly[1].z = w->eatPos.z - w->eatSize.z;
    poly[2].x = w->eatPos.x + w->eatSize.x;
    poly[2].y = w->eatPos.y;
    poly[2].z = w->eatPos.z + w->eatSize.z;
    poly[3].x = w->eatPos.x - w->eatSize.x;
    poly[3].y = w->eatPos.y;
    poly[3].z = w->eatPos.z + w->eatSize.z;
    if (w->pEat == 0) {
        f32 h = w->eatSize.y;

        w->pEat = EatMgr.create(&pos, &rot, poly, w->eatN, w->eatFlag, h);
    } else {
        w->pEat->flags |= 4;
        w->pEat->setCoord(&pos, &rot);
    }
}

void cEmObj::clrEat()
{
    EmObjWork* w = EMOBJ_WK(this);

    if (w->pEat) {
        w->pEat->flags &= ~4;
    }
    x3E0 &= ~4;
}

void cEmObj::setYarare(s16 no, Vec* pos, u16 flag, int cube, f32 w, f32 h, f32 rad)
{
    Vec p;
    int n;
    int f;

    if (pos == 0) {
        p.x = 0.0f;
        p.y = 0.0f;
        p.z = 0.0f;
    } else {
        p.x = pos->x;
        p.y = pos->y;
        p.z = pos->z;
    }
    n = no;
    f = flag | 1;
    if (cube == 0) {
        YarareInitCube(this, n, f, p.x, p.y, p.z, w, h, rad);
    } else {
        YarareInit(this, n, f, p.x, p.y, p.z, w, h);
    }
}

void cEmObj::setEff(u8 v)
{
    EMOBJ_WK(this)->eff = v;
}

u8 cEmObj::getEff()
{
    return EMOBJ_WK(this)->eff;
}

void cEmObj::setEtc(u8 v)
{
    EMOBJ_WK(this)->etc = v;
}

u8 cEmObj::getEtc()
{
    return EMOBJ_WK(this)->etc;
}
