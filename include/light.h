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
    u8 pad_0C[4];
    Vec pos;           // 0x10
    f32 x1C;           // 0x1C (esp11: sizeX * scale * 10)
    GXColor color;     // 0x20 base color
    f32 power;         // 0x24
    u8 pad_28[2];
    u8 attr;           // 0x2A  (db_work "ATTR")
    u8 pad_2B[0x38 - 0x2B];
    Vec normal;        // 0x38 direction
    u8 pad_44[0x78 - 0x44];
    u8 work[0x40];     // 0x78 per-light-type work area
    u8 pad_B8[0x138 - 0xB8];
    u8 x138;           // 0x138
    u8 pad_139[3];
    GXColor curColor;  // 0x13C color actually applied
    s16 x140;          // 0x140
    u8 pad_142[2];
    Vec curPos;        // 0x144  position actually applied (db_work draws a sphere of radius x1C here)
    u8 pad_150[0x1D4 - 0x150];

    cLight();
    virtual ~cLight() {}
    void move();
    void setSpotTarget(Vec* target);
    void setSpotNormal(Vec* normal);
};

class cLight01 : public cLight {
public:
    cLight01();
};

class cLight02 : public cLight {
public:
    cLight02();
};

class cLight06 : public cLight {
public:
    cLight06();
};

class cLight07 : public cLight {
public:
    cLight07();
};

class cLight08 : public cLight {
public:
    cLight08();
};

// Light path follower kept in cLight::work (game/lightPath.cpp). Opaque here.
class cLightPathData;
class cLightPath {
public:
    void setPath(cLightPathData* data, u8 no);
    int movePath();
};

class cLightWork;

// Environment block at cLightMgr+0x38 (returned by getEnvPtr). Only the depth-of-field
// fields filter01 reads are known.
struct cLightEnv {
    u8 pad_0[0x8];
    s32 x8;          // 0x08  gx_sub: 0 = the background colour has no rgb (alpha only)
    u8 pad_C[8];
    GXColor bgColor; // 0x14  background colour (gx_sub)
    u8 pad_18[0x28 - 0x18];
    s32 x28;   // 0x28  focus depth (screen z, 0..65535)
    u8 x2C;    // 0x2C
    u8 x2D;    // 0x2D  focus level (0 = depth of field off)
    u8 x2E;    // 0x2E  focus mode (0 near, 1 far)
};

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
    // range-checked variant returning NULL (db_work)
    cLight* getWorkPtr(u32 no) {
        if (no >= nArray) {
            return 0;
        }
        return (cLight*)((u8*)pArray + size * no);
    }
    cLight* createNew() { return cManager<cLight>::create(); }
    cLight* createNo(int id, u32 no) { return cManager<cLight>::create(id, no); }

    cLight* create(cLightWork* w);
    cLight* create(int type);
    cLight* create(int kind, int type, int no, int x);  // 0x8014E21C (esp11)
    void update(int area_no, int camera_no);
    cLightPathData* getPathPtr(u8 no);
    cLightEnv* getEnvPtr();  // 0x8014EFCC: &this->env (at +0x38)
    void setFog();           // 0x8014FAC8
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
