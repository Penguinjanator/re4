#include "atari.h"
#include "light.h"
#include "gx.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"
#include "main_sub.h"
#include "tpl.h"
#include "espgen.h"

// Scrolling-texture sprite (Esp08_Trans) and the heat-shimmer variant (Esp08_TransShimmer).
// TODO: Esp08_Trans / Esp08_TransShimmer are not written yet (unit not linked).
struct Esp08Work {
    f32 rateX;     // 0x00 texture repeat along s (>= 1)
    f32 rateY;     // 0x04 texture repeat along t
    f32 spdX;      // 0x08 scroll speed
    f32 spdY;      // 0x0C
    f32 ofsX;      // 0x10 scroll offset (kept in 0..1)
    f32 ofsY;      // 0x14
    u8 maskType;   // 0x18 0/1
    u8 pad_19[3];
    f32 colA0;     // 0x1C initial alpha (esp->colA)
    u8 fadeFrames; // 0x20 frames the alpha fades in (0: none)
    u8 fadeCnt;    // 0x21
};

class cEsp08 : public cEsp {
public:
    Esp08Work work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

extern "C" {
cEsp* Esp08_Create();
void Esp08_Trans(cEsp08* esp);
void Esp08_TransShimmer(cEsp08* esp, int type);
}

cEsp* Esp08_Create()
{
    return new cEsp08;
}

void cEsp08::move()
{
    Esp08Work* w = &work;

    if (w->fadeFrames != 0) {
        colA = w->colA0;
    }
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
            return;
        }
        w->ofsX += w->spdX;
        w->ofsY += w->spdY;
        while (w->ofsX > 1.0f) {
            w->ofsX -= 1.0f;
        }
        while (w->ofsY > 1.0f) {
            w->ofsY -= 1.0f;
        }
        while (w->ofsX < 0.0f) {
            w->ofsX += 1.0f;
        }
        while (w->ofsY < 0.0f) {
            w->ofsY += 1.0f;
        }
        if (w->fadeFrames != 0) {
            if (pG->flags_5010 & 0x02000000) {
                w->fadeCnt++;
            } else {
                if (w->fadeCnt == 0) {
                    return;
                }
                w->fadeCnt--;
            }
            if (w->fadeCnt == 0) {
                return;
            }
            if (w->fadeCnt >= w->fadeFrames) {
                w->fadeCnt = w->fadeFrames;
            }
            colA = w->colA0 * (1.0f - (f32) w->fadeCnt / (f32) (int) w->fadeFrames);
        }
    }
}

int cEsp08::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp08Work* w = &work;
    u32 type;

    w->rateX = (f32) (int) gen->xC8 * 0.1f + 1.0f;
    w->rateY = (f32) (int) gen->xC9 * 0.1f + 1.0f;
    if (w->rateX < 1.0f) {
        w->rateX = 1.0f;
    }
    if (w->rateY < 1.0f) {
        w->rateY = 1.0f;
    }
    w->spdX = (f32) (s32) gen->prm.w.xCC * 0.001f;
    w->spdY = (f32) (s32) gen->prm.w.xD0 * 0.001f;
    w->ofsX = 0.0f;
    w->ofsY = 0.0f;
    w->fadeFrames = gen->xCB;
    w->colA0 = colA;
    w->maskType = gen->xCA;
    type = w->maskType;
    if (type > 1) {
        pLog->err(0, 0, "ESP08 : MaskType[%x] invalid", type);
        return 0;
    }
    return 1;
}
