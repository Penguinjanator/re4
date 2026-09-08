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
void TransMatrix(Mtx m, Vec* pos);
void ScaleMatrix(Mtx m, Vec* scale);

extern "C" {
// game/math_sub.cpp (C linkage)
void SetOrientationZX(Vec* z, Vec* x, Mtx m);
void SetOrientationZY(Vec* z, Vec* y, Mtx m);
void low_RotMatrix(Mtx m, Vec* rot);
void RotMatrixZXY(Mtx m, Vec* rot);
void Matrix2AxisAngle(Mtx m, Vec* rot);
void VecRadLimit(Vec* v);
f32 VecElevation(Vec* v);
void MtxRotAxisPosRad(Mtx m, Vec* axis, Vec* pos, f32 rad);
void VecLinearCombination(Vec* a, Vec* b, f32 s, f32 t, Vec* out);
void VecInternalDivisionAngle(Vec* a, Vec* b, f32 s, Vec* out, f32 t);
void VecLinearDecomposition(Vec* v, Vec* a, Vec* b, f32* s, f32* t);
f32 hermite(f32* p, f32* v, f32 t);
f32** malloc_2dim_array_f32(int n, int m);
void free_2dim_array_f32(int n, int m, f32** p);
int de_Boor_Cox(int n, f32* knot, int k, f32 t, f32* out);
f32 MtxNNLUDecomposition(int n, f32* a, int* ip);
f32 MtxNNInverse(int n, f32* m, f32* inv);
void MtxNNMultVecSR(int n, int m, f32* mtx, f32* v, f32* out);
void OrthographicProjection(Vec* p, Vec* out, Vec* dir, Vec* plane_p, Vec* plane_n);
f32 IPOW(f32 x, int n);
f32 SQRTF(f32 x);
f32 SINF(f32 x);
f32 COSF(f32 x);
f32 LIMIT_ANGLE(f32 x);
f32 VecAngle(Vec* a, Vec* b);
// game/sub2.cpp
f32 RootSumSquare3(Vec* v);
int GetScreenPos(Vec* pos, Vec* scr);
f32 GetDistance(Vec* a, Vec* b);
f32 GetDistance3(Vec* a, Vec* b);
void RotVector(Vec* src, Vec* rot, Vec* dst);
// Angle step from `ang` towards `target` seen from `pos`, clamped to +-limit.
f32 Muku(Vec* pos, Vec* target, f32 ang, f32 limit);
// Step from `ang` towards `target`, at most +-limit.
f32 Muku2(f32 ang, f32 target, f32 limit);
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
