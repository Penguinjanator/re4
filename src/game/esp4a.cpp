#include "atari.h"
#include "light.h"
#include "global.h"
#include "esp.h"

struct Esp4aWork {
    u8 quake_type;    // 0x00 quake type
    u8 pad_1[3];
    f32 range;  // 0x04 distance from the camera at which the quake fades to zero (0 = no fade)
};

extern "C" void QuakeExec(int a, int b, int c, u8 type, f32 power);

// Camera quake driven by the effect's alpha, attenuated by the distance to the camera.
class cEsp4a : public cEsp {
public:
    Esp4aWork m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp4a_Create()
{
    return new cEsp4a;
}

void cEsp4a::move()
{
    Esp4aWork* w = &m_Free;
    f32 power;

    if (CommonMove()) {
        power = m_Col_a + m_Col_a;
        if (w->range != 0.0f) {
            f32 dist = PSVECDistance(&pG->Cam.param.pos, &m_Pos);
            if (dist < w->range) {
                power *= (w->range - dist) / w->range;
            } else {
                power = 0.0f;
            }
        }
        QuakeExec(0, 0, 1, w->quake_type, power);
    }
}

void Esp4a_Trans()
{
}

int cEsp4a::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp4aWork* w = &m_Free;

    switch ((s8)gen->xC8) {
    case 0:
        w->quake_type = 2;
        break;
    case 1:
        w->quake_type = 1;
        break;
    case 2:
        w->quake_type = 3;
        break;
    default:
        pLog->err(0, 0, "ESP4A : Invalid Type[%d]", (s8)gen->xC8);
        return 0;
    }
    w->range = gen->xE0;
    return 1;
}
