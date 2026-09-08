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

// Model data referenced by a bin (game/model.cpp `ModelData`); only the flag word is known.
struct ModelData {
    u8 pad_0[0x20];
    u32 flags;       // 0x20  bit30 (0x40000000): SmxGetFlag bit1
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
    u8 pad_0[0xC];
    ModelData* pData;    // 0x0C
    u8 pad_10[0x38 - 0x10];
    ModelBound bound;    // 0x38
    u8 pad_5C[0x8C - 0x5C];
    u8 color[4];         // 0x8C  RGBA (word store; 0xFF fill when the RGB part is 0)
    u8 color2[4];        // 0x90  second RGBA (0x93 = 0 or 0xFF)
    u8 pad_94[0xD6 - 0x94];
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
    u8 pad_0[0x54];
    u32 x54;         // 0x54  (scroll: SmxWork.x4)
    u8 pad_58[0x74 - 0x58];

    int init2(int a, int b, const Vec* p0, const Vec* p1, int c);  // every caller passes a 5th int (r8); the body ignores it
    void updateMatrix(cModel* m);
};

// Model / model parts (game/model.cpp). Parts are cModel too, stride 0x1D8.
class cModel : public cCoord {
public:
    cModel* pParts;  // 0xF4 child parts list
    u8 pad_F8[4];
    u8 xFC;          // 0xFC
    u8 xFD;          // 0xFD
    u8 xFE;          // 0xFE
    u8 xFF;          // 0xFF  (t_option clears FC..FF after a weapon change)
    u8 id;           // 0x100
    u8 type;         // 0x101 per-object sub type
    u8 nParts;       // 0x102
    u8 x103;         // 0x103  (scroll: 0x80 = SmxSetFlag bit3, 0xFF = off)
    u8 pad_104[0x12E - 0x104];
    u8 x12E;         // 0x12E  2 = scroll (Smd) object
    u8 x12F;         // 0x12F  scroll: SmxWork.type2 (3 by default)
    u8 pad_130[5];
    u8 x135;         // 0x135  scroll: SmxWork.x3
    u8 pad_136[0x15C - 0x136];
    cModelInfo* pInfo;     // 0x15C
    u8 pad_160[4];
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
};

#endif
