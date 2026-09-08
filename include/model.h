#ifndef MODEL_H
#define MODEL_H

#include "types.h"
#include "vec.h"
#include "cManager.h"

// game/math_sub.cpp (C++ linkage; math_sub.h declares them too)
void RotMatrix(Mtx m, Vec* rot);
void TransMatrix(Mtx m, Vec* pos);
void ScaleMatrix(Mtx m, Vec* scale);

// Coordinate base (game/model.cpp). Layout known only partially; pads keep offsets exact.
class cCoord : public cUnit {
public:
    Mtx mat;        // 0x0C local matrix (rot * trans * scale)
    Mtx worldMat;   // 0x3C
    cCoord* pParent;  // 0x6C  parent coord (parts: the model; pl_ashley concatenates its mat)
    Vec worldPos;   // 0x70
    Vec oldWorldPos;  // 0x7C  worldPos of the previous frame (cAtariInfo::getSpeedVector)
    Vec x88;        // 0x88  (pl_ashley moveBust: GetDistance3 from worldPos)
    Vec pos;        // 0x94
    Vec rot;        // 0xA0
    Vec scale;      // 0xAC
    Vec prevScale;  // 0xB8  scale before MotionHokan rescaled it (blend: interpolated scale)
    Mtx prevMat;    // 0xC4  worldMat of the previous motion (MotionHokan interpolates from it)

    // In-class (eff_sys inlines the constructor into g_EffParentWorld's static initialiser and
    // owns the first `_vt.6cCoord` copy together with the out-of-line ~cCoord/matUpdate bodies).
    cCoord() {
        be_flag = 1;
        PSMTXIdentity(mat);
        PSMTXIdentity(worldMat);
        pParent = NULL;
        scale.x = 1.0f;
        scale.y = 1.0f;
        scale.z = 1.0f;
        prevScale.x = 1.0f;
        prevScale.y = 1.0f;
        prevScale.z = 1.0f;
    }
    virtual ~cCoord() {}
    virtual void matUpdate() {
        RotMatrix(worldMat, &rot);
        TransMatrix(worldMat, &pos);
        ScaleMatrix(worldMat, &scale);
        PSMTXCopy(worldMat, mat);
    }
};

class cModel;
class cLight;

// One primitive part of a ModelData (dbmodule DrawObjWireframe): 0x20 header, then the GX-style stream.
struct ModelPart {
    u8 pad_0[0x18];
    u32 size;        // 0x18  byte length of the primitive stream following the header
    u32 nPoly;       // 0x1C  polygon count (debug statistics)
};

// Model data referenced by a bin (game/model.cpp `ModelData`); only the flag word is known.
struct ModelData {
    u8 pad_0[0xC];
    void* pClr;      // 0x0C  vertex colour array (GX_VA_CLR0, RGBA8; used when flags bit31 is set)
    void* pTex;      // 0x10  texture coordinate array (GX_VA_TEX0)
    u8 pad_14[4];
    u8 x18;          // 0x18  (mirror: 1 with x19 == 1 and x2A <= 0xFF selects the original vertex arrays)
    u8 x19;          // 0x19
    u16 nParts;      // 0x1A  primitive part count (dbmodule DrawObjWireframe)
    struct ModelPart* pParts;  // 0x1C  first part header (0x20 bytes + primitive stream)
    u32 flags;       // 0x20  bit31: s16 tex coords (frac 8), bit30 (0x40000000): SmxGetFlag bit1
    u8 pad_24[4];
    u8 shift;        // 0x28  vertex fixed-point shift (dbmodule: scale = 1 / (1 << shift))
    u8 pad_29;
    u16 x2A;         // 0x2A
    u32 shapeOfs;    // 0x2C  offset of the shape (vertex delta) table (shape.cpp)
    void* vtxOrig;   // 0x30  original vertex positions (shape.cpp ResetShape source)
    void* nrmOrig;   // 0x34  original vertex normals
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

// Per-model info block (game/model.cpp `cModelInfo`, at cModel+0x15C), a cUnit managed by
// ModInfoMgr (be_flag 0x00: bit1 has shape animation (shape.cpp), 0x40 pl_leon eye; next 0x04;
// vptr 0x08). Partial layout.
class cModelInfo : public cUnit {
public:
    ModelData* pData;    // 0x0C
    void* pTpl;          // 0x10  texture palette of the model (eff_sys RoomEfmRegist)
    cModelInfo* pNext;   // 0x14  next parts info
    u8 pad_18[0x38 - 0x18];
    ModelBound bound;    // 0x38
    f32 x5C;             // 0x5C  (pl_leon setModel: face info zeroes 0x5C/0x70/0x84)
    u8 pad_60[0x70 - 0x60];
    f32 x70;             // 0x70
    u8 pad_74[0x84 - 0x74];
    f32 x84;             // 0x84
    u8 pad_88[4];
    u8 color[4];         // 0x8C  RGBA (word store; 0xFF fill when the RGB part is 0)
    u8 color2[4];        // 0x90  second RGBA (0x93 = 0 or 0xFF)
    void* pPosBuf[2];    // 0x94  double-buffered vertex position arrays (pG->vtx_buf_no selects)
    void* pNrmBuf[2];    // 0x9C  double-buffered vertex normal arrays
    ShapeData* pShape;   // 0xA4  current shape animation, NULL when none (shape.cpp)
    ShapeKey shape[5];   // 0xA8  blended shapes
    u32 shapeFlags;      // 0xD0  1: loop, 2: hold last frame, 4: reverse, 8: x100 weights
    s16 shapeFrame;      // 0xD4
    u8 xD6;              // 0xD6  previous color[3]
    u8 pad_D7[0xDC - 0xD7];
    u16 flagsDC;         // 0xDC  bit0: has uv scroll, bit2: texBlendTbl set
    u16 blendRatio;      // 0xDE  (TexRender: 0xFF while rendered to texture)
    u8 pad_E0;
    u8 blendType;        // 0xE1
    u8 pad_E2[2];
    void* texBlendTbl;   // 0xE4  (TexRender: 6-byte table {1, 0, ?, ?, 0xF7, tex id})
    u8 pad_E8[0xF0 - 0xE8];
    f32 uvScrollU;       // 0xF0
    f32 uvScrollV;       // 0xF4

