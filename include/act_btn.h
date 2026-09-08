#ifndef ACT_BTN_H
#define ACT_BTN_H

#include "types.h"

// Action button prompt manager (game/act_btn.cpp `ActBtn`, 0x104 bytes). Layout opaque; only the
// entry point the player units use is declared.
class cActionButton {
public:
    u8 pad_0[0x104];

    // set(kind, slot, a, b, flags, type, c, d): pulls a work, fills it and adds the prim
    void set(int kind, int slot, int a, int b, int flags, int type, int c, int d);
};

extern cActionButton ActBtn;

#endif
