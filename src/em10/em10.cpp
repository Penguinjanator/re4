// em10/em10.cpp: the Ganado enemy library (D:/Bio4/Prog/em10.cpp), the same object in the 16 modules
// em10..em17, em19..em1f, em20 (config/G4BE08/modules.py). cEm10 and its routines, the per-weapon
// damage reactions, the route / attack / find checks and the player-side event routines (plem10*).

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
#include "motion.h"
#include "game.h"
#include "rnd.h"
#include "global.h"
#include "math_sub.h"
#include "db_log.h"
#include "joy.h"

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

// Helpers of this unit used before their definition.
int em10CrashCk(cEm10* em);
void em10LostHead(cEm10* em, int a, int b);
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

// Dead flag test (cDmgInfo upper 16 bits): an inline returning 0/1 gives the `li 1; andis.; bne; li 0` chain.
static inline int em10DeadCk(cEm* em)
{
    return (em->flags_324 & 0xFFFF0000) ? 1 : 0;
}

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

Em10Func Em10SetFunc = 0;

static Em10Func Em10_R0_move_tbl[5] = {
    em10_R0_Init,
    em10_R0_Move,
    em10_R0_Damage,
    em10_R0_Die,
    (Em10Func) Em_R0_Scenario,
};

static Em10Func Em10_R1_move_tbl[110][2] = {
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

static void em10_R0_Init(cEm10* em)
{
}

static void em10_R0_Move(cEm10* em)
{
}

static void em10_R0_Damage(cEm10* em)
{
}

static void em10_R0_Die(cEm10* em)
{
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
    if ((w->flags & 0x200000) && w->x4E4) {
        ((cObjLadder*) w->x4E4)->setDown2();
        w->x4E4 = 0;
    }
    if (w->x55C) {
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
            break;
        case 0x10:
        case 0x1A:
            EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            break;
        case 0xB:
        case 0xC:
        case 0x1B:
        case 0x1D:
        case 0x27:
            EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            break;
        case 7:
        case 8:
        case 0x21:
            if (near) {
                EmDmBloodSet2(em, 0x10, 0x65, 0, 0, 0);
            } else {
                EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            }
            break;
        default:
            EmDmBloodSet2(em, 0x10, 0x64, 0, 0, 0);
            break;
        }
    } else {
        if (part->partsNo == 5 && (w->pParasite || w->x58C)) {
            EmDmBloodSet2(em, 0x10, 0x26, 0, 0, 0);
            return;
        }
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
            if (ChkWaterEffectEnable(&em->pos)) {
                EmDmBloodSet2(em, 0x10, 0x3B, 0, 0, 0);
            } else {
                EmDmBloodSet2(em, 0x10, 1, 0, 0, 0);
            }
            break;
        case 0x10:
        case 0x1A:
            EmDmBloodSet2(em, 0x10, 0x29, 0, 0, 0);
            break;
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
            break;
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
            break;
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
            break;
        default:
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
            break;
        }
    }
}

// ===== STUBS (development only, removed as functions are written) =====
static void em10_R1_br_Dummy(cEm10* em)
{
}

static void em10_R1_br_Wait(cEm10* em)
{
}

static void em10_R1_Wait(cEm10* em)
{
}

static void em10_R1_Keeper(cEm10* em)
{
}

static void em10_R1_Hide(cEm10* em)
{
}

static void em10_R1_HideFall(cEm10* em)
{
}

static void em10_R1_HideJump(cEm10* em)
{
}

static void em10_R1_R10CParasite(cEm10* em)
{
}

static void em10_R1_R10CPCancel(cEm10* em)
{
}

static void em10_R1_R204Prayer(cEm10* em)
{
}

static void em10_R1_R222DragonA(cEm10* em)
{
}

static void em10_R1_R222DragonB(cEm10* em)
{
}

static void em10_R1_R222DragonC(cEm10* em)
{
}

static void em10_R1_R227Barrel(cEm10* em)
{
}

static void em10_R1_R21BTrolleyJump(cEm10* em)
{
}

static void em10_R1_R21BTrolleyJump2(cEm10* em)
{
}

static void em10_R1_R303FireDash(cEm10* em)
{
}

static void em10_R1_R10FGJump(cEm10* em)
{
}

