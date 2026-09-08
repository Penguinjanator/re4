#ifndef COCKPIT_H
#define COCKPIT_H

#include "types.h"

// HUD (game/cockpit.cpp), instance `Cckpt` (0xCC bytes). Only what other units call is declared.
class Cockpit {
public:
    u8 pad_0[0xCC];

    void lifeMeterDisp(int sw);
};

extern Cockpit Cckpt;

#endif
