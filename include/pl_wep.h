#ifndef PL_WEP_H
#define PL_WEP_H

#include "types.h"
#include "vec.h"
#include "model.h"

// Weapon objects (game/objWep.cpp, objRocket.cpp) as the player units see them: a cModel with the
// weapon virtuals (vtable order from objWep's `cObjWep virtual table`; its fields sit inside the
// cObj work area, so it is not a cObj subclass). Only what the player units touch is named; to be
// merged into the obj headers when those units are decompiled.
class cObjWep : public cModel {
public:
    u8 pad_1D8[0x330 - 0x1D8];
    f32 x330;            // 0x330  lock random: pitch range (pl_wep PlWepLockRand)
    f32 x334;            // 0x334  lock random: yaw range
    f32 x338;            // 0x338  lock random: pitch step
    f32 x33C;            // 0x33C  lock random: yaw step
    u8 pad_340[0x34E - 0x340];
    u8 x34E;             // 0x34E  (knife down: 1)
    u8 x34F;             // 0x34F  (knife down: 0)
    u8 x350;             // 0x350  display flags (setDisp: type 0/1/2 -> bit 0x4/0x8/0x10)
    u8 pad_351[7];
    Vec x358;            // 0x358  muzzle / hit marker position (pl_wep getMarkerPos, PlWepHitCheck2)

    virtual void moveAll();
    virtual void moveStay();
    virtual void moveReady();
    virtual void moveFire();
    virtual void moveDown();
    virtual void moveReload();
    virtual void moveDrop();
    virtual void init();
    virtual void setMotion();     // pl_sub PlReloadBullet
    virtual void interrupt();
    virtual void endReload();
    virtual int keyKamae();       // pl_sub joyKamae
    virtual void fire();
    virtual void beginReload();

    void setDisp(int type, int on);
};

class cObjLauncher : public cObjWep {
public:
    void grip(int a);
    void gripBack();
};

// Player weapon control (game/pl_wep.cpp), 0x44 bytes at cEm::pWep.
class cPlWep {
public:
    u8 pad_0[0x20];
    u8 x20;              // 0x20  (ctor: 0)
    u8 pad_21[3];
    u8 knifeStance;      // 0x24  knife ready stance: 0 low, 1 middle, 2 high
    f32 pitch;           // 0x28  aim pitch
    f32 x2C;             // 0x2C
    u8 pad_30[4];
    cObjWep* pObj;       // 0x34  weapon object (cObjLauncher for the rocket launcher)
    cObjWep* pObj2;      // 0x38  second weapon object (rifles / launchers display part)
    u8 pad_3C[4];
    u8 x40;              // 0x40  lock frames left (lockInit/lockNext: 10; lockMove clears it on a stick move)
    u8 pad_41[3];

    cPlWep();
    f32 getAngle();
    f32 getPitch();
    void move();
    int getMarkerPos(Vec* out);
    cModel* lockInit();
    void lockMove();
    cModel* lockNext();
    void setTrans(int on, int type);   // pObj/pObj2 display by weapon (pl_sub PlSetHand)
};

// knife/weapon collision (pl, top, bottom, type, flags, length)
u32 PlWepHitCheck2(cModel* pl, Vec* p0, Vec* p1, int type, u32 flag, f32 len);
void PlWepLockCtrl(cModel* pl);

extern "C" {
u32 PlWepHitCheck3(Vec* pos, int type, u32 prio, f32 len);
void PlWepAutoTrack(cModel* pl, int mode, f32 rate);
void PlWepLockRandInit();
void PlWepLockRand(cModel* pl, int flag, f32* pitch, f32* yaw);
void PlSetLockPitch(cModel* pl);
int GetWepSizeGroup(int no);
int PlCornerCheck();
cModel* SearchLockEm(Vec* pos, cModel* skip);
cModel* SearchTargetEm(Vec* pos, cModel* skip, f32 range);
}

extern u8 lockCtr;
extern void (*WeaponInitFunc)(cModel*);

#endif
