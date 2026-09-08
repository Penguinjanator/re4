#ifndef MERCENARIES_H
#define MERCENARIES_H

#include "types.h"

// Mercenaries mode id (game/mercenaries.cpp `mercId`, 0x64 bytes); layout opaque.
class MercID {
public:
    u8 pad_0[0x64];

    void set();
};

extern MercID mercId;

#endif
