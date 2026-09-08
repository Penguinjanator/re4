#include "em.h"
#include "area.h"
#include "player.h"
#include "pl_npc.h"
#include "math_sub.h"

// One light area (0xD8 bytes): the light each character kind gets scaled inside the area.
struct LightAreaData {
    u8 x0;
    u8 x1;
    u8 lightNoPl;    // 0x02  light no for the player (0xFF: none)
    u8 lightNoEm;    // 0x03  light no for the other characters
    u8 area[0x34];   // 0x04  AreaHitCheck data
    s8 power;        // 0x38  scale in percent
    u8 lightNoSub;   // 0x39  light no for the sub character
    u8 pad_3A[0xD8 - 0x3A];
};

struct LightAreaHed {
    u32 num;                // 0x00
    u8 pad_4[0x10 - 0x4];
    LightAreaData data[1];  // 0x10
};

static LightAreaHed* g_pLightAreaHed;

extern "C" {
void LightAreaInit();
int LightAreaDataLoad(LightAreaHed* p);
void LightAreaUpdate();
// pl_wep.h view: the weapon object and the rocket a launcher carries
struct LightAreaWep {
    u8 pad_0[0x34];
    cEm* pObj;   // 0x34
};

struct LightAreaLauncher {
    u8 pad_0[0x384];
    cEm* rocket;  // 0x384
};

static inline void LitAreaSet(EmLightArea* la, u32 bit)
{
    la->flags |= bit;
}

static inline void LitAreaReset(EmLightArea* la, u32 bit)
{
    la->flags &= ~bit;
}

// the player's weapon object (pl_wep.h view) and the rocket a launcher carries
#define WEP_OBJ() (((LightAreaWep*) pPL->pWep)->pObj)
#define WEP_ROCKET(w) (((LightAreaLauncher*) (w))->rocket)

void LightAreaUpdateSub(cEm* em, int type)
{
    static f32 lit_pow_mul = 0.3f;
    Vec pos;
    EmLightArea* la;
    LightAreaHed* hed;
    LightAreaData* d;
    int hit;
    u32 i;
    f32 rate;
    f32 scale;

    pos = em->pos;
    pos.y += 100.0f;
    la = &em->litArea;
    hed = g_pLightAreaHed;
    d = hed->data;
    if (la->chk(2) == 0) {
        la->scale = 1.0f;
    }
    hit = 0;
    rate = 0.05f;
    for (i = 0; i < hed->num; i++, d++) {
        if (type == 0 && d->lightNoPl == 0xFF) {
            continue;
        }
        if (type == 1 && d->lightNoEm == 0xFF) {
            continue;
        }
        if (type == 2 && d->lightNoSub == 0xFF) {
            continue;
        }
        if (AreaHitCheck(d->area, &pos) != 1) {
            continue;
        }
        hit = 1;
        rate = (f32) d->power / 100.0f;
        if (type == 0) {
            la->lightNo = d->lightNoPl;
        } else if (type == 1) {
            la->lightNo = d->lightNoEm;
        } else if (type == 2) {
            la->lightNo = d->lightNoSub;
        }
        break;
    }
    scale = la->scale;
    if (hit == 1) {
        scale += (rate - scale) * lit_pow_mul;
        la->flags |= 2;
    } else {
        scale += (1.0f - scale) * lit_pow_mul;
        if (fabsf(1.0f - scale) < 0.05f) {
            la->flags &= ~2;
        }
    }
    la->scale = scale;
    if (em == pPL) {
        if (WEP_OBJ() != 0) {
            cEm* wep;

            LitAreaSet(&WEP_OBJ()->litArea, 1);
            WEP_OBJ()->litArea.scale = scale;
            WEP_OBJ()->litArea.lightNo = la->lightNo;
            if (la->chk(2)) {
                LitAreaSet(&WEP_OBJ()->litArea, 2);
            } else {
                LitAreaReset(&WEP_OBJ()->litArea, 2);
            }
            wep = WEP_OBJ();
            if (wep->id == 0x23) {
                if (WEP_ROCKET(wep) != 0) {
                    LitAreaSet(&WEP_ROCKET(wep)->litArea, 1);
                    WEP_ROCKET(wep)->litArea.scale = scale;
                    WEP_ROCKET(wep)->litArea.lightNo = la->lightNo;
                    if (pPL->litArea.chk(2)) {
                        LitAreaSet(&WEP_ROCKET(wep)->litArea, 2);
                    } else {
                        LitAreaReset(&WEP_ROCKET(wep)->litArea, 2);
                    }
                }
            }
        }
    }
}

asm(".section .sdata; .balign 8");
