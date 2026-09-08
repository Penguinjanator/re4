#ifndef TRANS_OT_H
#define TRANS_OT_H

#include "types.h"
#include "vec.h"

// game/trans_ot.cpp: ordering-table draw list.
extern "C" {
// Queue `func` in ordering table `ot`; `no` is the slot (clamped), `pos`/`radius` do a frustum cull when given.
int AddOtDirect(int ot, void* data, void (*func)(), u32 no, u16 flag, Vec* pos, f32 radius);
}

#endif
