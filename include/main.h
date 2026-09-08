#ifndef MAIN_H
#define MAIN_H

#include "types.h"

// game/main.cpp globals that are not part of GlobalWork.

// Persistent system settings (pRK).
struct RK {
    u8 pad_0[0x11];
    u8 progressive;   // 0x11  progressive scan on
    u8 tv_mode_done;  // 0x12  TV mode prompt already handled
    u8 pad_13[0x40 - 0x13];
};

extern RK* pRK;

// Key/debug flags (main.cpp `Key`, 0xB8 bytes).
struct KeyWork {
    u8 pad_0[0x18];
    u64 flags_18;   // 0x18  bit 31 = skip TV-mode prompt
    u8 pad_20[0xB8 - 0x20];
};

extern KeyWork Key;

#endif
