#include "types.h"
#include "cManager.h"
#include "map_obj.h"
#include "eprintf.h"

cMapMgr MapMgr;

int MapObjWork = 0;

cMapMgr::cMapMgr() : cManager<cMap>(sizeof(cMap), 0)
{
    setName("cMapMgr");
}

int cMapMgr::construct(cMap* p, u32 id)
{
    u8 n = 0;
    u32 i;

    for (i = 0; i < nArray; i++) {
        cMap* q = getWork(i);
        if ((q->be_flag & 0x201) != 1 || q->id != (int)id) {
            continue;
        }
        n++;
    }
    p = new (p) cMap;
    p->index = n;
    p->id = id;
    return 1;
}

cMap* cMapMgr::room(int id, int no)
{
    u32 i;

    for (i = 0; i < nArray; i++) {
        cMap* p = getWork(i);
        if ((p->be_flag & 0x201) == 1 && p->id == id && p->index == no) {
            return p;
        }
    }
    return 0;
}

void cMapMgr::move()
{
    u32 i;

    for (i = 0; i < nArray; i++) {
        cMap* p = getWork(i);
        if ((p->be_flag & 0x201) == 1 && (p->be_flag & 0x20)) {
            dieCheck();
            p->move();
            p->updateOldPos();
        }
    }
    dispInfo();
}

int cMapMgr::dispInfo()
{
    u32 n = 0;
    u32 i;

    if (pArray == 0) {
        return 0;
    }
    for (i = 0; i < nArray; i++) {
        cMap* p = (cMap*)((u8*)pArray + size * i);
        if ((p->be_flag & 0x201) == 1) {
            n++;
        }
    }
    eprintf(0x1D8, 0x2A, 0, 1, "%3d", nArray - n);
    return 1;
}

cMap::cMap()
{
    be_flag |= 0x1023;
    x12E = 2;
}

void cMap::move()
{
    static int timer = 0;

    if (xFC != 0) {
        return;
    }
    timer = 0;
    xFC++;
}
