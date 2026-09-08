#ifndef OBJ_H
#define OBJ_H

#include "types.h"
#include "cManager.h"
#include "model.h"

// Per-object work layouts (game/obj03.cpp ...), all overlaid at cObj+0x328.
struct Obj03Work {
    u8 x0;        // 0x00
    u8 x1;        // 0x01
    u8 x2;        // 0x02
    u8 x3;        // 0x03
    f32 length;   // 0x04 path length
    f32 t;        // 0x08 current position on the path
    f32 speed;    // 0x0C
    u32 flags;    // 0x10 bit0: debug draw
    void* data;   // 0x14
    void* path;   // 0x18
};

// Sub-object at cObj+0x2B4 (0x74 bytes). Only the flag word obj03 clears is known.
struct ObjSub2B4 {
    u8 pad_0[0x1A];
    u16 flags;            // 0x1A
    u8 pad_1C[0x74 - 0x1C];

    void clrFlags(u16 mask) { flags &= mask; }
};

// Map object work (game/obj.cpp), sizeof 0x3D8. Per-object modules keep their state in `work`.
class cObj : public cModel {
public:
    void* pMotion;        // 0x1D8 motion data (MotionMove) or NULL (matUpdate)
    u8 pad_1DC[0x2B4 - 0x1DC];
    ObjSub2B4 sub2B4;     // 0x2B4 .. 0x328
    // 0x328: per-object work area
    union {
        u8 work[0x3D0 - 0x328];  // 0x328 per-object work area
        Obj03Work obj03;
    };
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
