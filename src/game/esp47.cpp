#include "atari.h"
#include "light.h"
#include "esp.h"

#define SCR_W 480.0f
#define SCR_H 448.0f
#define SCR_L 32.0f

// Screen-space sprite that wraps around the screen edges (drawn again on the opposite side).
class cEsp47 : public cEsp {
public:
    u32 xF8;

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp47_Create()
{
    return new cEsp47;
}

void cEsp47::move()
{
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        }
    }
}

void Esp47_Trans(cEsp* pEsp)
{
    EspAnmData* anm;
    f32 sx = pEsp->m_Size_base_x * pEsp->m_Size_mul;
    f32 sy = pEsp->m_Size_base_y * pEsp->m_Size_mul;
    f32 rx;
    f32 ry;
    f32 w;
    f32 h;
    int flag;

    if (!EspGetAnmAddr(pEsp->m_Type, &anm)) {
        pLog->err(0, 0, "ESP : TexId[%x] no data", pEsp->m_Type);
        return;
    }
    if ((f32)anm->x4 == 0.0f) {
        rx = -0.5f;
    } else {
        rx = (f32)(-anm->x4) / anm->x0;
    }
    if ((f32)anm->x6 == 0.0f) {
        ry = -0.5f;
    } else {
        ry = (f32)anm->x6 / anm->x2 + -1.0f;
    }
    w = rx * sx;
    h = ry * sy;

    while (pEsp->m_Pos.x > SCR_W) {
        pEsp->m_Pos.x -= SCR_H;
    }
    while (pEsp->m_Pos.x < SCR_L) {
        pEsp->m_Pos.x += SCR_H;
    }
    while (pEsp->m_Pos.y > SCR_H) {
        pEsp->m_Pos.y -= SCR_H;
    }
    while (pEsp->m_Pos.y < 0.0f) {
        pEsp->m_Pos.y += SCR_H;
    }

    flag = 0;
    EspCommonTrans(pEsp);
    if (pEsp->m_Pos.x + w < SCR_L) {
        flag = 1;
        pEsp->m_Pos.x += SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.x -= SCR_H;
    }
    if (pEsp->m_Pos.x - w > SCR_W) {
        flag |= 2;
        pEsp->m_Pos.x -= SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.x += SCR_H;
    }
    if (pEsp->m_Pos.y + h < 0.0f) {
        flag |= 4;
        pEsp->m_Pos.y += SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.y -= SCR_H;
    }
    if (pEsp->m_Pos.y - h > SCR_H) {
        flag |= 8;
        pEsp->m_Pos.y -= SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.y += SCR_H;
    }
    if (flag == 5) {
        pEsp->m_Pos.x += SCR_H;
        pEsp->m_Pos.y += SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.x -= SCR_H;
        pEsp->m_Pos.y -= SCR_H;
    }
    if (flag == 9) {
        pEsp->m_Pos.x += SCR_H;
        pEsp->m_Pos.y -= SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.x -= SCR_H;
        pEsp->m_Pos.y += SCR_H;
    }
    if (flag == 6) {
        pEsp->m_Pos.x -= SCR_H;
        pEsp->m_Pos.y += SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.x += SCR_H;
        pEsp->m_Pos.y -= SCR_H;
    }
    if (flag == 10) {
        pEsp->m_Pos.x -= SCR_H;
        pEsp->m_Pos.y -= SCR_H;
        EspCommonTrans(pEsp);
        pEsp->m_Pos.x += SCR_H;
        pEsp->m_Pos.y += SCR_H;
    }
}

int cEsp47::SetFreeWork(EspGenWork* gen, u32* seed)
{
    return 1;
}
