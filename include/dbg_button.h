#ifndef DBG_BUTTON_H
#define DBG_BUTTON_H

#include "types.h"
#include "db_log.h"

// Debug button-menu helpers (header only, no function of it survives in the DOL and it has no
// range check, so the original file name is unknown). event.cpp and sscrn.cpp include it after
// event.h: only the string literals of its inline members land in their .rodata, in this order.
// Nothing here is polymorphic, so unused inlines emit no code.
class cDbgButtonBase {
public:
    u8* pWork;
    int num;
    int cursor;

    int Init(int n) {
        if (pWork == 0) {
            pLog->err(0, 0, "cDbgButtonBase::Init(): new failed.");
            return 0;
        }
        num = n;
        return 1;
    }
    const char* cursorMark() { return ">"; }
    const char* nameHeader() { return "Name:                 "; }
    const char* nameBlank() { return "               "; }
    const char* noHeader() { return "No  :"; }
    const char* noBlank() { return " xx "; }
    const char* okMark() { return "[OK]"; }
    const char* fmtNo() { return "%s%02d%s"; }
    const char* fmtName() { return "%s%s%02d%s"; }
    const char* fmtNum() { return " %02d"; }
    const char* okButton() { return " [OK] "; }
    const char* cancelButton() { return "[CANCEL]"; }
};

#endif
