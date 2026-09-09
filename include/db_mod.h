#ifndef DB_MOD_H
#define DB_MOD_H

#include "types.h"
#include "model.h"

// Model viewer of the debug tools (Tools/t_esp db_mod.cpp). The entry points have C linkage in the .sym.
// The data views below are what t_mv addresses; db_mod.cpp owns the real definitions.

// One viewer slot (0x2754 bytes): the model pointer at 0x04.
struct DbModSlot {
    u32 x0;
    cModel* pModel;  // 0x04
    u8 pad_8[0x2754 - 8];
};

// Viewer state (pointer at Tools .bss 0x13AAE8); 0x3449 is the "model set loaded" byte t_mv tests.
struct DbModState {
    u8 pad_0[0x3449];
    u8 loaded;  // 0x3449
};

extern DbModSlot dbModSlot[64];  // Tools .bss 0xE8
extern DbModState* pDbModState;  // Tools .bss 0x13AAE8

extern "C" {
void dbModelInit();
void dbModelQuit();
int dbModel(int mode);
void dbModMotionMove();
u32 dbModGetViewFlag();
}

#endif
