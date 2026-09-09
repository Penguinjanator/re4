#ifndef T_PRIM_H
#define T_PRIM_H

#include "types.h"
#include "vec.h"
#include "gx.h"

// Debug primitive drawing environment (game/t_prim.cpp).
struct TprimRect {
    f32 x, y, w, h;
};

struct TprimView {
    TprimRect rect;
    f32 nearz;
    f32 farz;
};

void TprimInitEnv2D3D(TprimView* view, MtxPtr proj, MtxPtr view_mtx);
void TprimDraw3D(u32 blend);
void TprimSetBlend(u32 blend);
// The tool modules link the full t_prim (tools/t_prim.cpp); the DOL link dead-stripped these.
void TprimDraw2D(u32 blend);
void TprimDrawPolyFn(Vec* v, GXColor* col, u16 n);
void TprimDrawCursor(Vec* pos, GXColor* col, f32 z);
void TprimDrawMtxDirection(Mtx m, GXColor* fill, GXColor* line);

#endif
