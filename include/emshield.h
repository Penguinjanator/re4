#ifndef EMSHIELD_H
#define EMSHIELD_H

#include "types.h"
#include "vec.h"
#include "em.h"

// shield enemy (game/emshield.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in emshield.o).
class cEmShield : public cEm {
public:
    virtual void move();
};

#endif
