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
// TODO: Espgen45_Move00 and Espgen45_TransSub are not written yet.

// Parameter block handed over by esp4c (Esp4cWork, 0x20 bytes; the layout is esp4c.cpp's).
struct Esp4cWork {
    u8 type;      // 0x00
    u8 x1;        // 0x01
    u8 x2;        // 0x02
    u8 x3;        // 0x03
    s16 indS;     // 0x04 indirect matrix parameters (SetIndMtx)
    s16 indT;     // 0x06
    f32 damp;     // 0x08 (Espgen42Work::damp)
    f32 spread;   // 0x0C (Espgen42Work::spread)
    Vec rot;      // 0x10 surface rotation (SetWaterWork45)
    u8 flag;      // 0x1C
    u8 x1D;       // 0x1D
    u8 x1E;       // 0x1E
    u8 x1F;       // 0x1F
};

extern "C" {
// game/trans_lit.cpp
void commonWaterLightSet(cLight** list, int n, u32 alpha);
// Dolphin SDK performance monitor registers (base/PPCArch.h)
void PPCMtpmc1(u32 v);
void PPCMtpmc2(u32 v);
void PPCMtpmc3(u32 v);
void PPCMtpmc4(u32 v);
void PPCMtmmcr0(u32 v);
void PPCMtmmcr1(u32 v);
void Espgen45_Move00(EspgenWork* w);
void Espgen45_TransSub(EspgenWork* w);
void SetIndMtx_801291F4(Espgen42Work* p);   // the DOL's local SetIndMtx (Espgen42 owns the global one); sym_map name
EspgenWork* SetWaterWork45(EspgenWork* w, Vec* pos, Vec* rot, f32 size, u32 nx, u32 ny, f32 rate);
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
static f32 inv_mul = 1.0f;
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

// Bump texture (I8, 8x4 tiles) index of grid point (x, y).
#define BUMP_INDEX(x, y, w1) (((y) / 4 * 32) * ((w1) >> 3) + ((x) / 8) * 32 + (((y) & 3) << 3) + ((x) & 7))
// Noise texture (0xFE) index of grid point (x, y).
#define NOISE_INDEX(x, y) ((((y) << 6) & 0xB00) + (((x) << 2) & 0xA0) + (((y) & 3) << 3) + ((x) & 7))

// Unused neighbour weights; the reference parameters leave the literals in the frame.
static inline void WaveDir(const f32& a, const f32& b, const f32& c, const f32& d)
{
}

void Espgen45_Move00(EspgenWork* w)
{
    static f32 g45_wave_mul = 0.001f;
    static f32 wt_pow = 10.0f;
    Espgen42Work* p = (Espgen42Work*) w->work;
    WaveDir(1.0f, 0.0f, 0.0f, 1.0f);
    Vec v;
    WaveDir(-1.0f, 0.0f, 0.0f, -1.0f);
    Mtx m;
    GXTexObj* tex;
    u8* noise;
    u32 frame;
    f32 size;
    f32 rate;
    u8 rotY;
    u8 mode;
    int i;
    int j;
    int k;

    pG->flags_500C |= 0x200;
    frame = pG->flags_51E4 % 60;
    if (g_bTargetCamera == 1) {
        g_Target_x = pG->Cam.param.pos.x;
        g_Target_z = pG->Cam.param.pos.z;
    }
    p->pos0.x = g_Target_x;
    p->pos0.z = g_Target_z;
    if (g_bTargetHeight == 1) {
        p->pos0.y = g_Target_y;
    } else {
        p->pos0.y = p->xC0;
    }
    size = p->size;
    if (g_bSizeOverWrite == 1) {
        size = g_Size;
    }
    if (g_bSetParam == 0) {
        PSMTXScale(p->mat, size, size * 0.05f + 100.0f, size);
    } else {
        RotMatrix(p->mat, &g_Free.rot);
        PSMTXScale(m, size, size * 0.05f + 100.0f, size);
        PSMTXConcat(p->mat, m, p->mat);
    }
    if (g_bSetParam == 0) {
        rotY = p->rotY;
    } else {
        rotY = g_Free.x3;
    }
    rate = 1.0f - (f32) (int) rotY / 255.0f;
    if (rate == 0.0f) {
        rate = 0.0001f;
    }
    p->mat[1][1] *= rate;
    PSMTXTransApply(p->mat, p->mat, p->pos0.x, p->pos0.y, p->pos0.z);
    PSMTXInverse(p->mat, p->inv);
    PPCMtpmc1(0);
    PPCMtpmc2(0);
    PPCMtpmc3(0);
    PPCMtpmc4(0);
    PPCMtmmcr1(0x78000000);
    PPCMtmmcr0(0x42);
    tex = EspGetTexObj(0xFE, frame);
    if (tex == NULL) {
        return;
    }
    noise = (u8*) GXGetTexObjData(tex) + 0x80000000;
    u32 nx = p->nx;
    u32 ny = p->ny;
    f32 hx = (f32) (int) (nx / 2);
    f32 hy = (f32) (int) (ny / 2);
    f32 inx = 1.0f / (f32) (int) nx * inv_mul;
    f32 iny = 1.0f / (f32) (int) ny * inv_mul;
    if (g_bSetParam == 0) {
        mode = p->mode;
    } else {
        mode = g_Free.type;
    }
    if (mode != 1) {
        if ((pG->flags_64 & 0x00800000) && (Joy[0].on & 0x100)) {
            f32* h = p->hB;
            int idx = (int) ((f32) (int) (nx * ny) * 0.5f);
            h[idx] -= wt_pow;
        }
        f32 damp;
        f32 spread;
        if (g_bSetParam == 0) {
            damp = p->damp;
        } else {
            damp = g_Free.damp;
        }
        f32 cdamp = 2.0f - damp * 4.0f;
        if (g_bSetParam == 0) {
            spread = p->spread;
        } else {
            spread = g_Free.spread;
        }
        f32* cur;
        f32* next;
        if (pG->flags_51E4 & 1) {
            cur = p->hA;
            next = p->hB;
        } else {
            cur = p->hB;
            next = p->hA;
        }
        f32 fy = 1.0f;
        for (i = 1; i < p->ny; i++) {
            u32 w1 = nx + 1;
            f32 fx = 1.0f;
            k = i * w1 + 1;
            for (j = 1; j < nx; j++) {
                f32 n = (f32) noise[NOISE_INDEX(j, i)] - 80.0f;
                f32 sum = cur[k - 1] + cur[k + 1] + cur[k - (nx + 1)] + cur[k + (nx + 1)];
                next[k] = damp * sum + cdamp * cur[k] - next[k];
                next[k] = (n * g45_wave_mul + next[k]) * spread;
                p->pos[k].y = next[k];
                v.x = p->pos[k - 1].y - p->pos[k + 1].y;
                v.y = 2.0f;
                v.z = p->pos[k - nx].y - p->pos[k + nx].y;
                PSVECScale(&v, &p->nrm[k], 0.4347826f);
                p->bump[BUMP_INDEX(j, i, w1)] = (u8) (p->nrm[k].x * 255.0f * 2.0f + 128.0f);
                p->nrm[k].x += (fx - hx) * inx;
                p->nrm[k].y *= 0.25f;
                p->nrm[k].z += (fy - hy) * iny;
                fx += 1.0f;
                k++;
            }
            fy += 1.0f;
        }
    } else {
        for (i = 1; i < p->ny; i++) {
            k = i * (p->nx + 1);
            for (j = 1; j < p->nx; j++) {
                int nz = noise[NOISE_INDEX(j, i)];
                f32* hA = p->hA;
                f32* hB = p->hB;
                f32 sum = hA[k - 1] + hA[k + 1] + hA[k - 1 - p->ny] + hA[k + 1 + p->ny];
                hB[k] += sum - hA[k] * 4.0f;
                f32 n = (f32) nz - 80.0f;
                hA[k] += n * 0.0001f + hB[k] * 0.04f;
                hB[k] *= 0.92f;
                p->pos[k].y = n * 0.0018f + hA[k];
                v.x = p->pos[k - 1].y - p->pos[k + 1].y;
                v.y = 2.0f;
                v.z = p->pos[k - p->nx].y - p->pos[k + p->nx].y;
                PSVECScale(&v, &p->nrm[k], 0.4347826f);
                p->bump[BUMP_INDEX(j, i, p->nx + 1)] = (u8) (p->nrm[k].x * 255.0f * 2.0f + 128.0f);
                p->nrm[k].x += ((f32) j - (f32) (p->nx / 2)) * (1.0f / (f32) p->nx);
                p->nrm[k].y *= 0.25f;
                p->nrm[k].z += ((f32) i - (f32) (p->ny / 2)) * (1.0f / (f32) p->ny);
                k++;
            }
        }
    }
    {
        u32 n = sizeof(Vec) * (p->ny + 1) * (p->nx + 1);
        DCStoreRange(p->pos, n);
        DCStoreRange(p->nrm, n);
        DCStoreRange(p->bump, sizeof(Vec) * (p->ny + 1) * (p->nx + 1));
    }
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

void SetIndMtx_801291F4(Espgen42Work* p)
{
    f32 m[2][3];
    s16 indS;
    s16 indT;

    if (g_bSetParam == 0) {
        indS = p->indS;
    } else {
        indS = g_Free.indS;
    }
    if (g_bSetParam == 0) {
        indT = p->indT;
    } else {
        indT = g_Free.indT;
    }
    m[0][0] = (f32) indS * 0.001f + 0.01f;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = (f32) indT * 0.007f + 0.07f;
    m[1][2] = 0.0f;
    GXSetIndTexMtx(1, m, 1);
}

// Dead-stripped from the DOL (string kept): pulls a generator and sets the surface up.
static EspgenWork* SetWater(Vec* pos, Vec* rot, f32 size, u32 nx, u32 ny)
{
    EspgenWork* w;

    if (PullEspgen(&w) == 0) {
        pLog->err(0, 0, "Espgen45 : work pull failed");
        return NULL;
    }
    return SetWaterWork45(w, pos, rot, size, nx, ny, 1.0f);
}

EspgenWork* SetWaterWork45(EspgenWork* w, Vec* pos, Vec* rot, f32 size, u32 nx, u32 ny, f32 rate)
{
    Espgen42Work* p = (Espgen42Work*) w->work;
    Mtx m;
    u32 n;
    u8* d;
    int i;
    int j;
    int k;
    f32 fx;
    f32 fy;

    w->id = 0x45;
    p->nx = nx;
    p->ny = ny;
    p->size = size;
    p->spread = 0.95f;
    p->damp = 0.05f;
    p->pos0 = *pos;
    if (g_bSetParam == 0) {
        PSMTXScale(p->mat, p->size, p->size * 0.05f + 100.0f, p->size);
    } else {
        RotMatrix(p->mat, &g_Free.rot);
        PSMTXScale(m, p->size, p->size * 0.05f + 100.0f, p->size);
        PSMTXConcat(p->mat, m, p->mat);
    }
    PSMTXTransApply(p->mat, p->mat, p->pos0.x, p->pos0.y, p->pos0.z);
    PSMTXInverse(p->mat, p->inv);
    if (rate == 0.0f) {
        rate = 0.0002f;
    }
    p->mat[1][1] *= rate;
    n = sizeof(f32) * (p->ny + 1) * (p->nx + 1);
#line 1452 "D:/Bio4/Prog/espgen45.cpp"
    p->hA = (f32*) MEM_ALLOC(n, 1, 13);
    if (p->hA == NULL) {
        goto nomem;
    }
    memclr_asm(p->hA, n);
#line 1459 "D:/Bio4/Prog/espgen45.cpp"
    p->hB = (f32*) MEM_ALLOC(n, 1, 13);
    if (p->hB == NULL) {
        goto nomem;
    }
    memclr_asm(p->hB, n);
    n = sizeof(Vec) * (p->ny + 1) * (p->nx + 1);
#line 1468 "D:/Bio4/Prog/espgen45.cpp"
    p->pos = (Vec*) MEM_ALLOC(n, 1, 13);
    if (p->pos == NULL) {
        goto nomem;
    }
    memclr_asm(p->pos, n);
#line 1475 "D:/Bio4/Prog/espgen45.cpp"
    p->nrm = (Vec*) MEM_ALLOC(n, 1, 13);
    if (p->nrm == NULL) {
        goto nomem;
    }
    memclr_asm(p->nrm, n);
#line 1484 "D:/Bio4/Prog/espgen45.cpp"
    p->bump = (u8*) MEM_ALLOC(sizeof(Vec) * p->ny * p->nx, 1, 13);
    if (p->bump == NULL) {
        goto nomem;
    }
    p->dlSize = ((p->nx + 1) * (p->ny * 2) * 12 + 0x61) & ~0x1F;
#line 1497 "D:/Bio4/Prog/espgen45.cpp"
    p->dl = (u8*) MEM_ALLOC(p->dlSize, 1, 13);
    if (p->dl == NULL) {
    nomem:
        pLog->err(0, 0, "Espgen45 : not enough memory");
        PushEspgen(w);
        return NULL;
    }
    memclr_asm(p->dl, p->dlSize);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 3);
    GXSetVtxDesc(10, 3);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 10, 0, 4, 0);
    GXSetVtxAttrFmt(0, 13, 1, 4, 0);
    d = p->dl;
    *d = 0;
    d++;
    *d = 0x98;
    d++;
    *(u16*) d = (p->nx + 1) * (p->ny * 2);
    d += 2;
    for (i = 0; i < p->ny; i++) {
        k = i * (p->nx + 1);
        for (j = 0; j < p->nx + 1; j++) {
            *(u16*) d = k;
            d += 2;
            *(u16*) d = k;
            d += 2;
            *(f32*) d = (f32) j / (f32) (p->nx + 1);
            d += 4;
            *(f32*) d = (f32) i / (f32) (p->ny + 1);
            d += 4;
            k++;
            *(u16*) d = p->nx + k;
            d += 2;
            *(u16*) d = p->nx + k;
            d += 2;
            *(f32*) d = (f32) j / (f32) (p->nx + 1);
            d += 4;
            *(f32*) d = (f32) (i + 1) / (f32) (p->ny + 1);
            d += 4;
        }
        i++;
        if (i < p->ny) {
            for (j = p->nx; j >= 0; j--) {
                k = i * (p->nx + 1) + j;
                *(u16*) d = k;
                d += 2;
                *(u16*) d = k;
                d += 2;
                *(f32*) d = (f32) j / (f32) (p->nx + 1);
                d += 4;
                *(f32*) d = (f32) i / (f32) (p->ny + 1);
                d += 4;
                k++;
                *(u16*) d = p->nx + k;
                d += 2;
                *(u16*) d = p->nx + k;
                d += 2;
                *(f32*) d = (f32) j / (f32) (p->nx + 1);
                d += 4;
                *(f32*) d = (f32) (i + 1) / (f32) (p->ny + 1);
                d += 4;
            }
        }
    }
    fy = 0.0f;
    for (i = 0; i < p->ny + 1; i++) {
        int idx = i * (p->nx + 1);
        fx = 0.0f;
        for (j = 0; j < p->nx + 1; j++) {
            p->pos[idx].x = fx - (f32) (p->nx / 2);
            fx += 1.0f;
            p->pos[idx].y = fRand1_1() * 0.2f;
            p->pos[idx].z = fy - (f32) (p->ny / 2);
            p->nrm[idx].x = 0.0f;
            p->nrm[idx].y = 1.0f;
            p->nrm[idx].z = 0.0f;
            p->hA[idx] = 0.0f;
            p->hB[idx] = 0.0f;
            p->nrm[idx].x += ((f32) j - (f32) (p->nx / 2)) * (1.0f / (f32) p->nx);
            p->nrm[idx].y *= 0.25f;
            p->nrm[idx].z += ((f32) i - (f32) (p->ny / 2)) * (1.0f / (f32) p->ny);
            idx++;
        }
        fy += 1.0f;
    }
    {
        static f32 g45_init_y = 0.0f;
        static f32 g45_init_y2 = 0.0f;
        for (j = 0; j < p->nx + 1; j++) {
            p->pos[j].y = g45_init_y;
        }
        for (j = 0; j < p->nx + 1; j++) {
            p->pos[p->ny * (p->nx + 1) + j].y = g45_init_y;
        }
        for (i = 0; i < p->ny + 1; i++) {
            p->pos[i * (p->nx + 1)].y = g45_init_y2;
        }
        for (i = 0; i < p->ny + 1; i++) {
            p->pos[i * (p->nx + 1) + p->nx].y = g45_init_y2;
        }
    }
    n = sizeof(Vec) * (p->ny + 1) * (p->nx + 1);
    DCStoreRange(p->pos, n);
    DCStoreRange(p->nrm, n);
    DCStoreRange(p->bump, sizeof(Vec) * (p->ny + 1) * (p->nx + 1));
    n = sizeof(f32) * (p->ny + 1) * (p->nx + 1);
    DCStoreRange(p->hA, n);
    DCStoreRange(p->hB, n);
    DCStoreRange(p->dl, p->dlSize);
    return w;
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
    if (SetWaterWork45(w, (Vec*) &rec->x0C, &r, rec->x88, nx, ny, rate) == 0) {
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
    p->mode = rec->xC8;
    p->xC0 = p->x18;
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
