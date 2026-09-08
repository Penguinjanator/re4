#ifndef DATACTRL_H
#define DATACTRL_H

#include "types.h"

// Room data unit controller (game/datactrl.cpp, `DC`, 0xAA4 bytes). Only the members other units
// use are declared; the layout is still opaque.
class cDataCtrl {
public:
    u8 pad_0[0xAA4];

    u32 getAramFree(u32 size);
};
extern cDataCtrl DC;

#endif
