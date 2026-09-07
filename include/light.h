#ifndef LIGHT_H
#define LIGHT_H

#include "types.h"
#include "vec.h"
#include "gx.h"
#include "cManager.h"
#include "db_log.h"

#line 14 "D:/Bio4/Prog/light.h"

// One light work (sizeof 0x1D4). Per-type modules (light01..light10) keep their state in `work`.
class cLight : public cUnit {
public:
    u8 pad_0C[0x20 - 0x0C];
    GXColor color;     // 0x20 base color
    f32 power;         // 0x24
    u8 pad_28[0x38 - 0x28];
    Vec normal;        // 0x38 direction
    u8 pad_44[0x78 - 0x44];
    u8 work[0x40];     // 0x78 per-light-type work area
    u8 pad_B8[0x138 - 0xB8];
    u8 x138;           // 0x138
    u8 pad_139[3];
    GXColor curColor;  // 0x13C color actually applied
    s16 x140;          // 0x140
    u8 pad_142[0x1D4 - 0x142];

    cLight();
    virtual ~cLight() {}
    void move();
    void setSpotTarget(Vec* target);
    void setSpotNormal(Vec* normal);
};

class cLight02 : public cLight {
public:
    cLight02();
};

class cLight06 : public cLight {
public:
    cLight06();
};

class cLight08 : public cLight {
public:
    cLight08();
    ~cLight08();
};

class cLightWork;

class cLightMgr : public cManager<cLight> {
public:
    u8 pad_34[0x180 - 0x34];
    void* x180;
    void* x184;
    void* x188;
    u8 pad_18C[0x208 - 0x18C];

    cLightMgr();
    virtual ~cLightMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree();
    virtual void memClear(cLight* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cLight* p, int id);

    cLight* getWork(u32 no) {
        if (no >= nArray) {
            dbgAssert(__FILE__, __LINE__);
        }
        return (cLight*)((u8*)pArray + size * no);
    }
    cLight* createNew() { return cManager<cLight>::create(); }
    cLight* createNo(int id, u32 no) { return cManager<cLight>::create(id, no); }

    cLight* create(cLightWork* w);
    cLight* create(int type);
    void update(int area_no, int camera_no);
};

extern cLightMgr LightMgr;

// per-type move handlers (light01.cpp .. light10.cpp)
void Light00_Move(cLight* l);
void Light01_Move(cLight* l);
void Light02_Move(cLight* l);
void Light03_Move(cLight* l);
void Light04_Move(cLight* l);
void Light05_Move(cLight* l);
void Light06_Move(cLight* l);
void Light07_Move(cLight* l);
void Light08_Move(cLight* l);
void Light10_Move(cLight* l);

#endif
