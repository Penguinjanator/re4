#ifndef EM_H
#define EM_H

#include "types.h"
#include "cManager.h"
#include "model.h"
#include "atariInfo.h"
#include "main_mem.h"

// Character work (game/em.cpp), sizeof 0xDE0. The player classes derive from it, so the
// player-only fields the pl_* units touch live here too (they all sit below 0xDE0).
// Hit box ("yarare") / damage part info (cEm+0x33C for the player; GetWepTargetList returns
// pointers to these per target), 0x34 bytes; extra boxes are chained through `next` (at_mod.cpp
// YarareAdd / YarareAddCube).
struct EmHitInfo {
    Vec ofs;              // 0x00  box centre offset from the model / parts (yarareInit0 x, y, z)
    Vec pos;              // 0x0C  hit position in the parts (obj1b: the spear sticks here)
    f32 width;            // 0x18
    f32 height;           // 0x1C
    f32 depth;            // 0x20  cube depth (YarareInitCube / YarareAddCube set it with flags bit3)
    u16 flags;            // 0x24  bit3 (0x8): cube, bit5 (0x20): the hit sets cDmgInfo bit5 too (pl_wep PlWepHitCheck2)
    s16 partsNo;          // 0x26  parts the effect is placed at (0 = the model itself), 1-based
    f32 rad;              // 0x28
    u8 pad_2C[4];
    EmHitInfo* next;      // 0x30  next hit box of the model (YarareAdd)
};

// Damage info at cEm+0x324 (game/em.cpp), 0x18 bytes. set(0, 10, kind, pos, rad, part) registers a hit.
class cDmgInfo {
public:
    union {
        u32 flags;        // 0x00
        struct {
            u8 stat;      // 0x00  bit0: a hit is registered, bit5 (pl_wep)
            u8 x1;        // 0x01  frames the hit stays registered (move: bit7 = hold, low bits count down)
            u16 x2;
        };
        struct {
            u8 pad_0[2];
            u8 kind;      // 0x02  set() kind
            u8 x3;
        };
    };
    Vec pos;              // 0x04  hit position
    f32 rad;              // 0x10
    EmHitInfo* part;      // 0x14  hit part

    cDmgInfo();
    void set(int a, int b, u8 kind, Vec* pos, f32 rad, EmHitInfo* part);
    void set(int a, int b);   // stores the two bytes at 0/1 (pl_sub: set(0, 10), set(0, 0x80))
    void clear();
    void move();              // counts x1 down; clears stat when it reaches 0
};

// Room water effect table registered at cEm::pRoomEff (pl_sub PlRegistRoomEff): 3 entries of
// {u32 id; u8 pad[3]; u8 type;} used as EstSet(..., id, type, ...) for the ripple / splash effects.
struct PlRoomEff {
    u32 id;
    u8 pad_4[3];
    u8 type;
};

// Light area block (game/light_area.cpp) at cEm+0x30C: scales one light's colour on the model.
struct EmLightArea {
    u32 x0;          // 0x00
    u32 flags;       // 0x04  bit0 active, bit1 scale valid
    s32 lightNo;     // 0x08  cLight::x140 of the light to scale
    f32 scale;       // 0x0C

    int chk(u32 bit)
    {
        if (flags & bit) {
            return 1;
        }
        return 0;
    }
};

