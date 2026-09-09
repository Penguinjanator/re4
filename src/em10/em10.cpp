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
