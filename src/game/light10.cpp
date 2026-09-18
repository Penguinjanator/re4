// game/light10: light type 0x10, muzzle flash / explosion fade-out (D:/Bio4/Prog/light10.cpp).
#include "light.h"

struct Light10Work {
    u8 wait;  // 0x00 frames before the fade starts
};

// LightFuncTbl[0x10]: waits `wait` frames, then multiplies Intensity by 0.3 each frame and deletes
// the light below 0.1.
// Fade-out light: waits, then decays power by 30% a frame and dies below 0.1.
void Light10_Move(cLight* l)
{
    Light10Work* w = (Light10Work*)l->work;

    if (w->wait != 0) {
        w->wait--;
        return;
    }
    l->Intensity *= 0.3f;
    if (l->Intensity < 0.1f) {
        l->Intensity = 0.0f;
        delete l;
    }
}
