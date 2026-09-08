#ifndef WIDGET_H
#define WIDGET_H

#include "types.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/widget.h"

// Header-only widget helpers. Contents unknown; the debug tools include it after light.h and
// its range check emits the file-name string into their .rodata.
class cWidget {
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
