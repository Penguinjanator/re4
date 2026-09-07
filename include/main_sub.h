#ifndef MAIN_SUB_H
#define MAIN_SUB_H

#include "types.h"

// Render target description (game/main.cpp `Screen`, 0x18 bytes; layout partially known).
struct ScreenInfo {
    f32 x;       // 0x00
    f32 y;       // 0x04
    f32 width;   // 0x08 (512.0f)
    f32 height;  // 0x0C (448.0f)
    u8 pad_10[0x18 - 0x10];
};

extern ScreenInfo Screen;

// game/main_sub.cpp
int Render_checkBlurPermission();

#endif
