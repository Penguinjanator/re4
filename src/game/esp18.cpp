#include "light.h"
#include "atari.h"
#include "gx.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"
#include "main_sub.h"

extern f32 ZNEAR;
extern f32 ZFAR;

struct Esp18Work {
    Vec pos0;    // 0x00 initial position
    f32 depth;   // 0x0C -gen->xD8
};

// Heat shimmer: copies the frame buffer and redraws it through an indirect texture in
// esp18_lp layers.
class cEsp18 : public cEsp {
public:
    Esp18Work work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

extern "C" {
cEsp* Esp18_Create();
void Esp18_Trans(cEsp* esp);
}

cEsp* Esp18_Create()
{
    return new cEsp18;
}

void cEsp18::move()
{
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        }
    }
}

int cEsp18::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp18Work* w = &work;

    w->pos0 = pos;
    w->depth = -gen->xD8;
    return 1;
}

// TODO: Esp18_Trans (0x1180 bytes: frame copy + indirect texture shimmer, esp18_lp layers)
void Esp18_Trans(cEsp* esp)
{
}
