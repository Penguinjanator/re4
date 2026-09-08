#ifndef EMWEP_H
#define EMWEP_H

#include "types.h"
#include "vec.h"
#include "em.h"

// weapon enemy: dropped / thrown weapons (game/emwep.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in emwep.o).
class cEmWep : public cEm {
public:
    virtual void move();
};

#endif
