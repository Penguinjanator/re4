#ifndef MODEL_H
#define MODEL_H

#include "types.h"
#include "vec.h"
#include "cManager.h"

// Coordinate base (game/model.cpp). Layout known only partially; pads keep offsets exact.
class cCoord : public cUnit {
public:
    Mtx mat;        // 0x0C local matrix (rot * trans * scale)
    Mtx worldMat;   // 0x3C
    u32 x6C;        // 0x6C
    Vec worldPos;   // 0x70
    u8 pad_7C[0x94 - 0x7C];
    Vec pos;        // 0x94
    Vec rot;        // 0xA0
    Vec scale;      // 0xAC
    u8 pad_B8[0xF4 - 0xB8];

    cCoord();
    virtual ~cCoord() {}
    virtual void matUpdate();
};

class cModel;

// One primitive part of a ModelData (dbmodule DrawObjWireframe): 0x20 header, then the GX-style stream.
struct ModelPart {
    u8 pad_0[0x18];
    u32 size;        // 0x18  byte length of the primitive stream following the header
    u32 nPoly;       // 0x1C  polygon count (debug statistics)
};

// Model data referenced by a bin (game/model.cpp `ModelData`); only the flag word is known.
struct ModelData {
    u8 pad_0[0x1A];
    u16 nParts;      // 0x1A  primitive part count (dbmodule DrawObjWireframe)
    struct ModelPart* pParts;  // 0x1C  first part header (0x20 bytes + primitive stream)
    u32 flags;       // 0x20  bit30 (0x40000000): SmxGetFlag bit1
    u8 pad_24[4];
    u8 shift;        // 0x28  vertex fixed-point shift (dbmodule: scale = 1 / (1 << shift))
    u8 pad_29[3];
    u32 shapeOfs;    // 0x2C  offset of the shape (vertex delta) table (shape.cpp)
    void* vtxOrig;   // 0x30  original vertex positions (shape.cpp ResetShape source)
    u8 pad_34[4];
    u16 nVtx;        // 0x38  vertex count (8 bytes each)
};

// Shape (morph) animation data referenced by cModelInfo::pShape (game/shape.cpp).
struct ShapeData {
    u16 nFrame;      // 0x00  frame count (low 14 bits)
    u8 num;          // 0x02  channel count
    // u8  idx[num]      0x03  shape table index per channel
    // u16 flags[num]    0x03 + num  bit2: active, bits 12-15: interpolation type
    // s32 table[num]    4-aligned after that, preceded by a marker word (< 0 once relocated)
};

// One active shape channel of a model (cModelInfo+0xA8, 5 entries).
struct ShapeKey {
    f32 rate;        // 0x00
    ShapeData* data; // 0x04
};

// Bounding volume of a model (cModelInfo+0x38).
struct ModelBound {
    Vec min;             // 0x00
    Vec center;          // 0x0C  light info origin (cLightInfo::init2 p0)
    Vec size;            // 0x18  (cLightInfo::init2 p1, copied field by field to the stack)
};

// Per-model info block (game/model.cpp `cModelInfo`, at cModel+0x15C). Partial layout.
class cModelInfo {
public:
    u32 flags;           // 0x00  bit1: has shape animation (shape.cpp)
    u8 pad_4[8];
    ModelData* pData;    // 0x0C
    u8 pad_10[4];
    cModelInfo* pNext;   // 0x14  next parts info
    u8 pad_18[0x38 - 0x18];
    ModelBound bound;    // 0x38
    u8 pad_5C[0x8C - 0x5C];
    u8 color[4];         // 0x8C  RGBA (word store; 0xFF fill when the RGB part is 0)
    u8 color2[4];        // 0x90  second RGBA (0x93 = 0 or 0xFF)
    u8 pad_94[0xA4 - 0x94];
    ShapeData* pShape;   // 0xA4  current shape animation, NULL when none (shape.cpp)
    ShapeKey shape[5];   // 0xA8  blended shapes
    u32 shapeFlags;      // 0xD0  1: loop, 2: hold last frame, 4: reverse, 8: x100 weights
    s16 shapeFrame;      // 0xD4
    u8 xD6;              // 0xD6  previous color[3]
    u8 pad_D7[0xDC - 0xD7];
    u16 flagsDC;         // 0xDC  bit0: has uv scroll
    u8 pad_DE[0xF0 - 0xDE];
    f32 uvScrollU;       // 0xF0
    f32 uvScrollV;       // 0xF4

    void addTplAddr(void* tpl);
};

// Light set of a model (game/lightInfo.cpp), embedded in cModel at 0x164 (0x74 bytes).
class cLightInfo {
public:
    u8 pad_0[0x51];
    u8 x51;          // 0x51  bits 0-1: 2 = follow the model matrix (obj04: updateMatrix each frame)
    u8 pad_52[2];
    u32 x54;         // 0x54  (scroll: SmxWork.x4)
    u8 pad_58[0x74 - 0x58];

    int init2(int a, int b, const Vec* p0, const Vec* p1, int c);  // every caller passes a 5th int (r8); the body ignores it
    void updateMatrix(cModel* m);
    u32 getLightNum();
};

// Model / model parts (game/model.cpp). Parts are cModel too, stride 0x1D8.
class cModel : public cCoord {
public:
    cModel* pParts;  // 0xF4 child parts list
    u32 serial;      // 0xF8  identity check for parent links (obj04: parent->serial == work.parentSerial)
    u8 xFC;          // 0xFC
    u8 xFD;          // 0xFD
    u8 xFE;          // 0xFE
    u8 xFF;          // 0xFF  (t_option clears FC..FF after a weapon change)
    u8 id;           // 0x100
    u8 type;         // 0x101 per-object sub type
    u8 nParts;       // 0x102
    u8 x103;         // 0x103  (scroll: 0x80 = SmxSetFlag bit3, 0xFF = off)
    Vec speed;       // 0x104
    Vec oldPos;      // 0x110  position before the speed was added (obj04 collision segment)
    u8 pad_11C[0x12E - 0x11C];
    u8 x12E;         // 0x12E  2 = scroll (Smd) object
    u8 x12F;         // 0x12F  scroll: SmxWork.type2 (3 by default)
    void* pCldShMd;  // 0x130  (db_work "pCldShMd")
    u8 shdCol;       // 0x134  (db_work "SHD COL")
    u8 x135;         // 0x135  scroll: SmxWork.x3, db_work "CullMode"
    u8 pad_136[0x154 - 0x136];
    f32 alpha;             // 0x154  0..1 (obj04: work color a / 255)
    u8 pad_158[4];
    cModelInfo* pInfo;     // 0x15C
    cModelInfo* pShMdInfo; // 0x160  (db_work "pShMdIfo")
    cLightInfo lightInfo;  // 0x164 .. 0x1D8

    cModel();
    virtual ~cModel() {}
    virtual void matUpdate();
    virtual void move();
    virtual void setNoSuspend(int on);

    cModel* getPartsPtr(int no);
    int modelInit(void* bin, void* tpl);
    void partsMatCalc();
    void partsWorldCalc();
    void setPos(Vec* pos);
    void setAng(Vec* ang);
    void drawAllBoundingBox(cModelInfo* info);
    void debugSkeletonDisp();
    // MotionSetCore(this, &motion (0x1D8), data, a, b, c, d) / MotionMove(this, 0)
    int motionSet(void* data, int a, int b, int c, int d);
    int motionMove();
};

#endif
