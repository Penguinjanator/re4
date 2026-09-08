#ifndef DMG_H
#define DMG_H

#include "types.h"
#include "vec.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/dmg.h"

// Damage header. Contents unknown; the obstacle units include it after light.h and its range
// check emits the file-name string into their .rodata.
class cDmgTbl {
public:
    u8* pData;
    u32 nData;

    u8* getData(u32 no) {
        if (no >= nData) {
            dbgAssert(__FILE__, __LINE__);
        }
        return pData + no;
    }
};

// Damage volume manager (game/dmg.cpp `DmgMgr`, 0x34 bytes, a cManager); layout opaque.
class cDmgMgr {
public:
    u8 pad_0[0x34];

    // Registers a damage volume: kind, frames, centre, radius, height. Returns 1 when a volume
    // was created (int result: the call's set of r3 changes the haifa depend counts, obj10 dmgSet).
    int set(int kind, int time, Vec* pos, f32 r, f32 h);
    // Damage volume containing `pos`: its kind (1/4/5/7 break the item enemies), 0 when none; `out` gets the hit point
    int hitCheck(Vec* pos, Vec* out);
};

extern cDmgMgr DmgMgr;

#endif
