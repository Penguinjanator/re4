#ifndef MAP_OBJ_H
#define MAP_OBJ_H

#include "types.h"
#include "cManager.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/map_obj.h"

// Map object work (game/map_obj.cpp). Layout not yet established; only what the debug tools
// need to include this header (the file-name string its range check emits) is here.
class cMap : public cUnit {
public:
    cMap();
    virtual ~cMap();
    void move();
};

class cMapMgr : public cManager<cMap> {
public:
    cMapMgr();
    virtual ~cMapMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(cMap* p, u32 size);
    virtual int construct(cMap* p, u32 id);

    cMap* getWork(u32 no) {
        if (no >= nArray) {
            dbgAssert(__FILE__, __LINE__);
        }
        return (cMap*)((u8*)pArray + size * no);
    }

    void room();
    void move();
    void dispInfo();
};

extern cMapMgr MapMgr;

#endif
