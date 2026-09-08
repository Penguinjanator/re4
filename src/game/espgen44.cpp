#include "atari.h"
#include "light.h"
#include "esp.h"
#include "espgen.h"
#include "filter.h"

struct Espgen44Work {
    u8 type;           // 0x14 0 = Filter05, 1 = Filter06
};

void Espgen44_Move(EspgenWork* w)
{
}

void Espgen44_Trans(EspgenWork* w)
{
}

void Espgen44_Destruct(EspgenWork* w)
{
    Espgen44Work* p = (Espgen44Work*) w->work;

    switch (p->type) {
    case 0:
        Filter05SetParam(0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f);
        break;
    case 1: {
        Vec zero = {0.0f, 0.0f, 0.0f};
        Filter06SetParam(0, 0, 0, 0, 0, &zero, &zero, 0, 0.0f, 0.0f, 0.0f);
        break;
    }
    }
}

int Espgen44_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u8 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, void* p8, int flag)
{
    Espgen44Work* p = (Espgen44Work*) w->work;
    if (rec->x1 == 0) {
        p->type = 0;
    } else {
        p->type = 1;
    }

    switch (p->type) {
    case 0: {
        int level = (s8) rec->xC8 * 100 + 100;
        f32 scale = rec->x88 * 0.005f;
        int kind = rec->x2;
        Filter05SetParam(level, rec->x9C, rec->x9D, rec->x9E, rec->x9F, rec->xC2, kind, rec->xAC, 0.0f, scale);
        break;
    }
    case 1: {
        int level = (s8) rec->xC8 * 100 + 100;
        f32 scale = rec->x88 * 0.005f;
        int kind = rec->xC9;
        Filter06SetParam(level, rec->x9C, rec->x9D, rec->x9E, rec->x9F, &rec->x24, &rec->x34, kind, rec->xAC, 0.0f,
                         scale);
        break;
    }
    }
    return 1;
}

// the split object pads .rodata to 8 bytes
asm(".section .rodata; .balign 8");
