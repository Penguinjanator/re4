#include "atari.h"
#include "esp.h"

struct Esp1bWork {
    int n;   // 0x00 number of points
    Vec v0;  // 0x04
    Vec v1;  // 0x10
    Vec v2;  // 0x1C
};

static f32 esp1b_scale = 0.005f;

// Spline sprite (drawn by Esp1b_SpTrans in esp_sub.cpp).
class cEsp1b : public cEsp {
public:
    Esp1bWork work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp1b_Create()
{
    return new cEsp1b;
}

void cEsp1b::move()
{
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        }
    }
}

int cEsp1b::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp1bWork* w = &work;
    int n;

    n = (s8)gen->xC8 + 4;
    if (n <= 1) {
        pLog->err(0, 0, "ESP1B : Wk0[%d] Invalid.", (s8)gen->xC8);
        n = 2;
    }
    if (n > 0x40) {
        pLog->err(0, 0, "ESP1B : Wk0[%d] Invalid.", (s8)gen->xC8);
        n = 0x40;
    }
    dispFlag |= 0x10;
    w->n = n;
    w->v0 = *(Vec*)&gen->xD8;
    w->v1 = *(Vec*)&gen->xE4;
    w->v2 = *(Vec*)&gen->xF0;
    PSVECScale(&w->v0, &w->v0, esp1b_scale);
    PSVECScale(&w->v1, &w->v1, esp1b_scale);
    PSVECScale(&w->v2, &w->v2, esp1b_scale);
    return 1;
}
