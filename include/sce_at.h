#ifndef SCE_AT_H
#define SCE_AT_H

#include "types.h"
#include "scheduler.h"

class cObj;

// Scenario collision areas (game/sce_at.cpp).
extern "C" {
// Area `no`: run `func(obj)` (prio, otPrio) when the player enters it.
void SceAtDataSet_exec(int no, int prio, int a, TaskFunc func, cObj* obj, int b);
void SceAtSetEnable(int no, int on);
}

// Area `no` follows parts `parts` of `obj`; 0 when the area does not exist.
int SceAtSetParent(int no, cObj* obj, int parts);

#endif
