#ifndef MATH_SUB_H
#define MATH_SUB_H

#include "types.h"
#include "vec.h"
#include "db_log.h"

#ifndef PI
#define PI 3.1415927f
#endif

// Float abs as the original SDK header defines it: a volatile asm, which also acts as a
// scheduling barrier (loads after it are not hoisted above it).
static inline f32 fabsf(f32 x)
{
    f32 r;
    asm volatile("fabs %0,%1" : "=f"(r) : "f"(x));
    return r;
}

// game/math_sub.cpp
void RotMatrix(Mtx m, Vec* rot);
void RotMatrixZXY(Mtx m, Vec* rot);
void TransMatrix(Mtx m, Vec* pos);
void ScaleMatrix(Mtx m, Vec* scale);

extern "C" {
// game/math_sub.cpp (C linkage)
void low_RotMatrix(Mtx m, Vec* rot);
void Matrix2AxisAngle(Mtx m, Vec* rot);
f32 SQRTF(f32 x);
f32 SINF(f32 x);
f32 COSF(f32 x);
f32 LIMIT_ANGLE(f32 x);
f32 VecAngle(Vec* a, Vec* b);
// game/sub2.cpp
f32 RootSumSquare3(Vec* v);
// lib math
f32 sinf(f32 x);
f32 cosf(f32 x);
f32 atan2f(f32 y, f32 x);
}

// Debug-checked normalize: zero vectors are reported with the caller's file/line.
#define VECNormalize(src, dst)                                                          \
    if (0.0f == (src)->x && 0.0f == (src)->y && 0.0f == (src)->z) {                    \
        pLog->err(0, 0, "VECNormalize:[%s/%d]", __FILE__, __LINE__);                    \
        (dst)->x = (dst)->y = (dst)->z = 0.0f;                                          \
    } else                                                                              \
        PSVECNormalize(src, dst)

#endif