    void addTplAddr(void* tpl);
    void setTexBlendTbl(void* tbl);
    void resetTexBlendTbl();
    void setBlendRatio(u16 ratio);
    void setBlendType(u8 type);
};

// Light set of a model (game/lightInfo.cpp), embedded in cModel at 0x164 (0x74 bytes).
class cLightInfo {
public:
    Mtx mat;         // 0x00  light space matrix (lightHitCheckBBox transforms the light into it)
    cLight* pLight[8];        // 0x30  lights applied to the model (cLightMgr::setModel2 / setCloth)
    u8 x50;          // 0x50  cLight::xF kind mask the model accepts (0x41: parent lights only)
    u8 x51;          // 0x51  bits 0-1: 2 = follow the model matrix (obj04: updateMatrix each frame); hit check shape (0 cylinder, 1/3 sphere, 2 box)
    s8 x52;          // 0x52  parts index + 1 the light origin follows (getPos), 0 = model
    u8 pad_53;
    u32 x54;         // 0x54  (scroll: SmxWork.x4); bit i: light i never applies (setModel2)
    u8 pad_58[0x64 - 0x58];
    Vec size;        // 0x64  hit check size: x radius, y half height (cylinder), xyz box half size
    u8 pad_70[0x74 - 0x70];

    int init2(int a, int b, const Vec* p0, const Vec* p1, int c);  // every caller passes a 5th int (r8); the body ignores it
    void updateMatrix(cModel* m);
    u32 getLightNum();
    cModel* getPos(cModel* m, Vec* out);  // light origin of `m` (the parts x52 - 1 selects); returns the coord it belongs to
};

// Model / model parts (game/model.cpp). Parts are cModel too, stride 0x1D8.
class cModel : public cCoord {
public:
    cModel* pParts;  // 0xF4 child parts list
    u32 serial;      // 0xF8  identity check for parent links (obj04: parent->serial == work.parentSerial)
    union {
        u32 stat;    // 0xFC  the four status bytes as one word (obj14 ckBreak: word compares)
        struct {
            u8 xFC;  // 0xFC  routine / state
            u8 xFD;  // 0xFD  routine index (move table)
            u8 xFE;  // 0xFE  step
            u8 xFF;  // 0xFF  (t_option clears FC..FF after a weapon change)
        };
    };
    u8 id;           // 0x100
    u8 type;         // 0x101 per-object sub type
    u8 nParts;       // 0x102
    u8 x103;         // 0x103  (scroll: 0x80 = SmxSetFlag bit3, 0xFF = off)
    Vec speed;       // 0x104
    Vec oldPos;      // 0x110  position before the speed was added (obj04 collision segment)
    Vec wallNrm;     // 0x11C  normal of the wall the scenario check pushed the model out of (atari scrAtCheckSphere), zero when none
    union {
        struct {
            Vec* pFloorNrm;  // 0x128  player: floor normal the shoulder camera tilts with (cam_qfps setPlayerLocation)
            u8 x12C;         // 0x12C  (TexRenderModSet sets 2)
            u8 x12D;         // 0x12D  (pl_leon setModel sets 1)
            u8 x12E;         // 0x12E  2 = scroll (Smd) object
            u8 x12F;         // 0x12F  scroll: SmxWork.type2 (3 by default)
            void* pCldShMd;  // 0x130  (db_work "pCldShMd")
            u8 shdCol;       // 0x134  (db_work "SHD COL")
            u8 x135;         // 0x135  scroll: SmxWork.x3, db_work "CullMode"
            u8 x136;         // 0x136  TexRender: 2 while rendered to texture, 0 after
            u8 x137;         // 0x137  TexRender: 0x10
            u8 x138;         // 0x138  TexRender: 0x90
            u8 x139;         // 0x139  mirror: 0xFF; trans_lit adds it to the ambient colour
            u8 x13A;         // 0x13A  mirror: 0xFF; trans_lit adds it to the ambient colour
            u8 x13B;         // 0x13B  mirror: 0xFF; trans_lit adds it to the ambient colour
            u8 pad_13C[0x150 - 0x13C];
        };
        // Effect model parts physics (obj05 cObj05::move runs its parts as loose particles).
        struct {
            int efmStat;     // 0x128  0 waiting, 1 flying, 2 at rest
            Vec efmSpd;      // 0x12C
            Vec efmRotSpd;   // 0x138
        };
    };
    // 0x150..0x15C: pendulum parts treat these three words as a Vec (obj14 adds the hit impulse
    // to parts 1/2 here); the object itself keeps its alpha at 0x154.
    f32 x150;              // 0x150
    f32 alpha;             // 0x154  0..1 (obj04: work color a / 255)
    f32 x158;              // 0x158
    cModelInfo* pInfo;     // 0x15C
    cModelInfo* pShMdInfo; // 0x160  (db_work "pShMdIfo")
    cLightInfo lightInfo;  // 0x164 .. 0x1D8

