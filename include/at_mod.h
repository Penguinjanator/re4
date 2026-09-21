#ifndef AT_MOD_H
#define AT_MOD_H

#include "types.h"
#include "vec.h"
#include "em.h"

// Character-to-character collision (game/at_mod.cpp): pushes the cAtariInfo rectangles /
// cylinders of the enemies and objects apart, and line checks against their hit boxes.
extern "C" {
void EmAtCheck(cModel* em);
void __em_at_core(cModel* pMod, cModel* pMod2);
u32 At_em_rect_rect_ck(cModel* pMod, cModel* pMod2);
int em_rect2_ck_sub(cModel* pMod, cModel* pMod2);
u32 At_em_sphere_rect_ck(cModel* sph, cModel* rect);
u32 At_em_sphere_sphere_ck(cModel* pMod, cModel* pMod2);
// Segment a-b against every enemy / object with collision (flag bit0: rectangles as cubes,
// bit1: cylinders, bit2: the player too). The nearest hit point / normal go to `hit` / `nrm`.
BOOL EmHitCheck(Vec* hit, Vec* nrm, Vec* pos0, Vec* pos1, u32 flag);
int ObjHitCheck(Vec* hit, Vec* nrm, Vec* pos0, Vec* pos1, u32 flag);
int ComnHitCheck(Vec* hit, Vec* nrm, cModel* m, Vec* pos0, Vec* pos1, u32 flag);
void DrawOba(cModel* m);
BOOL ObaLineHitChk(cModel* m, cAtariInfo* info, const Vec& pos10, const Vec& pos11, Vec& hit, Vec& nrm);
}

#endif
