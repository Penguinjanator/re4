#ifndef SCE_H
#define SCE_H

#include "types.h"

// Scenario helpers (game/sce_com.cpp / sce_sys.cpp), C linkage.
extern "C" {
void SceEventStart(int mode);
void SceEventEnd(int mode);
void SceSleep(int frames);
}

#endif
