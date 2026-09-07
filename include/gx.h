#ifndef GX_H
#define GX_H

// GX types needed by game code. dolphin/gx.h pulls in the CodeWarrior libc and cannot be
// compiled by ProDG/GCC; shares the dolphin/gx/GXStruct.h guard so both can coexist.

#include "types.h"

#ifndef _DOLPHIN_GX_GXSTRUCT_H_
#define _DOLPHIN_GX_GXSTRUCT_H_

typedef struct {
    u8 r, g, b, a;
} GXColor;

#endif

#endif
