#ifndef OBJ20_H
#define OBJ20_H

#include "types.h"
#include "vec.h"
#include "obj.h"

// obj20 (game/obj20.cpp): an invisible collision obstacle attached to a parent object. C linkage.

extern "C" {
// Creates the obstacle on `parent` (parts partsNo + ofs for type 0, parent origin + ofs for type 1),
// collision radius rad / height h, priority level 1, not drawn.
cObj* SetObaModel(cObj* parent, int partsNo, Vec* ofs, f32 rad, f32 h, u8 type);
}

#endif
