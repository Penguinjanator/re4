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

// Filter05SetParam with all-immediate arguments: the original issues the FPR argument copies
// interleaved before the trailing `li`s (`fmr f2; li r7; fmr f3; li r8; li r9`), which GCC 2.95
// only produces when the float parameters are declared first (atari_init.h idiom).
void Filter05SetParamF(f32 x, f32 y, f32 z, int a, int b, int c, int d, int e, int f, int g) asm("Filter05SetParam__Fiiiiiiifff");
// Same call with loaded arguments (Espgen44_SetFreeWork): there the original's move order is the
// five leading ints, the three floats, then the two trailing ints (`lbz r9` early, `lfs f1` after
// `lbz r7`, `lbz r8` last) — a per-call-site interleaving (the only one of 7 tried that matches).
void Filter05SetParamM(int a, int b, int c, int d, int e, f32 x, f32 y, f32 z, int f, int g) asm("Filter05SetParam__Fiiiiiiifff");

void Espgen44_Destruct(EspgenWork* w)
{
    Espgen44Work* p = (Espgen44Work*) w->work;

    switch (p->type) {
    case 0:
        Filter05SetParamF(0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0, 0);
        break;
    case 1: {
        Vec zero = {0.0f, 0.0f, 0.0f};
        Filter06SetParam(0, 0, 0, 0, 0, 0.0f, &zero, 0.0f, &zero, 0.0f, 0);
        break;
    }
    }
}

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
        Filter05SetParamM(level, rec->Col_start_r, rec->Col_start_g, rec->Col_start_b, rec->Col_start_a, rec->Col_d_a, 0.0f, scale, rec->Blend_type, kind);
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
