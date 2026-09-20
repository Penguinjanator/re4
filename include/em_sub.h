#ifndef EM_SUB_H
#define EM_SUB_H

#include "types.h"
#include "vec.h"
#include "em.h"
#include "emhit.h"
#include "item.h"

// game/em_sub.cpp: the shared enemy helper library (hit boxes, damage, blood effects, weapon
// target lists, catch motions, item drops). EmGetDmPos, EmDmBloodSet2, EmPlBloodSet2,
// EmAtkLineHitCk, EmAtkSetDamagePL, VehicleAdjust and PlSetDamage are declared in emhit.h.

// GetWepTargetList* entry: the enemy and the hit box that was hit.
struct WepTarget {
    cEm* em;
    YARARE_INFO* part;
};

extern u32 No_drop_cnt;
extern u32 No_drop_cnt2;

extern "C" {
void Em_R0_Scenario(cEm* em);
void EmDmBloodSet(cEm* em);
void EmDmBloodSet3(cEm* em, u32 no, u32 prm, u32 rnd, u16 esp_core_flg, u32 f);
void EmPlBloodSet(cEm* em, Vec* pos, u32 type, u8 eff_id, u8 est_id);
void EmSubBloodSet(cEm* em, Vec* pos, u32 type, u8 eff_id, u8 est_id);
// Hit box of `em` touched by the capsule of the 8-corner `box`; the best one by squared distance.
YARARE_INFO* emBoxAtCk(cEm* em, Vec* box, Vec* pos, int flag);
YARARE_INFO* emLineAtCk(cEm* em, Vec* pPos, Vec* pPos2, f32 len, int flag);
YARARE_INFO* emLineAtCk2(cEm* em, Vec* pPos, Vec* pPos2, f32 len, Vec* out, int flag);
int emLineCapsuleCrossCk(Vec* a, Vec* b, Vec* top, Vec* bottom, f32 r, Vec* hit);
int emLineCubeCrossCk(Vec* a, Vec* b, Mtx m, f32 sx, f32 sy, f32 sz, Vec* ofs, Vec* hit);
int emLinePolyCrossCk(Vec* pPos, Vec* pPos2, Vec* poly, Vec* hit);
YARARE_INFO* emSphereAtCk(cEm* em, Vec* pos, Vec* pos2, f32 r, int flag, f32 r2);
u32 GetWepTargetList(Vec* box, Vec* pos, WepTarget* list, u32 max, int flag);
u32 GetWepTargetList2(Vec* pPos, Vec* pPos2, WepTarget* list, u32 max, Vec* hit, Vec* nrm, u32* attr, int type,
                      int flag);
int GetWepTargetListBomb(Vec* pos, f32 r, WepTarget* list, int max, int type, int flag);
int PlBombHitCk(Vec* pos, f32 r);
int GetWepTargetPos(Vec* pPos, Vec* pPos2, int plCheck, int wepNo, cEm** outEm, int* outAttr);
YARARE_INFO* EmYarareContactCk(cEm* em, Vec* pos, Vec* out, f32 r);
void EmYarareDisp(cEm* em);
void EmScenario(cEm* em);
int LifeDownSet(cEm* em, int dmg, int flag);
int LifeDownSet2(cEm* em, int dmg, int rnd, int flag);
int EmAtkHitCk(EmAtkInfo* info, Vec* pPos, Vec* pPosOld, int noSub);
int EmAtkHitCk2(EmAtkInfo* info, Vec* pPos, Vec* pPosOld);
YARARE_INFO* EmAtkLineHitCkSub(Vec* pPos, Vec* pPos2, Vec* hit, Vec* nrm);
void EmAtkSetDamageSub(YARARE_INFO* part, EmAtkInfo* info, Vec* pPos, Vec* pPos2);
YARARE_INFO* EmAtkHitSubCk2(EmAtkInfo* info, Vec* pPos, Vec* pPosOld);
void EmCatchPLSet(cEm* em, f32 ang, u32 type, int a, f32 x, f32 y, f32 z);
int EmCatchMotionMove(cEm* em, f32 rate, f32 rate2);
int EmRackCk(cEm* em, Vec* pos, f32 ang);
int GetBulletPoint();
void GetDropBullet(int* id, int* num);
int GetRecoveryPoint();
void EmSetDropItem(cEm* em);
void EmReserveDropItem(cEm* em);
void RandomItemSet(cEm* em);
int RandomItemCk(int id, int* outId, int* outNum, int flag);
int CheckInWater(cModel* m, int parts);
int HandgunCk(int wep);
int TrolleyItemSetCk(Vec* pos, ITEM_ID id, int num);
int BullItemSetCk(Vec* pos, ITEM_ID id, int num);
void adjust_add_set(Vec add);
}

// Position of `em` (the player when NULL) plus `t` of its parts 0 movement this frame (C++ linkage;
// Bio4.sym marks it local but the em3c module calls it).
void GetPlPos(Vec* out, cEm* em, f32 t);

#endif
