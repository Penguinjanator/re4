#ifndef PENDULUM_H
#define PENDULUM_H

#include "types.h"
#include "vec.h"

class cModel;

// Pendulum / cloth chain work (game/pendulum.cpp), 0x60 bytes (same object as pl_cloth.h's
// PlCloth). Field meanings from obj14ClothSet; the rest is zeroed there.
struct PenCloth {
    int num;             // 0x00  number of chain links
    const u8* pParts;    // 0x04  parts index per link
    u32 x08;
    u32 x0C;
    u32 x10;
    u32 x14;
    const u8* pUp;       // 0x18  upper neighbour per link (0xFF = none)
    const u8* pDown;     // 0x1C  lower neighbour per link (0xFF = none)
    u32 x20;
    u32 x24;
    const f32* pMax;     // 0x28  max swing angle per link
    u32 x2C;
    u32 x30;
    u32 x34;
    u32 x38;
    f32 x3C;             // 0x3C  (15.0 for the bell)
    f32 x40;             // 0x40  (1.0)
    u32 x44;
    f32 x48;
    f32 x4C;
    f32 x50;
    u32 x54;
    u32 x58;
    u32 flags;           // 0x5C  (0x100)
};

extern "C" {
void PenClothSet(cModel* m, PenCloth* c, f32 len);
void PenClothMove3(cModel* m, PenCloth* c);
void PenClothMove(cModel* m, PenCloth* c);
// global wind: direction (radians), strength, x (cPenWind::set in light.cpp)
void PenWindSet(f32 dir, f32 power, f32 x);
}

#endif
