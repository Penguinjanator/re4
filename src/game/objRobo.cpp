#include "atari.h"
#include "light.h"
#include "dmg.h"
#include "flag_rsf.h"
#include "obj.h"
#include "em.h"
#include "emhit.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"
#include "est.h"
#include "snd.h"
#include "scroll.h"
#include "sce_sys.h"
#include "sce_at.h"
#include "player.h"
#include "libc/string.h"

// Giant statue (Salazar's robot) of room 4-2: waits on the gondola, walks the passage, waits at
// the door, then chases the player over the bridge, breaking its pieces one by one.
class cObjRobo : public cObj {
public:
    virtual void move();

    void SetBeginEvent();
    void SetEndEvent();
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

extern "C" {
int MotionMove(cModel* m, int a);
cObj* SetObjRobo(void* bin, void* tpl, Vec* pos, Vec* rot);
}
void MotionSetCore(cModel* m, void* work, void* mot, int a, int b, int c, int d);

f32 RoboFallSpdX = -50.0f;
f32 RoboFallSpdY = -100.0f;
f32 RoboFallSpdZ = -10.0f;
f32 BridgeStartX = -63000.0f;
static f32 RoboHitRadius = 6000.0f;
static f32 posysub = 500.0f;

// Hit box table: model parts and the YarareInit cylinder (x, y, z offset, radius, height).
struct RoboHitTbl {
    int parts;
    f32 x;
    f32 y;
    f32 z;
    f32 w;
    f32 h;
};

cObj* SetObjRobo(void* bin, void* tpl, Vec* pos, Vec* rot)
{
    cObj* obj;
    RoboWork* w;
    static const Vec p0 = { 0.0f, 0.0f, 0.0f };
    static const Vec p1 = { 5000.0f, 10000.0f, 5000.0f };

    obj = ObjMgr.create(0x37);
    if (obj == 0) {
        return 0;
    }
    w = &obj->robo;
    memset(w, 0, sizeof(RoboWork));
    if (obj->modelInit(bin, tpl) == 0) {
        pLog->err(0, 0, "SetLadder() failed.");
        ObjMgr.destroy(obj);
        return 0;
    }
#line 94 "D:/Bio4/Prog/objRobo.cpp"
    obj->p2A4 = MEM_ALLOC(0x98, 1, 0xD);
    obj->lightInfo.init2(0, 1, &p0, &p1, 0x10);
    obj->sub2B4.clrFlags(0xFCFF);
    if (pos) {
        obj->pos = *pos;
    } else {
        obj->pos.x = 0.0f;
        obj->pos.y = 0.0f;
        obj->pos.z = 0.0f;
    }
    obj->oldPos = obj->pos;
    if (rot) {
        obj->rot = *rot;
    } else {
        obj->rot.x = 0.0f;
        obj->rot.y = 0.0f;
        obj->rot.z = 0.0f;
    }
    RotMatrix(obj->worldMat, &obj->rot);
    TransMatrix(obj->worldMat, &obj->pos);
    ScaleMatrix(obj->worldMat, &obj->scale);
    PSMTXCopy(obj->worldMat, obj->mat);
    obj->partsMatCalc();
    obj->partsWorldCalc();
    w->routine = 0;
    w->step = 0;
    return obj;
}

void cObjRobo::move()
{
    static void (*R0Tbl[])(cObjRobo*) = {
        R0Init,       R0WaitGondola, R0WalkPassage, R0WaitDoor,
        R0WalkBridge, R0WaitBreak,   R0WaitDie,     R0Event,
    };

    R0Tbl[robo.routine](this);
}

void cObjRobo::SetBeginEvent()
{
    RoboWork* w = &robo;

    setNoSuspend(1);
    w->step = 0;
    w->routine = 7;
}

void cObjRobo::SetEndEvent()
{
    setNoSuspend(0);
}

void cObjRobo::R0Init(cObjRobo* robo)
{
    RoboWork* w = &robo->robo;
    Vec pos = { 0.0f, 0.0f, 0.0f };
    Vec rot = { 0.0f, 0.0f, 0.0f };
    Vec pos2;
    Vec rot2;
    RoboHitTbl tbl[14] = {
        { 0x00, 0.0f, 0.0f, 0.0f, 2900.0f, 3500.0f },
        { 0x01, 0.0f, 0.0f, 0.0f, 1300.0f, 0.0f },
        { 0x02, 0.0f, 1000.0f, 0.0f, 1200.0f, -2500.0f },
        { 0x03, 0.0f, 0.0f, 0.0f, 1200.0f, -3000.0f },
        { 0x06, 0.0f, 0.0f, 0.0f, 1300.0f, 0.0f },
        { 0x07, 0.0f, 1000.0f, 0.0f, 1200.0f, -2500.0f },
        { 0x08, 0.0f, 0.0f, 0.0f, 1200.0f, -3000.0f },
        { 0x0B, 0.0f, 0.0f, 0.0f, 1800.0f, 0.0f },
        { 0x0C, 0.0f, 0.0f, -500.0f, 1300.0f, 3500.0f },
        { 0x0E, 0.0f, 0.0f, 0.0f, 1800.0f, 0.0f },
        { 0x0F, 0.0f, 0.0f, -500.0f, 1300.0f, 3500.0f },
        { 0x13, 0.0f, 2000.0f, -1500.0f, 2000.0f, 0.0f },
        { 0x15, 0.0f, 100.0f, -300.0f, 1000.0f, 0.0f },
        { 0x16, 0.0f, 0.0f, 200.0f, 1000.0f, 0.0f },
    };
    int i;
    cObj* smd;
    cEmHit* hit;

    MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x42), 0, 0, 4, 0);
    for (i = 0; i < 2; i++) {
        w->sat[i] = 0;
        w->eat[i] = 0;
    }
    w->sat[0] = SatMgr.create(ROOM_ARC_PTR(pG->pRoomArc, 5), 0, &pos, &rot, 2);
    w->sat[1] = SatMgr.create(ROOM_ARC_PTR(pG->pRoomArc, 5), 0, &pos, &rot, 3);
    w->eat[0] = EatMgr.create(ROOM_ARC_PTR(pG->pRoomArc, 0x12), 0, &pos, &rot, 6);
    w->eat[1] = EatMgr.create(ROOM_ARC_PTR(pG->pRoomArc, 0x12), 0, &pos, &rot, 7);
    w->eat2 = EatMgr.create(ROOM_ARC_PTR(pG->pRoomArc, 0x12), 0, &robo->pos, &rot, 2);
    for (i = 0; i < 2; i++) {
        Vec pos2 = { 0.0f, 0.0f, 0.0f };
        Vec rot2 = { 0.0f, 0.0f, 0.0f };

        smd = SetObjSmd((void*) (pG->pArc->ofs_20 + (u32) pG->pArc), (void*) (pG->pArc->ofs_24 + (u32) pG->pArc),
                        &pos2, &rot2, 0x10, 1);
        w->smd[i] = smd;
        if (smd) {
            if (i == 0) {
                SceAtSetParent(0x26, smd, 0);
            } else {
                SceAtSetParent(0x27, smd, 0);
            }
            smd->be_flag &= ~2;
        }
    }
    SceAtDataSet_exec(3, 0x12, 0, (TaskFunc) TaskSwitchBack, robo, 1);
    SceAtDataSet_exec(4, 0x12, 0, (TaskFunc) TaskSwitchFront, robo, 1);
    if (RsfCheck(pG->room_id, 9)) {
        w->step = 0;
        w->routine = 1;
    } else {
        w->step = 0;
        w->routine = 7;
    }
    for (i = 0; i < 14; i++) {
        w->hit[i] = 0;
        hit = SetEmHit((void*) (pG->pArc->ofs_20 + (u32) pG->pArc), (void*) (pG->pArc->ofs_24 + (u32) pG->pArc), 0, 0, 1);
        if (hit) {
            hit->setParent(robo, tbl[i].parts, 0);
            if (tbl[i].parts == 0x15 || tbl[i].parts == 0x16) {
                YarareInit(hit, tbl[i].x, tbl[i].y, tbl[i].z, tbl[i].w, tbl[i].h, 0, 1);
            } else {
                YarareInit(hit, tbl[i].x, tbl[i].y, tbl[i].z, tbl[i].w, tbl[i].h, 0, 0x41);
            }
            w->hit[i] = hit;
        }
    }
}

