#ifndef MODEL_H
#define MODEL_H

#include "types.h"
#include "vec.h"
#include "cManager.h"

// Coordinate base (game/model.cpp). Layout known only partially; pads keep offsets exact.
class cCoord : public cUnit {
public:
    Mtx mat;       // 0x0C
    Mtx mat2;      // 0x3C
    u32 x6C;       // 0x6C
    Vec pos;       // 0x70 world position
    u8 pad_7C[0x94 - 0x7C];
    Vec trans;     // 0x94 translation set by game code (player position for pPL)
    Vec rot;       // 0xA0 rotation, rot.y = facing angle (radians)
    u8 pad_AC[0xF4 - 0xAC];
};

// Model / model parts (game/model.cpp). Parts are cModel too, stride 0x1D8.
class cModel : public cCoord {
public:
    cModel* pParts;  // 0xF4 child parts list
    u8 pad_F8[0x100 - 0xF8];
    u8 id;           // 0x100
    u8 pad_101[0x1D8 - 0x101];

    cModel();
    cModel* getPartsPtr(int no);
};

#endif
