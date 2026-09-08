#ifndef EM_H
#define EM_H

#include "types.h"
#include "cManager.h"
#include "model.h"
#include "atariInfo.h"

// Character work (game/em.cpp), sizeof 0xDE0. The player classes derive from it, so the
// player-only fields the pl_* units touch live here too (they all sit below 0xDE0).
// Hit box / damage part info (cEm+0x33C for the player; GetWepTargetList returns pointers to
// these per target). Only what obj08 reads is named.
struct EmHitInfo {
    u8 pad_0[0x18];
    f32 width;            // 0x18
    f32 height;           // 0x1C
    u8 pad_20[6];
    s16 partsNo;          // 0x26  parts the effect is placed at (0 = the model itself), 1-based
    f32 rad;              // 0x28
};

// Damage info at cEm+0x324 (game/em.cpp). set(0, 10, kind, pos, rad, part) registers a hit.
class cDmgInfo {
public:
    u32 flags;            // 0x00

    void set(int a, int b, u8 kind, Vec* pos, f32 rad, EmHitInfo* part);
};

class cEm : public cModel {
public:
    void* pMotion;        // 0x1D8  motion work head: current motion data, NULL = stopped (pl_push stopTarget)
    u8 pad_1DC[0x290 - 0x1DC];
    f32 frame;            // 0x290  motion frame (db_cam prints it as an int)
    u16 frameMax;         // 0x294
    u8 pad_296[0x2A4 - 0x296];
    struct EmWork2A4* p2A4;  // 0x2A4  0x1FE-byte work (player.cpp mem_alloc; cam_ctrl reads its byte 5)
    u8 pad_2A8[0x2B4 - 0x2A8];
    cAtariInfo atari;     // 0x2B4 .. 0x300  collision info (rect size at 0x2C0/0x2C4)
    u8 pad_300[8];
    void* pFootShadowTbl; // 0x308  player: foot shadow table (pl_leon: pl_fs_tbl)
    u8 pad_30C[0x320 - 0x30C];
    s16 hp;               // 0x320
    s16 hpMax;            // 0x322
    union {
        u32 flags_324;    // 0x324  (db_cam: upper 16 bits set = dead)
        struct {
            u8 x324;      // 0x324  (pl_dmg: cleared when the damage motion ends)
            u8 x325;      // 0x325  (pl_dmg: 5 at the end, bit7 while the damage motion plays)
            u16 x326;
        } st;
        cDmgInfo dmg;     // 0x324  (obj08: dmg.set on a hit target)
    };
    Vec x328;             // 0x328  (obj14: damage position when EmGetDmPos has none)
    u8 pad_334[8];
    EmHitInfo hitInfo;    // 0x33C .. 0x368  (obj08: the player's hit part for the damage effect)
    u8 pad_368[0x370 - 0x368];
    f32 plDist2;          // 0x370  squared distance to the player (db_work prints its sqrt)
    u8 pad_374[0x38D - 0x374];
    u8 x38D;              // 0x38D  (db_cam "set=")
    u8 pad_38E[0x398 - 0x38E];
    u8 emsetNo;           // 0x398
    u8 pad_399[0x3C8 - 0x399];
    u32 flags_3C8;        // 0x3C8  (db_cam "Flag=")
    u8 pad_3CC[0x3E0 - 0x3CC];
    u32 x3E0;             // 0x3E0  player: event walk flag / damage timer
    int x3E4;             // 0x3E4  player damage: 1 = turning towards x400
    u32 x3E8;             // 0x3E8  player damage (blow): water splash done
    u8 pad_3EC[0x400 - 0x3EC];
    f32 x400;             // 0x400  player: event turn limit / damage direction angle (123.0 = none)
    Vec evTarget;         // 0x404  player event: walk-to position
    u8 pad_410[0x41C - 0x410];
    u32 flags_41C;        // 0x41C  player: bit8 (0x100) event motion done -> reset routine
    u32 flags_420;        // 0x420  player: bit6 (0x40) knife routine ends into routine 0x11
    void** pMotTbl;       // 0x424  player: motion data table ([0] walk, [2] turn, [0x5F..0x6C] set by setMotion)
    u8 pad_428[0x4FF - 0x428];
    u8 xButtonWait;       // 0x4FF  player: frames until the X button (partner command) is accepted again
    u8 pad_500[8];
    cModel* pLockEm;      // 0x508  player: locked-on enemy (pl_wep lock, knife aim)
    u8 pad_50C[0x788 - 0x50C];
    class cPlWep* pWep;   // 0x788  player: weapon control (pl_wep.cpp, 0x44 bytes)
    class cPlNeck* pNeck; // 0x78C  player: neck control (pl_class.cpp, 0x1C bytes)
    class cPlWaist* pWaist;  // 0x790  player: waist control (pl_class.cpp, 0xC bytes)
    class cPlBody* pBody; // 0x794  player: body / face / hand model set (pl_body.cpp, 0xF0 bytes)
    u8 pad_798[0xC];
    class cMotBase* pMotBase;  // 0x7A4  (0x38 bytes)
    u8 pad_7A8[4];
    Vec bustBase[3];      // 0x7AC  Ashley: rest positions of parts 0x1D, 0x1E, 0x1A (pl_ashley moveBust)
    u8 pad_7D0[0xDE0 - 0x7D0];

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
    virtual void memFree(void* p);
    virtual void memClear(cEm* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cEm* p, u32 id);

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
