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

// Hanging object work (game/obj00.cpp): follows a parts of its parent (`oya`) with a slerp
// blend, or falls as a three-point rope (obj00FallMove).
struct Obj00Work {
    u32 flags;            // 0x00  bit2: falling, bit3: blending toward the parent, bit5: fading out
    void* pMot;           // 0x04
    int motA;             // 0x08  MotionSetCore 4th argument
    u32 motPrm;           // 0x0C  low 16 bits: MotionSetCore 6th argument
    cModel* oya;          // 0x10  parent
    int partsNo;          // 0x14  parts of the parent to follow
    f32 rate;             // 0x18  blend rate (1.0 = parent matrix)
    f32 rateSpd;          // 0x1C
    s16 fallSpd[3][3];    // 0x20  rope point speeds * 10
    u8 pad_32[2];
    Mtx mat;              // 0x34  previous parent matrix
    u8 seBlk;             // 0x64  landing SE
    u8 seNo;              // 0x65
    u8 seId;              // 0x66
    u8 sePlayed;          // 0x67
};

// Hanging / thrown object work (game/obj12.cpp `cObj12`): the obj00 layout with a life counter,
// the landing SE moved to 0x68 and a rope `type` selecting the three rope offsets.
struct Obj12Work {
    u32 flags;            // 0x00  bit2: falling, bit3: blending toward the parent, bit7: keep the parent matrix, bit8: thrown, bit9: fading out after `life`
    void* pMot;           // 0x04
    int motResult;        // 0x08  MotionMove result of this frame
    u32 motPrm;           // 0x0C
    cModel* oya;          // 0x10  parent
    int partsNo;          // 0x14
    f32 rate;             // 0x18  blend rate (1.0 = parent matrix)
    f32 rateSpd;          // 0x1C
    s16 fallSpd[3][3];    // 0x20  rope point speeds * 10 (fallSpd[0] is the throw speed)
    u8 pad_32[2];
    Mtx mat;              // 0x34  previous parent matrix
    int life;             // 0x64  frames before the fade out
    u8 seBlk;             // 0x68  landing SE (0xFF = none)
    u8 seNo;              // 0x69
    u8 seId;              // 0x6A
    u8 sePlayed;          // 0x6B
    u8 type;              // 0x6C  rope offsets table index (setFall)
};

// Event costume / cloth model work (game/obj18.cpp): a model that follows a parts of its parent
// (like obj00) and runs one of the cloth simulations by `type`.
struct Obj18Work {
    u32 flags;            // 0x00  bit3: blending toward the parent, bit6: cloth simulation off
    u8 pad_4[0xC];
    cModel* oya;          // 0x10  parent
    int partsNo;          // 0x14
    f32 rate;             // 0x18  blend rate (1.0 = parent matrix)
    f32 rateSpd;          // 0x1C
    u8 pad_20[0x14];
    Mtx mat;              // 0x34  previous parent matrix
    u32 x64;              // 0x64
    u32 type;             // 0x68  SetObj18 type (cloth set)
    u32 cmf;              // 0x6C  Obj18CmfSet/Get flag bits
    cObj* child;          // 0x70  ribbon / rope object created by SetObj18
    int x74;              // 0x74
    u8 pad_78[0xC];
    u8 debugFlag;         // 0x84
    u8 pad_85[3];
};

// Grenade work (game/obj01.cpp): flies under gravity, bounces off the scenario, can be held by a
// model (follows its parts) and explodes / lands in water after `life` frames.
struct Obj01Work {
    u32 flags;            // 0x00  bit0 start motion, bit1 motion running, bit2 water / bounce check, bit3 rotate parts 0
    void* pMot;           // 0x04
    u8 pad_8[2];
    u16 motPrm;           // 0x0A
    Vec rotSpd;           // 0x0C
    Vec spd;              // 0x18
    f32 grav;             // 0x24
    f32 rad;              // 0x28  bounce radius
    int life;             // 0x2C  frames until the explosion (0 = now)
    cModel* hold;         // 0x30  model holding it (follows `holdParts`)
    int holdParts;        // 0x34
    Vec holdOfs;          // 0x38
    Vec holdRot;          // 0x44
    int estNo0;           // 0x50  explosion effects (-1 = none: fade out instead)
    int estPrm0;          // 0x54
    int estNo1;           // 0x58
    int estPrm1;          // 0x5C
    int estNo2;           // 0x60  water splash
    int estPrm2;          // 0x64
    int estNo3;           // 0x68  underwater explosion
    int estPrm3;          // 0x6C
    u32 type;             // 0x70  0 plain, 1 hand grenade, 2 incendiary, 3 flash, 4 ?
    int holdTimer;        // 0x74  frames until it leaves the holder's hand
    u8 seDone;            // 0x78  landing SE state
    u8 pad_79[3];
    u32 flags7C;          // 0x7C  bit3: water splash done
};

