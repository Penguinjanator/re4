#include "types.h"
#include "event.h"
#include "atari.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "global.h"
#include "main.h"
#include "main_mem.h"
#include "scheduler.h"
#include "libgpu.h"
#include "room_data.h"
#include "cam_ctrl.h"
#include "player.h"
#include "pl_sub.h"
#include "em_set.h"
#include "mes.h"
#include "snd.h"
#include "fade.h"
#include "pad.h"
#include "db_log.h"
#include "sce_sys.h"

extern "C" {
void SceInitItemEvent();                       // game/sce_com.cpp
void SceAtSetSaveItem();                       // game/sce_at.cpp
void SceAtRoomSet();
void SceAtCheckMoveScrAt();
void* SceAtPtr(int no);
int SceAtItemFlgCk(int no);
void SubMissionCheck();                        // game/stage.cpp
int getRoomEtcBreak(void* p, cEm** em, int a); // game/EtcModel.cpp
void SubScreenWait(int frames);                // game/sscrn.cpp
void GXDrawDone();
}

#line 34 "D:/Bio4/Prog/sce_sys.cpp"

cSceSys SceSys;
static ScePrim* pCSceTask;
static u32 SceExecOt;

void ScenarioInit()
{
    memclr_asm(&SceSys, sizeof(cSceSys));
}

void ScenarioRoomInit()
{
    cSceSys* s = &SceSys;

    s->sndFlag = 1;
    s->cancelFlagNo = -1;
    s->pause = 0;
    s->x8 = 0;
    s->xC = 0;
    s->x10 = 0;
    s->x14 = 0;
    s->cancelFunc = 0;
    s->cancelArg = 0;
    s->x134 = 0;
    s->x75 = 0;
    s->x76 = 0;
    s->eventCancel = 0;
    s->x6D = 0;
    s->x6C = 0;
    s->x6E = 0;
    s->x6F = 0;
    s->x70 = 0;
    pG->flags_51BC &= ~0x80;
    ScenarioTaskAllOff();
    SceInitItemEvent();
    SceAtSetSaveItem();
    if (!(pG->flags_68 & 0x4000000)) {
        SceExecInitCondition();
        RoomData.execInitFunc(pG->room_id);
        s->scheduler();
    }
    if (pG->flags_5018 & 0x4000000) {
        SubCharInit(1, &pG->sub_pos, pG->sub_angle);
        SubCharCtrl(1, 0);
    }
    EmSetFromList();
    SceAtRoomSet();
}

void ScenarioTaskAllOff()
{
    u32 i;

    ClearOTagR(SceSys.otag, 16);
    memclr_asm(SceSys.prim, sizeof(SceSys.prim));
    for (i = 5; i <= 17; i++) {
        SceKill((int) i);
    }
}

void scenarioLoopBeforeInit()
{
    SceSys.x7A = 0x3C;
    if (pSUB != 0) {
        SceSys.x7A = 0x5A;
    }
    SceExecCheckCondition();
}

void scenarioLoopAfterInit()
{
    SubMissionCheck();
    SceAtCheckMoveScrAt();
}

void ScenarioMove()
{
    if (pG->flags_68 & 0x4000000) {
        return;
    }
    scenarioLoopBeforeInit();
    if (SceSys.x76 == 0 && SceSys.pause == 0) {
        if (!(pG->flags_500C & 0x100000)) {
            scenarioCheckEventCancel();
            RoomData.execMainFunc(pG->room_id);
        }
        if (!(pG->flags_170 & 0x400) || (pG->flags_6C & 0x80)) {
            EvtMgr.Run();
        }
    }
    SceSys.scheduler();
    scenarioLoopAfterInit();
}

u32* scenarioSetOtStart()
{
    return &SceSys.otag[15];
}

u32* scenarioGetOtAddr(u32* p)
{
    u32 v;

    for (;;) {
        v = *p;
        if (v == 0xFFFFFFFF) {
            return 0;
        }
        p = (u32*) (v | 0x80000000);
        if ((s32) v < 0) {
            return p;
        }
    }
}

void SceTaskDelete(TASK* t)
{
    ScePrim* p;
    u32* ot = scenarioSetOtStart();

    while ((p = (ScePrim*) scenarioGetOtAddr(ot)) != 0) {
        ot = (u32*) p;
        if (p->task == t) {
            DelPrim(&SceSys.otag[15], (u32*) p);
            if (p->cancel == 1) {
                p->cancel = 0;
                SceSys.eventCancel = 0;
            }
            return;
        }
    }
}

