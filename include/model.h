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

// Model / model parts (game/model.cpp). Parts are cModel too, stride 0x1D8.
class cModel : public cCoord {
public:
    cModel* pParts;  // 0xF4 child parts list
    u8 pad_F8[0x100 - 0xF8];
    u8 id;           // 0x100
    u8 pad_101[0x1D8 - 0x101];

    cModel();
    virtual ~cModel() {}
    virtual void matUpdate();
    virtual void move();
    virtual void setNoSuspend(int on);

    cModel* getPartsPtr(int no);
    void partsMatCalc();
    void partsWorldCalc();
};

#endif