class cEm : public cModel {
public:
    void* pMotion;        // 0x1D8  motion work head: current motion data, NULL = stopped (pl_push stopTarget)
    u8 pad_1DC[0x21A - 0x1DC];
    u16 motState;         // 0x21A  MotionWork::state (emobj EmObjMove clears it when no motion plays)
    u32 motFlags2;        // 0x21C  MotionWork::flags2 (emhit: bit30 = no matrix update before MotionMove)
    u8 pad_220[0x28A - 0x220];
    u8 seNo;              // 0x28A  sound number + 1 to play at parts 0 this frame (emMove SndCall(8, ...)), 0 = none
    u8 pad_28B[0x290 - 0x28B];
    f32 frame;            // 0x290  motion frame (db_cam prints it as an int)
    u16 frameMax;         // 0x294
    u8 pad_296[0x2A4 - 0x296];
    struct EmWork2A4* p2A4;  // 0x2A4  0x1FE-byte work (player.cpp mem_alloc; cam_ctrl reads its byte 5)
    u8 pad_2A8[0x2B4 - 0x2A8];
    // 0x2B4 .. 0x300  collision info (rect size at 0x2C0/0x2C4); wrapped so that cEm::cEm does not
    // run cAtariInfo's constructor (the original constructs only the cDmgInfo)
    union {
        struct {
            cAtariInfo atari;
        };
    };
    u8 pad_300[8];
    void* pFootShadowTbl; // 0x308  player: foot shadow table (pl_leon: pl_fs_tbl)
    EmLightArea litArea;  // 0x30C  light_area: per-light colour scale (trans_lit lightSetColor)
    u8 pad_31C[0x320 - 0x31C];
    s16 hp;               // 0x320
    s16 hpMax;            // 0x322
    // 0x324 .. 0x33C: the cDmgInfo (em.cpp constructs it explicitly; a class with a constructor
    // cannot sit in a union directly) and the same bytes under the names the other units use.
    union {
        struct {
            cDmgInfo dmg; // 0x324  (obj08: dmg.set on a hit target)
        };
        struct {
            union {
                u32 flags_324;    // 0x324  (db_cam: upper 16 bits set = dead)
                struct {
                    u8 x324;      // 0x324  (pl_dmg: cleared when the damage motion ends)
                    u8 x325;      // 0x325  (pl_dmg: 5 at the end, bit7 while the damage motion plays)
                    u16 x326;
                } st;
                struct {
                    u8 dmHit;     // 0x324  damage registered this frame (emhit emHitDmCk consumes it)
                    u8 dmType;    // 0x325  emhit: 1, 0x11 for weapon 0x10
                    u8 dmWep;     // 0x326  weapon id of the damage (cEmHit::ckDmgWeapon)
                    u8 dm327;
                };
            };
            Vec x328;             // 0x328  (obj14: damage position when EmGetDmPos has none)
            f32 dmRad;            // 0x334  cDmgInfo::set rad
            EmHitInfo* dmPart;    // 0x338  cDmgInfo::set part (emswitch: its rad decides the blood type)
        };
    };
    EmHitInfo hitInfo;    // 0x33C .. 0x370  (obj08: the player's hit part for the damage effect)
    f32 plDist2;          // 0x370  squared distance to the player (db_work prints its sqrt)
    f32 x374;             // 0x374  (em_set: 1e16 at creation)
    u32 x378;             // 0x378  (pl_sub EndPlDamage/EndSubDamage: x378 = x37C)
    u32 x37C;             // 0x37C
    Vec lockOfs;          // 0x380  lock-on point offset in the lockParts' matrix (pl_wep)
    u8 lockParts;         // 0x38C  parts the lock-on point follows (pl_wep; AutoTrack uses the low 3 bits)
    u8 x38D;              // 0x38D  (db_cam "set=")
    u8 pad_38E[0x398 - 0x38E];
    u8 emsetNo;           // 0x398
    u8 pad_399[0x3A8 - 0x399];
    Vec x3A8;             // 0x3A8  (objTrolley objTrolleySetAdjust adds the car movement to it)
    u8 pad_3B4[4];
    int dmgType;          // 0x3B8  (pl_sub SetPlDamage/SetSubDamage first argument)
    u8 pad_3BC[8];
    u32 status;           // 0x3C4  setStatus / clearStatus / checkStatus bits (bit0 = in battle, bit1, bit11)
    u32 flags_3C8;        // 0x3C8  (db_cam "Flag=")
    f32 x3CC;             // 0x3CC  (em_set: list entry s16 x1A * 1000)
    u8 x3D0;              // 0x3D0  (em_set: list entry byte 0xB)
    u8 itemFlag;          // 0x3D1  setItem 5th argument (setNoItem: 0)
    u8 pad_3D2[4];
    u16 itemNo;           // 0x3D6  setItem a (setNoItem: 0xFFFF)
    u16 itemNum;          // 0x3D8  setItem b
    u16 item3DA;          // 0x3DA  setItem c
    u16 item3DC;          // 0x3DC  setItem d
    u8 pad_3DE[2];
    u32 x3E0;             // 0x3E0  player: event walk flag / damage timer
    int x3E4;             // 0x3E4  player damage: 1 = turning towards x400
    u32 x3E8;             // 0x3E8  player damage (blow): water splash done
    u8 pad_3EC[0x400 - 0x3EC];
    union {
        f32 x400;         // 0x400  player: event turn limit / damage direction angle (123.0 = none)
        struct {
            u16 subFlags;   // 0x400  sub character (cSubChar): bit7 (0x80) manual control, bit6 (0x40) ok to control, bit4 (0x10), bit3 (0x8) move-to, bit0
            u16 subFlags2;  // 0x402  cSubChar (pl_sub SubCharMoveTo clears 0x60)
        };
    };
    Vec evTarget;         // 0x404  player event: walk-to position
    u8 pad_410[0x41C - 0x410];
    u32 flags_41C;        // 0x41C  player: bit8 (0x100) event motion done -> reset routine
    u32 flags_420;        // 0x420  player: bit6 (0x40) knife routine ends into routine 0x11
    void** pMotTbl;       // 0x424  player: motion data table ([0] walk, [2] turn, [0x5F..0x6C] set by setMotion)
    void** pRegistMot;    // 0x428  player: registered motion table (pl_sub PlRegistMotion fills [0..11])
    u8 pad_42C[0x4FC - 0x42C];
    u8 x4FC;              // 0x4FC  (pl_sub PlChangeData/PlMotionReset clear it)
    u8 x4FD;              // 0x4FD
    u8 x4FE;              // 0x4FE
    u8 xButtonWait;       // 0x4FF  player: frames until the X button (partner command) is accepted again
    u8 pad_500[8];
    cModel* pLockEm;      // 0x508  player: locked-on enemy (pl_wep lock, knife aim)
    u8 pad_50C[0x518 - 0x50C];
    int gachaCnt;         // 0x518  player: button mash counter (pl_sub PlGacha*)
    u8 pad_51C[2];
    u8 eyeMode;           // 0x51E  player (pl_sub PlSetEyeMode)
    u8 pad_51F[0x530 - 0x51F];
    int subHideMode;      // 0x530  cSubChar (pl_sub SubCharCtrlHide)
    int subX534;          // 0x534  cSubChar (SubCharCtrlHide mode 0 sets 1)
    u8 pad_538[0x544 - 0x538];
    Vec subHidePos;       // 0x544  cSubChar hide position
    u8 pad_550[0x568 - 0x550];
    int subAux0;          // 0x568  cSubChar (SetSubAux/SetSubBulldozer arguments)
    int subAux1;          // 0x56C
    f32 subMoveTo[4];     // 0x570  cSubChar (SubCharMoveTo x, y, z, w)
    u8 pad_580[4];
    void* subMot0;        // 0x584  cSubChar registered motions (SubCharRegistMotion, SetSubDamage)
    void* subMot1;        // 0x588
    u8 subFlags58C;       // 0x58C  cSubChar (SetSubDamage sets 0x40)
    u8 pad_58D[0x740 - 0x58D];
    struct PlRoomEff* pRoomEff;  // 0x740  player: room water effect table (pl_sub PlRegistRoomEff/PlWaterProc)
    void* boss0;          // 0x744  player (pl_sub PlRegistBoss)
    void* boss1;          // 0x748
    u8 pad_74C[0x788 - 0x74C];
    class cPlWep* pWep;   // 0x788  player: weapon control (pl_wep.cpp, 0x44 bytes)
    class cPlNeck* pNeck; // 0x78C  player: neck control (pl_class.cpp, 0x1C bytes)
    class cPlWaist* pWaist;  // 0x790  player: waist control (pl_class.cpp, 0xC bytes)
    class cPlBody* pBody; // 0x794  player: body / face / hand model set (pl_body.cpp, 0xF0 bytes)
    u8 pad_798[0xC];
    class cMotBase* pMotBase;  // 0x7A4  (0x38 bytes)
    u8 pad_7A8[4];
    Vec bustBase[3];      // 0x7AC  Ashley: rest positions of parts 0x1D, 0x1E, 0x1A (pl_ashley moveBust)
    u8 pad_7D0[0x9BC - 0x7D0];
    f32 x9BC;             // 0x9BC  (objTrolley objTrolleyFallEM: rot.y when thrown off the car)
    u8 pad_9C0[0xD60 - 0x9C0];
    Mtx rackMat;          // 0xD60  cEmRack push range matrix (setRange: rot * trans of the rack)
    Mtx rackInvMat;       // 0xD90  its inverse (adjustRange transforms the position into range space)
    f32 rackRange[4];     // 0xDC0  cEmRack push limits (adjustRange dir 0: [1], 1: -[2], 2: [0], 3: -[3])
    u8 rackFlags;         // 0xDD0  cEmRack: bit4 (0x10) range set; SetRack initialises it to 0xF
    u8 pad_DD1[0xDE0 - 0xDD1];

