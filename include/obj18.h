#ifndef OBJ18_H
#define OBJ18_H

#include "types.h"
#include "vec.h"

class cObj;
class cModel;

// game/obj18.cpp: event costume / cloth objects (C++ linkage; obj18.cpp declares them itself).
cObj* SetObj18(void* bin, void* tpl, Vec* pos, Vec* rot, int type);
int DelObj18(cObj* pObj);
void OyaSetObj18(cObj* obj, cModel* oya, int partsNo);
int obj18GetOya(cModel** oya, cObj* pObj);
void Obj18CmfSet(cObj* pObj, u32 commonFlag);
u32 Obj18CmfGet(cObj* pObj);

#endif
