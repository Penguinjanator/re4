#include "types.h"
#include "vec.h"
#include "cManager.h"
#include "ctrl.h"
#include "model.h"
#include "snd.h"

void cCtrl11::move()
{
    Ctrl11Work* w = (Ctrl11Work*) work;
    int i;

    for (i = 0; i < 15; i++) {
        if (w->timer[i] != 0) {
            w->timer[i]--;
        }
    }
}

cCtrl* GetCtrlCtrl11()
{
    cCtrl* c;
    u32 i;
    u32 n = CtrlMgr.nArray;

    for (i = 0; i < n; i++) {
        c = CtrlMgrWork(i);
        if ((c->be_flag & 0x201) == 1 && c->Id == 0x11) {
            return c;
        }
    }
    c = CtrlMgr.createBack(0x11);
    if (c == 0) {
        return 0;
    }
    return c;
}

u32 Ctrl11SetSe(cCtrl* pCtrl, cModel* m, s16 time, u16 no, int idx)
{
    Ctrl11Work* w;

    if (pCtrl == 0) {
        return 0;
    }
    if (pCtrl->Id != 0x11) {
        return 0;
    }
    w = (Ctrl11Work*) pCtrl->work;
    if (w->timer[idx] != 0) {
        return 0;
    }
    w->Se_id[idx] = SndCall(8, no, &m->pos, m->id, 0, m);
    w->timer[idx] = time;
    return w->Se_id[idx];
}

u32 Ctrl11SetSe2(cCtrl* pCtrl, cModel* m, s16 time, u16 no, int idx, u16 blk)
{
    Ctrl11Work* w;

    if (pCtrl == 0) {
        return 0;
    }
    if (pCtrl->Id != 0x11) {
        return 0;
    }
    w = (Ctrl11Work*) pCtrl->work;
    w->Se_id[idx] = SndCall(blk, no, &m->getPartsPtr(0)->world, m->id, 0, m);
    w->timer[idx] = time;
    return w->Se_id[idx];
}

u32 Ctrl11StopAndSetSe(cCtrl* pCtrl, cModel* m, s16 time, u16 no, int idx)
{
    Ctrl11Work* w;

    if (pCtrl == 0) {
        return 0;
    }
    if (pCtrl->Id != 0x11) {
        return 0;
    }
    w = (Ctrl11Work*) pCtrl->work;
    SndStop(w->Se_id[idx], 0);
    w->Se_id[idx] = SndCall(8, no, &m->getPartsPtr(0)->world, m->id, 0, m);
    w->timer[idx] = time;
    return w->Se_id[idx];
}

u32 Ctrl11SetSeEm38(cCtrl* pCtrl, cModel* m, u16 no)
{
    Ctrl11Work* w;

    if (pCtrl == 0) {
        return 0;
    }
    if (pCtrl->Id != 0x11) {
        return 0;
    }
    w = (Ctrl11Work*) pCtrl->work;
    SndStop(w->Se_id_em38, 0);
    w->Se_id_em38 = SndCall(8, no, &m->getPartsPtr(0)->world, m->id, 0, m);
    return w->Se_id_em38;
}
