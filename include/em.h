#ifndef EM_H
#define EM_H

#include "types.h"
#include "cManager.h"
#include "model.h"

// Enemy work (game/em.cpp), sizeof 0xDE0.
class cEm : public cModel {
public:
    u8 pad_1D8[0x290 - 0x1D8];
    f32 frame;            // 0x290  motion frame (db_cam prints it as an int)
    u16 frameMax;         // 0x294
    u8 pad_296[0x320 - 0x296];
    s16 hp;               // 0x320
    s16 hpMax;            // 0x322
    u32 flags_324;        // 0x324  (db_cam: upper 16 bits set = dead)
    u8 pad_328[0x370 - 0x328];
    f32 plDist2;          // 0x370  squared distance to the player (db_work prints its sqrt)
    u8 pad_374[0x38D - 0x374];
    u8 x38D;              // 0x38D  (db_cam "set=")
    u8 pad_38E[0x398 - 0x38E];
    u8 emsetNo;           // 0x398
    u8 pad_399[0x3C8 - 0x399];
    u32 flags_3C8;        // 0x3C8  (db_cam "Flag=")
    u8 pad_3CC[0xDE0 - 0x3CC];

    cEm();
    virtual ~cEm() {}
    int checkStatus(int stat);
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
