// game/light01: light type 1, flicker (D:/Bio4/Prog/light01.cpp). Light types are the per-frame
// move entries of LightFuncTbl (game.cpp) run by cLightMgr::move on every alive cLight; each reads
// its parameters from the light's `work` bytes set by the room light data.
#include "light.h"
#include "rnd.h"

struct Light01Work {
    u8 pad_0[4];
    s8 range;  // 0x04 random brightness range (+-)
};

// Nothing beyond the cLight constructor.
cLight01::cLight01()
{
}

// LightFuncTbl[1]: torches and candles: DispCol = Col plus one random offset in [-range, range)
// applied to r, g and b each frame.
// Flicker light: adds a random offset in [-range, range) to every color channel.
void Light01_Move(cLight* pLi)
{
    Light01Work* w = (Light01Work*)pLi->work;
    int r;
    int c;

    if (w->range != 0) {
        r = Rnd() % (w->range + w->range) - w->range;
    } else {
        r = 0;
    }
    c = pLi->Col.r + r;
    if (c < 0) {
        c = 0;
    } else if (c > 255) {
        c = 255;
    }
    pLi->DispCol.r = c;
    c = pLi->Col.g + r;
    if (c < 0) {
        c = 0;
    } else if (c > 255) {
        c = 255;
    }
    pLi->DispCol.g = c;
    c = pLi->Col.b + r;
    if (c < 0) {
        c = 0;
    } else if (c > 255) {
        c = 255;
    }
    pLi->DispCol.b = c;
    pLi->DispCol.a = pLi->Col.a;
}
