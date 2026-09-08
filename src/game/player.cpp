// game/player.cpp: player manager: PlayerInit / life reset, cPlayer construction (init0 / init1 /
// startUp), the per-frame cPlayer::move and the routine 0 / routine 1 (movement) functions.
// INCOMPLETE: pl_R1_Back .. getHeighAdjust (from 0x80058328 on) are not written yet; the unit is
// not linked (not in MATCHING).

#include "atari.h"
#include "light.h"
#include "player.h"
#include "pl_push.h"
#include "global.h"
#include "db_log.h"
#include "main.h"
#include "motion.h"
#include "math_sub.h"
#include "cMotBase.h"
#include "at_mod.h"
#include "em_sub.h"
#include "esp.h"
#include "pl_sub.h"
#include "cam_ctrl.h"
#include "main_mem.h"

extern "C" {
void ReleaseWepData();                        // game/read.cpp
void ShapeMove(void* info);                   // game/shape.cpp
void PlayerInit();
void PlayerLifeReset();
void pl_R0_Move(cPlayer* pl);
void pl_R1_Footwork(cPlayer* pl);
void pl_R1_Walk(cPlayer* pl);
void pl_R1_Back(cPlayer* pl);
void pl_R1_Run(cPlayer* pl);
void pl_R1_Turn(cPlayer* pl);
void pl_R1_Turn180(cPlayer* pl);
void pl_R1_Weapon(cPlayer* pl);
void pl_R1_Boat(cPlayer* pl);
void pl_R1_Ladder(cPlayer* pl);
void pl_R1_Crouch(cPlayer* pl);
void pl_R1_JumpFall(cPlayer* pl);
void pl_R1_Whistle(cPlayer* pl);
void pl_R1_Aux(cPlayer* pl);
void pl_R1_LevelUp(cPlayer* pl);
void pl_R1_LevelDown(cPlayer* pl);
void pl_R1_ObjPush(cPlayer* pl);
void pl_R1_Fance(cPlayer* pl);
void pl_R1_Fall(cPlayer* pl);
void pl_R0_Dijection(cPlayer* pl);
void pl_R1_BoatDrive(cPlayer* pl);
}
void Pl_R0_Damage(cPlayer* pl);   // game/pl_dmg.cpp
void Pl_R0_Die(cPlayer* pl);

#line 41 "D:/Bio4/Prog/player.cpp"
#define PL_MEM_ALLOC(size, line) mem_alloc(size, __FILE__, line, 1, 13)

static inline void U8Set(u8& d, u8 v) { d = v; }
static inline void U16Set(u16& d, u16 v) { d = v; }

#define VALID_PTR(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)

// Parts index mirror table (left <-> right parts of the player model).
u16 pl00_mirror[80] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000A, 0x0011, 0x0016, 0x0017, 0x0018, 0x0019, 0x0012, 0x0013, 0x0014, 0x0015, 0x001B, 0x001A,
    0x001C, 0x001D, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029,
    0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045,
    0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
};

// Routine 0 table (cModel::xFC): move, damage, die, scenario, -, event, dijection.
void (*Pl_func_tbl[7])(cPlayer*) = {
    pl_R0_Move, Pl_R0_Damage, Pl_R0_Die, (void (*)(cPlayer*)) EmScenario, 0, Pl_R0_Event, pl_R0_Dijection,
};

// Routine 1 table (cModel::xFD) of routine 0.
void (*Pl_func_move_tbl[21])(cPlayer*) = {
    pl_R1_Footwork, pl_R1_Walk, pl_R1_Back, pl_R1_Run, pl_R1_Turn, pl_R1_Turn180, pl_R1_Weapon, pl_R1_LevelUp,
    pl_R1_LevelDown, pl_R1_ObjPush, pl_R1_Aux, PlKnifeMove, pl_R1_Fance, 0, pl_R1_Fall, pl_R1_Boat, pl_R1_Ladder,
    pl_R1_Crouch, 0, pl_R1_JumpFall, pl_R1_Whistle,
};

