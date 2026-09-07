#ifndef JOY_H
#define JOY_H

#include "types.h"

// Controller state (game/main.cpp `Joy[4]`, 0x268 bytes each). Only the fields the debug
// tools read are named; extend the pads, never rewrite.
struct JOY {
    u8 pad_0[0x10];
    u32 on;   // 0x10  buttons currently held
    u32 trg;  // 0x14  buttons pressed this frame
    u8 pad_18[4];
    u32 rpt;  // 0x1C  buttons held, with auto-repeat
    u32 rel;  // 0x20  buttons released this frame
    u8 pad_24[0x268 - 0x24];
};

extern JOY Joy[4];

// button bits
#define JOY_LEFT   0x0001
#define JOY_RIGHT  0x0002
#define JOY_DOWN   0x0004
#define JOY_UP     0x0008
#define JOY_Z      0x0010
#define JOY_R      0x0020
#define JOY_L      0x0040
#define JOY_A      0x0100
#define JOY_B      0x0200
#define JOY_X      0x0400
#define JOY_Y      0x0800
#define JOY_START  0x1000

#endif