void cSceSys::scheduler()
{
    OSThread* parent = ParentThread();
    TASK* ctask;
    ScePrim* p;
    u8 running;
    int i;
    int waitRead;

    pParentThread = 0;
    ctask = CTASK.p;
    if (x76 == 0 && pause == 0) {
        for (i = 0; i < 13; i++) {
            prim[i].running = 0;
        }
    } else {
        prim[0].running = 0;
        for (i = 1; i < 13; i++) {
            prim[i].running = 1;
        }
    }
    p = (ScePrim*) scenarioSetOtStart();
    while ((p = (ScePrim*) scenarioGetOtAddr((u32*) p)) != 0) {
        if (pG->flags_170 & 0x800000) {
            break;
        }
        running = p->running;
        if (running != 0) {
            continue;
        }
        p->running = 1;
        pCSceTask = p;
        pCTask = p->task;
        TaskSchedulerMain(p->task);
        if (CTASK->status == 3) {
            SceTaskDelete(CTASK.p);
            CTASK->status = running;
        }
        if (pG->x20 == 2) {
            waitRead = (x73 != 0) ? 1 : 0;
            if (waitRead) {
                pParentThread = parent;
                pCTask = ctask;
                TaskSleep(1);
                parent = ParentThread();
                ctask = CTASK.p;
                pParentThread = 0;
                p->running = 0;
            }
        }
        if (wait == 1) {
            pParentThread = parent;
            pCTask = ctask;
            GXDrawDone();
            parent = ParentThread();
            ctask = CTASK.p;
            pParentThread = 0;
            p->running = 0;
            wait = 0;
        }
        p = (ScePrim*) scenarioSetOtStart();
    }
    pParentThread = parent;
    pCTask = ctask;
}

ScePrim* SceExec(int prio, TaskFunc func, int arg, u8 flag, int otPrio, void* model)
{
    TASK* t;
    ScePrim* p;
    u32 no;
    int busy;
    int i;

    if (prio == 0) {
        SetTaskModelPtr(model, 0);
        ((void (*)(int)) func)(arg);
        return 0;
    }
    if ((u32) prio > 17) {
        prio = 0;
        if (Task[17].status == 0) {
            prio = 17;
        } else {
            no = 17;
            for (;;) {
                no--;
                if (no <= 5) {
                    break;
                }
                if (Task[no].status == 0) {
                    prio = no;
                    break;
                }
            }
        }
    }
    if ((u32) (prio - 5) > 12) {
        pLog->err(0, 0, "SCE_TASK DON'T EXEC");
        return 0;
    }
    t = TaskExec(prio, func, arg);
    if (t == 0) {
        return 0;
    }
    p = &SceSys.prim[prio - 5];
    p->cancel = 0;
    p->task = t;
    AddPrim(&SceSys.otag[otPrio], (u32*) p);
    t->level = 0xE;
    SetTaskModelPtr(model, t);
    if (flag != 0) {
        t->flag = flag;
    } else if (SceSys.checkCTaskRange() == 0) {
        t->flag = 1;
    } else {
        t->flag = pCTask->flag;
    }
    if (!(pG->flags_64 & 0x200000)) {
        busy = 0;
        for (i = 17; i >= 6; i--) {
            if (Task[i].status != 0) {
                busy++;
            }
        }
        if ((u32) (12 - busy) <= 2) {
            pLog->warn(0, 0, "SCE_TASK remain num %d", 12 - busy);
        }
    }
    return p;
}

void SceSleep(int frames)
{
    if (SceSys.checkCTaskRange() != 0) {
        TaskSleep(frames);
    }
}

void SceExit()
{
    if (SceSys.checkCTaskRange() != 0) {
        SceTaskDelete(pCTask);
        TaskExit();
    }
}

void SceKill(int prio)
{
    SceTaskDelete(&Task[prio]);
    TaskKill(prio);
}

void SceKill(ScePrim* p)
{
    SceTaskDelete(p->task);
    TaskKill(p->task);
}

void SceKill(TASK* t)
{
    SceTaskDelete(t);
    TaskKill(t);
}

void SceKill(void (*func)(int))
{
    ScePrim* p = (ScePrim*) scenarioSetOtStart();

    while ((p = (ScePrim*) scenarioGetOtAddr((u32*) p)) != 0) {
        if (p->task->func == func) {
            SceKill(p->task);
        }
    }
}

int cSceSys::checkCTaskRange()
{
    if (pCTask == 0) {
        return 0;
    }
    if (pCTask == CTASK_MAIN) {
        return 0;
    }
    return (u32) (pCTask->no - 5) <= 12;
}

ScePrim* SceCTask()
{
    return pCSceTask;
}

void SceExecInitCondition()
{
    ClearOTagR(&SceExecOt, 1);
}

