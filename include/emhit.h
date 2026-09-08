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

// Attack parameters handed to EmAtkSetDamagePL (obj15 Obj15_atk_info_tbl: {100.0, 8, 600, 0, 10, 0}).
struct EmAtkInfo {
    f32 range;   // 0x00
    int type;    // 0x04
    u16 dmg;     // 0x08
    u16 x0A;     // 0x0A
    u16 x0C;     // 0x0C
    u16 x0E;     // 0x0E
};

extern "C" {
cEmHit* SetEmHit(void* bin, void* tpl, Vec* pos, Vec* rot, int flag);
void YarareInit(cEmHit* em, f32 x, f32 y, f32 z, f32 rx, f32 rz, int a, int b);         // at_mod.cpp
void YarareInitCube(cEmHit* em, int a, int b, f32 x, f32 y, f32 z, f32 rx, f32 rz, f32 h);  // at_mod.cpp
int EmGetDmPos(cEm* em, Vec* pos, Vec* dir);                                     // em_sub.cpp
void EmDmBloodSet2(cEm* em, int a, int type, int b, int c, int d);               // em_sub.cpp
void EmPlBloodSet2(cModel* m, Vec* pos, int a, int b, int type);                 // em_sub.cpp
// Line `a`-`b` against the enemies: the hit enemy or NULL; hit point / normal and the scenario attribute out.
cEm* EmAtkLineHitCk(Vec* a, Vec* b, Vec* hit, Vec* nrm, u32* attr);              // em_sub.cpp
void EmAtkSetDamagePL(cEm* em, EmAtkInfo* info, Vec* a, Vec* b);                 // em_sub.cpp
}

void PlSetDamage(int type, int dmg, int flag);                                   // em_sub.cpp (C++ linkage)

#endif
