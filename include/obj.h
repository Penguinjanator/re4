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

class cObj;

// Effect model work (game/obj04.cpp `Efm04`): a thrown/falling particle-like model.
struct Efm04Work {
    u8 pad_0[0xC];
    f32 spdDamp;          // 0x0C  speed *= spdDamp every frame
    Vec acc;              // 0x10  added to speed every frame
    Vec rotSpd;           // 0x1C  added to rot every frame
    f32 scaleXZ;          // 0x28
    f32 scaleY;           // 0x2C
    f32 scale;            // 0x30  scale = scale + scaleSpd, scaleSpd *= scaleDamp
    f32 scaleSpd;         // 0x34
    f32 scaleDamp;        // 0x38
    u8 pad_3C[3];
    u8 alpha0;            // 0x3F  alpha at the end of the fade-in
    f32 r;                // 0x40
    f32 g;                // 0x44
    f32 b;                // 0x48
    f32 a;                // 0x4C
    f32 rMul;             // 0x50  fade-out multipliers
    f32 gMul;             // 0x54
    f32 bMul;             // 0x58
    f32 aMul;             // 0x5C
    u16 fadeStart;        // 0x60
    u16 fadeLen;          // 0x62
    u16 moveStart;        // 0x64
    u16 scaleStart;       // 0x66
    u16 life;             // 0x68  0 = forever
    u16 frame;            // 0x6A
    cObj* parent;         // 0x6C
    u32 parentSerial;     // 0x70
    cCoord* parentWorld;  // 0x74  pEffParentWorld when detached
    u8 rotFrame;          // 0x78  frame to re-orient along the parent (0xFF = never)
    u8 x79;
    u8 stopped;           // 0x7A  bit0: came to rest
    u8 x7B;
    u32 flags;            // 0x7C  bit0: floor collision, bit1: scenario collision, bit3: MotionMove
    f32 groundOfs;        // 0x80
    f32 bounceXZ;         // 0x84
    f32 bounceY;          // 0x88
};

// Sub-object at cObj+0x2B4 (0x74 bytes). Only the flag word obj03 clears is known.
struct ObjSub2B4 {
    u8 pad_0[0x1A];
    u16 flags;            // 0x1A
    u8 pad_1C[0x70 - 0x1C];
    s32 blk;              // 0x70  scroll block the object belongs to (-2 free, -1 SetObjSmd)

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
        Efm04Work efm04;
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
    virtual void destroy(cObj* p);
    virtual int construct(cObj* p, int id);

    cObj* getWork(u32 no) {
        if (no >= nArray) {
            return 0;
        }
        return (cObj*)((u8*)pArray + size * no);
    }
};

extern cObjMgr ObjMgr;

#endif
