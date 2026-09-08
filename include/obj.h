#ifndef OBJ_H
#define OBJ_H

#include "types.h"
#include "cManager.h"
#include "model.h"
#include "atariInfo.h"
#include "pendulum.h"

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

// Obstacle model work (game/obj20.cpp `SetObaModel`).
struct ObaModelWork {
    u8 pad_0[0xC];
    Vec ofs;              // 0x0C  position relative to the parent (parts) matrix
    int partsNo;          // 0x18  parts of the parent followed by type 0
    cObj* parent;         // 0x1C
};

// Fading attachment work (game/obj26.cpp): scales toward `tgtScale`, then shrinks and fades out.
struct Obj26Work {
    u8 pad_0[8];
    cObj* parent;         // 0x08  followed parts 2 of this object
    u8 pad_C[0xC];
    Vec tgtScale;         // 0x18
};

// Bell work (game/obj14.cpp): a hit-receiving enemy plus a pendulum chain for the swing.
struct BellWork {
    u8 pad_0[0xA];
    u16 ringTimer;        // 0x0A  frames the "rung" state is reported to pG (90 after a hit)
    class cEmHit* pEmHit; // 0x0C
    struct PenCloth cloth;  // 0x10 .. 0x70
};

// Floating island work (game/obj1c.cpp): drifts back toward its home position, plays crash
// motions and spawns effects while the player is on it.
struct IslandWork {
    u32 x00;              // 0x00
    u8 pad_4[8];
    int crashTimer;       // 0x0C  frames since setCrashBig (ckCrash)
    int estTimer;         // 0x10  frames until the next idle effect
    int crashEstWait;     // 0x14  frames the crash effect is suppressed
    Vec spd;              // 0x18  push speed (setCrashBig)
    Vec home;             // 0x24  position it drifts back to
    u8 espKind;           // 0x30  effect kind (EspPullCoreKind)
    u8 pad_31[3];
    void* motIdle;        // 0x34  motions: idle / crash, and their big-scale (>= 1.5) variants
    void* motCrash;       // 0x38
    void* motIdleBig;     // 0x3C
    void* motCrashBig;    // 0x40
};

// Thrown / shot object work (game/obj08.cpp): a projectile with gravity, scenario / enemy /
// player hit checks and up to four effect sets.
struct Obj08Work {
    u32 flags;            // 0x00  bit0 start motion, bit1 motion running, bit3 rotate, bit4 enemy hit check, bit5 player hit check
    void* pMot;           // 0x04
    u8 pad_8[2];
    u16 motPrm;           // 0x0A
    Vec rotSpd;           // 0x0C
    Vec spd;              // 0x18
    f32 grav;             // 0x24
    f32 rad;              // 0x28  hit radius (min 1.0)
    cModel* parent;       // 0x2C  thrower (its id goes to SndCall)
    int life;             // 0x30  frames left (-1 = forever)
    void* pAtk;           // 0x34  EmAtkHitCk attack data
    u32 atkFlags;         // 0x38  low 16 bits: GetWepTargetList flag, low byte: damage kind
    u32 estNo[4];         // 0x3C  effects: 0 ?, 1 scenario hit / timeout, 2 floor hit, 3 enemy / player hit
    u32 estPrm[4];        // 0x4C
    u16 seBlk;            // 0x5C  hit SE (0xFFFF = none)
    u16 seNo;             // 0x5E
    u8 estFlag;           // 0x60  1: the enemy-hit effect follows the target instead of the hit point
};

// Ladder / tower work (game/objYagura.cpp).
struct YaguraWork {
    u8 pad_0[0x20];
    void* pMotionVib;     // 0x20  vibration motion set by setVib()
};

// Sub-object at cObj+0x2B4 (0x74 bytes): the collision info followed by scroll bookkeeping.
struct ObjSub2B4 {
    cAtariInfo atari;     // 0x00 .. 0x4C  (flags at 0x1A)
    u8 pad_4C[0x70 - 0x4C];
    s32 blk;              // 0x70  scroll block the object belongs to (-2 free, -1 SetObjSmd)

    void clrFlags(u16 mask) { atari.flags &= mask; }
};

// Map object work (game/obj.cpp), sizeof 0x3D8. Per-object modules keep their state in `work`.
class cObj : public cModel {
public:
    void* pMotion;        // 0x1D8 motion data (MotionMove) or NULL (matUpdate)
    u8 pad_1DC[0x21C - 0x1DC];
    u32 x21C;             // 0x21C  bit30 (0x40000000): set by obj26MatCalc when following a parent
    u8 pad_220[0x2B4 - 0x220];
    ObjSub2B4 sub2B4;     // 0x2B4 .. 0x328
    // 0x328: per-object work area
    union {
        u8 work[0x3D0 - 0x328];  // 0x328 per-object work area
        Obj03Work obj03;
        Efm04Work efm04;
        ObaModelWork obaModel;
        Obj26Work obj26;
        YaguraWork yagura;
        BellWork bell;
        IslandWork island;
        Obj08Work o8;
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
    virtual void memFree(void* p);
    virtual void memClear(cObj* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual void destroy(cObj* p);
    virtual int construct(cObj* p, u32 id);

    cObj* getWork(u32 no) {
        if (no >= nArray) {
            return 0;
        }
        return (cObj*)((u8*)pArray + size * no);
    }
};

extern cObjMgr ObjMgr;

#endif
