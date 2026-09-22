#ifndef REF_ACCESS_H
#define REF_ACCESS_H

#include "types.h"
#include "vec.h"

// Reads and writes of a variable through a reference parameter. The original code reaches many globals and
// struct fields through inline helpers like these: the access is then a plain scalar load or store, which
// GCC 2.95 assumes may alias anything, so a following pointer load (pG, pPL) is not shared with the one
// before it, and a repeated read is not folded. Written directly, the same access compiles differently.
static inline void U8Set(u8& d, u8 v) { d = v; }
static inline void U8SetI(u8& d, int v) { d = v; }
static inline void S8Set(s8& d, s8 v) { d = v; }
static inline void ISet(int& d, int v) { d = v; }

static inline int IRef(int& v) { return v; }

// Fill a Vec (x, y, z) and return it: the element stores go through the vector's address, so the
// address pseudo is shared with a call that follows (`mr r4, rX`) (model, r202, r208, r219).
static inline Vec* VecSet(Vec* v, f32 x, f32 y, f32 z)
{
    v->x = x;
    v->y = y;
    v->z = z;
    return v;
}

#endif
