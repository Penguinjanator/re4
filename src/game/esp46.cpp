#include "atari.h"
#include "filter.h"
#include "esp.h"

struct Esp46Work {
    int level;   // 0x00 filter03 type
    u8 priority;      // 0x04
    u8 sp_flag;      // 0x05
};

// Screen filter (filter03) driver: never drawn itself, feeds its color into the filter.
class cEsp46 : public cEsp {
public:
    Esp46Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp46_Create()
{
    return new cEsp46;
}

void cEsp46::move()
{
    m_Size_mul = 1e22f;
    CommonMove();
}

void Esp46_Trans(cEsp46* pEsp)
{
    Esp46Work* w = &pEsp->m_Free;
    f32 a = pEsp->m_Col_a * (1.0f / 255.0f);

    Filter03SetParam(w->level, (u8)(pEsp->m_Col_r * a), (u8)(pEsp->m_Col_g * a), (u8)(pEsp->m_Col_b * a), w->priority, w->sp_flag);
}

int cEsp46::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp46Work* w = &m_Free;

    w->level = (s8)gen->xC8;
    w->priority = gen->xC9;
    if ((s8)gen->xCA <= 1) {
        w->sp_flag = gen->xCA;
    } else {
        pLog->err(0, 0, "ESP46 : WK2[%x] invalid.", (s8)gen->xCA);
    }
    m_Parts_no = 0xf8;
    return 1;
}
