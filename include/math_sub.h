#ifndef MATH_SUB_H
#define MATH_SUB_H

#include "types.h"
#include "vec.h"
#include "db_log.h"

// game/math_sub.cpp
void RotMatrix(Mtx m, Vec* rot);
void RotMatrixZXY(Mtx m, Vec* rot);

// Debug-checked normalize: zero vectors are reported with the caller's file/line.
#define VECNormalize(src, dst)                                                          \
    if (0.0f == (src)->x && 0.0f == (src)->y && 0.0f == (src)->z) {                    \
        pLog->err(0, 0, "VECNormalize:[%s/%d]", __FILE__, __LINE__);                    \
        (dst)->x = (dst)->y = (dst)->z = 0.0f;                                          \
    } else                                                                              \
        PSVECNormalize(src, dst)

#endif