void cObjRobo::R0WaitGondola(cObjRobo* robo)
{
    RoboWork* w = &robo->robo;
    Vec ft[2];
    Vec p;
    Vec d;
    int i;
    int parts;
    cEmHit* hit;

    switch (w->step) {
    case 0:
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x62), ROOM_ARC_PTR(pG->pRoomArc, 0x67), 0, 4, 0);
        w->step++;
    case 1:
        if (robo->motEvent & 1) {
            SndCall(6, 2, &robo->getPartsPtr(10)->worldPos, 0, 0, 0);
        }
        if (robo->motEvent & 2) {
            SndCall(6, 1, &robo->getPartsPtr(5)->worldPos, 0, 0, 0);
        }
        if (robo->motEvent & 4) {
            SndCall(6, 4, &robo->getPartsPtr(10)->worldPos, 0, 0, 0);
        }
        if (robo->motEvent & 8) {
            SndCall(6, 3, &robo->getPartsPtr(5)->worldPos, 0, 0, 0);
        }
        for (i = 0; i < 2; i++) {
            parts = 5;
            if (i == 0) {
                parts = 10;
            }
            ft[i].x = 0.0f;
            ft[i].y = 100.0f;
            ft[i].z = 0.0f;
            PSMTXMultVec(robo->getPartsPtr(parts)->mat, &ft[i], &ft[i]);
        }
        MotionMove(robo, 0);
        robo->partsWorldCalc();
        if (pPL->flags_420 & 0x80) {
            pPL->setPos(&pPL->evTarget2);
        }
        for (i = 0; i < 2; i++) {
            robo->SatMove(robo, &ft[i], i);
        }
        if (w->hit[13] && w->hit[13]->ckStatus() == 1 && !(pG->flags_174 & 0x80000000)) {
            SceExec(0x12, (TaskFunc) TaskSwitchFront, (int) robo, 0, 2, 0);
        }
        if (w->hit[12] && w->hit[12]->ckStatus() == 1 && !(pG->flags_174 & 0x40000000)) {
            SceExec(0x12, (TaskFunc) TaskSwitchBack, (int) robo, 0, 2, 0);
        }
        for (i = 0; i < 14; i++) {
            hit = w->hit[i];
            if (hit && hit->ckStatus() == 1) {
                EmDmBloodSet2(hit, 1, 0xF, 0, 0, 2);
                EmGetDmPos(hit, &p, &d);
            }
        }
        break;
    }
}

