#ifndef EMBARREL_H
#define EMBARREL_H

#include "types.h"
#include "vec.h"
#include "em.h"

// barrel enemy (game/embarrel.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in embarrel.o).
class cEmBarrel : public cEm {
public:
    virtual void move();
};

#endif
