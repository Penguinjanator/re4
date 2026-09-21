#ifndef T_UTIL_H
#define T_UTIL_H

#include "types.h"
#include "global.h"
#include "joy.h"
#include "eprintf.h"

// Debug tool helpers (game/t_util.cpp).

// One menu line for ToolMenuDisp_cur (0xC bytes).
struct TOOL_MENU {
    u8 Be_flg;          // 0x00  0 = greyed out (color 0x14) and not selectable
    const char* pName;   // 0x04
    void (*pFunc)();     // 0x08  called when selected (may be NULL)
};

// ToolMenuDisp_cur flag bits
#define TOOL_MENU_B_LAST   1  // B button jumps to the last entry
#define TOOL_MENU_START_LAST 2  // cursor starts on the last entry for a new menu

void TutilInitDefault();
void TutilQuitDefault();
// Draws `menu` (size in bytes) at x,y and moves the cursor with `joy`. Returns the selected
// entry when A is pressed on an enabled line, else -1. `cursor` may be NULL.
int ToolMenuDisp_cur(int x, int y, int flag, s8* cursor, TOOL_MENU* menu, int size, JOY* joy);
// tools/t_util.cpp only (dead-stripped from the DOL): screen position of a world point.
int TutilGetScreenPos(Vec* pos, f32* scr, int noSetup);
void TutilMoveCursor(Vec* pos, f32 speed, f32 step);
// t_event/t_sce: ToolMenuDisp_cur without a cursor variable
int ToolMenuDisp(int x, int y, int flag, TOOL_MENU* menu, int size, JOY* joy);
// Tools only: XZ position under a screen point (projection search around `center`, side `step`).
int TutilGet3DPosXZ(Vec* target, Vec* center, f32 step, Vec* out);
int TutilGet3DPosXZ_Mov(Vec* target, Vec* center, Vec* out);
int TutilGet3DPosXZ_All(Vec* target, Vec* center, Vec* out);

// Prints `n` menu strings one row (14 px) apart; inlined (the giv inits land after the PRE'd pointer
// high parts in the preheader) (t_cons, t_scroll; t_tplview has its own `*tbl++` form).
static inline void dispList(int x, int y, char** tbl, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        eprintf(x, y + i * 14, 0, 0, tbl[i]);
    }
}

#endif
