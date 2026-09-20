#ifndef OBJROBO_H
#define OBJROBO_H

#include "types.h"
#include "vec.h"
#include "obj.h"

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