cMot3 mot3;
f32 m3r[3];
void* PlWepMot[3];
Vec PlFancePos;

u8 PlMode = 0;
u8 PlFormMode = 0;
u8 PlDbFlag = 0;
u8 lbl_803140DB = 0;
void (*WeaponMoveFunc)(cPlayer*) = 0;
void (*BoatMoveFunc)(cPlayer*) = 0;

cPlMaho* pMaho;
u8 PlKaiou;
int PlFanceFlag;

// Equipped weapon / life by the player character.
void PlayerInit()
{
    switch (pG->x4FB8) {
    case 0:
    case 1:
        U8Set(pG->wep_no, 2);
        U8Set(pG->wep_type, 0);
        U8Set(pG->wep_lv, 0);
        U8Set(pG->wep_lv_mag, 0);
        U8Set(pG->wep_lv_ex, 0);
        U8Set(pG->x4FBA, 0);
        break;
    case 2:
        U8Set(pG->wep_no, 1);
        U8Set(pG->wep_lv, 0);
        U8Set(pG->wep_lv_mag, 0);
        U8Set(pG->wep_lv_ex, 0);
        U8Set(pG->x4FBA, 0);
        U8Set(pG->wep_type, 0);
        break;
    case 3:
        U8Set(pG->wep_no, 0xB);
        U8Set(pG->wep_lv, 0);
        U8Set(pG->wep_lv_mag, 0);
        U8Set(pG->wep_lv_ex, 0);
        U8Set(pG->x4FBA, 0);
        U8Set(pG->wep_type, 0);
        break;
    case 4:
        U8Set(pG->wep_no, 0x1C);
        U8Set(pG->wep_lv, 0);
        U8Set(pG->wep_lv_mag, 0);
        U8Set(pG->wep_lv_ex, 0);
        U8Set(pG->x4FBA, 0);
        U8Set(pG->wep_type, 0);
        break;
    case 5:
        U8Set(pG->wep_no, 2);
        U8Set(pG->wep_type, 1);
        U8Set(pG->wep_lv, 0);
        U8Set(pG->wep_lv_mag, 0);
        U8Set(pG->wep_lv_ex, 0);
        U8Set(pG->x4FBA, 0);
        break;
    }
    PlayerLifeReset();
    U16Set(pG->pl_life_max, pG->pl_life);
    BitOff(pG->flags_68, 0x00040000);
    ReleaseWepData();
    U16Set(pG->flags_4FBE, 1);
    PlKaiou = 0;
}

// Life by the player character (Leon 1200 / 1860 with flags_54 bit30, Ashley 600, Ada 1560,
// HUNK 1680, Krauser 2400, Wesker 1860); flags_6C bits: 1440 / 1920.
void PlayerLifeReset()
{
    switch (pG->x4FB8) {
    case 0:
        U16Set(pG->pl_life, 1200);
        U16Set(pG->pl_life_max, 1200);
        if (pG->flags_54 & 0x40000000) {
            U16Set(pG->pl_life, 1860);
            U16Set(pG->pl_life_max, 1860);
        }
        break;
    case 1:
        U16Set(pG->pl_life, 600);
        U16Set(pG->pl_life_max, 600);
        break;
    case 2:
        U16Set(pG->pl_life, 1560);
        U16Set(pG->pl_life_max, 1560);
        break;
    case 3:
        U16Set(pG->pl_life, 1680);
        U16Set(pG->pl_life_max, 1680);
        break;
    case 4:
        U16Set(pG->pl_life, 2400);
        U16Set(pG->pl_life_max, 2400);
        break;
    case 5:
        U16Set(pG->pl_life, 1860);
        U16Set(pG->pl_life_max, 1860);
        break;
    default:
        U16Set(pG->pl_life, 1200);
        U16Set(pG->pl_life_max, 1200);
        break;
    }
    U16Set(pG->sub_life, 600);
    U16Set(pG->sub_life_max, 600);
    if (pG->flags_6C & 0x00800000) {
        U16Set(pG->pl_life, 1440);
        U16Set(pG->pl_life_max, 1440);
    }
    if (pG->flags_6C & 0x00040000) {
        U16Set(pG->pl_life, 1920);
        U16Set(pG->pl_life_max, 1920);
    }
}