int SceExecCheckCondition_sub(SceCond* c)
{
    cEm* em;
    u32 no;
    u32 bit;

    switch (c->type) {
    case 0:
        no = (u32) c->param;
        bit = 0;
        if (pG->emlist_no >= 0) {
            bit = pG->em_dead[pG->emlist_no][no >> 5] & (0x80000000 >> (no & 31));
        }
        if (bit != 0) {
            return 1;
        }
        break;
    case 1:
        if (CamCtrl.CurrentAreaNo() == (int) c->param) {
            return 1;
        }
        break;
    case 2:
        em = (cEm*) c->param;
        if (em->life <= 0 && em->x0FC == 3) {
            return 1;
        }
        break;
    case 3:
        if (((int (*)()) c->param)() == 1) {
            return 1;
        }
        break;
    case 4:
        if (getRoomEtcBreak(c->param, &em, 1) == 1 && em->life <= 0) {
            return 1;
        }
        break;
    case 5:
        if (SceAtPtr((int) c->param) != 0 && SceAtItemFlgCk((int) c->param) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

void SceExecCheckCondition()
{
    SceCond* c = (SceCond*) SceExecOt;
    SceCond* next;

    if ((u32) c == 0xFFFFFFFF) {
        return;
    }
    do {
        next = (SceCond*) (((u32) c->next) | 0x80000000);
        c = (SceCond*) (((u32) c) | 0x80000000);
        if ((s32) c < 0) {
        }
        c = next;
    } while ((u32) c->next != 0xFFFFFFFF);
}

void SceExecLinkCondition(int type, void* param, u8 prio, TaskFunc func, int arg, u8 flag)
{
#line 608
    SceCond* c = (SceCond*) MEM_ALLOC(sizeof(SceCond), 1, 13);

    c->type = type;
    c->param = param;
    c->prio = prio;
    c->func = func;
    c->arg = arg;
    c->flag = flag;
    AddPrim(&SceExecOt, (u32*) c);
}

void SceExecLinkEmDead(void* param, u8 prio, TaskFunc func, int arg, u8 flag)
{
    SceExecLinkCondition(2, param, prio, func, arg, flag);
}

int EmMoveActiveCheck(cEm* em)
{
    if (!(em->flags & 1) || !(em->flags & 0x20) || !(em->flags & 2)) {
        return 0;
    }
    if (em->life <= 0) {
        return 0;
    }
    if (em == pPL) {
        return 0;
    }
    return 1;
}

void SceExecEventCancel()
{
    MessageControl* mes = &cMes;
    cSceSys* s;
    int i;
    u32 no;
    GXColor start;
    GXColor end;

    pG->flags_170 = SceSys.stop_bak;
    for (i = 0; i <= 0xF; i++) {
        mes->Delete(i);
    }
    s = &SceSys;
    if (s->sndFlag == 1) {
        SndEventStrStop(0);
    }
    if (s->cancelFlagNo >= 0) {
        no = s->cancelFlagNo;
        (&pG->flags_174)[no >> 5] |= 0x80000000 >> (no & 31);
    }
    for (i = 5; i <= 17; i++) {
        if (s->prim[i - 5].cancel == 1) {
            SceKill(i);
            s->prim[i - 5].cancel = 0;
            break;
        }
    }
    if (SceSys.cancelFunc != 0) {
        SceExec(i, SceSys.cancelFunc, SceSys.cancelArg, 2, 2, 0);
    }
    *(u32*) &start = 0xFF;
    *(u32*) &end = 0;
    FadeSet(0x80000000, &start, &end, 10, 0, 0);
}

void SceSetEventCancel(int on, TaskFunc func, int arg, int flagNo, int sndFlag)
{
    u32 no;

    if (on != 1) {
        on = 0;
    }
    SceSys.eventCancel = on;
    SceCTask()->cancel = on;
    if (flagNo >= 0) {
        no = flagNo;
        (&pG->flags_174)[no >> 5] &= ~(0x80000000 >> (no & 31));
    }
    SceSys.sndFlag = sndFlag;
    SceSys.cancelFunc = func;
    SceSys.cancelArg = arg;
    SceSys.cancelFlagNo = flagNo;
}

int scenarioCheckEventCancel()
{
    cSceSys* s = &SceSys;
    GXColor start;
    GXColor end;

    if (s->eventCancel == 1 && (Key.trg & 0x20000000)) {
        *(u32*) &end = 0xFF;
        *(u32*) &start = 0;
        FadeSet(0, &start, &end, 0, 0, 0);
        s->stop_bak = pG->flags_170;
        pG->flags_170 = 0xFFFFFFFF;
        KeyStop(0xEFCF0000);
        SceExecEventCancel();
        s->eventCancel = 0;
        SubScreenWait(0x14);
        return 1;
    }
    return 0;
}
