#ifndef ATARI_H
#define ATARI_H

#include "types.h"
#include "db_log.h"

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