cPlayer::cPlayer()
{
    flags_420 = 0;
    pPL = this;
    x378 = 0x807EC000;
    hp = pG->pl_life;
    pMotTbl = (void**) PL_MEM_ALLOC(0x1B4, 373);
    memclr_asm(pMotTbl, 0x1B4);
    pRegistMot = (void**) PL_MEM_ALLOC(0x30, 376);
    memclr_asm(pRegistMot, 0x30);
    flags_420 |= 1;
    debugInit();
}

// Sub objects: weapon, body, push, waist, motion base.
void cPlayer::init0()
{
    cPlPush* push;

    pWep = new cPlWep;
    pBody = new cPlBody(this);
    push = new cPlPush;
    push->x8 = 0;
    push->pTarget = 0;
    pPush = push;
    push->pPl = this;
    pWaist = new cPlWaist;
    pMotBase = new cMotBase;
}

// Neck, light set, collision, hit boxes, routine 0, mirror table, first world calc.
void cPlayer::init1()
{
    if (!VALID_PTR(pParts)) {
        pLog->err(0, 0, "cPlayer::cPlayer() FAILED");
        EmMgr.destroy(this);
        return;
    }
    pNeck = new cPlNeck(this);
    be_flag |= 0x07000000;
    x12F = 7;
    {
        static const Vec lightOfs = { 0.0f, 1000.0f, 0.0f };
        static const Vec lightSize = { 1000.0f, 0.0f, 0.0f };
        lightInfo.init2(0, 1, &lightOfs, &lightSize, 1);
    }
    litArea.flags |= 1;
    atari.init(1, 0x1000, 10, 0.0f, 0.0f, 0.0f, -200.0f, 400.0f, 200.0f, 800.0f);
    lockOfs.x = 0.0f;
    lockOfs.y = 0.0f;
    lockOfs.z = 0.0f;
    lockParts = 2;
    YarareInit(this, 0.0f, -30.0f, 0.0f, 100.0f, 210.0f, 2, 1);
    // TODO: the four extra hit boxes live at cEm+0x530/0x564/0x598/0x5CC (the cSubChar fields of em.h
    // overlay them); give them names in em.h.
    YarareAdd(this, (EmHitInfo*) ((u8*) this + 0x530), 0.0f, 0.0f, 0.0f, 130.0f, 120.0f, 3, 1);
    YarareAdd(this, (EmHitInfo*) ((u8*) this + 0x564), 0.0f, 0.0f, 0.0f, 80.0f, -20.0f, 5, 1);
    YarareAdd(this, (EmHitInfo*) ((u8*) this + 0x598), -300.0f, 170.0f, 0.0f, 300.0f, 10.0f, 0x13, 1);
    YarareAdd(this, (EmHitInfo*) ((u8*) this + 0x5CC), 300.0f, 170.0f, 0.0f, 300.0f, 10.0f, 0x17, 1);
    MOTION(this)->flip = pl00_mirror;
    x4FE = 0;
    alpha = 1.0f;
    flags_41C = 0;
    Pl_func_tbl[0] = pl_R0_Move;
    satCheckFlag = 0;
    xButtonWait = 0;
    pRoomEff = 0;
    flags_420 |= 0x800;
    sndId504 = 0;
    eyeMode = 0;
    boss0 = 0;
    partsWorldCalc();
    initCloth();
    if (pG->x4FB8 == 0) {
        pBody->makeSpaeData();
    }
    p2A4 = (EmWork2A4*) PL_MEM_ALLOC(0x1FE, 424);
}

