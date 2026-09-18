#include "atari.h"
#include "esp.h"

struct Esp4fWork {
    u8 area_no;  // 0x00
};

// Effect that is only drawn while inside a given room area.
class cEsp4f : public cEsp {
public:
    Esp4fWork m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp4f_Create()
{
    return new cEsp4f;
}

void cEsp4f::move()
{
    Esp4fWork* w = &m_Free;
    Vec wpos;

    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        } else {
            if (parent == pEffParentWorld) {
                wpos = m_Pos;
            } else {
                PSMTXMultVec(parent->mat, &m_Pos, &wpos);
            }
            if (EffAreaCheckNo(&wpos, w->area_no)) {
                PushEsp(this);
            }
        }
    }
}

int cEsp4f::SetFreeWork(EspGenWork* gen, u32* seed)
{
    m_Free.area_no = gen->Work8[0];
    return 1;
}
