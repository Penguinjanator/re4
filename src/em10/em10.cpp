// em10/em10.cpp: the Ganado enemy library (D:/Bio4/Prog/em10.cpp), the same object in the 16 modules
// em10..em17, em19..em1f, em20 (config/G4BE08/modules.py). cEm10 and its routines, the per-weapon
// damage reactions, the route / attack / find checks and the player-side event routines (plem10*).

#include "sscrn.h"
#include "atari.h"
#include "atari_init.h"
#include "light.h"
#include "dmg.h"
#include "ctrl.h"
#include "map_obj.h"
#include "widget.h"
#include "em10.h"
#include "em_sub.h"
#include "em_set.h"
#include "emhit.h"
#include "emwep.h"
#include "emdoor.h"
#include "emwindow.h"
#include "emswitch.h"
#include "emshield.h"
#include "at_mod.h"
#include "esp.h"
#include "est.h"
#include "embarrel.h"
#include "ctrl.h"
#include "snd.h"
#include "quake.h"
#include "pad.h"
#include "main.h"
#include "act_btn.h"
#include "player.h"
#include "pl_npc.h"
#include "pl_sub.h"
#include "pl_wep.h"
#include "pl_cloth.h"
#include "cockpit.h"
#include "camera.h"
#include "cam_ctrl.h"
#include "route_ck.h"
#include "foot_shadow.h"
#include "dbmodule.h"
#include "mercenaries.h"
#include "motion.h"
#include "game.h"
#include "rnd.h"
#include "global.h"
#include "math_sub.h"
#include "db_log.h"
#include "joy.h"
#include "em_cloth.h"
#include "item.h"
#include "sce_at.h"

// motion.h declares the one-argument form; the enemies pass a second argument (pl_npc.cpp).
u16 MotionMoveF(cModel* m, int flag) asm("MotionMove");

// Routine dispatch tables (.data).
static void em10_R0_Init(cEm10* em);
static void em10_R0_Move(cEm10* em);
static void em10_R0_Damage(cEm10* em);
static void em10_R0_Die(cEm10* em);
static void em10DmSetWep00(cEm10* em);
static void em10DmSetWep02(cEm10* em);
static void em10DmSetWep03(cEm10* em);
static void em10DmSetWep09(cEm10* em);
static void em10DmSetWep23(cEm10* em);
static void em10_R1_br_Dummy(cEm10* em);
static void em10_R1_br_Wait(cEm10* em);
static void em10_R1_Wait(cEm10* em);
static void em10_R1_Keeper(cEm10* em);
static void em10_R1_Hide(cEm10* em);
static void em10_R1_HideFall(cEm10* em);
static void em10_R1_HideJump(cEm10* em);
static void em10_R1_R10CParasite(cEm10* em);
static void em10_R1_R10CPCancel(cEm10* em);
static void em10_R1_R204Prayer(cEm10* em);
static void em10_R1_R222DragonA(cEm10* em);
static void em10_R1_R222DragonB(cEm10* em);
static void em10_R1_R222DragonC(cEm10* em);
static void em10_R1_R227Barrel(cEm10* em);
static void em10_R1_R21BTrolleyJump(cEm10* em);
static void em10_R1_R21BTrolleyJump2(cEm10* em);
static void em10_R1_R303FireDash(cEm10* em);
static void em10_R1_R10FGJump(cEm10* em);
static void em10_R1_R10FGondola(cEm10* em);
static void em10_R1_R209DashSit(cEm10* em);
static void em10_R1_StickClaw(cEm10* em);
static void em10_R1_R11DAppear1(cEm10* em);
static void em10_R1_R11DAppear2(cEm10* em);
static void em10_R1_R212Drill(cEm10* em);
static void em10_R1_R209Gatling(cEm10* em);
static void em10_R1_R201EventWait(cEm10* em);
static void em10_R1_FindLost(cEm10* em);
static void em10_R1_R100WalkStay(cEm10* em);
static void em10_R1_R202Finger(cEm10* em);
static void em10_R1_StayWalk(cEm10* em);
static void em10_R1_AttackWait(cEm10* em);
static void em10_R1_R100TurnWalk(cEm10* em);
static void em10_R1_R100Cliff(cEm10* em);
static void em10_R1_R101Bucket(cEm10* em);
static void em10_R1_R101Suki(cEm10* em);
static void em10_R1_Work(cEm10* em);
static void em10_R1_UFOCatch(cEm10* em);
static void em10_R1_R300TakeAshley(cEm10* em);
static void em10_R1_R30FBullJump(cEm10* em);
static void em10_R1_R320Gatling(cEm10* em);
static void em10_R1_R321DeadBody(cEm10* em);
static void em10_R1_R300Gatling(cEm10* em);
static void em10_R1_R101Cart(cEm10* em);
static void em10_R1_br_EvtDash(cEm10* em);
static void em10_R1_EvtDash(cEm10* em);
static void em10_R1_br_EvtWalk(cEm10* em);
static void em10_R1_EvtWalk(cEm10* em);
static void em10_R1_Pickup(cEm10* em);
static void em10_R1_Find(cEm10* em);
static void em10_R1_C_SawStart(cEm10* em);
static void em10_R1_BombIgnition(cEm10* em);
static void em10_R1_br_Walk(cEm10* em);
static void em10_R1_Walk(cEm10* em);
static void em10_R1_br_Dash(cEm10* em);
static void em10_R1_Dash(cEm10* em);
static void em10_R1_br_Back(cEm10* em);
static void em10_R1_Back(cEm10* em);
static void em10_R1_br_Goto(cEm10* em);
static void em10_R1_Goto(cEm10* em);
static void em10_R1_GuardWalk(cEm10* em);
static void em10_R1_Turn180(cEm10* em);
static void em10_R1_Threat(cEm10* em);
static void em10_R1_SideStep(cEm10* em);
static void em10_R1_HideSide(cEm10* em);
static void em10_R1_AppearSide(cEm10* em);
static void em10_R1_SitDown(cEm10* em);
static void em10_R1_Stay(cEm10* em);
static void em10_R1_RoofWait(cEm10* em);
static void em10_R1_Guard(cEm10* em);
static void em10_R1_DownWakeWait(cEm10* em);
static void em10_R1_DownWake(cEm10* em);
static void em10_R1_Crash(cEm10* em);
static void em10_R1_ClimbOver(cEm10* em);
static void em10_R1_DoorAtk(cEm10* em);
static void em10_R1_RackAtk(cEm10* em);
static void em10_R1_WindowAtk(cEm10* em);
static void em10_R1_LadderClimb(cEm10* em);
static void em10_R1_VLadderClimb(cEm10* em);
static void em10_R1_LadderReset(cEm10* em);
static void em10_R1_JumpDown(cEm10* em);
static void em10_R1_Jump(cEm10* em);
static void em10_R1_JumpUp(cEm10* em);
static void em10_R1_Trade(cEm10* em);
static void em10_R1_Drive(cEm10* em);
static void em10_R1_Catapult(cEm10* em);
static void em10_R1_RockPush(cEm10* em);
static void em10_R1_ParasiteAtk(cEm10* em);
static void em10_R1_ShotBowgun(cEm10* em);
static void em10_R1_ShotRocket(cEm10* em);
static void em10_R1_ShotGatling(cEm10* em);
static void em10_R1_ThrowAxe(cEm10* em);
static void em10_R1_ThrowBomb(cEm10* em);
static void em10_R1_FixBomber(cEm10* em);
static void em10_R1_R305Bomber(cEm10* em);
static void em10_R1_R408Bomber(cEm10* em);
static void em10_R1_RocketWait(cEm10* em);
static void em10_R1_AxeAtk(cEm10* em);
static void em10_R1_ShieldAtk(cEm10* em);
static void em10_R1_TorchFrame(cEm10* em);
static void em10_R1_SukiAtk(cEm10* em);
static void em10_R1_ScytheAtk(cEm10* em);
static void em10_R1_ClawAtk(cEm10* em);
static void em10_R1_br_CSawWalkAtk(cEm10* em);
static void em10_R1_CSawWalkAtk(cEm10* em);
static void em10_R1_ClawWalkAtk(cEm10* em);
static void em10_R1_br_ClawCriAtk(cEm10* em);
static void em10_R1_ClawCriAtk(cEm10* em);
static void em10_R1_ClawCriHit(cEm10* em);
static void plem10_ClawCriHit(cPlayer* pl);
static void em10_R1_br_C_SawAtk(cEm10* em);
static void em10_R1_C_SawAtk(cEm10* em);
static void em10_R1_C_SawHit(cEm10* em);
static void plem10_C_SawHit(cPlayer* pl);
static void em10_R1_br_C_SawCriAtk(cEm10* em);
static void em10_R1_C_SawCriAtk(cEm10* em);
static void em10_R1_C_SawCriHit(cEm10* em);
static void plem10_C_SawCriHit(cPlayer* pl);
static void em10_R1_br_Catch(cEm10* em);
static void em10_R1_Catch(cEm10* em);
static void em10_R1_NeckHang(cEm10* em);
static void plem10_NeckHang(cPlayer* pl);
static void em10_R1_NeckHang_Luis(cEm10* em);
static void subem10_NeckHang_Luis(cSubChar* sub);
static void em10_R1_NeckHang_Ashley(cEm10* em);
static void subem10_NeckHang_Ashley(cSubChar* sub);
static void em10_R1_Backhold(cEm10* em);
static void plem10_Backhold(cPlayer* pl);
static void em10_R1_Bombhold(cEm10* em);
static void plem10_Bombhold(cPlayer* pl);
static void em10_R1_br_DashCatch(cEm10* em);
static void em10_R1_DashCatch(cEm10* em);
static void em10_R1_TakeAway(cEm10* em);
static void subem10_TakeAway(cSubChar* sub);
static void em10_R1_Dm_Small(cEm10* em);
static void em10_R1_Dm_Head(cEm10* em);
static void em10_R1_Dm_Flash(cEm10* em);
static void em10_R1_Dm_Claw(cEm10* em);
static void em10_R1_Dm_Claw_Big(cEm10* em);
static void em10_R1_Dm_Gatling(cEm10* em);
static void em10_R1_Dm_FS(cEm10* em);
static void em10_R1_Dm_KneeKick(cEm10* em);
static void em10_R1_Dm_NeckBreak(cEm10* em);
static void em10_R1_Dm_Showtay(cEm10* em);
static void em10_R1_Dm_Heel(cEm10* em);
static void em10_R1_Dm_DashUp(cEm10* em);
static void em10_R1_Dm_DashDown(cEm10* em);
static void em10_R1_Dm_Blow(cEm10* em);
static void em10_R1_Dm_Fence(cEm10* em);
static void em10_R1_Dm_Ladder(cEm10* em);
static void em10_R1_Dm_Roof(cEm10* em);
static void em10_R1_Dm_KneeDown(cEm10* em);
static void em10_R1_Dm_KnockOut(cEm10* em);
static void em10_R1_Dm_Down(cEm10* em);
static void em10_R1_Dm_Frame(cEm10* em);
static void em10_R1_Dm_TakeAway(cEm10* em);
static void em10_R1_Die_Cramp(cEm10* em);
static void em10_R1_Die_Lost(cEm10* em);
static void em10_R1_Die_Down(cEm10* em);
static void em10_R1_Die_Normal(cEm10* em);
static void em10_R1_Die_RunDown(cEm10* em);
static void em10_R1_Die_Bomb(cEm10* em);
static void plemDmFrame(cPlayer* pl);
static void plem10DmGondolaShake(cPlayer* pl);
static void subem10DmGondolaShake(cSubChar* sub);
static void plemDmMStar(cPlayer* pl);
static void plemDmStun(cPlayer* pl);
static void em10KickAction(cEm10* em);
static void em10KneeDownAction(cEm10* em);
static void plem10Kick(cPlayer* pl);
static void plem10Kick2(cPlayer* pl);
static void em10FSAction(cEm10* em);
static void plem10FS(cPlayer* pl);
static void plem10KneeKick(cPlayer* pl);
static void plem10NeckBreak(cPlayer* pl);
static void plem10Showtay(cPlayer* pl);
static void em10TradeAction(cEm10* em);

// EstSet with the enemy as owner argument (esp.h declares the int form).
void EstSetEm(cModel* em, int b, Vec* pos, Vec* rot, int c, int d, int e, int f, cModel* g, void* h) asm("EstSet");

// COMPILER-DIFF: narrow-argument truncation (AGENTS.md item 4). The original passes -1 to the u16
// count of Ctrl12CntAdd as `li r5, -1`; an int-view declaration reproduces it.
void Ctrl12CntAddI(cCtrl* c, int idx, int add) asm("Ctrl12CntAdd__FP5cCtrliUs");
// COMPILER-DIFF: narrow-argument extension (AGENTS.md item 2): the s16 wait time is sign-extended.
void Ctrl12SetS(cCtrl* c, int idx, s16 val) asm("Ctrl12Set__FP5cCtrliUs");
// COMPILER-DIFF: narrow-argument truncation (AGENTS.md item 4): int-view of the u16 se number / block.
u32 Ctrl11SetSe2I(cCtrl* c, cModel* m, s16 time, int no, int idx, int blk) asm("Ctrl11SetSe2__FP5cCtrlP6cModelsUsiUs");

// Helpers of this unit used before their definition.
int em10CrashCk(cEm10* em);
int em10LostHead(cEm10* em, int a, int b);
void em10BloodSet(cEm10* em, int a);
int em10SetDmVal(cEm10* em);
void em10CoreBreak(cEm10* em, int a);
int em10FindCk2(cEm10* em);
void em10KickHitMark(cEm10* em);
int em10RoofDmCk(cEm10* em);
void em10SetPoint(cEm10* em);
void em1cBloodSet(cEm10* em, int near);
int em10ArmorCk(cEm10* em, int parts);
int em10ChgParasiteCk(cEm10* em);
int em10LostHeadCk(cEm10* em);
int em10ModelInit(cEm10* em);
void em10InitRtnSet(cEm10* em);
void Em1fClothSet(cModel* m, PlCloth* c);
extern "C" int EspDataLoad(u32 addr, u32 owner, int flag);
extern "C" cObj* SetObj16(void* bin, void* tpl, cModel* target, cModel* body, int partsNo, u8 type, Vec* pos, Vec* rot);
void em10SetWaitMotion(cEm10* em, int a);
void em10SetWalkMotion(cEm10* em, int a);
void em10BeltSet(cEm10* em);
void em10ChainSet(cEm10* em);
int em10GotoCk(cEm10* em);
int em10FindCk(cEm10* em, int a);
void em10FindNotify(cEm10* em);
void em10WalkRtnSet(cEm10* em);
void em10HandSet(cEm10* em, int a);
void em10ActEvtSetTrade(cEm10* em);
int em10AtkRtnCk(cEm10* em, int a);
void em10SetDashMotion(cEm10* em);
void em10HeadSet(cEm10* em, int a);
void em10DragonFireCk(cEm10* em);
int em10JumpDownCk(cEm10* em);
cObjGondola* em10GetGondola(cEm10* em);
void em10CallVoiceSe2(cEm10* em, int no, int a);
cEmWep* em10MakeWeapon(cEm10* em, int type);
void em10WeaponSet(cEm10* em);
int em10ClimbOverCk(cEm10* em);
void em10SetDamageVoice(cEm10* em, int a, int b);
extern "C" void OSReport(const char* fmt, ...);
void em10RouteCk(cEm10* em);
void em10ClawMove(cEm10* em);
void em10NeckMove(cEm10* em);
void em10WaistMove(cEm10* em);
void em10SlopeMove(cEm10* em);
void em10ScaleCompress(cEm10* em);
void em10BombNeckMove(cEm10* em);
void em10ChainSawMove(cEm10* em);
void Em1fClothMove(cModel* m, PlCloth* c);
void em10BowgunMove(cEm10* em);
void em10SetParasite(cEm10* em);
void em10SetWaterEff(cEm10* em);
void em10FootSe(cEm10* em);
void em10GatlingRollMove(cEm10* em);
int em10CsawHitCk(cEm10* em);
int em10DoorOpenCk(cEm10* em, int a);
int em10RackBreakCk(cEm10* em);
int em10LadderClimbCk(cEm10* em);
int em10VLadderClimbCk(cEm10* em);
int em10LadderResetCk(cEm10* em);
int em10JumpCk(cEm10* em);
int em10WindowCk(cEm10* em);
int em10CatchCk(cEm10* em);
int em10SomebodyNearCk(cEm10* em);
u32 em10GetWanderRoute(cEm10* em);
int em10CatchSubCk(cEm10* em);
void em10ReturnStartPosCk(cEm10* em);
void em10BreathSe(cEm10* em);
void em10SetCrash(cEm10* em, f32 r);
void em10MouthPartsReset(cEm10* em);
void em10SetDmWaterEff(cEm10* em, int a);
void em10BombThrow(cEm10* em);
int em10ThrowBombCk(cEm10* em);
int em10SetDamageDoor(cEm10* em, int a);
void em10SetDamageRack(cEm10* em, int a);
void em10AtkCk(cEm10* em, Vec* a, Vec* b, int c, int d);
int em10IgnitionCk(cEm10* em);
int em10ClawStickCK(cEm10* em);
int em10FindLostCk(cEm10* em);
int em10GotoPosCk(cEm10* em);
extern "C" int em10ReturnPosCk(cEm10* em);
extern "C" void em10ClothPartsSet(cEm10* em, int on);
extern "C" void em10GoodsPartsSet(cEm10* em, int on);
extern "C" int em10AtkDoorCk(cEm10* em);
extern "C" int em10AtkRackCk(cEm10* em);
extern "C" void Em10SetSeTbl(cEm10* em, int type);
extern "C" void em10WeaponSet2(cEm10* em);
extern "C" int em10CatchSubRtnCk(cEm10* em);
extern "C" void em10SetRtnFind(cEm10* em);
extern "C" int em10BullJumpCk(cEm10* em);
extern "C" cModel* em10SearchTruck(cEm10* em);
extern "C" void em10CallVoiceSe(cEm10* em, u16 no);
extern "C" int em10RouteTargetSet(cEm10* em);
extern "C" int em10SomebodyDamageNowCk(cEm10* em);
extern "C" int em10TorchFrameAtkCk(cEm10* em);
extern "C" int em10TorchFrameAtkCkSub(cEm10* em);
extern "C" void em10PlHeadLost();
extern "C" f32 em10GetPower(cEm10* em);
extern "C" void em10WepSeEffSet(cEm10* em, cEmWep* wep, u8 type);
extern "C" int em10ThrowScaCk(cEm10* em);
extern "C" int em10ThrowNearCk(cEm10* em);
extern "C" int em10WindowCk2(cEm10* em);
extern "C" int em10ClimbOverCk2(cEm10* em);
extern "C" void cModel_swapModelInfo(cModel* m, ModelData* old, cModelInfo* info);
extern "C" void plem10KickCamMove(cPlayer* pl, int a);
int em10HideRtnCk2(cEm10* em);
extern "C" void em10SetAccesory(cEm10* em);
extern "C" void em10WeaponInit(cEm10* em);
extern "C" void em10ShieldSet(cEm10* em);
extern "C" int em10DootAtkCk(cEm10* em);
extern "C" int em10ScreenInCk(cEm10* em);
extern "C" int em10SetWanderRoute(cEm10* em);
extern "C" void em10CamMoveTakeaway(cEm10* em);
extern FootShadowTbl Em10_fs_tbl;
extern "C" void em10BellAtkCk(cEm10* em, Vec* pos, u32 no);
extern "C" int em10ShotGatlingCk(cEm10* em);
extern "C" int em10GoSubStayCk(cEm10* em);
extern "C" int em10ReturnCk(cEm10* em);
extern "C" int em10BombThrowScaCk(cEm10* em);
extern "C" void em10CsawSignSe(cEm10* em);
extern "C" int em10ShotBowgunCk(cEm10* em);
extern "C" int em10ThreatCk(cEm10* em);
extern "C" int em10ClawCriAtkCk(cEm10* em);
extern "C" void em10SetTakeawayPosUpdate(cEm10* em);
int em10HideToStepCk(cEm10* em, int a);
extern "C" int em10ParasiteAtkCk(cEm10* em);
extern "C" int em10ShieldAtkCk(cEm10* em);
extern "C" int em10AxeAtkCk(cEm10* em);
extern "C" int em10SukiAtkCk(cEm10* em);
extern "C" int em10ScytheAtkCk(cEm10* em);
extern "C" int em10ClawAtkCk(cEm10* em);
extern "C" int em10ShotRocketCk(cEm10* em);
extern "C" int em10ThrowAxeCk(cEm10* em);
extern "C" int em10CsawAtkCk(cEm10* em);
extern "C" int em10CatchPLRtnCk(cEm10* em);
extern "C" int em10BackCk(cEm10* em);

// Dead flag test (cDmgInfo upper 16 bits): an inline returning 0/1 gives the `li 1; andis.; bne; li 0` chain.
static inline int em10DeadCk(cEm* em)
{
    return (em->flags_324 & 0xFFFF0000) ? 1 : 0;
}

// Reference store (same mechanism as FSet): keeps the following global load after the store.
static inline void U16Set(u16& d, u16 v) { d = v; }

static inline int em10DmgDeadCk(cDmgInfo* d)
{
    return (*(u32*) d & 0xFFFF0000) ? 1 : 0;
}

#define EM10_WINDOW(w) ((w)->pWindow)

// Routine test on the cModel status word (xFC / xFD as the upper half of `stat`).
#define EM_RTN(em, fc, fd) (((em)->stat & 0xFFFF0000) == (u32) (((fc) << 24) | ((fd) << 16)))

// The bell / rung point (emwep.cpp): the byte-pointer copy keeps the pG reload before the next store.
#define SET_BELL_POS(pos) memcpy((u8*) pG + ((u32) &((GlobalWork*) 0)->bell_pos), pos, sizeof(Vec))

// Routine bytes written through an int inline (player.cpp PlRoutineSet): the stores come out
// in the target's order.
static inline void EmRoutineSet(cEm* em, int r0, int r1, int r2, int r3)
{
    em->xFC = r0;
    em->xFD = r1;
    em->xFE = r2;
    em->xFF = r3;
}

#define G_ROOM_ID32 (*(u32*) &pG->stage_no)
#define EM_RTN_SET(em, fc, fd) ((em)->stat = (u32) (((fc) << 24) | ((fd) << 16)))

Em10Func Em10SetFunc = 0;

static Em10Func Em10_R0_move_tbl[5] = {
    em10_R0_Init,
    em10_R0_Move,
    em10_R0_Damage,
    em10_R0_Die,
    (Em10Func) Em_R0_Scenario,
};

// Routine 1 handlers: the branch check and the move of each sub-routine (cModel::xFD).
struct Em10Routine {
    Em10Func br;
    Em10Func move;
};

static Em10Routine Em10_R1_move_tbl[110] = {
    { em10_R1_br_Wait, em10_R1_Wait },  // 0x00
    { em10_R1_br_Dummy, em10_R1_Keeper },  // 0x01
    { em10_R1_br_Dummy, em10_R1_Hide },  // 0x02
    { em10_R1_br_Dummy, em10_R1_HideFall },  // 0x03
    { em10_R1_br_Dummy, em10_R1_HideJump },  // 0x04
    { em10_R1_br_Dummy, em10_R1_R100TurnWalk },  // 0x05
    { em10_R1_br_Dummy, em10_R1_R100Cliff },  // 0x06
    { em10_R1_br_Dummy, em10_R1_R101Bucket },  // 0x07
    { em10_R1_br_Dummy, em10_R1_R101Suki },  // 0x08
    { em10_R1_br_Dummy, em10_R1_R101Cart },  // 0x09
    { em10_R1_br_EvtDash, em10_R1_EvtDash },  // 0x0A
    { em10_R1_br_EvtWalk, em10_R1_EvtWalk },  // 0x0B
    { em10_R1_br_Dummy, em10_R1_Pickup },  // 0x0C
    { em10_R1_br_Dummy, em10_R1_Find },  // 0x0D
    { em10_R1_br_Dummy, em10_R1_C_SawStart },  // 0x0E
    { em10_R1_br_Dummy, em10_R1_BombIgnition },  // 0x0F
    { em10_R1_br_Walk, em10_R1_Walk },  // 0x10
    { em10_R1_br_Dash, em10_R1_Dash },  // 0x11
    { em10_R1_br_Back, em10_R1_Back },  // 0x12
    { em10_R1_br_Goto, em10_R1_Goto },  // 0x13
    { em10_R1_br_Dummy, em10_R1_GuardWalk },  // 0x14
    { em10_R1_br_Dummy, em10_R1_Turn180 },  // 0x15
    { em10_R1_br_Dummy, em10_R1_Threat },  // 0x16
    { em10_R1_br_Dummy, em10_R1_SideStep },  // 0x17
    { em10_R1_br_Dummy, em10_R1_HideSide },  // 0x18
    { em10_R1_br_Dummy, em10_R1_AppearSide },  // 0x19
    { em10_R1_br_Dummy, em10_R1_SitDown },  // 0x1A
    { em10_R1_br_Dummy, em10_R1_Stay },  // 0x1B
    { em10_R1_br_Dummy, em10_R1_RoofWait },  // 0x1C
    { em10_R1_br_Dummy, em10_R1_Guard },  // 0x1D
    { em10_R1_br_Dummy, em10_R1_DownWakeWait },  // 0x1E
    { em10_R1_br_Dummy, em10_R1_DownWake },  // 0x1F
    { em10_R1_br_Dummy, em10_R1_ParasiteAtk },  // 0x20
    { em10_R1_br_Dummy, em10_R1_ShotBowgun },  // 0x21
    { em10_R1_br_Dummy, em10_R1_ShotRocket },  // 0x22
    { em10_R1_br_Dummy, em10_R1_ShotGatling },  // 0x23
    { em10_R1_br_Dummy, em10_R1_ThrowAxe },  // 0x24
    { em10_R1_br_Dummy, em10_R1_ThrowBomb },  // 0x25
    { em10_R1_br_Dummy, em10_R1_AxeAtk },  // 0x26
    { em10_R1_br_Dummy, em10_R1_ShieldAtk },  // 0x27
    { em10_R1_br_Dummy, em10_R1_TorchFrame },  // 0x28
    { em10_R1_br_Dummy, em10_R1_SukiAtk },  // 0x29
    { em10_R1_br_Dummy, em10_R1_ScytheAtk },  // 0x2A
    { em10_R1_br_Dummy, em10_R1_ClawAtk },  // 0x2B
    { em10_R1_br_Dummy, em10_R1_ClawWalkAtk },  // 0x2C
    { em10_R1_br_ClawCriAtk, em10_R1_ClawCriAtk },  // 0x2D
    { em10_R1_br_Dummy, em10_R1_ClawCriHit },  // 0x2E
    { em10_R1_br_C_SawAtk, em10_R1_C_SawAtk },  // 0x2F
    { em10_R1_br_Dummy, em10_R1_C_SawHit },  // 0x30
    { em10_R1_br_C_SawCriAtk, em10_R1_C_SawCriAtk },  // 0x31
    { em10_R1_br_Dummy, em10_R1_C_SawCriHit },  // 0x32
    { em10_R1_br_Catch, em10_R1_Catch },  // 0x33
    { em10_R1_br_Dummy, em10_R1_NeckHang },  // 0x34
    { em10_R1_br_Dummy, em10_R1_NeckHang_Luis },  // 0x35
    { em10_R1_br_Dummy, em10_R1_NeckHang_Ashley },  // 0x36
    { em10_R1_br_Dummy, em10_R1_Backhold },  // 0x37
    { em10_R1_br_Dummy, em10_R1_Bombhold },  // 0x38
    { em10_R1_br_DashCatch, em10_R1_DashCatch },  // 0x39
    { em10_R1_br_Dummy, em10_R1_TakeAway },  // 0x3A
    { em10_R1_br_Dummy, em10_R1_Crash },  // 0x3B
    { em10_R1_br_Dummy, em10_R1_ClimbOver },  // 0x3C
    { em10_R1_br_Dummy, em10_R1_DoorAtk },  // 0x3D
    { em10_R1_br_Dummy, em10_R1_RackAtk },  // 0x3E
    { em10_R1_br_Dummy, em10_R1_WindowAtk },  // 0x3F
    { em10_R1_br_Dummy, em10_R1_LadderClimb },  // 0x40
    { em10_R1_br_Dummy, em10_R1_VLadderClimb },  // 0x41
    { em10_R1_br_Dummy, em10_R1_LadderReset },  // 0x42
    { em10_R1_br_Dummy, em10_R1_JumpDown },  // 0x43
    { em10_R1_br_Dummy, em10_R1_Jump },  // 0x44
    { em10_R1_br_Dummy, em10_R1_JumpUp },  // 0x45
    { em10_R1_br_Dummy, em10_R1_Trade },  // 0x46
    { em10_R1_br_Dummy, em10_R1_Drive },  // 0x47
    { em10_R1_br_Dummy, em10_R1_Catapult },  // 0x48
    { em10_R1_br_Dummy, em10_R1_RockPush },  // 0x49
    { em10_R1_br_Dummy, em10_R1_R10CParasite },  // 0x4A
    { em10_R1_br_Dummy, em10_R1_R10CPCancel },  // 0x4B
    { em10_R1_br_Dummy, em10_R1_R100WalkStay },  // 0x4C
    { em10_R1_br_Dummy, em10_R1_R202Finger },  // 0x4D
    { em10_R1_br_Dummy, em10_R1_StayWalk },  // 0x4E
    { em10_R1_br_Dummy, em10_R1_AttackWait },  // 0x4F
    { em10_R1_br_Dummy, em10_R1_FixBomber },  // 0x50
    { em10_R1_br_Dummy, em10_R1_R204Prayer },  // 0x51
    { em10_R1_br_Dummy, em10_R1_R222DragonA },  // 0x52
    { em10_R1_br_Dummy, em10_R1_R222DragonB },  // 0x53
    { em10_R1_br_Dummy, em10_R1_R222DragonC },  // 0x54
    { em10_R1_br_Dummy, em10_R1_R227Barrel },  // 0x55
    { em10_R1_br_Dummy, em10_R1_R10FGJump },  // 0x56
    { em10_R1_br_Dummy, em10_R1_R10FGondola },  // 0x57
    { em10_R1_br_Dummy, em10_R1_R209DashSit },  // 0x58
    { em10_R1_br_Dummy, em10_R1_StickClaw },  // 0x59
    { em10_R1_br_Dummy, em10_R1_R11DAppear1 },  // 0x5A
    { em10_R1_br_Dummy, em10_R1_R11DAppear2 },  // 0x5B
    { em10_R1_br_Dummy, em10_R1_R212Drill },  // 0x5C
    { em10_R1_br_Dummy, em10_R1_FindLost },  // 0x5D
    { em10_R1_br_Dummy, em10_R1_R201EventWait },  // 0x5E
    { em10_R1_br_Dummy, em10_R1_R209Gatling },  // 0x5F
    { em10_R1_br_Dummy, em10_R1_RocketWait },  // 0x60
    { em10_R1_br_Dummy, em10_R1_R21BTrolleyJump },  // 0x61
    { em10_R1_br_Dummy, em10_R1_R21BTrolleyJump2 },  // 0x62
    { em10_R1_br_Dummy, em10_R1_R303FireDash },  // 0x63
    { em10_R1_br_Dummy, em10_R1_Work },  // 0x64
    { em10_R1_br_Dummy, em10_R1_UFOCatch },  // 0x65
    { em10_R1_br_Dummy, em10_R1_R300TakeAshley },  // 0x66
    { em10_R1_br_Dummy, em10_R1_R30FBullJump },  // 0x67
    { em10_R1_br_Dummy, em10_R1_R320Gatling },  // 0x68
    { em10_R1_br_Dummy, em10_R1_R300Gatling },  // 0x69
    { em10_R1_br_Dummy, em10_R1_R305Bomber },  // 0x6A
    { em10_R1_br_Dummy, em10_R1_R321DeadBody },  // 0x6B
    { em10_R1_br_Dummy, em10_R1_R408Bomber },  // 0x6C
    { em10_R1_br_CSawWalkAtk, em10_R1_CSawWalkAtk },  // 0x6D
};

static Em10Func Em10_R1_dmg_tbl[22] = {
    em10_R1_Dm_Small,
    em10_R1_Dm_Head,
    em10_R1_Dm_DashUp,
    em10_R1_Dm_DashDown,
    em10_R1_Dm_Blow,
    em10_R1_Dm_Fence,
    em10_R1_Dm_Ladder,
    em10_R1_Dm_Roof,
    em10_R1_Dm_KneeDown,
    em10_R1_Dm_KnockOut,
    em10_R1_Dm_Down,
    em10_R1_Dm_Frame,
    em10_R1_Dm_TakeAway,
    em10_R1_Dm_Flash,
    em10_R1_Dm_Claw,
    em10_R1_Dm_Claw_Big,
    em10_R1_Dm_FS,
    em10_R1_Dm_Gatling,
    em10_R1_Dm_Showtay,
    em10_R1_Dm_Heel,
    em10_R1_Dm_KneeKick,
    em10_R1_Dm_NeckBreak,
};

static Em10Func Em10_R1_die_tbl[6] = {
    em10_R1_Die_Cramp,
    em10_R1_Die_Down,
    em10_R1_Die_Normal,
    em10_R1_Die_Lost,
    em10_R1_Die_RunDown,
    em10_R1_Die_Bomb,
};

// Damage reaction per weapon id (cEm::dmWep).
static Em10Func Em10DmSetWep_tbl[46] = {
    em10DmSetWep00, em10DmSetWep02, em10DmSetWep02, em10DmSetWep02,
    em10DmSetWep02, em10DmSetWep09, em10DmSetWep09, em10DmSetWep03,
    em10DmSetWep03, em10DmSetWep02, em10DmSetWep02, em10DmSetWep02,
    em10DmSetWep02, em10DmSetWep09, em10DmSetWep02, em10DmSetWep09,
    em10DmSetWep02, em10DmSetWep02, em10DmSetWep09, em10DmSetWep09,
    em10DmSetWep00, em10DmSetWep02, em10DmSetWep02, em10DmSetWep23,
    em10DmSetWep02, em10DmSetWep02, em10DmSetWep02, em10DmSetWep02,
    em10DmSetWep09, em10DmSetWep02, em10DmSetWep02, em10DmSetWep02,
    em10DmSetWep02, em10DmSetWep03, em10DmSetWep00, em10DmSetWep00,
    em10DmSetWep00, em10DmSetWep00, em10DmSetWep02, em10DmSetWep02,
    em10DmSetWep02, em10DmSetWep09, em10DmSetWep23, em10DmSetWep02,
    em10DmSetWep09, em10DmSetWep09,
};

// ---------------------------------------------------------------------------------------------------

cEm10::~cEm10()
{
    Em10Work* w = EM10_WK(this);

    if (w->pWep) {
        if (w->pWep->isAlive()) {
            EmMgr.destroy(w->pWep);
        }
        w->pWep = 0;
        w->wepType = 0;
    }
    if (w->pWep2) {
        if (w->pWep2->isAlive()) {
            EmMgr.destroy(w->pWep2);
        }
        w->pWep2 = 0;
        w->wep2Type = 0;
    }
    if (w->x178) {
        if (w->x178->isAlive()) {
            ObjMgr.destroy(w->x178);
        }
        w->x178 = 0;
    }
    if (w->x17C) {
        if (w->x17C->isAlive()) {
            ObjMgr.destroy(w->x17C);
        }
        w->x17C = 0;
    }
    if (w->pHead && w->mot[51]) {
        MotionSetCore(w->pHead, MOTION(w->pHead), w->mot[51], 0, 0, 0, 0);
        atariInitF(&w->pHead->atari, 0.0f, 150.0f, -300.0f, 300.0f, 300.0f, 300.0f, 150.0f, 1, 0x2000, 10);
        w->pHead->atari.flags &= ~0x100;
        w->pHead->atari.flags |= 0x200;
        w->pHead->atari.flags |= 0x10;
        w->pHead = 0;
    }
    if (w->wepType == 4) {
        SndStop(w->sndId, 0);
    }
    if (w->x590) {
        if (w->x590->isAlive()) {
            ObjMgr.destroy((cObj*) w->x590);
        }
        w->x590 = 0;
    }
    if (w->x594) {
        if (w->x594->isAlive()) {
            ObjMgr.destroy((cObj*) w->x594);
        }
        w->x594 = 0;
    }
    if (w->pParasite) {
        if (w->pParasite->isAlive()) {
            ObjMgr.destroy(w->pParasite);
        }
        w->pParasite = 0;
    }
    if (w->x58C) {
        if (w->x58C->isAlive()) {
            w->x58C->setReset();
        }
        w->x58C = 0;
    }
}

void cEm10::setNoSuspend(int on)
{
    Em10Work* w = EM10_WK(this);
    u32 i;

    if (on) {
        be_flag |= 0x800;
    } else {
        be_flag &= ~0x800;
    }
    if (w->pWep) {
        if (w->pWep->isAlive()) {
            w->pWep->setNoSuspend(on);
        } else {
            w->pWep = 0;
        }
    }
    if (w->pWep2) {
        if (w->pWep2->isAlive()) {
            w->pWep2->setNoSuspend(on);
        } else {
            w->pWep2 = 0;
        }
    }
    if (w->x178) {
        if (w->x178->isAlive()) {
            w->x178->setNoSuspend(on);
        } else {
            w->x178 = 0;
        }
    }
    if (w->x17C) {
        if (w->x17C->isAlive()) {
            w->x17C->setNoSuspend(on);
        } else {
            w->x17C = 0;
        }
    }
    if (w->pHead) {
        if (w->pHead->isAlive()) {
            w->pHead->setNoSuspend(on);
        } else {
            w->pHead = 0;
        }
    }
    if (w->pShield) {
        if (w->pShield->isAlive()) {
            w->pShield->setNoSuspend(on);
        } else {
            w->pShield = 0;
        }
    }
    if (w->x590) {
        if (w->x590->isAlive()) {
            w->x590->setNoSuspend(on);
        } else {
            w->x590 = 0;
        }
    }
    if (w->x594) {
        if (w->x594->isAlive()) {
            w->x594->setNoSuspend(on);
        } else {
            w->x594 = 0;
        }
    }
    if (w->x58C) {
        if (w->x58C->isAlive()) {
            w->x58C->setNoSuspend(on);
        } else {
            w->x58C = 0;
        }
    }
    if (w->pParasite) {
        if (w->pParasite->isAlive()) {
            w->pParasite->setNoSuspend(on);
        } else {
            w->pParasite = 0;
        }
    }
    for (i = 0; i < 5; i++) {
        if (w->x578[i]) {
            if (w->x578[i]->isAlive()) {
                w->x578[i]->setNoSuspend(on);
            } else {
                w->x578[i] = 0;
            }
        }
    }
}





void em10DmCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int dmg;

    if (em10CrashCk(em)) {
        return;
    }
    if ((em->be_flag & 2) && !em10DeadCk(em) && em->hp > 0) {
        switch (DmgMgr.hitCheck(&em->pos, 0)) {
        case 1:
        case 4:
        case 5:
        case 7:
            if (w->x68C == 0) {
                if (w->pGatling) {
                    w->x68C = 120;
                    EstSetEm(em, -1, 0, 0, 0x10, 0x1E, 0, 0, em, 0);
                    LifeDownSet(em, 1200, 0);
                    if (em->hp > 0) {
                        if (w->x6C3 == 0) {
                            em->xFE = 4;
                        }
                    } else {
                        em->xFE = 6;
                    }
                    return;
                }
                if (EM_RTN(em, 1, 0x5E) && (w->flags & 8)) {
                    w->x68C = 120;
                    EstSetEm(em, -1, 0, 0, 0x10, 0x1E, 0, 0, em, 0);
                    return;
                }
                w->x68C = 120;
                EstSetEm(em, -1, 0, 0, 0x10, 0x1E, 0, 0, em, 0);
                if (w->flags & 0x4000) {
                    LifeDownSet(em, 500, 0);
                    EmRoutineSet(em, 2, 0xC, 0, 0);
                } else if (w->flags & 0x20000) {
                    LifeDownSet(em, 500, 0);
                    EmRoutineSet(em, 2, 5, 0, 0);
                } else if (w->flags & 0x10000) {
                    LifeDownSet(em, 500, 0);
                    EmRoutineSet(em, 2, 6, 0, 0);
                } else if (w->flags & 0x10000000) {
                    LifeDownSet(em, 500, 0);
                    if (em->hp > 0) {
                        return;
                    }
                    EmRoutineSet(em, 2, 4, 0, 0);
                } else if (w->flags & 0x100000) {
                    LifeDownSet(em, 500, 0);
                    EmRoutineSet(em, 2, 4, 0, 0);
                } else if (em->type == 0xA || em->type == 0xD) {
                    LifeDownSet(em, 200, 0);
                    if (em->hp <= 0) {
                        EmRoutineSet(em, 2, 0xB, 0, 1);
                    } else {
                        em->x328 = pPL->pos;
                        EmRoutineSet(em, 2, 0xE, 0, 0);
                    }
                } else if (em->type == 2) {
                    LifeDownSet(em, 200, 0);
                    if (em->hp <= 0) {
                        EmRoutineSet(em, 2, 0xB, 0, 1);
                    } else {
                        EmRoutineSet(em, 2, 0x11, 0, 0);
                    }
                } else if (w->flags & 0x10) {
                    LifeDownSet(em, 500, 0);
                    if (em->hp <= 0) {
                        EmRoutineSet(em, 3, 1, 0, 0);
                    } else {
                        w->x67A = 0;
                        EmRoutineSet(em, 2, 0xA, 0, 0);
                    }
                } else {
                    EmRoutineSet(em, 2, 0xB, 0, 1);
                }
                return;
            }
            break;
        }
    }
    if (em->dmHit == 0) {
        return;
    }
    if ((w->flags & 0x200000) && w->pLadder) {
        w->pLadder->setDown2();
        w->pLadder = 0;
    }
    if (w->pTruck) {
        em->dmHit = 0;
        if (em->hp <= 0) {
            return;
        }
        em10LostHead(em, 0, 0);
        em->hp = 0;
        GameAddPoint(9);
        return;
    }
    if (w->x564) {
        em->dmHit = 0;
        if (em->hp <= 0) {
            return;
        }
        if (em->dmPart->partsNo == 5) {
            em10LostHead(em, 0, 0);
        } else {
            em10BloodSet(em, 0);
        }
        em->hp = 0;
        GameAddPoint(9);
        return;
    }
    if (EM_RTN(em, 1, 0x5E) && (w->flags & 8)) {
        em10BloodSet(em, 0);
        if (!(pG->flags_5010 & 0x20000000)) {
            BitOn(pG->flags_5010, 0x20000000);
            SET_BELL_POS(&em->pos);
            pG->bell_stat = 0;
        }
        em->dmHit = 0;
        return;
    }
    dmg = em10SetDmVal(em);
    LifeDownSet2(em, dmg, 0, 0);
    w->x686 -= dmg;
    if (w->x686 & 0x8000) {
        w->x686 = 0;
    }
    if (EM_RTN(em, 1, 0x5F)) {
        em->dmHit = 0;
        em10BloodSet(em, 0);
        if (em->hp > 0) {
            if (w->x6C3 == 0) {
                em->xFE = 4;
            }
        } else {
            em->xFE = 6;
        }
        return;
    }
    if (em->type == 6) {
        if (em->dmWep == 0x14) {
            return;
        }
        if (em->dmWep == 0x16) {
            return;
        }
        if (em->dmWep == 0x17) {
            return;
        }
        if (em->dmWep == 0x2A) {
            return;
        }
        if (em->dmWep == 0xE) {
            return;
        }
        em->hp = 0;
    }
    if (em->hp <= 0) {
        em10CoreBreak(em, 0);
    }
    if (em->dmWep <= 0x2D) {
        u32 no = em->dmWep;
        if ((em->dmWep == 9 || em->dmWep == 10) && w->wepType == 4) {
            no = 5;
        }
        Em10DmSetWep_tbl[no](em);
    } else {
        pLog->err(0, 0, "em10DmCk() -> Wep no over max!!!");
    }
    if (em->dmWep == 0x10) {
        em->dmType = 0x11;
    }
    if (em10FindCk2(em)) {
        w->flags &= ~0x20000000;
        em->setFindPL();
    }
    if ((s16) w->x660 < 300) {
        w->x660 = 300;
    }
    switch (w->x5EC) {
    case 6:
    case 7:
    case 8:
    case 10:
    case 11:
    case 12:
    case 13:
        if (em->type == 0xA || em->type == 0xD) {
            if (em10FindCk2(em)) {
                w->flags &= ~0x20000000;
                em->setFindPL();
            }
        } else {
            em->setFindPL();
            w->x5EC = 0;
        }
        break;
    }
    if (em->x3D0 == 0 || em->x3D0 == 2) {
        w->x6B8 = 1;
    }
    if (w->wepType == 9) {
        int parts = 9;
        EmHitInfo* part = em->dmPart;
        if (em->flags_3C8 & 0x1000000) {
            parts = 0xF;
        }
        if (part->partsNo == parts) {
            w->x640 = 3;
            GameAddPoint(9);
        }
    }
    if (!(pG->flags_5010 & 0x20000000)) {
        BitOn(pG->flags_5010, 0x20000000);
        SET_BELL_POS(&em->pos);
        pG->bell_stat = 0;
    }
    em->dmHit = 0;
}

static void em10DmSetWep00(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EmHitInfo* part = em->dmPart;

    em->dmg.set(0, 8);
    if ((part->partsNo == 5 || part->partsNo == 0x25) && w->pParasite) {
        SndCall(8, 0x83, &em->pos, em->id, 0, em);
    } else {
        SndCall(8, 0xC, &em->pos, em->id, 0, em);
    }
    if (em->dmWep == 0x25) {
        EmRoutineSet(em, 2, 0x12, 0, 0);
        return;
    }
    em10KickHitMark(em);
    if ((s16) w->x682 != 0) {
        w->x68C = 120;
        EmRoutineSet(em, 2, 0xB, 0, 0);
    } else if (w->flags & 0x4000) {
        EmRoutineSet(em, 2, 0xC, 0, 0);
    } else if (w->flags & 0x20000) {
        if (em->hp > 0 && em->type == 0x16) {
            return;
        }
        EmRoutineSet(em, 2, 5, 0, 0);
    } else if (w->flags & 0x10000) {
        if (em->hp > 0 && em->type == 0x16) {
            return;
        }
        EmRoutineSet(em, 2, 6, 0, 0);
    } else if (w->flags & 0x10000000) {
        if (em->hp > 0) {
            return;
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x100000) {
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x40000000) {
        if (pG->x4FB8 == 5) {
            EmRoutineSet(em, 2, 0x13, 0, 0);
        } else {
            EmRoutineSet(em, 2, 4, 0, 1);
        }
    } else if (em->hp <= 0) {
        em10LostHead(em, 0, 0);
        if ((w->flags & 0x40) && (part->partsNo == 0x13 || part->partsNo == 0x17 || part->partsNo == 0x14 || part->partsNo == 0x18)) {
            EmRoutineSet(em, 2, 3, 0, 0);
        } else {
            EmRoutineSet(em, 2, 4, 0, 1);
        }
    } else if (w->flags & 8) {
        w->x67A = 0;
    } else if (w->flags & 0x10) {
        w->x67A = 0;
        EmRoutineSet(em, 2, 0xA, 0, 0);
    } else if (em->dmWep == 0x24) {
        EmRoutineSet(em, 2, 0, 0, 0);
    } else if (pG->x4FB8 == 5) {
        EmRoutineSet(em, 2, 0x13, 0, 0);
    } else {
        EmRoutineSet(em, 2, 4, 0, 1);
    }
}

static void em10DmSetWep23(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int one = 1;

    em->dmType = one;
    if (em->type == 0xA || em->type == 0xD) {
        return;
    }
    if (em->x38D == 0x19) {
        return;
    }
    if (w->pGatling) {
        return;
    }
    if (w->pShield) {
        return;
    }
    if (w->pParasite || w->x58C) {
        em->hp = 0;
        EmSetDie(em);
        EmReserveDropItem(em);
        em10SetPoint(em);
        EstSetEm(em, -1, 0, 0, 0x10, 0x56, 0, 0, em, 0);
        em10CoreBreak(em, 0);
        if (EM10_WK(em)->flags & 0x10) {
            EmRoutineSet(em, 3, one, 0, 0);
            return;
        }
        if (em10RoofDmCk(em)) {
            return;
        }
        if ((s16) w->x682 != 0) {
            w->x68C = 120;
            EmRoutineSet(em, 2, 0xB, 0, 0);
        } else if (EM10_WK(em)->flags & 0x4000) {
            EmRoutineSet(em, 2, 0xC, 0, 0);
        } else if (EM10_WK(em)->flags & 0x20000) {
            EmRoutineSet(em, 2, 5, 0, 0);
        } else if (EM10_WK(em)->flags & 0x10000) {
            EmRoutineSet(em, 2, 6, 0, 0);
        } else if (EM10_WK(em)->flags & 0x10000000) {
            EmRoutineSet(em, 2, 4, 0, 0);
        } else if (EM10_WK(em)->flags & 0x100000) {
            EmRoutineSet(em, 2, 4, 0, 0);
        } else {
            EmRoutineSet(em, 3, 2, 0, 0);
        }
    } else {
        if ((s16) w->x682 != 0) {
            w->x68C = 120;
            EmRoutineSet(em, 2, 0xB, 0, 0);
        } else if (EM10_WK(em)->flags & 0x4000) {
            EmRoutineSet(em, 2, 0xC, 0, 0);
        } else if (EM10_WK(em)->flags & 0x20000) {
            EmRoutineSet(em, 2, 5, 0, 0);
        } else if (EM10_WK(em)->flags & 0x10000) {
            EmRoutineSet(em, 2, 6, 0, 0);
        } else if (EM10_WK(em)->flags & 0x10000000) {
            if (em->hp > 0) {
                return;
            }
            EmRoutineSet(em, 2, 4, 0, 0);
        } else if (EM10_WK(em)->flags & 0x100000) {
            EmRoutineSet(em, 2, 4, 0, 0);
        } else if (em->hp <= 0) {
            EmSetDie(em);
            EmReserveDropItem(em);
            em10SetPoint(em);
            if (EM10_WK(em)->flags & 0x10) {
                EmRoutineSet(em, 3, one, 0, 0);
            } else {
                if (em10RoofDmCk(em)) {
                    return;
                }
                EmRoutineSet(em, 3, 2, 0, 0);
            }
        } else if (EM10_WK(em)->flags & 8) {
            w->x67A = 0;
        } else if (w->flags & 0x10) {
            EmRoutineSet(em, 2, 0xA, 0, 0);
        } else {
            if (em10RoofDmCk(em)) {
                return;
            }
            EmRoutineSet(em, 2, 0xD, 0, 0);
        }
    }
}

// Chainsaw / parasite / partner Ganados take a few hits before reacting: count the guard down,
// then re-arm it with 2..4 hits.
#define EM10_GUARD_CK(w)                                                                            \
    if ((w)->x6BB) {                                                                                \
        (w)->x6BB--;                                                                                \
        return;                                                                                     \
    }                                                                                               \
    (w)->x6BB = Rnd() % 3 + 2

static void em10DmSetWep02(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EmHitInfo* part = em->dmPart;
    int react;
    int ret;
    int type;

    em->dmType = 1;
    if ((part->partsNo == 5 || part->partsNo == 0x25) && w->pParasite) {
        SndCall(8, 0x83, &em->pos, em->id, 0, em);
    } else if (em10ArmorCk(em, part->partsNo)) {
        SndCall(8, 0xD, &em->pos, em->id, 0, em);
    } else {
        SndCall(8, 0xC, &em->pos, em->id, 0, em);
    }
    react = em10ArmorCk(em, part->partsNo) == 0;
    if (em->type == 0xA || em->type == 0xD) {
        if (Rnd() & 3) {
            react = 0;
        }
    }
    if ((s16) w->x682 != 0) {
        w->x68C = 120;
        EmRoutineSet(em, 2, 0xB, 0, 0);
    } else if (w->flags & 0x4000) {
        if (em->hp <= 0 && part->partsNo == 5 && em->dmWep != 0x10) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
            if (w->wepType == 4 || w->pParasite || w->x58C) {
                EM10_GUARD_CK(w);
            }
        }
        if (react) {
            EmRoutineSet(em, 2, 0xC, 0, 0);
        }
    } else if (w->flags & 0x20000) {
        if (em->hp <= 0 && part->partsNo == 5 && em->dmWep != 0x10) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
            if (em->type == 0x16) {
                return;
            }
            if (w->wepType == 4 || w->pParasite || w->x58C) {
                EM10_GUARD_CK(w);
            }
        }
        if (react) {
            EmRoutineSet(em, 2, 5, 0, 0);
        }
    } else if (w->flags & 0x10000) {
        if (em->hp <= 0 && part->partsNo == 5 && em->dmWep != 0x10) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
            if (em->type == 0x16) {
                return;
            }
            if (w->wepType == 4 || w->pParasite || w->x58C) {
                EM10_GUARD_CK(w);
            }
        }
        if (react) {
            EmRoutineSet(em, 2, 6, 0, 0);
        }
    } else if (w->flags & 0x10000000) {
        em10BloodSet(em, 0);
        if (em->hp > 0) {
            return;
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x100000) {
        if (em->hp <= 0 && part->partsNo == 5 && em->dmWep != 0x10) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
        }
        if (react) {
            EmRoutineSet(em, 2, 4, 0, 0);
        }
    } else if (w->flags & 0x40000000) {
        if (em->hp <= 0 && part->partsNo == 5 && em->dmWep != 0x10) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
            if (em->type == 0x16) {
                return;
            }
            if (w->wepType == 4 || w->pParasite || w->x58C) {
                EM10_GUARD_CK(w);
            }
        }
        if (react) {
            EmRoutineSet(em, 2, 8, 0, 0);
        }
    } else {
        if (em10ChgParasiteCk(em)) {
            return;
        }
        if (em->hp <= 0) {
            EmSetDie(em);
            EmReserveDropItem(em);
            em10SetPoint(em);
            if (em->type == 0xA || em->type == 0xD || em->type == 2 || em->type == 0x16) {
                em10BloodSet(em, 0);
                EmRoutineSet(em, 3, 2, 0, 0);
                return;
            }
            if (w->flags & 0x10) {
                if (part->partsNo == 5 && em->dmWep != 0x10) {
                    em10LostHead(em, 1, 0);
                    GameAddPoint(9);
                } else {
                    em10BloodSet(em, 0);
                }
                EmRoutineSet(em, 3, 1, 0, 0);
                return;
            }
            if ((w->flags & 0x40) && (part->partsNo == 0x13 || part->partsNo == 0x17 || part->partsNo == 0x14 || part->partsNo == 0x18)) {
                EmRoutineSet(em, 2, 3, 0, 0);
                return;
            }
            if (em10RoofDmCk(em)) {
                em10BloodSet(em, 0);
                return;
            }
            if (part->partsNo == 5 && em->dmWep != 0x10) {
                u8 r;
                em10LostHead(em, 0, 0);
                GameAddPoint(9);
                r = Rnd() % 3;
                if (r == 0 || !em10LostHeadCk(em)) {
                    EmRoutineSet(em, 2, 4, 0, 0);
                    return;
                }
                if (em->xFC == 1 && (em->xFD == 0x10 || em->xFD == 0x39 || em->xFD == 0x33) && em10LostHeadCk(em) && w->wepType != 4) {
                    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000) {
                        em->xFC = 2;
                        em->xFD = 9;
                        em->xFF = em->xFE = 0;
                    }
                    if ((Rnd() & 0xF) == 5) {
                        w->x6C1 = 0x5A;
                        em->hp = 1;
                    }
                    return;
                }
                EmRoutineSet(em, 2, 9, 0, 0);
                return;
            }
            if (part->partsNo == 8 || part->partsNo == 0xE) {
                u8 r = Rnd() % 3;
                if (r != 0) {
                    EmRoutineSet(em, 2, 4, 0, 0);
                    BitOn(pG->flags_5010, 0x20000);
                    return;
                }
            }
            em->xFC = 3;
            em->xFD = 2;
            em->xFF = em->xFE = 0;
            em10BloodSet(em, 0);
            return;
        }
        if (w->flags & 8) {
            w->x67A = 0;
            em10BloodSet(em, 0);
            return;
        }
        if (w->flags & 0x10) {
            w->x67A = 0;
            em10BloodSet(em, 0);
            if (react) {
                EmRoutineSet(em, 2, 0xA, 0, 0);
            }
            return;
        }
        em10BloodSet(em, 0);
        ret = em10RoofDmCk(em);
        if (ret) {
            return;
        }
        type = em->type;
        if (type == 6) {
            em->xFC = 2;
            em->xFD = 0;
            em->xFE = 0;
            em->xFF = 0;
            return;
        }
        if (type == 2) {
            switch (em->dmWep) {
            default: {
                u8 r;
                if (part->partsNo != 5) {
                    return;
                }
                r = Rnd() % 10;
                if (r != 0) {
                    return;
                }
                em->xFC = 2;
                em->xFD = 1;
                em->xFE = 0;
                em->xFF = 0;
                return;
            }
            case 9:
            case 10:
                if (part->partsNo == 5) {
                    em->xFC = 2;
                    em->xFD = 1;
                    em->xFE = 0;
                    em->xFF = 0;
                    return;
                } else {
                    u8 r = Rnd() % 10;
                    if (r < 5) {
                        return;
                    }
                    em->xFC = 2;
                    em->xFD = 0x11;
                    em->xFE = 0;
                    em->xFF = 0;
                    return;
                }
            }
        }
        if (part->partsNo == 5 && w->pShield == 0) {
            if (w->wepType == 4) {
                EM10_GUARD_CK(w);
            }
            if (!react) {
                return;
            }
            if (w->pParasite && w->x6C5 == 1) {
                if (Rnd() & 3) {
                    EmRoutineSet(em, 2, 0, 0, 0);
                }
                return;
            }
            EmRoutineSet(em, 2, 1, 0, 0);
            return;
        }
        if (part->partsNo == 0x25) {
            EmRoutineSet(em, 2, 0xF, 0, 0);
            return;
        }
        if (w->flags & 0x40) {
            if (w->wepType == 4 || w->pParasite || w->x58C) {
                EM10_GUARD_CK(w);
            }
            if (!react) {
                return;
            }
            if (part->partsNo == 0x13 || part->partsNo == 0x17 || part->partsNo == 0x14 || part->partsNo == 0x18) {
                EmRoutineSet(em, 2, 3, 0, 0);
            } else {
                EmRoutineSet(em, 2, 2, 0, 0);
            }
            return;
        }
        if (em->dmWep == 0x10 && pG->x4FB8 != 4) {
            u8 r = Rnd() % 3;
            if (r == 0) {
                return;
            }
            if (!react) {
                return;
            }
        }
        if (em->type == 0xA || em->type == 0xD) {
            return;
        }
        if (w->wepType == 4 || w->pParasite || w->x58C) {
            EM10_GUARD_CK(w);
        }
        if (em->type == 0x16) {
            return;
        }
        if (react) {
            EmRoutineSet(em, 2, 0, 0, 0);
        }
    }
}

static void em10DmSetWep03(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EmHitInfo* part = em->dmPart;
    int near = 0;

    em->dmType = 1;
    if (part->rad < 36000000.0f) {
        near = 1;
    }
    em10ArmorCk(em, part->partsNo);
    if (em->type == 0xA || em->type == 0xD || em->type == 2 || em->type == 0x16) {
        Rnd();
    }
    if ((part->partsNo == 5 || part->partsNo == 0x25) && w->pParasite) {
        SndCall(8, 0x83, &em->pos, em->id, 0, em);
    } else if (em10ArmorCk(em, part->partsNo)) {
        SndCall(8, 0xD, &em->pos, em->id, 0, em);
    } else if (near) {
        SndCall(8, 0x7A, &em->pos, em->id, 0, em);
    } else {
        SndCall(8, 0xC, &em->pos, em->id, 0, em);
    }
    if ((s16) w->x682 != 0) {
        w->x68C = 120;
        EmRoutineSet(em, 2, 0xB, 0, 0);
    } else if (w->flags & 0x4000) {
        EmRoutineSet(em, 2, 0xC, 0, 0);
    } else if (w->flags & 0x20000) {
        if (em->hp <= 0 && part->partsNo == 5) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, near);
            if (em->type == 0x16) {
                return;
            }
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x10000) {
        if (em->hp <= 0 && part->partsNo == 5) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, near);
            if (em->type == 0x16) {
                return;
            }
        }
        EmRoutineSet(em, 2, 6, 0, 0);
    } else if (w->flags & 0x10000000) {
        em10BloodSet(em, 0);
        if (em->hp > 0) {
            return;
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x100000) {
        if (em->hp <= 0 && part->partsNo == 5) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, near);
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x40000000) {
        if (em->hp <= 0 && part->partsNo == 5) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, near);
            if (em->type == 0x16) {
                return;
            }
        }
        EmRoutineSet(em, 2, 8, 0, 0);
    } else {
        if (em10ChgParasiteCk(em)) {
            return;
        }
        if (em->hp <= 0) {
            em10BloodSet(em, near);
            if (em->type == 0xA || em->type == 0xD || em->type == 2 || em->type == 0x16) {
                EmRoutineSet(em, 3, 2, 0, 0);
                return;
            }
            if (w->flags & 0x10) {
                if (part->partsNo == 5 && near) {
                    em10LostHead(em, 1, 0);
                    GameAddPoint(9);
                } else {
                    em10BloodSet(em, near);
                }
                EmRoutineSet(em, 3, 1, 0, 0);
                return;
            }
            if (part->partsNo == 5 && near) {
                em10LostHead(em, 0, 0);
                GameAddPoint(9);
                if (em->xFC == 1 && (em->xFD == 0x10 || em->xFD == 0x39 || em->xFD == 0x33) && em10LostHeadCk(em) && w->wepType != 4) {
                    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000) {
                        em->xFC = 2;
                        em->xFD = 4;
                        em->xFF = em->xFE = 0;
                    }
                    if ((Rnd() & 0xF) == 5) {
                        w->x6C1 = 0x5A;
                        em->hp = 1;
                    }
                    return;
                }
                EmRoutineSet(em, 2, 4, 0, 0);
                return;
            }
            em10BloodSet(em, near);
            {
                int ret = em10RoofDmCk(em);
                if (ret) {
                    return;
                }
                em->xFC = 2;
                em->xFD = 4;
                em->xFE = 0;
                em->xFF = 0;
                return;
            }
            em->xFC = 2;
            em->xFD = 4;
            em->xFE = 0;
            em->xFF = 0;
            return;
        }
        if (w->flags & 8) {
            w->x67A = 0;
            em10BloodSet(em, near);
            return;
        }
        if (w->flags & 0x10) {
            w->x67A = 0;
            em10BloodSet(em, near);
            EmRoutineSet(em, 2, 0xA, 0, 0);
            return;
        }
        if (w->pShield) {
            em10BloodSet(em, near);
            EmRoutineSet(em, 2, 0, 0, 0);
            return;
        }
        em10BloodSet(em, near);
        {
        int ret = em10RoofDmCk(em);
        if (ret) {
            return;
        }
        }
        if (part->partsNo == 0x25) {
            em->xFC = 2;
            em->xFD = 0xF;
            em->xFE = 0;
            em->xFF = 0;
            return;
        }
        if (near && em->type == 2) {
            int type = em->type;
            if (part->partsNo == 5) {
                em->xFC = type;
                em->xFD = 1;
                em->xFE = 0;
                em->xFF = 0;
                return;
            } else {
                u8 r = Rnd() % 10;
                if (r > 4) {
                    em->xFC = type;
                    em->xFD = 0x11;
                    em->xFE = 0;
                    em->xFF = 0;
                    return;
                }
            }
        }
        if (em->type == 0xA || em->type == 0xD || em->type == 2) {
            return;
        }
        if (near) {
            if (em->type == 0x16) {
                u8 r = Rnd() % 10;
                if (r <= 4 && part->partsNo != 5) {
                    return;
                }
                EmRoutineSet(em, 2, 0, 0, 0);
                return;
            }
            EmRoutineSet(em, 2, 4, 0, 0);
            return;
        }
        if (em->type == 0x16) {
            return;
        }
        if (w->flags & 0x40) {
            if (part->partsNo == 0x13 || part->partsNo == 0x17 || part->partsNo == 0x14 || part->partsNo == 0x18) {
                EmRoutineSet(em, 2, 3, 0, 0);
            } else {
                EmRoutineSet(em, 2, 2, 0, 0);
            }
        } else {
            EmRoutineSet(em, 2, 0, 0, 0);
        }
    }
}

static void em10DmSetWep09(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EmHitInfo* part = em->dmPart;
    cModel* parts;
    int mag;
    int type;

    switch (em->dmWep) {
    default:
        mag = 0;
        em->dmg.set(0, 5);
        break;
    case 0xD:
    case 0x12:
    case 0x13:
    case 0x29:
    case 0x2D:
        mag = 1;
        em->dmg.set(0, 0x1E);
        break;
    }
    if (em->dmWep == 0x2D) {
        em->hp = 0;
        EmRoutineSet(em, 3, 5, 0, 2);
        return;
    }
    parts = em->getPartsPtr(part->partsNo - 1);
    if ((part->partsNo == 5 || part->partsNo == 0x25) && w->pParasite) {
        SndCall(8, 0x83, &parts->worldPos, em->id, 0, em);
    } else if (em10ArmorCk(em, part->partsNo)) {
        SndCall(8, 0xD, &parts->worldPos, em->id, 0, em);
    } else if (em->dmWep == 0x1C) {
        SndCall(8, 0x81, &parts->worldPos, em->id, 0, em);
    } else {
        SndCall(8, 0xC, &parts->worldPos, em->id, 0, em);
    }
    if (w->wepType == 9) {
        switch (em->dmWep) {
        case 0xD:
        case 0x12:
        case 0x13:
        case 0x29:
            em->hp = 0;
            EmRoutineSet(em, 3, 5, 0, 0);
            return;
        }
    }
    if ((s16) w->x682 != 0) {
        w->x68C = 120;
        EmRoutineSet(em, 2, 0xB, 0, 0);
    } else if (w->flags & 0x4000) {
        EmRoutineSet(em, 2, 0xC, 0, 0);
    } else if (w->flags & 0x20000) {
        if (em->hp <= 0 && part->partsNo == 5 && mag == 0) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
            if (em->type == 0x16) {
                return;
            }
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x10000) {
        if (em->hp <= 0 && part->partsNo == 5 && mag == 0) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
            if (em->type == 0x16) {
                return;
            }
        }
        EmRoutineSet(em, 2, 6, 0, 0);
    } else if (w->flags & 0x10000000) {
        em10BloodSet(em, 0);
        if (em->hp > 0) {
            return;
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x100000) {
        if (em->hp <= 0 && part->partsNo == 5 && mag == 0) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
        }
        EmRoutineSet(em, 2, 4, 0, 0);
    } else if (w->flags & 0x40000000) {
        if (em->hp <= 0 && part->partsNo == 5 && mag == 0) {
            em10LostHead(em, 1, 0);
            GameAddPoint(9);
        } else {
            em10BloodSet(em, 0);
            if (em->type == 0x16) {
                return;
            }
        }
        EmRoutineSet(em, 2, 8, 0, 0);
    } else {
        if (mag == 0 && em10ChgParasiteCk(em)) {
            return;
        }
        if (em->hp <= 0) {
            if (em->type == 0xA || em->type == 0xD || em->type == 2) {
                em10BloodSet(em, 0);
                EmRoutineSet(em, 3, 2, 0, 0);
                return;
            }
            if (part->rad < 1000000.0f && w->wepType != 4 && em->type != 0xA && em->type != 0xD && G_ROOM_ID != 0x100 &&
                em->x38D != 0x39 && em->type != 2) {
                switch (em->dmWep) {
                case 0xD:
                case 0x13:
                case 0x29:
                    EmRoutineSet(em, 3, 5, 0, 1);
                    return;
                }
            }
            if (em->hp <= 0 && part->partsNo == 5 && mag == 0) {
                em10LostHead(em, 1, 0);
                GameAddPoint(9);
            } else {
                em10BloodSet(em, 0);
            }
            EmRoutineSet(em, 2, 4, 0, 0);
            return;
        }
        if (w->flags & 8) {
            w->x67A = 0;
            em10BloodSet(em, 0);
            return;
        }
        if (w->flags & 0x10) {
            w->x67A = 0;
            em10BloodSet(em, 0);
            EmRoutineSet(em, 2, 0xA, 0, 0);
            return;
        }
        if (part->partsNo == 0x25) {
            EmRoutineSet(em, 2, 0xF, 0, 0);
            return;
        }
        type = em->type;
        if (type == 0xA || type == 0xD) {
            EmRoutineSet(em, 2, 0xE, 0, 0);
            return;
        }
        if (type == 2) {
            int no;
            em10BloodSet(em, 0);
            no = 0x11;
            if (part->partsNo == 5) {
                no = 1;
            }
            em->xFC = type;
            em->xFD = no;
            em->xFE = 0;
            em->xFF = 0;
            return;
        }
        if (type == 0x16) {
            u8 r = Rnd() % 10;
            if (r <= 4 && part->partsNo != 5) {
                return;
            }
            EmRoutineSet(em, 2, 0, 0, 0);
            return;
        }
        if (em->dmWep == 0x1C) {
            if (part->partsNo == 5 && w->pShield == 0) {
                if (w->pParasite && w->x6C5 == 1) {
                    if (!(Rnd() & 3)) {
                        return;
                    }
                    em10BloodSet(em, 0);
                    EmRoutineSet(em, 2, 4, 0, 0);
                    return;
                }
                EmRoutineSet(em, 2, 1, 0, 0);
                return;
            }
            if (part->partsNo == 0x13 || part->partsNo == 0x17 || part->partsNo == 0x14 || part->partsNo == 0x18) {
                EmRoutineSet(em, 2, 0, 0, 0);
                return;
            }
        }
        em10BloodSet(em, 0);
        EmRoutineSet(em, 2, 4, 0, 0);
    }
}

void em10KickHitMark(cEm10* em)
{
    cModel* parts;
    Vec rot;

    parts = em->getPartsPtr(4);
    rot.x = 0.0f;
    rot.y = GetXZAngle(&em->x328, &em->pos);
    rot.z = 0.0f;
    if (ChkWaterEffectEnable(&em->pos)) {
        EstSet(0, -1, &parts->worldPos, &rot, 0x10, 0x38, 0, 0, 0, 0);
    } else {
        EstSet(0, -1, &parts->worldPos, &rot, 0x10, 0x25, 0, 0, 0, 0);
    }
}

// Damage blood / hit effect by weapon (near: the shot came from close range).
void em10BloodSet(cEm10* em, int near)
{
    Em10Work* w = EM10_WK(em);
    Camera* cam = &pG->Cam;
    EmHitInfo* part;
    cModel* parts;
    f32 dist;
    Vec pos;
    Vec dir;
    Vec dir2;
    Mtx m;
    cEsp* esp;
    u32 i;

    if (em->type == 0xA || em->type == 0xD) {
        em1cBloodSet(em, near);
        return;
    }
    parts = em->getPartsPtr(0);
    dist = (cam->param.pos.x - parts->worldPos.x) * (cam->param.pos.x - parts->worldPos.x) +
           (cam->param.pos.y - parts->worldPos.y) * (cam->param.pos.y - parts->worldPos.y) +
           (cam->param.pos.z - parts->worldPos.z) * (cam->param.pos.z - parts->worldPos.z);
    part = em->dmPart;
    if (part == 0) {
        return;
    }
    if (em10ArmorCk(em, part->partsNo)) {
        switch (em->dmWep) {
        case 0:
        case 0x14:
        case 0x17:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x2A:
            return;
        case 1:
        case 2:
        case 3:
        case 4:
        case 0x11:
        case 0x19:
        case 0x1C:
        case 0x1F:
        case 0x20:
        case 0x26:
        case 0x2B:
            EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            return;
        case 0x10:
        case 0x1A:
            EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            return;
        case 0xB:
        case 0xC:
        case 0x1B:
        case 0x1D:
        case 0x27:
            EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            return;
        case 7:
        case 8:
        case 0x21:
            if (near) {
                EmDmBloodSet2(em, 0x10, 0x65, 0, 0, 0);
            } else {
                EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            }
            return;
        case 5:
        case 6:
        case 9:
        case 0xA:
        case 0xD:
        case 0xF:
        case 0x12:
        case 0x13:
        case 0x15:
        case 0x28:
        case 0x29:
        case 0x2C:
        case 0x2D:
            break;
        }
        EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
    } else {
        if (part->partsNo == 5 && (w->pParasite || w->x58C)) {
            EmDmBloodSet2(em, 0x10, 0x26, 0, 0, 0);
            return;
        }
        switch (em->dmWep) {
        case 0:
        case 0x14:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x2A:
            return;
        case 1:
        case 2:
        case 3:
        case 4:
        case 0x11:
        case 0x19:
        case 0x1C:
        case 0x1F:
        case 0x20:
        case 0x26:
        case 0x2B:
            if (ChkWaterEffectEnable(&em->pos)) {
                EmDmBloodSet2(em, 0x10, 0x3B, 0, 0, 0);
            } else {
                EmDmBloodSet2(em, 0x10, 1, 0, 0, 0);
            }
            return;
        case 0x10:
        case 0x1A:
            EmDmBloodSet2(em, 0x10, 0x29, 0, 0, 0);
            return;
        case 0xB:
        case 0xC:
        case 0x1B:
        case 0x1D:
        case 0x27:
            if (ChkWaterEffectEnable(&em->pos)) {
                EmDmBloodSet2(em, 0x10, 0x3D, 0, 0, 0);
            } else {
                EmDmBloodSet2(em, 0x10, 0xC, 0, 0, 0);
            }
            if ((Rnd() & 3) == 0) {
                if (EmGetDmPos(em, &pos, &dir)) {
                    EstSet(0, -1, &pos, 0, 0x10, 0xD, 0, 0, 0, 0);
                }
            }
            return;
        case 7:
        case 8:
        case 0x21:
            if (near) {
                if (Ctrl12Ck(w->pCtrl12, 0xB)) {
                    if (dist < 16000000.0f) {
                        if (ChkWaterEffectEnable(&em->pos)) {
                            EmDmBloodSet2(em, 0x10, 0x40, 0, 0, 0);
                        } else {
                            EmDmBloodSet2(em, 0x10, 0x37, 0, 0, 0);
                        }
                    } else {
                        if (ChkWaterEffectEnable(&em->pos)) {
                            EmDmBloodSet2(em, 0x10, 0x3E, 0, 0, 0);
                        } else {
                            EmDmBloodSet2(em, 0x10, 0x35, 0, 0, 0);
                        }
                    }
                } else {
                    if (dist < 16000000.0f) {
                        if (ChkWaterEffectEnable(&em->pos)) {
                            EmDmBloodSet2(em, 0x10, 0x3F, 0, 0, 0);
                        } else {
                            EmDmBloodSet2(em, 0x10, 0x36, 0, 0, 0);
                        }
                    } else {
                        if (ChkWaterEffectEnable(&em->pos)) {
                            EmDmBloodSet2(em, 0x10, 0x3C, 0, 0, 0);
                        } else {
                            EmDmBloodSet2(em, 0x10, 2, 0, 0, 0);
                        }
                    }
                    Ctrl12Set(w->pCtrl12, 0xB, 0x2D);
                }
            } else {
                if (ChkWaterEffectEnable(&em->pos)) {
                    EmDmBloodSet2(em, 0x10, 0x3B, 0, 0, 0);
                } else {
                    EmDmBloodSet2(em, 0x10, 1, 0, 0, 0);
                }
            }
            return;
        case 9:
        case 0xA:
        case 0x28:
            EmDmBloodSet2(em, 0x10, 0x87, 0, 0, 0);
            if (EmGetDmPos(em, &pos, &dir2)) {
                EspSeqData* est = EspGetEstAddr(0x10, 0x88, 1);
                if (est) {
                    for (i = 0; i < est->num; i++) {
                        if (EspEstSetSelect(0x10, 0x88, i, &esp, 0)) {
                            esp->pos = pos;
                            esp->parent = em->getPartsPtr(part->partsNo - 1);
                            PSMTXInverse(((cModel*) esp->parent)->mat, m);
                            PSMTXMultVec(m, &esp->pos, &esp->pos);
                            esp->partsNo = ((u8*) &part->partsNo)[1] - 1;
                        }
                    }
                }
            }
            return;
        case 5:
        case 6:
        case 0xD:
        case 0xF:
        case 0x12:
        case 0x13:
        case 0x15:
        case 0x29:
        case 0x2C:
        case 0x2D:
            break;
        }
        {
            if (Ctrl12Ck(w->pCtrl12, 0xB)) {
                if (dist < 16000000.0f) {
                    if (ChkWaterEffectEnable(&em->pos)) {
                        EmDmBloodSet2(em, 0x10, 0x40, 0, 0, 0);
                    } else {
                        EmDmBloodSet2(em, 0x10, 0x37, 0, 0, 0);
                    }
                } else {
                    if (ChkWaterEffectEnable(&em->pos)) {
                        EmDmBloodSet2(em, 0x10, 0x3E, 0, 0, 0);
                    } else {
                        EmDmBloodSet2(em, 0x10, 0x35, 0, 0, 0);
                    }
                }
            } else {
                if (dist < 16000000.0f) {
                    if (ChkWaterEffectEnable(&em->pos)) {
                        EmDmBloodSet2(em, 0x10, 0x3F, 0, 0, 0);
                    } else {
                        EmDmBloodSet2(em, 0x10, 0x36, 0, 0, 0);
                    }
                } else {
                    if (ChkWaterEffectEnable(&em->pos)) {
                        EmDmBloodSet2(em, 0x10, 0x3C, 0, 0, 0);
                    } else {
                        EmDmBloodSet2(em, 0x10, 2, 0, 0, 0);
                    }
                }
                Ctrl12Set(w->pCtrl12, 0xB, 0x2D);
            }
        }
    }
}

void cEm10::move()
{
    Em10Work* w = EM10_WK(this);
    f32 dist;
    Mtx m;
    Vec v;

    if (xFC) {
        em10DmCk(this);
    }
    w->flags &= 0xACC081A7;
    clearStatus(3);
    hitInfo.flags |= 1;
    if ((w->flags & 0x80) && w->pParasite == 0 && w->x58C == 0) {
        hitInfo.flags &= ~1;
    }
    em10RouteCk(this);
    {
        u32 lost = w->flags & 0x800000;
        w->flags &= ~0x04000004;
        if (lost) {
            w->x656++;
        } else {
            w->x656 = 0;
        }
    }
    if (w->x67C) {
        w->x67C--;
    }
    if (pPL->xFC == 1 || (pG->flags_5010 & 0x8000)) {
        if ((s16) w->x67C <= 4) {
            w->x67C = 5;
        }
    }
    if (pG->flags_5010 & 0x2000) {
        w->x67C = 0;
    }
    if (w->x674) {
        w->x674--;
    }
    if (pG->x4F88 > 6) {
        if (w->x674) {
            w->x674--;
        }
    }
    if (pG->x4F88 > 9) {
        if (w->x674) {
            w->x674--;
        }
    }
    if (w->x660) {
        w->x660--;
    }
    if (w->x68C) {
        w->x68C--;
    }
    if (w->flags & 1) {
        w->x638 = 0;
    } else {
        w->x638++;
    }
    if (w->x664) {
        w->x664--;
    }
    if (w->x670) {
        w->x670--;
    }
    if (w->x696) {
        w->x696--;
    }
    if (w->x682) {
        w->x682--;
    }
    if (w->x6A3) {
        w->x6A3--;
    }
    if (w->x644) {
        w->x644--;
    }
    if (w->x65E) {
        w->x65E--;
    }
    if (w->x6C3) {
        w->x6C3--;
    }
    if (w->x6BC) {
        w->x6BC--;
        if (w->x6BC == 0) {
            atari.flags &= ~8;
        }
    }
    if (w->x6C1) {
        w->x6C1--;
    }
    if ((w->flags & 1) && w->x646) {
        w->x646--;
    }
    if (w->x648) {
        w->x648--;
    }
    w->x6C2 = 0;
    Em10_R0_move_tbl[xFC](this);
    if (xFC == 0xFF) {
        EmMgr.destroy(this);
        return;
    }
    if (seFlags28B & 0x80) {
        w->flags |= 0x01000010;
        setStatus(3);
    }
    if (w->flags & 0x800) {
        scale.x = 1.0f;
        scale.y = 1.0f;
        scale.z = 1.0f;
    } else {
        scale.x = scale.x * 0.9f + w->scaleBase.x * 0.1f;
        scale.y = scale.y * 0.9f + w->scaleBase.y * 0.1f;
        scale.z = scale.z * 0.9f + w->scaleBase.z * 0.1f;
    }
    em10ClawMove(this);
    if (!(w->flags & 0x400000)) {
        em10NeckMove(this);
        em10WaistMove(this);
        em10SlopeMove(this);
        partsWorldCalc();
        em10ScaleCompress(this);
        em10BombNeckMove(this);
        partsFixAdjust();
        PartsWorldPosCalc(this);
        if (!(w->flags & 0x400000)) {
            u16 atFlags;
            f32 moved;
            dist = SQRTF((oldPos.x - pos.x) * (oldPos.x - pos.x) + (oldPos.z - pos.z) * (oldPos.z - pos.z));
            if ((seFlags28B & 0x40) || (w->flags & 0x10091000)) {
                atari.flags |= 0x10;
            } else {
                atari.flags &= ~0x10;
            }
            atFlags = atari.flags;
            if ((seFlags28B & 0x40) || (w->flags & 0x11000)) {
                atari.flags &= ~0x100;
            }
            EmAtCheck(this);
            atari.move();
            if (w->flags & 0x80000) {
                SatMgr.checkAir(this, 0x1C2810);
            } else {
                SatMgr.check(this, 0);
            }
            atari.flags = atFlags;
            moved = SQRTF((pos.x - oldPos.x) * (pos.x - oldPos.x) + (pos.z - oldPos.z) * (pos.z - oldPos.z));
            if (moved < dist * 0.5f) {
                w->x634++;
            } else {
                if (w->x634 > 60) {
                    w->x634 = 60;
                }
                if (w->x634) {
                    w->x634--;
                }
            }
        }
    }
    {
        Camera* cam = &pG->Cam;
        int hide = 0;
        Vec* nrm = pFloorNrm;
        if (nrm == 0 || nrm->y < 0.8f) {
            hide = 1;
        }
        if (cam->param.pos.y < pos.y) {
            hide = 1;
        }
        if ((w->flags & 0x101B0000) || hide) {
            if (shdCol <= 0xF6) {
                shdCol += 8;
            } else {
                shdCol = 0xFF;
            }
        } else {
            if (shdCol > 8) {
                shdCol -= 8;
            } else {
                shdCol = hide;
            }
        }
    }
    switch (x38D) {
    case 7:
    case 8:
    case 9:
        if (!EM_RTN(this, 1, 8)) {
            EffectEspDelete(0, w->x69E, (u32) this, 0);
            EffectEspgenDelete(0, w->x69E, (int) this);
            EffectEfmDelete(0, w->x69E, (int) this);
        }
        break;
    case 5:
    case 6:
    case 0x1A:
        if (!EM_RTN(this, 1, 7)) {
            EffectEspDelete(0, w->x69E, (u32) this, 0);
            EffectEspgenDelete(0, w->x69E, (int) this);
            EffectEfmDelete(0, w->x69E, (int) this);
        }
        break;
    case 0xA:
    case 0xB:
        break;
    }
    em10ChainSawMove(this);
    if (type == 6) {
        Em18ClothMove(this, (PlCloth*) &w->cloth);
    }
    if (type == 0x16) {
        Em1fClothMove(this, (PlCloth*) &w->cloth);
    }
    if (w->pWep && w->wepType == 4 && (w->flags & 0x80000000) && hp > 0 && (s16) pG->pl_life > 0) {
        if (w->x684) {
            w->x684--;
            if ((s16) w->x684 == 0) {
                w->x684 = 60;
                w->sndId = SndCall(6, 0x4D, &pos, 0, 0, this);
            }
        }
    } else if (w->sndId) {
        SndStop(w->sndId, 0);
        w->sndId = 0;
    }
    if (w->pHead && !EM_RTN(this, 1, 9)) {
        if (w->mot[51]) {
            MotionSetCore(w->pHead, MOTION(w->pHead), w->mot[51], 0, 0, 0, 0);
        }
        atariInitF(&w->pHead->atari, 0.0f, 150.0f, -300.0f, 300.0f, 300.0f, 300.0f, 150.0f, 1, 0x2000, 10);
        w->pHead->atari.flags &= ~0x100;
        w->pHead->atari.flags |= 0x200;
        w->pHead->atari.flags |= 0x10;
        EstSetEm(w->pHead, -1, 0, 0, 0x10, 0x16, 0, 0, w->pHead, 0);
        EstSetEm(w->pHead, -1, 0, 0, 0x10, 0x16, 0, 0, w->pHead, 0);
        EstSetEm(w->pHead, -1, 0, 0, 0x10, 0x16, 0, 0, w->pHead, 0);
        w->pHead = 0;
    }
    em10BowgunMove(this);
    if (w->x658) {
        w->x658--;
        if (w->x658 == 0) {
            em10SetParasite(this);
        }
    }
    em10SetWaterEff(this);
    if (w->x58C && hp > 0) {
        if (w->x694) {
            w->x694--;
        } else {
            w->x694 = 0x1D;
            EstSetEm(this, -1, 0, 0, 0x10, 0x33, 0, 0, this, 0);
        }
    }
    em10FootSe(this);
    if (w->wepType == 9 && w->x640 && w->pWep && hp > 0) {
        if (w->x640 <= 999) {
            w->x640--;
        }
        w->x6A4++;
        if (w->x6A4 % 6 == 0) {
            SndCall(8, 0x95, &pos, id, 0, this);
        }
        if (w->x640 == 0) {
            cModel* parts = w->pWep->getPartsPtr(0);
            w->pWep->setLost();
            w->pWep = 0;
            w->wepType = 0;
            if (hp > 0) {
                hp = 0;
                EmRoutineSet(this, 3, 5, 0, 0);
            } else {
                if (w->flags & 0x01400000) {
                    EstSet(0, -1, &parts->worldPos, 0, 0x10, 0x2A, 0, 0, 0, 0);
                } else {
                    EstSetEm(this, -1, 0, 0, 0x10, 0x30, 0, 0, this, 0);
                }
                SndCall(8, 0x96, &pos, id, 0, this);
                SndCall(8, 8, &pos, id, 0, this);
                be_flag &= ~2;
            }
            dmType = 0;
            PlWepHitCheck2(0, &pos, &pos, 0x13, 2, 6000.0f);
        }
    }
    if (type == 0xA || type == 0xD) {
        w->hit[9].flags |= 1;
    } else {
        w->hit[9].flags &= ~1;
        if (w->pParasite && w->pParasite->pParts && w->pParasite->isAlive()) {
            cModel* parts;
            PSMTXInverse(getPartsPtr(4)->mat, m);
            if (w->x6C5 == 1) {
                parts = w->pParasite->getPartsPtr(9);
            } else {
                parts = w->pParasite->getPartsPtr(0x15);
            }
            PSMTXMultVec(m, &parts->worldPos, &v);
            w->hit[9].ofs = v;
            w->hit[9].flags |= 1;
        }
        if (w->x58C && w->x58C->pParts && w->x58C->isAlive()) {
            cModel* parts;
            PSMTXInverse(getPartsPtr(4)->mat, m);
            parts = w->x58C->getPartsPtr(2);
            PSMTXMultVec(m, &parts->worldPos, &v);
            w->hit[9].ofs = v;
            w->hit[9].flags |= 1;
        }
    }
    if (w->pShield) {
        if (flags_3C8 & 0x1000000) {
            w->hit[3].flags &= ~1;
            w->hit[7].flags &= ~1;
        } else {
            w->hit[4].flags &= ~1;
            w->hit[8].flags &= ~1;
        }
    } else {
        w->hit[3].flags |= 1;
        w->hit[7].flags |= 1;
        w->hit[4].flags |= 1;
        w->hit[8].flags |= 1;
    }
    if (w->pShield && w->pShield->hp <= 0) {
        w->pShield = 0;
    }
    em10GatlingRollMove(this);
    if ((flags_3C8 & 0x40) && !(w->flags & 0x100) && G_ROOM_ID == 0x206) {
        setFindPL();
    }
}

// Motion parts flip table (MotionWork::flip): left / right parts swapped.
static u16 em10_xflip_tbl[80] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x11, 0x16, 0x17, 0x18, 0x19, 0x12, 0x13, 0x14, 0x15, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
};

static void em10_R0_Init(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    f32 sc;

    if (Em10SetFunc == 0) {
        em->xFC = 0xFF;
        return;
    }
    Em10SetFunc(em);
    EspDataLoad((u32) PL_ARC_PTR(em->subArc, 4), 0x10, 0);
    if (em10ModelInit(em) == 0) {
        em->xFC = 0xFF;
        return;
    }
    switch (em->emsetNo & 0xF) {
    default:
        sc = fRand1_1() * 0.01f + 1.03f;
        break;
    case 7:
        sc = 1.1f;
        break;
    case 0:
    case 4:
    case 9:
    case 0xB:
        sc = fRand1_1() * 0.01f + 1.01f;
        break;
    }
    if (em->type == 6) {
        sc = 1.05f;
    }
    if (em->type == 0xA || em->type == 0xD) {
        sc = 1.25f;
    }
    if (em->type == 2) {
        sc = 1.25f;
    }
    if (em->type == 0x16) {
        sc = 1.3f;
    }
    if (em->type == 0x18) {
        sc = 1.2f;
    }
    switch (em->type) {
    case 0xB:
    case 0xC:
        sc = 1.0f;
        break;
    }
    em->scale.z = sc;
    em->scale.y = sc;
    em->scale.x = sc;
    w->scaleBase = em->scale;
    em->motFlip = em10_xflip_tbl;
    switch (em->type) {
    case 6:
        Em18ClothSet(em, (PlCloth*) &w->cloth, 0);
        break;
    case 0x16:
        Em1fClothSet(em, (PlCloth*) &w->cloth);
        break;
    case 3:
    case 5:
    case 7:
    case 8:
    case 9:
    case 0xA:
    case 0x17:
        break;
    }
    w->pCtrl11 = GetCtrlCtrl11();
    w->pCtrl12 = GetCtrlCtrl12();
    w->startPos = em->pos;
    w->startRotY = em->rot.y;
    w->x4D8 = w->startPos;
    w->x4C4 = em->x38D;
    {
        static const Vec ofs = { 0.0f, 0.0f, 0.0f };
        static const Vec size = { 1000.0f, 1000.0f, 0.0f };
        em->lightInfo.init2(0, 1, &ofs, &size, 2);
    }
    em->lockParts = 2;
    em->lockOfs.x = 0.0f;
    em->lockOfs.y = 0.0f;
    em->lockOfs.z = 0.0f;
    atariInitF(&em->atari, 0.0f, 0.0f, 0.0f, 400.0f, 250.0f, 250.0f, 800.0f, 1, 0x2000, 10);
    if (em->type == 6) {
        em->atari.flags |= 8;
        em->setStatus(0xB);
    }
    em->litArea.on(1);
    if (em->type == 6) {
        em->setStatus(1);
        if (em->type == 6) {
            em->hp = 1;
        }
    }
    if (em->type == 0xA || em->type == 0xD) {
        YarareInit(em, 0.0f, 300.0f, 50.0f, 150.0f, 110.0f, 3, 5);
    } else {
        YarareInit(em, 0.0f, 0.0f, 0.0f, 160.0f, 110.0f, 5, 1);
    }
    YarareAdd(em, &w->hit[0], 0.0f, -30.0f, 0.0f, 210.0f, 290.0f, 2, 1);
    switch (em->type) {
    case 7:
    case 8:
    case 9:
    case 0xB:
    case 0xC:
        YarareAdd(em, &w->hit[1], -20.0f, -400.0f, 0.0f, 180.0f, 400.0f, 0x14, 1);
        YarareAdd(em, &w->hit[2], 20.0f, -400.0f, 0.0f, 180.0f, 400.0f, 0x18, 1);
        break;
    default:
        YarareAdd(em, &w->hit[1], -20.0f, -400.0f, 0.0f, 150.0f, 400.0f, 0x14, 1);
        YarareAdd(em, &w->hit[2], 20.0f, -400.0f, 0.0f, 150.0f, 400.0f, 0x18, 1);
        break;
    }
    if (em->type == 0xA || em->type == 0xD) {
        YarareAdd(em, &w->hit[3], -300.0f, 0.0f, 0.0f, 100.0f, 250.0f, 9, 3);
        YarareAdd(em, &w->hit[4], 50.0f, 0.0f, 0.0f, 100.0f, 250.0f, 0xF, 3);
    } else {
        YarareAdd(em, &w->hit[3], -350.0f, 0.0f, 0.0f, 100.0f, 350.0f, 9, 3);
        YarareAdd(em, &w->hit[4], 0.0f, 0.0f, 0.0f, 100.0f, 350.0f, 0xF, 3);
    }
    switch (em->type) {
    case 7:
    case 8:
    case 9:
    case 0xB:
    case 0xC:
        YarareAdd(em, &w->hit[5], -20.0f, -300.0f, 0.0f, 200.0f, 300.0f, 0x13, 1);
        YarareAdd(em, &w->hit[6], 20.0f, -300.0f, 0.0f, 200.0f, 300.0f, 0x17, 1);
        break;
    default:
        YarareAdd(em, &w->hit[5], -20.0f, -300.0f, 0.0f, 170.0f, 300.0f, 0x13, 1);
        YarareAdd(em, &w->hit[6], 20.0f, -300.0f, 0.0f, 170.0f, 300.0f, 0x17, 1);
        break;
    }
    YarareAdd(em, &w->hit[7], -300.0f, 0.0f, 0.0f, 120.0f, 300.0f, 8, 3);
    YarareAdd(em, &w->hit[8], 0.0f, 0.0f, 0.0f, 120.0f, 300.0f, 0xE, 3);
    if (em->type == 0xA || em->type == 0xD) {
        YarareAdd(em, &w->hit[9], 0.0f, 50.0f, 0.0f, 200.0f, 0.0f, 0x25, 1);
    } else {
        YarareAdd(em, &w->hit[9], 0.0f, 0.0f, 0.0f, 300.0f, 0.0f, 5, 0);
    }
    w->x69C = 0x2C;
    w->x69D = 0x2D;
    w->x69E = 0x2E;
    w->x69F = 0x2F;
    w->x6A0 = 0x30;
    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000 && em->emsetNo == 0) {
        em->be_flag |= 0x10000;
    }
    switch (em->x38D) {
    case 2:
    case 3:
    case 4:
    case 0x18:
    case 0x19:
    case 0x1C:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2C:
    case 0x2D:
    case 0x31:
    case 0x34:
        em->be_flag |= 0x10000;
        break;
    case 0x35:
        break;
    }
    em10InitRtnSet(em);
    if (em->type == 6) {
        em->be_flag |= 0x10000;
    }
    if (pG->flags_6C & 0x200) {
        em->flags_3C8 &= ~0x100000;
    }
    MotionMoveF(em, 0);
    em10_R0_Move(em);
    OSReport("em10 free size = 0x%x\n", sizeof(Em10Work));
}

static void em10_R0_Move(cEm10* em)
{
    Em10_R1_move_tbl[em->xFD].br(em);
    Em10_R1_move_tbl[em->xFD].move(em);
}

static void em10_R0_Damage(cEm10* em)
{
    EM10_WK(em)->flags |= 8;
    Em10_R1_dmg_tbl[em->xFD](em);
}

static void em10_R0_Die(cEm10* em)
{
    EM10_WK(em)->flags |= 8;
    Em10_R1_die_tbl[em->xFD](em);
}

// Initial routine from the enemy set number (cEm::x38D) and the work defaults.
void em10InitRtnSet(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void** mot0 = &w->evtMot[0];
    void** mot4 = &w->evtMot[4];
    u32 i;

    switch (em->type) {
    case 2:
    case 5:
    case 7:
    case 8:
    case 9:
    case 0xA:
    case 0xD:
        w->x69A = 2;
        break;
    default:
        w->x69A = Rnd() & 1;
        break;
    }
    w->x6AC = em->emsetNo % 7;
    if (em->x3CC <= 0.0f) {
        em->x3CC = 20000.0f;
    }
    w->flags = 0;
    w->x69B = Rnd() % 5 + 5;
    w->x67C = 0;
    w->x5B8 = 0;
    w->x5BC = 0;
    w->x5C4 = 0;
    w->x5C8 = 0.0f;
    w->x5CC = 0.0f;
    w->x5D0 = 0.0f;
    w->x674 = Rnd() % 450 + 150;
    w->x67E = Rnd() % 90 + 90;
    w->x66C = 0.0f;
    w->x680 = 0;
    w->x598.x = 0.0f;
    w->x598.y = 0.0f;
    w->x598.z = 0.0f;
    w->x634 = 0;
    w->x68C = 0;
    w->x668 = 0;
    w->x528 = 100000000.0f;
    w->x6AD = 0xFF;
    w->x524 = 100000000.0f;
    w->x52C = 100000000.0f;
    w->x530 = 100000000.0f;
    w->x664 = Rnd() % 150;
    w->x6B5 = 2;
    w->x6B6 = 0;
    w->x5D8 = 1.0f;
    w->x58C = 0;
    w->x660 = 0;
    w->sndId = 0;
    w->x670 = 0;
    w->x696 = 0;
    w->x5EC = 0;
    w->x682 = 0;
    w->pParasite = 0;
    for (i = 0; i < 5; i++) {
        w->x578[i] = 0;
    }
    w->pTruck = 0;
    w->x658 = 0;
    w->x6B7 = 0;
    w->x6AE = 0;
    w->x6B8 = 0;
    w->x6B9 = 0;
    w->x6BA = 0;
    w->x64C = 0.0f;
    w->x6BB = Rnd() % 3 + 2;
    w->x6A3 = 0;
    w->x640 = 0;
    w->x694 = 0;
    w->x65E = 0;
    w->x6BC = 0;
    w->pSwitch = 0;
    w->pDragon = 0;
    w->pGondola = 0;
    w->x6BE = 0;
    w->x6BF = 0;
    w->x646 = 450;
    w->x686 = Rnd() % 150 + 150;
    w->x6C4 = 0;
    w->x594 = 0;
    w->x6C0 = 0;
    w->x564 = 0;
    w->pGatling = 0;
    w->x654 = 0;
    w->x656 = 0;
    w->x6C1 = 0;
    w->x6C3 = 0;
    w->x6A6 = 0;
    w->x6A8 = 0;
    w->x590 = 0;
    for (i = 0; i < 3; i++) {
        mot0[i] = 0;
        mot4[i] = 0;
    }
    if (em->type == 0x17) {
        EstSetEm(em, -1, 0, 0, 0x10, 0x8E, 0x800, w->x69D, em, 0);
    } else if (em->type == 0x19) {
        EstSetEm(em, -1, 0, 0, 0x10, 0x8F, 0x800, w->x69D, em, 0);
    } else if (GetEm10EyeEffectEnable()) {
        EstSetEm(em, -1, 0, 0, 0, 0x35, 0x800, w->x69D, em, 0);
    }
    if (em->type == 0xA || em->type == 0xD) {
        Vec pos;
        Vec rot;
        pos.x = pos.y = pos.z = 0.0f;
        rot.x = rot.y = rot.z = 0.0f;
        w->pParasite = (cObj16*) SetObj16(PL_ARC_PTR(em->subArc, 0x227), PL_ARC_PTR(em->subArc, 0x228), em, em, 0x24, 4, &pos, &rot);
        if (w->pParasite) {
            PlArc* arc = em->subArc;
            w->pParasite->setMotData(PL_ARC_PTR(arc, 0x229), PL_ARC_PTR(arc, 0x229), PL_ARC_PTR(arc, 0x229), PL_ARC_PTR(arc, 0x229),
                                     PL_ARC_PTR(arc, 0x229), PL_ARC_PTR(arc, 0x229), PL_ARC_PTR(arc, 0x229), PL_ARC_PTR(arc, 0x22A),
                                     PL_ARC_PTR(arc, 0x22A), PL_ARC_PTR(arc, 0x229), PL_ARC_PTR(arc, 0x229));
        }
    }
    if (em->x38D < 0x14 || em->x38D > 0x15) {
        if (w->wepType == 7) {
            w->pWep->setEffAlways(0x10, 0x14);
            w->pWep->setEffFall(0x10, 0x1C);
        }
        if (w->wepType == 8 && w->pWep) {
            EstSetEm(w->pWep, -1, 0, 0, 0x10, 0x1F, 0, w->x69F, w->pWep, 0);
        }
    }
    if (em->x38D == 0x1D) {
        w->x6B9 = 1;
    }
    if (em->x38D == 0x21) {
        w->x6BA = 1;
    }
    if ((em->flags_3C8 & 0x40) && G_ROOM_ID == 0x206) {
        em->setFindPL();
    }
    switch (em->x38D) {
    default:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        if (em->type == 6 || (em->x3D0 != 1 && em->x3D0 != 3)) {
            EmRoutineSet(em, 1, 0, 0, 0);
        } else {
            EmRoutineSet(em, 1, 1, 0, 0);
        }
        break;
    case 0xE:
    case 0x1D:
    case 0x21:
        em10SetWalkMotion(em, 0);
        em->setStatus(5);
        em->setFindPL();
        w->x6AC = Rnd() % 11;
        EmRoutineSet(em, 1, 0x10, 0, 0);
        break;
    case 0x1B:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 5, 0, 0);
        break;
    case 2:
    case 3:
    case 4:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 6, 0, 0);
        break;
    case 5:
    case 6:
    case 0x1A:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 7, 0, 0);
        break;
    case 7:
    case 8:
    case 9:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 8, 0, 0);
        break;
    case 0xA:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 8, 0, 0);
        break;
    case 0xB:
        if (w->mot[48] && w->mot[49]) {
            w->pHead = (cEm*) SetObj12(w->mot[48], w->mot[49], &em->pos, &em->rot);
        }
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 9, 0, 0);
        break;
    case 0xC:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0xA, 0, 0);
        break;
    case 0x2F:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0xA, 2, 0);
        break;
    case 0xD:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0xB, 0, 0);
        break;
    case 0xF:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x18, 0, 0);
        break;
    case 0x10:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x18, 0, 1);
        break;
    case 0x11:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x1A, 0, 1);
        break;
    case 0x13:
        em10SetWaitMotion(em, 0);
        em->clearStatus(5);
        EmRoutineSet(em, 1, 2, 0, 0);
        break;
    case 0x14:
        em10SetWaitMotion(em, 0);
        em->clearStatus(5);
        EmRoutineSet(em, 1, 3, 0, 0);
        break;
    case 0x15:
        em10SetWaitMotion(em, 0);
        em->clearStatus(5);
        EmRoutineSet(em, 1, 4, 0, 0);
        break;
    case 0x16:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x19, 0, 0);
        break;
    case 0x17:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x19, 0, 1);
        break;
    case 0x18:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x47, 0, 0);
        break;
    case 0x19:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x48, 0, 0);
        break;
    case 0x1C:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x49, 0, 0);
        break;
    case 0x1E:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x4A, 0, 0);
        break;
    case 0x30:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x4B, 0, 0);
        break;
    case 0x1F:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x4C, 0, 0);
        break;
    case 0x20:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x4D, 0, 0);
        break;
    case 0x22:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x4E, 0, 0);
        break;
    case 0x23:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x4F, 0, 0);
        break;
    case 0x24:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x50, 0, 0);
        break;
    case 0x25:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x50, 0, 1);
        break;
    case 0x2B:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x50, 0, 2);
        break;
    case 0x26:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x51, 0, 0);
        break;
    case 0x27:
        w->pDragon = GetCtrlDragon(0);
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        em->atari.throughOn();
        EmRoutineSet(em, 1, 0x52, 0, 0);
        break;
    case 0x28:
        w->pDragon = GetCtrlDragon(1);
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        em->atari.throughOn();
        EmRoutineSet(em, 1, 0x53, 0, 0);
        break;
    case 0x29:
        w->pDragon = GetCtrlDragon(2);
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        em->atari.throughOn();
        EmRoutineSet(em, 1, 0x54, 0, 0);
        break;
    case 0x2A:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x55, 0, 0);
        break;
    case 0x2C:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x56, 0, 0);
        break;
    case 0x31:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x56, 0, 1);
        break;
    case 0x2D:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x57, 0, 0);
        break;
    case 0x2E:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x58, 0, 0);
        break;
    case 0x32:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x5A, 0, 0);
        break;
    case 0x33:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x5B, 0, 0);
        break;
    case 0x34:
        em10SetWaitMotion(em, 0);
        em->clearStatus(5);
        EmRoutineSet(em, 1, 0x5C, 0, 0);
        break;
    case 0x35:
        em10SetWaitMotion(em, 0);
        em->clearStatus(5);
        EmRoutineSet(em, 1, 0x5E, 0, 0);
        break;
    case 0x36:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x60, 0, 0);
        break;
    case 0x37:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x61, 0, 0);
        break;
    case 0x38:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x62, 0, 0);
        break;
    case 0x39:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x63, 0, 0);
        break;
    case 0x3A:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x64, 0, 0);
        break;
    case 0x3B:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        em->scale.x = 1.0f;
        em->scale.y = 1.0f;
        em->scale.z = 1.0f;
        w->scaleBase.z = 1.0f;
        w->scaleBase.x = 1.0f;
        w->scaleBase.y = 1.0f;
        EmRoutineSet(em, 1, 0x66, 0, 0);
        break;
    case 0x3C:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x68, 0, 0);
        break;
    case 0x3D:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x69, 0, 0);
        break;
    case 0x3E:
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x6A, 0, 0);
        break;
    case 0x3F: {
        MotionData* mot = (MotionData*) PL_ARC_PTR(em->subArc, 0x67);
        MotionSetCore(em, MOTION(em), mot, 0, 0, 1, (u16) ((mot->maxFrame & 0x3FFF) - 1));
        em->hp = 0;
        em->clearStatus(5);
        EmRoutineSet(em, 1, 0x6B, 0, 0);
        break;
    }
    case 0x40:
        w->flags |= 0x100;
        em10SetWaitMotion(em, 0);
        em->setStatus(5);
        EmRoutineSet(em, 1, 0x6C, 0, 0);
        break;
    }
    if (em->type == 6) {
        em->clearStatus(5);
    }
    switch (em->type) {
    case 2:
        em10BeltSet(em);
        break;
    case 0xA:
    case 0xD:
        em10ChainSet(em);
        break;
    }
}

static void em10_R1_Wait(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int ret;

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0xA);
        if (w->pParasite || w->x58C) {
            em->setWeaponFall();
            if (w->pShield) {
                w->pShield->setFall(20.0f, 0);
                w->pShield = 0;
            }
        }
        em->xFE++;
    case 1:
        if (em->flags_3C8 & 8) {
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, PI);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        MotionMoveF(em, 0);
        if (pG->flags_64 & 0x02000000) {
            break;
        }
        ret = em10GotoCk(em);
        if (ret) {
            break;
        }
        if (em->type == 6) {
            em->setFindPL();
            w->flags |= 0x40000;
            break;
        }
        if ((s16) pG->pl_life <= 0 || (pSUB && (s16) pG->sub_life <= 0)) {
            em->xFC = 1;
            em->xFD = 0x1B;
            em->xFE = 0;
            em->xFF = 0;
            break;
        }
        if (em10FindCk(em, 0)) {
            break;
        }
        if (em->flags_3C8 & 1) {
            em->setFindPL();
        }
        if (w->flags & 0x100) {
            em10FindNotify(em);
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
    em10ActEvtSetTrade(em);
}

static void em10_R1_Keeper(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    f32 ang;

    if ((EM10_WK(em)->flags & 0x100) && em->xFE == 0 && w->x508 > 1.5707964f) {
        em->xFE = 2;
    }
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0xA);
        em->xFE++;
    case 1:
        if (w->flags & 0x100) {
            w->flags |= 0x40000;
        }
        if (w->flags & 0x100) {
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.024543693f);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        } else if (em->flags_3C8 & 8) {
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, PI);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (em->plDist2 < 2500000000.0f || (em->flags_3C8 & 0x40) || G_ROOM_ID == 0x320) {
            w->flags &= ~0x400000;
            if (!(em->be_flag & 2)) {
                em->be_flag |= 2;
                em->dmType = 0;
            }
            em->atari.flags |= 0x300;
            em->alpha = 1.0f;
            MotionMoveF(em, 0);
            if (pG->flags_64 & 0x02000000) {
                break;
            }
            if (em10GotoCk(em)) {
                return;
            }
            if (w->flags & 0x100) {
                if (w->x52C < em->x3CC || (em->flags_3C8 & 0x40) || !(w->flags & 0x08000000)) {
                    em10WalkRtnSet(em);
                    break;
                }
            } else if (em10FindCk(em, 0)) {
                break;
            }
            if (em->flags_3C8 & 1) {
                em->setFindPL();
            }
        } else {
            if (em->alpha > 0.0f) {
                em->alpha -= 0.1f;
            } else {
                w->flags |= 0x400000;
                em->alpha = 0.0f;
                em->be_flag &= ~2;
                em->atari.flags &= ~0x300;
                em->dmType = 0x80;
            }
        }
        if (em->type == 6) {
            em->setFindPL();
            w->flags |= 0x40000;
        }
        break;
    case 2: {
        void* m0 = PL_ARC_PTR(em->subArc, 0x18);
        void* m1 = PL_ARC_PTR(em->subArc, 0x19);
        int hokan = 1;
        if (w->x518 < 0.0f) {
            hokan = 0x41;
        }
        if (w->pShield) {
            m0 = PL_ARC_PTR(em->subArc, 0x16C);
            m1 = PL_ARC_PTR(em->subArc, 0x16D);
            hokan = 1;
            if (em->flags_3C8 & 0x01000000) {
                hokan = 0x41;
            }
        }
        if (em->type == 0xA || em->type == 0xD) {
            m0 = PL_ARC_PTR(em->subArc, 0x116);
            m1 = PL_ARC_PTR(em->subArc, 0x117);
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, hokan, 0);
        w->x18 = em->rot.y + PI;
        w->x4 = 60;
        em->xFE++;
    }
    case 3:
        if (em->seFlags28B & 8) {
            ang = Muku(&em->pos, &pPL->pos, w->x18, 0.09817477f);
            w->x18 += ang;
            w->x18 = LIMIT_ANGLE(w->x18);
            em->rot.y += ang;
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (MotionMoveF(em, 0)) {
            em->xFE = 0;
        } else {
            em10AtkRtnCk(em, 0);
        }
        break;
    }
    em10HandSet(em, 0);
    em10ActEvtSetTrade(em);
}

// Hidden (invisible, no collision) and appearing states shared by the hide routines.
static inline void em10HideOn(cEm10* em, Em10Work* w)
{
    em->be_flag &= ~2;
    EM10_WK(em)->flags |= 0x400000;
    em->atari.flags &= ~0x300;
    em->alpha = 0.0f;
    em->dmType = 0x80;
    w->x6B6 = 1;
}

static inline void em10HideOff(cEm10* em, Em10Work* w)
{
    em->be_flag |= 2;
    em->atari.flags |= 0x300;
    MotionMoveF(em, 0);
    em->alpha = 1.0f;
    w->x6B6 = 0;
    em->setFindPL();
    em->setStatus(5);
    em->dmType = 0;
    em->x38D = 0;
    w->flags &= ~0x400000;
    if (w->wepType == 7) {
        w->pWep->setEffAlways(0x10, 0x14);
        w->pWep->setEffFall(0x10, 0x1C);
    }
    if (w->wepType == 8 && w->pWep) {
        EstSetEm(w->pWep, -1, 0, 0, 0x10, 0x1F, 0, w->x69F, w->pWep, 0);
    }
}

static void em10_R1_Hide(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWalkMotion(em, 0);
        MotionMoveF(em, 0);
        em10HideOn(em, w);
        em->xFE++;
    case 1:
        if (!(em->flags_3C8 & 1)) {
            break;
        }
        w->flags |= 4;
        em10RouteCk(em);
        em->xFE++;
    case 2:
        em10SetWalkMotion(em, 0);
        MotionMoveF(em, 0);
        em10HideOff(em, w);
        if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000) {
            w->x674 = 150;
        }
        em10WalkRtnSet(em);
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_HideFall(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        MotionMoveF(em, 0);
        em10HideOn(em, w);
        em->xFE++;
    case 1:
        if (!(em->flags_3C8 & 1)) {
            break;
        }
        em->xFE++;
    case 2:
        em10HideOff(em, w);
        {
            f32 y = SatMgr.getFloor(&em->pos, 600.0f, 100000.0f, 0, 0);
            w->x4D8 = em->pos;
            w->x4D8.y = y;
        }
        w->x5E0 = em->pos;
        EmRoutineSet(em, 1, 0x43, 0, 0);
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_HideJump(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    f32 y;

    switch (em->xFE) {
    case 0:
        em10SetWalkMotion(em, 0);
        MotionMoveF(em, 0);
        em10HideOn(em, w);
        em->xFE++;
    case 1:
        if (!(em->flags_3C8 & 1)) {
            break;
        }
        em->xFE++;
    case 2:
        em10HideOff(em, w);
        em->xFF = 0;
        em10SetDashMotion(em);
        w->x6AC = 0;
        RouteCkToPos(em, &pPL->pos, &w->x534, 0, 0);
        w->x504 = Muku(&em->pos, &w->x534, em->rot.y, PI);
        w->x508 = fabsf(w->x504);
        w->x54C = w->x534;
        w->x518 = w->x504;
        w->x51C = w->x508;
        w->x520 = em->plDist2;
        em->xFE++;
    case 3:
        em->dmType = 0xA;
        w->flags |= 0x1000;
        em->rot.y += Muku(&em->pos, &w->x54C, em->rot.y, 0.15707964f);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        MotionMoveF(em, 0);
        v.y = 0.0f;
        v.x = 0.0f;
        v.z = 600.0f;
        PSMTXMultVec(em->mat, &v, &v);
        y = SatMgr.getFloor(&v, 600.0f, 100000.0f, 0, 0);
        if (y < em->pos.y - 350.0f) {
            w->x4D8 = em->pos;
            w->x4D8.y = y;
            em->dmType = 0xA;
            w->x5E0 = em->pos;
            EmRoutineSet(em, 1, 0x43, 0, 0);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R10CParasite(cEm10* em)
{
    em->dmType = 2;
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->xFE++;
        }
        break;
    case 2:
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        } else if (em->frame > 200.7f && em->frame < 201.3f) {
            em->flags_3C8 |= 0x100000;
            em10LostHead(em, 2, 0);
        }
        break;
    case 3:
        em10SetWalkMotion(em, 7);
        em->xFE++;
    case 4:
        MotionMoveF(em, 0);
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R10CPCancel(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    em->dmType = 2;
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        w->flags |= 0x80;
        w->x6AE = 1;
        Ctrl12CntAddI(w->pCtrl12, 4, -1);
        Ctrl12CntAddI(w->pCtrl12, 4, 1);
        em10SetParasite(em);
        em10HeadSet(em, 1);
        EffectEspDelete(0, w->x69D, (u32) em, 0);
        EffectEspgenDelete(0, w->x69D, (int) em);
        EffectEfmDelete(0, w->x69D, (int) em);
        if (w->x1A0) {
            w->x1A0->be_flag &= ~8;
        }
        if (w->x1A4) {
            w->x1A4->be_flag &= ~8;
        }
        if (w->x1A8) {
            w->x1A8->be_flag &= ~8;
        }
        if (w->x1AC) {
            w->x1AC->be_flag &= ~8;
        }
        if (w->x1B4) {
            w->x1B4->be_flag &= ~8;
        }
        if (w->x1B8) {
            w->x1B8->be_flag &= ~8;
        }
        if (w->x1BC) {
            w->x1BC->be_flag &= ~8;
        }
        em->flags_3C8 |= 0x100000;
        em->setFindPL();
        em10WalkRtnSet(em);
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R204Prayer(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        w->x4 = 10;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em10FindCk(em, 1)) {
            break;
        }
        if ((w->flags & 0x100) || w->x5EC) {
            if (w->x4) {
                w->x4--;
            } else if (!em10GotoCk(em)) {
                em10WalkRtnSet(em);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

// Dragon statue control (game/ctrl14.cpp) as the Ganado riders call it (room 222).
class cCtrlDragon : public cCtrl {
public:
    virtual void setTarget(Vec* pos);           // 0x30
    virtual void getMtx(Mtx m, int flag);       // 0x38
    virtual f32 getAngle();                     // 0x40
    virtual f32 getFireAngle();                 // 0x48
    virtual void setAngleX(f32 x);              // 0x50
    virtual void setAngleY(f32 y);              // 0x58
    virtual void addAngle(f32 d);               // 0x60
    virtual void setFireAngle(f32 a);           // 0x68
    virtual void setHome();                     // 0x70
    virtual void setFire();                     // 0x78
    virtual int ckHitFire(Vec* p);              // 0x80
    virtual int ckHitFireBlocked();             // 0x88
};

#define EM10_DRAGON(w) ((cCtrlDragon*) (w)->pDragon)

static void em10_R1_R222DragonA(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx m;
    Mtx minv;
    Vec pl;
    Vec v;
    f32 x;
    f32 y;
    f32 ay;
    f32 ang;

    em->atari.throughOn();
    em->setStatus(3);
    switch (em->xFE) {
    case 0:
        EM10_DRAGON(w)->getMtx(m, 1);
        v.x = 0.0f;
        v.y = -1000.0f;
        v.z = -1000.0f;
        PSMTXMultVec(m, &v, &em->pos);
        em->dmType = 0x1E;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xAE), (int) PL_ARC_PTR(em->subArc, 0xAF), 3, 5, 0);
        w->x20 = 0;
        w->x4 = 90;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->pDragon == 0) {
            break;
        }
        EM10_DRAGON(w)->getMtx(m, 1);
        v.x = 0.0f;
        v.y = -1000.0f;
        v.z = 2000.0f;
        PSMTXMultVec(m, &v, &v);
        if (w->x20) {
            if (w->x4) {
                w->x4--;
                PosToPos(&em->pos, &v, &em->pos, 0.1f);
            } else {
                em->xFE++;
            }
        } else if ((em->pos.x - v.x) * (em->pos.x - v.x) + (em->pos.z - v.z) * (em->pos.z - v.z) < 10000.0f) {
            em10SetWaitMotion(em, 0xA);
            w->x20 = 1;
            if (w->pDragon) {
                EM10_DRAGON(w)->setFire();
            }
        }
        break;
    case 2:
        em10SetWaitMotion(em, 0xA);
        w->x4 = 90;
        em->xFE++;
    case 3:
        if (w->pDragon) {
            EM10_DRAGON(w)->getMtx(m, 1);
            v.x = 0.0f;
            v.y = -1000.0f;
            v.z = 2000.0f;
            PSMTXMultVec(m, &v, &em->pos);
            EM10_DRAGON(w)->getMtx(m, 0);
            PSMTXInverse(m, minv);
            PSMTXMultVec(minv, &pPL->pos, &pl);
            x = pl.x;
            if (x > 50.0f) {
                x = 50.0f;
            }
            if (x < -50.0f) {
                x = -50.0f;
            }
            EM10_DRAGON(w)->setAngleX(x);
            pl.y += 2000.0f;
            y = pl.y;
            ay = fabsf(y);
            if (y > 50.0f) {
                y = 50.0f;
            }
            if (y < -50.0f) {
                y = -50.0f;
            }
            EM10_DRAGON(w)->setAngleY(y);
            EM10_DRAGON(w)->setTarget(&pl);
            ang = EM10_DRAGON(w)->getAngle();
            ang = Muku(&pl, &pPL->pos, ang, 0.0015339808f);
            EM10_DRAGON(w)->addAngle(ang);
            if (w->x4 == 0) {
                if (ay < 150.0f && fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, PI)) < 0.5235988f &&
                    (em->pos.x - pPL->pos.x) * (em->pos.x - pPL->pos.x) + (em->pos.y - pPL->pos.y) * (em->pos.y - pPL->pos.y) +
                            (em->pos.z - pPL->pos.z) * (em->pos.z - pPL->pos.z) <
                        400000000.0f) {
                    em->xFE++;
                    break;
                }
            } else {
                w->x4--;
            }
        }
        MotionMoveF(em, 0);
        break;
    case 4:
        em10SetWaitMotion(em, 0);
        w->x4 = 150;
        w->x20 = 0;
        EM10_DRAGON(w)->setFire();
        w->x18 = 0.0f;
        w->x1C = EM10_DRAGON(w)->getFireAngle();
        w->x8 = 30;
        w->xC = 15;
        em->xFE++;
    case 5:
        if (w->pDragon) {
            EM10_DRAGON(w)->getMtx(m, 1);
            v.x = 0.0f;
            v.y = -1000.0f;
            v.z = 2000.0f;
            PSMTXMultVec(m, &v, &em->pos);
            if (w->x4 > 30) {
                if (w->x8) {
                    w->x8--;
                } else {
                    ang = SINF(w->x18) * 0.19634955f + w->x1C;
                    EM10_DRAGON(w)->setFireAngle(ang);
                    w->x18 += 0.06981317f;
                    if (w->xC) {
                        w->xC--;
                    } else {
                        em10DragonFireCk(em);
                    }
                }
            }
        }
        MotionMoveF(em, 0);
        if (w->x4) {
            w->x4--;
        } else {
            em->xFE = 2;
        }
        break;
    }
    if (em->hp <= 0) {
        w->x5DC = em->rot.y;
        if (Rnd() & 1) {
            w->x5DC += 1.5707964f;
        } else {
            w->x5DC -= 1.5707964f;
        }
        w->x5DC = LIMIT_ANGLE(w->x5DC);
        EmRoutineSet(em, 2, 7, 0, 1);
    } else {
        em10HandSet(em, 0);
    }
}
static void em10_R1_R222DragonB(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx m;
    Mtx minv;
    Vec pl;
    Vec v;
    f32 x;
    f32 y;
    f32 ay;
    f32 ang;

    em->atari.throughOn();
    em->setStatus(3);
    switch (em->xFE) {
    case 0:
        EM10_DRAGON(w)->getMtx(m, 1);
        v.x = 0.0f;
        v.y = -1000.0f;
        v.z = -1000.0f;
        PSMTXMultVec(m, &v, &em->pos);
        em->dmType = 0x1E;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xAE), (int) PL_ARC_PTR(em->subArc, 0xAF), 3, 5, 0);
        w->x20 = 0;
        w->x4 = 90;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->pDragon == 0) {
            break;
        }
        EM10_DRAGON(w)->getMtx(m, 1);
        v.x = 0.0f;
        v.y = -1000.0f;
        v.z = 2000.0f;
        PSMTXMultVec(m, &v, &v);
        if (w->x20) {
            if (w->x4) {
                w->x4--;
                PosToPos(&em->pos, &v, &em->pos, 0.1f);
            } else {
                em->xFE++;
            }
        } else if ((em->pos.x - v.x) * (em->pos.x - v.x) + (em->pos.z - v.z) * (em->pos.z - v.z) < 10000.0f) {
            em10SetWaitMotion(em, 0xA);
            w->x20 = 1;
            if (w->pDragon) {
                EM10_DRAGON(w)->setFire();
            }
        }
        break;
    case 2:
        em10SetWaitMotion(em, 0xA);
        w->x4 = 90;
        em->xFE++;
    case 3:
        if (w->pDragon) {
            EM10_DRAGON(w)->getMtx(m, 1);
            v.x = 0.0f;
            v.y = -1000.0f;
            v.z = 2000.0f;
            PSMTXMultVec(m, &v, &em->pos);
            EM10_DRAGON(w)->getMtx(m, 0);
            PSMTXInverse(m, minv);
            PSMTXMultVec(minv, &pPL->pos, &pl);
            x = pl.x;
            if (x > 50.0f) {
                x = 50.0f;
            }
            if (x < -50.0f) {
                x = -50.0f;
            }
            EM10_DRAGON(w)->setAngleX(x);
            pl.y += 2000.0f;
            y = pl.y;
            ay = fabsf(y);
            if (y > 50.0f) {
                y = 50.0f;
            }
            if (y < -50.0f) {
                y = -50.0f;
            }
            if (pPL->pos.x > -32000.0f) {
                y = 50.0f;
                if (w->x4 <= 29) {
                    w->x4 = 30;
                }
            }
            EM10_DRAGON(w)->setAngleY(y);
            EM10_DRAGON(w)->setTarget(&pl);
            ang = EM10_DRAGON(w)->getAngle();
            ang = Muku(&pl, &pPL->pos, ang, 0.0015339808f);
            EM10_DRAGON(w)->addAngle(ang);
            if (w->x4 == 0) {
                if (ay < 150.0f && fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, PI)) < 0.5235988f &&
                    (em->pos.x - pPL->pos.x) * (em->pos.x - pPL->pos.x) + (em->pos.y - pPL->pos.y) * (em->pos.y - pPL->pos.y) +
                            (em->pos.z - pPL->pos.z) * (em->pos.z - pPL->pos.z) <
                        400000000.0f) {
                    em->xFE++;
                    break;
                }
            } else {
                w->x4--;
            }
        }
        MotionMoveF(em, 0);
        break;
    case 4:
        em10SetWaitMotion(em, 0);
        w->x4 = 150;
        w->x20 = 0;
        EM10_DRAGON(w)->setFire();
        w->x18 = 0.0f;
        w->x1C = EM10_DRAGON(w)->getFireAngle();
        w->x8 = 30;
        w->xC = 15;
        em->xFE++;
    case 5:
        if (w->pDragon) {
            EM10_DRAGON(w)->getMtx(m, 1);
            v.x = 0.0f;
            v.y = -1000.0f;
            v.z = 2000.0f;
            PSMTXMultVec(m, &v, &em->pos);
            if (w->x4 > 30) {
                if (w->x8) {
                    w->x8--;
                } else {
                    ang = SINF(w->x18) * 0.19634955f + w->x1C;
                    EM10_DRAGON(w)->setFireAngle(ang);
                    w->x18 += 0.06981317f;
                    if (w->xC) {
                        w->xC--;
                    } else {
                        em10DragonFireCk(em);
                    }
                }
            }
        }
        MotionMoveF(em, 0);
        if (w->x4) {
            w->x4--;
        } else {
            em->xFE = 2;
        }
        break;
    }
    if (em->hp <= 0) {
        w->x5DC = em->rot.y;
        if (Rnd() & 1) {
            w->x5DC += 1.5707964f;
        } else {
            w->x5DC -= 1.5707964f;
        }
        w->x5DC = LIMIT_ANGLE(w->x5DC);
        EmRoutineSet(em, 2, 7, 0, 1);
    } else {
        em10HandSet(em, 0);
    }
}
static void em10_R1_R222DragonC(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx m;
    Mtx minv;
    Vec pl;
    Vec v;
    f32 y;
    f32 ay;
    f32 ang;

    em->atari.throughOn();
    em->setStatus(3);
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        w->x4 = 90;
        em->xFE++;
    case 1:
        if (w->pDragon) {
            EM10_DRAGON(w)->getMtx(m, 1);
            v.x = 0.0f;
            v.y = -1000.0f;
            v.z = 2000.0f;
            PSMTXMultVec(m, &v, &em->pos);
            EM10_DRAGON(w)->getMtx(m, 0);
            PSMTXInverse(m, minv);
            PSMTXMultVec(minv, &pPL->pos, &pl);
            pl.y += 2000.0f;
            y = pl.y;
            ay = fabsf(y);
            if (y > 200.0f) {
                y = 200.0f;
            }
            if (y < -200.0f) {
                y = -200.0f;
            }
            if (pPL->pos.x > -53000.0f && (s32) pG->flags_174 >= 0) {
                y = 200.0f;
                if (w->x4 <= 99) {
                    w->x4 = 100;
                }
                EM10_DRAGON(w)->setHome();
            } else {
                EM10_DRAGON(w)->setTarget(&pl);
                ang = EM10_DRAGON(w)->getAngle();
                ang = Muku(&pl, &pPL->pos, ang, 0.0061359233f);
                EM10_DRAGON(w)->addAngle(ang);
            }
            EM10_DRAGON(w)->setAngleY(y);
            if (w->x4 == 0) {
                if (ay < 150.0f && fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, PI)) < 1.1170107f &&
                    (em->pos.x - pPL->pos.x) * (em->pos.x - pPL->pos.x) + (em->pos.y - pPL->pos.y) * (em->pos.y - pPL->pos.y) +
                            (em->pos.z - pPL->pos.z) * (em->pos.z - pPL->pos.z) <
                        400000000.0f) {
                    em->xFE++;
                    break;
                }
            } else {
                w->x4--;
            }
        }
        MotionMoveF(em, 0);
        break;
    case 2:
        em10SetWaitMotion(em, 0);
        w->x4 = 150;
        w->x20 = 0;
        EM10_DRAGON(w)->setFire();
        w->x18 = 0.0f;
        w->x1C = EM10_DRAGON(w)->getFireAngle();
        w->x8 = 30;
        w->xC = 15;
        em->xFE++;
    case 3:
        if (w->pDragon) {
            EM10_DRAGON(w)->getMtx(m, 1);
            v.x = 0.0f;
            v.y = -1000.0f;
            v.z = 2000.0f;
            PSMTXMultVec(m, &v, &em->pos);
            if (w->x4 > 30) {
                if (w->x8) {
                    w->x8--;
                } else {
                    ang = SINF(w->x18) * 0.19634955f + w->x1C;
                    EM10_DRAGON(w)->setFireAngle(ang);
                    w->x18 += 0.06981317f;
                    if (w->xC) {
                        w->xC--;
                    } else {
                        em10DragonFireCk(em);
                    }
                }
            }
        }
        MotionMoveF(em, 0);
        if (w->x4) {
            w->x4--;
        } else {
            em->xFE = 0;
        }
        break;
    }
    if (em->hp <= 0) {
        w->x5DC = em->rot.y;
        if (Rnd() & 1) {
            w->x5DC += 1.5707964f;
        } else {
            w->x5DC -= 1.5707964f;
        }
        w->x5DC = LIMIT_ANGLE(w->x5DC);
        EmRoutineSet(em, 2, 7, 0, 1);
    } else {
        em10HandSet(em, 0);
    }
}

static void em10_R1_R227Barrel(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        w->x4 = 150;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->pos.y < pPL->pos.y + 2000.0f) {
            em10WalkRtnSet(em);
        } else if (!(em->flags_3C8 & 1)) {
            w->x4 = 0;
        } else if (w->x4) {
            w->x4--;
        } else {
            em->xFE++;
        }
        break;
    case 2:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x9C), (int) PL_ARC_PTR(em->subArc, 0x9D), 30, 1, 0);
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            em->xFE = 0;
        } else if ((em->seFlags28B & 1) && w->pSwitch) {
            ((cEmSwitch*) w->pSwitch)->setClose();
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R21BTrolleyJump(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            w->x5E0 = em->pos;
            em->x38D = 0;
            w->flags |= 0x10080000;
            em->xFC = 1;
            em->xFD = 0x43;
            em->xFE = 0;
            em->xFF = 0;
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R21BTrolleyJump2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->xFE++;
        }
        break;
    case 2:
        if (w->wepType == 7) {
            w->pWep->setEffAlways(0x10, 0x14);
            w->pWep->setEffFall(0x10, 0x1C);
        }
        if (w->wepType == 8 && w->pWep) {
            EstSetEm(w->pWep, -1, 0, 0, 0x10, 0x1F, 0, w->x69F, w->pWep, 0);
        }
        em->xFF = 0;
        em10SetDashMotion(em);
        w->x6AC = 0;
        em->xFE++;
    case 3:
        em->dmType = 0xA;
        MotionMoveF(em, 0);
        if (em10JumpDownCk(em)) {
            w->x5E0 = em->pos;
            em->x38D = 0;
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R303FireDash(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0: {
        cModelInfo* info;
        em10SetWaitMotion(em, 0);
        em->be_flag &= ~2;
        em->hp = 0;
        for (info = em->pInfo; info; info = info->pNext) {
            info->color[0] = 0x80;
            info->color[1] = 0x30;
            info->color[2] = 0x30;
        }
        if (w->x178) {
            ((cObj12*) w->x178)->setBurn();
        }
        w->x8 = 10;
        em->xFE++;
    }
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->be_flag |= 2;
            em->hp = em->hpMax;
            SndCall(6, 3, &em->pos, 0, 0, em);
            if (w->x4) {
                w->x4--;
            } else {
                EmRoutineSet(em, 1, 0x39, 0, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R10FGJump(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    Vec dir;
    Vec dir2;

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        w->x4 = 90;
        em->atari.throughOn();
        em->hp = 1;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->evtMot[0] && w->evtMot[1] && w->evtMot[2]) {
            cObjGondola* g = em10GetGondola(em);
            if (g) {
                f32 fr;
                if (em->x38D == 0x2C) {
                    fr = 682.0f;
                } else {
                    fr = 1286.0f;
                }
                if (g->motFrame == fr) {
                    w->pGondola = g;
                    em->xFE++;
                }
            }
        }
        break;
    case 2:
        MotionSetCore(em, MOTION(em), w->evtMot[0], 0, 3, 1, 0);
        w->x4 = 10;
        em->xFE++;
    case 3:
        if (w->x4) {
            w->x4--;
        } else {
            w->flags |= 0x100000;
        }
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        }
        break;
    case 4:
        MotionSetCore(em, MOTION(em), w->evtMot[1], 0, 3, 1, 0);
        if (w->pGondola) {
            w->pGondola->setVib();
            if ((s16) pG->pl_life > 0) {
                SetPlDamage((int) em, plem10DmGondolaShake);
            }
            if (pSUB && pSUB->hp > 0) {
                SetSubDamage((int) em, (void*) subem10DmGondolaShake);
            }
        }
        SndCall(6, 0xC, &em->pos, 0, 0, em);
        em->xFE++;
    case 5:
        w->flags |= 0x100000;
        if (w->pGondola) {
            cModel* parts = w->pGondola->getPartsPtr(1);
            dir.x = 0.0f;
            dir.y = 0.0f;
            dir.z = 1.0f;
            PSMTXMultVecSR(parts->mat, &dir, &dir);
            em->rot.y = atan2f(dir.x, dir.z);
            v.x = 51.9f;
            v.y = -2757.56f;
            v.z = -1419.8f;
            PSMTXMultVec(parts->mat, &v, &em->pos);
            if (em->frame > 41.7f && em->frame < 42.3f) {
                SndCall(6, 0xD, &em->pos, 0, 0, em);
                EstSetEm(em, -1, 0, 0, 1, 2, 0, 0, em, 0);
            }
        }
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        }
        break;
    case 6:
        MotionSetCore(em, MOTION(em), w->evtMot[2], 0, 3, 5, 0);
        w->x4 = 5;
        em->xFE++;
    case 7:
        w->flags |= 0x100000;
        if (w->pGondola) {
            cModel* parts = w->pGondola->getPartsPtr(1);
            dir2.x = 0.0f;
            dir2.y = 0.0f;
            dir2.z = 1.0f;
            PSMTXMultVecSR(parts->mat, &dir2, &dir2);
            em->rot.y = atan2f(dir2.x, dir2.z);
            v.x = 51.9f;
            v.y = -2757.56f;
            v.z = -1419.8f;
            PSMTXMultVec(parts->mat, &v, &em->pos);
        }
        MotionMoveF(em, 0);
        if (em->frame > 32.7f && em->frame < 33.3f) {
            SndCall(6, 0xD, &em->pos, 0, 0, em);
            EstSetEm(em, -1, 0, 0, 1, 2, 0, 0, em, 0);
        }
        if (w->x4 && em->frame > 32.7f && em->frame < 33.3f && w->pGondola) {
            w->x4--;
            if (w->x4 > 0) {
                w->pGondola->setDamage();
                w->pGondola->setVib();
            } else {
                w->pGondola->setBreak();
                w->pGondola->setVib();
                if ((s16) pG->pl_life > 0) {
                    SetPlDamage((int) em, plem10DmGondolaShake);
                    pPL->xFF = 1;
                }
                if (pSUB && pSUB->hp > 0) {
                    SetSubDamage((int) em, (void*) subem10DmGondolaShake);
                    pSUB->xFF = 1;
                }
                em->dmType = 0x80;
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

// Thrown-weapon attack parameters (.data): axe / dynamite throw, scythe throw.
// Attack parameters by attack number (em10AtkCk / em10BellAtkCk index it; the axe / scythe throws
// hand entries 5 / 6 to cEmWep::setThrow).
static EmAtkInfo Em10AtkTbl[19] = {
    { 250.0f, 8, 380, 0, 10, 0 },
    { 250.0f, 8, 380, 0, 10, 0 },
    { 500.0f, 8, 480, 0, 10, 0 },
    { 350.0f, 8, 480, 0, 10, 0 },
    { 350.0f, 8, 480, 0, 10, 0 },
    { 250.0f, 8, 380, 0, 10, 0 },
    { 750.0f, 8, 700, 0, 10, 0 },
    { 250.0f, 8, 400, 0, 10, 0 },
    { 250.0f, 8, 380, 0, 10, 0 },
    { 250.0f, 8, 700, 0, 10, 0 },
    { 250.0f, 8, 700, 0, 10, 0 },
    { 500.0f, 8, 10, 0, 10, 0 },
    { 500.0f, 8, 9999, 8, 10, 0 },
    { 200.0f, 8, 640, 0, 10, 0 },
    { 200.0f, 8, 1400, 0, 10, 0 },
    { 200.0f, 8, 900, 0, 10, 0 },
    { 200.0f, 8, 800, 0, 10, 0 },
    { 200.0f, 8, 0, 0, 10, 0 },
    { 250.0f, 8, 570, 0, 10, 0 },
};

static void em10_R1_R10FGondola(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec tgt = { 26400.0f, 10279.0f, -32116.0f };
    Vec pos;
    Vec d;
    Mtx m;
    Vec wpos;
    Vec spd;
    Vec diff;
    Vec plPos;
    Vec wpos2;
    cModel* parts;
    f32 len;
    f32 t;

    parts = pPL->getPartsPtr(0);
    PSVECSubtract(&parts->worldPos, &parts->x88, &d);
    len = SQRTF(d.x * d.x + d.y * d.y + d.z * d.z);
    PSVECScale(&d, &d, len / 350.0f + 10.0f);
    PSVECAdd(&pPL->pos, &d, &pos);
    pos.y += 1300.0f;
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->motFlags &= ~1;
        em->atari.throughOn();
        w->x4 = 90;
        em->xFE++;
    case 1:
        em->rot.y += Muku(&em->pos, &pos, em->rot.y, PI);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        MotionMoveF(em, 0);
        if ((em->pos.x - tgt.x) * (em->pos.x - tgt.x) + (em->pos.y - tgt.y) * (em->pos.y - tgt.y) + (em->pos.z - tgt.z) * (em->pos.z - tgt.z) <
            64000000.0f) {
            EmRoutineSet(em, 3, 3, 0, 0);
            return;
        }
        if (w->pWep == 0) {
            em->xFE = 4;
        } else {
            f32 lim;
            switch (em->emsetNo % 3) {
            default:
                lim = 400000000.0f;
                break;
            case 1:
                lim = 324000000.0f;
                break;
            case 2:
                lim = 256000000.0f;
                break;
            }
            if (em->plDist2 < lim && em->pos.y < pPL->pos.y) {
                em->xFE = 2;
            }
        }
        break;
    case 2: {
        int hokan = 0;
        if (em->flags_3C8 & 0x01000000) {
            hokan = 0x40;
        }
        if (w->wepType != 6) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x82), (int) PL_ARC_PTR(em->subArc, 0x83), 10, hokan, 0x10);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x13D), (int) PL_ARC_PTR(em->subArc, 0x13F), 10, hokan, 0);
        }
        w->x4 = 10;
        em10CallVoiceSe2(em, w->se6D2, 8);
        em->xFE++;
    }
    case 3:
        em->rot.y += Muku(&em->pos, &pos, em->rot.y, PI);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (MotionMoveF(em, 0)) {
            em->xFE++;
            break;
        }
        if (em->seFlags28B & 4) {
            w->x670 = 2;
        }
        if ((em->seFlags28B & 1) && w->pWep) {
            wpos.x = w->pWep->mat[0][3];
            wpos.y = w->pWep->mat[1][3];
            wpos.z = w->pWep->mat[2][3];
            PSVECSubtract(&pos, &wpos, &diff);
            len = SQRTF(diff.x * diff.x + diff.z * diff.z) * 0.0028571428f;
            t = diff.y / ((len + 1.0f) * 0.5f * len);
            spd.y = t * len;
            switch (Rnd() % 5) {
            case 0:
            default:
                spd.x = fRand1_1() * 10.0f;
                break;
            case 1:
                spd.x = 5.0f;
                break;
            case 2:
                spd.x = -5.0f;
                break;
            case 3:
                spd.x = 10.0f;
                break;
            case 4:
                spd.x = -10.0f;
                break;
            }
            spd.z = 350.0f;
            PSMTXRotRad(m, 'y', atan2f(diff.x, diff.z));
            PSMTXMultVecSR(m, &spd, &spd);
            if (w->wepType != 6) {
                w->pWep->setThrow(&spd, t, &Em10AtkTbl[5]);
            } else {
                plPos = pPL->pos;
                plPos.y += 1500.0f;
                wpos2.x = w->pWep->mat[0][3];
                wpos2.y = w->pWep->mat[1][3];
                wpos2.z = w->pWep->mat[2][3];
                PSVECSubtract(&plPos, &wpos2, &spd);
#line 6995 "D:/Bio4/Prog/em10.cpp"
                VECNormalize(&spd, &spd);
                PSVECScale(&spd, &spd, 250.0f);
                w->pWep->setThrowScythe(&spd, &Em10AtkTbl[6]);
            }
            w->x670 = 10;
            w->wepType = 0;
            w->pWep = 0;
        }
        break;
    case 4: {
        int hokan = 0;
        if (em->flags_3C8 & 0x01000000) {
            hokan = 0x40;
        }
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x76), (int) PL_ARC_PTR(em->subArc, 0x77), 10, hokan, 0);
        em->xFE++;
    }
    case 5:
        em->rot.y += Muku(&em->pos, &pos, em->rot.y, PI);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if ((em->seFlags28B & 4) && w->pWep2) {
            w->pWep = em10MakeWeapon(em, w->wep2Type);
            if (w->pWep) {
                w->wepType = w->wep2Type;
            } else {
                w->wepType = 0;
            }
            em10WeaponSet(em);
        }
        if (MotionMoveF(em, 0)) {
            em->xFE = 0;
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R209DashSit(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xAE), (int) PL_ARC_PTR(em->subArc, 0xAF), 0, 5, 0);
        w->x4 = 70;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->x4) {
            w->x4--;
        } else {
            EmRoutineSet(em, 1, 0x1A, 0, 1);
        }
        break;
    }
    em10HandSet(em, 0);
}

// Claw walk / claw attack shared tail: back to the walk or the dash.
static inline void em10ClawAtkEnd(cEm10* em, Em10Work* w)
{
    if (!(w->flags & 0x08000000) &&
        (em->pos.x - w->x534.x) * (em->pos.x - w->x534.x) + (em->pos.z - w->x534.z) * (em->pos.z - w->x534.z) > 9000000.0f) {
        w->x6AC = 0;
        EmRoutineSet(em, 1, 0x11, 0, 0);
    } else {
        em10WalkRtnSet(em);
    }
}

static void em10_R1_StickClaw(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->xFE == 0 && fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, PI)) > 1.9198622f) {
        em->xFE = 2;
    }
    if (em10FindCk2(em)) {
        w->x654 = 150;
    }
    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x115), 0, 3, 1, 0);
        EstSetEm(em, -1, 0, 0, 0x10, 0x76, 0, 0, em, 0);
        w->x4 = 25;
        w->x8 = 46;
        if (em->xFF == 0) {
            em10CallVoiceSe2(em, 0x71, 6);
        }
        if (w->x5EC == 0) {
            u8 r = Rnd() % 10;
            if (r > 4) {
                w->x5F0 = pPL->pos;
            }
            w->x654 = 150;
        }
        em->xFE++;
    case 1:
        if (em->frame > 9.7f && em->frame < 10.3f) {
            w->x6BE = 1;
            w->x6BF = 1;
        }
        if (MotionMoveF(em, 0) && !em10AtkRtnCk(em, 0)) {
            em10ClawAtkEnd(em, w);
        }
        break;
    case 2:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x116), (int) PL_ARC_PTR(em->subArc, 0x117), 10, 1, 0);
        em10CallVoiceSe2(em, 0x71, 6);
        em->xFF = 1;
        if (w->x5EC == 0) {
            u8 r = Rnd() % 10;
            if (r > 4) {
                w->x5F0 = pPL->pos;
            }
            w->x654 = 150;
        }
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0) || (em->seFlags28B & 1)) {
            if (!em10AtkRtnCk(em, 0)) {
                em10ClawAtkEnd(em, w);
            }
        } else if (em->frame > 9.7f && em->frame < 10.3f) {
            w->x6BE = 1;
            w->x6BF = 1;
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R11DAppear1(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        if (em->flags_3C8 & 0x01000000) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xE7), (int) PL_ARC_PTR(em->subArc, 0xE8), 10, 0x41, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xE7), (int) PL_ARC_PTR(em->subArc, 0xE8), 10, 1, 0);
        }
        EstSetEm(em, -1, 0, 0, 0x10, 0x2B, 0, 0, em, 0);
        w->x4 = 30;
        em->xFE++;
    case 1:
        if (em->seFlags28B & 4) {
            if (em->type == 0x16) {
                EstSetEm(w->pWep, -1, 0, 0, 0x10, 0x96, 0, w->x69C, w->pWep, 0);
            } else {
                EstSetEm(w->pWep, -1, 0, 0, 0x10, 9, 0, w->x69C, w->pWep, 0);
            }
            w->flags |= 0x80000000;
            w->x65C = Rnd() % 150 + 150;
            SndCall(6, 0x4C, &em->pos, 0, 0, em);
            w->x684 = 60;
            em10FindNotify(em);
        }
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        }
        break;
    case 2:
        em10SetDashMotion(em);
        em->xFE++;
    case 3:
        MotionMoveF(em, 0);
        if (!em10JumpDownCk(em)) {
            em10ClimbOverCk(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R11DAppear2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        EM10_WK(em)->flags |= 0x80000000;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->evtMot[0] == 0) {
            break;
        }
        em->xFE++;
    case 2:
        MotionSetCore(em, MOTION(em), w->evtMot[0], 0, 0, 1, 0);
        SndStop(w->x5C4, 0);
        w->x5C4 = SndCall(6, 0x50, &em->pos, 0, 0, em);
        SndStop(w->x5B8, 0);
        SndStop(w->x5BC, 0);
        w->flags |= 0x80000000;
        EstSetEm(w->pWep, -1, 0, 0, 0x10, 9, 0, w->x69C, w->pWep, 0);
        w->flags |= 0x80000000;
        w->x684 = 60;
        w->x65C = Rnd() % 150 + 150;
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        } else if (em->frame > 59.7f && em->frame < 60.3f) {
            SndStop(w->x5B8, 0);
            SndStop(w->x5BC, 0);
            w->x5B8 = SndCall(6, 0x3D, &em->pos, 0, 0, em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R212Drill(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->atari.throughOn();
        w->scaleBase.x = 1.0f;
        w->scaleBase.y = 1.0f;
        w->scaleBase.z = 1.0f;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->x564 && w->evtMot[0] && w->evtMot[1]) {
            em->setStatus(5);
            em->xFE++;
        }
        break;
    case 2:
        MotionSetCore(em, MOTION(em), w->evtMot[0], 0, 0, 5, 0);
        em->xFE++;
    case 3:
        v.x = w->x20 ? 465.71f : -604.1f;
        v.y = 550.9f;
        v.z = -1243.91f;
        PSMTXMultVec(((cModel*) w->x564)->getPartsPtr(0)->mat, &v, &em->pos);
        em->rot.y = ((cModel*) w->x564)->rot.y;
        MotionMoveF(em, 0);
        if (em->hp <= 0) {
            em->xFE++;
        }
        break;
    case 4:
        MotionSetCore(em, MOTION(em), w->evtMot[1], 0, 3, 1, 0);
        em10SetDamageVoice(em, w->se6CE, w->se6C6);
        em->xFE++;
    case 5:
        v.x = w->x20 ? 465.71f : -604.1f;
        v.y = 550.9f;
        v.z = -1243.91f;
        PSMTXMultVec(((cModel*) w->x564)->getPartsPtr(0)->mat, &v, &em->pos);
        em->rot.y = ((cModel*) w->x564)->rot.y;
        if (MotionMoveF(em, 0)) {
            EmSetDie(em);
            EmReserveDropItem(em);
            em10SetPoint(em);
            em->clearStatus(5);
        }
        break;
    }
    em10HandSet(em, 0);
}

int cEm10::ckFindPL()
{
    return (hp > 0 && checkStatus(5) && type != 6 && (EM10_WK(this)->flags & 0x100)) ? 1 : 0;
}

void cEm10::setFindPL()
{
    Em10Work* w = EM10_WK(this);

    if (hp > 0 && checkStatus(5) && type != 6) {
        w->flags |= 0x100;
        w->flags &= ~0x800000;
        w->x638 = 0;
        w->x6B8 = 1;
    }
}

void cEm10::clearFindPL()
{
    Em10Work* w = EM10_WK(this);

    if (hp > 0 && checkStatus(5) && type != 6) {
        w->flags &= ~0x100;
        w->flags &= ~0x800000;
        w->x638 = 0;
    }
}

void em10ChainSawMove(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->wepType == 4 && w->pWep) {
        cModel* p = w->pWep->getPartsPtr(1);
        p->rot.z = (pG->flags_51E4 & 1) ? 0.0f : 1.0f;
    }
}

extern "C" int em10GetWanderRouteEmi(cEm10* em);

u32 em10GetWanderRoute(cEm10* em)
{
    int n = em10GetWanderRouteEmi(em);
    if (n < 0) {
        return n;
    }
    n = RouteCkGetPointNumber();
    if (n <= 0) {
        return -1;
    }
    n = Rnd() % n;
    if ((Rnd() & 3) == 0) {
        n = RouteCkGetNearPoint(&pPL->pos);
    }
    return n;
}

int em10LostHeadCk(cEm10* em)
{
    if (pSys->region != 0) {
        return 1;
    }
    switch (em->type) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 11:
    case 12:
        if (GetEm10EyeEffectEnable()) {
            return 1;
        }
        return 0;
    }
    return 1;
}

extern "C" cModel* em10SearchTruck(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) == 1 && e->id == 0x3B) {
            *(cEm10**) ((u8*) e + 0x64C) = em;  // truck (em3b) work: driver
            w->pTruck = e;
            return (cModel*) 1;
        }
    }
    return 0;
}

void cEm10::setGatling(cObjGatling* g, void* m0, void* m1, void* m2, void* m3)
{
    Em10Work* w = EM10_WK(this);

    if (g) {
        w->evtMot[0] = m0;
        w->evtMot[1] = m1;
        w->evtMot[2] = m2;
        w->evtMot[3] = m3;
        w->pGatling = g;
        w->gatlingMode = 1;
        EmRoutineSet(this, 1, 0x5F, 0, 0);
        be_flag |= 0x10000;
        g->setRide(this);
    }
}

extern "C" void em10SetAtkWait(cEm10* em, int set)
{
    Em10Work* w = EM10_WK(em);
    s16 t = 30;

    if (pG->x4F88 <= 3) {
        t = 45;
    }
    if (pG->x4F88 <= 1) {
        t = 75;
    }
    w->x67C = t;
    if (set) {
        Ctrl12SetS(w->pCtrl12, 6, t);
        Ctrl12SetS(w->pCtrl12, 8, t);
    }
}

extern "C" void em10GetWanderRoutePos(cEm10* em, Vec* pos);

extern "C" u32 em10WanderRouteUpdate(cEm10* em, int no)
{
    Em10Work* w = EM10_WK(em);
    Vec pos;

    if (no > 0) {
        em10GetWanderRoutePos(em, &pos);
        f32 d = (em->pos.x - pos.x) * (em->pos.x - pos.x) + (em->pos.z - pos.z) * (em->pos.z - pos.z);
        if (!(d < 2250000.0f)) {
            if (w->x634 <= 30) {
                return no;
            }
        }
    }
    return em10GetWanderRoute(em);
}

extern "C" void em10GetWanderRoutePos(cEm10* em, Vec* pos)
{
    Em10Work* w = EM10_WK(em);
    EmiData* emi = (EmiData*) pG->pRoomEmi;

    if (emi && (int) w->x63C >= 0 && (int) w->x63C < emi->n) {
        EmiEntry* e = &emi->entry[w->x63C];
        if ((*(u32*) e & 0xFFFF0000) == 0x01030000) {
            *pos = e->pos;
            return;
        }
    }
    RouteCkGetPoint(w->x63C, pos);
}

extern "C" void em10ParasiteGoOut(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->x58C) {
        w->x58C->hp = 1000;
        if (w->flags & 0x20) {
            w->x58C->v98(1);
        } else {
            w->x58C->v98(0);
        }
        w->x58C = 0;
    }
}

extern "C" void em10ClothPartsSet(cEm10* em, int no)
{
    Em10Work* w = EM10_WK(em);
    cModelInfo* info;
    void* bin;

    if (em->type != 6) {
        return;
    }
    switch (no) {
    case 0:
    default:
        bin = w->mot[19];
        break;
    case 1:
        bin = w->mot[18];
        break;
    }
    info = ModInfoMgr.create(bin, w->mot[17]);
    if (info) {
        if (w->x194) {
            cModel_swapModelInfo(em, w->x194->pData, info);
        } else {
            em->addModel(info);
        }
        w->x194 = info;
    }
}

extern "C" void em10GoodsPartsSet(cEm10* em, int on)
{
    Em10Work* w = EM10_WK(em);

    if (em->type != 6) {
        return;
    }
    if (!w->x198) {
        cModelInfo* info = ModInfoMgr.create(w->mot[20], w->mot[0]);
        if (info) {
            em->addModel(info);
        }
        w->x198 = info;
    }
    if (!on) {
        w->x198->be_flag &= ~8;
    } else {
        w->x198->be_flag |= 8;
    }
}

void cEm10::setWeapon(void* bin, void* tpl, int type)
{
    Em10Work* w = EM10_WK(this);
    Vec pos;
    Vec rot;

    if (!w->pWep) {
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 0.0f;
        rot.x = 0.0f;
        rot.y = 0.0f;
        rot.z = 0.0f;
        w->pWep = SetWeapon(bin, tpl, &pos, &rot, 0);
        if (w->pWep) {
            w->wepType = type;
            em10WepSeEffSet(this, w->pWep, w->wepType);
            em10WeaponSet(this);
        }
    }
}

int em10FindLostCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->type != 10 && em->type != 13) {
        return 0;
    }
    if (em10FindCk2(em)) {
        w->x654 = 150;
    }
    if (w->x654 != 0) {
        w->x654--;
        if ((s16) w->x654 == 0) {
            EmRoutineSet(em, 1, 0x5D, 0, 0);
            return 1;
        }
    }
    return 0;
}

void em10ScaleCompress(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx m;
    Vec s;
    cModel* p;

    if (em->xFC == 3 && em->xFD == 3) {
        PSMTXIdentity(m);
        s.x = 1.0f;
        s.y = w->x5D8;
        s.z = 1.0f;
        ScaleMatrix(m, &s);
        for (p = em->pParts; p; p = p->pParts) {
            PSMTXConcat(m, p->worldMat, p->worldMat);
            p->worldMat[0][3] = p->mat[0][3];
            p->worldMat[1][3] = p->mat[1][3];
            p->worldMat[2][3] = p->mat[2][3];
        }
    }
}

int em10ClimbOverCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em10ClimbOverCk2(em)) {
    case 1:
        EmRoutineSet(em, 1, 0x3C, 0, 0);
        return 1;
    case 2:
        if ((em->flags_3C8 & 0x400) && w->x5EC == 0) {
            return 0;
        }
        EmRoutineSet(em, 1, 0x43, 0, 1);
        return 1;
    }
    return 0;
}

extern "C" int em10GetGoSub(cEm10* em)
{
    int n = 0;
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        if (!(EM10_WK(e)->flags & 0x08000000)) {
            continue;
        }
        n++;
    }
    return n;
}

int em10ChgParasiteCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (!(em->flags_3C8 & 0x100000) || em->hp > 0) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    if (Ctrl12CntCk(w->pCtrl12, 4, 2)) {
        return 0;
    }
    if (em->type != 8) {
        u8 r = Rnd() % 10;
        if (r > 4) {
            return 0;
        }
    }
    if (em10LostHead(em, 2, 0)) {
        return 1;
    }
    return 0;
}

void em10GatlingRollMove(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u8 on;

    if (em->type != 2) {
        return;
    }
    on = w->x6A6;
    if (on) {
        if (w->x6A8 == 0) {
            w->x6A8 = SndCall(6, 0x24, &em->pos, 0, 0, em);
        }
        em->getPartsPtr(0x22)->rot.x += 0.34906584f;
    } else if (w->x6A8) {
        SndStop(w->x6A8, 0);
        SndCall(6, 0x25, &em->pos, 0, 0, em);
        w->x6A8 = on;
    }
    w->x6A6 = 0;
}

// ===== STUBS (development only, removed as functions are written) =====
static void em10_R1_br_Dummy(cEm10* em)
{
}

static void em10_R1_br_Wait(cEm10* em)
{
}

static void em10_R1_R209Gatling(cEm10* em)
{
}

static void em10_R1_R201EventWait(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x108), 0, 0, 5, 0);
        em->atari.clrFlag100();
        em->xFE++;
    case 1:
        em->atari.flags |= 8;
        w->x6BC = 2;
        w->flags |= 8;
        MotionMoveF(em, 0);
        if (!w->evtMot[0] || !w->evtMot[1]) {
            break;
        }
        MotionMoveF(em, 0);
        em->xFE++;
    case 2:
        em->pos.x = 34588.95f;
        em->pos.y = -2003.77f;
        em->pos.z = -41650.69f;
        em->rot.y = -1.5707964f;
        MotionSetCore(em, MOTION(em), w->evtMot[0], 0, 0, 5, 0);
        em->xFE++;
    case 3:
        em->atari.flags |= 8;
        w->x6BC = 2;
        w->flags |= 8;
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->xFE++;
        }
        break;
    case 4:
        MotionSetCore(em, MOTION(em), w->evtMot[1], 0, 0, 5, 0);
        em->setStatus(5);
        em->setFindPL();
        w->x5F0 = pPL->pos;
        w->x4 = 30;
        em10CallVoiceSe2(em, w->se6CA, 8);
        em->xFE++;
    case 5:
        if (w->x4) {
            w->x4--;
            w->flags |= 8;
            if (w->x4 == 0) {
                em->atari.setFlag100();
            }
        }
        if (MotionMoveF(em, 0)) {
            em->atari.setFlag100();
            EmRoutineSet(em, 1, 0x59, 0, 0);
        } else if (MOTION(em)->seqFrame > 14.7f && MOTION(em)->seqFrame < 15.3f) {
            SndCall(6, 0x11, &em->pos, 0, 0, em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_FindLost(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 10);
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            w->flags |= 0x800000;
            w->flags &= ~0x100;
            w->x63C = em10GetWanderRoute(em);
            w->x656 = 0;
            if (w->x6BE != 4) {
                w->x6BE = 3;
            }
            if (w->x6BF != 4) {
                w->x6BF = 3;
            }
            em10WalkRtnSet(em);
        }
        if (em10FindCk2(em)) {
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R100WalkStay(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    em->dmg.x1 = 2;
    switch (em->xFE) {
    case 0:
        em10SetWalkMotion(em, 0);
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        }
        break;
    case 2:
        em10SetWaitMotion(em, 10);
        em->xFE++;
    case 3:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->flags_3C8 &= ~1;
            w->flags &= ~0x100;
            if (em->x3D0 != 1 && em->x3D0 != 3) {
                EmRoutineSet(em, 1, 0, 0, 0);
            } else {
                EmRoutineSet(em, 1, 1, 0, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R202Finger(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void* m0;
    void* m1;
    int flag;

    w->flags |= 0x8000;
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->xFE++;
    case 1:
        em->dmg.x1 = 2;
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->xFE++;
        }
        break;
    case 2:
        m0 = PL_ARC_PTR(em->subArc, 0x73);
        m1 = PL_ARC_PTR(em->subArc, 0x74);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        em->xFE++;
    case 3:
        em->dmg.x1 = 2;
        if (em->seFlags28B & 8) {
            w->flags |= 0x2000000;
        }
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        }
        break;
    case 4:
        em10SetWaitMotion(em, 10);
        em->xFE++;
    case 5:
        MotionMoveF(em, 0);
        break;
    }
    em10HandSet(em, 0);
}


static void em10_R1_AttackWait(cEm10* em)
{
}

static void em10_R1_R100TurnWalk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    PlArc* arc;
    void* m0;
    void* m1;
    int flag;

    em->dmg.x1 = 0x80;
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        w->x4 = 90;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->x4) {
            w->x4--;
            break;
        }
        em->xFE++;
    case 2:
        arc = em->subArc;
        m0 = PL_ARC_PTR(arc, 0x18);
        m1 = PL_ARC_PTR(arc, 0x19);
        flag = (w->x518 < 0.0f) ? 0x41 : 1;
        if (w->pShield) {
            m0 = PL_ARC_PTR(arc, 0x16C);
            m1 = PL_ARC_PTR(arc, 0x16D);
            flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        }
        if (em->type == 10 || em->type == 13) {
            m0 = PL_ARC_PTR(arc, 0x116);
            m1 = PL_ARC_PTR(arc, 0x117);
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        }
        break;
    case 4:
        em10SetWalkMotion(em, 7);
        em->xFE++;
    case 5:
        MotionMoveF(em, 0);
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R100Cliff(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void* m0;
    void* m1;
    u32 i;

    em->dmg.x1 = 0x80;
    switch (em->xFE) {
    case 0:
        if (w->evtMot[0]) {
            m0 = w->evtMot[0];
            m1 = w->evtMot[4];
        } else {
            m0 = PL_ARC_PTR(em->subArc, 5);
            m1 = 0;
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 0, 5, 0);
        MotionMoveF(em, 0);
        em->hp = 0;
        em->atari.flags &= ~0x200;
        em->xFE++;
        break;
    case 1:
        if (w->evtMot[0]) {
            m0 = w->evtMot[0];
            m1 = w->evtMot[4];
        } else {
            m0 = PL_ARC_PTR(em->subArc, 5);
            m1 = 0;
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 0, 5, 0);
        MotionMoveF(em, 0);
        w->x4 = (*(u16*) m0 & 0x3FFF) - 10;
        if (em->plDist2 < 784000000.0f || (em->flags_3C8 & 1)) {
            em->xFE++;
            em->flags_3C8 |= 1;
            if (em->x38D == 2) {
                SndCall(6, 4, &em->pos, 0, 0, em);
            }
            for (i = 0; i < EmMgr.nArray; i++) {
                cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
                if ((e->be_flag & 0x201) != 1) {
                    continue;
                }
                if (e->id <= 0xF) {
                    continue;
                }
                if (e->id > 0x20) {
                    continue;
                }
                if (e->hp <= 0) {
                    continue;
                }
                if (e == em) {
                    continue;
                }
                if (e->x38D <= 4 && e->x38D >= 2) {
                    e->flags_3C8 |= 1;
                }
            }
        }
        break;
    case 2:
        EmSetDie(em);
        MotionMoveF(em, 0);
        if (w->x4) {
            w->x4--;
        } else {
            em->alpha -= 0.1f;
            if (em->alpha <= 0.0f) {
                em->alpha = 0.0f;
                em->atari.flags &= 0xFCFF;
                em->be_flag &= ~2;
                em->clearStatus(5);
                em->xFE++;
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R101Bucket(cEm10* em)
{
}

static void em10_R1_R101Suki(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x1B5), (int) PL_ARC_PTR(em->subArc, 0x1B6), 3, 5, 0);
        w->x4 = Rnd() % 5 + 5;
        EstSetEm(em, -1, 0, 0, 0x10, 0x15, 0, w->x69E, em, 0);
        em->xFE++;
    case 1:
        if (em->seFlags28B & 0x20) {
            SndCall(6, 0x10, &em->pos, 0, 0, em);
        }
        if (em->seFlags28B & 1) {
            EstSetEm(em, -1, 0, 0, 0x10, 0xF, 0, 0, em, 0);
        }
        if (em->seFlags28B & 4) {
            EstSetEm(em, -1, 0, 0, 0x10, 0x11, 0, 0, em, 0);
        }
        if (MotionMoveF(em, 0)) {
            if (w->x4) {
                w->x4--;
                EstSetEm(em, -1, 0, 0, 0x10, 0x15, 0, w->x69E, em, 0);
            } else {
                em->xFE++;
            }
        }
        break;
    case 2:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x1B7), 0, 3, 1, 0);
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            em->xFE = 0;
        }
        break;
    }
    if (!em10FindCk(em, 0)) {
        if (w->flags & 0x100) {
            em10WalkRtnSet(em);
        } else {
            em10HandSet(em, 0);
        }
    }
}

static void em10_R1_Work(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        break;
    }
    if (!em10FindCk(em, 0)) {
        if (w->flags & 0x100) {
            em10WalkRtnSet(em);
        } else {
            em10HandSet(em, 0);
        }
    }
}

static void em10_R1_UFOCatch(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    em->dmg.x1 = 2;
    switch (em->xFE) {
    case 0:
        em->atari.throughOn();
        if (w->evtMot[0]) {
            MotionSetCore(em, MOTION(em), w->evtMot[0], 0, 10, 5, 0);
        } else {
            em10SetWaitMotion(em, 0);
        }
        em->flags_3C8 &= ~1;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->xFE++;
        }
        break;
    case 2:
        if (w->evtMot[1]) {
            MotionSetCore(em, MOTION(em), w->evtMot[1], 0, 3, 1, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x21), 0, 3, 1, 0);
        }
        em10SetDamageVoice(em, w->se6D4, w->se6D4);
        em->flags_3C8 &= ~1;
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            em->setLost();
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R300TakeAshley(cEm10* em)
{
    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x2A0), (int) PL_ARC_PTR(em->subArc, 0x2A6), 0, 5, 0);
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R30FBullJump(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    int end;

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR((PlArc*) pG->pRoomArc, 0x39), (int) PL_ARC_PTR((PlArc*) pG->pRoomArc, 0x3A), 3, 1, 0);
        em10CallVoiceSe2(em, w->se6D7, 8);
        em->xFE++;
    case 1:
        w->flags |= 0x10080000;
        em->setStatus(3);
        end = MotionMoveF(em, 0);
        if (em->seFlags28B & 4) {
            f32 y;
            v = em->pos;
            v.y = em->oldPos.y;
            y = SatMgr.getFloor(&v, 600.0f, 100000.0f, 0, 0);
            if (em->pos.y < y) {
                em->pos.y = y;
                w->x5A4.y = 0.0f;
                SndCall(8, 5, &em->pos, em->id, 0, em);
            } else if (!end) {
                break;
            }
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x25), 0, 3, 1, 0);
            MotionMoveF(em, 0);
            em->xFE = 2;
        }
        break;
    case 2:
        if (ChkWaterEffectEnable(&em->pos)) {
            EstSet(0, -1, &em->pos, &em->rot, 0x10, 0x31, 0, 0, (u32) em, 0);
        } else {
            EstSet(0, -1, &em->pos, &em->rot, 0x10, 0x17, 0, 0, (u32) em, 0);
        }
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R320Gatling(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 10);
        w->flags |= 0x100;
        em->xFE++;
    case 1:
        em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, PI);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        MotionMoveF(em, 0);
        if (!(pG->flags_64 & 0x2000000) && !em10GotoCk(em)) {
            if (em->type == 6) {
                em->setFindPL();
                w->flags |= 0x40000;
            } else if ((s16) pG->pl_life <= 0 || (pSUB && (s16) pG->sub_life <= 0)) {
                EmRoutineSet(em, 1, 0x1B, 0, 0);
            } else if (!em10FindCk(em, 0)) {
                if (em->plDist2 < 9000000.0f) {
                    em->x38D = 0;
                    em10WalkRtnSet(em);
                } else {
                    em10AtkRtnCk(em, 0);
                }
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_R321DeadBody(cEm10* em)
{
    EmRoutineSet(em, 3, 0, 0, 0);
}

static void em10_R1_R300Gatling(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (!w->evtMot[0] || !w->evtMot[4]) {
            break;
        }
        em->xFE++;
    case 2:
        MotionSetCore(em, MOTION(em), w->evtMot[0], (int) w->evtMot[4], 0, 1, 0);
        em10CallVoiceSe2(em, w->se6D2, 8);
        SndCall(8, 0xB3, &em->pos, em->id, 0, em);
        EstSetEm(em, -1, 0, 0, 1, 0x11, 0, 0, em, 0);
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        } else if (em->seFlags28B & 1) {
            SndCall(6, 9, &em->pos, 0, 0, 0);
        }
        break;
    }
    em10HandSet(em, 0);
}

// Cart route of room 101 (.data 0x6C8): the points the Ganado pushes the cart along.
static Vec em10_r101_cart_route[7] = {
    { -11340.0f, 0.0f, 2630.0f },
    { -4020.0f, 0.0f, -550.0f },
    { -3680.0f, 0.0f, -960.0f },
    { 1100.0f, 0.0f, -1590.0f },
    { 1240.0f, 0.0f, 2700.0f },
    { -8420.0f, 0.0f, 6720.0f },
    { -15600.0f, 0.0f, -6190.0f },
};

static void em10_R1_R101Cart(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x1B8), (int) PL_ARC_PTR(em->subArc, 0x1B9), 3, 5, 0);
        if (w->pHead && w->mot[50]) {
            MotionSetCore(w->pHead, MOTION(w->pHead), w->mot[50], 0, 0, 4, 0);
        }
        w->x4 = 0;
        w->x8 = Rnd() % 15 + 5;
        em->xFE++;
    case 1:
        RouteCkToPos(em, &em10_r101_cart_route[w->x4], &w->x54C, 0, 0);
        w->x518 = Muku(&em->pos, &w->x54C, em->rot.y, PI);
        w->x51C = fabsf(w->x518);
        {
            Vec* rp = &em10_r101_cart_route[w->x4];
            f32 dx = em->pos.x - rp->x;
            f32 dz = em->pos.z - rp->z;
            w->x520 = dx * dx + dz * dz;
        }
        em->rot.y += Muku(&em->pos, &w->x54C, em->rot.y, 0.012271847f);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (w->x520 < 1000000.0f) {
            w->x4++;
            if (w->x4 > 6) {
                w->x4 = 0;
            }
        }
        MotionMoveF(em, 0);
        break;
    }
    if (!em10FindCk(em, 0)) {
        if (w->flags & 0x100) {
            em10WalkRtnSet(em);
        } else {
            em10HandSet(em, 0);
            if (w->pHead) {
                v.x = 0.0f;
                v.y = 0.0f;
                v.z = 1700.0f;
                PSMTXMultVec(em->mat, &v, &w->pHead->pos);
                w->pHead->rot = em->rot;
            }
            if (w->x8) {
                w->x8--;
            } else {
                w->x8 = Rnd() % 30 + 15;
                EstSetEm(w->pHead, -1, 0, 0, 0x10, 0x16, 0, 0, w->pHead, 0);
            }
        }
    }
}

static void em10_R1_br_EvtDash(cEm10* em)
{
    if (!em10DoorOpenCk(em, 0) && !em10RackBreakCk(em) && !em10LadderClimbCk(em) && !em10VLadderClimbCk(em) && !em10LadderResetCk(em) && !em10JumpDownCk(em) && !em10JumpCk(em) && !em10ClimbOverCk(em)) {
        em10WindowCk(em);
    }
}

static void em10_R1_EvtDash(cEm10* em)
{
}

static void em10_R1_br_EvtWalk(cEm10* em)
{
    if (!em10DoorOpenCk(em, 0) && !em10RackBreakCk(em) && !em10LadderClimbCk(em) && !em10VLadderClimbCk(em) && !em10LadderResetCk(em) && !em10JumpDownCk(em) && !em10JumpCk(em) && !em10ClimbOverCk(em)) {
        em10WindowCk(em);
    }
}

static void em10_R1_EvtWalk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;

    switch (em->xFE) {
    case 0:
        em->xFF = Rnd() % 256;
        em10SetWalkMotion(em, 7);
        if (w->wepType == 4 && (s32) w->flags < 0) {
            if (em->type == 0x16) {
                EstSetEm(w->pWep, -1, 0, 0, 0x10, 0x96, 0, w->x69C, w->pWep, 0);
            } else {
                EstSetEm(w->pWep, -1, 0, 0, 0x10, 9, 0, w->x69C, w->pWep, 0);
            }
            w->flags |= 0x80000000;
            w->x65C = Rnd() % 150 + 150;
            SndCall(6, 0x4C, &em->pos, 0, 0, em);
            w->x684 = 60;
        }
        w->x4 = 120;
        em->xFF = 0;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (w->x4) {
            w->x4--;
        } else {
            em->xFC = 1;
            em->xFD = 0x10;
            em->xFE = 0;
            em->xFF = (u8) (MOTION(em)->seqFrame * 255.0f / (f32) MOTION(em)->seqMax);
            w->x6AC = Rnd() % 11;
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_Pickup(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    if ((w->flags & 0x100) && (em->flags_3C8 & 0x2000)) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x76), (int) PL_ARC_PTR(em->subArc, 0x77), 10, flag, 0);
        em->xFE++;
    case 1:
        if ((em->seFlags28B & 4) && w->pWep2) {
            if (em->flags_3C8 & 0x10000) {
                w->pWep = em10MakeWeapon(em, w->wep2Type);
                if (w->pWep) {
                    w->wepType = w->wep2Type;
                } else {
                    w->wepType = 0;
                }
            } else {
                w->pWep2->setTransMode(1);
                w->pWep = w->pWep2;
                w->wepType = w->wep2Type;
                w->wep2Type = 0;
                w->pWep2 = 0;
            }
            em10WeaponSet(em);
        }
        if (MotionMoveF(em, 0)) {
            cEmWep* wep = w->pWep;
            u8 type = w->wepType;
            if (wep && type == 4 && !(w->flags & 0x80000000)) {
                EmRoutineSet(em, 1, 0xE, 0, 0);
                break;
            }
            if (wep && type == 9 && w->x640 == 0 && (w->x524 < 15000.0f || em->x3D0 == 2) && (w->flags & 1)) {
                if (em->flags_3C8 & 0x10000) {
                    EmRoutineSet(em, 1, 0xF, 0, 0);
                    break;
                }
                if (em->pos.y > pPL->pos.y - 500.0f) {
                    EmRoutineSet(em, 1, 0xF, 0, 0);
                    break;
                }
            }
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Find(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void* m0;
    void* m1;
    int flag;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        m0 = PL_ARC_PTR(em->subArc, 0x73);
        m1 = PL_ARC_PTR(em->subArc, 0x74);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        w->x4 = 30;
        em->xFE++;
    case 1:
        em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.19634955f);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (em->seFlags28B & 0x20) {
            em10CallVoiceSe2(em, w->se6CA, 8);
        }
        if (em->seFlags28B & 8) {
            w->flags |= 0x2000000;
        }
        if (w->x4) {
            w->x4--;
            if (w->x4 == 0) {
                em10FindNotify(em);
            }
        }
        if (MotionMoveF(em, 0)) {
            if (em->flags_3C8 & 0x2000000) {
                EmRoutineSet(em, 1, 0x11, 0, 0);
            } else {
                em10WalkRtnSet(em);
            }
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 2);
    em10GotoCk(em);
}

static void em10_R1_C_SawStart(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        if (em->flags_3C8 & 0x1000000) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xE7), (int) PL_ARC_PTR(em->subArc, 0xE8), 10, 0x41, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xE7), (int) PL_ARC_PTR(em->subArc, 0xE8), 10, 1, 0);
        }
        EstSetEm(em, -1, 0, 0, 0x10, 0x2B, 0, 0, em, 0);
        w->x4 = 30;
        em->xFE++;
    case 1:
        if (em->seFlags28B & 4) {
            if (em->type == 0x16) {
                EstSetEm(w->pWep, -1, 0, 0, 0x10, 0x96, 0, w->x69C, w->pWep, 0);
            } else {
                EstSetEm(w->pWep, -1, 0, 0, 0x10, 9, 0, w->x69C, w->pWep, 0);
            }
            w->flags |= 0x80000000;
            w->x65C = Rnd() % 150 + 150;
            SndCall(6, 0x4C, &em->pos, 0, 0, em);
            w->x684 = 60;
            em10FindNotify(em);
        }
        if (MotionMoveF(em, 0)) {
            if (em->flags_3C8 & 0x2000000) {
                EmRoutineSet(em, 1, 0x11, 0, 0);
            } else {
                em10WalkRtnSet(em);
            }
        } else if ((em->seFlags28B & 1) && w->x51C > 1.9634955f) {
            EmRoutineSet(em, 1, 0x15, 0, 0);
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_BombIgnition(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec ofs;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        if (em->flags_3C8 & 0x1000000) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x7E), (int) PL_ARC_PTR(em->subArc, 0x7F), 10, 0x41, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x7E), (int) PL_ARC_PTR(em->subArc, 0x7F), 10, 1, 0);
        }
        w->x4 = 30;
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            EmRoutineSet(em, 1, 0x11, 0, 0);
        } else if (em->seFlags28B & 1) {
            w->x640 = 9999;
            w->pWep->setEffAlways(0x10, 0x2D);
            ofs.x = 0.0f;
            ofs.y = 40.0f;
            ofs.z = 60.0f;
            w->pWep->setEffAlways2(0x10, 0x2F, 0, &ofs, 3);
            SndCall(8, 0x94, &em->pos, em->id, 0, em);
            em10ThrowBombCk(em);
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_br_Walk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0) {
        if (pG->flags_64 & 0x2000000) {
            EmRoutineSet(em, 1, 0, 0, 0);
        } else if (!em10GotoCk(em) && !em10DoorOpenCk(em, 0) && !em10RackBreakCk(em) && !em10LadderClimbCk(em) && !em10VLadderClimbCk(em) && !em10LadderResetCk(em) && !em10JumpDownCk(em) && !em10JumpCk(em)) {
            em10ReturnStartPosCk(em);
            if (!em10ClimbOverCk(em) && !em10WindowCk(em)) {
                if ((em->flags_3C8 & 0x400) && w->x5EC == 0 && w->x634 > 30 && fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, PI)) < 0.5235988f) {
                    EmRoutineSet(em, 1, 0x1C, 0, 0);
                } else if (!em10IgnitionCk(em) && !em10ClawStickCK(em) && !em10FindLostCk(em)) {
                    if (w->flags & 0x20000000) {
                        f32 dx = em->pos.x - w->x4D8.x;
                        f32 dz = em->pos.z - w->x4D8.z;
                        if (dx * dx + dz * dz < 4000000.0f) {
                            w->flags &= ~0x20000000;
                            EmRoutineSet(em, 1, 1, 0, 0);
                            return;
                        }
                        if (em->plDist2 < 4000000.0f && w->x530 < em->x3CC) {
                            w->flags &= ~0x20000000;
                        }
                    }
                    em10GotoPosCk(em);
                }
            }
        }
    }
}

static void em10_R1_Walk(cEm10* em)
{
}

static void em10_R1_br_Dash(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0) {
        if (pG->flags_64 & 0x2000000) {
            EmRoutineSet(em, 1, 0, 0, 0);
        } else if (!em10GotoCk(em) && !em10DoorOpenCk(em, 0) && !em10RackBreakCk(em) && !em10LadderClimbCk(em) && !em10VLadderClimbCk(em) && !em10LadderResetCk(em) && !em10JumpDownCk(em) && !em10JumpCk(em)) {
            em10ReturnStartPosCk(em);
            if (!em10ClimbOverCk(em) && !em10WindowCk(em) && !em10IgnitionCk(em) && !em10ClawStickCK(em) && !em10FindLostCk(em)) {
                if (w->flags & 0x20000000) {
                    f32 dx = em->pos.x - w->x4D8.x;
                    f32 dz = em->pos.z - w->x4D8.z;
                    if (dx * dx + dz * dz < 4000000.0f) {
                        w->flags &= ~0x20000000;
                        EmRoutineSet(em, 1, 1, 0, 0);
                        return;
                    }
                    if (em->plDist2 < 4000000.0f && w->x530 < em->x3CC) {
                        w->flags &= ~0x20000000;
                    }
                }
                em10GotoPosCk(em);
            }
        }
    }
}

static void em10_R1_Dash(cEm10* em)
{
}

static void em10_R1_br_Back(cEm10* em)
{
    if (em->hp > 0) {
        em10GotoCk(em);
    }
}

static void em10_R1_Back(cEm10* em)
{
}

static void em10_R1_br_Goto(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0 && !em10DoorOpenCk(em, 1) && !em10RackBreakCk(em) && !em10LadderClimbCk(em) && !em10VLadderClimbCk(em) && !em10LadderResetCk(em) && !em10JumpDownCk(em) && !em10JumpCk(em)) {
        em10ReturnStartPosCk(em);
        if (!em10ClimbOverCk(em) && !em10WindowCk(em) && em->xFE == 0 && w->x5EC != 8 && w->x51C > 1.9198622f) {
            EmRoutineSet(em, 1, 0x15, 0, 2);
        }
    }
}

static void em10_R1_Goto(cEm10* em)
{
}

static void em10_R1_GuardWalk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    PlArc* arc;
    int end;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    em->hitInfo.flags &= ~1;
    switch (em->xFE) {
    case 0:
        arc = em->subArc;
        if (em->xFF) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(arc, 6), (int) PL_ARC_PTR(arc, 7), 30, 5, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(arc, 6), (int) PL_ARC_PTR(arc, 7), 30, 0x45, 0);
        }
        em->xFF = Rnd() & 1;
        w->x4 = Rnd() % 120 + 120;
        em->xFE++;
    case 1:
        if ((em->flags_3C8 & 0x400) && w->x5EC == 0 && em->pos.y > pPL->pos.y + 500.0f) {
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
        } else {
            em->rot.y += Muku(&em->pos, &w->x54C, em->rot.y, 0.09817477f);
        }
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        end = MotionMoveF(em, 0);
        if ((w->flags & 0x80) && w->x6C1 == 0 && em->hp == 1) {
            em->hp = 0;
        }
        if (em->hp <= 0) {
            if (end) {
                EmRoutineSet(em, 2, 9, 0, 0);
            }
        } else if ((s16) pG->pl_life <= 0 || (pSUB && (s16) pG->sub_life <= 0)) {
            EmRoutineSet(em, 1, 0x1B, 0, 0);
        } else if (!em10AtkRtnCk(em, 0)) {
            if (w->x4) {
                w->x4--;
            } else {
                u8 f = (u8) (MOTION(em)->seqFrame * 255.0f / (f32) MOTION(em)->seqMax);
                w->x6AC = Rnd() % 11;
                em->xFC = 1;
                em->xFD = 0x10;
                em->xFE = 0;
                em->xFF = f;
            }
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_Turn180(cEm10* em)
{
}

static void em10_R1_Threat(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        if (Muku(&pPL->pos, &em->pos, pPL->rot.y, PI) > 0.0f) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x71), (int) PL_ARC_PTR(em->subArc, 0x72), 30, 1, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x71), (int) PL_ARC_PTR(em->subArc, 0x72), 30, 0x41, 0);
        }
        w->x4 = 10;
        em->xFE++;
    case 1:
        if (w->x4) {
            w->x4--;
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        } else {
            em10AtkRtnCk(em, 0);
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_SideStep(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        switch (em->xFF) {
        case 0:
        default:
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x2AC), (int) PL_ARC_PTR(em->subArc, 0x2AD), 5, 1, 0);
            break;
        case 1:
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x2AC), (int) PL_ARC_PTR(em->subArc, 0x2AD), 5, 0x41, 0);
            break;
        case 2:
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x98), (int) PL_ARC_PTR(em->subArc, 0x99), 5, 1, 0);
            break;
        case 3:
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x98), (int) PL_ARC_PTR(em->subArc, 0x99), 5, 0x41, 0);
            break;
        }
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            w->x696 = 1;
            if (!em10AtkRtnCk(em, 0) && !em10HideRtnCk2(em)) {
                em10WalkRtnSet(em);
            }
        } else if (em->seFlags28B & 1) {
            w->x696 = 1;
            em10AtkRtnCk(em, 0);
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_HideSide(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 10);
        w->x4 = Rnd() % 150 + 90;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->flags_3C8 &= ~1;
            em->setFindPL();
            em->xFC = 1;
            em->xFD = 0x17;
            em->xFE = 0;
        } else {
            if (w->x4 == 0) {
                if (em10HideToStepCk(em, em->xFF)) {
                    if (!(w->flags & 0x100)) {
                        em->setFindPL();
                        em10CallVoiceSe2(em, w->se6CA, 8);
                        if (w->x4) {
                            w->x4--;
                            if (w->x4 == 0) {
                                em10FindNotify(em);
                            }
                        }
                    }
                    break;
                }
            } else {
                w->x4--;
            }
            if (w->x524 < 4000.0f) {
                em10WalkRtnSet(em);
            }
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_AppearSide(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 10);
        w->x4 = Rnd() % 150 + 90;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->setFindPL();
            em->xFC = 1;
            em->xFD = 0x17;
            em->xFE = 0;
        } else if (w->x52C < em->x3CC) {
            em->setFindPL();
            em10WalkRtnSet(em);
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
}

static void em10_R1_SitDown(cEm10* em)
{
}

static void em10_R1_Stay(cEm10* em)
{
}

static void em10_R1_RoofWait(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 10);
        w->x4 = 10;
        w->x8 = 0x23;
        em->xFE++;
    case 1:
        if (w->x4) {
            w->x4--;
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        MotionMoveF(em, 0);
        if (fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, PI)) > 0.7853982f) {
            if ((s16) pG->pl_life > 0 && (!pSUB || (s16) pG->sub_life > 0)) {
                em10WalkRtnSet(em);
            }
        } else {
            em10AtkRtnCk(em, 0);
        }
        break;
    }
    em10BreathSe(em);
    em10HandSet(em, 0);
    if ((w->flags & 0x80) && w->x6C1 == 0 && em->hp == 1) {
        em->hp = 0;
    }
    if (em->hp <= 0) {
        EmRoutineSet(em, 2, 9, 0, 0);
    }
}

static void em10_R1_Guard(cEm10* em)
{
}

static void em10_R1_DownWakeWait(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        w->x67A = Rnd() % 60 + 60;
        em->xFE++;
    case 1:
        w->flags |= 0x10;
        w->flags |= 0x1000000;
        em->setStatus(3);
        MotionMoveF(em, 0);
        if (em->plDist2 > 6250000.0f && w->pParasite && w->x6C5 != 1 && w->pParasite->ckAtkEnable()) {
            w->pParasite->setAtk(1);
        }
        if ((s16) w->x67A != 0) {
            w->x67A--;
        } else {
            EmRoutineSet(em, 1, 0x1F, 0, 0);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_DownWake(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        flag = (MOTION(em)->flags & 0x40) ? 0x41 : 1;
        if (w->flags & 0x20) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x6D), (int) PL_ARC_PTR(em->subArc, 0x6E), 15, flag, 0);
        } else if (em->type == 6) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xAA), (int) PL_ARC_PTR(em->subArc, 0xAD), 15, flag, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x6F), (int) PL_ARC_PTR(em->subArc, 0x70), 15, flag, 0);
        }
        if (!(w->flags & 0x80)) {
            em10CallVoiceSe2(em, w->se6D1, 8);
        }
        em->xFE++;
    case 1:
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x18;
            w->flags |= 0x1000000;
            em->setStatus(3);
        } else {
            w->flags &= ~0x10;
            w->flags &= ~0x1000000;
        }
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        } else {
            if (em->plDist2 > 6250000.0f && w->pParasite && w->x6C5 != 1 && w->pParasite->ckAtkEnable()) {
                w->pParasite->setAtk(1);
            }
            if (em->seFlags28B & 2) {
                em10SetDmWaterEff(em, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Crash(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void* m1;
    void* m0;
    int flag;

    switch (em->xFE) {
    case 0:
        flag = (Rnd() & 1) ? 0x41 : 1;
        if (em->xFF) {
            m1 = PL_ARC_PTR(em->subArc, 0x1E);
            m0 = PL_ARC_PTR(em->subArc, 0x1D);
        } else {
            m1 = PL_ARC_PTR(em->subArc, 0x1C);
            m0 = PL_ARC_PTR(em->subArc, 0x1B);
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 6, flag, Rnd() % 5);
        em->xFE++;
    case 1:
        w->flags |= 0x2000;
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
    w->flags |= 0x2000;
    em10SetCrash(em, 500.0f);
}

static void em10_R1_ClimbOver(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx m;
    Vec v;

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x1F), (int) PL_ARC_PTR(em->subArc, 0x20), 10, 1, 0);
        PSVECSubtract(&w->x5E0, &em->pos, &w->x5E0);
        PSMTXRotRad(m, 'y', em->rot.y);
        PSMTXInverse(m, m);
        PSMTXMultVecSR(m, &w->x5E0, &w->x5E0);
        w->x5E0.y = 0.0f;
        em->xFE++;
    case 1:
        PSVECScale(&w->x5E0, &v, 0.2f);
        PSVECSubtract(&w->x5E0, &v, &w->x5E0);
        PSMTXRotRad(m, 'y', em->rot.y);
        PSMTXMultVecSR(m, &v, &v);
        PSVECAdd(&em->pos, &v, &em->pos);
        if (em->seFlags28B & 4) {
            w->flags |= 0x20000;
            em->setStatus(3);
        }
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
            w->x6B9 = 0;
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_DoorAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void* m0;
    void* m1;
    int flag;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        m0 = PL_ARC_PTR(em->subArc, 0x79);
        m1 = PL_ARC_PTR(em->subArc, 0x7A);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        if (w->pWep && w->wepType == 4) {
            m0 = PL_ARC_PTR(em->subArc, 0xE9);
            m1 = PL_ARC_PTR(em->subArc, 0xEA);
            SndCall(6, 0x50, &em->pos, 0, 0, em);
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        em10CallVoiceSe2(em, w->se6D2, 8);
        w->x697 = 0;
        em->xFE++;
    case 1:
        em->atari.flags |= 8;
        w->x6BC = 2;
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        } else if ((em->seFlags28B & 0x20) || (em->seFlags28B & 1)) {
            if (em->seFlags28B & 1) {
                if (w->wepType == 4) {
                    em10SetDamageDoor(em, 2);
                } else {
                    if (em10SetDamageDoor(em, 1) == 1) {
                        em10SetDamageRack(em, 0);
                        break;
                    }
                }
                em10SetDamageRack(em, 2);
            } else {
                em10SetDamageDoor(em, 0);
                em10SetDamageRack(em, 0);
            }
        } else if (em->seFlags28B & 4) {
            int ok = em10AtkDoorCk(em) == 0;
            if (em10AtkRackCk(em)) {
                ok = 0;
            }
            if (ok) {
                em->xFE++;
            }
        }
        break;
    case 2:
        m0 = PL_ARC_PTR(em->subArc, 0x79);
        m1 = PL_ARC_PTR(em->subArc, 0x7B);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_RackAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void* m0;
    void* m1;
    int flag;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        m0 = PL_ARC_PTR(em->subArc, 0x7C);
        m1 = PL_ARC_PTR(em->subArc, 0x7D);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        em10CallVoiceSe2(em, w->se6D2, 8);
        if (w->pWep && w->wepType == 4) {
            m0 = PL_ARC_PTR(em->subArc, 0xE9);
            m1 = PL_ARC_PTR(em->subArc, 0xEA);
            SndCall(6, 0x50, &em->pos, 0, 0, em);
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        w->x697 = 0;
        em->xFE++;
    case 1:
        em->atari.flags |= 8;
        w->x6BC = 2;
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        } else if (em->seFlags28B & 1) {
            em10SetDamageDoor(em, 2);
            em10SetDamageRack(em, 2);
        }
        break;
    case 2:
        m0 = PL_ARC_PTR(em->subArc, 0x79);
        m1 = PL_ARC_PTR(em->subArc, 0x7B);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_WindowAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    void* m0;
    void* m1;
    int flag;
    int ok;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        m0 = PL_ARC_PTR(em->subArc, 0x79);
        m1 = PL_ARC_PTR(em->subArc, 0x7A);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        if (w->pWep && w->wepType == 4) {
            m0 = PL_ARC_PTR(em->subArc, 0xE9);
            m1 = PL_ARC_PTR(em->subArc, 0xEA);
            SndCall(6, 0x50, &em->pos, 0, 0, em);
        }
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        em10CallVoiceSe2(em, w->se6D2, 8);
        w->x697 = 0;
        em->xFE++;
    case 1:
        em->atari.flags |= 8;
        w->x6BC = 2;
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        } else if ((em->seFlags28B & 0x20) || (em->seFlags28B & 1)) {
            if (em->seFlags28B & 1) {
                ok = 0;
                if (EM10_WINDOW(w) && EM10_WINDOW(w)->hp > 0) {
                    if (EM10_WINDOW(w)->hp <= 1 || w->wepType == 4) {
                        EM10_WINDOW(w)->SetBreakAll(&em->pos, 0, 0);
                        w->pWindow = 0;
                        ok = 1;
                    } else {
                        EM10_WINDOW(w)->SetShake();
                    }
                }
                if (w->wepType == 4 || ok) {
                    em10SetDamageDoor(em, 2);
                    em10SetDamageRack(em, 2);
                    break;
                }
            } else {
                if (EM10_WINDOW(w) && EM10_WINDOW(w)->hp > 0) {
                    EM10_WINDOW(w)->SetShake();
                }
                em10SetDamageDoor(em, 0);
            }
            em10SetDamageRack(em, 0);
        } else if (em->seFlags28B & 4) {
            ok = 1;
            if (EM10_WINDOW(w) && EM10_WINDOW(w)->hp > 0) {
                ok = 0;
            }
            if (em10AtkDoorCk(em)) {
                ok = 0;
            }
            if (em10AtkRackCk(em)) {
                ok = 0;
            }
            if (ok) {
                em->xFE++;
            }
        }
        break;
    case 2:
        m0 = PL_ARC_PTR(em->subArc, 0x79);
        m1 = PL_ARC_PTR(em->subArc, 0x7B);
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), m0, (int) m1, 10, flag, 0);
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_LadderClimb(cEm10* em)
{
}

static void em10_R1_VLadderClimb(cEm10* em)
{
}

static void em10_R1_LadderReset(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    Mtx m;
    Vec ofs;
    int flag;

    switch (em->xFE) {
    case 0:
        PSMTXRotRad(m, 'y', w->pLadder->rot.y);
        TransMatrix(m, &w->pLadder->pos);
        if (Muku2(w->pLadder->rot.y, em->rot.y, PI) > 0.0f) {
            ofs.x = -615.27f;
            ofs.y = 0.0f;
            ofs.z = 1432.8f;
            flag = 1;
            w->x5DC = w->pLadder->rot.y + 1.5707964f;
        } else {
            ofs.x = 615.27f;
            ofs.y = 0.0f;
            ofs.z = 1432.8f;
            flag = 0x41;
            w->x5DC = w->pLadder->rot.y - 1.5707964f;
        }
        w->x5DC = Muku2(em->rot.y, w->x5DC, PI);
        PSMTXMultVec(m, &ofs, &ofs);
        PSVECSubtract(&ofs, &em->pos, &w->x5E0);
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xBA), 0, 10, flag, 0);
        w->x4 = 0x1F;
        w->x8 = 0x71;
        em->xFE++;
    case 1:
        PSVECScale(&w->x5E0, &v, 0.2f);
        PSVECAdd(&em->pos, &v, &em->pos);
        PSVECSubtract(&w->x5E0, &v, &w->x5E0);
        {
            f32 d = w->x5DC * 0.2f;
            em->rot.y += d;
            w->x5DC -= d;
        }
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (w->x4) {
            w->x4--;
            if (w->x4 == 0) {
                w->pLadder->setReset(1);
            }
        }
        if (w->x8) {
            w->x8--;
            if (w->x4 == 0) {
                w->flags |= 0x200000;
            }
        } else if (w->pLadder) {
            w->pLadder = 0;
        }
        if (MotionMoveF(em, 0)) {
            w->x67C = 15;
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_JumpDown(cEm10* em)
{
}

static void em10_R1_Jump(cEm10* em)
{
}

static void em10_R1_JumpUp(cEm10* em)
{
}

static void em10_R1_Trade(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xA5), 0, 10, 1, 0);
        KeyStop(0xEFCF0000);
        w->x5B8 = SndCall(8, 0x84, &em->pos, em->id, 0, em);
        em->xFE++;
    case 1:
        em->dmg.x1 = 2;
        if (MOTION(em)->seqFrame > 35.7f && MOTION(em)->seqFrame < 36.3f) {
            em10ClothPartsSet(em, 1);
            SndCall(8, 0x86, &em->pos, em->id, 0, em);
        }
        if (MOTION(em)->seqFrame > 40.7f && MOTION(em)->seqFrame < 41.3f) {
            em10GoodsPartsSet(em, 1);
        }
        em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        if (MotionMoveF(em, 0)) {
            em->xFE++;
        }
        break;
    case 2:
        if (SubScreenOpen(0x10, 0)) {
            em->xFE++;
        }
        break;
    case 3:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xA7), 0, 10, 1, 0);
        SndCall(8, 0x86, &em->pos, em->id, 0, em);
        w->x5B8 = SndCall(8, 0x9A, &em->pos, em->id, 0, em);
        pG->flags_170 &= 0x7FFFFFFF;
        em->xFE++;
    case 4:
        if (MOTION(em)->seqFrame > 33.7f && MOTION(em)->seqFrame < 34.3f) {
            em10ClothPartsSet(em, 0);
        }
        if (MOTION(em)->seqFrame > 26.7f && MOTION(em)->seqFrame < 27.3f) {
            em10GoodsPartsSet(em, 0);
        }
        if (MotionMoveF(em, 0)) {
            EmRoutineSet(em, 1, 0, 0, 0);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Drive(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cModel* truck;

    em->atari.flags &= 0xFCFF;
    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x1BA), 0, 0, 5, 0);
        w->pTruck = 0;
        w->scaleBase.x = 1.0f;
        w->scaleBase.y = 1.0f;
        w->scaleBase.z = 1.0f;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em10SearchTruck(em)) {
            em->xFE++;
        }
        break;
    case 2:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x1BA), 0, 0, 5, 0);
        em->xFE++;
    case 3:
        em->pos.x = 0.0f;
        em->pos.y = 0.0f;
        em->pos.z = 0.0f;
        em->rot.x = 0.0f;
        em->rot.y = 0.0f;
        em->rot.z = 0.0f;
        RotMatrix(em->mat, &em->rot);
        TransMatrix(em->mat, &em->pos);
        ScaleMatrix(em->mat, &em->scale);
        PSMTXConcat(w->pTruck->getPartsPtr(0)->mat, em->mat, em->mat);
        MOTION(em)->flags2 |= 0x40000000;
        MotionMoveF(em, 0);
        if (em->hp <= 0 || (w->pTruck->flags_3C8 & 2)) {
            em->xFE++;
        }
        break;
    case 4:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x1BB), 0, 3, 1, 0);
        em->hp = 0;
        em->clearStatus(5);
        em->xFE++;
    case 5:
        em->pos.y = -100.0f;
        em->pos.x = 0.0f;
        em->pos.z = 0.0f;
        em->rot.x = 0.0f;
        em->rot.y = 0.0f;
        em->rot.z = 0.0f;
        truck = w->pTruck->getPartsPtr(0);
        RotMatrix(em->mat, &em->rot);
        TransMatrix(em->mat, &em->pos);
        ScaleMatrix(em->mat, &em->scale);
        PSMTXConcat(truck->mat, em->mat, em->mat);
        MOTION(em)->flags2 |= 0x40000000;
        MotionMoveF(em, 0);
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Catapult(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 n;
    u32 r;

    w->flags |= 0x8000;
    switch (em->xFE) {
    case 0:
        n = *(u16*) PL_ARC_PTR(em->subArc, 5) & 0x3FFF;
        r = Rnd() % n;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 5), 0, 10, 5, (u16) r);
        em->hp = 1;
        em->xFE++;
    case 1:
        w->flags |= 0x40000;
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->flags_3C8 &= ~1;
            em->xFE++;
        }
        break;
    case 2:
        em->xFE++;
    case 3:
        if (MotionMoveF(em, 0)) {
            em->xFE = 0;
        }
        break;
    }
    if (!em10GotoCk(em)) {
        em10HandSet(em, 0);
    }
}

static void em10_R1_RockPush(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    w->flags |= 0x8000;
    em->atari.throughOn();
    em->dmg.x1 = 2;
    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 5), 0, 0, 5, 0);
        em->be_flag &= ~2;
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        if (em->flags_3C8 & 1) {
            em->flags_3C8 &= ~1;
            em->be_flag |= 2;
            em->xFE++;
        }
        break;
    case 2:
        w->x4 = 150;
        em->xFE++;
    case 3:
        if (w->x4) {
            w->x4--;
        } else {
            em->alpha = 0.0f;
            em->be_flag &= ~2;
            w->flags |= 0x400000;
        }
        if (MotionMoveF(em, 0)) {
            em->hp = 0;
            em->alpha = 0.0f;
            em->be_flag &= ~2;
            w->flags |= 0x400000;
            em->xFE++;
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_ParasiteAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int r;
    int flag;

    switch (em->xFE) {
    case 0:
        flag = 0;
        if (w->x58C) {
            f32 d;
            flag = 0x1E;
            if (w->x58C->vC8() && pSUB) {
                d = w->x514;
            } else {
                d = em->plDist2;
            }
            if (d < 2560000.0f) {
                w->x58C->v68();
            } else {
                w->x58C->v78();
                r = Rnd();
                w->x648 = r % 300 + 300;
            }
        }
        if (w->pParasite) {
            if (w->x6C5 != 1) {
                flag = 0x1E;
                if (w->flags & 0x8000000) {
                    w->pParasite->setAtk(1);
                } else {
                    w->pParasite->setAtk(0);
                }
            } else {
                w->pParasite->setCritical();
            }
        }
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x28E), 0, 10, 1, flag);
        w->x697 = 0;
        w->x4 = 120;
        em->setWeaponFall();
        if (w->pShield) {
            w->pShield->setFall(20.0f, 0);
            w->pShield = 0;
        }
        em->xFE++;
    case 1:
        if (w->pParasite && w->pParasite->ckAtkHit()) {
            w->x697 = 1;
        }
        if (w->x58C && w->x58C->v70()) {
            w->x697 = 1;
        }
        if (MotionMoveF(em, 0)) {
            if (w->x697) {
                w->x67C = 15;
                if (pG->x4F88 <= 3) {
                    w->x67C = 45;
                }
                if (pG->x4F88 <= 1) {
                    w->x67C = 90;
                }
            }
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_ShotBowgun(cEm10* em)
{
}

static void em10_R1_ShotRocket(cEm10* em)
{
}

static void em10_R1_ShotGatling(cEm10* em)
{
}

static void em10_R1_ThrowAxe(cEm10* em)
{
}

static void em10_R1_ThrowBomb(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        if (em->flags_3C8 & 0x1000000) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x82), (int) PL_ARC_PTR(em->subArc, 0x83), 10, 0x41, 0x10);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x82), (int) PL_ARC_PTR(em->subArc, 0x83), 10, 1, 0x10);
        }
        w->x4 = 10;
        em10CallVoiceSe2(em, w->se6D2, 8);
        if (w->x640 > 90) {
            w->x640 = 90;
        }
        em->xFE++;
    case 1:
        if (w->x4) {
            w->x4 -= 1;
            em->rot.y = em->rot.y + Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (em->seFlags28B & 4) {
            w->x670 = 2;
        }
        if ((em->seFlags28B & 1) && w->pWep) {
            em10BombThrow(em);
        }
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_FixBomber(cEm10* em)
{
}

static void em10_R1_R305Bomber(cEm10* em)
{
}

static void em10_R1_R408Bomber(cEm10* em)
{
}

static void em10_R1_RocketWait(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 10);
        w->x18 = em->rot.y;
        em->xFE++;
    case 1:
        em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
        em->rot.y = LIMIT_ANGLE(em->rot.y);
        MotionMoveF(em, 0);
        if (!em10GotoCk(em) && (em->flags_3C8 & 1)) {
            em->x38D = 0;
            EmRoutineSet(em, 1, 0x22, 0, 0);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_AxeAtk(cEm10* em)
{
}

static void em10_R1_ShieldAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    cModel* parts;
    int flag;

    switch (em->xFE) {
    case 0:
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x16E), (int) PL_ARC_PTR(em->subArc, 0x16F), 10, flag, 0);
        em10CallVoiceSe2(em, w->se6D2, 8);
        w->x4 = 20;
        if (pG->x4F88 <= 3) {
            w->x4 = 5;
        }
        if (pG->x4F88 > 6) {
            w->x4 = 30;
        }
        w->x697 = 0;
        em->xFE++;
    case 1:
        if ((w->x4 && --w->x4) || (em->seFlags28B & 8)) {
            f32 ang = (em->seFlags28B & 8) ? 0.049087387f : 0.09817477f;
            if ((w->flags & 0x8000000) && pSUB) {
                em->rot.y += Muku(&em->pos, &pSUB->pos, em->rot.y, ang);
            } else {
                em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, ang);
            }
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (em->seFlags28B & 0x20) {
            if (w->wepType == 7) {
                SndCall(8, 0x42, &em->pos, em->id, 0, em);
            } else {
                SndCall(8, 0x3D, &em->pos, em->id, 0, em);
            }
        }
        if (em->seFlags28B & 1) {
            if (em->flags_3C8 & 0x1000000) {
                parts = em->getPartsPtr(10);
            } else {
                parts = em->getPartsPtr(16);
            }
            v = parts->worldPos;
            em10AtkCk(em, &v, &parts->x88, 4, 0);
            v.y -= 500.0f;
            em10AtkCk(em, &v, &parts->x88, 4, 0);
        }
        if (MotionMoveF(em, 0)) {
            if (w->x697 == 0) {
                GameAddPoint(0xB);
            }
            if (w->x697) {
                w->x67C = 15;
                if (pG->x4F88 <= 3) {
                    w->x67C = 45;
                }
                if (pG->x4F88 <= 1) {
                    w->x67C = 90;
                }
            }
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_TorchFrame(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        w->x8 = 0;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x84), (int) PL_ARC_PTR(em->subArc, 0x85), 10, flag, 0);
        w->x682 = 60;
        w->x8 = 1;
        SndCall(8, 0x8C, &em->pos, em->id, 0, em);
        w->x4 = 10;
        w->x697 = 0;
        w->x698 = 0;
        em->xFE++;
    case 1:
        if (w->x4) {
            f32 a;
            w->x4--;
            if ((w->flags & 0x8000000) && pSUB) {
                a = Muku(&em->pos, &pSUB->pos, em->rot.y, 0.09817477f);
            } else {
                a = Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
            }
            em->rot.y += a;
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if ((em->seFlags28B & 1) && w->pWep) {
            em10TorchFrameAtkCk(em);
            em10TorchFrameAtkCkSub(em);
        }
        if (em->seFlags28B & 2) {
            EstSetEm(em, -1, 0, 0, 0x10, 0x23, 0, w->x69E, em, 0);
        }
        if (em->seFlags28B & 0x20) {
            w->x5C0 = SndCall(8, 0x8D, &em->pos, em->id, 0, em);
        }
        if (MotionMoveF(em, 0)) {
            if (w->x697 == 0) {
                GameAddPoint(0xB);
            }
            if (w->x697) {
                w->x67C = 15;
                if (pG->x4F88 <= 3) {
                    w->x67C = 45;
                }
                if (pG->x4F88 <= 1) {
                    w->x67C = 90;
                }
            }
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_SukiAtk(cEm10* em)
{
}

static void em10_R1_ScytheAtk(cEm10* em)
{
}

static void em10_R1_ClawAtk(cEm10* em)
{
}

static void em10_R1_br_CSawWalkAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0) {
        if (pG->flags_64 & 0x2000000) {
            EmRoutineSet(em, 1, 0, 0, 0);
        } else if (!em10GotoCk(em) && !em10DoorOpenCk(em, 0) && !em10RackBreakCk(em) && !em10LadderClimbCk(em) && !em10VLadderClimbCk(em) && !em10LadderResetCk(em) && !em10JumpDownCk(em) && !em10JumpCk(em)) {
            em10ReturnStartPosCk(em);
            if (!em10ClimbOverCk(em) && !em10WindowCk(em)) {
                if ((em->flags_3C8 & 0x400) && w->x5EC == 0 && w->x634 > 30 && fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, PI)) < 0.5235988f) {
                    EmRoutineSet(em, 1, 0x1C, 0, 0);
                } else if (!em10IgnitionCk(em) && !em10ClawStickCK(em) && !em10FindLostCk(em)) {
                    if (w->flags & 0x20000000) {
                        f32 dx = em->pos.x - w->x4D8.x;
                        f32 dz = em->pos.z - w->x4D8.z;
                        if (dx * dx + dz * dz < 4000000.0f) {
                            w->flags &= ~0x20000000;
                            EmRoutineSet(em, 1, 1, 0, 0);
                            return;
                        }
                        if (em->plDist2 < 4000000.0f && w->x530 < em->x3CC) {
                            w->flags &= ~0x20000000;
                        }
                    }
                    em10GotoPosCk(em);
                }
            }
        }
    }
}

static void em10_R1_CSawWalkAtk(cEm10* em)
{
}

static void em10_R1_ClawWalkAtk(cEm10* em)
{
}

static void em10_R1_br_ClawCriAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    Vec p;
    Mtx* m;

    if (em->hp <= 0) {
        return;
    }
    if (em->xFE != 5) {
        return;
    }
    if (!(em->seFlags28B & 1)) {
        return;
    }
    {
        p.x = 500.0f;
        p.y = 0.0f;
        p.z = 0.0f;
        m = &em->getPartsPtr(10)->mat;
        PSMTXMultVec(*m, &p, &p);
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(*m, &v, &v);
        em10AtkCk(em, &v, &p, 0xE, 0);
        v.x = -300.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(*m, &v, &v);
        em10AtkCk(em, &v, &p, 0xE, 0);
        v.x = -600.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(*m, &v, &v);
        em10AtkCk(em, &v, &p, 0xE, 0);
        switch (pG->x4FB8) {
        case 0:
        case 3:
        case 5:
            if (pSys->region && w->x697 && (s16) pG->pl_life <= 0) {
                pG->pl_life = 1;
                EmRoutineSet(em, 1, 0x2E, 0, 0);
            }
            break;
        }
    }
}

static void em10_R1_ClawCriAtk(cEm10* em)
{
}

static void em10_R1_ClawCriHit(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    em->dmg.x1 = 2;
    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x120), 0, 10, 1, 0);
        EstSetEm(em, -1, 0, 0, 0x10, 0x8C, 0, 0, em, 0);
        EmCatchPLSet(em, 0.0f, 2, (int) plem10_ClawCriHit, 241.15f, 0.0f, 975.16f);
        w->x4 = 10;
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        em->xFE++;
    case 1:
        if (MOTION(em)->seqFrame > 123.7f && MOTION(em)->seqFrame < 124.3f) {
            w->x6BE = 1;
            pG->pl_life = 0;
            DiedemoExec(2, 0);
        }
        if (w->x4) {
            w->x4--;
            EmCatchMotionMove(em, 0.3f, 0.2f);
        } else {
            MotionMoveF(em, 0);
        }
        break;
    }
    em->x3A8 = em->pos;
    em10HandSet(em, 0);
}

static void plem10_ClawCriHit(cPlayer* pl)
{
    cEm* em;

    pG->flags_5010 |= 0x8000;
    pl->dmg.set(0, 10);
    em = (cEm*) pl->dmgType;
    pl->subArc = em->subArc;
    switch (pl->xFE) {
    case 0:
        pl->atari.throughOn();
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0x123), 0, 5, 1, 0);
        EstSetEm(pl, -1, 0, 0, 0x10, 0x8B, 0, 0, pl, 0);
        pl->atari.set(10, 400.0f, 700.0f);
        pl->x3E8 = SndCall(1, 0xC, &pPL->pos, 0, 0, pPL);
        pl->x3E0 = 10;
        pl->xFE++;
    case 1:
        if (pl->x3E0) {
            pl->x3E0--;
            EmCatchMotionMove(pl, 0.3f, 0.2f);
        } else {
            MotionMoveF(pl, 0);
        }
        break;
    }
    pl->x3A8 = pl->pos;
    pl->subArc = pl->subArc2;
}

static void em10_R1_br_C_SawAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0 && (em->seFlags28B & 1) && w->pWep && em10CsawHitCk(em)) {
        EM_RTN_SET(em, 1, 0x30);
    }
}

static void em10_R1_C_SawAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xE9), (int) PL_ARC_PTR(em->subArc, 0xEA), 10, flag, 0x1E);
        w->x697 = 0;
        w->x6BB = 0;
        w->x4 = 5;
        SndStop(w->x5C4, 0);
        w->x5C4 = SndCall(6, 0x50, &em->pos, 0, 0, em);
        SndStop(w->x5B8, 0);
        SndStop(w->x5BC, 0);
        w->x5B8 = SndCall(6, 0x3D, &em->pos, 0, 0, em);
        em->xFE++;
    case 1:
        if (w->x4) {
            w->x4--;
            em->rot.y += Muku(&em->pos, &w->x54C, em->rot.y, 0.09817477f);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (MotionMoveF(em, 0)) {
            if (w->x697 == 0) {
                GameAddPoint(0xB);
            }
            em10WalkRtnSet(em);
        } else if (em->seFlags28B & 1) {
            em10SetDamageDoor(em, 2);
            em10SetDamageRack(em, 2);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_C_SawHit(cEm10* em)
{
}

static void plem10_C_SawHit(cPlayer* pl)
{
}

static void em10_R1_br_C_SawCriAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0 && (em->seFlags28B & 1) && w->pWep && em10CsawHitCk(em)) {
        if (fabsf(pPL->pos.y - em->pos.y) > 50.0f) {
            EM_RTN_SET(em, 1, 0x30);
        } else {
            EM_RTN_SET(em, 1, 0x32);
        }
    }
}

static void em10_R1_C_SawCriAtk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    if (w->flags & 0x100) {
        w->flags |= 0x40000;
    }
    switch (em->xFE) {
    case 0:
        flag = (em->flags_3C8 & 0x1000000) ? 0x41 : 1;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0xEB), (int) PL_ARC_PTR(em->subArc, 0xEC), 10, flag, 0);
        w->x697 = 0;
        w->x6BB = 0;
        w->x4 = 5;
        SndStop(w->x5C4, 0);
        w->x5C4 = SndCall(6, 0x50, &em->pos, 0, 0, em);
        SndStop(w->x5B8, 0);
        SndStop(w->x5BC, 0);
        w->x5B8 = SndCall(6, 0x3D, &em->pos, 0, 0, em);
        em->xFE++;
    case 1:
        if (w->x4) {
            w->x4--;
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.09817477f);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (MotionMoveF(em, 0)) {
            if (w->x697 == 0) {
                GameAddPoint(0xB);
            }
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_C_SawCriHit(cEm10* em)
{
}

static void plem10_C_SawCriHit(cPlayer* pl)
{
    cEm* em;

    pG->flags_5010 |= 0x8000;
    pl->dmg.set(0, 10);
    em = (cEm*) pl->dmgType;
    pl->subArc = em->subArc;
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0x103), 0, 5, 1, 0);
        pl->atari.set(10, 400.0f, 700.0f);
        pl->x3E8 = SndCall(1, 0xC, &pPL->pos, 0, 0, pPL);
        pl->xFE++;
    case 1:
        if (MOTION(pl)->seqFrame > 17.7f && MOTION(pl)->seqFrame < 18.3f) {
            pG->pl_life = 0;
            SndStop(pl->x3E8, 0);
            em10PlHeadLost();
        }
        if (MOTION(pl)->seqFrame > 76.7f && MOTION(pl)->seqFrame < 77.3f) {
            EstSetEm(pl, -1, 0, 0, 0x10, 0x4D, 0, 0, pl, 0);
        }
        if (MOTION(pl)->seqFrame > 39.7f && MOTION(pl)->seqFrame < 40.3f) {
            SndCall(5, 4, &pl->pos, 0, 0, pl);
        }
        if (MOTION(pl)->seqFrame > 70.7f && MOTION(pl)->seqFrame < 71.3f) {
            SndCall(5, 5, &pl->pos, 0, 0, pl);
        }
        if (EmCatchMotionMove(pl, 0.3f, 0.2f)) {
            pl->xFE++;
        }
        break;
    }
    pl->x3A8 = pl->pos;
    pl->subArc = pl->subArc2;
    if ((s16) pG->pl_life <= 0) {
        SndStop(pl->x3E8, 0);
    }
}

static void em10_R1_br_Catch(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0 && (em->seFlags28B & 2)) {
        if (em10CatchCk(em)) {
            if (pG->x4FB8 == 1) {
                EM_RTN_SET(em, 1, 0x36);
            } else if ((fabsf(Muku2(em->rot.y, pPL->rot.y, PI)) < 1.5707964f && (em10SomebodyNearCk(em) || pSUB)) || w->wepType == 9) {
                if (w->wepType == 9 && w->x640) {
                    EM_RTN_SET(em, 1, 0x38);
                } else {
                    EM_RTN_SET(em, 1, 0x37);
                }
            } else {
                EM_RTN_SET(em, 1, 0x34);
            }
        } else {
            em10CatchSubCk(em);
        }
    }
}

static void em10_R1_Catch(cEm10* em)
{
}

static void em10_R1_NeckHang(cEm10* em)
{
}

static void plem10_NeckHang(cPlayer* pl)
{
}

static void em10_R1_NeckHang_Luis(cEm10* em)
{
}

static void subem10_NeckHang_Luis(cSubChar* sub)
{
    cSubChar* s = pSUB;
    cEm* em;

    BitOn(pG->flags_5010, 0x10000);
    BitOn(pG->flags_5014, 0x20000000);
    s->dmg.set(0, 10);
    em = (cEm*) s->dmgType;
    s->subArc = em->subArc;
    switch (s->xFE) {
    case 0:
        MotionSetCore(s, MOTION(s), PL_ARC_PTR(s->subArc, 0x297), 0, 5, 1, 0);
        SubCharSetFace(1);
        s->atari.set(10, 480.00003f, 400.0f);
        SndCall(8, 9, &s->pos, s->id, 0, s);
        s->xFE++;
    case 1:
        EmCatchMotionMove(s, 0.3f, 0.2f);
        em = (cEm*) s->dmgType;
        if (em->xFC != 1 && em->xFD != 0x34) {
            EndSubDamage();
            s->dmg.set(0, 0x1E);
        } else {
            s->xFE = ((cEm*) s->dmgType)->xFE;
        }
        break;
    case 2:
        MotionSetCore(s, MOTION(s), PL_ARC_PTR(s->subArc, 0x299), 0, 5, 1, 0);
        SndCall(8, 0x11, &s->pos, s->id, 0, s);
        s->subHideMode = 15;
        s->xFE++;
    case 3:
        if (MotionMoveF(s, 0)) {
            EndSubDamage();
            s->dmg.set(0, 0x1E);
        } else if (s->subHideMode) {
            s->subHideMode--;
            em = (cEm*) s->dmgType;
            if (em->xFC != 1 && em->xFD != 0x34) {
                EndSubDamage();
                s->dmg.set(0, 0x1E);
            }
        }
        break;
    }
    s->x3A8 = s->pos;
    s->subArc = s->subArc2;
}

static void em10_R1_NeckHang_Ashley(cEm10* em)
{
}

static void subem10_NeckHang_Ashley(cSubChar* sub)
{
}

static void em10_R1_Backhold(cEm10* em)
{
}

static void plem10_Backhold(cPlayer* pl)
{
    cEm* em;
    PlArc* arc;

    BitOn(pG->flags_5010, 0x8000);
    BitOn(pG->flags_5010, 0x2000);
    em = (cEm*) pl->dmgType;
    arc = em->subArc;
    pl->subArc = arc;
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(arc, 0x29A), 0, 5, 1, 0);
        PlSetFace(1);
        pl->pWep->setTrans(0, 0);
        pl->atari.set(10, 480.00003f, 400.0f);
        pl->dmg.set(0, 10);
        pl->xFE++;
    case 1:
        EmCatchMotionMove(pl, 0.3f, 0.2f);
        em = (cEm*) pPL->dmgType;
        if (!EM_RTN(em, 1, 0x37)) {
            pl->pWep->setTrans(1, 0);
            EndPlDamage();
            pl->dmg.set(0, 0x1E);
        } else {
            pl->xFE = em->xFE;
        }
        break;
    case 2:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(arc, 0x29B), 0, 5, 1, 0);
        pl->xFE++;
    case 3:
        pl->dmg.x1 = 2;
        if (MotionMoveF(pl, 0)) {
            pl->pWep->setTrans(1, 0);
            EndPlDamage();
            pl->dmg.set(0, 0x1E);
        }
        break;
    }
    pl->x3A8 = pl->pos;
    pl->subArc = pl->subArc2;
}

static void em10_R1_Bombhold(cEm10* em)
{
}

static void plem10_Bombhold(cPlayer* pl)
{
    cEm* em;
    PlArc* arc;

    pG->flags_5010 |= 0x8000;
    pl->dmg.set(0, 10);
    em = (cEm*) pl->dmgType;
    arc = em->subArc;
    pl->subArc = arc;
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(arc, 0x29A), 0, 5, 1, 0);
        PlSetFace(1);
        pl->pWep->setTrans(0, 0);
        pl->atari.set(10, 480.00003f, 400.0f);
        pl->xFE++;
    case 1:
        EmCatchMotionMove(pl, 0.3f, 0.2f);
        em = (cEm*) pPL->dmgType;
        if (!EM_RTN(em, 1, 0x38)) {
            pl->pWep->setTrans(1, 0);
            EndPlDamage();
            pl->dmg.set(0, 0x1E);
        } else {
            pl->xFE = ((cEm*) pl->dmgType)->xFE;
        }
        break;
    case 2:
        pG->pl_life = 0;
        EstSetEm(pl, -1, 0, 0, 0x10, 0x4E, 0, 0, pl, 0);
        pl->x3E0 = 3;
        pl->xFE++;
    case 3:
        EmCatchMotionMove(pl, 0.3f, 0.2f);
        if (pl->x3E0) {
            pl->x3E0--;
            if (pl->x3E0 == 0) {
                pl->be_flag &= ~2;
            }
        }
        break;
    case 4:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(arc, 0x29B), 0, 5, 1, 0);
        pl->xFE++;
    case 5:
        if (MotionMoveF(pl, 0)) {
            pl->pWep->setTrans(1, 0);
            EndPlDamage();
        }
        break;
    }
    pl->x3A8 = pl->pos;
    pl->subArc = pl->subArc2;
}

static void em10_R1_br_DashCatch(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp > 0 && (em->seFlags28B & 2) && em10CatchCk(em)) {
        if (fabsf(Muku2(em->rot.y, pPL->rot.y, PI)) < 1.5707964f && (em10SomebodyNearCk(em) || pSUB || w->wepType == 9)) {
            if (w->wepType == 9 && w->x640) {
                EM_RTN_SET(em, 1, 0x38);
            } else {
                EM_RTN_SET(em, 1, 0x37);
            }
        } else {
            EM_RTN_SET(em, 1, 0x34);
        }
    }
}

static void em10_R1_DashCatch(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int end;
    int r;

    switch (em->xFE) {
    case 0:
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x8E), (int) PL_ARC_PTR(em->subArc, 0x8F), 10, 1, 0);
        em->xFF = 0;
        em10CallVoiceSe2(em, w->se6CB, 8);
        w->x4 = 15;
        em->xFE++;
    case 1:
        if (w->x4) {
            w->x4--;
            em->rot.y += Muku(&em->pos, &pPL->pos, em->rot.y, 0.19634955f);
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        end = MotionMoveF(em, 0);
        if (end) {
            r = Rnd();
            w->x664 = r % 300 + 300;
            GameAddPoint(0xB);
            em10WalkRtnSet(em);
        } else if ((em->seFlags28B & 1) && CheckInWater(em, 0)) {
            EstSetEm(em, -1, 0, 0, 1, 0x33, 0, 0, em, 0);
            SndCall(6, 0x16, &em->pos, 0, 0, em);
        } else if ((em->seFlags28B & 0x20) && CheckInWater(em, 0)) {
            EstSetEm(em, -1, 0, 0, 1, 0x34, 0, 0, em, 0);
            SndCall(6, 0x11, &em->pos, 0, 0, em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_TakeAway(cEm10* em)
{
}

static void subem10_TakeAway(cSubChar* sub)
{
}

static void em10_R1_Dm_Small(cEm10* em)
{
}

static void em10_R1_Dm_Head(cEm10* em)
{
}

static void em10_R1_Dm_Flash(cEm10* em)
{
}

static void em10_R1_Dm_Claw(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    w->x686 = Rnd() % 150 + 150;
    w->flags &= ~8;
    em10FindCk2(em);
    w->flags |= 0x100;
    w->x654 = 0x1C2;
    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        if (fabsf(Muku(&em->pos, &em->dmg.pos, em->rot.y, PI)) < 1.5707964f) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x10F), (int) PL_ARC_PTR(em->subArc, 0x110), 3, 1, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x111), (int) PL_ARC_PTR(em->subArc, 0x112), 3, 1, 0);
        }
        em10SetDamageVoice(em, w->se6C7, w->se6C6);
        w->x5F0 = pG->bell_pos;
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        } else if (em->seFlags28B & 1) {
            em10CallVoiceSe2(em, w->se6D1, 8);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_Claw_Big(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    f32 a;

    w->x686 = Rnd() % 150 + 150;
    em10FindCk2(em);
    w->flags |= 0x100;
    w->x654 = 0x1C2;
    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x113), (int) PL_ARC_PTR(em->subArc, 0x114), 3, 1, 0);
        if (w->pParasite) {
            if (w->pParasite->ckAtkEnable()) {
                w->pParasite->setDamage();
            }
            EstSetEm(w->pParasite, -1, 0, 0, 0x10, 0x77, 0, 0, w->pParasite, 0);
        }
        em10CallVoiceSe(em, w->se6C7);
        w->x5F0 = pG->bell_pos;
        w->x18 = em->rot.y + PI;
        em->xFE++;
    case 1:
        if (em->seFlags28B & 8) {
            a = Muku(&em->pos, &w->x54C, w->x18, 0.09817477f);
            w->x18 += a;
            w->x18 = LIMIT_ANGLE(w->x18);
            em->rot.y += a;
            em->rot.y = LIMIT_ANGLE(em->rot.y);
        }
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_Gatling(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    w->x686 = Rnd() % 150 + 150;
    em10FindCk2(em);
    w->flags |= 0x100;
    w->x654 = 0x1C2;
    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        if (fabsf(Muku(&em->pos, &em->dmg.pos, em->rot.y, PI)) < 1.5707964f) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x194), (int) PL_ARC_PTR(em->subArc, 0x195), 3, 1, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x196), (int) PL_ARC_PTR(em->subArc, 0x197), 3, 1, 0);
        }
        em10SetDamageVoice(em, w->se6C7, w->se6C6);
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_FS(cEm10* em)
{
}

static void em10_R1_Dm_KneeKick(cEm10* em)
{
}

static void em10_R1_Dm_NeckBreak(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int end;

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x2BA), (int) PL_ARC_PTR(em->subArc, 0x2BB), 0, 1, 0);
        EstSetEm(em, -1, 0, 0, 0x10, 0x90, 0, 0, em, 0);
        EmCatchPLSet(em, 0.0f, 2, (int) plem10NeckBreak, -75.21f, 0.0f, 1076.1f);
        em10SetDmWaterEff(em, 1);
        SndStop(w->x5C4, 0);
        em->hp = 0;
        w->flags &= ~0x20;
        w->x4 = 30;
        em->xFE++;
    case 1:
        em->dmg.set(0, 2);
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x10;
            w->flags |= 0x1000000;
            em->setStatus(3);
        } else {
            em->pos.y = pPL->pos.y;
            w->flags |= 0x80000;
        }
        if (em->seFlags28B & 2) {
            em10SetDmWaterEff(em, 1);
        }
        if (w->x4) {
            w->x4--;
            end = EmCatchMotionMove(em, 1.0f, 1.0f);
        } else {
            end = MotionMoveF(em, 0);
        }
        if (end) {
            em->pos.y = SatMgr.getFloor(&em->pos, 600.0f, 100000.0f, 0, 0);
            EmRoutineSet(em, 3, 0, 0, 1);
        } else if (em->seFlags28B & 1) {
            SndCall(1, 0x4C, &em->getPartsPtr(4)->worldPos, 0, 0, em);
            em10SetDamageVoice(em, w->se6CE, w->se6C6);
            em->setWeaponFall();
            if (w->pShield) {
                w->pShield->setFall(20.0f, 0);
                w->pShield = 0;
            }
            em10SetPoint(em);
        }
        break;
    }
    em->x3A8 = em->pos;
    em10HandSet(em, 0);
    em10SetCrash(em, 1500.0f);
}

static void em10_R1_Dm_Showtay(cEm10* em)
{
}

static void em10_R1_Dm_Heel(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x2B7), (int) PL_ARC_PTR(em->subArc, 0x2B8), 3, 1, 0);
        w->flags &= ~0x20;
        if (em->hp <= 0) {
            em10LostHead(em, 1, 0);
            em->setWeaponFall();
            if (w->pShield) {
                w->pShield->setFall(20.0f, 0);
                w->pShield = 0;
            }
        }
        if (!(w->flags & 0x80)) {
            em10CallVoiceSe2(em, w->se6D0, 8);
        }
        if (w->pParasite && w->pParasite->ckAtkEnable()) {
            w->pParasite->setDamage();
        }
        if (w->x58C && w->x58C->vB8()) {
            w->x58C->vC0();
        }
        em10SetDmWaterEff(em, 0);
        SndStop(w->x5C4, 0);
        GameAddPoint(9);
        w->x10 = 10;
        em->xFE++;
    case 1:
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x10;
            w->flags |= 0x1000000;
            em->setStatus(3);
        }
        if (MotionMoveF(em, 0)) {
            if (em->hp <= 0) {
                EmRoutineSet(em, 3, 0, 0, 0);
            } else {
                EmRoutineSet(em, 1, 0x1E, 0, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_DashUp(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        flag = (Rnd() & 1) ? 1 : 0x41;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x57), (int) PL_ARC_PTR(em->subArc, 0x58), 6, flag, 0);
        em10SetDamageVoice(em, w->se6CE, w->se6C6);
        em10SetDmWaterEff(em, 0);
        SndStop(w->x5C4, 0);
        if (w->pParasite && w->pParasite->ckAtkEnable()) {
            w->pParasite->setDamage();
        }
        if (w->x58C && w->x58C->vB8()) {
            w->x58C->vC0();
        }
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_DashDown(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        flag = 1;
        if (em->dmPart->partsNo == 0x13) {
            flag = 0x41;
        }
        if (em->dmPart->partsNo == 0x14) {
            flag = 0x41;
        }
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x59), (int) PL_ARC_PTR(em->subArc, 0x5A), 3, flag, 0);
        w->flags &= ~0x20;
        em10SetDamageVoice(em, w->se6CE, w->se6C6);
        em10SetDmWaterEff(em, 0);
        SndStop(w->x5C4, 0);
        if (w->pParasite && w->pParasite->ckAtkEnable()) {
            w->pParasite->setDamage();
        }
        if (w->x58C && w->x58C->vB8()) {
            w->x58C->vC0();
        }
        em->xFE++;
    case 1:
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x10;
            w->flags |= 0x1000000;
            em->setStatus(3);
        }
        if (em->seFlags28B & 1) {
            if (ChkWaterEffectEnable(&em->pos)) {
                EstSetEm(em, -1, 0, 0, 0x10, 0x32, 0, 0, em, 0);
            } else {
                EstSetEm(em, -1, 0, 0, 0x10, 0x1A, 0, 0, em, 0);
            }
        }
        if (MotionMoveF(em, 0)) {
            if (em->hp <= 0) {
                EmRoutineSet(em, 3, 0, 0, 1);
            } else {
                EmRoutineSet(em, 1, 0x1E, 0, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_Blow(cEm10* em)
{
}

static void em10_R1_Dm_Fence(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        em->xFF = Rnd() & 1;
        flag = em->xFF ? 1 : 0x41;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x5B), (int) PL_ARC_PTR(em->subArc, 0x5C), 3, flag, 0);
        w->flags |= 0x20;
        em10SetDamageVoice(em, w->se6CE, w->se6C6);
        em->setWeaponFall();
        if (w->pShield) {
            w->pShield->setFall(20.0f, 0);
            w->pShield = 0;
        }
        if (w->pParasite && w->pParasite->ckAtkEnable()) {
            w->pParasite->setDamage();
        }
        if (w->x58C && w->x58C->vB8()) {
            w->x58C->vC0();
        }
        em10SetDmWaterEff(em, 1);
        SndStop(w->x5C4, 0);
        em->xFE++;
    case 1:
        em->dmg.set(0, 2);
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x10;
            w->flags |= 0x1000000;
            em->setStatus(3);
        }
        if (em->seFlags28B & 1) {
            if (ChkWaterEffectEnable(&em->pos)) {
                EstSetEm(em, -1, 0, 0, 0x10, 0x32, 0, 0, em, 0);
            } else {
                EstSetEm(em, -1, 0, 0, 0x10, 0x1A, 0, 0, em, 0);
            }
        }
        if (MotionMoveF(em, 0)) {
            if (em->hp <= 0) {
                EmRoutineSet(em, 3, 0, 0, 1);
            } else {
                EmRoutineSet(em, 1, 0x1E, 0, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
    em10SetCrash(em, 800.0f);
}

static void em10_R1_Dm_Ladder(cEm10* em)
{
}

static void em10_R1_Dm_Roof(cEm10* em)
{
}

static void em10_R1_Dm_KneeDown(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        if (fabsf(Muku(&em->pos, &em->dmg.pos, em->rot.y, PI)) < 1.5707964f) {
            em->xFF = Rnd() & 1;
            flag = (MOTION(em)->flags & 0x40) ? 0x41 : 1;
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x3D), (int) PL_ARC_PTR(em->subArc, 0x3E), 15, flag, 0);
            w->flags |= 0x20;
        } else {
            em->xFF = Rnd() & 1;
            flag = (MOTION(em)->flags & 0x40) ? 0x41 : 1;
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x49), (int) PL_ARC_PTR(em->subArc, 0x4A), 15, flag, 0);
            w->flags &= ~0x20;
        }
        if (em->hp <= 0) {
            em->setWeaponFall();
            if (w->pShield) {
                w->pShield->setFall(20.0f, 0);
                w->pShield = 0;
            }
        }
        if (!(w->flags & 0x80)) {
            em10CallVoiceSe2(em, w->se6D0, 8);
        }
        if (w->pParasite && w->pParasite->ckAtkEnable()) {
            w->pParasite->setDamage();
        }
        if (w->x58C && w->x58C->vB8()) {
            w->x58C->vC0();
        }
        em10SetDmWaterEff(em, 0);
        SndStop(w->x5C4, 0);
        GameAddPoint(9);
        w->x10 = 10;
        em->xFE++;
    case 1:
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x10;
            w->flags |= 0x1000000;
            em->setStatus(3);
        }
        if (MotionMoveF(em, 0)) {
            if (em->hp <= 0) {
                EmRoutineSet(em, 3, 0, 0, 0);
            } else {
                EmRoutineSet(em, 1, 0x1E, 0, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_KnockOut(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        em->xFF = Rnd() & 1;
        flag = em->xFF ? 1 : 0x41;
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x69), (int) PL_ARC_PTR(em->subArc, 0x6A), 15, flag, 0);
        w->flags &= ~0x20;
        if (em->hp <= 0) {
            em->setWeaponFall();
            if (w->pShield) {
                w->pShield->setFall(20.0f, 0);
                w->pShield = 0;
            }
        }
        if (!(w->flags & 0x80)) {
            em10CallVoiceSe2(em, w->se6D0, 8);
        }
        em10SetDmWaterEff(em, 0);
        SndStop(w->x5C4, 0);
        if (w->pParasite && w->pParasite->ckAtkEnable()) {
            w->pParasite->setDamage();
        }
        if (w->x58C && w->x58C->vB8()) {
            w->x58C->vC0();
        }
        w->x10 = 10;
        em->xFE++;
    case 1:
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x10;
            w->flags |= 0x1000000;
            em->setStatus(3);
        }
        if (em->seFlags28B & 1) {
            if (ChkWaterEffectEnable(&em->pos)) {
                EstSetEm(em, -1, 0, 0, 0x10, 0x32, 0, 0, em, 0);
            } else {
                EstSetEm(em, -1, 0, 0, 0x10, 0x1A, 0, 0, em, 0);
            }
        }
        if (MotionMoveF(em, 0)) {
            if (em->hp <= 0) {
                EmRoutineSet(em, 3, 0, 0, 0);
            } else {
                EmRoutineSet(em, 1, 0x1E, 0, 0);
            }
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_Down(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int flag;

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        flag = (MOTION(em)->flags & 0x40) ? 0x41 : 1;
        if (w->flags & 0x20) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x4F), 0, 5, flag, 0);
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x50), 0, 5, flag, 0);
        }
        if (w->pParasite && w->pParasite->ckAtkEnable()) {
            w->pParasite->setDamage();
        }
        if (w->x58C && w->x58C->vB8()) {
            w->x58C->vC0();
        }
        em->xFE++;
    case 1:
        w->flags |= 0x10;
        w->flags |= 0x1000000;
        em->setStatus(3);
        if (MotionMoveF(em, 0)) {
            EmRoutineSet(em, 1, 0x1F, 0, 0);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Dm_Frame(cEm10* em)
{
}

static void em10_R1_Dm_TakeAway(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        w->flags &= ~0x8000000;
        em10MouthPartsReset(em);
        if (w->x6BE != 4) {
            w->x6BE = 3;
        }
        if (w->x6BF != 4) {
            w->x6BF = 3;
        }
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x59), (int) PL_ARC_PTR(em->subArc, 0x5A), 3, 1, 0);
        em10SetDmWaterEff(em, 0);
        SndStop(w->x5C4, 0);
        w->flags &= ~0x20;
        em->xFE++;
    case 1:
        if (MotionMoveF(em, 0)) {
            if (em->hp <= 0) {
                EmRoutineSet(em, 3, 0, 0, 1);
            } else {
                EmRoutineSet(em, 1, 0x1E, 0, 0);
            }
        } else if ((em->seFlags28B & 4) && em->hp <= 0) {
            EmRoutineSet(em, 3, 2, 0, 0);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Die_Cramp(cEm10* em)
{
}

static void em10_R1_Die_Lost(cEm10* em)
{
}

static void em10_R1_Die_Down(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    w->flags |= 0x10;
    w->flags |= 0x1000000;
    em->setStatus(3);
    switch (em->xFE) {
    case 0:
        em10MouthPartsReset(em);
        em10SetPoint(em);
        if (w->flags & 0x20) {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x6B), 0, 6, 1, 0);
            w->x4 = 0x46;
        } else {
            MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x6C), 0, 6, 1, 0);
            w->x4 = 0x4A;
        }
        em10SetDamageVoice(em, w->se6CE, w->se6C6);
        em->setWeaponFall();
        if (w->pShield) {
            w->pShield->setFall(20.0f, 0);
            w->pShield = 0;
        }
        em->xFE++;
    case 1:
        if (w->x4) {
            w->x4--;
            if (w->x4 == 0) {
                SndCall(8, 4, &em->pos, em->id, 0, em);
            }
        }
        if (MotionMoveF(em, 0)) {
            EmRoutineSet(em, 3, 0, 0, 1);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Die_Normal(cEm10* em)
{
}

static void em10_R1_Die_RunDown(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    switch (em->xFE) {
    case 0:
        em10MouthPartsReset(em);
        em10SetPoint(em);
        MotionSetCore(em, MOTION(em), PL_ARC_PTR(em->subArc, 0x59), (int) PL_ARC_PTR(em->subArc, 0x5A), 3, 1, 0);
        em->hp = 0;
        w->flags &= ~0x20;
        em10SetDamageVoice(em, w->se6CE, w->se6C6);
        em10SetDmWaterEff(em, 0);
        SndStop(w->x5C4, 0);
        em->setWeaponFall();
        if (w->pShield) {
            w->pShield->setFall(20.0f, 0);
            w->pShield = 0;
        }
        em->xFE++;
    case 1:
        if (em->seFlags28B & 0x80) {
            w->flags |= 0x10;
            w->flags |= 0x1000000;
            em->setStatus(3);
        }
        if (em->seFlags28B & 1) {
            if (ChkWaterEffectEnable(&em->pos)) {
                EstSetEm(em, -1, 0, 0, 0x10, 0x32, 0, 0, em, 0);
            } else {
                EstSetEm(em, -1, 0, 0, 0x10, 0x1A, 0, 0, em, 0);
            }
        }
        if (MotionMoveF(em, 0)) {
            EmRoutineSet(em, 3, 0, 0, 1);
        }
        break;
    }
    em10HandSet(em, 0);
}

static void em10_R1_Die_Bomb(cEm10* em)
{
}

static void plemDmFrame(cPlayer* pl)
{
    cEm* em = (cEm*) pl->dmgType;
    int end;
    int dmg;

    pl->subArc = em->subArc;
    pl->dmg.set(0, 0x1E);
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0x29C), 0, 3, 1, 0);
        PlSetDamageSe(0);
        EstSetEm(pl, -1, 0, 0, 0x10, 0x28, 0, 0, pl, 0);
        VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 0xB, 1);
        pl->x3E0 = 50;
        pl->xFE++;
    case 1:
        end = MotionMoveF(pl, 0);
        if (pl->x3E0) {
            dmg = 10;
            dmg = (int) (em10GetPower((cEm10*) pl) * (f32) dmg);
            if ((pl->flags_3C8 & 4) && !(pG->flags_54 & 0x20)) {
                dmg = (dmg + 1) / 2;
            }
            LifeDownSet(pPL, dmg, 0);
            pl->x3E0--;
            if (pl->x3E0 == 0 && (s16) pG->pl_life <= 0) {
                PlSetDamageSe(0xD);
                PlSetDamage(6, 0, 0);
                break;
            }
        }
        if (end) {
            EndPlDamage();
            pl->dmg.set(0, 0xF);
        }
        break;
    }
    plem10KickCamMove(pl, pl->xFF);
    pl->subArc = pl->subArc2;
}


static void subem10DmGondolaShake(cSubChar* sub)
{
    cSubChar* s = pSUB;

    s->dmg.x1 = 2;
    BitOn(pG->flags_5010, 0x10000);
    BitOn(pG->flags_5014, 0x20000000);
    switch (s->xFE) {
    case 0:
        MotionSetCore(s, MOTION(s), PL_ARC_PTR(s->subArc, 0x43), 0, 3, 1, 0);
        s->xFE++;
    case 1:
        if (MotionMoveF(s, 0)) {
            s->xFE++;
        }
        break;
    case 2:
        MotionSetCore(s, MOTION(s), PL_ARC_PTR(s->subArc, 0x44), 0, 3, 1, 0);
        s->xFE++;
    case 3:
        if (MotionMoveF(s, 0) && s->xFF == 0) {
            s->xFE++;
        }
        break;
    case 4:
        MotionSetCore(s, MOTION(s), PL_ARC_PTR(s->subArc, 0x45), 0, 3, 1, 0);
        s->xFE++;
    case 5:
        if (MotionMoveF(s, 0)) {
            EndSubDamage();
        }
        break;
    }
}

static void plemDmMStar(cPlayer* pl)
{
    cEm* em = (cEm*) pl->dmgType;
    int flag;

    pl->subArc = em->subArc;
    if (pl->xFF == 0) {
        pl->dmg.set(0, 2);
    }
    switch (pl->xFE) {
    case 0:
        flag = pl->xFF ? 0x41 : 1;
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0x17A), 0, 3, flag, 0);
        PlSetFace(1);
        PlSetDamageSe(0);
        if (pl->xFF) {
            pl->dmg.set(0, 0xF);
        }
        pl->xFE++;
    case 1:
        if (MotionMoveF(pl, 0)) {
            EndPlDamage();
            if (!pl->xFF) {
                pl->dmg.set(0, 0xF);
            }
        }
        break;
    }
    pl->subArc = pl->subArc2;
}

static void plemDmStun(cPlayer* pl)
{
    cEm* em = (cEm*) pl->dmgType;

    pl->subArc = em->subArc;
    pl->dmg.set(0, 2);
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0x1A7), 0, 3, 1, 0);
        PlSetDamageSe(0);
        EstSetEm(pl, -1, 0, 0, 0xCD, 2, 0, 0, pl, 0);
        pl->xFE++;
    case 1:
        if (MotionMoveF(pl, 0)) {
            pl->dmg.set(0, 0xF);
            EndPlDamage();
        } else if (MOTION(pl)->seqFrame > 16.7f && MOTION(pl)->seqFrame < 17.3f && (s16) pG->pl_life <= 0) {
            EmRoutineSet(pPL, 2, 0, 0, 0);
        }
        break;
    }
    pl->subArc = pl->subArc2;
}

static void em10KickAction(cEm10* em)
{
    switch (pG->x4FB8) {
    default:
        SetPlDamage((int) em, plem10Kick);
        break;
    case 5:
        SetPlDamage((int) em, plem10Showtay);
        break;
    case 4:
        SetPlDamage((int) em, plem10Kick2);
        break;
    case 3:
        EmRoutineSet(em, 2, 0x15, 0, 0);
        em->dmg.set(0, 0x1E);
        pPL->dmg.set(0, 0x1E);
        break;
    }
    pPL->dmg.set(0, 0x1E);
    if (pSUB && !em10DmgDeadCk(&pSUB->dmg)) {
        pSUB->dmg.set(0, 0x1E);
    }
}

static void em10KneeDownAction(cEm10* em)
{
    switch (pG->x4FB8) {
    case 4:
        SetPlDamage((int) em, plem10Kick2);
        break;
    case 3:
    case 5:
        SetPlDamage((int) em, plem10Kick);
        pPL->xFF = 1;
        break;
    default:
        SetPlDamage((int) em, plem10Kick);
        pPL->xFF = 1;
        break;
    }
    pPL->dmg.set(0, 0x1E);
    if (pSUB && !em10DmgDeadCk(&pSUB->dmg)) {
        pSUB->dmg.set(0, 0x1E);
    }
}

static void plem10Kick(cPlayer* pl)
{
}

static void plem10Kick2(cPlayer* pl)
{
}

static void em10FSAction(cEm10* em)
{
    if (!em10DmgDeadCk(&em->dmg)) {
        if (pG->x4FB8 == 4) {
            EmRoutineSet(em, 2, 0x14, 0, 0);
            SetPlDamage((int) em, plem10KneeKick);
            em->dmg.set(0, 0x1E);
            pPL->dmg.set(0, 0x1E);
            if (pSUB && !em10DmgDeadCk(&pSUB->dmg)) {
                pSUB->dmg.set(0, 0x1E);
            }
        } else {
            EmRoutineSet(em, 2, 0x10, 0, 0);
            SetPlDamage((int) em, plem10FS);
            em->dmg.set(0, 0x1E);
            pPL->dmg.set(0, 0x1E);
            if (pSUB && !em10DmgDeadCk(&pSUB->dmg)) {
                pSUB->dmg.set(0, 0x1E);
            }
        }
    }
}

static void plem10FS(cPlayer* pl)
{
    cEm* em = (cEm*) pl->dmgType;

    pl->subArc = em->subArc;
    pl->dmg.set(0, 0x1E);
    pG->flags_5014 |= 0x40000000;
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0xD6), 0, 0, 1, 0);
        GameAddPoint(9);
        pl->atari.flags |= 8;
        pl->xFF = Rnd() & 1;
        pl->xFE++;
    case 1:
        if (MOTION(pl)->seqFrame > 9.7f && MOTION(pl)->seqFrame < 10.3f) {
            SndCall(1, 0x11, &pl->pos, 0, 0, pPL);
            SndCall(1, 0x10, &pl->pos, 0, 0, pPL);
        }
        if (MOTION(pl)->seqFrame > 60.7f && MOTION(pl)->seqFrame < 61.3f) {
            SndCall(1, 0x43, &pl->pos, 0, 0, pPL);
            SndCall(1, 0x44, &pl->pos, 0, 0, pPL);
        }
        if (MotionMoveF(pl, 0)) {
            EndPlDamage();
            pl->dmg.set(0, 0xF);
        }
        break;
    }
    pl->subArc = pl->subArc2;
}

static void plem10KneeKick(cPlayer* pl)
{
    cEm* em = (cEm*) pl->dmgType;

    pl->subArc = em->subArc;
    pl->dmg.set(0, 0x1E);
    pG->flags_5014 |= 0x40000000;
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0x2B4), 0, 0, 1, 0);
        GameAddPoint(9);
        EstSetEm(pl, -1, 0, 0, 3, 9, 0, 0, pl, 0);
        pl->atari.flags |= 8;
        pl->xFF = Rnd() & 1;
        pl->xFE++;
    case 1:
        if (MOTION(pl)->seqFrame > 17.7f && MOTION(pl)->seqFrame < 18.3f) {
            SndCall(1, 0x3B, &pl->pos, 0, 0, pPL);
            SndCall(1, 0x3D, &pl->pos, 0, 0, pPL);
        }
        if (MOTION(pl)->seqFrame > 35.7f && MOTION(pl)->seqFrame < 36.3f) {
            SndCall(5, 2, &pl->pos, 0, 0, pl);
        }
        if ((MOTION(pl)->seqFrame > 9.7f && MOTION(pl)->seqFrame < 10.3f) || (MOTION(pl)->seqFrame > 46.7f && MOTION(pl)->seqFrame < 47.3f)) {
            SndCall(5, 3, &pl->pos, 0, 0, pl);
        }
        if (MotionMoveF(pl, 0)) {
            EndPlDamage();
            pl->dmg.set(0, 0xF);
        }
        break;
    }
    pl->subArc = pl->subArc2;
}

static void plem10NeckBreak(cPlayer* pl)
{
    cEm* em = (cEm*) pl->dmgType;
    int end;

    pl->subArc = em->subArc;
    pl->dmg.set(0, 0x1E);
    pG->flags_5014 |= 0x40000000;
    switch (pl->xFE) {
    case 0:
        MotionSetCore(pl, MOTION(pl), PL_ARC_PTR(pl->subArc, 0x2B9), 0, 0, 1, 0);
        GameAddPoint(9);
        EstSetEm(pl, -1, 0, 0, 3, 8, 0, 0, pl, 0);
        SndCall(1, 0x4D, &pl->pos, 0, 0, pPL);
        pl->pWep->setTrans(0, 0);
        pl->setRightHand(0);
        pl->setLeftHand(0);
        pl->x3E0 = 30;
        pl->x3E4 = 70;
        pl->xFE++;
    case 1:
        if (pl->x3E0) {
            pl->x3E0--;
            end = EmCatchMotionMove(pl, 1.0f, 1.0f);
            DmgMgr.set(3, 2, &pl->pos, 50.0f, 50.0f);
        } else {
            end = MotionMoveF(pl, 0);
        }
        if (pl->x3E4) {
            pl->x3E4--;
        } else if (joyKamae() || (Key.on & 0x10F)) {
            pl->pWep->setTrans(1, 0);
            pl->setRightHand(1);
            pl->setLeftHand(0x63);
            EndPlDamage();
            pl->dmg.set(0, 0xF);
            break;
        }
        if (end) {
            pl->pWep->setTrans(1, 0);
            pl->setRightHand(1);
            pl->setLeftHand(0x63);
            EndPlDamage();
            pl->dmg.set(0, 0xF);
        }
        break;
    }
    pl->x3A8 = pl->pos;
    pl->subArc = pl->subArc2;
}

static void plem10Showtay(cPlayer* pl)
{
}

static void em10TradeAction(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (pG->room_id != 0x20F) {
        if (w->x6B0) {
            SubScreenOpen(0x10, 0);
        } else {
            EmRoutineSet(em, 1, 0x46, 0, 0);
            w->x6B0 = 1;
        }
    } else {
        SubScreenOpen(0x10, 0);
    }
    pPL->dmg.set(0, 0x1E);
}

// ===== cEm10 accessors and small helpers =====
u32 cEm10::ckGoto()
{
    return EM10_WK(this)->x5EC;
}

void cEm10::setGatlingMode(u8 no)
{
    EM10_WK(this)->gatlingMode = no;
}

void cEm10::setR11DMotion(void* m0)
{
    EM10_WK(this)->evtMot[0] = m0;
}

void cEm10::setSwitch(cModel* sw)
{
    EM10_WK(this)->pSwitch = sw;
}

void cEm10::chgSet(u8 no)
{
    EM10_WK(this)->x4C4 = no;
    x38D = no;
}

extern "C" void plem10KickCamMove(cPlayer* pl, int a)
{
}

int cEm10::checkThrow()
{
    if ((s16) EM10_WK(this)->x670 == 0) {
        return 0;
    }
    return 1;
}

int cEm10::ckBombFire()
{
    return EM10_WK(this)->x640 != 0;
}

int cEm10::ckParasite()
{
    return EM10_WK(this)->pParasite != 0;
}

int cEm10::ckResetEnable()
{
    if (EM10_WK(this)->x6B7 == 0) {
        return 0;
    }
    return 1;
}

int cEm10::ckShiled()
{
    return EM10_WK(this)->pShield != 0;
}

int cEm10::ckTakeAway()
{
    if (EM10_WK(this)->flags & 0x4000) {
        return 1;
    }
    return 0;
}

int cEm10::ckWeapon()
{
    return EM10_WK(this)->pWep != 0;
}

void cEm10::setDrill(void* m0, void* m1, void* m2, void* m3)
{
    Em10Work* w = EM10_WK(this);

    w->evtMot[0] = m0;
    w->evtMot[1] = m1;
    w->x184 = (cModelInfo*) m2;
    w->x20 = (int) m3;
}

void cEm10::setEvtMotion(void* m0, void* m1, void* m2, void* m3)
{
    Em10Work* w = EM10_WK(this);

    w->evtMot[0] = m0;
    w->evtMot[4] = m1;
    w->evtMot[1] = m2;
    w->evtMot[5] = m3;
}

void cEm10::setGondolaMotion(void* m0, void* m1, void* m2, void* m3)
{
    Em10Work* w = EM10_WK(this);

    w->evtMot[0] = m0;
    w->evtMot[1] = m1;
    w->evtMot[2] = m2;
    w->evtMot[3] = m3;
}

void em10ReturnStartPosCk(cEm10* em)
{
    em10ReturnPosCk(em);
}

void cEm10::setUFOCatch(void* m0, void* m1)
{
    Em10Work* w = EM10_WK(this);

    w->evtMot[0] = m0;
    w->evtMot[1] = m1;
    EmRoutineSet(this, 1, 0x65, 0, 0);
}

int cEm10::ckBowgunFire()
{
    Em10Work* w = EM10_WK(this);

    if (w->wepType != 8) {
        return 0;
    } else {
        if (w->x6C2 == 0) {
            return 0;
        }
        return 1;
    }
}

void Em1fClothMove(cModel* m, PlCloth* c)
{
    PenClothMove(m, (PenCloth*) c);
    m->be_flag &= ~0xE00000;
}

int cEm10::ckR305BomberEnable()
{
    if (xFC != 1) {
        return 0;
    }
    if (xFD == 0x6A) {
        return xFE == 1;
    }
    return 0;
}

int em10GotoCk(cEm10* em)
{
    if (EM10_WK(em)->x5EC == 0) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x13, 0, 0);
    return 1;
}

void em10SetCrash(cEm10* em, f32 r)
{
    if (em->seFlags28B & 0x10) {
        DmgMgr.set(3, 2, &em->pos, 1500.0f, r);
    }
}

cObjGondola* em10GetGondola(cEm10* em)
{
    cObj* o;

    for (o = ObjMgr.pAlive; o; o = (cObj*) o->next) {
        if (o->id == 0x35 && ((cObjGondola*) o)->ckRide()) {
            return (cObjGondola*) o;
        }
    }
    return 0;
}

void em10SetDamageVoice(cEm10* em, int a, int b)
{
    Em10Work* w = EM10_WK(em);

    if (em->hp <= 0) {
        if (!(w->flags & 0x80)) {
            em10CallVoiceSe2(em, b, 8);
        }
        if (em->type != 10 && em->type != 13 && (w->x58C || w->pParasite)) {
            SndStop(w->x5B8, 0);
            SndStop(w->x5BC, 0);
            w->x5B8 = Ctrl11SetSe2(w->pCtrl11, em, Rnd() % 20 + 20, 0x89, 6, 8);
            w->x67E = Rnd() % 120 + 120;
        }
    } else if (!(w->flags & 0x80)) {
        em10CallVoiceSe(em, a);
    }
}

void em10MouthPartsReset(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cModel* p;

    if (em->hp > 0 && !(w->flags & 0x80)) {
        p = em->getPartsPtr(0x1B);
        p->rot.x = 0.0f;
        p->rot.y = 0.0f;
        p->rot.z = 0.0f;
        p = em->getPartsPtr(0x1C);
        p->rot.x = 0.0f;
        p->rot.y = 0.0f;
        p->rot.z = 0.0f;
        p = em->getPartsPtr(0x1D);
        p->rot.x = 0.0f;
        p->rot.y = 0.0f;
        p->rot.z = 0.0f;
        p = em->getPartsPtr(0x1E);
        p->rot.x = 0.0f;
        p->rot.y = 0.0f;
        p->rot.z = 0.0f;
        p = em->getPartsPtr(0x1F);
        p->rot.x = 0.0f;
        p->rot.y = 0.0f;
        p->rot.z = 0.0f;
        p = em->getPartsPtr(0x20);
        p->rot.x = 0.0f;
        p->rot.y = 0.0f;
        p->rot.z = 0.0f;
        p = em->getPartsPtr(0x21);
        p->rot.x = 0.0f;
        p->rot.y = 0.0f;
        p->rot.z = 0.0f;
    }
}

void em10SetDmWaterEff(cEm10* em, int a)
{
    Em10Work* w = EM10_WK(em);
    cModel* p;
    s8 wait;

    if (CheckInWater(em, 4)) {
        wait = w->x6A3;
        if (wait == 0) {
            p = em->getPartsPtr(0);
            switch (a) {
            case 0:
            default:
                EstSetEm(em, -1, 0, 0, 1, 0x34, 0, (int) wait, em, 0);
                SndCall(6, 0x11, &p->worldPos, 0, 0, em);
                w->x6A3 = 5;
                break;
            case 1:
                EstSetEm(em, -1, 0, 0, 1, 0x33, 0, (int) wait, em, 0);
                SndCall(6, 0x16, &p->worldPos, 0, 0, em);
                w->x6A3 = 10;
                break;
            }
        }
    }
}

extern "C" int em10ReturnPosCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EmiData* emi;
    EmiEntry* e;
    u32 i;

    if (pG->pRoomEmi == 0) {
        return 0;
    }
    if (w->flags & 0x4000) {
        return 0;
    }
    if (em->x3D0 != 1 && em->x3D0 != 3) {
        return 0;
    }
    emi = (EmiData*) pG->pRoomEmi;
    for (i = 0; i < emi->n; i++) {
        f32 dx;
        f32 dy;
        f32 dz;
        e = &emi->entry[i];
        if (e->type != 0xC) {
            continue;
        }
        dx = em->pos.x - e->pos.x;
        dy = em->pos.y - e->pos.y;
        dz = em->pos.z - e->pos.z;
        if (dx * dx + dy * dy + dz * dz > 9000000.0f) {
            continue;
        }
        w->flags |= 0x20000000;
        w->x4F8 = em->pos;
        EmRoutineSet(em, 1, 0x1B, 0, 0);
        return 1;
    }
    return 0;
}

void em10CallVoiceSe2(cEm10* em, int no, int a)
{
    Em10Work* w = EM10_WK(em);

    SndStop(w->x5B8, 0);
    SndStop(w->x5BC, 0);
    if (!(w->flags & 0x80)) {
        w->x5B8 = Ctrl11SetSe2I(w->pCtrl11, em, Rnd() % 20 + 20, no, 6, a);
        w->x67E = Rnd() % 120 + 120;
    }
}

void em10BreathSe(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->x67E) {
        w->x67E--;
    } else {
        w->x67E = Rnd() % 120 + 120;
        if (!(w->flags & 0x80)) {
            w->x5BC = Ctrl11SetSe(w->pCtrl11, em, Rnd() % 20 + 20, w->se6CD, 6);
        }
    }
}

extern "C" int em10SomebodyFindNowCk(cEm10* em)
{
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        if (EM_RTN(e, 1, 0xD)) {
            return 0;
        }
    }
    return 1;
}

extern "C" int em10PlRunCk(cEm10* em)
{
    Mtx m;
    Vec v;

    if (pPL->xFC != 0) {
        return 0;
    }
    if (pPL->xFD != 3) {
        return 0;
    }
    if (pG->x4F88 > 3) {
        if (fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
            return 0;
        }
        PSMTXInverse(pPL->mat, m);
        PSMTXMultVec(m, &em->pos, &v);
        if (v.x > 1500.0f) {
            return 0;
        }
        if (v.x < -1500.0f) {
            return 0;
        }
        return 1;
    }
    return 0;
}

extern "C" int em10SearchParasite(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;

    w->x58C = 0;
    for (i = 0; i < EmMgr.nArray; i++) {
        cEmPartner* e = (cEmPartner*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id != 0x25) {
            continue;
        }
        if (!e->v50()) {
            continue;
        }
        e->v58(em, 3, 0, 0);
        w->x58C = e;
        return 1;
    }
    return 0;
}

extern "C" void em10SackSet(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cModelInfo* info;

    if (w->wepType != 4) {
        return;
    }
    if (!w->mot[21] || !w->mot[22]) {
        return;
    }
    info = ModInfoMgr.create(w->mot[21], w->mot[22]);
    if (info) {
        if (w->x19C) {
            cModel_swapModelInfo(em, w->x19C->pData, info);
        } else {
            em->addModel(info);
        }
        w->x19C = info;
    }
    if ((u8) (em->type - 11) <= 1) {
        if (w->x18C) {
            w->x18C->be_flag &= ~8;
        }
        EstSetEm(em, -1, 0, 0, 0x10, 0x55, 0, 0, em, 0);
    }
}

void cEm10::setGoto(Vec* pos, int range)
{
    Em10Work* w = EM10_WK(this);
    f32 y;

    if (w->flags & 0x4000) {
        return;
    }
    w->x5EC = range;
    w->x5F0 = *pos;
    y = SatMgr.getFloor(&w->x5F0, 600.0f, 100000.0f, 0, 0);
    if (y != -100000.0f) {
        w->x5F0.y = y + 50.0f;
    }
    w->x4EC = *pos;
    w->flags |= 0x04000004;
    if (!(flags_3C8 & 0x40)) {
        w->flags &= ~0x100;
    }
    w->flags &= ~0x800000;
}

void cEm10::setLost()
{
    Em10Work* w = EM10_WK(this);

    atari.throughOn();
    EmSetDie(this);
    EmReserveDropItem(this);
    clearStatus(5);
    em10CoreBreak(this, 1);
    if (w->x58C) {
        w->x58C->setReset();
        w->x58C = 0;
    }
    w->x5D8 = 1.0f;
    if (w->pWep) {
        w->pWep->setLost();
        w->pWep = 0;
        w->wepType = 0;
    }
    if (w->pWep2) {
        w->pWep2->setLost();
        w->pWep2 = 0;
        w->wep2Type = 0;
    }
    be_flag &= ~2;
    w->flags |= 0x400000;
    w->x6B7 = 1;
}

static u8 em1f_cloth_parts[6] = { 0x22, 0x23, 0x24, 0x25, 0x26, 0x27 };
static u8 em1f_cloth_up[6] = { 0xFF, 0x22, 0x23, 0x24, 0x25, 0x26 };
static u8 em1f_cloth_down[6] = { 0x23, 0x24, 0x25, 0x26, 0x27, 0xFF };
static f32 em1f_cloth_max[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
PlClothAt em1f_cloth_at[1] = {
    { 0, 1, 1, 1.0f, 250.0f, { 0.0f, 0.0f, 100.0f }, { 0.0f, 0.0f, 0.0f } },
};

void Em1fClothSet(cModel* m, PlCloth* c)
{
    c->num = 6;
    c->pParts = em1f_cloth_parts;
    c->pLeft = 0;
    c->pRight = 0;
    c->pUpLeft = 0;
    c->x14 = 0;
    c->pUp = em1f_cloth_up;
    c->pDown = em1f_cloth_down;
    c->x20 = 0;
    c->pRate = 0;
    c->pMax = em1f_cloth_max;
    c->pWindS = 0;
    c->pWindR = 0;
    c->pAt = em1f_cloth_at;
    c->nAt = 1;
    c->x3C = 20.0f;
    c->x40 = 0.1f;
    c->x44 = 4;
    c->x48 = 0.0f;
    c->x4C = 0.05f;
    c->x50 = 0.0f;
    c->x54 = 0;
    c->pModel = m;
    c->flags = 0x100;
    PenClothSet(m, (PenCloth*) c, 100.0f);
}

extern "C" f32 em10GetPower(cEm10* em)
{
    f32 p = 1.0f;

    switch (em->type) {
    default:
        p = 1.0f;
        break;
    case 2:
        p = 1.0f;
        break;
    case 7:
        p = 1.1f;
        break;
    case 8:
        p = 1.5f;
        break;
    case 9:
        p = 1.3f;
        break;
    case 10:
        p = 1.0f;
        break;
    case 13:
        p = 1.0f;
        break;
    case 14:
        p = 1.6f;
        break;
    case 15:
        p = 1.6f;
        break;
    case 16:
        p = 1.6f;
        break;
    case 17:
        p = 1.6f;
        break;
    case 18:
        p = 1.6f;
        break;
    case 19:
        p = 1.6f;
        break;
    case 20:
        p = 1.6f;
        break;
    case 21:
        p = 1.6f;
        break;
    case 23:
        p = 1.6f;
        break;
    case 24:
        p = 1.6f;
        break;
    case 25:
        p = 1.6f;
        break;
    }
    return p;
}

extern "C" int em10BackCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int back = 0;

    if ((s16) w->x67C != 0) {
        back = 1;
    }
    if (Ctrl12Ck(w->pCtrl12, 9)) {
        back = 1;
    }
    if (!back) {
        return 0;
    }
    if (em->x3D0 == 3) {
        if (EM_RTN(em, 1, 0x1B)) {
            return 1;
        }
    } else if (em->plDist2 > 4000000.0f) {
        if (em->x3D0 == 2) {
            return 0;
        }
        if (EM_RTN(em, 1, 0x1B)) {
            return 0;
        }
    } else if (w->wepType != 4) {
        EmRoutineSet(em, 1, 0x12, 0, 0);
        return 1;
    }
    EmRoutineSet(em, 1, 0x1B, 0, 0);
    return 1;
}

static void plem10DmGondolaShake(cPlayer* pl)
{
    Em10Work* w = EM10_WK((cEm10*) pPL->dmgType);
    cEm* em = (cEm*) pl->dmgType;

    pl->subArc = em->subArc;
    pl->dmg.set(0, 5);
    switch (pl->xFE) {
    case 0:
        if (pl->xFF) {
            MotionSetCore(pl, MOTION(pl), w->evtMot[3], 0, 3, 5, 0);
        } else {
            MotionSetCore(pl, MOTION(pl), w->evtMot[3], 0, 3, 1, 0);
        }
        PlSetDamageSe(0);
        pl->xFE++;
    case 1:
        if (MotionMoveF(pl, 0) && !pl->xFF) {
            EndPlDamage();
        }
        break;
    }
    pl->subArc = pl->subArc2;
}

void cEm10::setGotoSwitch(cModel* sw, int near, Vec* pos)
{
    Em10Work* w = EM10_WK(this);
    Vec* dst;
    f32 y;

    if (w->flags & 0x4000) {
        return;
    }
    w->x5EC = near ? 3 : 4;
    if (pos) {
        dst = &w->x5F0;
        *dst = *pos;
    } else {
        dst = &w->x5F0;
        *dst = sw->pos;
    }
    y = SatMgr.getFloor(dst, 600.0f, 100000.0f, 0, 0);
    if (y != -100000.0f) {
        w->x5F0.y = y + 50.0f;
    }
    w->pSwitch = sw;
    w->x4EC = *dst;
    w->flags |= 0x04000004;
    if (!(flags_3C8 & 0x40)) {
        w->flags &= ~0x100;
    }
    w->flags &= ~0x800000;
}

extern "C" void em10BlendMotSet(cEm10* em, void* m0, void* m1, void* m2, int a, int b, int c, u16 d)
{
    Em10Work* w = EM10_WK(em);
    MotionWorkSub* bm;
    f32 rate = fabsf(w->blendRate);

    MotionSetCore(em, MOTION(em), m0, a, (u8) w->x740, d, (u16) w->x744);
    bm = &w->blendMot;
    if (w->blendRate < 0.0f) {
        MotionSetCore(em, bm, m1, b, (u8) w->x740, d, (u16) w->x744);
    } else {
        MotionSetCore(em, bm, m2, c, (u8) w->x740, d, (u16) w->x744);
    }
    em->blendMot = bm;
    bm->blendRate = rate * 0.00390625f;
    if (w->x740) {
        w->x740--;
    }
    w->x744++;
    if (w->x744 >= em->frameMax) {
        w->x744 = 0;
    }
}

extern "C" void em10FallWaterCk(cEm10* em)
{
    Vec hit;
    u32 attr;
    AtEffInfo* info;

    attr = EatMgr.hitCheck(&em->oldPos, &em->pos, &hit, 0, 0, 0);
    if (!attr) {
        return;
    }
    info = EatMgr.getEffInfo(EatGetEffectType(attr));
    if (!info) {
        return;
    }
    if (info->flags & 1) {
        if (pG->room_id == 0x311) {
            EstSet(0, -1, &em->pos, 0, 1, 3, 0, 0, 0, 0);
            SndCall(6, 0xA, &em->pos, 0, 0, em);
        } else {
            EstSetEm10WaterFall((Vec*) em);
            SndCall(6, 0x16, &em->pos, 0, 0, em);
        }
    }
}

int em10SomebodyNearCk(cEm10* em)
{
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        {
            f32 dx = em->pos.x - e->pos.x;
            f32 dy = em->pos.y - e->pos.y;
            f32 dz = em->pos.z - e->pos.z;
            if (dx * dx + dy * dy + dz * dz < 9000000.0f) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void em10BehindSeCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (Ctrl12Ck(w->pCtrl12, 0xC)) {
        return;
    }
    if ((s16) w->x67C != 0) {
        return;
    }
    if (em->plDist2 > 9000000.0f) {
        return;
    }
    if (w->x508 > 1.5707964f) {
        return;
    }
    if (w->wepType == 4) {
        return;
    }
    if (fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f)) < 1.5707964f) {
        return;
    }
    if (!(w->flags & 1)) {
        return;
    }
    em10CallVoiceSe2(em, w->se6D6, 8);
    Ctrl12Set(w->pCtrl12, 0xC, 300);
    w->x67E = (u8) (Rnd() % 120) + 120;
}

int em10WindowCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01010000 && (w->flags & 0x00800000)) {
        return 0;
    }
    switch ((u32) em10WindowCk2(em)) {
    case 0:
    default:
        return 0;
    case 1:
        EmRoutineSet(em, 1, 0x3C, 0, 0);
        return 1;
    case 2:
        EmRoutineSet(em, 1, 0x43, 0, 1);
        return 1;
    case 3:
        EmRoutineSet(em, 1, 0x3F, 0, 0);
        return 1;
    case 4:
        EmRoutineSet(em, 1, 0x3D, 0, 0);
        return 1;
    }
}

int em10ArmorCk(cEm10* em, int parts)
{
    Em10Work* w = EM10_WK(em);
    u8 armor = w->x6C5;

    if ((em->flags_3C8 & 0x200) && armor == 1 && parts == 5 && w->pParasite) {
        return 1;
    }
    if ((em->flags_3C8 & 0x200) && armor == 2 && parts == 5 && w->pParasite) {
        return 1;
    }
    switch (em->type) {
    case 10:
        switch ((u32) parts) {
        case 3:
        case 9:
        case 15:
        case 20:
        case 24:
            return 1;
        }
        return 0;
    case 13:
        if (parts == 0x25) {
            return 0;
        }
        return 1;
    case 24:
        switch ((u32) parts) {
        case 2:
        case 8:
        case 14:
        case 20:
        case 24:
            break;
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
    return 1;
}

void em10BowgunMove(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cModel* p;

    if (w->pWep && w->wepType == 8) {
        p = w->pWep->getPartsPtr(4);
        p->scale.x = 1.0f;
        p->scale.y = 1.0f;
        p->scale.z = 1.0f;
        p = w->pWep->getPartsPtr(1);
        if (w->x6B5 <= 2) {
            p->scale.x = 0.0f;
            p->scale.y = 0.0f;
            p->scale.z = 0.0f;
        } else {
            p->scale.x = 1.0f;
            p->scale.y = 1.0f;
            p->scale.z = 1.0f;
        }
        p = w->pWep->getPartsPtr(2);
        if (w->x6B5 <= 1) {
            p->scale.x = 0.0f;
            p->scale.y = 0.0f;
            p->scale.z = 0.0f;
        } else {
            p->scale.x = 1.0f;
            p->scale.y = 1.0f;
            p->scale.z = 1.0f;
        }
        p = w->pWep->getPartsPtr(3);
        if (w->x6B5 == 0) {
            p->scale.x = 0.0f;
            p->scale.y = 0.0f;
            p->scale.z = 0.0f;
        } else {
            p->scale.x = 1.0f;
            p->scale.y = 1.0f;
            p->scale.z = 1.0f;
        }
    }
}

int em10IgnitionCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cEmWep* wep;
    u8 type;

    if (!(w->flags & 0x100)) {
        return 0;
    }
    wep = w->pWep;
    if (!wep) {
        return 0;
    }
    type = w->wepType;
    if (type == 4 && !(w->flags & 0x80000000)) {
        EmRoutineSet(em, 1, 0xE, 0, 0);
        return 1;
    }
    if (wep && type == 9 && w->x640 == 0) {
        if (w->x524 < 15000.0f || em->x3D0 == 2) {
            if (w->flags & 1) {
                if (em->flags_3C8 & 0x10000) {
                    EmRoutineSet(em, 1, 0xF, 0, 0);
                    return 1;
                }
                if (em->pos.y > pPL->pos.y - 500.0f) {
                    EmRoutineSet(em, 1, 0xF, 0, 0);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void em10HeadSet(cEm10* em, int no)
{
    cModelInfo* info;
    void* bin;

    if (em->type == 10 || em->type == 13 || em->type == 2 || em->type == 22) {
        return;
    }
    Em10Work* w = EM10_WK(em);
    switch (no) {
    case 0:
    default:
        bin = w->mot[2];
        break;
    case 1:
        if (w->x6C5 == 1 && em->hp > 0 && (Rnd() & 3) && !(em->flags_3C8 & 0x204000)) {
            em->getPartsPtr(0x24)->rot.x = -0.6981317f;
            bin = w->mot[4];
        } else {
            bin = w->mot[3];
        }
        break;
    }
    info = ModInfoMgr.create(bin, w->mot[5]);
    if (info) {
        if (w->x18C) {
            cModel_swapModelInfo(em, w->x18C->pData, info);
        } else {
            em->addModel(info);
        }
        w->x18C = info;
        w->x18C->be_flag |= 8;
    }
}

extern "C" int em10ShotRocketCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int near;

    if (w->wepType != 0xC) {
        return 0;
    }
    if (!w->pWep) {
        return 0;
    }
    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000) {
        return 0;
    }
    if (w->x58C) {
        return 0;
    }
    if (w->pParasite) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    if (!em10ThrowScaCk(em)) {
        return 0;
    }
    if (w->x696 == 0 && !(w->flags & 0x100)) {
        return 0;
    }
    near = em10ThrowNearCk(em);
    if (near) {
        em->xFE = 0;
        em->xFC = 1;
        em->xFD = 0x17;
        em->xFF = Rnd() & 1;
        return 1;
    }
    if (em->plDist2 > 900000000.0f) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x22, 0, 0);
    return 1;
}

extern "C" int em10GetWanderRouteEmi(cEm10* em)
{
    EmiData* emi = (EmiData*) pG->pRoomEmi;
    int cnt;
    int i;
    int r;

    if (!emi) {
        return -1;
    }
    cnt = 0;
    for (i = 0; i < emi->n; i++) {
        if (emi->entry[i].type != 1) {
            continue;
        }
        if (emi->entry[i].sub != 3) {
            continue;
        }
        cnt++;
    }
    if (cnt == 0) {
        return -1;
    }
    r = Rnd() % cnt;
    cnt = 0;
    emi = (EmiData*) pG->pRoomEmi;
    for (i = 0; i < emi->n; i++) {
        if (emi->entry[i].type != 1) {
            continue;
        }
        if (emi->entry[i].sub != 3) {
            continue;
        }
        if (r == cnt) {
            return i;
        }
        cnt++;
    }
    return -1;
}

extern "C" int em10DootAtkCk(cEm10* em)
{
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        if (e->xFC != 1) {
            continue;
        }
        if (e->xFD != 0x3D && e->xFD != 0x3F) {
            continue;
        }
        {
            f32 dx = em->pos.x - e->pos.x;
            f32 dy = em->pos.y - e->pos.y;
            f32 dz = em->pos.z - e->pos.z;
            if (dx * dx + dy * dy + dz * dz > 9000000.0f) {
                continue;
            }
        }
        return 0;
    }
    return 1;
}

void em10FindNotify(cEm10* em)
{
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        {
            f32 dx = em->pos.x - e->pos.x;
            f32 dz = em->pos.z - e->pos.z;
            f32 d = dx * dx + dz * dz;
            if (em->flags_3C8 & 0x10) {
                if (d > 100000000.0f) {
                    continue;
                }
            } else {
                if (d > 625000000.0f) {
                    continue;
                }
            }
        }
        em->setFindPL();
    }
}

extern "C" int em10ScreenInCk(cEm10* em)
{
    Vec scr;
    Vec pos;

    pos = em->pos;
    if (GetScreenPos(&pos, &scr) && scr.x > 0.0f && scr.x < 512.0f && scr.y > 0.0f && scr.y < 448.0f) {
        return 1;
    }
    pos = em->getPartsPtr(4)->worldPos;
    if (GetScreenPos(&pos, &scr) && scr.x > 0.0f && scr.x < 512.0f && scr.y > 0.0f && scr.y < 448.0f) {
        return 1;
    }
    return 0;
}

extern "C" int em10SetWanderRoute(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec pos;
    Vec p2;
    int n;

    n = em10WanderRouteUpdate(em, w->x63C);
    w->x63C = n;
    if (n < 0) {
        return 0;
    }
    em10GetWanderRoutePos(em, &pos);
    RouteCkToPos(em, &pos, &w->x54C, 0, 0);
    w->x518 = Muku(&em->pos, &w->x54C, em->rot.y, 3.1415927f);
    w->x51C = fabsf(w->x518);
    w->x520 = (em->pos.x - w->x54C.x) * (em->pos.x - w->x54C.x) + (em->pos.z - w->x54C.z) * (em->pos.z - w->x54C.z);
    if (pG->flags_60 & 0x4000) {
        p2 = em->pos;
        p2.y += 250.0f;
        Draw_line3d(&p2, &pos, 0xFFFF0000, 0);
        Draw_line3d(&p2, &w->x54C, 0xFFFFFF40, 0);
    }
    return 1;
}

int em10ModelInit(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (!em->modelInit(w->mot[1], w->mot[0])) {
        pLog->err(0, 0, "EM10 pEm->modelInit() failed.");
        return 0;
    }
    w->x190 = 0;
    w->x194 = 0;
    w->x198 = 0;
    if (em->type == 6) {
        if (w->mot[16] && w->mot[17]) {
            w->x190 = ModInfoMgr.create(w->mot[16], w->mot[17]);
            if (w->x190) {
                em->addModel(w->x190);
            }
        }
        em10ClothPartsSet(em, 0);
        em10GoodsPartsSet(em, 0);
    }
    w->x18C = 0;
    em10HeadSet(em, 0);
    w->x184 = 0;
    w->x188 = 0;
    em10HandSet(em, 0);
    em->pFootShadowTbl = &Em10_fs_tbl;
    w->pHead = 0;
    em10SetAccesory(em);
    em10WeaponInit(em);
    em10ShieldSet(em);
    em10SackSet(em);
    return 1;
}

void em10SetWaterEff(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x00400000) {
        return;
    }
    if (!CheckInWater(em, 4)) {
        return;
    }
    if (w->x6A1) {
        w->x6A1--;
    } else {
        w->x6A1 = 8;
        EstSet((int) em, -1, 0, 0, 1, 0x30, 0, 0, (u32) em, 0);
    }
    if ((em->pos.x - em->oldPos.x) * (em->pos.x - em->oldPos.x) + (em->pos.z - em->oldPos.z) * (em->pos.z - em->oldPos.z) > 225.0f) {
        if (w->x6A2) {
            w->x6A2--;
        } else {
            w->x6A2 = 5;
            EstSet((int) em, -1, 0, 0, 1, 0x31, 0, 0, (u32) em, 0);
            SndCall(6, 0x11, &em->pos, 0, 0, em);
        }
    }
}

extern "C" void em10CamMoveTakeaway(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    GlobalWork* g = pG;
    Vec a;
    Vec b;

    a.x = 250.0f;
    a.y = 579.0f;
    a.z = -2530.0f;
    b.x = 0.0f;
    b.y = 1075.0f;
    b.z = 0.0f;
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    PosToPos(&g->Cam.param.pos, &a, &w->cam.param.pos, 1.0f);
    PosToPos(&g->Cam.param.at, &b, &w->cam.param.at, 1.0f);
    w->cam.up.x = 0.0f;
    w->cam.up.y = 1.0f;
    w->cam.up.z = 0.0f;
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        w->cam.dist = SQRTF(dx * dx + dy * dy + dz * dz);
    }
    w->cam.param.fovy = 55.0f;
    CameraSetOrientationUp(&w->cam);
    CamCtrl.x250 = (s32) &w->cam;
}

extern "C" int em10ReturnCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->flags & 0x20000000) {
        return 0;
    }
    if (em->x3D0 != 1 && em->x3D0 != 3) {
        return 0;
    }
    if (em->hp < em->hpMax / 2) {
        return 0;
    }
    if (w->x524 < 3000.0f) {
        return 0;
    }
    if (w->x52C < em->x3CC) {
        return 0;
    }
    {
        f32 dx = em->pos.x - w->x4D8.x;
        f32 dy = em->pos.y - w->x4D8.y;
        f32 dz = em->pos.z - w->x4D8.z;
        if (dx * dx + dy * dy + dz * dz < 6250000.0f) {
            if (EM_RTN(em, 1, 0x1B)) {
                return 0;
            }
            EmRoutineSet(em, 1, 0x1B, 0, 0);
            return 1;
        }
    }
    w->flags |= 0x20000000;
    w->x4F8 = em->pos;
    EmRoutineSet(em, 1, 0x1B, 0, 0);
    return 1;
}

extern "C" int em10BombThrowScaCk(cEm10* em)
{
    Vec a;
    Vec b;

    a.x = 300.0f;
    a.y = 2000.0f;
    a.z = 0.0f;
    b.x = 300.0f;
    b.y = 2000.0f;
    b.z = 5000.0f;
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0x404000)) {
        return 0;
    }
    a.x = -300.0f;
    a.y = 2000.0f;
    a.z = 0.0f;
    b.x = -300.0f;
    b.y = 2000.0f;
    b.z = 5000.0f;
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    return EatMgr.hitCheck(&a, &b, 0, 0, 0, 0x404000) == 0;
}

extern "C" void em10CsawSignSe(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if ((int) w->flags >= 0) {
        return;
    }
    if (w->wepType != 4) {
        return;
    }
    if ((s16) pG->pl_life <= 0) {
        return;
    }
    if (em->plDist2 > 25000000.0f) {
        if (w->x65C) {
            w->x65C--;
            return;
        }
        w->x65C = (u8) (Rnd() % 150) + 150;
        w->x65E = 150;
        w->x5C4 = SndCall(6, 0x4B, &em->pos, 0, 0, em);
    } else {
        if (w->x65E != 0) {
            return;
        }
        SndStop(w->x5C4, 0);
        w->x5C4 = SndCall(6, 0x3E, &em->pos, 0, 0, em);
        w->x65E = 150;
        w->x65C = (u8) (Rnd() % 150) + 150;
    }
}

extern "C" int em10ShotBowgunCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int near;

    if (w->wepType != 8) {
        return 0;
    }
    if (!w->pWep) {
        return 0;
    }
    if (em->flags_3C8 & 0x40) {
        em->setWeaponFall();
        return 0;
    }
    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000) {
        return 0;
    }
    if (w->x58C) {
        return 0;
    }
    if (w->pParasite) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    if (!em10ThrowScaCk(em)) {
        return 0;
    }
    if (w->x696 == 0 && !(w->flags & 0x100)) {
        return 0;
    }
    near = em10ThrowNearCk(em);
    if (near) {
        em->xFC = 1;
        em->xFD = 0x17;
        em->xFE = 0;
        em->xFF = Rnd() & 1;
        return 1;
    }
    if (em->plDist2 > 900000000.0f) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x21, 0, 0);
    return 1;
}

// Parts (cModel-shaped) fields model.h does not name: the rotation offset Vec at 0x128 and the flag word at 0x1C0.
#define PARTS_ROT_OFS(p) (*(Vec*) ((u8*) (p) + 0x128))
#define PARTS_FLAGS(p) (*(u32*) ((u8*) (p) + 0x1C0))

void em10WaistMove(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    f32 r;
    cModel* p;

    if (w->flags & 0x00400000) {
        return;
    }
    if (w->flags & 0x200) {
        if (em->flags_3C8 & 0x01000000) {
            v.x = 300.0f;
            v.y = 0.0f;
            v.z = 0.0f;
        } else {
            v.x = -300.0f;
            v.y = 0.0f;
            v.z = 0.0f;
        }
        PSMTXMultVec(em->mat, &v, &v);
        w->x5D4 = w->x5D4 * 0.9f + Muku(&v, &pPL->pos, em->rot.y, 0.7853982f) * 0.1f;
    } else {
        w->x5D4 = w->x5D4 * 0.9f;
    }
    r = w->x5D4 * 0.5f;
    p = em->getPartsPtr(1);
    PARTS_FLAGS(p) |= 0x40000000;
    PARTS_ROT_OFS(p).x = 0.0f;
    PARTS_ROT_OFS(p).y = r;
    PARTS_ROT_OFS(p).z = 0.0f;
    p = em->getPartsPtr(2);
    PARTS_FLAGS(p) |= 0x40000000;
    PARTS_ROT_OFS(p).x = 0.0f;
    PARTS_ROT_OFS(p).y = r;
    PARTS_ROT_OFS(p).z = 0.0f;
}

extern "C" int em10TorchFrameAtkCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx m;
    Vec v;

    if ((s16) pG->pl_life <= 0) {
        return 0;
    }
    if (em10DeadCk(pPL)) {
        return 0;
    }
    if (w->x697) {
        return 0;
    }
    PSMTXInverse(em->mat, m);
    PSMTXMultVec(m, &pPL->pos, &v);
    if (!(v.x > 500.0f) && !(v.x < -500.0f) && !(v.z > 3500.0f) && !(v.z < 0.0f) && !(v.y > 1500.0f) && !(v.y < -500.0f)) {
        w->x697 = 1;
        SndCall(8, 0x8F, &pPL->pos, em->id, 0, pPL);
        Ctrl12Set(w->pCtrl12, 9, 30);
        SetPlDamage((int) em, plemDmFrame);
        pPL->dmg.set(0, 30);
        return 1;
    }
    return 0;
}

extern "C" int em10ThreatCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx m;
    Vec v;

    if (pPL->xFC != 0) {
        return 0;
    }
    if (pPL->xFD != 3) {
        return 0;
    }
    if (w->pShield) {
        return 0;
    }
    if (w->pParasite) {
        return 0;
    }
    if (w->x58C) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    if (fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
        return 0;
    }
    PSMTXInverse(pPL->mat, m);
    PSMTXMultVec(m, &em->pos, &v);
    if (v.x > 3000.0f || v.x < -3000.0f) {
        return 0;
    }
    if (v.x > -500.0f && v.x < 500.0f) {
        return 0;
    }
    if (!(v.z < 1500.0f) && !(v.z > 5000.0f)) {
        EmRoutineSet(em, 1, 0x16, 0, 0);
        return 1;
    }
    return 0;
}

int em10ClawStickCK(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int r;

    if (em->type != 10 && em->type != 13) {
        return 0;
    }
    if (!(w->flags & 0x100)) {
        if (!em10FindCk2(em)) {
            return 0;
        }
        em->setFindPL();
        w->x674 = 0;
    }
    if (w->x6BE == 2 && w->x6BF == 2) {
        if (!em10FindCk2(em)) {
            return 0;
        }
        if (w->flags & 0x08000000) {
            return 0;
        }
        r = em10ClawCriAtkCk(em);
        if (r) {
            return 1;
        }
        if ((em->pos.x - w->x534.x) * (em->pos.x - w->x534.x) + (em->pos.z - w->x534.z) * (em->pos.z - w->x534.z) > 16000000.0f &&
            !EM_RTN(em, 1, 0x11)) {
            em10CallVoiceSe2(em, 0x71, 6);
            w->x6AC = 0;
            EmRoutineSet(em, 1, 0x11, 0, 0);
            return 1;
        }
        return 0;
    }
    r = em10ClawCriAtkCk(em);
    if (r) {
        return 1;
    }
    EmRoutineSet(em, 1, 0x59, 0, 0);
    return 1;
}

int em10LadderResetCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cObjLadder* o;
    Vec v;

    if (w->x54C.y - em->pos.y < 1000.0f) {
        return 0;
    }
    for (o = (cObjLadder*) ObjMgr.pAlive; o; o = (cObjLadder*) o->next) {
        f32 d;
        if (o->id != 0x13) {
            continue;
        }
        if (!o->ckReset()) {
            continue;
        }
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 1432.0f;
        PSMTXMultVec(o->mat, &v, &v);
        d = (em->pos.x - v.x) * (em->pos.x - v.x) + (em->pos.z - v.z) * (em->pos.z - v.z);
        if (d > 1210000.0f) {
            if (d < 9000000.0f) {
                em->setGoto(&v, 9);
                return 1;
            }
        } else {
            w->pLadder = o;
            o->setResetReserve();
            EmRoutineSet(em, 1, 0x42, 0, 0);
            return 1;
        }
    }
    return 0;
}

extern "C" void em10SetTakeawayPosUpdate(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;
    u32 j;
    f32 d;
    f32 dy;

    if (!pG->pRoomEmi) {
        return;
    }
    for (i = 0; i < ((EmiData*) pG->pRoomEmi)->n; i++) {
        EmiEntry* e = &((EmiData*) pG->pRoomEmi)->entry[i];
        if (e->type != 5) {
            continue;
        }
        if (e->sub == 0) {
            continue;
        }
        if (e->sub != 1) {
            continue;
        }
        d = (em->pos.x - e->pos.x) * (em->pos.x - e->pos.x) + (em->pos.z - e->pos.z) * (em->pos.z - e->pos.z);
        dy = fabsf(em->pos.y - e->pos.y);
        if (!(d < 16000000.0f && dy < 1000.0f)) {
            continue;
        }
        for (j = 0; j < ((EmiData*) pG->pRoomEmi)->n; j++) {
            EmiEntry* e2 = &((EmiData*) pG->pRoomEmi)->entry[j];
            if (e2->type != 5) {
                continue;
            }
            if (e2->sub != 0) {
                continue;
            }
            if (e2->state != e->state) {
                continue;
            }
            w->x4EC = e2->pos;
            return;
        }
    }
}

extern "C" void em10BellAtkCk(cEm10* em, Vec* pos, u32 no)
{
    EmAtkInfo info = Em10AtkTbl[no];
    cObjBell* o;
    Vec v;
    cModel* p;

    switch (no) {
    case 0xD:
    case 0xE:
        for (o = (cObjBell*) ObjMgr.pAlive; o; o = (cObjBell*) o->next) {
            if (o->id != 0x14) {
                continue;
            }
            if (!o->ckBreakEnable()) {
                continue;
            }
            p = o->getPartsPtr(1);
            v.x = 0.0f;
            v.y = -650.0f;
            v.z = 0.0f;
            PSMTXMultVec(p->mat, &v, &v);
            {
                f32 dx = v.x - pos->x;
                f32 dy = v.y - pos->y;
                f32 dz = v.z - pos->z;
                if (dx * dx + dy * dy + dz * dz < (info.range + 300.0f) * (info.range + 300.0f)) {
                    o->setBreak();
                    SndCall(6, 0xF, &em->pos, 0, 0, em);
                }
            }
        }
        break;
    }
}

void em10SetPoint(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int pt;

    if (w->x6C4) {
        return;
    }
    w->x6C4 = 1;
    switch (em->type) {
    case 0:
        pt = 0;
        break;
    case 1:
        pt = 0;
        break;
    case 2:
        pt = 8;
        break;
    case 3:
        pt = 0;
        break;
    case 4:
        pt = 0;
        break;
    default:
        pt = 0;
        break;
    case 5:
        pt = 4;
        break;
    case 6:
        pt = 4;
        break;
    case 7:
        pt = 3;
        break;
    case 8:
        pt = 4;
        break;
    case 9:
        pt = 3;
        break;
    case 0xa:
        pt = 5;
        break;
    case 0xb:
        pt = 1;
        break;
    case 0xc:
        pt = 1;
        break;
    case 0xd:
        pt = 5;
        break;
    case 0xe:
        pt = 6;
        break;
    case 0xf:
        pt = 6;
        break;
    case 0x10:
        pt = 6;
        break;
    case 0x11:
        pt = 6;
        break;
    case 0x12:
        pt = 6;
        break;
    case 0x13:
        pt = 6;
        break;
    case 0x14:
        pt = 6;
        break;
    case 0x15:
        pt = 6;
        break;
    case 0x16:
        pt = 2;
        break;
    case 0x17:
        pt = 6;
        break;
    case 0x18:
        pt = 6;
        break;
    case 0x19:
        pt = 6;
        break;
    }
    if (em->flags_3C8 & 0x10000000) {
        pt = 2;
    }
    MercSysSetPoint(pt, 0);
}

void cEm10::setHand(int no, int type)
{
    Em10Work* w = EM10_WK(this);
    void* tpl;
    void* bin;
    cModelInfo* info;

    if (type == 10 || type == 13 || type == 2 || type == 22 || type == 24) {
        return;
    }
    if (w->x184 && w->x188 && w->x6AD == type) {
        return;
    }
    switch ((u32) type) {
    case 0:
    default:
        bin = w->mot[6];
        tpl = w->mot[11];
        break;
    case 1:
        bin = w->mot[7];
        tpl = w->mot[12];
        break;
    case 2:
        bin = w->mot[8];
        tpl = w->mot[13];
        break;
    case 3:
        tpl = w->mot[14];
        bin = w->mot[9];
        if (w->wepType == 6) {
            bin = w->mot[10];
        }
        break;
    }
    if (no) {
        info = ModInfoMgr.create(bin, w->mot[0]);
        if (!info) {
            return;
        }
        if (w->x184) {
            cModel_swapModelInfo(this, w->x184->pData, info);
        } else {
            addModel(info);
        }
        w->x184 = info;
    } else {
        info = ModInfoMgr.create(tpl, w->mot[0]);
        if (!info) {
            return;
        }
        if (w->x188) {
            cModel_swapModelInfo(this, w->x188->pData, info);
        } else {
            addModel(info);
        }
        w->x188 = info;
    }
}

extern "C" int em10ShotGatlingCk(cEm10* em)
{
    Vec pos;
    Vec d;
    f32 ang;
    int hit;

    if (em->type != 2) {
        return 0;
    }
    if (!em10ThrowScaCk(em)) {
        return 0;
    }
    if (em->plDist2 > 900000000.0f) {
        return 0;
    }
    if (fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
        return 0;
    }
    PSVECSubtract(&pPL->pos, &em->pos, &d);
    ang = -atan2f(d.y, SQRTF(d.x * d.x + d.z * d.z));
    if (fabsf(ang) > 0.43633232f) {
        return 0;
    }
    pos = pPL->pos;
    pos.y += 1200.0f;
    hit = EatMgr.hitCheck(&em->getPartsPtr(10)->worldPos, &pos, 0, 0, 0, 0x404000);
    if (hit) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x23, 0, 0);
    return 1;
}

static u8 em10_belt_parts[8] = { 2, 3, 4, 5, 6, 7, 8, 9 };
static u8 em10_belt_up[8] = { 0xFF, 2, 3, 4, 5, 6, 7, 8 };
static u8 em10_belt_down[8] = { 3, 4, 5, 6, 7, 8, 9, 0xFF };
static f32 em10_belt_max[8] = { 0.3f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f };

void em10BeltSet(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec pos;
    Vec rot;

    if (em->type != 2) {
        return;
    }
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 0.0f;
    w->x590 = (cModel*) SetChain(PL_ARC_PTR(em->subArc, 0x22D), PL_ARC_PTR(em->subArc, 0x22E), &pos, &rot);
    if (!w->x590) {
        pLog->err(0, 0, "EM10 em10BeltSet SetChain failed.");
        return;
    }
    w->cloth.x58 = em;
    w->cloth.num = 8;
    w->cloth.pParts = em10_belt_parts;
    w->cloth.x08 = 0;
    w->cloth.x0C = 0;
    w->cloth.x10 = 0;
    w->cloth.x14 = 0;
    w->cloth.pUp = em10_belt_up;
    w->cloth.pDown = em10_belt_down;
    w->cloth.pMax = em10_belt_max;
    w->cloth.x2C = 0;
    w->cloth.x30 = 0;
    w->cloth.x34 = 0;
    w->cloth.x20 = 0;
    w->cloth.x24 = 0;
    w->cloth.x38 = 0;
    w->cloth.x3C = 25.0f;
    w->cloth.x40 = 0.9f;
    w->cloth.x44 = 10;
    w->cloth.x48 = 0.0f;
    w->cloth.x4C = 0.1f;
    w->cloth.x50 = 0.0f;
    w->cloth.flags = 0;
    w->cloth.x54 = 0;
    ((cObjChain*) w->x590)->setChain(&w->cloth);
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    ((cObjChain*) w->x590)->setParent(em, 0x23, &pos, 1);
}

extern "C" int em10ThrowScaCk(cEm10* em)
{
    Vec a;
    Vec b;
    Vec d;

    a.x = em->pos.x;
    a.y = em->pos.y + 1500.0f;
    a.z = em->pos.z;
    b.x = pPL->pos.x;
    b.y = pPL->pos.y + 1500.0f;
    b.z = pPL->pos.z;
    if ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z) > 25000000.0f) {
        PSVECSubtract(&b, &a, &d);
#line 38900 "D:/Bio4/Prog/em10.cpp"
        VECNormalize(&d, &d);
        PSVECScale(&d, &d, 5000.0f);
        PSVECAdd(&a, &d, &b);
    }
    return EatMgr.hitCheck(&a, &b, 0, 0, 0, 0x404000) == 0;
}

static u8 em10_chain_parts[4] = { 1, 2, 3, 4 };
static u8 em10_chain_up[4] = { 0xFF, 1, 2, 3 };
static u8 em10_chain_down[4] = { 2, 3, 4, 0xFF };
PlClothAt em10_chain_at[5] = {
    { 0, 1, 1, 1.0f, 200.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
    { 0, 1, 2, 0.5f, 200.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
    { 0, 2, 2, 1.0f, 200.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
    { 0, 2, 3, 0.5f, 170.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
    { 0, 3, 3, 1.0f, 150.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
};

void em10ChainSet(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec pos;
    Vec rot;

    if (em->type != 10 && em->type != 13) {
        return;
    }
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 0.0f;
    w->x594 = (cModel*) SetChain(PL_ARC_PTR(em->subArc, 0x225), PL_ARC_PTR(em->subArc, 0x226), &pos, &rot);
    if (!w->x594) {
        pLog->err(0, 0, "EM10 em10ChainSet SetChain failed.");
        return;
    }
    w->cloth.x58 = em;
    w->cloth.num = 4;
    w->cloth.pParts = em10_chain_parts;
    w->cloth.x08 = 0;
    w->cloth.x0C = 0;
    w->cloth.x10 = 0;
    w->cloth.x14 = 0;
    w->cloth.pUp = em10_chain_up;
    w->cloth.pDown = em10_chain_down;
    w->cloth.pMax = 0;
    w->cloth.x2C = 0;
    w->cloth.x30 = 0;
    w->cloth.x20 = 0;
    w->cloth.x24 = 0;
    w->cloth.x34 = em10_chain_at;
    w->cloth.x38 = 5;
    w->cloth.x3C = 25.0f;
    w->cloth.x40 = 0.9f;
    w->cloth.x44 = 10;
    w->cloth.x48 = 0.0f;
    w->cloth.x4C = 0.1f;
    w->cloth.x50 = 0.0f;
    w->cloth.flags = 0;
    w->cloth.x54 = 0;
    ((cObjChain*) w->x594)->setChain(&w->cloth);
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    ((cObjChain*) w->x594)->setParent(em, 0x25, &pos, 1);
}

static void em10_R1_StayWalk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int r;

    switch (em->xFE) {
    case 0:
        em10SetWaitMotion(em, 0);
        w->x4 = 600;
        if (w->wepType == 4) {
            if (em->type == 0x16) {
                EstSet((int) w->pWep, -1, 0, 0, 0x10, 0x96, 0, w->x69C, (u32) w->pWep, 0);
            } else {
                EstSet((int) w->pWep, -1, 0, 0, 0x10, 9, 0, w->x69C, (u32) w->pWep, 0);
            }
            w->flags |= 0x80000000;
            w->x65C = (u8) (Rnd() % 150) + 150;
            w->x684 = 60;
        }
        em->xFE++;
    case 1:
        MotionMoveF(em, 0);
        r = em10GotoCk(em);
        if (r) {
            break;
        }
        if (w->flags & 1) {
            w->x4 = r;
        }
        if (em->flags_3C8 & 1) {
            w->x4 = r;
        }
        if (w->x4) {
            w->x4--;
        } else {
            em->setFindPL();
            em10WalkRtnSet(em);
        }
        break;
    }
    em10HandSet(em, 0);
}

extern "C" int em10ThrowNearCk(cEm10* em)
{
    Mtx m;
    Vec v;
    u32 i;

    PSMTXRotRad(m, 'y', GetXZAngle(&em->pos, &pPL->pos));
    TransMatrix(m, &em->pos);
    PSMTXInverse(m, m);
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        PSMTXMultVec(m, &e->pos, &v);
        if (em->plDist2 < v.z * v.z) {
            continue;
        }
        if (v.z < 0.0f) {
            continue;
        }
        if (v.y > 1500.0f) {
            continue;
        }
        if (v.y < -1500.0f) {
            continue;
        }
        if (v.x > 250.0f) {
            continue;
        }
        if (v.x < -250.0f) {
            continue;
        }
        return 1;
    }
    return 0;
}

extern "C" void em10ShieldSet(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cEmShield* s;
    Vec pos;
    Vec rot;

    w->pShield = 0;
    if (w->x6C5 == 0) {
        return;
    }
    if ((int) em->flags_3C8 >= 0) {
        return;
    }
    if (em->type == 10 || em->type == 13 || em->type == 2 || em->type == 22 || em->type == 24) {
        return;
    }
    pos.x = pos.y = pos.z = 0.0f;
    rot.x = rot.y = rot.z = 0.0f;
    s = SetShield(PL_ARC_PTR(em->subArc, 0x163), PL_ARC_PTR(em->subArc, 0x164), &pos, &rot);
    if (!s) {
        return;
    }
    pos.x = -237.0f;
    pos.y = 0.02f;
    pos.z = 14.0f;
    rot.x = 0.0f;
    rot.y = -1.5707964f;
    rot.z = 0.0f;
    if (em->flags_3C8 & 0x01000000) {
        pos.x = pos.x * -1.0f;
        rot.y = 1.5707964f;
    }
    s->pos = pos;
    s->rot = rot;
    if (em->flags_3C8 & 0x01000000) {
        s->setParent(em, 0xA, 0);
    } else {
        s->setParent(em, 0x10, 0);
    }
    w->pShield = s;
}

void cEm10::setWeaponFall()
{
    Em10Work* w = EM10_WK(this);
    u8 wtype;

    if (!w->pWep) {
        return;
    }
    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000 && hp > 0) {
        return;
    }
    if (type == 6) {
        return;
    }
    wtype = w->wepType;
    if (wtype == 9) {
        return;
    }
    if (!(w->flags & 0x80) && !(flags_3C8 & 0x40)) {
        if (wtype == 8) {
            return;
        }
        if (type == 0x18) {
            return;
        }
    }
    switch (wtype) {
    case 6:
    case 7:
    case 0xB:
    case 0xF:
    case 0x10:
        if (hp > 300) {
            return;
        }
        break;
    }
    EffectEspDelete(0, w->x69E, (u32) this, 0);
    EffectEspgenDelete(0, w->x69E, (int) this);
    EffectEfmDelete(0, w->x69E, (int) this);
    if (w->wepType == 4) {
        if (hp > 0) {
            return;
        }
        if ((int) w->flags >= 0) {
            return;
        }
        EffectEspDelete(0, w->x69C, (u32) w->pWep, 0);
        EffectEspgenDelete(0, w->x69C, (int) w->pWep);
        EffectEfmDelete(0, w->x69C, (int) w->pWep);
        w->flags &= 0x7FFFFFFF;
    } else {
        w->pWep->setFall(0, 0, 20.0f);
        w->pWep = 0;
        w->wepType = 0;
    }
}

extern "C" int em10GoSubStayCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 cnt;
    u32 i;
    f32 d;

    if (!(w->flags & 0x08000000)) {
        return 0;
    }
    cnt = 0;
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        Em10Work* ew;
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        ew = EM10_WK(e);
        if (!(ew->flags & 0x08000000)) {
            continue;
        }
        if (ew->x528 >= w->x528) {
            continue;
        }
        cnt++;
    }
    if (pG->x4F88 <= 3) {
        if (cnt <= 1) {
            return 0;
        }
    } else if (cnt <= 3) {
        return 0;
    }
    d = w->x528;
    if (em->x3D0 == 2) {
        f32 lim = pG->x4F88 <= 3 ? 6000.0f : 4000.0f;
        if (d > lim) {
            return 0;
        }
    }
    if (cnt <= 7) {
        if (d > 8000.0f) {
            return 0;
        }
    }
    if (d > 12000.0f) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x1B, 0, 0);
    return 1;
}
void em10SetWaitMotion(cEm10* em, int a)
{
    Em10Work* w = EM10_WK(em);
    int flag = (em->flags_3C8 & 0x01000000) ? 0x45 : 5;
    void* mot = PL_ARC_PTR(em->subArc, 5);
    u32 n;
    int r;

    if (w->pWep) {
        if (w->wepType == 1) {
            mot = PL_ARC_PTR(em->subArc, 0x14A);
        }
        if (w->wepType == 4) {
            mot = PL_ARC_PTR(em->subArc, 0xE6);
        }
        if (w->wepType == 11) {
            mot = PL_ARC_PTR(em->subArc, 0xD7);
        }
        if (w->wepType == 7) {
            mot = PL_ARC_PTR(em->subArc, 0xD7);
        }
        if (w->wepType == 9) {
            mot = PL_ARC_PTR(em->subArc, 0xD7);
        }
        if (w->wepType == 6) {
            mot = PL_ARC_PTR(em->subArc, 0x13A);
        }
        if (w->wepType == 12) {
            mot = PL_ARC_PTR(em->subArc, 0x17C);
        }
    }
    if (w->pShield) {
        mot = PL_ARC_PTR(em->subArc, 0x165);
    }
    if (em->type == 6) {
        mot = PL_ARC_PTR(em->subArc, 0xA4);
    }
    if (em->type == 10) {
        mot = PL_ARC_PTR(em->subArc, 0x108);
    }
    if (em->type == 13) {
        mot = PL_ARC_PTR(em->subArc, 0x108);
    }
    if (em->type == 2) {
        mot = PL_ARC_PTR(em->subArc, 0x18F);
    }
    n = ((MotionData*) mot)->maxFrame & 0x3FFF;
    r = Rnd();
    MotionSetCore(em, MOTION(em), mot, 0, (u8) a, flag, (u16) (r % n));
}

int em10HideToStepCk(cEm10* em, int a)
{
    Vec v;
    Vec p;
    int hit;

    if ((pG->flags_51E4 & 7) != (em->emsetNo & 7)) {
        return 0;
    }
    if (a) {
        v.x = -2000.0f;
        v.y = 2000.0f;
        v.z = 0.0f;
        PSMTXMultVec(em->mat, &v, &v);
    } else {
        v.x = -2000.0f;
        v.y = 2000.0f;
        v.z = 0.0f;
        PSMTXMultVec(em->mat, &v, &v);
    }
    if (fabsf(Muku(&v, &pPL->pos, em->rot.y, 3.1415927f)) > 1.5707964f) {
        return 0;
    }
    {
        f32 dz = v.z - pPL->pos.z;
        f32 dx = v.x - pPL->pos.x;
        if (dx * dx + dz * dz > 169000000.0f) {
            return 0;
        }
    }
    p = pPL->pos;
    p.y += 2000.0f;
    hit = EatMgr.hitCheck(&v, &p, 0, 0, 0, 0);
    if (hit) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x17, hit, a);
    return 1;
}

extern "C" int em10RouteTargetSet(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (!pSUB) {
        return 0;
    }
    if (w->flags & 0x8000) {
        return 0;
    }
    if (w->flags & 0x04000000) {
        return 0;
    }
    if (pG->room_id != 0x30F && em->xFC == 1) {
        if (em->xFD == 0 || em->xFD == 0x1B || em->xFD == 1) {
            return 0;
        }
    }
    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01010000 || (G_ROOM_ID32 & 0xFFFF0000) == 0x01110000 ||
        (G_ROOM_ID32 & 0xFFFF0000) == 0x04000000) {
        if (pSUB->pos.y > 6000.0f) {
            return 0;
        }
    }
    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x030F0000 && w->wepType == 0) {
        return 0;
    }
    if (w->wepType == 8) {
        return 0;
    }
    if (em->type == 6) {
        return 0;
    }
    if (pG->flags_500C & 0x800) {
        return 0;
    }
    if (pG->flags_5010 & 0x10000) {
        return 0;
    }
    if (pG->flags_5010 & 8) {
        return 0;
    }
    if (em->flags_3C8 & 0x40) {
        return 1;
    }
    if (em->type == 10) {
        return 0;
    }
    if (em->type == 13) {
        return 0;
    }
    if ((u32) em10GetGoSub(em) > 1) {
        return 0;
    }
    if (w->flags & 0x08000000) {
        return 1;
    }
    if (w->x524 < w->x528 + 2000.0f) {
        return 0;
    }
    return 1;
}

extern "C" int em10SomebodyDamageNowCk(cEm10* em)
{
    u32 i;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        int dm;
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        dm = e->xFC == 2;
        if (e->hp <= 0) {
            if (e->xFC != 3) {
                continue;
            }
            if (e->xFD == 3) {
                continue;
            }
            dm = 1;
        }
        if (!dm) {
            continue;
        }
        if ((G_ROOM_ID32 & 0xFFFF0000) != 0x01010000) {
            f32 dx = em->pos.x - e->pos.x;
            f32 dy = em->pos.y - e->pos.y;
            f32 dz = em->pos.z - e->pos.z;
            f32 d = dx * dx + dy * dy + dz * dz;
            if (!(d < 9000000.0f)) {
                if (!(fabsf(Muku(&em->pos, &e->pos, em->rot.y, 3.1415927f)) < 1.0471976f)) {
                    continue;
                }
                if (!(d < 100000000.0f)) {
                    continue;
                }
            }
        }
        return 1;
    }
    return 0;
}

extern "C" int em10JumpDownCk2(cEm10* em);

int em10JumpDownCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int r;

    if ((em->flags_3C8 & 0x400) && w->x5EC == 0) {
        return 0;
    }
    if ((pG->flags_51E4 & 3) == (em->emsetNo & 3)) {
        f32 y = SatMgr.getFloor(&em->pos, 600.0f, 100000.0f, 0, 0);
        if (y < em->pos.y - 350.0f) {
            w->x5E0 = em->pos;
            EmRoutineSet(em, 1, 0x43, 0, 0);
            return 1;
        }
    }
    if (w->x650 < 600.0f) {
        return 0;
    }
    r = em10JumpDownCk2(em);
    switch ((u32) r) {
    case 0:
    default:
        break;
    case 1:
        w->x5E0 = em->pos;
        EmRoutineSet(em, r, 0x43, 0, 0);
        return 1;
    case 2:
        w->x5E0 = em->pos;
        em->xFD = 0x43;
        em->xFE = 0;
        em->xFC = 1;
        em->xFF = 1;
        return 1;
    }
    return 0;
}

void em10BombThrow(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    f32 d;
    int fuse;
    Vec spd;

    if (!w->pWep) {
        return;
    }
    if (w->wepType != 9) {
        return;
    }
    d = SQRTF(em->plDist2);
    if (d < 4000.0f) {
        d = 4000.0f;
    }
    fuse = (u8) (Rnd() % 10) + 50;
    if (pPL->pos.y < em->pos.y - 1000.0f) {
        if (pPL->pos.y < em->pos.y - 3000.0f) {
            d *= 0.015384615f;
            fuse = (u8) (Rnd() % 15) + 70;
        } else {
            d *= 0.018181818f;
            fuse = (u8) (Rnd() % 15) + 60;
        }
    } else {
        d *= 0.022222223f;
    }
    spd.x = fRand1_1() * 5.0f + 20.0f;
    spd.y = fRand1_1() * 5.0f + 50.0f;
    spd.z = fRand1_1() * 5.0f + d;
    PSMTXMultVecSR(em->mat, &spd, &spd);
    w->pWep->setBombThrow(&spd, fuse);
    w->pWep = 0;
    w->x640 = 0;
    w->wepType = 0;
    w->x670 = 10;
}

void em10BombNeckMove(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cModel* p;
    f32 d;
    f32 r;
    f32 amp;
    Mtx m;

    if (!(w->flags & 0x80)) {
        return;
    }
    if (w->x6C5 != 1) {
        return;
    }
    p = em->getPartsPtr(0x24);
    if ((w->flags & 0x01000020) == 0x01000000 && em->hp <= 0) {
        p->rot.x = p->rot.x * 0.9f + 0.17453294f;
    } else {
        p->rot.x = p->rot.x * 0.9f + -0.06981317f;
    }
    if (em->hp <= 0) {
        return;
    }
    d = sqrtf((p->worldPos.x - p->x88.x) * (p->worldPos.x - p->x88.x) +
              (p->worldPos.y - p->x88.y) * (p->worldPos.y - p->x88.y) +
              (p->worldPos.z - p->x88.z) * (p->worldPos.z - p->x88.z));
    if (d > 25.0f) {
        d = 25.0f;
    }
    r = d * 0.02f + 0.5f;
    amp = r * 0.5235988f;
    PSMTXRotRad(m, 'y', SINF(w->x64C) * amp);
    PSMTXConcat(p->mat, m, p->mat);
    w->x64C = r * 0.17453292f + w->x64C;
    p = em->getPartsPtr(0x1B);
    PSMTXConcat(p->mat, m, p->mat);
    if (w->pParasite) {
        p->scale.x *= 0.9f;
        if (p->scale.x < 0.1f) {
            p->scale.x = 0.1f;
        }
        p->scale.z = p->scale.y = p->scale.x;
    }
}

extern "C" void em10CallVoiceSe(cEm10* em, u16 no)
{
    Em10Work* w = EM10_WK(em);
    int idx;

    SndStop(w->x5B8, 0);
    SndStop(w->x5BC, 0);
    if (w->flags & 0x80) {
        return;
    }
    switch (em->type) {
    case 0:
        idx = 0;
        break;
    default:
        idx = 0;
        break;
    case 1:
        idx = 1;
        break;
    case 2:
        idx = 2;
        break;
    case 3:
        idx = 3;
        break;
    case 4:
        idx = 4;
        break;
    case 5:
        idx = 2;
        break;
    case 6:
        idx = 5;
        break;
    case 7:
        idx = 2;
        break;
    case 8:
        idx = 2;
        break;
    case 9:
        idx = 2;
        break;
    case 10:
        idx = 2;
        break;
    case 13:
        idx = 2;
        break;
    case 14:
        idx = 2;
        break;
    case 15:
        idx = 2;
        break;
    case 16:
        idx = 2;
        break;
    case 17:
        idx = 2;
        break;
    case 18:
        idx = 2;
        break;
    case 19:
        idx = 2;
        break;
    case 20:
        idx = 2;
        break;
    case 21:
        idx = 2;
        break;
    case 23:
        idx = 2;
        break;
    case 24:
        idx = 2;
        break;
    case 25:
        idx = 2;
        break;
    }
    w->x5B8 = Ctrl11StopAndSetSe(w->pCtrl11, em, Rnd() % 20 + 20, no, idx);
    w->x67E = Rnd() % 120 + 120;
}
extern "C" void Em10SetSeTbl(cEm10* em, int type)
{
    Em10Work* w = EM10_WK(em);

    switch ((u32) type) {
    case 0:
    default:
        w->se6C6 = 0x16;
        w->se6C7 = 0x17;
        w->se6C8 = 0x18;
        w->se6C9 = 0x19;
        w->se6CA = 0x1A;
        w->se6CB = 0x1B;
        w->se6CC = 0x1C;
        w->se6CD = 0x25;
        w->se6CE = 0x47;
        w->se6CF = 0x48;
        w->se6D0 = 0x49;
        w->se6D1 = 0x4A;
        w->se6D2 = 0x4B;
        w->se6D3 = 0x4C;
        w->se6D4 = 0x4D;
        w->se6D5 = 0x4E;
        w->se6D6 = 0x5B;
        w->se6D7 = 0xB5;
        w->se6D8 = 0x15;
        break;
    case 1:
        w->se6C6 = 0x1E;
        w->se6C7 = 0x1F;
        w->se6C8 = 0x20;
        w->se6C9 = 0x21;
        w->se6CA = 0x22;
        w->se6CB = 0x23;
        w->se6CC = 0x24;
        w->se6CD = 0x27;
        w->se6CE = 0x51;
        w->se6CF = 0x52;
        w->se6D0 = 0x53;
        w->se6D1 = 0x54;
        w->se6D2 = 0x55;
        w->se6D3 = 0x56;
        w->se6D4 = 0x57;
        w->se6D5 = 0x58;
        w->se6D6 = 0x5C;
        w->se6D7 = 0xB6;
        w->se6D8 = 0x1D;
        break;
    case 2:
        w->se6C6 = 0x2E;
        w->se6C7 = 0x2F;
        w->se6C8 = 0x30;
        w->se6C9 = 0x31;
        w->se6CA = 0x32;
        w->se6CB = 0x33;
        w->se6CC = 0x34;
        w->se6CD = 0x29;
        w->se6CE = 0x65;
        w->se6CF = 0x66;
        w->se6D0 = 0x67;
        w->se6D1 = 0x68;
        w->se6D2 = 0x69;
        w->se6D3 = 0x6A;
        w->se6D4 = 0x6B;
        w->se6D5 = 0x6C;
        w->se6D6 = 0x5D;
        w->se6D7 = 0xB7;
        w->se6D8 = 0x2D;
        break;
    case 3:
        w->se6C6 = 0x36;
        w->se6C7 = 0x37;
        w->se6C8 = 0x38;
        w->se6C9 = 0x39;
        w->se6CA = 0x3A;
        w->se6CB = 0x3B;
        w->se6CC = 0x3C;
        w->se6CD = 0x2B;
        w->se6CE = 0x6F;
        w->se6CF = 0x70;
        w->se6D0 = 0x71;
        w->se6D1 = 0x72;
        w->se6D2 = 0x73;
        w->se6D3 = 0x74;
        w->se6D4 = 0x75;
        w->se6D5 = 0x76;
        w->se6D6 = 0x5E;
        w->se6D7 = 0xB8;
        w->se6D8 = 0x35;
        break;
    }
    if (em->type == 6) {
        w->se6C6 = 0xE;
    }
}

extern "C" void em10WeaponSet2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec pos;
    Vec rot;

    if (w->pWep2) {
        return;
    }
    if (!(em->flags_3C8 & 0x2000)) {
        return;
    }
    if (em->flags_3C8 & 0x20000000) {
        int t = 0xB;
        if (w->x6C5 == 0) {
            t = 2;
        }
        w->wep2Type = t;
    }
    if (em->flags_3C8 & 0x08000000) {
        int t = w->x6C5;
        if (t != 2) {
            t = 3;
        }
        w->wep2Type = t;
    }
    if (em->flags_3C8 & 0x4000) {
        w->wep2Type = 0xA;
    }
    if (em->flags_3C8 & 0x8000) {
        w->wep2Type = 8;
    }
    if (em->flags_3C8 & 0x20000) {
        w->wep2Type = 9;
    }
    w->pWep2 = em10MakeWeapon(em, w->wep2Type);
    if (!w->pWep2) {
        w->wep2Type = 0;
        return;
    }
    if (em->type == 6) {
        w->pWep2->setTransMode(0);
    }
    if (em->flags_3C8 & 0x01000000) {
        pos.x = -140.0f;
        pos.y = -30.0f;
        pos.z = -150.0f;
        rot.x = -1.5707964f;
        rot.y = 0.0f;
        rot.z = 1.2566371f;
    } else {
        pos.x = 140.0f;
        pos.y = -30.0f;
        pos.z = -150.0f;
        rot.x = 1.5707964f;
        rot.y = 0.0f;
        rot.z = 1.8849555f;
    }
    w->pWep2->pos = pos;
    w->pWep2->rot = rot;
    w->pWep2->setParent(em, 0x11, 0);
}

extern "C" int em10AtkRackCk(cEm10* em)
{
    u32 i;
    Mtx inv;
    Vec v;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if (!(e->be_flag & 1)) {
            continue;
        }
        if (!(e->be_flag & 0x20)) {
            continue;
        }
        if (e->id != 0x45) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        {
            f32 dx = em->pos.x - e->pos.x;
            f32 dy = em->pos.y - e->pos.y;
            f32 dz = em->pos.z - e->pos.z;
            if (dx * dx + dy * dy + dz * dz > 4000000.0f) {
                continue;
            }
        }
        if (fabsf(Muku(&em->pos, &e->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
            continue;
        }
        PSMTXInverse(e->mat, inv);
        PSMTXMultVec(inv, &em->pos, &v);
        if (v.x > 1000.0f || v.x < -1000.0f) {
            continue;
        }
        if (v.y > 500.0f || v.y < -500.0f) {
            continue;
        }
        if (v.z > 1800.0f || v.z < -1800.0f) {
            continue;
        }
        return 1;
    }
    return 0;
}

extern "C" int em10CatchSubRtnCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    int hit;
    int rtn;

    if (!pSUB) {
        return 0;
    }
    if (w->x58C) {
        return 0;
    }
    if (w->wepType == 4) {
        return 0;
    }
    if (pG->flags_500C & 0x800) {
        return 0;
    }
    if (pG->flags_5010 & 8) {
        return 0;
    }
    if (w->pShield) {
        return 0;
    }
    if (em->type == 10 || em->type == 13 || em->type == 2 || em->type == 0x18) {
        return 0;
    }
    rtn = 1;
    if (pG->flags_5010 & 0x10000) {
        return 0;
    }
    if (pG->flags_5014 & 0x00800000) {
        return 0;
    }
    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if (w->x67C != 0) {
        return 0;
    }
    if (pSUB->hp <= 0) {
        return 0;
    }
    if (!(w->flags & 2)) {
        return 0;
    }
    if (fabsf(em->pos.y - pSUB->pos.y) > 300.0f) {
        return 0;
    }
    if (w->x514 > 1210000.0f) {
        return 0;
    }
    a = em->pos;
    b = pSUB->pos;
    a.y += 1500.0f;
    b.y += 1500.0f;
    hit = SatMgr.hitCheck(&a, &b, 0, 0, 0, 0);
    if (hit) {
        return 0;
    }
    em->xFD = 0x33;
    em->xFE = hit;
    em->xFC = rtn;
    em->xFF = rtn;
    return 1;
}

extern "C" void em10SetRtnFind(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    int r;
    f32 ang;

    em->setFindPL();
    if (w->pShield) {
        return;
    }
    if (em->type == 10 || em->type == 13 || em->type == 2) {
        return;
    }
    if (w->wepType == 8) {
        return;
    }
    if (w->flags & 0x80) {
        return;
    }
    if (w->pWep2 && !w->pParasite && !w->x58C && (!w->pWep || w->wepType == 5)) {
        em->setWeaponFall();
        EmRoutineSet(em, 1, 0xC, 0, 0);
        return;
    }
    if (em10IgnitionCk(em)) {
        return;
    }
    r = em10ClawStickCK(em);
    if (r) {
        return;
    }
    if (em10SomebodyFindNowCk(em)) {
        if (w->x508 < 1.5707964f) {
            ang = fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f));
            if (em->plDist2 < 9000000.0f || (em->plDist2 < 49000000.0f && ang < 0.7853982f)) {
                em10WalkRtnSet(em);
            } else {
                EmRoutineSet(em, 1, 0xD, 0, 0);
            }
        } else {
            em->xFD = 0x15;
            em->xFE = r;
            em->xFC = 1;
            em->xFF = 1;
        }
    } else {
        em10WalkRtnSet(em);
    }
}

extern "C" int em10BullJumpCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cObjBull* o;
    cModel* p;
    Mtx m;
    Vec v;
    Mtx inv;
    Vec lv;
    f32 ang;

    if (pG->room_id != 0x30F) {
        return 0;
    }
    for (o = (cObjBull*) ObjMgr.pAlive; o; o = (cObjBull*) o->next) {
        if (o->id != 0x3E) {
            continue;
        }
        p = o->getPartsPtr(2);
        if (fabsf(Muku(&em->pos, &p->worldPos, em->rot.y, 3.1415927f)) > 0.5235988f) {
            continue;
        }
        ang = GetXZAngle(&em->pos, &p->worldPos);
        PSMTXRotRad(m, 'y', ang);
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 1500.0f;
        PSMTXMultVec(em->mat, &v, &v);
        v.y = p->worldPos.y + 500.0f;
        if (!o->ckBullRide(&v, 0, 0)) {
            continue;
        }
        PSMTXInverse(p->mat, inv);
        PSMTXMultVec(inv, &em->pos, &lv);
        if (lv.x < -1500.0f || lv.x > 1500.0f) {
            continue;
        }
        if (!(lv.z > -4000.0f)) {
            em->rot.y = ang;
            w->x5EC = 0;
            EmRoutineSet(em, 1, 0x67, 0, 0);
            return 1;
        }
    }
    return 0;
}

extern "C" int em10TorchFrameAtkCkSub(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx inv;
    Vec v;
    EmHitInfo* hit;
    u8 old;

    if (!pSUB) {
        return 0;
    }
    if (pSUB->hp <= 0) {
        return 0;
    }
    if (em10DeadCk(pSUB)) {
        return 0;
    }
    old = w->x698;
    if (old) {
        return 0;
    }
    PSMTXInverse(em->mat, inv);
    PSMTXMultVec(inv, &pSUB->pos, &v);
    if (v.x > 500.0f || v.x < -500.0f) {
        return 0;
    }
    if (v.z > 3500.0f || v.z < 0.0f) {
        return 0;
    }
    if (v.y > 1500.0f || v.y < -500.0f) {
        return 0;
    }
    w->x698 = 1;
    SndCall(8, 0x8F, &pSUB->pos, em->id, 0, pSUB);
    Ctrl12Set(w->pCtrl12, 9, 0x1E);
    EstSet((int) pSUB, -1, 0, 0, 0x10, 0x28, 0, 0, (u32) pSUB, (void*) old);
    v = pSUB->pos;
    v.y += 1300.0f;
    hit = EmAtkHitSubCk2(&Em10AtkTbl[17], &v, &em->pos);
    if (hit) {
        pSUB->dmg.set(0, 0xA, 0x18, &em->pos, hit->rad, hit);
    }
    return 1;
}

int em10JumpCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec n;
    Vec a;
    Vec b;
    Mtx m;
    f32 ang;
    f32 y;

    if (em10BullJumpCk(em)) {
        return 1;
    }
    if (w->x634 % 15 != 7) {
        return 0;
    }
    a.x = 0.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    if (w->pShield) {
        b.x = 0.0f;
        b.y = 500.0f;
        b.z = 700.0f;
    } else {
        b.x = 0.0f;
        b.y = 500.0f;
        b.z = 500.0f;
    }
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    if (!(SatMgr.hitCheck(&a, &b, 0, &n, 0, 0) & 0x80000)) {
        return 0;
    }
    ang = atan2f(-n.x, -n.z);
    PSMTXRotRad(m, 'y', ang);
    TransMatrix(m, &em->pos);
    a.x = 0.0f;
    a.y = 500.0f;
    a.z = 4500.0f;
    PSMTXMultVec(m, &a, &a);
    y = SatMgr.getFloor(&a, 600.0f, 100000.0f, 0, 0);
    if (fabsf(em->pos.y - y) > 1000.0f) {
        return 0;
    }
    em->rot.y = ang;
    EmRoutineSet(em, 1, 0x44, 0, 0);
    return 1;
}
int em10FindCk2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    if (w->x5EC) {
        return 0;
    }
    if (pG->flags_5010 & 0x20000000) {
        if (pG->bell_stat == 2) {
            f32 dx = em->pos.x - pG->bell_pos.x;
            f32 dy = em->pos.y - pG->bell_pos.y;
            f32 dz = em->pos.z - pG->bell_pos.z;
            if (dx * dx + dy * dy + dz * dz < 400000000.0f) {
                w->x5F0 = pG->bell_pos;
                w->x646 = 0;
                return 1;
            }
        }
        if (pG->bell_stat == 1) {
            f32 dx = em->pos.x - pG->bell_pos.x;
            f32 dy = em->pos.y - pG->bell_pos.y;
            f32 dz = em->pos.z - pG->bell_pos.z;
            if (dx * dx + dy * dy + dz * dz < 400000000.0f) {
                w->x5F0 = pG->bell_pos;
                return 1;
            }
        }
    }
    if ((s32) pG->flags_5010 < 0) {
        f32 r = 12250000.0f;
        if (EM_RTN(pPL, 0, 3)) {
            r = 64000000.0f;
        }
        if (em->plDist2 < r) {
            w->x5F0 = pPL->pos;
            return 1;
        }
    }
    if (em->plDist2 < 1000000.0f) {
        w->x5F0 = pPL->pos;
        return 1;
    }
    if ((pG->flags_500C & 0x00800000) && w->x524 < 25000.0f) {
        w->x5F0 = pPL->pos;
        return 1;
    }
    return 0;
}

void em10HandSet(cEm10* em, int type)
{
    Em10Work* w = EM10_WK(em);
    cEmWep* wep;
    void* bin;
    void* tpl;
    cModelInfo* info;

    if (em->type == 10 || em->type == 13 || em->type == 2 || em->type == 22 || em->type == 24) {
        return;
    }
    wep = w->pWep;
    if (wep) {
        type = 3;
    }
    if (w->x184 && w->x188 && w->x6AD == type) {
        if (type != 0) {
            return;
        }
        if (!wep) {
            return;
        }
    }
    switch ((u32) type) {
    case 0:
    default:
        bin = w->mot[6];
        tpl = w->mot[11];
        break;
    case 1:
        bin = w->mot[7];
        tpl = w->mot[12];
        break;
    case 2:
        if (em->flags_3C8 & 0x01000000) {
            bin = w->mot[6];
            tpl = w->mot[13];
        } else {
            bin = w->mot[8];
            tpl = w->mot[11];
        }
        break;
    }
    if (wep) {
        if (em->flags_3C8 & 0x01000000) {
            tpl = w->mot[14];
        } else {
            bin = w->mot[9];
        }
        if (w->wepType == 1 || w->wepType == 4) {
            bin = w->mot[9];
            tpl = w->mot[14];
        }
        if (w->wepType == 6) {
            tpl = w->mot[15];
        }
    }
    if (w->wepType == 12) {
        bin = PL_ARC_PTR(em->subArc, 0x18E);
    }
    info = ModInfoMgr.create(bin, w->mot[0]);
    if (info) {
        if (w->x184) {
            cModel_swapModelInfo(em, w->x184->pData, info);
        } else {
            em->addModel(info);
        }
        w->x184 = info;
    }
    info = ModInfoMgr.create(tpl, w->mot[0]);
    if (info) {
        if (w->x188) {
            cModel_swapModelInfo(em, w->x188->pData, info);
        } else {
            em->addModel(info);
        }
        w->x188 = info;
    }
    w->x6AD = type;
}

void em10SetDamageRack(cEm10* em, int a)
{
    u32 i;
    Mtx inv;
    Vec v;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEmRack* e = (cEmRack*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if (!(e->be_flag & 1)) {
            continue;
        }
        if (!(e->be_flag & 0x20)) {
            continue;
        }
        if (e->id != 0x45) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        {
            f32 dx = em->pos.x - e->pos.x;
            f32 dy = em->pos.y - e->pos.y;
            f32 dz = em->pos.z - e->pos.z;
            if (dx * dx + dy * dy + dz * dz > 4000000.0f) {
                continue;
            }
        }
        if (fabsf(Muku(&em->pos, &e->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
            continue;
        }
        PSMTXInverse(e->mat, inv);
        PSMTXMultVec(inv, &em->pos, &v);
        if (v.x > 1000.0f || v.x < -1000.0f) {
            continue;
        }
        if (v.y > 500.0f || v.y < -500.0f) {
            continue;
        }
        if (v.z > 1800.0f || v.z < -1800.0f) {
            continue;
        }
        switch ((u32) a) {
        case 0:
        case 1:
        default:
            e->setShock();
            break;
        case 2:
            e->setDown(&em->pos);
            break;
        }
        return;
    }
}

extern "C" int em10HeadLockCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx inv;
    Vec v;
    cModel* p;
    int r;

    if (pPL->xFC != 0) {
        return 0;
    }
    if (pPL->xFD != 6) {
        return 0;
    }
    if (em->plDist2 > 64000000.0f) {
        return 0;
    }
    if (!(w->flags & 0x100)) {
        return 0;
    }
    if (w->x58C) {
        return 0;
    }
    if (w->pParasite) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    if (pG->x4F88 <= 3) {
        return 0;
    }
    if (w->pShield) {
        return 0;
    }
    if (em->type == 10 || em->type == 13 || em->type == 2) {
        return 0;
    }
    if (em->flags_3C8 & 0x200) {
        if (w->x6C5 == 1) {
            return 0;
        }
        if (w->x6C5 == 2) {
            return 0;
        }
    }
    if (pG->wep_no == 0x10) {
        return 0;
    }
    if (!ItemMgr.bulletNumCurrent()) {
        return 0;
    }
    if (fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
        return 0;
    }
    PSMTXInverse(pPL->getPartsPtr(10)->mat, inv);
    p = em->getPartsPtr(4);
    v.x = 0.0f;
    v.y = 150.0f;
    v.z = 0.0f;
    PSMTXMultVec(p->mat, &v, &v);
    PSMTXMultVec(inv, &v, &v);
    if (v.x > 0.0f) {
        return 0;
    }
    if (v.z > 300.0f || v.z < -300.0f) {
        return 0;
    }
    if (v.y > 300.0f) {
        return 0;
    }
    r = 0;
    if (v.y < -300.0f) {
        return r;
    }
    return 1;
}

void em10CoreBreak(cEm10* em, int a)
{
    Em10Work* w = EM10_WK(em);
    u32 i;

    if (!w->pParasite) {
        return;
    }
    if (a != 2 && (em->type == 10 || em->type == 13)) {
        return;
    }
    EffectEspDelete(0, w->x6A0, (u32) w->pParasite, 0);
    EffectEspgenDelete(0, w->x6A0, (int) w->pParasite);
    EffectEfmDelete(0, w->x6A0, (int) w->pParasite);
    SndStop(w->x5B8, 0);
    SndStop(w->x5BC, 0);
    if (a) {
        w->pParasite->clearLostWait();
        w->pParasite = 0;
        for (i = 0; i <= 4; i++) {
            if (w->x578[i]) {
                ((cObj16*) w->x578[i])->clearLostWait();
                w->x578[i] = 0;
            }
        }
        return;
    }
    w->x5B8 = Ctrl11SetSe2(w->pCtrl11, em, Rnd() % 20 + 20, 0x89, 6, 8);
    w->x67E = Rnd() % 120 + 120;
    if (w->x6C5 == 1) {
        EstSet(0, -1, &w->pParasite->getPartsPtr(9)->worldPos, 0, 0x10, 0x80, 0, 0, a, (void*) a);
    } else {
        EstSet(0, -1, &em->getPartsPtr(3)->worldPos, 0, 0x10, 0x27, 0, 0, a, (void*) a);
    }
    w->pParasite->clearLostWait();
    w->pParasite = 0;
    for (i = 0; i <= 4; i++) {
        if (w->x578[i]) {
            ((cObj16*) w->x578[i])->clearLostWait();
            w->x578[i] = 0;
        }
    }
}

extern "C" int em10ClawAtkCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    int hit;

    if (em->type != 10 && em->type != 13) {
        return 0;
    }
    if (!(w->flags & 0x100)) {
        return 0;
    }
    if (w->x67C != 0) {
        return 0;
    }
    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if (!(w->flags & 1)) {
        return 0;
    }
    if (w->x508 > 0.7853982f) {
        return 0;
    }
    if (fabsf(em->pos.y - w->x534.y) > 1500.0f) {
        return 0;
    }
    {
        f32 dx = em->pos.x - w->x534.x;
        f32 dz = em->pos.z - w->x534.z;
        if (dx * dx + dz * dz > 1960000.0f) {
            return 0;
        }
    }
    a = em->pos;
    b = w->x534;
    a.y += 1500.0f;
    b.y += 1500.0f;
    hit = EatMgr.hitCheck(&a, &b, 0, 0, 0, 0x4000);
    if (hit) {
        return 0;
    }
    if (w->x6BE == 4 || w->x6BF == 4) {
        EmRoutineSet(em, 1, 0x59, hit, hit);
        return 1;
    }
    EmRoutineSet(em, 1, 0x2B, hit, hit);
    if (pG->x4F88 <= 3) {
        Ctrl12Set(w->pCtrl12, 6, 0x3C);
        Ctrl12Set(w->pCtrl12, 8, 0x78);
    } else if (pG->stage_no <= 2 && pG->x4F88 <= 9) {
        Ctrl12Set(w->pCtrl12, 6, 0x1E);
        Ctrl12Set(w->pCtrl12, 8, 0x78);
    }
    return 1;
}

int em10CrashCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    int r;

    if (em->dmg.stat != 0) {
        return 0;
    }
    if (em->hp <= 0) {
        return 0;
    }
    if (w->pShield && w->pShield->hp > 0) {
        return 0;
    }
    if (em->type == 10 || em->type == 13 || em->type == 2) {
        return 0;
    }
    if (em->x38D == 0x2D || em->x38D == 0x37 || em->x38D == 0x38) {
        return 0;
    }
    if (w->pGatling) {
        return 0;
    }
    if (em->xFC == 1) {
        if (em->xFD == 0x43 || em->xFD == 0x40 || em->xFD == 0x41 || em->xFD == 0x44 || em->xFD == 0x45 ||
            em->xFD == 0x48) {
            return 0;
        }
    }
    if (w->flags & 0x101B6818) {
        return 0;
    }
    if (em->type == 6) {
        return 0;
    }
    r = DmgMgr.hitCheck(&em->pos, &v) == 3;
    if (w->pShield && w->pShield->hp <= 0) {
        v.x = 0.0f;
        v.y = 1000.0f;
        v.z = 1000.0f;
        PSMTXMultVec(em->mat, &v, &v);
        r = 1;
    }
    if (!r) {
        return 0;
    }
    if (fabsf(Muku(&em->pos, &v, em->rot.y, 3.1415927f)) < 1.5707964f) {
        EmRoutineSet(em, 1, 0x3B, 0, 0);
    } else {
        em->xFD = 0x3B;
        em->xFE = 0;
        em->xFC = 1;
        em->xFF = 1;
    }
    w->x5E0 = v;
    if ((w->flags & 0x00200000) && w->pLadder) {
        w->pLadder->setDown2();
        w->pLadder = 0;
    }
    return 1;
}

int em10LadderClimbCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cObjLadder* o;
    Mtx m;
    Vec v;

    if (w->x634 % 15 != 4) {
        return 0;
    }
    if (w->x5EC == 0 && em->x3D0 == 5) {
        return 0;
    }
    if (w->x54C.y - em->pos.y < 1000.0f) {
        return 0;
    }
    for (o = (cObjLadder*) ObjMgr.pAlive; o; o = (cObjLadder*) o->next) {
        if (o->id != 0x13) {
            continue;
        }
        if (!o->ckClimb()) {
            continue;
        }
        {
            f32 dx = em->oldPos.x - o->pos.x;
            f32 dy = em->oldPos.y - o->pos.y;
            f32 dz = em->oldPos.z - o->pos.z;
            if (dx * dx + dy * dy + dz * dz > 4000000.0f) {
                continue;
            }
        }
        if (fabsf(Muku(&em->oldPos, &o->pos, em->rot.y, 3.1415927f)) > 1.5707964f) {
            continue;
        }
        PSMTXRotRad(m, 'y', o->rot.y);
        TransMatrix(m, &o->pos);
        PSMTXInverse(m, m);
        PSMTXMultVec(m, &em->pos, &v);
        if (v.z > 1000.0f || v.z < -500.0f) {
            continue;
        }
        if (v.x > 800.0f || v.x < -800.0f) {
            continue;
        }
        if (!(fabsf(v.y) > 500.0f)) {
            w->pLadder = o;
            o->setClimb();
            EmRoutineSet(em, 1, 0x40, 0, 0);
            return 1;
        }
    }
    return 0;
}
extern "C" void em10WepSeEffSet(cEm10* em, cEmWep* wep, u8 type)
{
    if (!wep) {
        return;
    }
    switch (type) {
    case 0:
    case 9:
        return;
    case 2:
    case 3:
    case 5:
    case 10:
    case 11:
    case 13:
    default:
        wep->setSeDamage(8, 0x3F, em->id);
        wep->setSeHit(8, 0xB, em->id);
        wep->setSeFall(8, 0x40, em->id);
        wep->setSeThrow(8, 0x45, em->id, 4);
        wep->setEffDamage(0, 0x18);
        wep->setEffHit(0x10, 0xB);
        wep->setEffWater(1, 0x37);
        break;
    case 7:
        wep->setSeDamage(8, 0x3F, em->id);
        wep->setSeHit(8, 0x46, em->id);
        wep->setSeFall(8, 0x43, em->id);
        wep->setSeThrow(8, 0x42, em->id, 4);
        wep->setEffDamage(0, 0x18);
        wep->setEffHit(0x10, 0xB);
        wep->setEffWater(1, 0x37);
        break;
    case 1:
    case 6:
    case 8:
        wep->setSeDamage(8, 0x3F, em->id);
        wep->setSeHit(8, 0xB, em->id);
        wep->setSeFall(8, 0x41, em->id);
        wep->setSeThrow(8, 0x45, em->id, 4);
        wep->setEffDamage(0, 0x18);
        wep->setEffHit(0x10, 0xB);
        wep->setEffWater(1, 0x37);
        break;
    case 12:
        wep->setSeDamage(8, 0x3F, em->id);
        wep->setSeHit(8, 0xB, em->id);
        wep->setSeFall(8, 0x41, em->id);
        wep->setSeThrow(8, 0x45, em->id, 4);
        wep->setEffDamage(0, 0x18);
        wep->setEffHit(0x10, 0xB);
        wep->setEffWater(1, 0x37);
        break;
    }
}

void em10DragonFireCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;

    if (!w->pDragon) {
        return;
    }
    if (!EM10_DRAGON(w)->ckHitFireBlocked()) {
        return;
    }
    if ((s16) pG->pl_life > 0) {
        int dead = em10DeadCk(pPL);
        if (!dead) {
            if (EM10_DRAGON(w)->ckHitFire(&pPL->pos)) {
                SndCall(8, 0x8F, &pPL->pos, em->id, 0, pPL);
                Ctrl12Set(w->pCtrl12, 9, 0x1E);
                pPL->rot.y += Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f);
                SetPlDamage((int) em, plemDmFrame);
                pPL->dmg.set(0, 0x1E);
            }
        }
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm10* e = (cEm10*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        int dead;
        cDmgInfo* d;
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        d = &e->dmg;
        dead = em10DmgDeadCk(d);
        if (dead) {
            continue;
        }
        if (e == em) {
            continue;
        }
        {
            Em10Work* ew = EM10_WK(e);
            if (!EM10_DRAGON(w)->ckHitFire(&e->pos)) {
                continue;
            }
            d->set(0, 0x1E);
            ew->x68C = 0x78;
            e->xFC = 2;
            e->xFD = 0xB;
            e->xFE = dead;
            e->xFF = dead;
        }
    }
}

int em10AtkRtnCk(cEm10* em, int a)
{
    Em10Work* w = EM10_WK(em);

    if (pG->flags_64 & 0x02000000) {
        em->xFD = 0;
        em->xFE = 0;
        em->xFF = 0;
        em->xFC = 1;
        return 1;
    }
    if (a == 0) {
        if (em10BackCk(em)) {
            return 1;
        }
    }
    if (w->x644 != 0) {
        return 0;
    }
    if (w->x696 == 0) {
        if (Ctrl12Ck(w->pCtrl12, 6)) {
            return 0;
        }
    }
    if (pG->flags_5010 & 0x00200000) {
        return 0;
    }
    if (em10ParasiteAtkCk(em)) {
        return 1;
    }
    if (w->x58C) {
        return 0;
    }
    if (em->type != 10 && em->type != 13) {
        if (w->pParasite) {
            return 0;
        }
    }
    if (em->type == 0x16) {
        return 0;
    }
    if (em10ShieldAtkCk(em)) {
        return 1;
    }
    if (em10AxeAtkCk(em)) {
        return 1;
    }
    if (em10SukiAtkCk(em)) {
        return 1;
    }
    if (em10ScytheAtkCk(em)) {
        return 1;
    }
    if (em10ClawAtkCk(em)) {
        return 1;
    }
    if (em10ClawCriAtkCk(em)) {
        return 1;
    }
    if (em10ShotBowgunCk(em)) {
        return 1;
    }
    if (em10ShotRocketCk(em)) {
        return 1;
    }
    if (em10ShotGatlingCk(em)) {
        return 1;
    }
    if (em10ThrowAxeCk(em)) {
        return 1;
    }
    if (em10ThrowBombCk(em)) {
        return 1;
    }
    if (pG->room_id == 0x21B) {
        if (pG->flags_5014 & 0x08000000) {
            return 0;
        }
    }
    if (em10CsawAtkCk(em)) {
        return 1;
    }
    if (em10CatchPLRtnCk(em)) {
        return 1;
    }
    if (em10CatchSubRtnCk(em)) {
        return 1;
    }
    return 0;
}
extern "C" void em10PlHeadLost()
{
    Vec v;
    Vec ofs;
    cObj* o;
    cModel* p;
    int region = pSys->region;

    if (region == 0) {
        PlSetDamageSe(0xD);
        EstSet((int) pPL, -1, 0, 0, 0x10, 0x57, 0, 0, (u32) pPL, 0);
        return;
    }
    SndCall(1, 0x3E, &pPL->pos, 0, 0, pPL);
    EstSet((int) pPL, -1, 0, 0, 0x10, 0x45, 0, 0, (u32) pPL, 0);
    pPL->setHead(0);
    p = pPL->getPartsPtr(3);
    if (pG->x4FB8 == 2) {
        v.x = 0.0f;
        v.y = 84.0f;
        v.z = 0.0f;
    } else {
        v.x = 0.0f;
        v.y = 68.0f;
        v.z = 28.0f;
    }
    ofs.x = 0.0f;
    ofs.y = 50.0f;
    ofs.z = -25.0f;
    PSMTXMultVec(p->mat, &v, &v);
    PSMTXMultVecSR(pPL->mat, &ofs, &ofs);
    o = SetObj01(PL_ARC_PTR(pG->pPlArc, 0xC), PL_ARC_PTR(pG->pPlArc, 7), &v, &pPL->rot, &ofs, 10.0f, 150.0f, 1000,
                 0x11);
    if (o) {
        o->lightInfo.x50 = 1;
        Obj01SetEst(o, 0, -1, 4, 0, -1, 0, -1, 0, -1);
    }
    EstSet((int) o, -1, 0, 0, 0x10, 0x46, 0, 0, (u32) o, 0);
}

extern "C" int em10AtkDoorCk(cEm10* em)
{
    u32 i;
    Mtx m;
    Vec v;

    for (i = 0; i < EmMgr.nArray; i++) {
        cEmDoor* d = (cEmDoor*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        EmDoorWork* dw;
        f32 hw;
        if (!(d->be_flag & 1)) {
            continue;
        }
        if (!(d->be_flag & 0x20)) {
            continue;
        }
        if (d->id != 0x41) {
            continue;
        }
        if (d->hp <= 0) {
            continue;
        }
        {
            f32 dx = em->pos.x - d->pos.x;
            f32 dy = em->pos.y - d->pos.y;
            f32 dz = em->pos.z - d->pos.z;
            if (dx * dx + dy * dy + dz * dz > 4000000.0f) {
                continue;
            }
        }
        dw = EMDOOR_WK(d);
        PSMTXRotRad(m, 'y', dw->rotY);
        TransMatrix(m, &d->pos);
        v.x = -dw->width * 0.5f;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(m, &v, &v);
        TransMatrix(m, &v);
        if (fabsf(Muku(&em->pos, &v, em->rot.y, 3.1415927f)) > 0.7853982f) {
            continue;
        }
        PSMTXInverse(m, m);
        PSMTXMultVec(m, &em->pos, &v);
        hw = dw->width * 0.5f + 300.0f;
        if (v.x > hw) {
            continue;
        }
        if (v.x < -hw) {
            continue;
        }
        if (v.y > 500.0f || v.y < -500.0f) {
            continue;
        }
        if (v.z > 800.0f || v.z < -800.0f) {
            continue;
        }
        if (d->ckOpen() == 1) {
            continue;
        }
        return 1;
    }
    return 0;
}

extern "C" void em10ActEvtSetFS(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    int hit;

    if (em->hp <= 0) {
        return;
    }
    if (em->plDist2 > 1000000.0f) {
        return;
    }
    if (fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f)) > 1.0471976f) {
        return;
    }
    if (em->type == 10 || em->type == 13 || em->type == 2 || em->type == 0x16) {
        return;
    }
    if (pG->x4FB8 != 4) {
        if (w->x6C5 == 0) {
            return;
        }
    }
    if (fabsf(em->pos.y - pPL->pos.y) > 700.0f) {
        return;
    }
    a = em->pos;
    b = pPL->pos;
    a.y += 1500.0f;
    b.y += 1500.0f;
    if (SatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return;
    }
    a = em->pos;
    b = pPL->pos;
    a.y += 1500.0f;
    b.y += 1500.0f;
    hit = EatMgr.hitCheck(&a, &b, 0, 0, 0, 0);
    if (hit) {
        return;
    }
    if (pG->x4FB8 == 4) {
        ActBtn.set(0x3A, 0xB, (int) em10FSAction, (int) em, 1, 1, 0, hit);
    } else {
        ActBtn.set(0x2C, 0xB, (int) em10FSAction, (int) em, 1, 1, 0, hit);
    }
}
void em10ActEvtSetTrade(cEm10* em)
{
    Mtx inv;
    Vec v;
    u32 i;

    if (em->type != 6) {
        return;
    }
    if (em->hp <= 0) {
        return;
    }
    if (fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f)) > 0.7853982f) {
        return;
    }
    PSMTXInverse(pPL->mat, inv);
    PSMTXMultVec(inv, &em->pos, &v);
    switch (pG->room_id) {
    default:
        if (v.z > 1500.0f) {
            return;
        }
        if (v.z < 0.0f) {
            return;
        }
        if (v.x > 700.0f) {
            return;
        }
        if (v.x < -700.0f) {
            return;
        }
        if (fabsf(v.y) > 700.0f) {
            return;
        }
        break;
    case 0x20F:
    case 0x301:
    case 0x305:
        if (em->plDist2 > 2250000.0f) {
            if (v.z > 3000.0f) {
                return;
            }
            if (v.z < 0.0f) {
                return;
            }
            if (v.x > 700.0f) {
                return;
            }
            if (v.x < -700.0f) {
                return;
            }
        }
        if (fabsf(v.y) > 700.0f) {
            return;
        }
        break;
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* e = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id <= 0xF) {
            continue;
        }
        if (e->id > 0x20) {
            continue;
        }
        if (e == em) {
            continue;
        }
        if (!e->checkStatus(5)) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        {
            f32 dx = em->pos.x - e->pos.x;
            f32 dy = em->pos.y - e->pos.y;
            f32 dz = em->pos.z - e->pos.z;
            if (dx * dx + dy * dy + dz * dz < 100000000.0f) {
                return;
            }
        }
    }
    ActBtn.set(0, 2, (int) em10TradeAction, (int) em, 0, 1, 0, 0);
}

extern "C" int em10WindowCk2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    Vec dir;
    Vec pos;
    u16 status;
    f32 ang;
    f32 y;

    if (w->x634 % 15 != 10) {
        return 0;
    }
    a.x = 0.0f;
    a.y = 400.0f;
    a.z = 0.0f;
    b.x = 0.0f;
    b.y = 400.0f;
    b.z = 1000.0f;
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    if (!ChkWindow(em, &a, &b, 1, &status, &dir, &pos, &w->pWindow)) {
        return 0;
    }
    if (!w->pWindow->ChkEnableDamage()) {
        return 0;
    }
    Mtx m;
    Vec tmp;
    Vec v;
    ang = atan2f(-dir.x, -dir.z);
    em->rot.y = ang;
    PSMTXRotRad(m, 'y', ang);
    TransMatrix(m, &pos);
    a.x = 0.0f;
    a.y = 0.0f;
    a.z = -400.0f;
    PSMTXMultVec(m, &a, &w->x5E0);
    w->x5E0.y = em->pos.y;
    tmp = em->pos;
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 1000.0f;
    PSMTXMultVec(m, &v, &v);
    y = SatMgr.getFloor(&v, 600.0f, 100000.0f, 0, 0);
    if (y < em->pos.y - 500.0f) {
        return 2;
    }
    if (!(status & 1)) {
        if ((w->flags & 0x4000) && w->pWindow) {
            w->pWindow->SetBreakAll(&em->pos, 0, 0);
            w->pWindow = 0;
            return 0;
        }
        if (em10DootAtkCk(em) == 0) {
            return 0;
        }
        return 3;
    }
    if (EmRackCk(em, &em->pos, ang)) {
        return 1;
    }
    if (em10DootAtkCk(em) == 0) {
        return 0;
    }
    return 4;
}

int em10FindCk(cEm10* em, int a)
{
    Em10Work* w = EM10_WK(em);
    int find = 0;
    u32 flags;
    int dead;

    if (w->flags & 0x100) {
        return 0;
    }
    if (em->type == 10 || em->type == 13) {
        if (a == 2) {
            return 0;
        }
    }
    if (w->flags & 1) {
        f32 r;
        f32 d;
        switch (w->x5EC) {
        case 0:
        case 6:
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
            r = 225000000.0f;
            break;
        default:
            r = 36000000.0f;
            break;
        }
        d = em->plDist2;
        if (em->plDist2 < r) {
            if (w->x508 < 1.0471976f) {
                find = 1;
            }
        }
        if ((s32) pG->flags_5010 < 0) {
            if (d < 25000000.0f) {
                find = 1;
            }
        }
        if (d < 12250000.0f) {
            find = 1;
        }
        if (a != 0) {
            if (fabsf(em->pos.y - pPL->pos.y) > 2000.0f) {
                find = 0;
            }
        }
    }
    if (em10SomebodyDamageNowCk(em)) {
        find = 1;
    }
    flags = em->flags_3C8;
    if (!(flags & 0x10)) {
        switch (w->x5EC) {
        case 0:
        case 6:
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
            if (pG->flags_5010 & 0x20000000) {
                f32 r = 25000.0f;
                // TODO: the target keeps a dead `cmpwi bell_stat,0 / beq / cmpwi bell_stat,1` pair here
                {
                    f32 dx = em->pos.x - pG->bell_pos.x;
                    f32 dy = em->pos.y - pG->bell_pos.y;
                    f32 dz = em->pos.z - pG->bell_pos.z;
                    if (dx * dx + dy * dy + dz * dz < r * r) {
                        if ((w->flags & 1) && w->x524 < r) {
                            find = 1;
                        }
                    }
                }
            }
            if (pG->flags_500C & 0x00800000) {
                if (w->x524 < 25000.0f) {
                    find = 1;
                }
            }
            break;
        }
    }
    dead = em10DeadCk(em);
    if (dead) {
        find = 1;
    }
    if (flags & 0x80) {
        find = 1;
    }
    switch (find) {
    case 0:
        return 0;
    case 1:
        em10SetRtnFind(em);
        return 1;
    default:
        return 0;
    }
}

void em10NeckMove(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    cModel* p;
    Mtx m;
    Vec v;
    Vec d;
    f32 a;
    f32 l;

    if (w->flags & 0x00400000) {
        return;
    }
    if (w->x58C != 0 || w->pParasite != 0) {
        w->flags &= ~0x00040000;
    }
    p = em->getPartsPtr(4);
    if (!(w->flags & 0x08000000)) {
        cModel* h = pPL->getPartsPtr(4);
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(h->mat, &v, &v);
    } else {
        cModel* h = pSUB->getPartsPtr(4);
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        PSMTXMultVec(h->mat, &v, &v);
    }
    if (w->flags & 0x00040000) {
        w->x5CC = w->x5CC * 0.9f + Muku(&em->pos, &v, em->rot.y, 1.0471976f) * 0.1f;
        PSVECSubtract(&v, &p->worldPos, &d);
        l = SQRTF(d.x * d.x + d.z * d.z);
        a = -atan2f(d.y, l);
        if (a > 0.7853982f) {
            a = 0.7853982f;
        }
        if (a < -0.7853982f) {
            a = -0.7853982f;
        }
        w->x5C8 = w->x5C8 * 0.9f + a * 0.1f;
    } else {
        w->x5C8 = w->x5C8 * 0.9f;
        w->x5CC = w->x5CC * 0.9f;
    }
    p = em->getPartsPtr(3);
    PARTS_FLAGS(p) |= 0x40000000;
    PARTS_ROT_OFS(p).x = w->x5C8;
    PARTS_ROT_OFS(p).y = w->x5CC;
    PARTS_ROT_OFS(p).z = 0.0f;
    if (w->flags & 0x02000000) {
        w->x5D0 = w->x5D0 * 0.9f + -w->x5C8 * 0.1f;
    } else {
        w->x5D0 = w->x5D0 * 0.9f;
    }
    if (!(fabsf(w->x5D0) < 0.01f)) {
        p = em->getPartsPtr(13);
        PSMTXRotRad(m, 'z', w->x5D0);
        PSMTXConcat(p->worldMat, m, p->worldMat);
    }
}

extern "C" int em10ClawCriAtkCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    u8 r;

    if (em->type != 0xA && em->type != 0xD) {
        return 0;
    }
    if (!(w->flags & 0x100)) {
        return 0;
    }
    if (w->x67C != 0) {
        return 0;
    }
    if ((s16) w->x646 != 0) {
        return 0;
    }
    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if ((w->flags & 0x08000000) && pSUB != 0) {
        return 0;
    }
    if (fabsf(em->pos.y - pPL->pos.y) > 1500.0f) {
        return 0;
    }
    if (em->plDist2 < 12250000.0f || em->plDist2 > 225000000.0f) {
        return 0;
    }
    if (!(Rnd() & 3) && !(pG->flags_5010 & 0x20000000)) {
        w->x646 = 150;
        return 0;
    }
    a = em->pos;
    b = pPL->pos;
    a.y += 1500.0f;
    b.y += 1500.0f;
    if (SatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    if (em->plDist2 > 25000000.0f) {
        EmRoutineSet(em, 1, 0x2D, 0, 0);
    } else if (em->plDist2 > 9000000.0f && (r = Rnd() % 10, r > 4)) {
        EmRoutineSet(em, 1, 0x2D, 0, 0);
    } else {
        EmRoutineSet(em, 1, 0x2C, 0, 0);
    }
    if (pG->x4F88 <= 3) {
        Ctrl12Set(w->pCtrl12, 6, 60);
        Ctrl12Set(w->pCtrl12, 8, 120);
    } else if (pG->stage_no <= 2 && pG->x4F88 <= 9) {
        Ctrl12Set(w->pCtrl12, 6, 30);
        Ctrl12Set(w->pCtrl12, 8, 120);
    }
    return 1;
}

static Vec em10_campos2_r = { 1300.0f, 500.0f, 0.0f };
static Vec em10_campos2_l = { -1300.0f, 500.0f, 0.0f };
// 0xF8 explicitly zero-initialised bytes follow in .data (GCC 2.95 keeps `= {0}` aggregates out of .bss); nothing references them.
static Camera em10_campos2_cam = { 0 };

extern "C" void em10SetCampos2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec pos;
    Vec out;
    Vec d;
    f32 h;
    f32 len;

    if (Rnd() & 1) {
        pos = em10_campos2_r;
    } else {
        pos = em10_campos2_l;
    }
    PSMTXMultVec(em->mat, &pos, &w->x608);
    pos = pPL->getPartsPtr(4)->worldPos;
    if (GetWaterHeight(&em->pos, &h) && w->x608.y < h) {
        w->x608.y = em->pos.y + 1500.0f;
    }
    if (EatMgr.hitCheck(&pos, &w->x608, &out, 0, 0, 0)) {
        PSVECSubtract(&out, &pos, &d);
        len = SQRTF(d.x * d.x + d.y * d.y + d.z * d.z) - 250.0f;
#line 32773 "D:/Bio4/Prog/em10.cpp"
        VECNormalize(&d, &d);
        PSVECScale(&d, &d, len);
        PSVECAdd(&pos, &d, &w->x608);
    }
    w->cam.param.at = pos;
    w->cam.param.pos = w->x608;
    w->cam.up.x = 0.0f;
    w->cam.up.y = 1.0f;
    w->cam.up.z = 0.0f;
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        w->cam.dist = SQRTF(dx * dx + dy * dy + dz * dz);
    }
    w->cam.param.fovy = 55.0f;
    CameraSetOrientationUp(&w->cam);
}

int em10ThrowBombCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    int hit;

    if (w->x58C != 0) {
        return 0;
    }
    if (w->pParasite != 0) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    if (w->wepType != 9) {
        return 0;
    }
    if (w->pWep == 0) {
        return 0;
    }
    if (w->x640 == 0) {
        return 0;
    }
    if (!(em->flags_3C8 & 0x00010000)) {
        return 0;
    }
    if (w->x696 == 0) {
        if ((pG->flags_51E4 & 0xF) != (em->emsetNo & 0xF)) {
            return 0;
        }
    }
    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if (Ctrl12Ck(w->pCtrl12, 8)) {
        return 0;
    }
    if (pG->x4F88 <= 3) {
        if (!em10ThrowNearCk(em)) {
            return 0;
        }
        if (!em10ScreenInCk(em)) {
            return 0;
        }
    }
    if (fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
        return 0;
    }
    if (em->plDist2 < 12250000.0f || em->plDist2 > 225000000.0f) {
        return 0;
    }
    if (fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f)) > 1.0471976f) {
        return 0;
    }
    a = em->pos;
    b = pPL->pos;
    a.y += 1500.0f;
    b.y += 1500.0f;
    hit = EatMgr.hitCheck(&a, &b, 0, 0, 0, 0x4000);
    if (hit) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x25, hit, hit);
    if (pG->x4F88 <= 3) {
        Ctrl12Set(w->pCtrl12, 6, 60);
        Ctrl12Set(w->pCtrl12, 8, 120);
    } else if (pG->stage_no <= 2 && pG->x4F88 <= 9) {
        Ctrl12Set(w->pCtrl12, 6, 30);
        Ctrl12Set(w->pCtrl12, 8, 120);
    }
    return 1;
}

// em10ScytheAtkCk / em10SukiAtkCk share one body apart from the weapon kind and the attack routine.
#define EM10_WEP_ATK_CK(em, w, kind, rtn)                                                          \
    {                                                                                              \
        Vec a;                                                                                     \
        Vec b;                                                                                     \
        int hit;                                                                                   \
        u8 r;                                                                                      \
        if (w->wepType != kind) {                                                                  \
            return 0;                                                                              \
        }                                                                                          \
        if (w->pWep == 0) {                                                                        \
            return 0;                                                                              \
        }                                                                                          \
        if (w->x67C != 0) {                                                                        \
            return 0;                                                                              \
        }                                                                                          \
        if (w->x58C != 0) {                                                                        \
            return 0;                                                                              \
        }                                                                                          \
        if (w->pParasite != 0) {                                                                   \
            return 0;                                                                              \
        }                                                                                          \
        if (w->flags & 0x80) {                                                                     \
            return 0;                                                                              \
        }                                                                                          \
        if (Ctrl12Ck(w->pCtrl12, 6)) {                                                             \
            return 0;                                                                              \
        }                                                                                          \
        if (!(w->flags & 1)) {                                                                     \
            return 0;                                                                              \
        }                                                                                          \
        if (w->x508 > 0.7853982f) {                                                                \
            return 0;                                                                              \
        }                                                                                          \
        if (fabsf(em->pos.y - pPL->pos.y) > 1500.0f) {                                             \
            return 0;                                                                              \
        }                                                                                          \
        if (em->plDist2 > 4000000.0f) {                                                            \
            if (!em10PlRunCk(em)) {                                                                \
                return 0;                                                                          \
            }                                                                                      \
            if (em->plDist2 > 20250000.0f) {                                                       \
                return 0;                                                                          \
            }                                                                                      \
        }                                                                                          \
        if (pG->x4F88 <= 3) {                                                                      \
            if (!em10ScreenInCk(em)) {                                                             \
                return 0;                                                                          \
            }                                                                                      \
        }                                                                                          \
        a = em->pos;                                                                               \
        b = pPL->pos;                                                                              \
        a.y += 1500.0f;                                                                            \
        b.y += 1500.0f;                                                                            \
        hit = EatMgr.hitCheck(&a, &b, 0, 0, 0, 0x4000);                                            \
        if (hit) {                                                                                 \
            return 0;                                                                              \
        }                                                                                          \
        if (pG->x4F88 <= 1 && !EM_RTN(em, 1, 0x1B) && (r = Rnd() % 10, r > 4)) {                   \
            w->x67C = 30;                                                                          \
            EmRoutineSet(em, 1, 0x1B, hit, hit);                                                   \
            return 1;                                                                              \
        }                                                                                          \
        EmRoutineSet(em, 1, rtn, 0, 0);                                                            \
        if (pG->x4F88 <= 3) {                                                                      \
            Ctrl12Set(w->pCtrl12, 6, 60);                                                          \
            Ctrl12Set(w->pCtrl12, 8, 120);                                                         \
        } else if (pG->stage_no <= 2 && pG->x4F88 <= 9) {                                          \
            Ctrl12Set(w->pCtrl12, 6, 30);                                                          \
            Ctrl12Set(w->pCtrl12, 8, 120);                                                         \
        }                                                                                          \
        return 1;                                                                                  \
    }

extern "C" int em10ScytheAtkCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EM10_WEP_ATK_CK(em, w, 6, 0x2A);
}

extern "C" int em10SukiAtkCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EM10_WEP_ATK_CK(em, w, 1, 0x29);
}

int em10RackBreakCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;
    Mtx inv;
    Vec v;
    Vec a;
    Vec b;

    if (w->x634 % 15 != 8) {
        return 0;
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEmRack* e = (cEmRack*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if (!(e->be_flag & 1)) {
            continue;
        }
        if (!(e->be_flag & 0x20)) {
            continue;
        }
        if (e->id != 0x45) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if ((em->pos.x - e->pos.x) * (em->pos.x - e->pos.x) + (em->pos.y - e->pos.y) * (em->pos.y - e->pos.y) + (em->pos.z - e->pos.z) * (em->pos.z - e->pos.z) > 4000000.0f) {
            continue;
        }
        if (fabsf(Muku(&em->pos, &e->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
            continue;
        }
        PSMTXInverse(e->mat, inv);
        PSMTXMultVec(inv, &em->pos, &v);
        if (v.x > 1000.0f || v.x < -1000.0f) {
            continue;
        }
        if (v.y > 500.0f || v.y < -500.0f) {
            continue;
        }
        if (v.z > 1200.0f || v.z < -1200.0f) {
            continue;
        }
        if (w->flags & 0x4000) {
            e->setDown(&em->pos);
            return 1;
        }
        if (!em10DootAtkCk(em)) {
            return 0;
        }
        a = em->pos;
        b = e->pos;
        b.y = a.y = em->pos.y + 1500.0f;
        if (SatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
            continue;
        }
        EmRoutineSet(em, 1, e->type == 0 ? 0x3E : 0x3D, 0, 0);
        return 1;
    }
    return 0;
}

extern "C" int em10JumpDownCk2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec n;
    Vec a;
    Vec b;
    f32 ang;
    int hit;

    if (w->x634 % 15 != 6) {
        return 0;
    }
    if (w->x6AF == 0) {
        if ((w->flags & 0x08000000) && pSUB != 0) {
            if (w->x514 < 25000000.0f) {
                if (pSUB->pos.y > em->pos.y - 500.0f) {
                    return 0;
                }
            }
        } else {
            if (em->plDist2 < 25000000.0f && pPL->pos.y > em->pos.y - 500.0f) {
                return 0;
            }
        }
    }
    a.x = 100.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    if (w->pShield) {
        b.x = 100.0f;
        b.y = 500.0f;
        b.z = 700.0f;
    } else {
        b.x = 100.0f;
        b.y = 500.0f;
        b.z = 500.0f;
    }
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    hit = SatMgr.hitCheck(&a, &b, 0, &n, 0, 0);
    if (hit & 0x142010) {
        ang = atan2f(-n.x, -n.z);
        if (fabsf(Muku2(em->rot.y, ang, 3.1415927f)) < 0.5235988f) {
            goto found;
        }
    }
    a.x = -100.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    if (w->pShield) {
        b.x = -100.0f;
        b.y = 500.0f;
        b.z = 700.0f;
    } else {
        b.x = -100.0f;
        b.y = 500.0f;
        b.z = 500.0f;
    }
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    hit = SatMgr.hitCheck(&a, &b, 0, &n, 0, 0);
    if (hit & 0x142010) {
        ang = atan2f(-n.x, -n.z);
        if (fabsf(Muku2(em->rot.y, ang, 3.1415927f)) < 0.5235988f) {
found:
            em->rot.y = ang;
            if (hit & 0x40000) {
                return 2;
            }
            return 1;
        }
    }
    return 0;
}

void cEm10::setReset()
{
    Em10Work* w = EM10_WK(this);
    f32 sc;
    cModel* p;
    cModelInfo* info;

    alpha = 1.0f;
    be_flag = (be_flag | 2) & ~0x10000;
    atari.flags |= 0x300;
    atari.flags &= ~0x10;
    em10HeadSet(this, 0);
    switch (emsetNo & 0xF) {
    default:
        sc = fRand1_1() * 0.01f + 1.03f;
        break;
    case 7:
        sc = 1.1f;
        break;
    case 0:
    case 4:
    case 9:
    case 0xB:
        sc = fRand1_1() * 0.01f + 1.01f;
        break;
    }
    if (type == 6) {
        sc = 1.05f;
    }
    scale.z = sc;
    scale.y = sc;
    scale.x = sc;
    w->scaleBase = scale;
    p = getPartsPtr(0x1B);
    p->scale.x = 1.0f;
    p->scale.y = 1.0f;
    p->scale.z = 1.0f;
    pos = w->startPos;
    oldPos = pos;
    rot.x = 0.0f;
    rot.y = w->startRotY;
    rot.z = 0.0f;
    hp = hpMax;
    w->x4D8 = w->startPos;
    x38D = w->x4C4;
    if (w->x1A0) {
        w->x1A0->be_flag |= 8;
    }
    if (w->x1A4) {
        w->x1A4->be_flag |= 8;
    }
    if (w->x1A8) {
        w->x1A8->be_flag |= 8;
    }
    if (w->x1AC) {
        w->x1AC->be_flag |= 8;
    }
    if (w->x1B4) {
        w->x1B4->be_flag |= 8;
    }
    if (w->x1B8) {
        w->x1B8->be_flag |= 8;
    }
    if (w->x1BC) {
        w->x1BC->be_flag |= 8;
    }
    if (w->x1C0) {
        w->x1C0->be_flag |= 8;
    }
    em10WeaponInit(this);
    em10ShieldSet(this);
    for (info = pInfo; info; info = info->pNext) {
        info->color[0] = 0xFF;
        info->color[1] = 0xFF;
        info->color[2] = 0xFF;
    }
    em10InitRtnSet(this);
    hitInfo.flags = 1;
    MotionMoveF(this, 0);
    em10_R0_Move(this);
    partsWorldCalc();
    for (p = pParts; p; p = p->pParts) {
        p->oldWorldPos = p->worldPos;
        p->x88 = p->oldWorldPos;
    }
}

extern "C" int em10ClimbOverCk2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    Vec c;
    Vec n;
    Mtx m;
    Vec d;
    Vec e1;
    Vec e2;
    int side;
    f32 ang;
    f32 y;

    if (pG->flags_5014 & 0x08000000) {
        return 0;
    }
    if (w->x634 % 15 != 11) {
        return 0;
    }
    if (w->x5EC == 0 && em->x3D0 == 5) {
        return 0;
    }
    a.x = 0.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    b.x = 0.0f;
    b.y = 500.0f;
    b.z = 800.0f;
    PSMTXMultVec(em->mat, &a, &a);
    PSMTXMultVec(em->mat, &b, &b);
    if (!(SatMgr.hitCheck(&a, &b, &c, &n, 0, 0) & 0x20)) {
        return 0;
    }
    side = 0;
    ang = atan2f(-n.x, -n.z);
    em->rot.y = ang;
    w->x5E0 = em->pos;
    PSMTXRotRad(m, 'y', ang);
    TransMatrix(m, &em->pos);
    e1.x = 300.0f;
    e1.y = 1200.0f;
    e1.z = 0.0f;
    e2.x = 300.0f;
    e2.y = 1200.0f;
    e2.z = 800.0f;
    PSMTXMultVec(m, &e1, &e1);
    PSMTXMultVec(m, &e2, &e2);
    if (SatMgr.hitCheck(&e1, &e2, 0, 0, 0, 0x20)) {
        side = 1;
    }
    e1.x = -300.0f;
    e1.y = 1200.0f;
    e1.z = 0.0f;
    e2.x = -300.0f;
    e2.y = 1200.0f;
    e2.z = 800.0f;
    PSMTXMultVec(m, &e1, &e1);
    PSMTXMultVec(m, &e2, &e2);
    if (SatMgr.hitCheck(&e1, &e2, 0, 0, 0, 0x20)) {
        side |= 2;
    }
    if (side == 3) {
        return 0;
    }
    if (side & 1) {
        d.x = -300.0f;
        d.y = 0.0f;
        d.z = 0.0f;
        PSMTXMultVec(m, &d, &w->x5E0);
    }
    if (side & 2) {
        d.x = 300.0f;
        d.y = 0.0f;
        d.z = 0.0f;
        PSMTXMultVec(m, &d, &w->x5E0);
    }
    d.x = 0.0f;
    d.y = 0.0f;
    d.z = 1000.0f;
    PSMTXMultVec(m, &d, &d);
    y = SatMgr.getFloor(&d, 600.0f, 100000.0f, 0, 0);
    if (y < em->pos.y - 500.0f) {
        return 2;
    }
    return 1;
}

// .data 0x640: gatling attack parameters (only [0] is used).
static EmAtkInfo em10_gatling_atk[2] = {
    { 200.0f, 8, 900, 0, 10, 0 },
    { 200.0f, 8, 800, 0, 10, 0 },
};

extern "C" int em10GatlingHitCk(cEm10* em)
{
    Vec a;
    Vec b;
    EmAtkInfo info;
    Vec hit;
    Vec nrm;
    Vec dir;
    Vec rot;
    Vec s;
    u32 attr;
    cEm* e;
    cModel* p;
    f32 l;

    EstSet((int) em, -1, 0, 0, 0xCC, 0, 0, 0, (u32) em, 0);
    SndCall(6, 9, &em->pos, 0, 0, 0);
    a.x = 0.0f;
    a.y = 0.0f;
    a.z = -80.0f;
    b.x = -50000.0f;
    b.y = fRand1_1() * 500.0f;
    b.z = fRand1_1() * 500.0f;
    p = em->getPartsPtr(10);
    PSMTXMultVec(p->mat, &a, &a);
    PSMTXMultVec(p->mat, &b, &b);
    em->dmType = 1;
    PlWepHitCheck2(0, &a, &b, 0xC, 3, 6000.0f);
    em->dmType = 0;
    e = EmAtkLineHitCk(&a, &b, &hit, &nrm, &attr);
    if (e != 0) {
        VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 7, 1);
        SndCall(8, 0x81, &pPL->getPartsPtr(0)->worldPos, em->id, 0, pPL);
        QuakeExec(0, 0, 5, 22.0f, 2);
        EmPlBloodSet2(em, &a, 1, 0xCC, 2);
        info = em10_gatling_atk[0];
        EmAtkSetDamagePL(e, &info, &a, &b);
        return 1;
    }
    l = SQRTF(nrm.x * nrm.x + nrm.z * nrm.z);
    rot.x = -atan2f(nrm.y, l);
    rot.y = atan2f(nrm.x, nrm.z);
    rot.z = 0.0f;
    PSVECScale(&nrm, &dir, 30.0f);
    PSVECAdd(&hit, &dir, &hit);
    EstSet(0, -1, &hit, &rot, 0xCC, 1, 0, 0, 0, 0);
    PSVECSubtract(&hit, &a, &s);
    EspSetGatling(a, s);
    SndCall(6, 0xA, &hit, 0, 0, 0);
    return 0;
}

#define EMI_DATA ((EmiData*) pG->pRoomEmi)

int em10GotoPosCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;
    u32 j;
    u32 k;
    int found;
    u32 cnt;
    EmiEntry* e;
    EmiEntry* f;
    EmiEntry* g;

    if (pG->pRoomEmi == 0) {
        return 0;
    }
    if (w->x5EC != 0) {
        return 0;
    }
    for (i = 0; i < EMI_DATA->n; i++) {
        e = &EMI_DATA->entry[i];
        if (e->type != 0xF) {
            continue;
        }
        if (e->sub != 0) {
            continue;
        }
        {
            f32 dx = em->pos.x - e->pos.x;
            f32 dy = em->pos.y - e->pos.y;
            f32 dz = em->pos.z - e->pos.z;
            if (dx * dx + dy * dy + dz * dz > 9000000.0f) {
                continue;
            }
        }
        found = 0;
        for (j = 0; j < EMI_DATA->n; j++) {
            f = &EMI_DATA->entry[j];
            if (f->type != 0xF) {
                continue;
            }
            if (f->sub != 2) {
                continue;
            }
            if (f->pad_3 != e->pad_3) {
                continue;
            }
            {
                f32 dx = f->pos.x - pPL->pos.x;
                f32 dy = f->pos.y - pPL->pos.y;
                f32 dz = f->pos.z - pPL->pos.z;
                if (dx * dx + dy * dy + dz * dz < 9000000.0f) {
                    found = 1;
                    break;
                }
            }
        }
        if (!found) {
            continue;
        }
        cnt = 0;
        for (k = 0; k < EmMgr.nArray; k++) {
            cEm* o = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * k);
            if ((o->be_flag & 0x201) != 1) {
                continue;
            }
            if (o->id <= 0xF) {
                continue;
            }
            if (o->id > 0x20) {
                continue;
            }
            if (o->hp <= 0) {
                continue;
            }
            if (o == em) {
                continue;
            }
            {
                f32 dx = o->pos.x - e->pos.x;
                f32 dy = o->pos.y - e->pos.y;
                f32 dz = o->pos.z - e->pos.z;
                if (dx * dx + dy * dy + dz * dz > 25000000.0f) {
                    continue;
                }
            }
            if (EM_RTN(o, 1, 0x13)) {
                continue;
            }
            cnt++;
        }
        if (cnt < e->state) {
            return 0;
        }
        for (k = 0; k < EMI_DATA->n; k++) {
            g = &EMI_DATA->entry[k];
            if (g->type != 0xF) {
                continue;
            }
            if (g->sub != 1) {
                continue;
            }
            if (g->pad_3 != e->pad_3) {
                continue;
            }
            if (!(em->flags_3C8 & 0x40)) {
                w->flags &= ~0x100;
            }
            em->setGoto(&g->pos, 0xC);
            return 1;
        }
    }
    return 0;
}

int em10HideRtnCk2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;
    EmiEntry* e;

    if (pG->pRoomEmi == 0) {
        return 0;
    }
    for (i = 0; i < EMI_DATA->n; i++) {
        e = &EMI_DATA->entry[i];
        if (e->type != 1) {
            continue;
        }
        switch (e->sub) {
        case 0:
            {
                f32 dx = e->pos.x - em->pos.x;
                f32 dy = e->pos.y - em->pos.y;
                f32 dz = e->pos.z - em->pos.z;
                if (dx * dx + dy * dy + dz * dz > 1000000.0f) {
                    continue;
                }
            }
            if (fabsf(Muku(&e->pos, &pPL->pos, e->rotY, 3.1415927f)) > 1.0471976f) {
                continue;
            }
            {
                f32 dx = e->pos.x - pPL->pos.x;
                f32 dy = e->pos.y - pPL->pos.y;
                f32 dz = e->pos.z - pPL->pos.z;
                if (dx * dx + dy * dy + dz * dz < 25000000.0f) {
                    continue;
                }
            }
            em->rot.y = e->rotY;
            EmRoutineSet(em, 1, 0x18, 0, 0);
            return 1;
        case 1:
            {
                f32 dx = e->pos.x - em->pos.x;
                f32 dy = e->pos.y - em->pos.y;
                f32 dz = e->pos.z - em->pos.z;
                if (dx * dx + dy * dy + dz * dz > 1000000.0f) {
                    continue;
                }
            }
            if (fabsf(Muku(&e->pos, &pPL->pos, e->rotY, 3.1415927f)) > 1.0471976f) {
                continue;
            }
            {
                f32 dx = e->pos.x - pPL->pos.x;
                f32 dy = e->pos.y - pPL->pos.y;
                f32 dz = e->pos.z - pPL->pos.z;
                if (dx * dx + dy * dy + dz * dz < 25000000.0f) {
                    continue;
                }
            }
            em->rot.y = e->rotY;
            EmRoutineSet(em, 1, 0x18, 0, 1);
            return 1;
        case 2:
            if (w->wepType != 8) {
                continue;
            }
            {
                f32 dx = e->pos.x - em->pos.x;
                f32 dy = e->pos.y - em->pos.y;
                f32 dz = e->pos.z - em->pos.z;
                if (dx * dx + dy * dy + dz * dz > 1000000.0f) {
                    continue;
                }
            }
            if (fabsf(Muku(&e->pos, &pPL->pos, e->rotY, 3.1415927f)) > 1.0471976f) {
                continue;
            }
            {
                f32 dx = e->pos.x - pPL->pos.x;
                f32 dy = e->pos.y - pPL->pos.y;
                f32 dz = e->pos.z - pPL->pos.z;
                if (dx * dx + dy * dy + dz * dz < 25000000.0f) {
                    continue;
                }
            }
            em->rot.y = e->rotY;
            EmRoutineSet(em, 1, 0x1A, 0, 0);
            return 1;
        }
    }
    return 0;
}

extern "C" int em10ThrowAxeCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    int hit;

    if (w->x58C != 0) {
        return 0;
    }
    if (w->pParasite != 0) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    switch (w->wepType) {
    case 2:
    case 3:
    case 5:
    case 6:
        break;
    default:
        return 0;
    }
    if (w->pWep == 0) {
        return 0;
    }
    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01000000) {
        return 0;
    }
    if (w->x696 == 0) {
        if ((pG->flags_51E4 & 0xF) != (em->emsetNo & 0xF)) {
            return 0;
        }
    }
    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if (Ctrl12Ck(w->pCtrl12, 8)) {
        return 0;
    }
    if (pG->x4F88 <= 3) {
        if (!em10ThrowNearCk(em)) {
            return 0;
        }
        if (!em10ScreenInCk(em)) {
            return 0;
        }
    }
    if (fabsf(Muku(&em->pos, &pPL->pos, em->rot.y, 3.1415927f)) > 0.7853982f) {
        return 0;
    }
    if (em->flags_3C8 & 0x00010000) {
        if (em->plDist2 < 9000000.0f || em->plDist2 > 100000000.0f) {
            return 0;
        }
        if (fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f)) > 1.0471976f) {
            return 0;
        }
    } else {
        if (em->plDist2 < 20250000.0f || em->plDist2 > 49000000.0f) {
            return 0;
        }
        if (fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f)) > 0.3926991f) {
            return 0;
        }
        if (Rnd() & 1) {
            return 0;
        }
    }
    a = em->pos;
    b = pPL->pos;
    a.y += 1500.0f;
    b.y += 1500.0f;
    hit = EatMgr.hitCheck(&a, &b, 0, 0, 0, 0x4000);
    if (hit) {
        return 0;
    }
    EmRoutineSet(em, 1, 0x24, hit, hit);
    if (pG->x4F88 <= 3) {
        Ctrl12Set(w->pCtrl12, 6, 60);
        Ctrl12Set(w->pCtrl12, 8, 120);
    } else if (pG->stage_no <= 2 && pG->x4F88 <= 9) {
        Ctrl12Set(w->pCtrl12, 6, 30);
        Ctrl12Set(w->pCtrl12, 8, 120);
    }
    return 1;
}

void em10CamMove2(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec pl;
    Vec d;
    Vec hit;
    Vec d2;
    Camera* c = &pG->Cam;

    pl = pPL->getPartsPtr(4)->worldPos;
    PSVECSubtract(&w->x608, &pl, &d);
    if (d.x * d.x + d.z * d.z > 2890000.0f) {
        d.y = 0.0f;
#line 32818 "D:/Bio4/Prog/em10.cpp"
        VECNormalize(&d, &d);
        PSVECScale(&d, &d, 1700.0f);
        PSVECAdd(&pl, &d, &d);
        d.y = w->x608.y;
        PosToPos(&w->x608, &d, &w->x608, 0.2f);
    }
    PosToPos(&c->param.at, &pl, &w->cam.param.at, 1.0f);
    PosToPos(&c->param.pos, &w->x608, &w->cam.param.pos, 1.0f);
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        if (dx * dx + dy * dy + dz * dz > 100.0f) {
            PSVECSubtract(&w->cam.param.pos, &w->cam.param.at, &d2);
#line 32836 "D:/Bio4/Prog/em10.cpp"
            VECNormalize(&d2, &d2);
            PSVECScale(&d2, &d2, 250.0f);
            PSVECAdd(&w->cam.param.pos, &d2, &w->cam.param.pos);
            if (EatMgr.hitCheck(&w->cam.param.at, &w->cam.param.pos, &hit, 0, 0x8000, 0)) {
                w->cam.param.pos = hit;
            }
            PSVECSubtract(&w->cam.param.pos, &d2, &w->cam.param.pos);
        }
    }
    w->cam.up.x = 0.0f;
    w->cam.up.y = 1.0f;
    w->cam.up.z = 0.0f;
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        w->cam.dist = SQRTF(dx * dx + dy * dy + dz * dz);
    }
    w->cam.param.fovy = 55.0f;
    CameraSetOrientationUp(&w->cam);
    CamCtrl.x250 = (s32) &w->cam;
}

int em10CatchCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx inv;
    Vec v;
    Vec a;
    Vec b;
    Mtx m;

    if (em10DeadCk(pPL)) {
        return 0;
    }
    if ((s16) pG->pl_life <= 0) {
        return 0;
    }
    if (em->hp <= 0) {
        return 0;
    }
    if (!(em->seFlags28B & 2)) {
        return 0;
    }
    if (!(w->flags & 1)) {
        return 0;
    }
    if (w->x6C1 > 0x2D) {
        return 0;
    }
    if (pG->flags_5010 & 0x8000) {
        return 0;
    }
    if ((w->flags & 0x80) && (em->flags_3C8 & 0x100000)) {
        return 0;
    }
    PSMTXInverse(em->mat, inv);
    PSMTXMultVec(inv, &pPL->pos, &v);
    if (v.y < -500.0f || v.y > 500.0f) {
        return 0;
    }
    if (v.z < 0.0f || v.z > 900.0f) {
        return 0;
    }
    if (!(v.x > -400.0f)) {
        return 0;
    }
    if (!(v.x < 400.0f)) {
        return 0;
    }
    a = em->pos;
    b = pPL->pos;
    a.y += 500.0f;
    b.y += 500.0f;
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    PSMTXRotRad(m, 'y', GetXZAngle(&em->pos, &pPL->pos));
    TransMatrix(m, &em->pos);
    a.x = 300.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    b.x = 300.0f;
    b.y = 500.0f;
    b.z = 500.0f;
    PSMTXMultVec(m, &a, &a);
    PSMTXMultVec(m, &b, &b);
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    a.x = 300.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    b.x = 300.0f;
    b.y = 500.0f;
    b.z = 500.0f;
    PSMTXMultVec(m, &a, &a);
    PSMTXMultVec(m, &b, &b);
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    pPL->dmg.set(0, 2);
    em->dmg.set(0, 2);
    VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 7, 1);
    Ctrl12Set(w->pCtrl12, 6, 30);
    Ctrl12Set(w->pCtrl12, 8, 120);
    return 1;
}

// .data 0x610: chainsaw attack parameters (only [0] is used).
static EmAtkInfo em10_csaw_atk[3] = {
    { 500.0f, 8, 9999, 8, 10, 0 },
    { 400.0f, 8, 640, 0, 10, 0 },
    { 400.0f, 8, 1400, 0, 10, 0 },
};

int em10CsawHitCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    EmAtkInfo info;
    Vec a;
    Vec b;
    int hit;

    if (!(em->seFlags28B & 1)) {
        return 0;
    }
    if (w->pWep == 0) {
        return 0;
    }
    info = em10_csaw_atk[0];
    a.x = 0.0f;
    a.y = 0.0f;
    a.z = 500.0f;
    b.x = 0.0f;
    b.y = 0.0f;
    b.z = -1000.0f;
    PSMTXMultVec(w->pWep->mat, &a, &a);
    PSMTXMultVec(w->pWep->mat, &b, &b);
    hit = EmAtkHitCk(&info, &a, &b, 0);
    if (hit & 1) {
        U16Set(pG->pl_life, 1);
        pPL->dmg.set(0, 0x80);
        em->dmg.set(0, 0x80);
        VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 7, 1);
        return 1;
    } else if (hit & 2) {
        LifeDownSet(pSUB, 9999, 0);
        VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 7, 1);
    } else {
        a.x = 0.0f;
        a.y = 0.0f;
        a.z = 0.0f;
        b.x = 0.0f;
        b.y = 0.0f;
        b.z = -1000.0f;
        PSMTXMultVec(w->pWep->mat, &a, &a);
        PSMTXMultVec(w->pWep->mat, &b, &b);
        hit = EmAtkHitCk(&info, &a, &b, 0);
        if (hit & 1) {
            U16Set(pG->pl_life, 1);
            pPL->dmg.set(0, 0x80);
            em->dmg.set(0, 0x80);
            VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 7, 1);
            return 1;
        } else if (hit & 2) {
            LifeDownSet(pSUB, 9999, 0);
            VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 7, 1);
        } else {
            a.x = 0.0f;
            a.y = 0.0f;
            a.z = -200.0f;
            b.x = 0.0f;
            b.y = 0.0f;
            b.z = -1000.0f;
            PSMTXMultVec(w->pWep->mat, &a, &a);
            PSMTXMultVec(w->pWep->mat, &b, &b);
            a.y = em->pos.y + 500.0f;
            hit = EmAtkHitCk(&info, &a, &b, 0);
            if (hit & 1) {
                U16Set(pG->pl_life, 1);
                pPL->dmg.set(0, 0x80);
                em->dmg.set(0, 0x80);
                VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 0xD, 1);
                return 1;
            } else if (hit & 2) {
                LifeDownSet(pSUB, 9999, 0);
                VibSetData((VibDataTbl*) (pG->pArc->ofs_1C + (u32) pG->pArc), 0xD, 1);
                return 0;
            }
        }
    }
    return 0;
}

int em10DoorOpenCk(cEm10* em, int kick)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    u32 i;
    f32 ang;

    if (w->x634 % 15 != 9) {
        return 0;
    }
    if (w->x644 != 0) {
        kick = 1;
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEmDoor* e = (cEmDoor*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        EmDoorWork* dw;
        if ((e->be_flag & 0x201) != 1) {
            continue;
        }
        if (e->id != 0x41) {
            continue;
        }
        if (e->hp <= 0) {
            continue;
        }
        if ((em->pos.x - e->pos.x) * (em->pos.x - e->pos.x) + (em->pos.y - e->pos.y) * (em->pos.y - e->pos.y) + (em->pos.z - e->pos.z) * (em->pos.z - e->pos.z) > 6250000.0f) {
            continue;
        }
        dw = EMDOOR_WK(e);
        ang = fabsf(Muku2(dw->rotY, em->rot.y, 3.1415927f));
        if (ang > 0.7853982f && ang < 2.3561945f) {
            continue;
        }
        PSMTXMultVec(dw->inv, &em->pos, &v);
        if (ang < 1.5707964f) {
            if (v.z > 0.0f || v.z < -800.0f) {
                continue;
            }
        } else {
            if (v.z < 0.0f || v.z > 800.0f) {
                continue;
            }
        }
        if (v.x > dw->width || v.x < -dw->width) {
            continue;
        }
        if (v.y > 500.0f || v.y < -500.0f) {
            continue;
        }
        switch (e->ckOpen()) {
        case 0:
        default:
            if (w->flags & 0x4000) {
                e->setOpen(&em->pos, 0, 0, 0);
                return 1;
            }
            if (em->plDist2 < 25000000.0f && kick == 0) {
                if (!em10DootAtkCk(em)) {
                    return 0;
                }
                em->xFC = 1;
                em->xFD = 0x3D;
                em->xFE = 0;
                em->xFF = 0;
                return 1;
            }
            e->setOpen(&em->pos, 0, 0, 0);
            return 0;
        case 2:
            if (w->flags & 0x4000) {
                e->setOpen(&em->pos, 0, 0, 0);
                return 1;
            }
            if (em10DootAtkCk(em)) {
                EmRoutineSet(em, 1, 0x3D, 0, 0);
                return 1;
            }
            return 0;
        case 1:
        case 3:
            return 0;
        }
    }
    return 0;
}

extern "C" int em10ParasiteAtkCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    Vec c;
    f32 dist;

    if (em->type == 0xA || em->type == 0xD) {
        return 0;
    }
    if (w->x58C == 0 && w->pParasite == 0) {
        return 0;
    }
    if (w->x67C != 0) {
        return 0;
    }
    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if (w->pParasite && !w->pParasite->ckAtkEnable()) {
        return 0;
    }
    if (w->x58C && !w->x58C->vB8()) {
        return 0;
    }
    if (w->x6C5 == 1) {
        dist = 4000000.0f;
    } else {
        dist = 9000000.0f;
    }
    if (w->x58C) {
        if ((s16) w->x648 != 0) {
            dist = 1690000.0f;
        } else {
            dist = 12250000.0f;
        }
    }
    if (!(w->flags & 0x08000000)) {
        if (!(w->flags & 1)) {
            return 0;
        }
        if (em->plDist2 > dist) {
            return 0;
        }
        if (fabsf(em->pos.y - pPL->pos.y) > 1500.0f) {
            return 0;
        }
        if (w->x508 > 0.7853982f) {
            return 0;
        }
        a = em->pos;
        b = pPL->pos;
        a.y += 1500.0f;
        b.y += 1500.0f;
        if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
            return 0;
        }
    } else {
        if (!(w->flags & 2)) {
            return 0;
        }
        if (w->x514 > 2250000.0f) {
            return 0;
        }
        if (fabsf(em->pos.y - pSUB->pos.y) > 1500.0f) {
            return 0;
        }
        if (w->x510 > 0.7853982f) {
            return 0;
        }
        a = em->pos;
        c = pSUB->pos;
        a.y += 1500.0f;
        c.y += 1500.0f;
        if (EatMgr.hitCheck(&a, &c, 0, 0, 0, 0x4000)) {
            return 0;
        }
    }
    EmRoutineSet(em, 1, 0x20, 0, 0);
    if (pG->x4F88 <= 3) {
        Ctrl12Set(w->pCtrl12, 6, 60);
        Ctrl12Set(w->pCtrl12, 8, 120);
    } else if (pG->stage_no <= 2 && pG->x4F88 <= 9) {
        Ctrl12Set(w->pCtrl12, 6, 30);
        Ctrl12Set(w->pCtrl12, 8, 120);
    }
    return 1;
}

int em10VLadderClimbCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec pos;
    s8 level;
    f32 ang;
    u32 i;

    if ((G_ROOM_ID32 & 0xFFFF0000) == 0x01010000 || (G_ROOM_ID32 & 0xFFFF0000) == 0x01110000 || (G_ROOM_ID32 & 0xFFFF0000) == 0x04000000) {
        return 0;
    }
    switch (em->id) {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x16:
    case 0x17:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
        break;
    default:
        return 0;
    }
    if (w->x634 % 15 != 5) {
        return 0;
    }
    if (w->x5EC == 0 && em->x3D0 == 5) {
        return 0;
    }
    if (w->x54C.y - em->pos.y < 1000.0f) {
        return 0;
    }
    if (!SceAtSearchLadder(em, &pos, &ang, (u8*) &level)) {
        return 0;
    }
    if ((em->pos.x - pos.x) * (em->pos.x - pos.x) + (em->pos.y - pos.y) * (em->pos.y - pos.y) + (em->pos.z - pos.z) * (em->pos.z - pos.z) > 1000000.0f) {
        return 0;
    }
    if (fabsf(Muku2(em->rot.y, ang, 3.1415927f)) > 1.5707964f) {
        return 0;
    }
    if ((em->pos.x - pPL->pos.x) * (em->pos.x - pPL->pos.x) + (em->pos.y - pPL->pos.y) * (em->pos.y - pPL->pos.y) + (em->pos.z - pPL->pos.z) * (em->pos.z - pPL->pos.z) < 4000000.0f) {
        return 0;
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* o = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((o->be_flag & 0x201) != 1) {
            continue;
        }
        if (o->id <= 0xF) {
            continue;
        }
        if (o->id > 0x20) {
            continue;
        }
        if (o->hp <= 0) {
            continue;
        }
        if (o == em) {
            continue;
        }
        if (!o->checkStatus(5)) {
            continue;
        }
        if (o->xFC != 1) {
            continue;
        }
        if (o->xFD != 0x41) {
            continue;
        }
        if (!((em->pos.x - o->pos.x) * (em->pos.x - o->pos.x) + (em->pos.y - o->pos.y) * (em->pos.y - o->pos.y) + (em->pos.z - o->pos.z) * (em->pos.z - o->pos.z) > 4000000.0f)) {
            return 0;
        }
    }
    w->x5DC = ang;
    w->x5E0 = pos;
    if (em->type == 2 || em->type == 0x16) {
        w->x5DC = ang;
        w->x5E0 = pos;
        w->x5E0.y += (f32) level * 1000.0f;
        EmRoutineSet(em, 1, 0x45, 0, 0);
        return 1;
    }
    EmRoutineSet(em, 1, 0x41, level, 0);
    return 1;
}

extern "C" void em10WeaponInit(cEm10* em)
{
    Em10Work* w = EM10_WK(em);

    w->wepType = 0;
    w->wep2Type = 0;
    w->pWep = 0;
    w->pWep2 = 0;
    if (em->type == 0xA || em->type == 0xD || em->type == 2) {
        return;
    }
    if (em->flags_3C8 & 0x40000000) {
        if (w->x6C5 == 0) {
            if (w->mot[43] && w->mot[44]) {
                w->wepType = 5;
            }
        } else {
            w->wepType = 0xC;
        }
    }
    if (w->mot[69] && w->mot[70] && (em->flags_3C8 & 0x04000000) && w->pWep == 0) {
        if (w->x6C5 == 2) {
            w->wepType = 0xF;
        } else {
            w->wepType = 6;
        }
    }
    if (w->mot[41] && w->mot[42] && w->x6C5 == 0 && (s32) em->flags_3C8 < 0 && w->pWep == 0) {
        w->wepType = 1;
    }
    if (w->mot[65] && w->mot[66] && (em->flags_3C8 & 0x20000000) && w->pWep == 0) {
        if (w->x6C5 == 0) {
            if (!(em->flags_3C8 & 0x2000)) {
                w->wepType = 2;
            }
        } else {
            em->flags_3C8 &= ~0x2000;
            w->wepType = 0xB;
        }
    }
    if (w->mot[63] && w->mot[64] && (em->flags_3C8 & 0x08000000) && w->pWep == 0 && !(em->flags_3C8 & 0x2000)) {
        if (w->x6C5 == 2) {
            w->wepType = 2;
        } else {
            w->wepType = 3;
        }
    }
    if (w->mot[77] && w->mot[78] && (em->flags_3C8 & 0x4000) && w->pWep == 0 && !(em->flags_3C8 & 0x2000)) {
        w->wepType = 0xA;
    }
    if (w->mot[67] && w->mot[68] && (em->flags_3C8 & 0x10000000) && w->pWep == 0) {
        w->wepType = 4;
    }
    if (w->mot[71] && w->mot[72] && (em->flags_3C8 & 0x800) && w->pWep == 0) {
        if (w->x6C5 == 2) {
            w->wepType = 0x10;
        } else {
            w->wepType = 7;
        }
    }
    if (w->mot[73] && w->mot[74] && em->type == 6) {
        em->flags_3C8 |= 0x0001A000;
    }
    if ((em->flags_3C8 & 0x8000) && w->pWep == 0) {
        w->x6B5 = 2;
        if (!(em->flags_3C8 & 0x2000)) {
            w->wepType = 8;
        }
    }
    if ((em->flags_3C8 & 0x00020000) && w->pWep == 0) {
        if (!(em->flags_3C8 & 0x2000)) {
            w->wepType = 9;
        }
        em->flags_3C8 &= ~0x00100000;
    }
    w->pWep = em10MakeWeapon(em, w->wepType);
    if (w->pWep == 0) {
        w->wepType = 0;
    }
    em10WeaponSet(em);
    em10WeaponSet2(em);
}

extern "C" void em10CamMoveAshley(cEm10* em, u32 no)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    Vec hit;
    Vec d;
    Camera* c = &pG->Cam;

    switch (no) {
    case 0:
    default:
        a.x = 957.0f;
        a.y = 1872.0f;
        a.z = 676.0f;
        b.x = -122.0f;
        b.y = 1622.0f;
        b.z = 49.0f;
        break;
    case 1:
        a.x = -1105.0f;
        a.y = 1590.1884f;
        a.z = -686.0f;
        b.x = -114.0f;
        b.y = 1589.0f;
        b.z = 54.0f;
        break;
    case 2:
        a.x = -1665.0f;
        a.y = 1750.1f;
        a.z = 1387.4f;
        b.x = -39.4f;
        b.y = 1339.7f;
        b.z = 376.8f;
        break;
    case 3:
        a.x = 1563.0f;
        a.y = 976.6f;
        a.z = -693.4f;
        b.x = -71.6f;
        b.y = 1339.7f;
        b.z = 318.0f;
        break;
    }
    FSet(w->cam.param.fovy, 50.0f);
    PSMTXMultVec(pPL->mat, &a, &a);
    PSMTXMultVec(pPL->mat, &b, &b);
    PosToPos(&c->param.at, &b, &w->cam.param.at, 0.2f);
    PosToPos(&c->param.pos, &a, &w->cam.param.pos, 0.2f);
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        if (dx * dx + dy * dy + dz * dz > 100.0f) {
            PSVECSubtract(&w->cam.param.pos, &w->cam.param.at, &d);
#line 33013 "D:/Bio4/Prog/em10.cpp"
            VECNormalize(&d, &d);
            PSVECScale(&d, &d, 250.0f);
            PSVECAdd(&w->cam.param.pos, &d, &w->cam.param.pos);
            if (EatMgr.hitCheck(&w->cam.param.at, &w->cam.param.pos, &hit, 0, 0x8000, 0)) {
                w->cam.param.pos = hit;
            }
            PSVECSubtract(&w->cam.param.pos, &d, &w->cam.param.pos);
        }
    }
    w->cam.up.x = 0.0f;
    w->cam.up.y = 1.0f;
    w->cam.up.z = 0.0f;
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        w->cam.dist = SQRTF(dx * dx + dy * dy + dz * dz);
    }
    CameraSetOrientationUp(&w->cam);
    CamCtrl.x250 = (s32) &w->cam;
}

extern "C" int em10CatchPLRtnCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    int hit;
    u8 r;
    int rr;

    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if (w->x67C != 0) {
        return 0;
    }
    if (pG->flags_5010 & 0x8000) {
        return 0;
    }
    if (w->pShield != 0) {
        return 0;
    }
    if (em->type == 0xA || em->type == 0xD || em->type == 2 || em->type == 0x18) {
        return 0;
    }
    if ((s16) pG->pl_life <= 0) {
        return 0;
    }
    if (!(em->x3E0 & 1)) {
        return 0;
    }
    if (w->pWep != 0 && !(w->flags & 0x08000000)) {
        if (w->wepType != 9) {
            return 0;
        }
    }
    if (fabsf(em->pos.y - pPL->pos.y) > 300.0f) {
        return 0;
    }
    if (!(em->flags_3C8 & 0x20) && w->x664 == 0 && !(w->flags & 0x08000000) && !Ctrl12Ck(w->pCtrl12, 6) && pG->x4F88 > 3 && em->plDist2 < 12250000.0f && em->plDist2 > 4000000.0f && w->x508 < 0.7853982f && fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f)) < 0.5235988f) {
        if ((Rnd() & 1) == 0) {
            EmRoutineSet(em, 1, 0x39, 0, 0);
            return 1;
        }
        rr = Rnd();
        w->x664 = rr % 300 + 300;
    }
    if (em->plDist2 > 1210000.0f) {
        if (!em10PlRunCk(em)) {
            return 0;
        }
        if (em->plDist2 > 4000000.0f) {
            return 0;
        }
    }
    a = em->pos;
    b = pPL->pos;
    a.y += 1500.0f;
    b.y += 1500.0f;
    hit = SatMgr.hitCheck(&a, &b, 0, 0, 0, 0);
    if (hit) {
        return 0;
    }
    if (pG->x4F88 <= 1 && !EM_RTN(em, 1, 0x1B) && (r = Rnd() % 10, r > 4)) {
        w->x67C = 30;
        EmRoutineSet(em, 1, 0x1B, hit, hit);
        return 1;
    }
    EmRoutineSet(em, 1, 0x33, 0, 0);
    return 1;
}

void em10FootSe(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    cModel* p;
    u32 se;
    int a;
    int b;

    if (em->type == 0xA || em->type == 0xD || w->wepType == 0xB) {
        if (w->x6C0 != 0) {
            w->x6C0--;
            if (w->x6C0 == 0) {
                SndCall(8, 0xA0, &em->getPartsPtr(0)->worldPos, em->id, 0, em);
            }
        }
    }
    if (em->seNo == 0) {
        return;
    }
    se = em->seNo - 1;
    if (CheckInWater(em, 0)) {
        p = em->getPartsPtr(0);
        if (se <= 5 || se == 0x77) {
            em->seNo = 0;
            return;
        }
    }
    if (em->type != 0xA && em->type != 0xD) {
        switch (se) {
        case 0:
            a = 0x10;
            b = 0x15;
            if (w->wepType == 0xB) {
                w->x6C0 = 1;
            }
            break;
        case 1:
            a = 0x11;
            b = 0x19;
            if (w->wepType == 0xB) {
                w->x6C0 = se;
            }
            break;
        case 2:
            a = 0x12;
            b = 0x15;
            if (w->wepType == 0xB) {
                SndCall(8, 0xA4, &em->getPartsPtr(0)->worldPos, em->id, 0, em);
            }
            break;
        case 3:
            a = 0x13;
            b = 0x19;
            if (w->wepType == 0xB) {
                SndCall(8, 0xA4, &em->getPartsPtr(0)->worldPos, em->id, 0, em);
            }
            break;
        default:
            return;
        }
    } else {
        switch (se) {
        case 0:
            w->x6C0 = 1;
            a = 0x18;
            b = 0x15;
            break;
        case 1:
            w->x6C0 = se;
            a = 0x19;
            b = 0x19;
            break;
        case 2:
            a = 0x1A;
            b = 0x15;
            SndCall(8, 0xA4, &em->getPartsPtr(0)->worldPos, em->id, 0, em);
            break;
        case 3:
            a = 0x1B;
            b = 0x19;
            SndCall(8, 0xA4, &em->getPartsPtr(0)->worldPos, em->id, 0, em);
            break;
        case 4:
            em->seNo = 0;
            SndCall(6, 0x6F, &em->getPartsPtr(0)->worldPos, 0, 0, em);
            return;
        case 5:
            em->seNo = 0;
            SndCall(6, 0x70, &em->getPartsPtr(0)->worldPos, 0, 0, em);
            return;
        default:
            return;
        }
    }
    em->seNo = 0;
    p = em->getPartsPtr(b);
    SndCall(5, a, &p->worldPos, 0, 0, em);
    if (ChkWaterEffectEnable(&em->pos)) {
        v = p->worldPos;
        v.y = SatMgr.getFloor(&v, 600.0f, 100000.0f, 0, 0);
        EstSet(0, -1, &v, 0, 0x10, 0x3A, 0, 0, 0, 0);
    }
}

extern "C" int em10DashCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    u32 i;
    u32 cnt;
    u32 lim;
    f32 d;
    f32 ang;

    if (w->wepType == 9 && w->x640 != 0) {
        EmRoutineSet(em, 1, 0x11, 0, 0);
        return 1;
    }
    if (em->type == 0xA || em->type == 0xD) {
        return 0;
    }
    if (w->x58C != 0) {
        return 0;
    }
    if (w->pParasite != 0) {
        return 0;
    }
    if ((em->flags_3C8 & 0x400) && w->x5EC == 0) {
        return 0;
    }
    if (em->flags_3C8 & 0x40) {
        return 0;
    }
    if (pG->x4F88 <= 1) {
        return 0;
    }
    if (em->flags_3C8 & 0x80) {
        w->x6AC = Rnd() % 3;
        EmRoutineSet(em, 1, 0x11, 0, 0);
        return 1;
    }
    if (em->x3D0 == 3) {
        if (EM_RTN(em, 1, 0x1B)) {
            return 1;
        }
        EmRoutineSet(em, 1, 0x1B, 0, 0);
        return 1;
    }
    if (w->flags & 0x20000000) {
        w->x6AC = Rnd() % 3;
        EmRoutineSet(em, 1, 0x11, 0, 0);
        return 1;
    }
    d = 12250000.0f;
    if (pG->x4F88 > 6) {
        d = 4000000.0f;
    }
    if (em->plDist2 < d) {
        return 0;
    }
    if (w->x674 != 0) {
        return 0;
    }
    if (!(w->flags & 1)) {
        return 0;
    }
    ang = fabsf(Muku(&pPL->pos, &em->pos, pPL->rot.y, 3.1415927f));
    if (pG->x4F88 <= 6 && ang > 0.3926991f) {
        return 0;
    }
    cnt = 0;
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* o = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((o->be_flag & 0x201) != 1) {
            continue;
        }
        if (o->id <= 0xF) {
            continue;
        }
        if (o->id > 0x20) {
            continue;
        }
        if (o->hp <= 0) {
            continue;
        }
        if (o == em) {
            continue;
        }
        if (!o->checkStatus(5)) {
            continue;
        }
        if (o->plDist2 < 12250000.0f) {
            cnt++;
        }
    }
    lim = 0;
    if (pG->x4F88 > 3) {
        lim = 1;
    }
    if (w->x6C5 != 0) {
        lim = 2;
    }
    if (pG->x4F88 > 6) {
        lim = 3;
    }
    if (pG->x4F88 > 9) {
        lim = 6;
    }
    if (cnt > lim) {
        return 0;
    }
    w->x6AC = Rnd() % 3;
    EmRoutineSet(em, 1, 0x11, 0, 0);
    return 1;
}

int em10CatchSubCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Mtx inv;
    Vec v;
    Vec a;
    Vec b;
    Mtx m;

    if (pSUB == 0) {
        return 0;
    }
    if (em10DeadCk(pSUB)) {
        return 0;
    }
    if (pSUB->hp <= 0) {
        return 0;
    }
    if (em->hp <= 0) {
        return 0;
    }
    if (!(em->seFlags28B & 2)) {
        return 0;
    }
    if (!(w->flags & 2)) {
        return 0;
    }
    if (pG->flags_5010 & 0x00010000) {
        return 0;
    }
    if (pG->flags_5014 & 0x00800000) {
        return 0;
    }
    if (pG->flags_500C & 0x800) {
        return 0;
    }
    if (pG->flags_5010 & 8) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    PSMTXInverse(em->mat, inv);
    PSMTXMultVec(inv, &pSUB->pos, &v);
    if (v.y < -500.0f || v.y > 500.0f) {
        return 0;
    }
    if (v.z < 0.0f || v.z > 900.0f) {
        return 0;
    }
    if (!(v.x > -400.0f && v.x < 400.0f)) {
        return 0;
    }
    pSUB->dmg.set(0, 2);
    em->dmg.set(0, 2);
    a = em->pos;
    b = pSUB->pos;
    a.y += 500.0f;
    b.y += 500.0f;
    if (SatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    PSMTXRotRad(m, 'y', GetXZAngle(&em->pos, &pPL->pos));
    TransMatrix(m, &em->pos);
    a.x = 300.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    b.x = 300.0f;
    b.y = 500.0f;
    b.z = 500.0f;
    PSMTXMultVec(m, &a, &a);
    PSMTXMultVec(m, &b, &b);
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    a.x = 300.0f;
    a.y = 500.0f;
    a.z = 0.0f;
    b.x = 300.0f;
    b.y = 500.0f;
    b.z = 500.0f;
    PSMTXMultVec(m, &a, &a);
    PSMTXMultVec(m, &b, &b);
    if (EatMgr.hitCheck(&a, &b, 0, 0, 0, 0)) {
        return 0;
    }
    if (pSUB->id == 4) {
        BitOn(pG->flags_5010, 0x00010000);
        BitOn(pG->flags_5014, 0x20000000);
        EM_RTN_SET(em, 1, 0x35);
    } else {
        BitOn(pG->flags_5010, 0x00010000);
        BitOn(pG->flags_5014, 0x20000000);
        EM_RTN_SET(em, 1, 0x3A);
    }
    return 1;
}

extern "C" void em10CamMove(cEm10* em, int no, int shake, f32 rate)
{
    Em10Work* w = EM10_WK(em);
    Vec v;
    Vec hit;
    Vec d;
    Camera* c = &pG->Cam;
    Vec* at;
    Vec* pos;

    switch (no) {
    case 0:
    default:
        v.x = 500.0f;
        v.y = 1600.0f;
        v.z = -2000.0f;
        PSMTXMultVec(pPL->mat, &v, &w->x608);
        break;
    case 1:
        v.x = 1500.0f;
        v.y = 1600.0f;
        v.z = 500.0f;
        PSMTXMultVec(pPL->mat, &v, &w->x608);
        break;
    }
    PSVECAdd(&pPL->getPartsPtr(4)->worldPos, &em->getPartsPtr(4)->worldPos, &v);
    PSVECScale(&v, &v, 0.5f);
    if (shake) {
        at = &w->cam.param.at;
        pos = &w->cam.param.pos;
        PosToPos(&c->param.at, &v, at, rate);
        PosToPos(&c->param.pos, &w->x608, pos, rate);
        v.x = fRand1_1() * 10.0f;
        v.y = fRand1_1() * 10.0f;
        v.z = fRand1_1() * 10.0f;
        PSVECAdd(pos, &v, pos);
        PSVECAdd(at, &v, at);
    } else {
        at = &w->cam.param.at;
        pos = &w->cam.param.pos;
        PosToPos(&c->param.at, &v, at, rate);
        PosToPos(&c->param.pos, &w->x608, pos, rate);
    }
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        if (dx * dx + dy * dy + dz * dz > 100.0f) {
            PSVECSubtract(pos, at, &d);
#line 32709 "D:/Bio4/Prog/em10.cpp"
            VECNormalize(&d, &d);
            PSVECScale(&d, &d, 250.0f);
            PSVECAdd(pos, &d, pos);
            if (EatMgr.hitCheck(at, pos, &hit, 0, 0x8000, 0)) {
                *pos = hit;
            }
            PSVECSubtract(pos, &d, pos);
        }
    }
    w->cam.up.x = 0.0f;
    w->cam.up.y = 1.0f;
    w->cam.up.z = 0.0f;
    {
        f32 dx = w->cam.param.pos.x - w->cam.param.at.x;
        f32 dy = w->cam.param.pos.y - w->cam.param.at.y;
        f32 dz = w->cam.param.pos.z - w->cam.param.at.z;
        w->cam.dist = SQRTF(dx * dx + dy * dy + dz * dz);
    }
    w->cam.param.fovy = 55.0f;
    CameraSetOrientationUp(&w->cam);
    CamCtrl.x250 = (s32) &w->cam;
}

extern "C" int em10CsawAtkCk(cEm10* em)
{
    Em10Work* w = EM10_WK(em);
    Vec a;
    Vec b;
    Vec c;
    int hit;
    u8 r;

    if (w->wepType != 4) {
        return 0;
    }
    if (w->pWep == 0) {
        return 0;
    }
    if (w->x67C != 0) {
        return 0;
    }
    if (w->x58C != 0) {
        return 0;
    }
    if (w->pParasite != 0) {
        return 0;
    }
    if (w->flags & 0x80) {
        return 0;
    }
    if (Ctrl12Ck(w->pCtrl12, 6)) {
        return 0;
    }
    if (!(w->flags & 0x08000000)) {
        if (!(w->flags & 1)) {
            return 0;
        }
        if (w->x508 > 0.7853982f) {
            return 0;
        }
        if (fabsf(em->pos.y - pPL->pos.y) > 500.0f) {
            return 0;
        }
        if (em->plDist2 > 2250000.0f) {
            if (!em10PlRunCk(em)) {
                return 0;
            }
            if (em->plDist2 > 16000000.0f) {
                return 0;
            }
        }
        if (pG->x4F88 <= 3) {
            if (!em10ScreenInCk(em)) {
                return 0;
            }
        }
        a = em->pos;
        b = pPL->pos;
        a.y += 1500.0f;
        b.y += 1500.0f;
        hit = EatMgr.hitCheck(&a, &b, 0, 0, 0, 0);
        if (hit) {
            return 0;
        }
        r = Rnd() % 100;
        if (r > 29 && pSys->region != 0) {
            EmRoutineSet(em, 1, 0x31, hit, hit);
        } else {
            EmRoutineSet(em, 1, 0x2F, hit, hit);
        }
        if (pG->x4F88 <= 3) {
            Ctrl12Set(w->pCtrl12, 6, 0x3C);
            Ctrl12Set(w->pCtrl12, 8, 0x78);
        } else if (pG->stage_no <= 2 && pG->x4F88 <= 9) {
            Ctrl12Set(w->pCtrl12, 6, 0x1E);
            Ctrl12Set(w->pCtrl12, 8, 0x78);
        }
        return 1;
    } else {
        if (!(w->flags & 2)) {
            return 0;
        }
        if (w->x510 > 0.7853982f) {
            return 0;
        }
        if (fabsf(em->pos.y - pSUB->pos.y) > 500.0f) {
            return 0;
        }
        if (w->x514 > 2250000.0f) {
            return 0;
        }
        a = em->pos;
        c = pSUB->pos;
        a.y += 1500.0f;
        c.y += 1500.0f;
        hit = EatMgr.hitCheck(&a, &c, 0, 0, 0, 0);
        if (hit) {
            return 0;
        }
        EmRoutineSet(em, 1, 0x2F, hit, hit);
        return 1;
    }
}