static void em10_R1_R10FGondola(cEm10* em)
{
}

static void em10_R1_R209DashSit(cEm10* em)
{
}

static void em10_R1_StickClaw(cEm10* em)
{
}

static void em10_R1_R11DAppear1(cEm10* em)
{
}

static void em10_R1_R11DAppear2(cEm10* em)
{
}

static void em10_R1_R212Drill(cEm10* em)
{
}

static void em10_R1_R209Gatling(cEm10* em)
{
}

static void em10_R1_R201EventWait(cEm10* em)
{
}

static void em10_R1_FindLost(cEm10* em)
{
}

static void em10_R1_R100WalkStay(cEm10* em)
{
}

static void em10_R1_R202Finger(cEm10* em)
{
}

static void em10_R1_StayWalk(cEm10* em)
{
}

static void em10_R1_AttackWait(cEm10* em)
{
}

static void em10_R1_R100TurnWalk(cEm10* em)
{
}

static void em10_R1_R100Cliff(cEm10* em)
{
}

static void em10_R1_R101Bucket(cEm10* em)
{
}

static void em10_R1_R101Suki(cEm10* em)
{
}

static void em10_R1_Work(cEm10* em)
{
}

static void em10_R1_UFOCatch(cEm10* em)
{
}

static void em10_R1_R300TakeAshley(cEm10* em)
{
}

static void em10_R1_R30FBullJump(cEm10* em)
{
}

static void em10_R1_R320Gatling(cEm10* em)
{
}

static void em10_R1_R321DeadBody(cEm10* em)
{
}

static void em10_R1_R300Gatling(cEm10* em)
{
}

static void em10_R1_R101Cart(cEm10* em)
{
}

static void em10_R1_br_EvtDash(cEm10* em)
{
}

static void em10_R1_EvtDash(cEm10* em)
{
}

static void em10_R1_br_EvtWalk(cEm10* em)
{
}

static void em10_R1_EvtWalk(cEm10* em)
{
}

static void em10_R1_Pickup(cEm10* em)
{
}

static void em10_R1_Find(cEm10* em)
{
}

static void em10_R1_C_SawStart(cEm10* em)
{
}

static void em10_R1_BombIgnition(cEm10* em)
{
}

static void em10_R1_br_Walk(cEm10* em)
{
}

static void em10_R1_Walk(cEm10* em)
{
}

static void em10_R1_br_Dash(cEm10* em)
{
}

static void em10_R1_Dash(cEm10* em)
{
}

static void em10_R1_br_Back(cEm10* em)
{
}

static void em10_R1_Back(cEm10* em)
{
}

static void em10_R1_br_Goto(cEm10* em)
{
}

static void em10_R1_Goto(cEm10* em)
{
}

static void em10_R1_GuardWalk(cEm10* em)
{
}

static void em10_R1_Turn180(cEm10* em)
{
}

static void em10_R1_Threat(cEm10* em)
{
}

static void em10_R1_SideStep(cEm10* em)
{
}

static void em10_R1_HideSide(cEm10* em)
{
}

static void em10_R1_AppearSide(cEm10* em)
{
}

static void em10_R1_SitDown(cEm10* em)
{
}

static void em10_R1_Stay(cEm10* em)
{
}

static void em10_R1_RoofWait(cEm10* em)
{
}

static void em10_R1_Guard(cEm10* em)
{
}

static void em10_R1_DownWakeWait(cEm10* em)
{
}

static void em10_R1_DownWake(cEm10* em)
{
}

static void em10_R1_Crash(cEm10* em)
{
}

static void em10_R1_ClimbOver(cEm10* em)
{
}

static void em10_R1_DoorAtk(cEm10* em)
{
}

static void em10_R1_RackAtk(cEm10* em)
{
}

static void em10_R1_WindowAtk(cEm10* em)
{
}

static void em10_R1_LadderClimb(cEm10* em)
{
}

static void em10_R1_VLadderClimb(cEm10* em)
{
}

static void em10_R1_LadderReset(cEm10* em)
{
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
}

static void em10_R1_Drive(cEm10* em)
{
}

static void em10_R1_Catapult(cEm10* em)
{
}

static void em10_R1_RockPush(cEm10* em)
{
}

static void em10_R1_ParasiteAtk(cEm10* em)
{
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
}

