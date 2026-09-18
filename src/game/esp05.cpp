#include "light.h"
#include "atari.h"
#include "math_sub.h"
#include "rnd.h"
#include "esp.h"

struct Esp05Work {
    f32 Pow;     // 0x00 wobble amplitude
    f32 Spd;  // 0x04
    f32 Theta;     // 0x08
};

// Fluttering sprite (falling leaf / feather): wobbles the position with sin/cos of a random angle.
class cEsp05 : public cEsp {
public:
    Esp05Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp05_Create()
{
    return new cEsp05;
}

void cEsp05::move()
{
    Esp05Work* w = &m_Free;

    if (CommonMove()) {
        m_Pos.x += w->Pow * sinf(w->Theta);
        m_Pos.z += w->Pow * cosf(w->Theta * 1.7f);
        m_Pos.y += w->Pow * 0.3f * cosf(w->Theta * 1.9f);
        w->Theta += w->Spd * (fRand0_1() + 0.2f);
        if (!AnmMove()) {
            PushEsp(this);
        }
    }
}

int cEsp05::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp05Work* w = &m_Free;

    w->Pow = gen->xD8;
    w->Spd = gen->xDC * 0.05f;
    w->Pow += w->Pow * fRand0_1() * (gen->xE0 * 0.1f);
    w->Spd += w->Spd * fRand0_1() * (gen->xE0 * 0.1f);
    w->Theta = fRand0_1() * PI * 2.0f;
    return 1;
}
