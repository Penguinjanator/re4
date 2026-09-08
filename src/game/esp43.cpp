#include "atari.h"
#include "light.h"
#include "global.h"
#include "esp.h"

struct Esp43Work {
    u8 estNo;    // 0x00 est to spawn when the effect starts (0xFF = none)
    u8 pad_1[3];
    u32 started; // 0x04
};

Vec g_pos;

// Effect that waits for the cutscene flag before starting and spawns an est on its first frame.
class cEsp43 : public cEsp {
public:
    Esp43Work work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp43_Create()
{
    return new cEsp43;
}

void cEsp43::move()
{
    Esp43Work* w = &work;
    u32 on = 1;

    if (pG->flags_64 & 0x00800000) {
        if (w->started == 0) {
            w->started = on;
        }
    }
    if (w->started == 0) {
        xAA = on;
        cnt = 0;
    } else {
        if (xAA == cnt && w->estNo != 0xff) {
            EstSet(0, -1, &pos, &rot, 1, w->estNo, info.x0, info.x2, info.x8, 0);
        }
        if (!CommonMove()) {
            return;
        }
    }
    if (!AnmMove()) {
        PushEsp(this);
    }
}

int cEsp43::SetFreeWork(EspGenWork* gen, u32* seed)
{
    work.estNo = gen->xC8;
    return 1;
}