    cModel();
    virtual ~cModel() {}
    virtual void matUpdate();
    virtual void move();
    virtual void setNoSuspend(int on);

    cModel* getPartsPtr(int no);
    int modelInit(void* bin, void* tpl);  // returns the cModelInfo* (pl_leon range-checks it)
    void addModel(cModelInfo* info);
    void deleteModelInfo(cModelInfo* info);
    void partsMatCalc();
    void partsWorldCalc();
    void setPos(Vec* pos);
    void setAng(Vec* ang);
    void updateOldPos();   // oldPos = pos for the model and its parts (emMove)
    void push();   // pl_sub PlChangeData
    void drawAllBoundingBox(cModelInfo* info);
    void debugSkeletonDisp();
    void error();  // too many lights: flags the model and logs it
    // MotionSetCore(this, &motion (0x1D8), data, a, b, c, d) / MotionMove(this, 0)
    void motionSet(void* data, int a, int b, int c, int d);  // void: a following call then keeps its arg li`s ranked below the `this` copy (pl_knife down00)
    int motionMove();
    int isTrans();  // be_flag bit1 (visible) and be_flag != 0 (objWep / objRocket)
    // Hang parts 0 on `parent` at pos / rot (objRocket loadRocket); the 4-argument form
    // selects parts `partsNo` of the parent (-1: the parent itself).
    void setParent(cModel* parent, Vec* pos, Vec* rot);
    void setParent(cModel* parent, int partsNo, Vec* pos, Vec* rot);
    void moveDataAddr(int ofs);   // model data moved by `ofs` bytes (block.cpp memory compaction)

    // The managers the models allocate from (game/model.cpp, .sdata: &ModInfoMgr / &PartsMgr;
    // sscrn SubScreenExitCore restores them after the sub screen swapped the area out).
    static class cModInfoMgr* mm;
    static class cPartsMgr* pm;
};

// Model info pool (game/model.cpp `ModInfoMgr`, 0x34 bytes): a cManager<cModelInfo>; the
// player units call the inline cManager::destroy on it (pl_ashley setRightHand/setLeftHand).
class cModInfoMgr : public cManager<cModelInfo> {
public:
    cModInfoMgr();
    virtual ~cModInfoMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(cModelInfo* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cModelInfo* p, u32 id);

    cModelInfo* create(void* bin, void* tpl);
};

extern cModInfoMgr ModInfoMgr;

// Parts pool (game/model.cpp `PartsMgr`, 0x34 bytes): a cManager<cParts>; layout opaque here.
class cPartsMgr;
extern cPartsMgr PartsMgr;

// game/model.cpp (C linkage): parts `no` of a parts list (NULL when out of range).
extern "C" cModel* GetPartsAddr(cModel* parts, int no);

#endif