void cObjRobo::R0WalkPassage(cObjRobo* robo)
{
    RoboWork* w = &robo->robo;
    Vec v;

    switch (w->step) {
    case 0:
        EstSet((int) robo, -1, 0, 0, 1, 7, 1, 3, 0, 0);
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x25), ROOM_ARC_PTR(pG->pRoomArc, 0x26), 0x3C, 5, 0);
        w->step++;
    case 1:
        robo->WalkSequence(robo, 1);
        break;
    }
    if (robo->pos.x <= -60000.0f) {
        v.x = -60000.0f;
        v.y = robo->pos.y;
        v.z = robo->pos.z;
        robo->setPos(&v);
    }
    MotionMove(robo, 0);
    robo->partsWorldCalc();
}

void cObjRobo::R0WaitDoor(cObjRobo* robo)
{
    RoboWork* w = &robo->robo;
    Vec v;

    switch (w->step) {
    case 0:
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x25), ROOM_ARC_PTR(pG->pRoomArc, 0x26), 0x3C, 5, 0);
        w->step++;
    case 1:
        robo->WalkSequence(robo, 0);
        if (robo->pos.x <= -55597.8984375f) {
            EffectEspDelete(1, 3, 0, 0);
            EffectEspgenDelete(1, 3, 0);
            EffectEfmDelete(1, 3, 0);
            BitOn(pG->flags_174, 0x10000);
            MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x5C), ROOM_ARC_PTR(pG->pRoomArc, 0x65), 0x3C, 4, 0);
            v.x = -55597.8984375f;
            v.y = robo->pos.y;
            v.z = robo->pos.z;
            robo->setPos(&v);
            w->timer = 0;
            w->step++;
        }
        break;
    case 2:
        if (robo->motEvent & 1) {
            EstSet((int) robo, -1, 0, 0, 1, 0xC, 1, 4, 0, 0);
            w->timer = 0;
        }
        w->timer++;
        if (w->timer == 10) {
            SndCall(6, 0x10, &robo->pos, 0, 0, 0);
        }
        if (w->timer == 30) {
            SndCall(6, 7, &robo->pos, 0, 0, 0);
        }
        break;
    }
    MotionMove(robo, 0);
    robo->partsWorldCalc();
}

