// game/scheduler: cooperative task scheduler on top of OS threads (D:/Bio4/Prog/scheduler.cpp).
#include "types.h"
#include "global.h"
#include "main_mem.h"
#include "db_log.h"
#include "eprintf.h"
#include "os_vi.h"
#include "scheduler.h"

extern "C" {
void OSReport(const char* msg, ...);
BOOL OSDisableInterrupts(void);
BOOL OSRestoreInterrupts(BOOL level);
void GXSetCurrentGXThread(void);
}
void DbMenuRestoreStopFlag();

TASK Task[TASK_NUM];
static OSSemaphore Sema;
TASK* CTASK_MAIN = (TASK*) -1;
TASK* pCTask = CTASK_MAIN;
OSThread* pParentThread;
static int iTask_exec_flg = 0;

void TaskKill(TASK* t);

void TaskSchedulerInit()
{
    u32 total = 0;
    u8* stack;
    u32 i;

    for (i = 0; i < TASK_NUM; i++) {
        Task[i].StackSize = GetStackSize(i);
        total += Task[i].StackSize;
    }
#line 48 "D:/Bio4/Prog/scheduler.cpp"
    stack = (u8*) MEM_ALLOC(total, 1, 13);
    memset_asm(stack, 0xB3, total);
    for (i = 0; i < TASK_NUM; i++) {
        Task[i].Task_no = i;
        Task[i].Status = 0;
        Task[i].pStack = stack + Task[i].StackSize;
        Task[i].suspend_cnt = 0;
        OSInitThreadQueue(&Task[i].Queue);
        stack = Task[i].pStack;
    }
    OSInitSemaphore(&Sema, 0);
    iTask_exec_flg = 0;
}

void TaskAllClear()
{
    u32 i;

    for (i = 0; i < TASK_NUM; i++) {
        TaskKill(i);
    }
    iTask_exec_flg = 0;
}

u32 GetStackSize(int no)
{
    if (no == 2) {
        return 0x3000;
    }
    if ((u32) no > 4) {
        return 0x1800;
    }
    return 0x2000;
}

void TaskScheduler()
{
    u32 i;
    TASK* t;

    pParentThread = OSGetCurrentThread();
    for (i = 0, t = Task; i <= TASK_ISR; i++, t++) {
        if (i == TASK_ISR) {
            continue;
        }
        if (i == 2) {
            DbMenuRestoreStopFlag();
        }
        pCTask = t;
        TaskSchedulerMain(t);
    }
    pCTask = CTASK_MAIN;
    if (pG->debug_mode == 6) {
        stackUsedCheck();
    }
}

void TaskSchedulerMain(TASK* t)
{
    if ((pG->Status_flg[1] & 0x10000000) && !(t->flag & 2)) {
        return;
    }
    if ((pG->Status_flg[0] & 0x100000) && !(t->flag & 4)) {
        return;
    }
    switch (t->Status) {
    case TASK_EXEC:
        OSCreateThread(&t->Thread, t->hook, (void*) t->arg, t->pStack, t->StackSize, t->Priority, 1);
        t->Status = TASK_RUN;
        OSResumeThread(&t->Thread);
        GXSetCurrentGXThread();
        break;
    case TASK_SLEEP:
        t->SleepCtr--;
        if (t->SleepCtr != 0) {
            return;
        }
        t->Status = TASK_RUN;
        OSWakeupThread(&t->Queue);
        GXSetCurrentGXThread();
        break;
    case TASK_RUN:
        OSResumeThread(&t->Thread);
        GXSetCurrentGXThread();
        break;
    default:
        return;
    }
    if (CTASK->Priority > 0xF) {
        OSWaitSemaphore(&Sema);
        GXSetCurrentGXThread();
    }
    StackOverflowCheck(t);
}

void stackUsedCheck()
{
    u32 i;
    int y = 0x24;

    eprintf2(9, 0x10, 0x22, 0x24, 0, 6, "   SIZE REST");
    for (i = 0; i < TASK_NUM; i++) {
        u32* p = (u32*) (Task[i].pStack - Task[i].StackSize);
        u32 n = 1;
        p++;
        while (n < (u32) (Task[i].StackSize >> 2) && *p == 0xB3B3B3B3) {
            n++;
            p++;
        }
        y += 0x10;
        eprintf(0x10, y, 0, 6, "%02d %4x %4x %08x", i, Task[i].StackSize, n * 4, Task[i].pStack - Task[i].StackSize);
    }
}

void StackOverflowCheck(TASK* t)
{
    if (*(u32*) (t->pStack - t->StackSize) != 0xDEADBABE) {
        OSReport("***************************************\n");
        OSReport("Stack overflow in Thread %d !!\n", t->Task_no);
        OSReport("***************************************\n");
        OSPanic("D:/Bio4/Prog/scheduler.cpp", 217, "End of biohazard4");
    }
}

