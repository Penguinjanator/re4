#ifndef EMBARRED_H
#define EMBARRED_H

#include "types.h"
#include "em.h"

// Barred gate enemy (game/emBarred.cpp); only the entry points other units call.
class cEmBarred : public cEm {
public:
    virtual void move();   // key function: the vtable stays in this unit (cEmMgr::construct stores it)

    void setOpen(int a);
    void setClose(int a);
    void setOpened();
    void setClosed();
};

#endif
