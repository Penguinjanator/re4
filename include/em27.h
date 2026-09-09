#ifndef EM27_H
#define EM27_H

#include "types.h"
#include "em.h"

// Enemy 0x27 (the lake fish, em27 module). Only the member the stage rooms call is declared;
// the em27 module's own header can extend this.
class cEm27 : public cEm {
public:
    void setWaterHeight(f32 h);   // setWaterHeight__5cEm27f
};

#endif
