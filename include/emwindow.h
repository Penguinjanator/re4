#ifndef EMWINDOW_H
#define EMWINDOW_H

#include "types.h"
#include "vec.h"
#include "emobj.h"

// Breakable window / fence enemy (game/emwindow.cpp). Only the entry points other units
// call are declared here.
class cEmWindow : public cEmObj {
public:
    int ChkStatus();        // etc flag word of this window (GetEtcFlgPtr), 0 when none; bit0 = broken
    void SetBreakModel();
};

// bin/tpl of the window model, position / rotation, `flag` (Et*_init 4th argument),
// WindowData row `type`, and the room archive the effect data comes from.
cEmWindow* SetWindow(void* bin, void* tpl, Vec* pos, Vec* rot, int flag, u8 type, void* arc);

#endif
