#include "atari.h"
#include "light.h"
#include "esp.h"

struct Esp49Work {
    f32 del_height;     // 0x00 depth below the water surface at which the effect dies
    f32 fade_height; // 0x04 depth over which the alpha fades
    f32 Base_alpha;     // 0x08 base alpha
    u8 estOn;      // 0x0C spawn an est when the effect dies underwater
    u8 EstNo;      // 0x0D
    u8 estPrm;     // 0x0E
};

// Effect that fades out and dies when it sinks below the water surface.
class cEsp49 : public cEsp {
public:
    Esp49Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp49_Create()
{
    return new cEsp49;
}

void cEsp49::move()
{
    Esp49Work* w = &m_Free;
    Vec wpos;
    Vec r;
    Vec ep;
    f32 h;

    m_Col_a = w->Base_alpha;
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
            return;
        }
        w->Base_alpha = m_Col_a;
        if (parent != pEffParentWorldS && (m_Release_time == 0xff || m_Release_time <= m_Life_time)) {
            PSMTXMultVec(parent->mat, &m_Pos, &wpos);
        } else {
            wpos = m_Pos;
        }
        if (GetWaterHeight(&m_Pos, &h)) {
            f32 d = h - m_Pos.y;
            if (d < w->del_height) {
                if (w->estOn & 1) {
                    r.x = r.y = r.z = 0.0f;
                    ep = m_Pos;
                    EstSet(0, -1, &ep, &r, w->EstNo, w->estPrm, info.Core_flg, info.Core_kind, info.Core_pEm, 0);
                }
                PushEsp(this);
            } else if (d < w->fade_height) {
                m_Col_a *= (d - w->del_height) / w->fade_height;
            }
        }
    }
}

int cEsp49::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp49Work* w = &m_Free;

    w->del_height = gen->Vec0.x;
    w->fade_height = gen->Vec0.z;
    if (w->del_height > w->fade_height) {
        w->fade_height = w->del_height;
    }
    w->EstNo = gen->Work8[0];
    w->estPrm = gen->Work8[1];
    w->estOn = gen->Work8[2];
    if (w->estOn > 1) {
        pLog->err(0, 0, "ESP_49 : FLAG[%d] invalid.", w->estOn);
        return 0;
    }
    w->Base_alpha = m_Col_a;
    return 1;
}
