#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "vec.h"
#include "model.h"

// Player work (game/player.cpp, pl_*.cpp). Partial layout; extend the pads, never rewrite.
class cPlayer : public cModel {
public:
    u8 pad_1D8[0x2A4 - 0x1D8];
    struct cPlWep* pWep;         // 0x2A4 equipped weapon (0x05: u8 busy flag)
    u8 pad_2A8[0x794 - 0x2A8];
    struct PlFaceInfo* pFace;    // 0x794 face shape data (t_option FACE CONTROL)

    // game/pl_wep.cpp
    void weaponRelease();
    void weaponLoad(int no, int type);  // stores pG 0x4FB0/0x4FB1, then ReadWepData
    void weaponInit();
};

// Only the shape work pointer the face tool passes to ShapeSet/ShapeEnd is known.
struct PlFaceInfo {
    u8 pad_0[0x10];
    void* pShape;                // 0x10
};

// Weapon record hung off the player; only the byte cam_ctrl reads is named.
struct cPlWep {
    u8 pad_0[5];
    u8 x5;                       // 0x05
};

extern cPlayer* pPL;

#endif
