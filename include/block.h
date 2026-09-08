#ifndef BLOCK_H
#define BLOCK_H

#include "types.h"

// Room block (streamed model data) manager (game/block.cpp, 0x80 bytes). Layout opaque.
class cBlock {
public:
    u8 pad_0[0x74];
    int allDisp;  // 0x74  1 = every block displayed (t_option "BLOCK ALL DISP")
    u8 pad_78[0x80 - 0x78];

    void dispAllBlock(int on);
};

extern cBlock Block;

#endif
