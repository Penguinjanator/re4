#include "light.h"
#include "atari.h"
#include "global.h"
#include "esp.h"
#include "espgen.h"
#include "math_sub.h"
#include "rnd.h"
#include "camera.h"
#include "os_vi.h"
#include "db_log.h"
#include "main_sub.h"
#include "joy.h"

// Effect controller 45: weather water surface (same height-field model as Espgen42, following the
// camera). The Estgen45Set* entry points let the room script (esp4c) override its parameters.
// TODO: Espgen45_Move00, SetIndMtx, Espgen45_TransSub and SetWaterWork45 are not written yet.

// Parameter block handed over by esp4c (Esp4cWork, 0x20 bytes).
struct Esp4cWork {
    u8 x[0x20];
};

extern "C" {
void Espgen45_Move00(EspgenWork* w);
void Espgen45_TransSub(EspgenWork* w);
int SetWaterWork45(EspgenWork* w, Vec* pos, Vec* rot, f32 size, f32 rate, u32 nx, u32 ny);
}

EspgenWork* g_pWater45;
static int g_bTargetCamera = 0;
static int g_bTargetHeight = 0;
static int g_bSizeOverWrite = 0;
static int g_bColorOverWrite = 0;
static int g_bColorMul = 0;
static int g_bSetParam = 0;
static f32 g_Target_x = 0.0f;
static f32 g_Target_y = 0.0f;
static f32 g_Target_z = 0.0f;
static f32 g_Size = 0.0f;
static u8 g_r = 0;
static u8 g_g = 0;
static u8 g_b = 0;
static u8 g_a = 0;
static f32 g_sr = 0.0f;
static f32 g_sg = 0.0f;
static f32 g_sb = 0.0f;
static f32 g_sa = 0.0f;
static f32 inv_mul = 0.0f;
static Esp4cWork g_Free;

static inline void ISet(int& d, int v) { d = v; }
static inline void U8Set(u8& d, u8 v) { d = v; }

void Espgen45_static_init()
{
    g_bTargetCamera = 1;
    g_bTargetHeight = 1;
    g_Target_x = 10000000000.0f;
    g_Target_y = -10000000000.0f;
    g_Target_z = 100000000000.0f;
    g_bSizeOverWrite = 0;
    g_bColorOverWrite = 0;
    g_bColorMul = 0;
    g_bSetParam = 0;
    g_Size = 0.0f;
    g_r = 0;
    g_g = 0;
    g_b = 0;
    g_a = 0;
    g_sr = 0.0f;
    g_sg = 0.0f;
    g_sb = 0.0f;
    g_sa = 0.0f;
}

void Espgen45_Move(EspgenWork* w)
{
    static void (*Espgen45MoveTbl[])(EspgenWork*) = {Espgen45_Move00};

    if (pG->flags_170 & 0x40000) {
        return;
    }
    Espgen45MoveTbl[w->step](w);
}

void Espgen45_Trans(EspgenWork* w)
{
    if ((w->flag & 1) && !(w->flag & 2)) {
        AddOtDirect(0x10, w, (void (*)()) Espgen45_TransSub, 1, 0x80, NULL, 0.0f);
    }
    pG->flags_5010 &= ~0x20;
}

void Espgen45_Destruct(EspgenWork* w)
{
    Espgen42Work* p = (Espgen42Work*) w->work;

    if (p->hA != NULL) {
        Mem_free(p->hA);
        p->hA = NULL;
    }
    if (p->hB != NULL) {
        Mem_free(p->hB);
        p->hB = NULL;
    }
    if (p->pos != NULL) {
        Mem_free(p->pos);
        p->pos = NULL;
    }
    if (p->nrm != NULL) {
        Mem_free(p->nrm);
        p->nrm = NULL;
    }
    if (p->bump != NULL) {
        Mem_free(p->bump);
        p->bump = NULL;
    }
    if (p->dl != NULL) {
        Mem_free(p->dl);
        p->dl = NULL;
    }
    g_pWater45 = NULL;
}

