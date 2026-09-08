#ifndef EM_H
#define EM_H

#include "types.h"
#include "cManager.h"
#include "model.h"

// Enemy work (game/em.cpp), sizeof 0xDE0.
class cEm : public cModel {
public:
    u8 pad_1D8[0x320 - 0x1D8];
    s16 hp;               // 0x320
    s16 hpMax;            // 0x322
    u8 pad_324[0x370 - 0x324];
    f32 plDist2;          // 0x370  squared distance to the player (db_work prints its sqrt)
    u8 pad_374[0x398 - 0x374];
    u8 emsetNo;           // 0x398
    u8 pad_399[0xDE0 - 0x399];

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

    cEm* getWork(u32 no) {
        if (no >= nArray) {
            return 0;
        }
        return (cEm*)((u8*)pArray + size * no);
    }
};

extern cEmMgr EmMgr;

#endif