// Thrown weapon item work (game/obj10.cpp `cWepItem`): the grenade layout (Obj01Work) with the
// landing SE counters split out.
struct WepItemWork {
    u32 flags;            // 0x00  bit0 start motion, bit1 motion running, bit2 water / bounce check, bit3 rotate parts 0
    void* pMot;           // 0x04
    u8 pad_8[2];
    u16 motPrm;           // 0x0A
    Vec rotSpd;           // 0x0C
    Vec spd;              // 0x18
    f32 grav;             // 0x24
    f32 rad;              // 0x28  bounce radius
    int life;             // 0x2C  frames until the explosion (0 = now)
    cModel* hold;         // 0x30  model holding it (follows `holdParts`)
    int holdParts;        // 0x34
    Vec holdOfs;          // 0x38
    Vec holdRot;          // 0x44
    int estNo0;           // 0x50  explosion effects (-1 = none: fade out instead)
    int estPrm0;          // 0x54
    int estNo1;           // 0x58
    int estPrm1;          // 0x5C
    int estNo2;           // 0x60  water splash
    int estPrm2;          // 0x64
    int estNo3;           // 0x68  underwater explosion
    int estPrm3;          // 0x6C
    u32 type;             // 0x70  0 plain, 1 water bomb, 2 explosive
    int holdTimer;        // 0x74  frames until it leaves the holder's hand
    u8 seLeft;            // 0x78  bounce SEs left to play
    u8 seCnt;             // 0x79  bounce SEs played
    u8 pad_7A[2];
    u32 flags7C;          // 0x7C  bit3: water splash done
};

// Gatling gun work (game/obj15.cpp `cObjGatling`): a mounted gun the player (or `ride`) fires
// at `target`; three cEmHit hit boxes take the damage, `eat` is its effect collision piece.
struct GatlingWork {
    u32 x00;              // 0x00
    int breakTimer;       // 0x04  frames of barrel spin-down after the break
    u8 pad_8[2];
    s16 cnt;              // 0x0A  frames since firing started (shots every 3rd frame after 30)
    u8 fire;              // 0x0C  setFire: start firing
    u8 firing;            // 0x0D
    u8 ammo;              // 0x0E  shots left (setReload: 40)
    u8 breakMode;         // 0x0F  0: weapon damage 0xD / 0x12 breaks it
    f32 rotY;             // 0x10  base yaw
    f32 maxRot;           // 0x14  yaw step limit (pi)
    int targetTimer;      // 0x18  frames until `target` reverts to the player
    u8 seOn;              // 0x1C  spin SE playing
    u8 pad_1D[3];
    u32 seHandle;         // 0x20
    class cSat* eat;      // 0x24
    class cEmHit* hit[3]; // 0x28
    u8 pad_34[8];
    class cEm* ride;      // 0x3C  enemy riding the gun
    class cModel* target; // 0x40  aimed-at model (player when NULL)
};

// Helicopter missile work (game/objMissile.cpp `cObjMissile`): hangs from a parts of the
// helicopter (setParent), then flies toward `target` (setFire) and explodes (objMissileBomb).
struct MissileWork {
    u32 x00;              // 0x00
    int timer;            // 0x04  fire wait / flight frames
    int hitWait;          // 0x08  frames before the hit checks start
    cModel* parent;       // 0x0C
    int partsNo;          // 0x10
    int noNormalize;      // 0x14  keep the parent parts matrix as it is
    Vec target;           // 0x18
    class cEmHit* hit;    // 0x24
    u8 hasTarget;         // 0x28
    u8 pad_29[3];
    Vec spd;              // 0x2C
};

// Gondola work (game/objGondola.cpp `cObjGondola`): a cable car the player / partner / up to
// five enemies ride; five scenario collision quads follow it.
struct GondolaWork {
    u32 x00;              // 0x00
    int timer;            // 0x04  break: frames before the sub motion starts
    u8 ridePL;            // 0x08  player is on board (ckRide)
    u8 pad_9[3];
    int rideSUB;          // 0x0C  partner is on board
    int cnt;              // 0x10  counts down every frame
    Vec x14;              // 0x14
    class cEm* rideEm[5]; // 0x20
    class cSat* sat[5];   // 0x34
    class cSat* sat2[5];  // 0x48
    struct MotionWork* subWork;  // 0x5C  sub (vibration / break) motion work (setSubMotion)
    void* subMot;         // 0x60  vibration motion (setVib)
    void* breakMot;       // 0x64  break motion (R0_Break)
};

// Chain link work (game/obj1d.cpp): hangs between two parts of a parent model, fades out when
// the parent is lost.
struct ChainWork {
    u32 flags;            // 0x00  bit1: keep the parent parts matrices as they are (no axis normalize)
    int timer;            // 0x04  frames before the fade-out (LostWait)
    u8 pad_8[4];
    cModel* parent;       // 0x0C
    int parts1;           // 0x10
    int parts2;           // 0x14
    Vec ofs1;             // 0x18  offset in parts1
    Vec ofs2;             // 0x24  offset in parts2
    struct PenCloth* cloth;  // 0x30
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
    u8 pad_220[0x290 - 0x220];
    f32 motFrame;         // 0x290  MotionWork::seqFrame (objGondola R0_Up waits for frame 4105)
    u8 pad_294[0x2A8 - 0x294];
    struct MotionWork* motBlend;  // 0x2A8  MotionWork::blend (objGondola setVib: the sub motion work)
    u8 pad_2AC[4];
    u32 x2B0;             // 0x2B0  (obj18: parts matrices are only recomputed while 0)
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
        ChainWork chain;
        Obj00Work o0;
        Obj12Work o12;
        Obj18Work o18;
        Obj01Work o1;
        WepItemWork wepItem;
        GatlingWork gatling;
        MissileWork missile;
        GondolaWork gondola;
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
