#ifndef COCKPIT_H
#define COCKPIT_H

#include "types.h"

// HUD (game/cockpit.cpp), instance `Cckpt` (0xCC bytes): the life meter, the bullet counter, the
// count-down timer and the action button prompt. Sub-object layouts are still partial.
class LifeMeter {
public:
    u8 pad_0[0x84];

    void roomInit();
    void move();
    void fix(int sw);
    void disp();
    void frameOut();
    void frameIn();
};

class BulletInfo {
public:
    u8 pad_0[0x2C];

    void roomInit();
    void move();
};

class CountDown {
public:
    u8 pad_0[0x18];

    void roomInit();
    void move();
    void disp();
    void frameIn();
    void frameOut();
    void initTime(int a, int b);
    void initTimeFrame();
    void warnTime();
    int getTime();
    int getFrame();
    void saveDisp();
    void loadDisp();
};

class ActionButton {
public:
    u8 pad_0[4];

    void roomInit();
    void move();
};

class Cockpit {
public:
    LifeMeter life;         // 0x00
    BulletInfo bullet;      // 0x84
    CountDown countDown;    // 0xB0
    ActionButton action;    // 0xC8  sizeof == 0xCC

    void gameInit();
    void roomInit();
    void move();
    void msgWindow(int mode);
    void lifeMeterDisp(int sw);
};

extern Cockpit Cckpt;

#endif
