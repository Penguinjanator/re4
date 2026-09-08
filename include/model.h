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

// Light set of a model (game/lightInfo.cpp), embedded in cModel at 0x164 (0x74 bytes).
class cLightInfo {
public:
    u8 pad_0[0x74];

    int init2(int a, int b, const Vec* p0, const Vec* p1);
    void updateMatrix(cModel* m);
};

// Model / model parts (game/model.cpp). Parts are cModel too, stride 0x1D8.
class cModel : public cCoord {
public:
    cModel* pParts;  // 0xF4 child parts list
    u8 pad_F8[4];
    u8 xFC;          // 0xFC
    u8 pad_FD[3];
    u8 id;           // 0x100
    u8 type;         // 0x101 per-object sub type
    u8 nParts;       // 0x102
    u8 pad_103[0x12E - 0x103];
    u8 x12E;         // 0x12E
    u8 pad_12F[0x164 - 0x12F];
    cLightInfo lightInfo;  // 0x164 .. 0x1D8

    cModel();
    virtual ~cModel() {}
    virtual void matUpdate();
    virtual void move();
    virtual void setNoSuspend(int on);

    cModel* getPartsPtr(int no);
    int modelInit();
    void partsMatCalc();
    void partsWorldCalc();
    void setPos(Vec* pos);
    void setAng(Vec* ang);
};

#endif
