#ifndef ESP_H
#define ESP_H

#include "types.h"
#include "vec.h"
#include "model.h"

// Effect generator work (game/eff_sys.cpp, game/espgen*.cpp). Layout known only partially.
struct EspGenWork {
    u8 pad_0[0xC8];
    u8 xC8;  // 0xC8 per-effect parameter (SE number, area number, ...)
    u8 pad_C9[0xD0 - 0xC9];
};

// One effect sprite (game/esp.cpp, game/esp_sub.cpp). sizeof 0xF8; the vptr sits at 0xF4.
class cEsp {
public:
    u8 pad_0[0x0D];
    u8 id;             // 0x0D effect id
    u8 pad_0E[0x24 - 0x0E];
    cCoord* parent;    // 0x24 parent coordinate (pEffParentWorld = world)
    u8 pad_28[4];
    Vec pos;           // 0x2C
    u8 pad_38[0xF4 - 0x38];
    // 0xF4 vptr

    void* operator new(unsigned int size);
    cEsp();
    virtual ~cEsp();
    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen);
    virtual void Destruct();

    int CommonMove();
    int AnmMove();
    void ColorUpdate();
    void ApplyMatrix();
    void CommonStateSet();
};

// game/esp.cpp
void PushEsp(cEsp* esp);
// game/eff_sys.cpp
int EspGenGetMoveLoop();
extern cCoord* pEffParentWorld;
// game/esp_app.cpp
void EffCallRoomSeFunc(int no, Vec* pos);
int EffAreaCheckNo(Vec* pos, u8 areaNo);

#endif
