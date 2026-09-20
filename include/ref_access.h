#ifndef REF_ACCESS_H
#define REF_ACCESS_H

#include "types.h"

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
static inline void PSet(ScePrim*& d, ScePrim* v) { d = v; }

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

#endif
