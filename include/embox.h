#ifndef EMBOX_H
#define EMBOX_H

#include "types.h"
#include "vec.h"
#include "em.h"

// breakable box enemy (game/embox.cpp). Only the class is declared yet: cEmMgr::construct builds it and stores
// its vtable (the key function `move` keeps the vtable in embox.o).
class cEmBox : public cEm {
public:
    virtual void move();
};

#endif
