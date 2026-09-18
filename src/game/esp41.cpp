#include "atari.h"
#include "global.h"
#include "player.h"
#include "em.h"
#include "math_sub.h"
#include "esp.h"

struct Esp41Work {
    f32 Dist;   // 0x00 attraction range
    f32 Pow;   // 0x04
    Vec Offset;     // 0x08 offset from the target position
    u8 Type;     // 0x14
};

// Effect attracted towards the player (or enemy 0 in a cutscene).
class cEsp41 : public cEsp {
public:
    Esp41Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp41_Create()
{
    return new cEsp41;
}

void cEsp41::move()
{
    Esp41Work* w = &m_Free;
    Vec d;
    Vec tgt;
    f32 dist;

    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        } else if (w->Type == 0) {
            cModel* target = pPL;
            if (pG->Debug_flg[1] & 0x00800000) {
                target = EmMgrWork(0);
                if (!(target->be_flag & 1)) {
                    return;
                }
            }
            PSVECAdd(&target->pos, &w->Offset, &tgt);
            PSVECSubtract(&m_Pos, &tgt, &d);
            dist = PSVECMag(&d);
            if (d.x == 0.0f && d.y == 0.0f && d.z == 0.0f) {
                d.y = 1.0f;
            }
#line 90 "D:/Bio4/Prog/esp41.cpp"
            VECNormalize(&d, &d);
            if (dist < w->Dist) {
                PSVECScale(&d, &d, (w->Dist - dist) * w->Pow);
                if (m_Tool_flg & 1) {
                    d.y = 0.0f;
                }
                PSVECAdd(&m_Speed, &d, &m_Speed);
            }
        }
    }
}

int cEsp41::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp41Work* w = &m_Free;

    w->Dist = (f32)(s8)gen->Work8[0] * 100.0f;
    w->Pow = (f32)(s8)gen->Work8[1] * 0.00005f;
    w->Type = gen->WorkSp8[0];
    w->Offset = *(Vec*)&gen->Vec0.x;
    if (w->Type != 0) {
        pLog->err(0, 0, "ESP41 : Type[%x] invalid.", w->Type);
        return 0;
    }
    return 1;
}
