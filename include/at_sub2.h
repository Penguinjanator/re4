#ifndef AT_SUB2_H
#define AT_SUB2_H

#include "types.h"

// Per-weapon hit effect ids (game/at_sub2.cpp). Layout partially known.
class AtEffInfo {
public:
    u8 pad_0[0x0C];
    u32 eff13[2];   // 0x0C weapon 0x13
    u32 eff16[2];   // 0x14 weapon 0x16
    u32 eff17[2];   // 0x1C weapon 0x17
    u32 effGun[2];  // 0x24 weapons 1..12, 0x0F, 0x11, 0x21
    u8 pad_2C[0x3C - 0x2C];
    u32 eff0D[2];   // 0x3C weapon 0x0D

    int getWepEff(int wepId, u32* eff1, u32* eff2);
};

#endif
