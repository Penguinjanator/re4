#ifndef OBJROBO_H
#define OBJROBO_H

#include "types.h"
#include "vec.h"
#include "obj.h"

// Room_flg bits of room 2-26 (the statue chase), from the PS2 symbols; RmfFlagChk(pG, n).
enum R226_FLAG {
    RMF_BOBO_SWITCH_EXEC_FRONT = 0,
    RMF_BOBO_SWITCH_EXEC_BACK = 1,
    RMF_PILLAR_ESCAPE_ON = 2,
    RMF_PILLAR_ESCAPE_ING = 3,
    RMF_PILLAR_ESCAPE_LAST = 4,
    RMF_PILLAR_SET_6 = 5,
    RMF_PILLAR_SET_7 = 6,
    RMF_PILLAR_SET_8 = 7,
    RMF_PILLAR_SET_9 = 8,
    RMF_PILLAR_SET_10 = 9,
    RMF_PILLAR_SET_11 = 10,
    RMF_PILLAR_SET_12 = 11,
    RMF_PILLAR_SET_13 = 12,
    RMF_BOBO_DOOR_PUNCH = 15,
    RMF_BOBO_SWITCH_FRONT = 16,
    RMF_BOBO_SWITCH_BACK = 17,
    RMF_BRIDGE_ST_00 = 18,
    RMF_BRIDGE_ST_01 = 19,
    RMF_BRIDGE_ST_02 = 20,
    RMF_BRIDGE_ST_03 = 21,
    RMF_BRIDGE_ST_04 = 22,
    RMF_BRIDGE_ST_05 = 23,
    RMF_BRIDGE_DIE_00 = 24,
    RMF_BRIDGE_DIE_01 = 25,
    RMF_BRIDGE_DIE_02 = 26,
    RMF_BRIDGE_DIE_03 = 27,
    RMF_BRIDGE_DIE_04 = 28,
    RMF_BRIDGE_DIE_05 = 29,
    RMF_BRIDGE_ON_AVOID = 30,
    RMF_BRIDGE_ON_SAFE = 31,
    RMF_PLAYER_DIE_PASSAGE_SET = 32,
    RMF_PLAYER_DIE_BRIDGE_SET = 33,
    RMF_PLAYER_DIE_ING = 34,
    RMF_BGM_ON = 35,
    RMF_ROBO_DOOR_BREAK = 36,
    RMF_FLAG_EMRESET00 = 64,
    RMF_BRIDGE_ON_00 = 65,
    RMF_BRIDGE_ON_01 = 66,
    RMF_BRIDGE_ON_02 = 67,
    RMF_BRIDGE_ON_03 = 68,
    RMF_BRIDGE_ON_04 = 69,
    RMF_BRIDGE_ON_05 = 70,
    RMF_EMSET_00 = 71,
    RMF_EMSET_01 = 72,
};

// Giant statue (Salazar's robot) of room 4-2: waits on the gondola, walks the passage, waits at
// the door, then chases the player over the bridge, breaking its pieces one by one.
class cObjRobo : public cObj {
public:
    virtual void move();

    void SetBeginEvent(u32 a);
    void SetEndEvent(u32 a);
    static void R0Init(cObjRobo* robo);
    static void R0WaitGondola(cObjRobo* robo);
    static void R0WalkPassage(cObjRobo* robo);
    static void R0WaitDoor(cObjRobo* robo);
    static void R0WalkBridge(cObjRobo* robo);
    static void R0WaitBreak(cObjRobo* robo);
    static void R0WaitDie(cObjRobo* robo);
    static void R0Event(cObjRobo* robo);
    void WalkSequence(cObjRobo* robo, int hitCk);
    static void TaskSwitchFront(cObjRobo* robo);
    static void TaskSwitchBack(cObjRobo* robo);
    int WalkHitCk(cObjRobo* robo);
    void SatMove(cObjRobo* robo, Vec* pos, int side);
    int SatMoveSub(cModel* em, Vec* pos, Vec* d);
};

cObjRobo* SetObjRobo(void* bin, void* tpl, Vec* pos, Vec* rot);

#endif