void* TaskExec_hook(void* value)
{
    if (ParentThread() != NULL) {
        OSSuspendThread(ParentThread());
    }
    asm("li 3, 4\n"
        "oris 3, 3, 4\n"
        "mtspr 914, 3\n"
        "li 3, 5\n"
        "oris 3, 3, 5\n"
        "mtspr 915, 3\n"
        "li 3, 6\n"
        "oris 3, 3, 6\n"
        "mtspr 916, 3\n"
        "li 3, 7\n"
        "oris 3, 3, 7\n"
        "mtspr 917, 3"
        :
        :
        : "r3");
    GXSetCurrentGXThread();
    CTASK->pFunc((int) value);
    return NULL;
}

TASK* TaskExec(int prio, TaskFunc func, int arg)
{
    TASK* t;

    if (TaskStatus(prio) != 0) {
        pLog->err(0, 0, "TASK DON'T EXEC : level %d", prio);
        return NULL;
    }
    t = &Task[prio];
    t->hook = TaskExec_hook;
    t->pFunc = (void (*)(int)) func;
    t->Status = TASK_EXEC;
    t->arg = arg;
    t->flag = 6;
    t->Priority = 0xF;
    return t;
}

void TaskSleep(int frames)
{
    if (frames == 0) {
        return;
    }
    CTASK->SleepCtr = frames;
    CTASK->Status = (CTASK->Status & TASK_SUSPEND) | TASK_SLEEP;
    if (ParentThread() != NULL) {
        OSResumeThread(ParentThread());
    }
    if (CTASK->Priority > 0xF) {
        OSSignalSemaphore(&Sema);
    }
    OSSleepThread(&pCTask->Queue);
    if (ParentThread() != NULL) {
        OSSuspendThread(ParentThread());
    }
    GXSetCurrentGXThread();
}

void TaskChain(TaskFunc func, int arg)
{
    CTASK->hook = TaskExec_hook;
    CTASK->pFunc = (void (*)(int)) func;
    CTASK->Status = TASK_EXEC;
    CTASK->arg = arg;
    if (ParentThread() != NULL) {
        OSResumeThread(ParentThread());
    }
    if (CTASK->Priority > 0xF) {
        OSSignalSemaphore(&Sema);
    }
    OSExitThread(&pCTask->Thread);
}

void TaskExit()
{
    TASK* t = pCTask;

    t->Status = TASK_NONE;
    t->suspend_cnt = 0;
    if (ParentThread() != NULL) {
        OSResumeThread(ParentThread());
    }
    if (CTASK->Priority > 0xF) {
        OSSignalSemaphore(&Sema);
    }
    OSExitThread(&pCTask->Thread);
}

void TaskKill(int prio)
{
    TaskKill(&Task[prio]);
}

void TaskKill(TASK* t)
{
    if (t->Status == 0) {
        return;
    }
    switch (t->Status & ~TASK_SUSPEND) {
    case TASK_NONE:
    case TASK_EXEC:
        break;
    case TASK_SLEEP:
        OSCancelThread(&t->Thread);
        break;
    case TASK_RUN:
        if (t->Status & TASK_SUSPEND) {
            OSCancelThread(&t->Thread);
        } else {
            TaskExit();
        }
        break;
    }
    t->Status = TASK_NONE;
    t->suspend_cnt = 0;
}

void TaskSuspend(int task)
{
    TASK* t = &Task[task];

    t->suspend_cnt++;
    t->Status |= TASK_SUSPEND;
}

void TaskSignal(int task)
{
    TASK* t = &Task[task];

    if (t->suspend_cnt == 0) {
        return;
    }
    t->suspend_cnt--;
    if (t->suspend_cnt != 0) {
        return;
    }
    t->Status &= ~TASK_SUSPEND;
}

u8 TaskStatus(int prio)
{
    return Task[prio].Status;
}

void SetTaskModelPtr(void* model, TASK* t)
{
    if (t == NULL) {
        CTASK->pModel = model;
    } else {
        t->pModel = model;
    }
}

void iTaskScheduler()
{
    BOOL lv = OSDisableInterrupts();

    if (iTask_exec_flg == 1) {
        TASK* t = &Task[TASK_ISR];
        pParentThread = NULL;
        pCTask = t;
        t->Status &= ~TASK_SUSPEND;
        TaskSchedulerMain(t);
        pCTask = CTASK_MAIN;
    }
    OSRestoreInterrupts(lv);
}

TASK* iTaskExec(TaskFunc func)
{
    TASK* t;
    int flg = iTask_exec_flg;

    if (flg == 0) {
        iTask_exec_flg = 1;
        t = TaskExec(TASK_ISR, func, 0);
        t->Priority = flg;
        return t;
    }
    return NULL;
}

void iTaskKill()
{
    iTask_exec_flg = 0;
    TaskKill(&Task[TASK_ISR]);
}

void iTaskExit()
{
    iTask_exec_flg = 0;
    TaskExit();
}

void iTaskSuspend()
{
    if (iTask_exec_flg == 1) {
        TASK* t = &Task[TASK_ISR];
        if (t->Thread.state == 2) {
            t->Status |= TASK_SUSPEND;
            OSSuspendThread(&t->Thread);
        }
    }
}

int iTaskStatus()
{
    return iTask_exec_flg;
}
