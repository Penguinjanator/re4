#include "atari.h"
#include "act_btn.h"
#include "global.h"
#include "libgpu.h"
#include "cockpit.h"
#include "mes.h"
#include "id_sys.h"
#include "main.h"
#include "player.h"
#include "pl_sub.h"
#include "sce_sys.h"
#include "sce_at.h"

cActionButton ActBtn;

typedef void (*ActBtnFunc)(int arg, int d);

void cActionButton::init()
{
    int stop;

    ClearOTagR(ot, 16);
    num = 0;
    BitOff(pG->flags_500C, 0x4000);
    BitOff(pG->flags_500C, 0x200000);
    stop = 1;
    if (!(pG->flags_170 & 0x100)) {
        stop = 0;
    }
    this->stop = stop;
}

void cActionButton::move()
{
    u32 tag;
    ActBtnWork* w;

    Cckpt.action.no = 0;
    active = 0;
    if ((pG->flags_170 & 0x100) || (pG->flags_500C & 0x100000) || stop) {
        init();
        return;
    }
    for (tag = ot[15]; tag != 0xFFFFFFFF; tag = w->tag) {
        w = (ActBtnWork*) (tag | 0x80000000);
        if ((s32) tag >= 0) {
            continue;
        }
        if (w->flags & 0x20) {
            break;
        }
        if (checkPLStatus(w) == 0) {
            continue;
        }
        active = 1;
        if (!(pG->flags_58 & 0x1000) && !(w->flags & 8)) {
            disp(w);
        }
        if (checkButton(w) == 1 && w->func != 0) {
            int flag = 1;

            if (w->flags & 4) {
                flag = 2;
            }
            switch (w->type) {
            case 0:
                ((ActBtnFunc) w->func)(w->arg, w->d);
                break;
            case 1:
                SceExec(0x12, (TaskFunc) w->func, w->arg, flag, w->slot, (void*) w->d);
                break;
            case 2: {
                SceAtWork* at = (SceAtWork*) w->arg;

                SceAtSetExecFlg(at->no);
                ((ActBtnFunc) w->func)(w->arg, w->d);
                if (at->x38 & 0x80) {
                    SceAtSetEnable(at->no, 0);
                }
                break;
            }
            }
        }
        break;
    }
    init();
}

void cActionButton::disp(ActBtnWork* w)
{
    int col = 0;
    u8 kind = w->kind;
    u8 btn = w->btn;
    IdUnit* u;
    s16 sx;
    s16 sy;
    int x;
    int y;

    if (w->flags & 0x80) {
        col = 7;
    }
    cMes.setLayout(1, 1);
    switch (btn) {
    case 1:
    case 6:
    case 0xC:
    case 0xF:
    case 0x10:
        u = IdSys.unitPtr(1, 0x20);
        sx = (s16) u->scr.x;
        sy = (s16) u->scr.y;
        x = (s16) (((f32) sx + 320.0f) * 0.8f);
        y = cMes.mes[1].fontH / 2;
        y = (s16) ((240.0f - (f32) sy) * 0.8f) - y;
        cMes.MesSet(kind + 0x16, x, (s16) y, 0x200F1, 1, col, 4);
        break;
    default:
        u = IdSys.unitPtr(0xF0, 0x20);
        sx = (s16) u->scr.x;
        sy = (s16) u->scr.y;
        x = (s16) (((f32) sx + 320.0f) * 0.8f);
        y = cMes.mes[1].fontH / 2;
        y = (s16) ((240.0f - (f32) sy) * 0.8f) - y;
        cMes.MesSet(kind + 0x16, x, (s16) y, 0xF1, 1, col, 4);
        break;
    }
    Cckpt.action.no = btn;
}

