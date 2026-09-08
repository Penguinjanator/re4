#ifndef ATARIINFO_H
#define ATARIINFO_H

#include "types.h"
#include "vec.h"

// Character collision info (game/atariInfo.cpp), 0x4C bytes; embedded in cPlayer at 0x2B4.
// Only the flag word the pl_* units touch is named. The flag helpers are parameterless in-class
// inlines on purpose: the original accesses go `addi rX,this,0x2B4; lhz 0x1A(rX); andi. 0xFCFF`
// (address computed once, used by the load and the store; the mask folded to 16 bits). A direct
// member access folds the address into one displacement; an inline taking the bit as a parameter
// gives `rlwinm` instead of `andi.`.
class cAtariInfo {
public:
    u8 pad_0[0x1A];
    u16 flags;       // 0x1A  bits 8-9 (0x300): collide with enemies (mahoThrough clears them)
    u8 pad_1C[0x4C - 0x1C];

    cAtariInfo();
    void init0();
    int init(int a, int b);
    void throughOn() { flags &= ~0x300; }   // pass through enemies (mahoThroughOn)
    void throughOff() { flags |= 0x300; }
};

#endif
