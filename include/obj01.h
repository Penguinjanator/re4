#ifndef OBJ01_H
#define OBJ01_H

#include "types.h"
#include "vec.h"
#include "obj.h"

// game/obj01.cpp: the thrown flame bottle / dynamite (the rooms throw them from Ganado positions).
cObj* SetObj01(void* bin, void* tpl, Vec* pos, Vec* rot, Vec* v, f32 a, f32 b, int c, int d);
void Obj01SetEst(cObj* obj, int eff, int est, u32 action, int eff2, int est2, int f, int g, int h, int i);

#endif
