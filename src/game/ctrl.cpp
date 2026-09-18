// game/ctrl.cpp: the control work manager (CtrlMgr). A cCtrl is a small per-room helper object
// (0x214 bytes) with a virtual move / trans, specialised by id at construction: 0 / 1 light
// path controls, 0x10, 0x11 shared SE handles, 0x12 shared timers / counters / texture render
// targets, 0x14 the dragon head statue. CtrlMgr.move runs every live control each frame.

#include "types.h"
#include "cManager.h"
#include "ctrl.h"
#include "light.h"

// A cManager<cCtrl> pool (type 2).
cCtrlMgr::cCtrlMgr() : cManager<cCtrl>(sizeof(cCtrl), 2)
{
    setName("cCtrlMgr");
}

// Places the cCtrl subclass for `id` into the fresh work (unknown ids get the base class) and
// marks it live.
int cCtrlMgr::construct(cCtrl* p, u32 id)
{
    p->Id = id;
    switch (id) {
    case 0:
        new (p) cCtrl00;
        p->be_flag = 1;
        break;
    case 1:
        new (p) cCtrl01;
        p->be_flag = 1;
        break;
    case 0x10:
        new (p) cCtrl10;
        p->be_flag = 1;
        break;
    case 0x11:
        new (p) cCtrl11;
        p->be_flag = 1;
        break;
    case 0x12:
        new (p) cCtrl12;
        p->be_flag = 1;
        break;
    case 0x14:
        new (p) cCtrl14;
        p->be_flag = 1;
        break;
    default:
        new (p) cCtrl;
        p->be_flag = 1;
        break;
    }
    return 1;
}

// Per-frame: dieCheck, then move() on every live control.
void cCtrlMgr::move()
{
    u32 i;

    dieCheck();
    for (i = 0; i < nArray; i++) {
        cCtrl* p = (cCtrl*) ((u8*) pArray + size * i);
        if ((p->be_flag & 0x201) == 1) {
            p->move();
        }
    }
}

// Draw registration: trans() on every live control.
int cCtrlMgr::trans()
{
    u32 i;

    for (i = 0; i < nArray; i++) {
        cCtrl* p = (cCtrl*) ((u8*) pArray + size * i);
        if ((p->be_flag & 0x201) == 1) {
            p->trans();
        }
    }
    return 1;
}

// Base control: nothing per frame.
void cCtrl::move()
{
}

// Base control: nothing to draw.
void cCtrl::trans()
{
}

cCtrlMgr CtrlMgr;
