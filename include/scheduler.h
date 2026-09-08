#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

// Cooperative task scheduler (game/scheduler.cpp).
void TaskSleep(int frames);
extern "C" int TaskExec(int prio, void (*func)(), int arg);
void TaskExit();
void TaskSuspend(int task);
void TaskSignal(int task);
extern "C" {
void TaskKill(int prio);
// interrupt-level task variants (dvd.cpp uses them for reads flagged 0x100)
int iTaskExec(void (*func)());
void iTaskExit();
void iTaskKill();
}

#endif
