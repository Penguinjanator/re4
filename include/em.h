#ifndef EM_H
#define EM_H

#include "types.h"
#include "cManager.h"
#include "model.h"
#include "atariInfo.h"

// Character work (game/em.cpp), sizeof 0xDE0. The player classes derive from it, so the
// player-only fields the pl_* units touch live here too (they all sit below 0xDE0).
class cEm : public cModel {
public:
    void* pMotion;        // 0x1D8  motion work head: current motion data, NULL = stopped (pl_push stopTarget)
    u8 pad_1DC[0x290 - 0x1DC];
    f32 frame;            // 0x290  motion frame (db_cam prints it as an int)
    u16 frameMax;         // 0x294
    u8 pad_296[0x2A4 - 0x296];
    struct cPlWep* pWep;  // 0x2A4  player: equipped weapon (0x05: u8 busy flag)
    u8 pad_2A8[0x2B4 - 0x2A8];
    cAtariInfo atari;     // 0x2B4 .. 0x300  collision info (rect size at 0x2C0/0x2C4)
    u8 pad_300[8];
    void* pFootShadowTbl; // 0x308  player: foot shadow table (pl_leon: pl_fs_tbl)
    u8 pad_30C[0x320 - 0x30C];
    s16 hp;               // 0x320
    s16 hpMax;            // 0x322
    u32 flags_324;        // 0x324  (db_cam: upper 16 bits set = dead)
    u8 pad_328[0x370 - 0x328];
    f32 plDist2;          // 0x370  squared distance to the player (db_work prints its sqrt)
    u8 pad_374[0x38D - 0x374];
    u8 x38D;              // 0x38D  (db_cam "set=")
    u8 pad_38E[0x398 - 0x38E];
    u8 emsetNo;           // 0x398
    u8 pad_399[0x3C8 - 0x399];
    u32 flags_3C8;        // 0x3C8  (db_cam "Flag=")
    u8 pad_3CC[0x3E0 - 0x3CC];
    u32 x3E0;             // 0x3E0  player event: set to 1 when the walk-to-target motion starts
    u8 pad_3E4[0x400 - 0x3E4];
    f32 evTurnSpeed;      // 0x400  player event: max turn per frame (rad)
    Vec evTarget;         // 0x404  player event: walk-to position
    u8 pad_410[0x41C - 0x410];
    u32 flags_41C;        // 0x41C  player: bit8 (0x100) event motion done -> reset routine
    u8 pad_420[4];
    void** pMotTbl;       // 0x424  player: motion data table ([0] walk, [2] turn, [0x5F..0x6C] set by setMotion)
    u8 pad_428[0x4FF - 0x428];
    u8 xButtonWait;       // 0x4FF  player: frames until the X button (partner command) is accepted again
    u8 pad_500[0x794 - 0x500];
    struct PlFaceInfo* pFace;  // 0x794  player: face/hand model set (t_option FACE CONTROL)
    u8 pad_798[0xDE0 - 0x798];

    cEm();
    virtual ~cEm() {}
    virtual void move();
    virtual void setItem(u16 a, u16 b, u16 c, u16 d, u8 e);  // 0x3D6.. item drop (0x3D1 flag)
    virtual void setNoItem();
    virtual int checkThrow();
    int checkStatus(int stat);
};

class cEmMgr : public cManager<cEm> {
public:
    u32 x34;

    cEmMgr();
    virtual ~cEmMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree();
    virtual void memClear(cEm* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cEm* p, int id);

    // first alive enemy with model id `id`, searching from `start->next` (or the list head)
    cEm* getEmPtr(int id, cEm* start);
    int isBattle();

    cEm* getWork(u32 no) {
        if (no >= nArray) {
            return 0;
        }
        return (cEm*)((u8*)pArray + size * no);
    }
};

extern cEmMgr EmMgr;

// Pushable rack/crate enemy (game/emrack.cpp); only what pl_push calls.
class cEmRack : public cEm {
public:
    int adjustRange(u8 dir);
};

#endif