static void em10_R1_AxeAtk(cEm10* em)
{
}

static void em10_R1_ShieldAtk(cEm10* em)
{
}

static void em10_R1_TorchFrame(cEm10* em)
{
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
}

static void em10_R1_CSawWalkAtk(cEm10* em)
{
}

static void em10_R1_ClawWalkAtk(cEm10* em)
{
}

static void em10_R1_br_ClawCriAtk(cEm10* em)
{
}

static void em10_R1_ClawCriAtk(cEm10* em)
{
}

static void em10_R1_ClawCriHit(cEm10* em)
{
}

static void plem10_ClawCriHit(cPlayer* pl)
{
}

static void em10_R1_br_C_SawAtk(cEm10* em)
{
}

static void em10_R1_C_SawAtk(cEm10* em)
{
}

static void em10_R1_C_SawHit(cEm10* em)
{
}

static void plem10_C_SawHit(cPlayer* pl)
{
}

static void em10_R1_br_C_SawCriAtk(cEm10* em)
{
}

static void em10_R1_C_SawCriAtk(cEm10* em)
{
}

static void em10_R1_C_SawCriHit(cEm10* em)
{
}

static void plem10_C_SawCriHit(cPlayer* pl)
{
}

static void em10_R1_br_Catch(cEm10* em)
{
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
}

static void em10_R1_Bombhold(cEm10* em)
{
}

static void plem10_Bombhold(cPlayer* pl)
{
}

static void em10_R1_br_DashCatch(cEm10* em)
{
}

static void em10_R1_DashCatch(cEm10* em)
{
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
}

static void em10_R1_Dm_Claw_Big(cEm10* em)
{
}

static void em10_R1_Dm_Gatling(cEm10* em)
{
}

static void em10_R1_Dm_FS(cEm10* em)
{
}

static void em10_R1_Dm_KneeKick(cEm10* em)
{
}

static void em10_R1_Dm_NeckBreak(cEm10* em)
{
}

static void em10_R1_Dm_Showtay(cEm10* em)
{
}

static void em10_R1_Dm_Heel(cEm10* em)
{
}

static void em10_R1_Dm_DashUp(cEm10* em)
{
}

static void em10_R1_Dm_DashDown(cEm10* em)
{
}

static void em10_R1_Dm_Blow(cEm10* em)
{
}

static void em10_R1_Dm_Fence(cEm10* em)
{
}

static void em10_R1_Dm_Ladder(cEm10* em)
{
}

static void em10_R1_Dm_Roof(cEm10* em)
{
}

static void em10_R1_Dm_KneeDown(cEm10* em)
{
}

static void em10_R1_Dm_KnockOut(cEm10* em)
{
}

static void em10_R1_Dm_Down(cEm10* em)
{
}

static void em10_R1_Dm_Frame(cEm10* em)
{
}

static void em10_R1_Dm_TakeAway(cEm10* em)
{
}

static void em10_R1_Die_Cramp(cEm10* em)
{
}

static void em10_R1_Die_Lost(cEm10* em)
{
}

static void em10_R1_Die_Down(cEm10* em)
{
}

static void em10_R1_Die_Normal(cEm10* em)
{
}

static void em10_R1_Die_RunDown(cEm10* em)
{
}

static void em10_R1_Die_Bomb(cEm10* em)
{
}

static void plemDmFrame(cPlayer* pl)
{
}

static void plem10DmGondolaShake(cPlayer* pl)
{
}

static void subem10DmGondolaShake(cSubChar* sub)
{
}

static void plemDmMStar(cPlayer* pl)
{
}

static void plemDmStun(cPlayer* pl)
{
}

static void em10KickAction(cEm10* em)
{
}

static void em10KneeDownAction(cEm10* em)
{
}

static void plem10Kick(cPlayer* pl)
{
}

static void plem10Kick2(cPlayer* pl)
{
}

static void em10FSAction(cEm10* em)
{
}

static void plem10FS(cPlayer* pl)
{
}

static void plem10KneeKick(cPlayer* pl)
{
}

static void plem10NeckBreak(cPlayer* pl)
{
}

static void plem10Showtay(cPlayer* pl)
{
}

static void em10TradeAction(cEm10* em)
{
}