int cActionButton::checkButton(ActBtnWork* w)
{
    u32 on = Key.on & 0x00CF0000;
    u32 trg = Key.trg & 0x00CF0000;
    u64 key;
    u32 flags;

    switch (w->btn) {
    case 1:
    case 2:
    case 0xE:
        flags = w->flags;
        if (!(flags & 0x40)) {
            if (!(flags & 2)) {
                if (flags & 0x10) {
                    if (on & 0x80000) {
                        return 1;
                    }
                    return 0;
                } else {
                    if (pG->flags_500C & 0x4000) {
                        return 1;
                    }
                    return 0;
                }
            } else {
                if (flags & 0x10) {
                    if (on & 0x80000) {
                        return 1;
                    }
                    return 0;
                } else {
                    if (trg & 0x80000) {
                        return 1;
                    }
                    return 0;
                }
            }
        } else {
            if (!(flags & 2)) {
                if (flags & 0x10) {
                    if (!(on & 0x80000)) {
                        return 0;
                    }
                    key = on;
                    if (key & ~(u64) 0x80000) {
                        return 0;
                    }
                    return 1;
                } else {
                    if (!(pG->flags_500C & 0x4000)) {
                        return 0;
                    }
                    key = trg;
                    if (key & ~(u64) 0x80000) {
                        return 0;
                    }
                    return 1;
                }
            } else {
                if (flags & 0x10) {
                    if (!(on & 0x80000)) {
                        return 0;
                    }
                    key = on;
                    if (key & ~(u64) 0x80000) {
                        return 0;
                    }
                    return 1;
                } else {
                    if (!(trg & 0x80000)) {
                        return 0;
                    }
                    key = trg;
                    if (key & ~(u64) 0x80000) {
                        return 0;
                    }
                    return 1;
                }
            }
        }
    case 3:
        if ((trg & 0xC00000) == 0xC00000 || ((on & 0x400000) && (trg & 0x800000)) ||
            ((trg & 0x400000) && (on & 0x800000))) {
            if (!(w->flags & 0x40)) {
                return 1;
            }
            if ((on & 0xC0000) == 0xC0000) {
                return 0;
            }
            return 1;
        }
        return 0;
    case 4:
        if ((trg & 0xC0000) == 0xC0000 || ((on & 0x80000) && (trg & 0x40000)) ||
            ((trg & 0x80000) && (on & 0x40000))) {
            if (!(w->flags & 0x40)) {
                return 1;
            }
            if ((on & 0xC00000) == 0xC00000) {
                return 0;
            }
            return 1;
        }
        return 0;
    case 5:
        if (!(trg & 0x40000)) {
            return 0;
        }
        if (!(w->flags & 0x40)) {
            return 1;
        }
        key = on;
        if ((key & ~(u64) 0x40000) == 0) {
            return 1;
        }
        return 0;
    case 6:
        if (!(trg & 0x20000)) {
            return 0;
        }
        if (!(w->flags & 0x40)) {
            return 1;
        }
        key = on;
        if ((key & ~(u64) 0x20000) == 0) {
            return 1;
        }
        return 0;
    case 7:
        if (!(trg & 0x10000)) {
            return 0;
        }
        if (!(w->flags & 0x40)) {
            return 1;
        }
        key = on;
        if ((key & ~(u64) 0x10000) == 0) {
            return 1;
        }
        return 0;
    case 9:
        return 0;
    case 0xA:
        return 0;
    }
    return 0;
}

int cActionButton::checkPLStatus(ActBtnWork* w)
{
    if (pPL->hp > 0) {
        if ((w->flags & 2) || pPL->actCheck() != 0) {
            switch (w->btn) {
            case 1:
            case 2:
            case 4:
            case 0xE:
                if (PlGetStatus() & 0x10) {
                    if (w->flags & 1) {
                        BitOn(pG->flags_500C, 0x200000);
                        return 1;
                    }
                    return 0;
                }
                break;
            }
            return 1;
        }
    }
    return 0;
}

ActBtnWork* cActionButton::pullWork()
{
    ActBtnWork* w;

    if (num > 7) {
        return 0;
    }
    w = &work[num];
    num++;
    return w;
}

void cActionButton::set(int kind, int slot, int func, int arg, int flags, int btn, int type, int d)
{
    ActBtnWork* w = pullWork();

    if (w == 0) {
        return;
    }
    if (kind > 0x41) {
        kind = 0x41;
    }
    w->kind = kind;
    w->func = (void*) func;
    w->arg = arg;
    w->type = type;
    w->flags = flags;
    w->btn = btn;
    w->slot = slot;
    w->d = d;
    AddPrim(&ot[slot], (u32*) w);
    switch (w->btn) {
    case 1:
    case 2:
    case 4:
    case 0xE:
        if (w->flags & 1) {
            BitOn(pG->flags_500C, 0x200000);
        }
        break;
    }
}
