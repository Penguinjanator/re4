#ifndef OBJ12_H
#define OBJ12_H

#include "types.h"
#include "vec.h"
#include "obj.h"

// Hanging object (game/obj12.cpp): the Ganado's sack / lantern, the door's locks and chain.
class cObj12 : public cObj {
public:
    void setParent(cModel* parent, int parts, int flag);
    void setFall(Vec* spd, u8 type);
    void setFallSe(u8 blk, u8 no, u8 id);
    void setBurn();
};

cObj* SetObj12(void* bin, void* tpl, Vec* pos, Vec* rot);

#endif