    cEm();
    virtual ~cEm() {}
    virtual void move();
    virtual void setItem(u16 a, u16 b, u16 c, u16 d, u8 e);  // 0x3D6.. item drop (0x3D1 flag)
    virtual void setNoItem();
    virtual int checkThrow();
    void setStatus(int bit);     // status |= 1 << bit
    void clearStatus(int bit);
    int checkStatus(int stat);
    int initWork();              // be_flag = 0x21, x12E = 0 (the constructor)
};

// Enemy manager (game/em.cpp). The construct id selects the class: 0 player, 1..0xE / others a
// read-table enemy (EmInitFunc), 0x40.. the object enemies (cEmObj, cEmDoor, ...), 0xFF a plain cEm.
class cEmMgr : public cManager<cEm> {
public:
    u32 x34;              // 0x34  next cModel::serial (construct)

    static const char* idName[96];   // debug names per construct id

    cEmMgr();
    // no user destructor: the synthesized one (and cManager<cEm>'s) land after the other inlines
    virtual void* memAlloc(u32 size) { return MemAlloc(size, 1); }
    virtual void memFree(void* p) { MemFree(p); }
    virtual void memClear(cEm* p, u32 size) { memclr_asm(p, size); }
    virtual void log(const char* fmt, ...);
    virtual void destroy(cEm* p);   // em.cpp overrides the cManager one (pl_sub SubCharCtrl / PlDataRelease)
    virtual int construct(cEm* p, u32 id);

