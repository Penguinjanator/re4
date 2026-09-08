#ifndef CTRL_H
#define CTRL_H

#include "types.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/ctrl.h"

// Control (ctrl*.cpp) header. Contents unknown; the object units include it after light.h and
// its range check emits the file-name string into their .rodata.
class cCtrlTbl {
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
