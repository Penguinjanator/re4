#ifndef REF_ACCESS_H
#define REF_ACCESS_H

#include "types.h"
#include "vec.h"

class cObj;
class cSat;
class cModelInfo;
class cModel;
class cDataUnit;
class cEm;
class cEmHit;
struct ScePrim;
struct GlobalWork;
struct SYSTEM_SAVE_WORK;
struct YARARE_INFO;
class cCoord;
struct ShadowMng;
class cEmWrap;
class cEmRock;
class IdBinocular;
struct FocusAnimation;
struct AttachCamera;

// Reads and writes of a variable through a reference parameter. The original code reaches many globals and
// struct fields through inline helpers like these: the access is then a plain scalar load or store, which
// GCC 2.95 assumes may alias anything, so a following pointer load (pG, pPL) is not shared with the one
// before it, and a repeated read is not folded. Written directly, the same access compiles differently.
static inline void U8Set(u8& d, u8 v) { d = v; }
static inline void U16Set(u16& d, u16 v) { d = v; }
static inline void U32Set(u32& d, u32 v) { d = v; }
static inline void U16And(u16& d, u16 m) { d &= m; }
static inline void U32Or(u32& d, u32 m) { d |= m; }
static inline void U8SetI(u8& d, int v) { d = v; }
static inline void U16SetI(u16& d, int v) { d = v; }
static inline void U32Inc(u32& d) { d++; }
static inline void S8Set(s8& d, s8 v) { d = v; }
static inline void S16Set(s16& d, s16 v) { d = v; }
static inline void S32Set(s32& d, s32 v) { d = v; }
static inline void ISet(int& d, int v) { d = v; }
static inline void IntSet(int& d, int v) { d = v; }
static inline void FSetP(f32& d, f32 v) { d = v; }
static inline void PSet(void*& d, void* v) { d = v; }
static inline void PSet(cObj*& d, cObj* v) { d = v; }
static inline void PSet(cSat*& d, cSat* v) { d = v; }
static inline void PSet(cModelInfo*& d, cModelInfo* v) { d = v; }
static inline void PSet(cModel*& d, cModel* v) { d = v; }
static inline void PSet(cDataUnit*& d, cDataUnit* v) { d = v; }
static inline void PSet(cEm*& d, cEm* v) { d = v; }
static inline void PSet(cEmHit*& d, cEmHit* v) { d = v; }
static inline void PSet(AttachCamera*& d, AttachCamera* v) { d = v; }
static inline void PSet(ScePrim*& d, ScePrim* v) { d = v; }
static inline void PSet(YARARE_INFO*& d, YARARE_INFO* v) { d = v; }
static inline void PSet(cCoord*& d, cCoord* v) { d = v; }
static inline void PSet(ShadowMng*& d, ShadowMng* v) { d = v; }
static inline void PSet(cObj**& d, cObj** v) { d = v; }
static inline void PSet(cEmWrap*& d, cEmWrap* v) { d = v; }
static inline void PSet(cEmRock*& d, cEmRock* v) { d = v; }
static inline void PSet(IdBinocular*& d, IdBinocular* v) { d = v; }
static inline void PSet(FocusAnimation*& d, FocusAnimation* v) { d = v; }
// A pointer stored into a u32 field (sce_at).
static inline void PSet(u32& d, void* v) { d = (u32) v; }

static inline u16 U16Ref(u16& v) { return v; }
static inline u32 BitChk(u32& f, u32 b) { return f & b; }
static inline s32 S32Ref(s32& v) { return v; }
static inline SYSTEM_SAVE_WORK* SysRef(SYSTEM_SAVE_WORK*& p) { return p; }
static inline GlobalWork* GRef(GlobalWork*& p) { return p; }
static inline u32 U32Ref(u32& v) { return v; }
static inline f32 FCRef(const f32& v) { return v; }
static inline f32 FRef(f32& v) { return v; }
static inline f32 FGet(f32& d) { return d; }
static inline int IRef(int& v) { return v; }
static inline int IGet(int& d) { return d; }

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