void cObjRobo::R0WalkBridge(cObjRobo* robo)
{
    RoboWork* w = &robo->robo;
    u32 smdNo[6] = { 0x43, 0x44, 0x4F, 0x50, 0x51, 0x52 };
    u32 flagNo[6] = { 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 };
    u32 flagNo2[6] = { 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D };
    u32 estNo[6] = { 3, 4, 5, 6, 7, 8 };
    u32 estNo2[6] = { 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C };
    Vec v;
    cObj* smd;
    int i;

    switch (w->step) {
    case 0:
        v.x = BridgeStartX;
        v.y = robo->pos.y;
        v.z = -16560.0f;
        robo->setPos(&v);
        EstSet((int) robo, -1, 0, 0, 1, 7, 1, 3, 0, 0);
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x25), ROOM_ARC_PTR(pG->pRoomArc, 0x26), 0, 5, 0);
        for (i = 0; i < 6; i++) {
            w->hitCnt[i] = 0;
        }
        w->step++;
    case 1:
        robo->WalkSequence(robo, 1);
        smd = SmdGetObjPtr(smdNo[0]);
        if (smd == 0) {
            break;
        }
        if (robo->pos.x < smd->pos.x) {
            w->cnt = 0;
            w->step++;
        }
        break;
    case 2:
        w->cnt++;
        if (w->cnt <= 9) {
            robo->WalkSequence(robo, 1);
        }
        if (w->cnt == 10) {
            SndCall(6, 0xA, &robo->pos, 0, 0, 0);
            EffectEspDelete(1, 3, 0, 0);
            EffectEspgenDelete(1, 3, 0);
            EffectEfmDelete(1, 3, 0);
            MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x63), 0, 0xA, 1, 0);
            EstSet((int) robo, -1, 0, 0, 1, 0x20, 1, 0, 0, 0);
            w->fallX = robo->pos.x;
            w->fallSpdY = RoboFallSpdY;
        }
        if (w->cnt == 90) {
            SndCall(6, 9, &robo->pos, 0, 0, 0);
        }
        for (i = 0; i < 6; i++) {
            smd = SmdGetObjPtr(smdNo[i]);
            if (smd) {
                w->fallX += RoboFallSpdX;
                if (w->fallX < smd->pos.x) {
                    u32* flags = &pG->flags_174;

                    if (!(flags[flagNo[i] >> 5] & (0x80000000 >> (flagNo[i] & 31)))) {
                        flags[flagNo[i] >> 5] |= 0x80000000 >> (flagNo[i] & 31);
                        EffectEspDelete(0x2001, (u8) estNo[i], 0, 0);
                        EffectEspgenDelete(0x2001, (u8) estNo[i], 0);
                        EffectEfmDelete(0x2001, (u8) estNo[i], 0);
                        EstSet(0, -1, 0, 0, 1, (u8) estNo2[i], 1, 0, 0, 0);
                    }
                }
            }
            {
                u32* flags = &pG->flags_174;

                if (flags[flagNo[i] >> 5] & (0x80000000 >> (flagNo[i] & 31))) {
                    w->hitCnt[i]++;
                    if (w->hitCnt[i] > 14) {
                        u32* flags2 = &pG->flags_174;

                        flags2[flagNo2[i] >> 5] |= 0x80000000 >> (flagNo2[i] & 31);
                    }
                }
            }
        }
        break;
    }
    MotionMove(robo, 0);
    robo->partsWorldCalc();
}

void cObjRobo::R0WaitBreak(cObjRobo* robo)
{
    RoboWork* w = &robo->robo;

    if (w->step == 0) {
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x3C), 0, 0, 4, 0);
        w->step++;
    }
    MotionMove(robo, 0);
    robo->partsWorldCalc();
}

