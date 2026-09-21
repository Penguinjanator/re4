#ifndef EST_H
#define EST_H

#include "types.h"
#include "vec.h"
#include "model.h"

// game/est.cpp: effect set table (est) helpers. EstSet itself is declared in esp.h.
extern "C" {
void AreaSstSet(int id);
int GetSstDispFlag(u32 id);
void SetSstDispFlag(u32 id, int on);
void SetSstAddAreaFlag(u32 flag);
void SstSet(u32 owner, int type, int no, int lo, int hi, int move);
void EffectEspDelete(int a, int b, void* c, cModel* model);
void EffectEspgenDelete(int Core_flg, int Core_kind, void* c);
void EffectEfmDelete(int Core_flg, int Core_kind, void* c);
void EffectDeleteAll();
void EffectEventDelete();
void EspDelete(int a, int b, void* c, cModel* model);
void EspDeleteEvent();
void EspSetWaterBomb(Vec* pos);
void EspSetWaterHitmark(Vec* pos);
int EspChkInPuddle(Vec* pos, Vec* nrm);
void EspSetEatEffect(Vec* pos, Vec* nrm, int type, u8 wep);
void EventCutEstSet(int owner, u32 no);
void EventCutEffDelete();
void EventAllEffDelete();
int ChkWaterEffectEnable(Vec* pos);
void EstSetEm10WaterFall(Vec* pos);
}

// Drop effect (owner a, kind b) in all three effect systems (r318, r31b, r31c).
static inline void EffectDelete(int a, int b)
{
    EffectEspDelete(a, b, 0, 0);
    EffectEspgenDelete(a, b, 0);
    EffectEfmDelete(a, b, 0);
}

#endif
