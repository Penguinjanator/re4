#ifndef SSCRN_H
#define SSCRN_H

#include "types.h"

// Sub screen (inventory / map / files) front end, game/sscrn.cpp. Layout still opaque.
struct SubScreenWork {
    u8 pad_0[0x374];
};

extern SubScreenWork SubScreenWk;

extern "C" {
void SubScreenExitCore(SubScreenWork* wk);
}

#endif
