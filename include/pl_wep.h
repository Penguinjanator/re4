#ifndef PL_WEP_H
#define PL_WEP_H

#include "types.h"
#include "vec.h"

class cModel;

// Weapon objects (game/objWep.cpp, objRocket.cpp) as the player units see them: really cObj
// subclasses (obj.h), kept opaque here so the player units do not pull in obj.h. Only what the
// player units touch is named; to be merged into the obj headers when those units are decompiled.
class cObjWep {
public:
    u8 pad_0[0x34E];
    u8 x34E;             // 0x34E  (knife down: 1)
    u8 x34F;             // 0x34F  (knife down: 0)
    u8 x350;             // 0x350  display flags (setDisp: type 0/1/2 -> bit 0x4/0x8/0x10)

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
    u8 x40;              // 0x40  (lockMove clears it on Joy trigger)
    u8 pad_41[3];

    cPlWep();
    f32 getAngle();
    f32 getPitch();
    void move();
    int getMarkerPos(Vec* out);
    void lockInit();
    void lockMove();
    void lockNext();
};

// knife/weapon collision (pl, top, bottom, type, flags, length)
int PlWepHitCheck2(cModel* pl, Vec* p0, Vec* p1, int type, u32 flag, f32 len);
void PlWepLockCtrl(cModel* pl);

extern u8 lockCtr;

#endif
