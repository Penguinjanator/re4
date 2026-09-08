#ifndef OBJ_H
#define OBJ_H

#include "types.h"
#include "cManager.h"
#include "model.h"

// Map object work (game/obj.cpp), sizeof 0x3D8. Per-object modules keep their state in `work`.
class cObj : public cModel {
public:
    void* pMotion;        // 0x1D8 motion data (MotionMove) or NULL (matUpdate)
    u8 pad_1DC[0x2B4 - 0x1DC];
    u8 pad_2B4[0x1A];
    u16 x2CE;             // 0x2CE
    u8 pad_2D0[0x328 - 0x2D0];
    u8 work[0x3D0 - 0x328];  // 0x328 per-object work area
    u8 x3D0;              // 0x3D0
    u8 pad_3D1[3];
    void (*callBack)(cObj*);  // 0x3D4

    cObj();
    virtual ~cObj() {}
};

class cObjMgr : public cManager<cObj> {
public:
    u32 x34;

    cObjMgr();
    virtual ~cObjMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree();
    virtual void memClear(cObj* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cObj* p, int id);
};

extern cObjMgr ObjMgr;

#endif