// Place the player at the room start position and run the first frames of its motion.
void cPlayer::startUp()
{
    be_flag |= 0x00200000;
    rot.y = pG->sub_angle;
    setPos(&pG->sub_pos);
    matUpdate();
    xFF = 1;
    x4FD = 0;
    x4FC = 0;
    xFE = 0;
    xFC = 0;
    xFD = 0;
    move();
    motionMove();
    motionMove();
    SatMgr.check(this, satCheckFlag);
    matUpdate();
}

// Per-frame update.
void cPlayer::move()
{
    Vec savePos;
    f32 water;
    int moved;
    int i;

    BitOff(pG->flags_5010, 2);
    BitOff(pG->flags_500C, 0x00800000);
    BitOff(pG->flags_5010, 0x40000000);
    BitOff(pG->flags_5010, 0x00800000);
    BitOff(pG->flags_5010, 0x00400000);
    BitOff(pG->flags_5010, 0x00200000);
    BitOff(pG->flags_5010, 0x00080000);
    BitOff(pG->flags_5010, 0x00008000);
    BitOff(pG->flags_5010, 0x00002000);
    BitOff(pG->flags_5014, 0x40000000);
    BitOff(pG->flags_5014, 0x01000000);
    BitOff(pG->flags_5010, 0x00040000);
    clearStatus(3);
    BitOff(pG->flags_5014, 0x80000000);
    if (pWep->pObj) {
        pWep->pObj->wep.target = 0;
    }
    pMotBase->adjust();
    dmg.move();
    keyConfig();
    dmgCheck();
    if (pBody->pShape) {
        ShapeMove(pBody->pShape);
    }
    if ((int) pos.x == (int) oldPos.x && (int) pos.y == (int) oldPos.y && (int) pos.z == (int) oldPos.z
        && (stat & 0xFFFFFF00) == 0x100 && !(pG->flags_500C & 0x20) && (int) pG->flags_60 >= 0) {
        moved = 0;
    } else {
        moved = 1;
    }
    if (Key.trg & 0x10) {
        flags_420 &= ~0x1000;
    }
    if (Key.trg & 0x800) {
        if (flags_420 & 0x1000) {
            flags_420 &= ~0x1000;
        } else {
            flags_420 |= 0x1000;
        }
    }
    moveBinocular();
    subCharLiveCheck();
    Pl_func_tbl[xFC](this);
    if (pG->flags_68 & 0x00010000) {
        if (PlKaiou != 0xFF) {
            for (i = 0; i < PlKaiou + 1; i++) {
                Pl_func_tbl[xFC](this);
            }
        }
    }
    if ((stat & 0xFFFFFF00) == 0x00060000) {
        pParts->rot.y *= 0.5f;
    }
    rot.y = LIMIT_ANGLE(rot.y);
    flags_41C = (u16) flags_41C;
    shadowCtrl();
    pMotBase->move();
    pNeck->move();
    moveEye();
    pBody->waistSet(pWaist->cur);
    pBody->move();
    moveMatCalcBefore();
    partsWorldCalc();
    partsFixAdjust();
    PartsWorldPosCalc(this);
    if (!(pG->flags_68 & 8)) {
        savePos = pos;
        EmAtCheck(this);
        SatMgr.check(this, satCheckFlag);
        if ((int) savePos.x != (int) pos.x || (int) savePos.y != (int) pos.y || (int) savePos.z != (int) pos.z) {
            moved = 1;
        }
    }
    atari.move();
    PartsWorldPosCalc(this);
    pWep->move();
    moveCloth();
    seqSeCtrl();
    updateOldPos();
    visibleCtrl();
    if (moved) {
        CamCtrl.qfps.setPlayerLocation(mat, pFloorNrm);
    }
    if (GetWaterHeight(&pos, &water) && water > pos.y) {
        PlWaterProc(this);
    }
    be_flag &= ~0x30000000;
    debugMove();
}

