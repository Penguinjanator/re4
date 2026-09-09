#ifndef OBJ13_H
#define OBJ13_H

#include "types.h"
#include "obj.h"

// Room-script view of the ladder object (game/obj13.cpp defines the full class with its virtuals;
// this declares only the out-of-line members the rooms call on a getRoomEtcLadder() result, so no
// vtable is emitted here).
class cObjLadder : public cObj {
public:
    int getStatus();
    void setStand();
    void setCamera(int no);
};

#endif
