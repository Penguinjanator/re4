#ifndef EMHIT_H
#define EMHIT_H

#include "types.h"
#include "vec.h"
#include "em.h"

// Hit-only enemy work (game/emhit.cpp): a cEm that exists to receive weapon damage for an
// object (bell, ...). Only the entry points the object units use are declared.
class cEmHit : public cEm {
public:
    int ckDmgWeapon();                              // weapon id of the damage taken this frame, 0 = none
    void setParent(cModel* parent, int a, int b);   // 0x8010539C
};

extern "C" {
cEmHit* SetEmHit(void* bin, void* tpl, Vec* pos, Vec* rot, int flag);
void YarareInit(cEmHit* em, f32 x, f32 y, f32 z, f32 rx, f32 rz, int a, int b);         // at_mod.cpp
int EmGetDmPos(cEm* em, Vec* pos, Vec* dir);                                     // em_sub.cpp
void EmDmBloodSet2(cEm* em, int a, int type, int b, int c, int d);               // em_sub.cpp
}

#endif
