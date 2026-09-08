#ifndef ID_SYS_H
#define ID_SYS_H

#include "types.h"
#include "vec.h"

// Screen id (widget) unit (game/id_sys.cpp). Only the fields stage.cpp touches are named.
struct IdUnit {
    u8 flags;        // 0x00  0x08: visible
    u8 pad_1[0x6E - 0x01];
    u8 no;           // 0x6E  digit / frame number
    u8 pad_6F;
    Vec pos;         // 0x70
    u8 pad_7C[3];
    u8 flags_7F;     // 0x7F  0x02: position given in screen space
    u8 pad_80[8];
    Vec scr;         // 0x88  screen position
};

class IDSystem {
public:
    u8 pad_0[0x50];

    void kill(u8 id, u8 type);
    void set(void* data, u8 id, u8 type, u8 a, u8 b, u8 c);
    IdUnit* unitPtr(u8 id, u8 type);
};

extern IDSystem IdSys;

#endif
