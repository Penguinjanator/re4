#ifndef EMDOOR_H
#define EMDOOR_H

#include "types.h"
#include "vec.h"
#include "em.h"

// door enemy: the room doors (game/emdoor.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in emdoor.o).
class cEmDoor : public cEm {
public:
    virtual void move();
};

#endif
