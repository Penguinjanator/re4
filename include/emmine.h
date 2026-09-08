#ifndef EMMINE_H
#define EMMINE_H

#include "types.h"
#include "vec.h"
#include "em.h"

// mine enemy (game/emmine.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in emmine.o).
class cEmMine : public cEm {
public:
    virtual void move();
};

#endif
