#ifndef EMROCK_H
#define EMROCK_H

#include "types.h"
#include "vec.h"
#include "em.h"

// rolling rock enemy (game/emrock.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in emrock.o).
class cEmRock : public cEm {
public:
    virtual void move();
};

#endif