void cObjRobo::R0WaitDie(cObjRobo* robo)
{
    RoboWork* w = &robo->robo;

    if (w->step == 0) {
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x3C), 0, 0, 4, 0);
        w->step++;
    }
    MotionMove(robo, 0);
    robo->partsWorldCalc();
}

void cObjRobo::R0Event(cObjRobo* robo)
{
    MotionMove(robo, 0);
    robo->partsWorldCalc();
}

// One walk frame: keep the statue on its line, play the step SEs / effects of the motion events
// and (hitCk) check whether a foot caught the player.
void cObjRobo::WalkSequence(cObjRobo* robo, int hitCk)
{
    Vec v;
    cModel* parts;

    v.x = robo->pos.x;
    v.y = robo->pos.y;
    v.z = -16560.0f;
    robo->setPos(&v);
    v.x = 0.0f;
    v.y = -PI / 2;
    v.z = 0.0f;
    robo->setAng(&v);
    if (robo->motEvent & 4) {
        parts = robo->getPartsPtr(0xD);
        if (parts) {
            SndCall(6, 7, &parts->worldPos, 0, 0, 0);
        }
    }
    if (robo->motEvent & 8) {
        parts = robo->getPartsPtr(0x10);
        if (parts) {
            SndCall(6, 7, &parts->worldPos, 0, 0, 0);
        }
    }
    if (robo->motEvent & 1) {
        if (hitCk == 1) {
            robo->WalkHitCk(robo);
        }
        EstSet((int) robo, -1, 0, 0, 1, 8, 1, 2, 0, 0);
        parts = robo->getPartsPtr(0xD);
        if (parts) {
            SndCall(6, 8, &parts->worldPos, 0, 0, 0);
        }
    }
    if (robo->motEvent & 2) {
        if (hitCk == 1) {
            robo->WalkHitCk(robo);
        }
        EstSet((int) robo, -1, 0, 0, 1, 9, 1, 2, 0, 0);
        parts = robo->getPartsPtr(0x10);
        if (parts) {
            SndCall(6, 8, &parts->worldPos, 0, 0, 0);
        }
    }
}

// Scenario task: the front arm swings down (or back up) over 15 frames.
void cObjRobo::TaskSwitchFront(cObjRobo* robo)
{
    cModel* parts;
    int i;
    f32 from;
    f32 range;
    f32 max;

    parts = robo->getPartsPtr(0x16);
    if (pG->flags_174 & 0x80000000) {
        return;
    }
    BitOn(pG->flags_174, 0x80000000);
    SceAtSetEnable(4, 0);
    if (parts) {
        SndCall(6, 5, &parts->pos, 0, 0, 0);
    }
    if (!(pG->flags_174 & 0x8000)) {
        BitOn(pG->flags_174, 0x8000);
        max = (f32) 15;
        from = 0.0f;
        range = -1.483529806137085f;
        for (i = 0; i < 15; i++) {
            parts->rot.y = range * (f32) i / max + from;
            SceSleep(1);
        }
        parts->rot.y = -1.483529806137085f;
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x61), ROOM_ARC_PTR(pG->pRoomArc, 0x66), 0xF0, 4, 0);
    } else {
        BitOff(pG->flags_174, 0x8000);
        max = (f32) 15;
        from = -1.483529806137085f;
        range = 0.0f - -1.483529806137085f;
        for (i = 0; i < 15; i++) {
            parts->rot.y = range * (f32) i / max + from;
            SceSleep(1);
        }
        parts->rot.y = 0.0f;
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x62), ROOM_ARC_PTR(pG->pRoomArc, 0x67), 0xF0, 4, 0);
    }
    BitOff(pG->flags_174, 0x4000);
    robo->getPartsPtr(0x15)->rot.x = 0.0f;
    SceSleep(1);
    BitOff(pG->flags_174, 0x80000000);
    SceAtSetEnable(4, 1);
}

