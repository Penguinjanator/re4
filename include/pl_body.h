#ifndef PL_BODY_H
#define PL_BODY_H

#include "types.h"
#include "vec.h"
#include "model.h"
#include "math_sub.h"

// One SPAE (spline attack effect) record built by cPlBody::makeSpaeData (0x58 bytes).
struct SpaeData {
    u32 id;         // 0x00  0x101
    u32 type;       // 0x04  2
    u32 x08;        // 0x08  0x18
    u16 x0C;        // 0x0C  0
    u16 x0E;        // 0x0E  2
    u32 x10;        // 0x10  0x38
    u16 x14;        // 0x14  1
    u16 x16;        // 0x16  2
    u32 x18;        // 0x18  0
    Vec scale;      // 0x1C  (1, 0, 0)
    u32 x28;        // 0x28  0x100
    Vec x2C;        // 0x2C  (0, 0, 0)
    u32 x38;        // 0x38  0
    Vec x3C;        // 0x3C  (0, 0, 0)
    u32 x48;        // 0x48  0x100
    Vec x4C;        // 0x4C  (1, 0, 0)
};

// Model part as seen by pl_body (cModel::getPartsPtr result): the waist twist writes its rotation.
struct PlBodyParts {
    u8 pad_0[0x128];
    Vec rot;        // 0x128
    u8 pad_134[0x1C0 - 0x134];
    u32 flags;      // 0x1C0  bit30: rotation override
};

// Player body helper (game/pl_body.cpp): waist twist and weapon hand (0xF0 bytes).
class cPlBody {
public:
    u32 x00;            // 0x00
    u32 x04;            // 0x04
    u32 x08;            // 0x08
    u32 wepHand;        // 0x0C  (initWepHand)
    u32 x10;            // 0x10
    u32 x14;            // 0x14
    u32 x18;            // 0x18
    u32 x1C;            // 0x1C
    u32 x20;            // 0x20
    u8 pad_24[0x38 - 0x24];
    cModel* pModel;     // 0x38
    f32 waist;          // 0x3C  waist twist angle (waistSet)
    SpaeData spae[2];   // 0x40

    cPlBody(cModel* model);
    void move();
    void waistSet(f32 angle);
    void waistMove();
    void makeSpaeData();
    void initWepHand(u32 hand);
};

#endif
