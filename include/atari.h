#ifndef ATARI_H
#define ATARI_H

#include "types.h"
#include "vec.h"
#include "db_log.h"

class cModel;

// One scenario collision piece (game/atari.cpp), returned by cSatMgr::create. Opaque apart
// from the flag byte the owners toggle (emobj setSatMain / clrSat: bit2 = active).
class cSat {
public:
    u8 pad_0[0x2A];
    s8 flags;        // 0x2A  bit2: piece takes part in the collision checks (signed: `&= ~4` is a word rlwinm)
    u8 pad_2B;

    void setCoord(Vec* pos, Vec* rot);
};

// Scenario collision manager (game/atari.cpp). Only the entry points used by the effect
// units are declared; the layout is opaque (sizeof 0x38).
class cSatMgr {
public:
    u8 pad_0[4];
    u32 x4;          // 0x04  (pl_debug satMakeTest prints it)
    u8 pad_8[0x38 - 0x8];

    // Runtime scenario piece from a polygon list (createFloorSat / createBoxSat / createSat by
    // flag bits 0x200 / 0x100); returns the registered piece or NULL.
    cSat* create(Vec* pos, Vec* rot, Vec* poly, int n, int flag, f32 h);
    // Piece from prebuilt collision data (obj15 cObjGatling::setEat: EatMgr.create(data, 0, &pos, &rot, type)).
    cSat* create(void* data, int flag, Vec* pos, Vec* rot, u8 type);
    int destroy(void* sat);
    // Ray from `top` down to `bottom`; returns the hit flags (bit2: no floor), hit point in `hit`.
    int hitCheck2(Vec* top, Vec* bottom, Vec* hit, u32* attr, int flag, int x);
    // Line segment `a`-`b` against the scenario; hit point and normal out. Returns 0 when nothing was hit.
    int hitCheck(Vec* a, Vec* b, Vec* hit, Vec* nrm, int x, int y);
    // Floor height under `pos`, searching `up` above and `down` below it.
    f32 getFloor(Vec* pos, f32 up, f32 down, u32* attr, int flag);
    // Sphere of radius `r` moving from `a` to `b` against the scenario; `b` is pushed out of the
    // polygons (cLight::hitAdjust). Returns 1 when the sphere was adjusted.
    int polySphereCk(Vec* a, Vec* b, f32 r, int x, int y, int flag);
    // Debug draw of the collision polygons (t_option "SCROLL VIEW").
    void disp(int flag);
    // Model against the scenario (obj00: `SatMgr.check(this, 0)`).
    int check(cModel* m, int flag);
    // Sphere of radius `r` moving from `oldPos` to `pos`; `pos` is pushed out of the polygons and
    // the hit normal goes to `nrm` (zero when nothing was hit). obj01 grenade bounce.
    int adjust(Vec* nrm, Vec* oldPos, Vec* pos, f32 r, int flag, int x);
};

extern cSatMgr SatMgr;

// Effect collision manager (game/atari.cpp `EatMgr`, 0x260 bytes; has its own vtable).
class AtEffInfo;
class cEatMgr : public cSatMgr {
public:
    u8 pad_38[0x260 - 0x38];

    AtEffInfo* getEffInfo(int type);  // game/atari.cpp
};

extern cEatMgr EatMgr;

extern "C" {
// Effect type of a hitCheck attribute word (game/at_sub.cpp).
int EatGetEffectType(u32 attr);
}

#line 8 "D:/Bio4/Prog/atari.h"

// 16 one-bit flags with range-checked access (game/atari.cpp).
class cFlag {
public:
    u16 flags;

    void set(u32 stat) {
        if (stat > 15) {
            pLog->err(0, 0, "cFlag.set() arg stat OVER FLOW %d", stat);
            return;
        }
        flags |= 1 << stat;
    }
    void clr(u32 stat) {
        if (stat > 15) {
            dbgAssert(__FILE__, __LINE__);
            return;
        }
        flags &= ~(1 << stat);
    }
    int check(u32 stat) {
        if (stat > 15) {
            pLog->err(0, 0, "cFlag.set() arg stat OVER FLOW %d", stat);
            return 0;
        }
        return flags & (1 << stat);
    }
};

#endif