    int arrayAlloc(u32 n);        // cManager<cEm>::arrayAlloc + pPL = pSUB = 0; returns 1
    void move();                  // dieCheck, RouteCk, emMove for every alive work (or only pSUB when stopped)
    // first alive enemy with model id `id`, searching from `start->next` (or the list head)
    cEm* getEmPtr(int id, cEm* start);
    int isBattle();               // 1 when any alive enemy has status bit0
    void destroyAll();            // killEm on every alive work (id != 0)
};

extern cEmMgr EmMgr;

// Work `no` of the enemy manager, NULL when out of range. A free function: a cEmMgr member (even an
// out-of-class inline) is emitted out of line into em.cpp, which owns the vtable (ctrl.h CtrlMgrWork).
static inline cEm* EmMgrWork(u32 no)
{
    if (no >= EmMgr.nArray) {
        return 0;
    }
    return (cEm*)((u8*)EmMgr.pArray + EmMgr.size * no);
}

// Pushable rack/crate enemy (game/emrack.cpp); only what pl_push calls.
class cEmRack : public cEm {
public:
    virtual void move();   // key function: keeps the vtable in emrack.o (cEmMgr::construct stores it)

    void setBreak();
    void setDown(Vec* pos);
    void setShock();
    void setEff(u8 eff);
    void setRange(f32 a, f32 b, f32 c, f32 d);
    int adjustRange(u8 dir);
};

extern "C" {
void emMove(cEm* em);        // per-frame update of one alive work: distance to the player, damage info, move()
void battleCheck(cEm* em);
void killEm(cEm* em);
}

#endif
