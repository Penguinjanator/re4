#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

// Cooperative task scheduler (game/scheduler.cpp).
void TaskSleep(int frames);
void TaskExit();

#endif