int Espgen45_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8)
{
    Espgen42Work* p = (Espgen42Work*) w->work;
    Vec r;
    u32 nx = 0x40;
    u32 ny = 0x40;
    f32 rate;

    if (EspGetTexObj(0xFE, 0) == NULL) {
        pLog->err(0, 0, "Espgen45 : WaterTex(0xfe) not found!");
        goto fail;
    }
    if (rec->flags & 1) {
        p->flag |= 1;
    }
    if (rec->flags & 0x4000) {
        p->flag |= 2;
        p->xC5 = rec->xC5;
        p->flag |= 1;
    }
    if (rec->xFC != 0) {
        nx = rec->xFC;
        if (nx > 0xB8) {
            pLog->warn(0, 0, "ESP_WATER : width > 184");
            nx = 0xB8;
        }
    }
    if (rec->xFD != 0) {
        ny = rec->xFD;
        if (ny > 0xB8) {
            pLog->warn(0, 0, "ESP_WATER : height > 184");
            ny = 0xB8;
        }
    }
    if (nx & 7) {
        u32 n = nx - (nx & 7);
        pLog->warn(0, 0, "ESP_WATER : width (%d -> %d)", nx, n);
        nx = n;
    }
    if (ny & 7) {
        u32 n = ny - (ny & 7);
        pLog->warn(0, 0, "ESP_WATER : height (%d -> %d)", ny, n);
        ny = n;
    }
    rate = 1.0f - (f32) (int) rec->xFE / 255.0f;
    p->rotY = rec->xFE;
    PSVECScale(&rec->x58, &r, 6.28f / 360.0f);
    if (SetWaterWork45(w, (Vec*) &rec->x0C, &r, rec->x88, rate, nx, ny) == 0) {
        goto fail;
    }
    p->col.r = rec->x9C;
    p->col.g = rec->x9D;
    p->col.b = rec->x9E;
    p->col.a = rec->x9F;
    p->amb.r = rec->xA0 * 255.0f;
    p->amb.g = rec->xA4 * 255.0f;
    p->amb.b = rec->xA8 * 255.0f;
    p->amb.a = rec->xAC * 255.0f;
    p->xC0 = p->x18;
    p->mode = rec->xC8;
    if (p->mode == 2) {
        p->damp = 0.5f - (f32) (s8) rec->xC9 * 0.005f;
        if (p->damp > 0.5f) {
            p->damp = 0.5f;
        }
        if (p->damp < 0.0f) {
            p->damp = 0.0f;
        }
        p->spread = 0.99f - (f32) (int) rec->xCA * 0.001f;
    }
    p->texId = rec->x2;
    p->indS = rec->prm.h.xCE;
    p->indT = rec->prm.h.xD2;
    p->stages = rec->xCB;
    g_pWater45 = w;
    Espgen45_Move(w);
    return 1;
fail:
    return 0;
}

void Estgen45SetTargetCamera(int on)
{
    g_bTargetCamera = on;
}

void Estgen45SetTargetHeight(int on)
{
    g_bTargetHeight = on;
}

void Estgen45SetSizeOverWrite(int on)
{
    g_bSizeOverWrite = on;
}

void Estgen45SetColorOverWrite(int on)
{
    g_bColorOverWrite = on;
}

void Estgen45SetColorMul(int on)
{
    g_bColorMul = on;
}

void Estgen45SetParamOverWrite(int on)
{
    g_bSetParam = on;
}

void Estgen45SetTargetPos(f32 x, f32 z)
{
    FSet(g_Target_x, x);
    FSet(g_Target_z, z);
    pG->flags_5010 |= 0x20;
}

void Estgen45SetHeight(f32 h)
{
    FSet(g_Target_y, h);
    pG->flags_5010 |= 0x20;
}

void Estgen45SetSize(f32 size)
{
    FSet(g_Size, size);
    pG->flags_5010 |= 0x20;
}

void Estgen45SetColor(u8 r, u8 g, u8 b, u8 a, f32 rs, f32 gs, f32 bs, f32 as)
{
    U8Set(g_r, r);
    U8Set(g_g, g);
    U8Set(g_b, b);
    U8Set(g_a, a);
    FSet(g_sr, rs);
    FSet(g_sg, gs);
    FSet(g_sb, bs);
    FSet(g_sa, as);
    pG->flags_5010 |= 0x20;
}

void Estgen45SetParam(Esp4cWork* w)
{
    g_Free = *w;
    pG->flags_5010 |= 0x20;
}

asm(".section .sdata; .balign 8");
