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
    u8 pad_0[0xC];
    f32 rectX;       // 0x0C  push rectangle half size along local X (pl_push)
    f32 rectZ;       // 0x10  along local Z
    u8 pad_14[4];
    u16 x18;         // 0x18  (pl_dmg Pl_R0_Die sets 4)
    u16 flags;       // 0x1A  bits 8-9 (0x300): collide with enemies (mahoThrough clears them)
    u8 pad_1C[0x4C - 0x1C];

    cAtariInfo();
    // init0(type, flags, prio, pos x/y/z, rectX, rectZ, h, w): stores everything, flags |= 0x300.
    void init0(int type, int flags, int prio, f32 x, f32 y, f32 z, f32 rx, f32 rz, f32 h, f32 w);
    // init(type, prio, flags, ...) = init0(type, flags, prio, ...); flags |= 1  (r5/r6 swapped)
    void init(int type, int prio, int flags, f32 x, f32 y, f32 z, f32 rx, f32 rz, f32 h, f32 w);
    void setPriority(int prio);  // flags bits 3-4
    void move();
    void throughOn() { flags &= ~0x300; }   // pass through enemies (mahoThroughOn)
    void throughOff() { flags |= 0x300; }
    void clrFlag100() { flags &= ~0x100; }  // obj20 SetObaModel
};

#endif
