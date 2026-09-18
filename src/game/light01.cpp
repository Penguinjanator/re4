#include "light.h"
#include "rnd.h"

struct Light01Work {
    u8 pad_0[4];
    s8 range;  // 0x04 random brightness range (+-)
};

cLight01::cLight01()
{
}

// Flicker light: adds a random offset in [-range, range) to every color channel.
void Light01_Move(cLight* l)
{
    Light01Work* w = (Light01Work*)l->work;
    int r;
    int c;

    if (w->range != 0) {
        r = Rnd() % (w->range + w->range) - w->range;
    } else {
        r = 0;
    }
    c = l->Col.r + r;
    if (c < 0) {
        c = 0;
    } else if (c > 255) {
        c = 255;
    }
    l->DispCol.r = c;
    c = l->Col.g + r;
    if (c < 0) {
        c = 0;
    } else if (c > 255) {
        c = 255;
    }
    l->DispCol.g = c;
    c = l->Col.b + r;
    if (c < 0) {
        c = 0;
    } else if (c > 255) {
        c = 255;
    }
    l->DispCol.b = c;
    l->DispCol.a = l->Col.a;
}
