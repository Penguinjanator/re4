#ifndef AT_MOD_H
#define AT_MOD_H

#include "types.h"
#include "vec.h"
#include "em.h"

// Character-to-character collision (game/at_mod.cpp): pushes the cAtariInfo rectangles /
// cylinders of the enemies and objects apart, and line checks against their hit boxes.
extern "C" {
void EmAtCheck(cEm* em);
void __em_at_core(cEm* pMod, cEm* pMod2);
int At_em_rect_rect_ck(cEm* pMod, cEm* pMod2);
int em_rect2_ck_sub(cEm* pMod, cEm* pMod2);
int At_em_sphere_rect_ck(cEm* sph, cEm* rect);
int At_em_sphere_sphere_ck(cEm* pMod, cEm* pMod2);
// Segment a-b against every enemy / object with collision (flag bit0: rectangles as cubes,
// bit1: cylinders, bit2: the player too). The nearest hit point / normal go to `hit` / `nrm`.
int EmHitCheck(Vec* hit, Vec* nrm, Vec* pos0, Vec* pos1, int flag);
int ObjHitCheck(Vec* hit, Vec* nrm, Vec* pos0, Vec* pos1, int flag);
int ComnHitCheck(Vec* hit, Vec* nrm, cEm* m, Vec* pos0, Vec* pos1, int flag);
void DrawOba(cEm* m);
int ObaLineHitChk(cEm* m, cAtariInfo* info, Vec* pos10, Vec* pos11, Vec* hit, Vec* nrm);
}

#endif
