#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "vec.h"
#include "model.h"
#include "atariInfo.h"

// Player work (game/player.cpp, pl_*.cpp). Partial layout; extend the pads, never rewrite.
class cPlayer : public cModel {
public:
    u8 motion[0x2A4 - 0x1D8];    // 0x1D8 motion work (MotionSetCore(this, &motion, ...)); size unknown
    struct cPlWep* pWep;         // 0x2A4 equipped weapon (0x05: u8 busy flag)
    u8 pad_2A8[0x2B4 - 0x2A8];
    cAtariInfo atari;            // 0x2B4 .. 0x300
    u8 pad_300[0x3E0 - 0x300];
    u32 x3E0;                    // 0x3E0  (pl_event: set to 1 when the walk-to-target motion starts)
    u8 pad_3E4[0x400 - 0x3E4];
    f32 evTurnSpeed;             // 0x400  event: max turn per frame (rad)
    Vec evTarget;                // 0x404  event: walk-to position
    u8 pad_410[0x41C - 0x410];
    u32 flags_41C;               // 0x41C  bit8 (0x100): event motion done -> reset routine
    u8 pad_420[4];
    void** pMotTbl;              // 0x424  motion data table ([0] walk, [2] turn)
    u8 pad_428[0x794 - 0x428];
    struct PlFaceInfo* pFace;    // 0x794 face shape data (t_option FACE CONTROL)

    // game/pl_class.cpp
    void setFootwork();
    // game/pl_debug.cpp
    void debugInit();
    void debugMove();
    void emSearch();
    // game/pl_wep.cpp
    void weaponRelease();
    void weaponLoad(int no, int type);  // stores pG 0x4FB0/0x4FB1, then ReadWepData
    void weaponInit();
};

// Debug cheat ("maho") command table (game/pl_debug.cpp), 0x16C bytes, `new`ed by cPlayer::debugInit.
struct PlMahoEntry {
    u8 x0;               // 0x00
    u8 x1;               // 0x01
    void (*func)();      // 0x04
    const char* code;    // 0x08  button sequence string
};

class cPlMaho {
public:
    PlMahoEntry tbl[30]; // 0x000
    u32 num;             // 0x168

    cPlMaho();
    void reset();
    void regist(const char* code, void (*func)());
};

extern cPlMaho* pMaho;   // game/player.cpp
extern u8 PlKaiou;       // game/player.cpp  kaiouken level (0..2)
extern u8 PlDbFlag;      // game/player.cpp  bit1: draw the player position marker
extern void* PlWepMot[3];  // game/player.cpp  weapon motion data

void PlWepMotSet(int no);
void DrawGage(int x, int y, int h, int w, int now, int max, int color);

// game/pl_event.cpp: routine 0 (event) and its sub-routines (index cModel::xFD)
void Pl_R0_Event(cPlayer* pl);
void pl_R1_Event_Normal(cPlayer* pl);
void pl_R1_Event_ToWalk(cPlayer* pl);
void pl_R1_Event_Smooth(cPlayer* pl);

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
