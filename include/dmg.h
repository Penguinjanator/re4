#ifndef DMG_H
#define DMG_H

#include "types.h"
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

#endif
