#include "model.h"
#include "db_log.h"
#include "math_sub.h"

// pointer to game memory (0x80000000 .. 0x82FFFFFF)
#define VALID_PTR(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)

cLightInfo::cLightInfo()
{
    int i;

    for (i = 0; i < 8; i++) {
        pLight[i] = 0;
    }
    EnableMask = 0;
    Flag = 0;
    PartsNo = 0;
    x53 = 0;
    SelectMask = 0;
    Offset.x = Offset.y = Offset.z = 0.0f;
    Size.x = Size.y = Size.z = 0.0f;
}

int cLightInfo::init2(int type, int partsNo, const Vec* pOffset, const Vec* pSize, int mask)
{
    int i;

    if (!VALID_PTR(pOffset) || !VALID_PTR(pSize)) {
        pLog->err(0, 0, "cLightInfo::init2() PTR ERROR %08X %08X", pOffset, pSize);
        return 0;
    }
    for (i = 0; i < 8; i++) {
        pLight[i] = 0;
    }
    Flag = type;
    EnableMask = mask;
    PartsNo = partsNo;
    SelectMask = 0xFFFFFFFF;
    Offset = *pOffset;
    Size = *pSize;
    if ((Flag & 3) == 0) {
        Radius = Size.x + Size.y;
    } else if ((Flag & 3) != 2) {
        Radius = Size.x;
    } else {
        Radius = SQRTF(Size.x * Size.x + Size.y * Size.y + Size.z * Size.z);
    }
    return 1;
}

u32 cLightInfo::getLightNum()
{
    u8 n = 0;
    u8 i;

    for (i = 0; i < 8; i++) {
        if (pLight[i] != 0) {
            n++;
        }
    }
    return n;
}

void cLightInfo::updateMatrix(cModel* m)
{
    Vec v;
    Mtx tmp;

    v.x = Offset.x * m->scale.x;
    v.y = Offset.y * m->scale.y;
    v.z = Offset.z * m->scale.z;
    RotVector(&v, &m->ang, &v);
    if (PartsNo == 0) {
        PSVECAdd(&v, &m->pos, &v);
    } else {
        if (m->pParts == 0) {
            return;
        }
        PSVECAdd(&v, &m->getPartsPtr(PartsNo - 1)->world, &v);
    }
    RotMatrix(tmp, &m->ang);
    TransMatrix(tmp, &v);
    PSMTXInverse(tmp, imat);
}

cModel* cLightInfo::getPos(cModel* m, Vec* out)
{
    cModel* c;

    if (PartsNo > 0) {
        c = m->getPartsPtr(PartsNo - 1);
        if (!VALID_PTR(c)) {
            pLog->err(0, 0, "litHitCk PNo%d %d %d %x %x", PartsNo - 1, m->kindid, m->id, Flag, EnableMask);
            c = m;
        }
        PSMTXMultVecSR(c->mat, &Offset, out);
        PSVECAdd(out, &c->world, out);
    } else {
        c = m;
        PSMTXMultVecSR(c->mat, &Offset, out);
        PSVECAdd(out, &c->pos, out);
    }
    return c;
}
