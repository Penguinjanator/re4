#ifndef EMTREE_H
#define EMTREE_H

#include "types.h"
#include "vec.h"
#include "em.h"

// tree enemy (game/emtree.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in emtree.o).
class cEmTree : public cEm {
public:
    virtual void move();
};

#endif
