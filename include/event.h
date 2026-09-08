#ifndef EVENT_H
#define EVENT_H

#include "types.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/event.h"

// Event system header. Contents unknown; db_work includes it between atari.h and light.h and
// its range check emits the file-name string into that unit's .rodata.
class cEvent {
public:
    u8* pData;
    u32 nData;

    u8* getData(u32 no) {
        if (no >= nData) {
            dbgAssert(__FILE__, __LINE__);
        }
        return pData + no;
    }
    // some inline in the original event.h carries an empty string literal: 4 zero bytes follow
    // the file-name string in every including unit's .rodata
    const char* emptyName() { return ""; }
};

#endif
