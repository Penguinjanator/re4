#include "atari.h"
#include "rnd.h"
#include "math_sub.h"
#include "esp.h"

struct Esp48Work {
    f32 dist_x;    // 0x00
    f32 time_x;   // 0x04
    f32 dist_y;    // 0x08
    f32 time_y;   // 0x0C
    f32 dist_z;    // 0x10
    f32 time_z;   // 0x14
    f32 timer;    // 0x18
    Vec add_ang;   // 0x1C rotation added this frame
};

// Effect whose rotation swings on a sine wave around the base rotation.
class cEsp48 : public cEsp {
public:
    Esp48Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp48_Create()
{
    return new cEsp48;
}

void cEsp48::move()
{
    Esp48Work* w = &m_Free;

    PSVECSubtract(&m_Ang, &w->add_ang, &m_Ang);
    if (CommonMove()) {
        w->timer += 0.01f;
        w->add_ang.x = w->dist_x * sinf(w->timer * w->time_x);
        w->add_ang.y = w->dist_y * sinf(w->timer * w->time_y);
        w->add_ang.z = w->dist_z * sinf(w->timer * w->time_z);
        PSVECAdd(&m_Ang, &w->add_ang, &m_Ang);
        if (!AnmMove()) {
            PushEsp(this);
        }
    }
}

int cEsp48::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp48Work* w = &m_Free;

    w->dist_x = gen->Vec0.x * 0.1f;
    w->time_x = gen->Vec0.y;
    w->dist_y = gen->Vec1.x * 0.1f;
    w->time_y = gen->Vec1.y;
    w->dist_z = gen->Vec2.x * 0.1f;
    w->time_z = gen->Vec2.y;
    w->timer = fRandSeed1_1(seed) * 2.0f * PI;
    return 1;
}
