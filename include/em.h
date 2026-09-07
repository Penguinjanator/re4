#ifndef EM_H
#define EM_H

#include "types.h"
#include "cManager.h"
#include "model.h"

// Enemy work (game/em.cpp), sizeof 0xDE0.
class cEm : public cModel {
public:
    u8 pad_1D8[0xDE0 - 0x1D8];

    cEm();
    virtual ~cEm() {}
};

class cEmMgr : public cManager<cEm> {
public:
    u32 x34;

    cEmMgr();
    virtual ~cEmMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree();
    virtual void memClear(cEm* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cEm* p, int id);

    // first alive enemy with model id `id`, searching from `start->next` (or the list head)
    cEm* getEmPtr(int id, cEm* start);
    int isBattle();
};

extern cEmMgr EmMgr;

#endif