// Routine 0/0: the movement sub routines (Pl_func_move_tbl by xFD).
void pl_R0_Move(cPlayer* pl)
{
    if (pl->dmgFlag520 == 0) {
        pl->dmgCnt522 = 0;
    }
    pl->dmgFlag520 = 0;
    Pl_func_move_tbl[pl->xFD](pl);
}

// Idle: footwork motion (pMotTbl[0] / the damaged pair 0x5F), neck motions 0x3F/0x40.
void pl_R1_Footwork(cPlayer* pl)
{
    switch (pl->xFE) {
    case 0: {
        int hokan;
        int frame;
        if (pl->xFF & 1) {
            hokan = pl->x4FD;
            frame = pl->x4FC;
        } else {
            hokan = 8;
            frame = 0;
        }
        pl->motionSet(pl->pMotTbl[0], pl->pMotTbl[1], pl->pMotTbl[0x5F], pl->pMotTbl[0x60], hokan, frame);
        if (dmMotCk()) {
            pl->pNeck->init(pl->pMotTbl[0x3F], pl->pMotTbl[0x40], 0);
        } else {
            pl->pNeck->init(PL_ARC_PTR(pG->pPlArc, 0x40), PL_ARC_PTR(pG->pPlArc, 0x41), 0);
        }
        pl->xFE = 1;
    }
    case 1:
        pl->motionMove();
        break;
    case 2:
        if (pl->motionMove()) {
            pl->xFE = 0;
        }
        break;
    }
    if (pl->actionSelect() == 0) {
        if ((Key.on & 1) && (Key.on & 0x40000000) && joyKamae() == 0) {
            pl->xFF = 0;
            pl->xFD = 3;
            pl->xFC = 0;
            pl->xFE = 0;
        } else {
            pl->checkCtrl();
        }
    }
}

// Walk (pMotTbl[2] / damaged 0x61), turning with SPEED_WALK_TURN; run (routine 3) on the run key.
void pl_R1_Walk(cPlayer* pl)
{
    if (pl->xFE == 0) {
        void** tbl;
        int frame;
        int hokan;
        if (pl->xFF & 4) {
            MotionData* data;
            if (dmMotCk()) {
                tbl = pl->pMotTbl;
                data = (MotionData*) tbl[6];
            } else {
                tbl = pl->pMotTbl;
                data = (MotionData*) PL_ARC_PTR(pG->pPlArc, 0x38);
            }
            frame = (u16) ((f32) (data->maxFrame & 0x3FFF) * (f32) pl->x4FC * 0.00390625f);
            hokan = pl->x4FD;
        } else {
            tbl = pl->pMotTbl;
            frame = 0;
            hokan = 8;
        }
        pl->motionSet(tbl[2], tbl[3], tbl[0x61], tbl[0x62], hokan, frame);
        if (dmMotCk()) {
            pl->pNeck->init(pl->pMotTbl[0x39], pl->pMotTbl[0x3A], frame);
        } else {
            pl->pNeck->init(PL_ARC_PTR(pG->pPlArc, 0x42), PL_ARC_PTR(pG->pPlArc, 0x43), frame);
        }
        pl->xFE = 1;
    }
    pl->motionMove();
    if (pl->actionSelect() == 0) {
        if ((Key.on & 0x40000000) && joyKamae() == 0) {
            pl->x4FD = 5;
            pl->xFC = 0;
            pl->xFD = 3;
            pl->xFE = 2;
            pl->xFF = 4;
            pl->x4FC = (u8) (pl->frame * 255.0f / (f32) pl->frameMax);
        } else if (!(Key.on & 1)) {
            pl->xFF = 0;
            pl->xFC = 0;
            pl->xFD = 0;
            pl->xFE = 0;
        } else {
            if (Key.on & 4) {
                pl->rot.y -= cPlayer::SPEED_WALK_TURN;
            }
            if (Key.on & 8) {
                pl->rot.y += cPlayer::SPEED_WALK_TURN;
            }
            pl->rot.y = LIMIT_ANGLE(pl->rot.y);
            pl->checkCtrl();
        }
    }
}
