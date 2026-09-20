// game/espgen44: effect controller 44, screen filter parameter setter (D:/Bio4/Prog/espgen44.cpp).
// The record programs Filter05 (Id 0) or Filter06 (Id != 0) from its colour/size fields when set
// up, and resets the filter to zero when the controller is destroyed; it does nothing per frame.
#include "atari.h"
#include "light.h"
#include "esp.h"
#include "espgen.h"
#include "filter.h"

struct Espgen44Work {
    u8 type;           // 0x14 0 = Filter05, 1 = Filter06
};

// EspgenMoveTbl entry for controller type 0x44: nothing per frame (the filter runs on its own).
void Espgen44_Move(EspgenWork* w)
{
}

// EspgenTransTbl entry: nothing to draw.
void Espgen44_Trans(EspgenWork* w)
{
}

// Destruct entry: turns the filter the record programmed back off (Filter05SetParam / Filter06SetParam
// with all-zero parameters).
void Espgen44_Destruct(EspgenWork* w)
{
    Espgen44Work* p = (Espgen44Work*) w->work;

    switch (p->type) {
    case 0:
        Filter05SetParam(0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0);
        break;
    case 1: {
        Vec zero = {0.0f, 0.0f, 0.0f};
        Filter06SetParam(0, 0, 0, 0, 0, 0.0f, &zero, 0.0f, &zero, 0.0f, 0);
        break;
    }
    }
}

// Programs the filter from the record: level = Work8[0]*100+100, colour Col_start_rgba, Col_d_a,
// scale = Size_base_x*0.005; Filter05 (Id 0) takes Blend_type and kind Tex_id, Filter06 takes
// Speed/R_speed as vectors and kind Work8[1].
int Espgen44_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* pSct)
{
    Espgen44Work* p = (Espgen44Work*) w->work;
    if (rec->Id == 0) {
        p->type = 0;
    } else {
        p->type = 1;
    }

    switch (p->type) {
    case 0: {
        int level = (s8) rec->Work8[0] * 100 + 100;
        f32 scale = rec->Size_base_x * 0.005f;
        int kind = rec->Tex_id;
        Filter05SetParam(level, rec->Col_start_r, rec->Col_start_g, rec->Col_start_b, rec->Col_start_a, rec->Col_d_a, 0.0f, scale, rec->Blend_type, kind);
        break;
    }
    case 1: {
        int level = (s8) rec->Work8[0] * 100 + 100;
        f32 scale = rec->Size_base_x * 0.005f;
        int kind = rec->Work8[1];
        Filter06SetParam(level, rec->Col_start_r, rec->Col_start_g, rec->Col_start_b, rec->Col_start_a, rec->Col_d_a, &rec->Speed, 0.0f, &rec->R_speed, scale,
                         kind);
        break;
    }
    }
    return 1;
}

// the split object pads .rodata to 8 bytes
asm(".section .rodata; .balign 8");
