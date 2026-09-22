#ifndef OBJ01_H
#define OBJ01_H

#include "types.h"
#include "vec.h"
#include "obj.h"

// game/obj01.cpp: the thrown flame bottle / dynamite (the rooms throw them from Ganado positions).
cObj* SetObj01(void* bin, void* tpl, Vec* pos, Vec* rot, Vec* v, f32 a, f32 b, int c, int d);
void Obj01SetEst(cObj* pObj, u32 eff, u32 est, u32 action, u32 eff2, u32 est2, u32 eff3, u32 est3, u32 eff4, u32 est4);

#endif
