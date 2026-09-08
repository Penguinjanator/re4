#ifndef CTRL_H
#define CTRL_H

#include "types.h"
#include "db_log.h"
#include "cManager.h"

// Control work (game/ctrl.cpp, sizeof 0x214). cCtrl00/01/10.. specialise it by `id`.
class cCtrl : public cUnit {
public:
    u8 id;                  // 0x0C  construct id (light: 1 = electric power path control)
    u8 pad_D[6];
    u8 work[0x214 - 0x13];  // 0x13  per-type work area

    cCtrl() {}
    virtual ~cCtrl() {}
    virtual void move();
    virtual void trans();
};

class cCtrlMgr : public cManager<cCtrl> {
public:
    cCtrlMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(cCtrl* p, u32 size);
    virtual int construct(cCtrl* p, u32 id);
};

extern cCtrlMgr CtrlMgr;

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
