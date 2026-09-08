#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "vec.h"
#include "model.h"
#include "em.h"
#include "global.h"
#include "pl_body.h"
#include "pl_wep.h"
#include "pl_cloth.h"

// 0x1FE-byte work at cEm::p2A4 (player.cpp mem_alloc); only the byte cam_ctrl reads is named.
struct EmWork2A4 {
    u8 x0;
    u8 pad_1[4];
    u8 x5;                       // 0x05
};

// Neck control (game/pl_class.cpp), 0x1C bytes at cEm::pNeck. Layout opaque here.
class cPlNeck {
public:
    u8 pad_0[0x1C];

    cPlNeck();
    void init(void* a, void* b, int c);  // a/b are range-checked pointers (motSet), c passed on
    void move();
    void setMode(int mode);   // stores byte 0xE (pl_sub PlSetNeck)
};

// Waist control (game/pl_class.cpp), 0xC bytes at cEm::pWaist.
class cPlWaist {
public:
    u32 x0;
    f32 cur;                     // 0x04  current angle
    u32 x8;

    cPlWaist();
    // cur = cur * (1 - rate) + target * rate; returns the delta applied
    f32 set(f32 target, f32 rate);
};

// Three-way motion blend (game/pl_class.cpp), 0xE8 bytes; `mot3` in player.cpp. Layout opaque here.
class cMot3 {
public:
    u8 pad_0[0xE8];

    cMot3();
    // set(model, motion0, motion1, motion2, MotionSetCore 4th arg, u8 mode, int, u16, u16)
    void set(cModel* m, void* m0, void* m1, void* m2, int a, u8 b, int c, u16 d, u16 e);
    void set0(void* m, u8 a, int b);
    void move(f32 rate);
};

extern cMot3 mot3;      // game/player.cpp
extern f32 m3r[3];      // game/player.cpp  mot3 blend rates ([0] current, [1] target, [2] mix)

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
    void beginDamage();
    void endDamage();
    int endCamera();
    void interrupt();
    void checkCtrl();    // Key 0x400/0x100000 -> pG->flags_500C bits
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
    // In-class on purpose: the original emits these after the destructor at the end of the unit
    // (in-class inline members of the class whose vtable the unit owns), not in source order.
    virtual void initCloth()
    {
        if (pG->costume != 2) {
            PlClothSetLeon(this, &leonHair, &leonJacket, &leonHolster);
        }
    }
    virtual void moveCloth()
    {
        if (pG->costume != 2) {
            PlClothMoveLeon(this, &leonHair, &leonJacket, &leonHolster);
        }
    }
};

// Ashley (game/pl_ashley.cpp): the partner character's model set and bust motion.
class cPlAshley : public cPlayer {
public:
    cPlAshley();
    virtual ~cPlAshley() {}
    virtual void move();
    virtual void setModel();
    virtual void setRightHand(int no);
    virtual void setLeftHand(u32 no);
    virtual void setFace(int no);
    virtual void moveMatCalcBefore();
    virtual void initCloth() { PlClothSetGirl(this, &girlHair, &girlSkirt, &girlSweater, 0); }
    virtual void moveCloth() { PlClothMoveGirl(this, &girlHair, &girlSkirt, &girlSweater); }
    void moveBust();
};

void pl01weaponSet(cPlayer* pl);  // game/pl_ashley.cpp: fills pMotTbl from the player archive

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

// game/pl_sub.cpp: control helpers
int joyFireOn();
int joyFireTrg();
int joyKamae();
int joyLKamae();

// game/pl_class.cpp
int dmMotCk();

// game/pl_knife.cpp: routine 2 (knife) and its sub-routines (index cModel::xFE / xFF)
void PlKnifeMove(cPlayer* pl);
void knife_r2_ready(cPlayer* pl);
void knife_r2_set(cPlayer* pl);
void knife_r2_fire(cPlayer* pl);
void knife_r2_down(cPlayer* pl);
void setWepTrans(cPlayer* pl, int on);

// (u32) view of pG->wep_no/wep_type (`(G_WEP_ID & 0xFFFF0000) == 0x0D020000`: rocket launcher)
#define G_WEP_ID (*(u32*) &pG->wep_no)
// (u32) view of pG->stage_no/room_no
#define G_ROOM_ID32 (*(u32*) &pG->stage_no)

#endif
