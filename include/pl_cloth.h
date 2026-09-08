#ifndef PL_CLOTH_H
#define PL_CLOTH_H

#include "types.h"
#include "vec.h"

class cModel;

// Cloth simulation work of one player accessory (game/pl_cloth.cpp), 0x60 bytes; layout opaque here.
struct PlCloth {
    u8 pad_0[0x60];
};

extern PlCloth leonHair;
extern PlCloth leonJacket;
extern PlCloth leonHolster;

extern "C" {
void PlClothSetLeon(cModel* pl, PlCloth* hair, PlCloth* jacket, PlCloth* holster);
void PlClothMoveLeon(cModel* pl, PlCloth* hair, PlCloth* jacket, PlCloth* holster);
}

#endif
