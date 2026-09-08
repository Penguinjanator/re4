#include "atari.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"

struct Esp0aWork {
    u8 type;   // 0x00 0: spawner (copies itself 50 times), 1: rim-lit sprite
    u8 alpha;  // 0x01 base alpha
};

static f32 RIMIT1 = 0.9f;
static f32 RIMIT2 = 0.6f;
static f32 RIMIT3 = 0.9f;
static f32 RIMIT4 = 0.6f;

// Rim highlight sprite: the alpha fades in when the sprite's direction (spd) points toward
// or away from the camera while the sprite is in front of it.
class cEsp0a : public cEsp {
public:
    Esp0aWork work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

extern "C" {
void Esp0a_Trans(cEsp0a* esp);
void Esp0a_Trans2(cEsp* esp);
}

cEsp* Esp0a_Create()
{
    return new cEsp0a;
}

void cEsp0a::move()
{
    Esp0aWork* w = &work;
    Vec dir;
    Vec vpos;
    Mtx m;
    f32 dot;

    switch (w->type) {
    case 0:
        PushEsp(this);
        break;
    case 1:
        PSMTXConcat(pG->Cam.viewMat, parent->mat, m);
        PSMTXMultVecSR(m, &spd, &dir);
        PSMTXMultVec(m, &pos, &vpos);
        if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f) {
            dir.y = 1.0f;
        }
        if (vpos.x == 0.0f && vpos.y == 0.0f && vpos.z == 0.0f) {
            vpos.y = 1.0f;
        }
#line 51 "D:/Bio4/Prog/esp0a.cpp"
        VECNormalize(&dir, &dir);
        VECNormalize(&vpos, &vpos);
        dot = PSVECDotProduct(&dir, &vpos);
        if (vpos.z < -RIMIT1 && dot < -RIMIT2) {
            colA = (f32)w->alpha * (1.0f - ((-dot - RIMIT2) / (1.0f - RIMIT2)) * (-vpos.z - RIMIT1) / (1.0f - RIMIT1));
        } else if (vpos.z < -RIMIT3 && dot > RIMIT4) {
            colA = (f32)w->alpha * (1.0f - ((dot - RIMIT4) / (1.0f - RIMIT4)) * (-vpos.z - RIMIT3) / (1.0f - RIMIT3));
        } else {
            colA = (f32)w->alpha;
        }
        break;
    }
}

void Esp0a_Trans(cEsp0a* esp)
{
    Esp0aWork* w = &esp->work;
    cEsp* base;
    cEsp* p;
    Vec wpos;
    u32 i;

    switch (w->type) {
    case 0:
        break;
    case 1:
        if (PullEsp(&base, 0)) {
            i = 0;
            *base = *esp;
            base->id = 0;
            for (; i < 50; i++) {
                if (PullEsp(&p, 0)) {
                    *p = *base;
                    p->spd.x = p->spd.y = p->spd.z = 0.0f;
                    p->scaleSpd = 0.0f;
                    p->colRSpd = p->colGSpd = p->colBSpd = p->colASpd = 1.0f;
                    p->xA8 = p->xAA = p->spdCnt = p->scaleCnt = 0;
                    p->life = 1;
                    p->cnt = 0;
                    if (p->parent != pEffParentWorld) {
                        PSMTXMultVec(p->parent->mat, &p->pos, &wpos);
                    } else {
                        wpos = p->pos;
                    }
                    AddOtWorldPos(p, Esp0a_Trans2, &wpos, 8, 0.0f);
                }
                if (!base->CommonMove()) {
                    break;
                }
            }
            if (base->flag & 1) {
                PushEsp(base);
            }
        }
        break;
    }
}

void Esp0a_Trans2(cEsp* esp)
{
    EspCommonTrans(esp);
}

int cEsp0a::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp0aWork* w = &work;
    cEsp* base;
    cEsp* p;
    u32 i;

    if (gen->xC9 != 0) {
        pLog->err(0, 0, "ESP0a : WK1 not 0!!");
    }
    w->type = gen->xCA;
    w->alpha = x83;
    switch (w->type) {
    case 0:
        if (PullEsp(&base, 0)) {
            i = 0;
            *base = *this;
            base->id = 0;
            for (; i < 50; i++) {
                if (PullEsp(&p, 0)) {
                    *p = *base;
                    p->spd.x = p->spd.y = p->spd.z = 0.0f;
                    p->scaleSpd = 0.0f;
                    p->colRSpd = p->colGSpd = p->colBSpd = p->colASpd = 1.0f;
                    p->xA8 = p->xAA = p->spdCnt = p->scaleCnt = 0;
                    p->cnt = 0;
                    p->life = (s8)gen->xC8;
                }
                if (!base->CommonMove()) {
                    break;
                }
            }
            if (base->flag & 1) {
                PushEsp(base);
            }
        }
        break;
    case 1:
        flags |= 0x400;
        dispFlag |= 2;
        break;
    }
    return 1;
}
