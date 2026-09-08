#ifndef ATARI_H
#define ATARI_H

#include "types.h"
#include "vec.h"
#include "db_log.h"

// Scenario collision manager (game/atari.cpp). Only the entry points used by the effect
// units are declared; the layout is opaque (sizeof 0x38).
class cSatMgr {
public:
    u8 pad_0[0x38];

    // Ray from `top` down to `bottom`; returns the hit flags (bit2: no floor), hit point in `hit`.
    int hitCheck2(Vec* top, Vec* bottom, Vec* hit, u32* attr, int flag, int x);
    // Floor height under `pos`, searching `up` above and `down` below it.
    f32 getFloor(Vec* pos, f32 up, f32 down, int a, int b);
};

extern cSatMgr SatMgr;

#line 8 "D:/Bio4/Prog/atari.h"

// 16 one-bit flags with range-checked access (game/atari.cpp).
class cFlag {
public:
    u16 flags;

    void set(u32 stat) {
        if (stat > 15) {
            pLog->err(0, 0, "cFlag.set() arg stat OVER FLOW %d", stat);
            return;
        }
        flags |= 1 << stat;
    }
    void clr(u32 stat) {
        if (stat > 15) {
            dbgAssert(__FILE__, __LINE__);
            return;
        }
        flags &= ~(1 << stat);
    }
    int check(u32 stat) {
        if (stat > 15) {
            pLog->err(0, 0, "cFlag.set() arg stat OVER FLOW %d", stat);
            return 0;
        }
        return flags & (1 << stat);
    }
};

#endif
