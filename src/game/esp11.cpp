#include "atari.h"
#include "light.h"
#include "global.h"
#include "esp.h"

struct Esp11Work {
    u8 CutNo;         // 0x00 light type (gen->xC9)
    u8 LitNo;           // 0x01 light number (gen->xCA)
    u8 Kind;         // 0x02 0/1: create a light, 2: fixed type 8, 3: no light (gen->xC8)
    u8 Type;         // 0x03 1: light follows the sprite (gen->xCB)
    cLight* pLi;  // 0x04
    GXColor Base_col;     // 0x08 base color of the light
    u8 ToolState;    // 0x0C (gen->xFC)
};

// Light source effect: creates a cLight and (mode 1) drives its position and color from the
// sprite.
class cEsp11 : public cEsp {
public:
    Esp11Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
    virtual void Destruct();
};

extern "C" {
void EffSetToolState(int state);
void Esp11_SetParam(cEsp11* esp);
}

cEsp* Esp11_Create()
{
    return new cEsp11;
}

void cEsp11::move()
{
    Esp11Work* w = &m_Free;

    if (w->ToolState != 0) {
        EffSetToolState(w->ToolState);
    }
    if (w->pLi != NULL && (w->pLi->be_flag & 0x201) != 1) {
        w->pLi = NULL;
        PushEsp(this);
    } else if (w->Type == 1) {
        if (CommonMove()) {
            Esp11_SetParam(this);
        }
    } else {
        if (m_Life_max != 0 && m_Life_max <= m_Life_time) {
            PushEsp(this);
        } else {
            m_Life_time++;
        }
    }
}

void Esp11_Trans(cEsp* esp)
{
}

void cEsp11::Destruct()
{
    cLight* l = m_Free.pLi;

    if (l != NULL && (l->be_flag & 0x201) == 1) {
        LightMgr.destroy(l);
    }
}

void Esp11_SetParam(cEsp11* esp)
{
    Esp11Work* w = &esp->m_Free;
    f32 alpha;

    if (w->pLi == NULL) {
        return;
    }
    w->pLi->Pos = esp->m_Pos;
    if (esp->parent != pEffParentWorldS) {
        PSMTXMultVec(esp->parent->mat, &w->pLi->Pos, &w->pLi->Pos);
    }
    w->pLi->Radius = esp->m_Size_base_x * esp->m_Size_mul * 10.0f;
    alpha = esp->m_Col_a / 255.0f;
    w->pLi->Col.r = (u8)(w->Base_col.r * esp->m_Col_r / 255.0f);
    w->pLi->Col.g = (u8)(w->Base_col.g * esp->m_Col_g / 255.0f);
    w->pLi->Col.b = (u8)(w->Base_col.b * esp->m_Col_b / 255.0f);
    w->pLi->Col.a = (u8)(w->Base_col.a * alpha * 0.5f);
}

int cEsp11::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp11Work* w = &m_Free;

    w->Kind = gen->Work8[0];
    w->CutNo = gen->Work8[1];
    w->LitNo = gen->Work8[2];
    w->Type = gen->Work8[3];
    w->ToolState = gen->WorkSp8[0];
    if (w->Kind == 2) {
        w->Kind = 0;
        w->CutNo = 8;
        w->LitNo = 0;
        w->Type = 1;
    }
    if (w->Kind <= 1) {
        w->pLi = LightMgr.create(w->Kind != 0, w->CutNo, w->LitNo, 0);
        if (w->pLi == NULL) {
            pLog->err(0, 0, "ESP11 : LightId[Kind:%d cut:%d no:%d] no data", w->Kind, w->CutNo, w->LitNo);
            return 0;
        }
        w->pLi->be_flag |= 0x40;
    } else if (w->Kind == 3) {
        w->pLi = NULL;
        return 1;
    } else {
        pLog->err(0, 0, "ESP11 : LightKind[%d] invalid", w->Kind);
        return 0;
    }
    w->Base_col = w->pLi->Col;
    if (w->Type == 0) {
    } else if (w->Type == 1) {
        Esp11_SetParam(this);
    } else {
        pLog->err(0, 0, "ESP11 : LightType[%d] invalid", w->Type);
        return 0;
    }
    if (pG->Debug_flg[1] & 0x00800000) {
        pG->Stop_flg &= ~0x01000000;
    }
    return 1;
}
