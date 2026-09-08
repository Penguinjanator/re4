#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "vec.h"
#include "model.h"
#include "em.h"

// Player model set (pl_leon.cpp setModel/setRightHand/...): the extra model infos hung off the
// player and the data pointers they were built from. Partial layout.
struct PlFaceInfo {
    void* pRightData;            // 0x00  right hand model data (0 = none)
    void* pLeftData;             // 0x04  left hand model data (0 = none)
    void* pHeadData;             // 0x08  head model data
    void* pRightDataAlt;         // 0x0C  right hand data used by setRightHand(1)
    cModelInfo* pShape;          // 0x10  head model info (face shape animation target of ShapeSet/ShapeEnd)
    u8 pad_14[8];
    cModelInfo* pRight;          // 0x1C  right hand model info
    cModelInfo* pLeft;           // 0x20  left hand model info
    cModelInfo* pHair;           // 0x24
    cModelInfo* pEye;            // 0x28  (flags |= 0x40)
    cModelInfo* pFace;           // 0x2C
    u32 leftNo;                  // 0x30  current left hand item no
    u32 leftNoPrev;              // 0x34  previous one (setLeftHand(0x63) restores it)
};

// Weapon record hung off the player; only the byte cam_ctrl reads is named.
struct cPlWep {
    u8 pad_0[5];
    u8 x5;                       // 0x05
};

// Player (game/player.cpp, pl_*.cpp): a cEm with the player virtuals. Its fields are the cEm ones
// (all below 0xDE0, see em.h). Vtable order (pl_class.cpp): cUnit/cCoord/cModel/cEm virtuals, then
// the player ones below.
class cPlayer : public cEm {
public:
    cPlayer();
    virtual ~cPlayer() {}
    virtual void beginEvent();
    virtual void endEvent();
    virtual void move();
    virtual void setNoSuspend(int on);
    virtual int checkXbutton();
    virtual void setModel() = 0;
    virtual void setMotion();
    virtual void setRightHand(int no) = 0;
    virtual void setLeftHand(u32 no) = 0;
    virtual void setFace(int no) = 0;
    virtual void setHead(int no);
    virtual void setHead(void* bin, void* tpl);
    virtual void setWound();
    virtual void moveMatCalcBefore();
    virtual void initCloth();
    virtual void moveCloth();

    // game/player.cpp
    void init0();
    void init1();
    void startUp();
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

// Leon (game/pl_leon.cpp): the player model set for the main character.
class cPlLeon : public cPlayer {
public:
    cPlLeon();
    virtual ~cPlLeon() {}
    virtual void move();
    virtual int checkXbutton();
    virtual void setModel();
    virtual void setMotion();
    virtual void setRightHand(int no);
    virtual void setLeftHand(u32 no);
    virtual void setFace(int no);
    virtual void setHead(int no);
    virtual void setHead(void* bin, void* tpl);
    virtual void setWound();
    virtual void initCloth();
    virtual void moveCloth();
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

extern cPlayer* pPL;

#endif
