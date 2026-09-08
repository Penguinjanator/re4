#ifndef JOY_H
#define JOY_H

#include "types.h"

// Controller state (game/main.cpp `Joy[4]`, 0x268 bytes each). Only the fields the debug
// tools read are named; extend the pads, never rewrite.
struct JOY {
    s8 sx;    // 0x00  main stick x
    s8 sy;    // 0x01  main stick y
    s8 ssx;   // 0x02  sub stick x
    s8 ssy;   // 0x03  sub stick y
    u8 trigL; // 0x04  analog L
    u8 trigR; // 0x05  analog R
    u8 pad_6[2];
    s8 x8;    // 0x08  tv_mode: -3/-2 counts toward the progressive-mode prompt
    u8 pad_9[0x10 - 0x09];
    u32 on;   // 0x10  buttons currently held
    u32 trg;  // 0x14  buttons pressed this frame
    u8 pad_18[4];
    u32 rpt;  // 0x1C  buttons held, with auto-repeat
    u32 rel;  // 0x20  buttons released this frame
    u8 pad_24[0x268 - 0x24];
};

extern JOY Joy[4];

// game/debug.cpp: pad used by the debug tools (currently &Joy[0])
JOY* GetBugCheckController();

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

// Pad snapshot used by the debug tools: copies Joy[no] into the tool work at byte offset `ofs`.
// The destination is byte-pointer arithmetic on purpose: only then does GCC 2.95 treat the
// stores as possibly aliasing the work pointer and reload it after each copy, which is what the
// original code does (a struct assignment or memcpy(&p->joy[no], ...) hoists the reload).
#define JOY_COPY(work, ofs, no) memcpy((u8*) (work) + (ofs), &Joy[no], sizeof(JOY))

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
