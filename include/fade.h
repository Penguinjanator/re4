#ifndef FADE_H
#define FADE_H

#include "types.h"
#include "gx.h"

// game/fade.cpp: up to 4 full-screen colour fades (C linkage).
struct FadeWork {
    GXColor start;  // 0x00
    GXColor end;    // 0x04
    GXColor cur;    // 0x08
    u8 pad_C[8];
    f32 z;          // 0x14
    u16 flags;      // 0x18  bit 0 = fading, bit 1 = keep drawing when done, bit 2 = late group (FadeControl(1))
    u16 x1A;        // 0x1A
    u32 time;       // 0x1C  frames
    u32 count;      // 0x20
};

extern FadeWork Fade[4];

extern "C" {
void FadeSet(int no, GXColor* start, GXColor* end, u32 time, u32 z, int late);
void FadeKillAll();
void FadeKill(int no);
void FadeInit();
void FadeControl(int late);
void fadeDraw(FadeWork* f);
}

#endif
