#include "atari.h"
#include "light.h"
#include "rnd.h"
#include "esp.h"

// Static sprite showing one selected (or random) animation pattern.
class cEsp4b : public cEsp {
public:
    u32 xF8;

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp4b_Create()
{
    return new cEsp4b;
}

void cEsp4b::move()
{
    CommonMove();
}

int cEsp4b::SetFreeWork(EspGenWork* gen, u32* seed)
{
    EspAnmData* anm;
    u32 ptn;

    if (!EspGetAnmAddr(m_Tex_id, &anm)) {
        pLog->err(0, 0, "ESP : TexId[%x] no data", m_Tex_id);
        return 0;
    }
    ptn = gen->Work8[0];
    if (ptn == 0xff) {
        m_Ptn_no = (Rnd() + Rnd()) % anm->Frames;
    } else {
        if (ptn >= anm->Frames) {
            pLog->err(0, 0, "ESP4B : PtnNo[%d >= %d] over", ptn, anm->Frames);
            return 0;
        }
        m_Ptn_no = ptn;
    }
    return 1;
}
