#ifndef OBJ_H
#define OBJ_H

#include "types.h"
#include "cManager.h"
#include "model.h"

// Map object work (game/obj.cpp). Layout beyond cModel not yet needed.
class cObj : public cModel {
public:
    cObj();
    virtual ~cObj() {}
};

class cObjMgr : public cManager<cObj> {
public:
    u32 x34;

    cObjMgr();
    virtual ~cObjMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree();
    virtual void memClear(cObj* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cObj* p, int id);
};

extern cObjMgr ObjMgr;

#endif
