#ifndef PL_NPC_H
#define PL_NPC_H

#include "types.h"
#include "em.h"

// Partner character base (game/pl_npc.cpp, `pSUB`): a cEm with the partner virtuals. Vtable order
// from pl_npc's `cSubChar virtual table`: cUnit/cCoord/cModel/cEm virtuals, then the ones below.
// The cSubChar-only fields (sub*) live in cEm (they sit below 0xDE0, see em.h).
class cSubChar : public cEm {
public:
    virtual void endDamageCore() = 0;   // slot 9: pure here; pl_sub EndSubDamage calls it for id 4
    virtual void setFace(int no);
    virtual void setHand(int no);
    virtual void initCloth();
    virtual void moveCloth();
    virtual void setEmFunc();           // pl_sub SetSubDamage (Ashley)

    void control(int mode);
    void analyze();
    void endDamage();
};

extern cSubChar* pSUB;   // game/em.cpp

#endif