// Scenario task: the back arm.
void cObjRobo::TaskSwitchBack(cObjRobo* robo)
{
    cModel* parts;
    int i;
    f32 from;
    f32 range;
    f32 max;

    parts = robo->getPartsPtr(0x15);
    if (pG->flags_174 & 0x40000000) {
        return;
    }
    BitOn(pG->flags_174, 0x40000000);
    SceAtSetEnable(3, 0);
    if (parts) {
        SndCall(6, 5, &parts->pos, 0, 0, 0);
    }
    if (!(pG->flags_174 & 0x4000)) {
        BitOn(pG->flags_174, 0x4000);
        max = (f32) 15;
        from = 0.0f;
        range = -1.483529806137085f;
        for (i = 0; i < 15; i++) {
            parts->rot.x = range * (f32) i / max + from;
            SceSleep(1);
        }
        parts->rot.x = -1.483529806137085f;
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x22), ROOM_ARC_PTR(pG->pRoomArc, 0x45), 0xF0, 4, 0);
    } else {
        BitOff(pG->flags_174, 0x4000);
        max = (f32) 15;
        from = -1.483529806137085f;
        range = 0.0f - -1.483529806137085f;
        for (i = 0; i < 15; i++) {
            parts->rot.x = range * (f32) i / max + from;
            SceSleep(1);
        }
        parts->rot.x = 0.0f;
        MotionSetCore(robo, &robo->pMotion, ROOM_ARC_PTR(pG->pRoomArc, 0x62), ROOM_ARC_PTR(pG->pRoomArc, 0x67), 0xF0, 4, 0);
    }
    BitOff(pG->flags_174, 0x8000);
    robo->getPartsPtr(0x16)->rot.y = 0.0f;
    SceSleep(1);
    BitOff(pG->flags_174, 0x40000000);
    SceAtSetEnable(3, 1);
}

// The player is under a foot: flag the death.
int cObjRobo::WalkHitCk(cObjRobo* robo)
{
    int dead;

    if ((s16) pG->pl_life > 0) {
        dead = 1;
        if ((pPL->flags_324 & 0xFFFF0000) == 0) {
            dead = 0;
        }
        if (dead == 0) {
            if (sqrtf((robo->pos.x - pPL->pos.x) * (robo->pos.x - pPL->pos.x) +
                      (robo->pos.z - pPL->pos.z) * (robo->pos.z - pPL->pos.z)) <= RoboHitRadius) {
                BitOn(pG->flags_178, 0x80000000);
                return 1;
            }
        }
    }
    return 0;
}

// Move the collision pieces of one side to the foot at `pos` and push the player / enemies
// standing on it along.
void cObjRobo::SatMove(cObjRobo* robo, Vec* pos, int side)
{
    RoboWork* w = &robo->robo;
    Vec a = { 0.0f, 0.0f, 0.0f };
    Vec b = { 0.0f, 0.0f, 0.0f };
    Vec c;
    Vec d;
    cModel* parts;
    int partsNo;
    u32 i;
    cEm* em;

    partsNo = 5;
    if (side == 0) {
        partsNo = 10;
    }
    parts = robo->getPartsPtr(partsNo);
    a.x = pos->x - 1000.0f;
    a.y = pos->y;
    a.z = pos->z;
    c.x = 1000.0f;
    c.y = 2000.0f;
    c.z = 1000.0f;
    PSMTXMultVec(parts->mat, &c, &c);
    PSVECSubtract(&c, pos, &d);
    if (!(pPL->flags_420 & 0x100)) {
        if (SatMoveSub(pPL, &a, &d) == 1) {
            pG->quake_ofs = d;
        }
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        em = (cEm*) ((u8*) EmMgr.pArray + EmMgr.size * i);
        if ((em->be_flag & 0x201) == 1 && em->id > 0xF && em->id <= 0x20) {
            SatMoveSub(em, &a, &d);
        }
    }
    if (w->sat[side]) {
        w->sat[side]->setCoord(&a, &b);
    }
    if (w->eat[side]) {
        w->eat[side]->setCoord(&a, &b);
    }
    if (w->smd[side]) {
        w->smd[side]->setPos(&a);
    }
}

// `em` stands within 100 of the foot at `pos`: move it by `d`. Returns 1 when it did.
int cObjRobo::SatMoveSub(cModel* em, Vec* pos, Vec* d)
{
    Vec t;

    if (pos->x - 100.0f <= em->pos.x && pos->x + 100.0f >= em->pos.x && pos->z - 100.0f <= em->pos.z &&
        pos->z + 100.0f >= em->pos.z && fabsf(pos->y - em->pos.y) <= posysub) {
        PSVECAdd(&em->pos, d, &t);
        em->setPos(&t);
        return 1;
    }
    return 0;
}
