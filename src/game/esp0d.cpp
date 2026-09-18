#include "atari.h"
#include "math_sub.h"
#include "esp.h"

struct Esp0dWork {
    u32 Type;        // 0x00
    f32 Dist;        // 0x04
    f32 Pow;       // 0x08
    cCoord* target;  // 0x0C
};

// Attractor effect: pulls the sprite toward its parent while inside `dist`.
class cEsp0d : public cEsp {
public:
    Esp0dWork m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp0d_Create()
{
    return new cEsp0d;
}

void cEsp0d::move()
{
    Esp0dWork* w = &m_Free;
    Vec v;
    f32 len;

    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        } else if (w->Type == 0) {
            PSVECSubtract(&m_Pos, &w->target->world, &v);
            len = PSVECMag(&v);
            if (len == 0.0f) {
                v.x = 0.0f;
                v.y = 1.0f;
                v.z = 0.0f;
            } else {
#line 77 "D:/Bio4/Prog/esp0d.cpp"
                VECNormalize(&v, &v);
            }
            if (len < w->Dist) {
                PSVECScale(&v, &v, (w->Dist - len) * w->Pow);
                if (m_Tool_flg & 1) {
                    v.y = 0.0f;
                }
                PSVECAdd(&m_Speed, &v, &m_Speed);
            }
        }
    }
}

int cEsp0d::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp0dWork* w = &m_Free;

    w->Dist = (f32)(s8)gen->xC8 * 100.0f;
    w->Pow = (f32)(s8)gen->xC9 * 0.00005f;
    w->Type = gen->xFC;
    if (m_Tool_flg & 0x20) {
        w->target = m_pMod->getPartsPtr(m_Parts_no);
    } else {
        w->target = parent;
    }
    if (w->Type != 0) {
        pLog->err(0, 0, "ESP0D : Type[%x] invalid.", w->Type);
        return 0;
    }
    return 1;
}
