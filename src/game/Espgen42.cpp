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

// Effect controller 42: room water surface. A (nx+1) x (ny+1) height field simulated on two
// ping-pong buffers, rendered as triangle strips through a display list with an indirect bump
// texture built every frame from the normals. Shared with the weather water (espgen45):
// AddWaterPower / GetWaterHeight / GetWaterCrossPos test both generators.

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
// game/espgen45.cpp
extern EspgenWork* g_pWater45;

void AddWaterPowerSub(EspgenWork* w);
void GetWaterHeightSub(EspgenWork* w);
void GetWaterCrossPosSub(EspgenWork* w);
void Espgen42_Move00(EspgenWork* w);
void Espgen42_TransSub(EspgenWork* w);
void SetIndMtx(Espgen42Work* p);
EspgenWork* SetWaterWork(EspgenWork* w, Vec* pos, Vec* rot, f32 size, u32 nx, u32 ny, f32 rate);
}

static EspgenWork* g_pWater;
static Vec Chk_pos;
static f32 Height_ret;
static f32 Add_power;
static int Height_find;
static Vec Cross_Chk_pos;
static Vec Cross_Chk_dest;
static Vec Cross_Ret_pos;
static int Cross_find;
int g_bNoWater = 0;

static inline void ISet(int& d, int v) { d = v; }
static inline f32 FGet(f32& d) { return d; }
static inline int IGet(int& d) { return d; }

void EspWaterInit()
{
    Estgen45SetTargetCamera(1);
    g_pWater = NULL;
    g_pWater45 = NULL;
    Espgen45_static_init();
    g_bNoWater = 0;
    Estgen45SetTargetCamera(1);
    Estgen45SetTargetHeight(0);
    Estgen45SetSizeOverWrite(0);
    Estgen45SetColorOverWrite(0);
    Estgen45SetColorMul(0);
    Estgen45SetParamOverWrite(0);
}

// Pushes the height field down around Chk_pos (the cell and its four neighbours).
static inline void AddWaterPowerCore(EspgenWork* w)
{
    Espgen42Work* p = (Espgen42Work*) w->work;
    Vec v;
    u32 x;
    u32 z;
    u32 idx;
    int i;

    v = Chk_pos;
    PSMTXMultVec(p->inv, &v, &v);
    if (v.x < (f32) (-p->nx / 2)) {
        return;
    }
    if (v.z < (f32) (-p->ny / 2)) {
        return;
    }
    if (v.x > (f32) (p->nx / 2)) {
        return;
    }
    if (v.z > (f32) (p->ny / 2)) {
        return;
    }
    z = (u32) (v.z + (f32) (p->ny / 2));
    x = (u32) (v.x + (f32) (p->nx / 2));
    idx = z * (p->nx + 1) + x;
    u32 k = 0;
    f32 pw = 1.0f;
    if (x <= 1) {
        return;
    }
    if (z <= 1) {
        return;
    }
    if (x >= (u32) (p->nx - 2)) {
        return;
    }
    if (z >= (u32) (p->ny - 2)) {
        return;
    }
    for (i = 0; i < 5; i++) {
        switch (i) {
        case 0:
            k = idx - 1;
            pw = 0.8f;
            break;
        case 1:
            k = idx + 1;
            pw = 0.8f;
            break;
        case 2:
            k = idx;
            pw = 1.0f;
            break;
        case 3:
            k = idx - p->nx;
            pw = 0.8f;
            break;
        case 4:
            k = idx + p->nx;
            pw = 0.8f;
            break;
        }
        if (k < (u32) (p->nx * p->ny)) {
            f32* h;
            if (pG->flags_51E4 & 1) {
                h = p->hB;
            } else {
                h = p->hA;
            }
            f32* hp = &h[k];
            *hp += FGet(Add_power) * pw;
        }
    }
}

void AddWaterPowerSub(EspgenWork* w)
{
    if (w->id == 0x42) {
        AddWaterPowerCore(w);
    } else if (w->id == 0x45) {
        AddWaterPowerCore(w);
    }
}

void AddWaterPower(Vec* pos, f32 power)
{
    if (pG->flags_500C & 0x200) {
        ISet(Height_find, 0);
        FSet(Add_power, power * 5.0f);
        Chk_pos = *pos;
        EspgenWork* w = g_pWater;
        if (w != NULL && (w->flag & 1) && !(w->flag & 2)) {
            AddWaterPowerSub(w);
        }
        if (g_pWater45 != NULL && (g_pWater45->flag & 1) && !(g_pWater45->flag & 2)) {
            AddWaterPowerSub(g_pWater45);
        }
    }
}

void GetWaterHeightSub(EspgenWork* w)
{
    Vec v;

    if (w->id != 0x42 && w->id != 0x45) {
        return;
    }
    Espgen42Work* p = (Espgen42Work*) w->work;
    v = Chk_pos;
    PSMTXMultVec(p->inv, &v, &v);
    if (v.x < (f32) (-p->nx / 2)) {
        return;
    }
    if (v.z < (f32) (-p->ny / 2)) {
        return;
    }
    if (v.x > (f32) (p->nx / 2)) {
        return;
    }
    if (v.z > (f32) (p->ny / 2)) {
        return;
    }
    v.y = 0.0f;
    PSMTXMultVec(p->mat, &v, &v);
    if (v.y > Height_ret) {
        Height_ret = v.y;
    }
    Height_find = 1;
}

void Espgen42SetNoWater(int on)
{
    g_bNoWater = on;
}

int GetWaterHeight(Vec* pos, f32* height)
{
    if (!(pG->flags_500C & 0x200)) {
        return 0;
    }
    if (g_bNoWater == 1) {
        return 0;
    }
    ISet(Height_find, 0);
    FSet(Height_ret, -100000000.0f);
    Chk_pos = *pos;
    if (g_pWater != NULL) {
        if (!(g_pWater->flag & 1) || (g_pWater->flag & 2)) {
            if (g_pWater45 != NULL) {
                if (!(g_pWater45->flag & 1) || (g_pWater45->flag & 2)) {
                    return 0;
                }
            }
        }
    }
    if (g_pWater != NULL && (g_pWater->flag & 1) && !(g_pWater->flag & 2)) {
        GetWaterHeightSub(g_pWater);
    }
    if (g_pWater45 != NULL && (g_pWater45->flag & 1) && !(g_pWater45->flag & 2)) {
        Espgen42Work* p = (Espgen42Work*) g_pWater45->work;
        if (p->flag & 1) {
            GetWaterHeightSub(g_pWater45);
        } else {
            Vec v = {0.0f, 0.0f, 0.0f};
            PSMTXMultVec(p->mat, &v, &v);
            Height_find = 1;
            Height_ret = v.y;
        }
    }
    *height = Height_ret;
    return Height_find;
}

void GetWaterCrossPosSub(EspgenWork* w)
{
    Espgen42Work* p;
    Vec d;
    Vec hit;
    Vec v;
    Vec v2;
    f32 t;

    if (w->id == 0x42) {
        PSVECSubtract(&Cross_Chk_dest, &Cross_Chk_pos, &d);
        p = (Espgen42Work*) w->work;
        v.z = 0.0f;
        v.y = 0.0f;
        v.x = 0.0f;
        PSMTXMultVec(p->mat, &v, &v);
        t = (v.y - Cross_Chk_pos.y) / (Cross_Chk_dest.y - Cross_Chk_pos.y);
        if (t < 0.0f) {
            return;
        }
        PSVECScale(&d, &d, t);
        PSVECAdd(&Cross_Chk_pos, &d, &hit);
        PSMTXMultVec(p->inv, &hit, &v);
        if (v.x < (f32) (-p->nx / 2)) {
            return;
        }
        if (v.z < (f32) (-p->ny / 2)) {
            return;
        }
        if (v.x > (f32) (p->nx / 2)) {
            return;
        }
        if (v.z > (f32) (p->ny / 2)) {
            return;
        }
        Cross_Ret_pos = hit;
        Cross_find = 1;
    } else if (w->id == 0x45) {
        PSVECSubtract(&Cross_Chk_dest, &Cross_Chk_pos, &d);
        p = (Espgen42Work*) w->work;
        v2.z = 0.0f;
        v2.y = 0.0f;
        v2.x = 0.0f;
        PSMTXMultVec(p->mat, &v2, &v2);
        t = (v2.y - Cross_Chk_pos.y) / (Cross_Chk_dest.y - Cross_Chk_pos.y);
        if (t < 0.0f) {
            return;
        }
        PSVECScale(&d, &d, t);
        PSVECAdd(&Cross_Chk_pos, &d, &hit);
        if (p->flag & 1) {
            PSMTXMultVec(p->inv, &hit, &v2);
            if (v2.x < (f32) (-p->nx / 2)) {
                return;
            }
            if (v2.z < (f32) (-p->ny / 2)) {
                return;
            }
            if (v2.x > (f32) (p->nx / 2)) {
                return;
            }
            if (v2.z > (f32) (p->ny / 2)) {
                return;
            }
        }
        Cross_Ret_pos = hit;
        Cross_find = 1;
    }
}

int GetWaterCrossPos(Vec* pos, Vec* dir, Vec* out)
{
    if (!(pG->flags_500C & 0x200)) {
        return 0;
    }
    if (dir->x == 0.0f && dir->y == 0.0f && dir->z == 0.0f) {
        return 0;
    }
    ISet(Cross_find, 0);
    Cross_Chk_pos = *pos;
    PSVECAdd(pos, dir, &Cross_Chk_dest);
    if (g_pWater != NULL) {
        if (!(g_pWater->flag & 1) || (g_pWater->flag & 2)) {
            if (g_pWater45 != NULL) {
                if (!(g_pWater45->flag & 1) || (g_pWater45->flag & 2)) {
                    return 0;
                }
            }
        }
    }
    if (g_pWater != NULL && (g_pWater->flag & 1) && !(g_pWater->flag & 2)) {
        GetWaterCrossPosSub(g_pWater);
    }
    if (g_pWater45 != NULL && (g_pWater45->flag & 1) && !(g_pWater45->flag & 2)) {
        GetWaterCrossPosSub(g_pWater45);
    }
    *out = Cross_Ret_pos;
    return IGet(Cross_find);
}

// Bump texture (I8, 8x4 tiles) index of grid point (x, y).
#define BUMP_INDEX(x, y, w1) (((y) / 4 * 32) * ((w1) >> 3) + ((x) / 8) * 32 + (((y) & 3) << 3) + ((x) & 7))
// Noise texture (0xFE) index of grid point (x, y).
#define NOISE_INDEX(x, y) ((((y) << 6) & 0xB00) + (((x) << 2) & 0xA0) + (((y) & 3) << 3) + ((x) & 7))

// u8 -> f32 through GQR2 from a stack byte (the compiler only emits psq_l from its own fpmem slot).
#define PSQ_L_U8(p) ({ f32 f_; asm volatile("psq_l %0,0(%1),1,2" : "=f"(f_) : "b"(p) : "memory"); f_; })

void Espgen42_Move00(EspgenWork* w)
{
    static f32 wt_pow = 10.0f;
    Espgen42Work* p = (Espgen42Work*) w->work;
    Vec d0;
    Vec d1;
    Vec v;
    Vec d2;
    Vec d3;
    u8 tmp;
    GXTexObj* tex;
    u8* noise;
    u32 frame;
    int i;
    int j;
    int k;

    d0.x = 1.0f;
    d0.z = 0.0f;
    d1.x = 0.0f;
    d1.z = 1.0f;
    d2.x = -1.0f;
    d2.z = 0.0f;
    d3.x = 0.0f;
    d3.z = -1.0f;
    frame = pG->flags_51E4 % 60;
    BitOn(pG->flags_500C, 0x200);
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
    f32 inx = 1.0f / (f32) (int) nx;
    f32 iny = 1.0f / (f32) (int) ny;
    if (p->mode != 1) {
        if ((pG->flags_64 & 0x00800000) && (Joy[0].on & 0x100)) {
            f32* h = p->hB;
            int idx = (int) ((f32) (int) (nx * ny) * 0.5f);
            h[idx] -= wt_pow;
        }
        f32 damp = p->damp;
        f32 cdamp = 2.0f - damp * 4.0f;
        f32 spread = p->spread;
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
                tmp = noise[NOISE_INDEX(j, i)];
                f32 n = PSQ_L_U8(&tmp) - 80.0f;
                f32* c = &cur[k];
                f32 sum = c[-1] + c[1] + c[-1 - (int) nx] + c[1 + (int) nx];
                f32 h = damp * sum + cdamp * cur[k];
                h -= next[k];
                h = n * 0.0002f + h;
                h *= spread;
                next[k] = h;
                p->pos[k].y = h;
                v.x = p->pos[k - 1].y - p->pos[k + 1].y;
                v.y = 2.0f;
                v.z = p->pos[k - nx].y - p->pos[k + nx].y;
                PSVECNormalize(&v, &p->nrm[k]);
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
                PSVECNormalize(&v, &p->nrm[k]);
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

void Espgen42_Move(EspgenWork* w)
{
    static void (*Espgen42MoveTbl[])(EspgenWork*) = {Espgen42_Move00};

    if (pG->flags_170 & 0x40000) {
        return;
    }
    Espgen42MoveTbl[w->step](w);
}

void Espgen42_Trans(EspgenWork* w)
{
    if ((w->flag & 1) && !(w->flag & 2)) {
        AddOtDirect(0x10, w, (void (*)()) Espgen42_TransSub, 1, 0x80, NULL, 0.0f);
    }
}

void SetIndMtx(Espgen42Work* p)
{
    f32 m[2][3];

    m[0][0] = (f32) (s16) p->indS * 0.001f + 0.01f;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = (f32) (s16) p->indT * 0.007f + 0.07f;
    m[1][2] = 0.0f;
    GXSetIndTexMtx(1, m, 1);
}

void Espgen42_TransSub(EspgenWork* w)
{
    GxStageWork* st;
    Espgen42Work* p;
    void* buf;
    s32 stage;

    if (!(w->flag & 1) || (w->flag & 2)) {
        return;
    }
    st = &pG->gxStage;
    p = (Espgen42Work*) w->work;
    st->tevStage = 0;
    st->texMap = 0;
    st->texCoord = 0;
    CameraCurrentProjection();
    GXSetCullMode(0);
    GXSetZMode(1, 3, 1);
    cModel model;
    u8 modelPad[0x320 - sizeof(cModel)];
    PSMTXIdentity(model.mat);
    {
        static const Vec p0 = {0.0f, 0.0f, 0.0f};
        static const Vec p1 = {10000.0f, 10000.0f, 10000.0f};
        model.lightInfo.init2(1, 0, &p0, &p1, 0x10);
    }
    model.pos.x = p->mat[0][3];
    model.pos.y = p->mat[1][3];
    model.pos.z = p->mat[2][3];
    LightMgr.setClothN(&model, 5);
    commonWaterLightSet(model.lightInfo.pLight, 5, p->amb.a);
    GXColor white;
    white.r = white.g = white.b = white.a = 0xFF;
    GXSetChanMatColor(4, white);
    GXSetChanAmbColor(4, p->amb);
    Mtx nrm;
    Mtx mv;
    Mtx tmp;
    PSMTXConcat(pG->Cam.viewMat, p->mat, mv);
    PSMTXCopy(p->mat, tmp);
    tmp[1][1] = p->size * 0.05f + 100.0f;
    PSMTXConcat(pG->Cam.viewMat, tmp, tmp);
    PSMTXInverse(tmp, nrm);
    PSMTXTranspose(nrm, nrm);
    GXLoadNrmMtxImm(nrm, 0);
    GXLoadPosMtxImm(mv, 0);
    GXSetCurrentMtx(0);
    GXSetBlendMode(1, 4, 5, 0);
    buf = GetDrawTmpBufAddr(0xE);
    if (buf == NULL) {
        pLog->warn(0, 0, "Espgen42() : not enough memory");
        return;
    }
    {
        f32 ofs = 56.0f;
        GXSetTexCopySrc(0, (u32) ofs, (u32) Screen.width, (u32) (Screen.height - ofs));
        GXSetTexCopyDst((u32) Screen.width / 2, (u32) ((f32) ((u32) Screen.height / 2) - ofs), 6, 1);
        GXCopyTex(buf, 0);
        GXPixModeSync();
        GXInvalidateTexAll();
        {
            GXTexObj tex;
            Mtx tm;
            Mtx pm;
            GXInitTexObj(&tex, buf, (u32) Screen.width / 2, (u32) ((f32) ((u32) Screen.height / 2) - ofs), 6, 0, 0, 0);
            GXLoadTexObj(&tex, st->texMap);
            C_MTXLightPerspective(pm, pG->Cam.param.fovy, 1.3333334f, 0.5f, -0.6666667f, 0.5f, 0.5f);
            PSMTXConcat(pm, mv, tm);
            GXLoadTexMtxImm(tm, 0x1E, 0);
            GXSetTexCoordGen(st->texCoord, 0, 0, 0x1E);
            GXSetTevColor(1, p->col);
            GXSetTevOrder(st->tevStage, st->texCoord, st->texMap, 4);
            GXSetTevColorIn(st->tevStage, 0xF, 2, 8, 0xF);
            if (p->stages > 0) {
                GXSetTevColorOp(st->tevStage, 0, 0, 2, 1, 0);
            } else {
                GXSetTevColorOp(st->tevStage, 0, 0, 0, 1, 0);
            }
            GXSetTevAlphaIn(st->tevStage, 7, 7, 7, 5);
            GXSetTevAlphaOp(st->tevStage, 0, 0, 0, 1, 0);
            stage = st->tevStage;
            st->tevStage++;
            st->texMap++;
            st->texCoord++;
            GXInitTexObj(&tex, p->bump, p->nx, p->ny, 1, 0, 0, 0);
            GXLoadTexObj(&tex, st->texMap);
            GXSetNumIndStages(1);
            GXSetTexCoordGen(st->texCoord, 1, 4, 0x3C);
            GXSetIndTexOrder(0, st->texCoord, st->texMap);
            GXSetIndTexCoordScale(0, 0, 0);
            SetIndMtx(p);
            GXSetTevIndWarp(stage, 0, 1, 0, 1);
            st->texCoord++;
            st->texMap++;
            if (p->stages > 1) {
                GXSetTevOrder(st->tevStage, st->texCoord, st->texMap, 4);
                GXSetTevColorIn(st->tevStage, 0xF, 0, 0, 0xF);
                GXSetTevColorOp(st->tevStage, 0, 0, 2, 1, 0);
                GXSetTevAlphaIn(st->tevStage, 7, 7, 7, 5);
                GXSetTevAlphaOp(st->tevStage, 0, 0, 0, 1, 0);
                st->tevStage++;
                st->texMap++;
                st->texCoord++;
            }
            if (p->stages > 2) {
                GXSetTevOrder(st->tevStage, st->texCoord, st->texMap, 4);
                GXSetTevColorIn(st->tevStage, 0xF, 0, 0, 0xF);
                GXSetTevColorOp(st->tevStage, 0, 0, 2, 1, 0);
                GXSetTevAlphaIn(st->tevStage, 7, 7, 7, 5);
                GXSetTevAlphaOp(st->tevStage, 0, 0, 0, 1, 0);
                st->tevStage++;
                st->texMap++;
                st->texCoord++;
            }
        }
        {
            GXTexObj* tex2 = EspGetTexObj(p->texId, 0);
            if (tex2 == NULL) {
                pLog->err(0, 0, "Espgen42 : TexId[%x] invalid.", p->texId);
                tex2 = &Specular;
            }
            GXLoadTexObj(tex2, st->texMap);
        }
        {
            Mtx ms;
            Mtx mt;
            Mtx m3;
            PSMTXCopy(pG->Cam.viewMat, m3);
            PSMTXInverse(m3, m3);
            PSMTXTranspose(m3, m3);
            PSMTXScale(ms, 1.0f, -0.5f, 0.0f);
            PSMTXTrans(mt, 0.5f, 0.5f, 1.0f);
            PSMTXConcat(ms, m3, m3);
            PSMTXConcat(mt, m3, m3);
            GXLoadTexMtxImm(m3, 0x21, 0);
        }
        GXSetTexCoordGen(st->texCoord, 0, 1, 0x21);
        GXSetTevOrder(st->tevStage, st->texCoord, st->texMap, 4);
        GXSetTevColorIn(st->tevStage, 0xF, 0xA, 9, 0);
        GXSetTevColorOp(st->tevStage, 0, 0, 0, 1, 0);
        GXSetTevAlphaIn(st->tevStage, 7, 7, 7, 5);
        GXSetTevAlphaOp(st->tevStage, 0, 0, 0, 1, 0);
        st->tevStage++;
        st->texMap++;
        st->texCoord++;
        GXSetNumTevStages(st->tevStage);
        GXSetNumTexGens(st->texCoord);
        GXClearVtxDesc();
        GXSetVtxDesc(9, 3);
        GXSetVtxDesc(10, 3);
        GXSetVtxDesc(13, 1);
        GXSetVtxAttrFmt(0, 9, 1, 4, 0);
        GXSetVtxAttrFmt(0, 10, 0, 4, 0);
        GXSetVtxAttrFmt(0, 13, 1, 4, 0);
        GXSetArray(9, p->pos, sizeof(Vec));
        GXSetArray(10, p->nrm, sizeof(Vec));
        GXCallDisplayList(p->dl, p->dlSize);
        GXSetNumTevStages(1);
        GXSetNumTexGens(0);
        GXSetNumIndStages(0);
        GXSetTevDirect(0);
        GXSetTevDirect(1);
    }
}

// Dead-stripped from the DOL (string kept): pulls a generator and sets the surface up.
static EspgenWork* SetWater(Vec* pos, Vec* rot, f32 size, u32 nx, u32 ny, f32 rate)
{
    EspgenWork* w;

    if (PullEspgen(&w) == 0) {
        pLog->err(0, 0, "Espgen42 : work pull failed");
        return NULL;
    }
    return SetWaterWork(w, pos, rot, size, nx, ny, rate);
}

EspgenWork* SetWaterWork(EspgenWork* w, Vec* pos, Vec* rot, f32 size, u32 nx, u32 ny, f32 rate)
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

    w->id = 0x42;
    p->damp = 0.05f;
    p->spread = 0.95f;
    p->nx = nx;
    p->ny = ny;
    p->size = size;
    RotMatrix(p->mat, rot);
    PSMTXScale(m, p->size, p->size * 0.05f + 100.0f, p->size);
    PSMTXConcat(p->mat, m, p->mat);
    PSMTXTransApply(p->mat, p->mat, pos->x, pos->y, pos->z);
    PSMTXInverse(p->mat, p->inv);
    if (rate == 0.0f) {
        rate = 0.0001f;
    }
    p->mat[1][1] *= rate;
    n = sizeof(f32) * (p->nx + 1) * (p->ny + 1);
#line 1050 "D:/Bio4/Prog/Espgen42.cpp"
    p->hA = (f32*) MEM_ALLOC(n, 1, 13);
    if (p->hA == NULL) {
        pLog->err(0, 0, "Eg42:not mem");
        PushEspgen(w);
        return NULL;
    }
    memclr_asm(p->hA, n);
#line 1057 "D:/Bio4/Prog/Espgen42.cpp"
    p->hB = (f32*) MEM_ALLOC(n, 1, 13);
    if (p->hB == NULL) {
        pLog->err(0, 0, "Eg42:not mem");
        PushEspgen(w);
        return NULL;
    }
    memclr_asm(p->hB, n);
    n = sizeof(Vec) * (p->nx + 1) * (p->ny + 1);
#line 1066 "D:/Bio4/Prog/Espgen42.cpp"
    p->pos = (Vec*) MEM_ALLOC(n, 1, 13);
    if (p->pos == NULL) {
        pLog->err(0, 0, "Eg42:not mem");
        PushEspgen(w);
        return NULL;
    }
    memclr_asm(p->pos, n);
#line 1073 "D:/Bio4/Prog/Espgen42.cpp"
    p->nrm = (Vec*) MEM_ALLOC(n, 1, 13);
    if (p->nrm == NULL) {
        pLog->err(0, 0, "Eg42:not mem");
        PushEspgen(w);
        return NULL;
    }
    memclr_asm(p->nrm, n);
#line 1082 "D:/Bio4/Prog/Espgen42.cpp"
    p->bump = (u8*) MEM_ALLOC(sizeof(Vec) * p->nx * p->ny, 1, 13);
    if (p->bump == NULL) {
        pLog->err(0, 0, "Eg42:not mem");
        PushEspgen(w);
        return NULL;
    }
    p->dlSize = ((p->nx + 1) * 2 * p->ny * 12 + 0x61) & ~0x1F;
#line 1095 "D:/Bio4/Prog/Espgen42.cpp"
    p->dl = (u8*) MEM_ALLOC(p->dlSize, 1, 13);
    if (p->dl == NULL) {
        pLog->err(0, 0, "Eg42:not mem");
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
    *(u16*) d = (p->nx + 1) * 2 * p->ny;
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
            *(u16*) d = p->nx + (k + 1);
            d += 2;
            *(u16*) d = p->nx + (k + 1);
            d += 2;
            *(f32*) d = (f32) j / (f32) (p->nx + 1);
            d += 4;
            *(f32*) d = (f32) (i + 1) / (f32) (p->ny + 1);
            d += 4;
            k++;
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
                *(u16*) d = p->nx + (k + 1);
                d += 2;
                *(u16*) d = p->nx + (k + 1);
                d += 2;
                *(f32*) d = (f32) j / (f32) (p->nx + 1);
                d += 4;
                *(f32*) d = (f32) (i + 1) / (f32) (p->ny + 1);
                d += 4;
            }
        }
    }
    fy = 0.0f;
    for (int i2 = 0; i2 < p->ny + 1; i2++) {
        int idx = i2 * (p->nx + 1);
        int jj;
        fx = 0.0f;
        for (jj = 0; jj < p->nx + 1; jj++) {
            p->pos[idx].x = fx - (f32) (int) (p->nx / 2);
            p->pos[idx].y = fRand1_1() * 0.2f;
            p->pos[idx].z = fy - (f32) (int) (p->ny / 2);
            p->nrm[idx].x = 0.0f;
            p->nrm[idx].y = 1.0f;
            p->nrm[idx].z = 0.0f;
            p->hA[idx] = 0.0f;
            p->hB[idx] = 0.0f;
            fx += 1.0f;
            idx++;
        }
        fy += 1.0f;
    }
    {
        static f32 g42_init_y = 0.0f;
        static f32 g42_init_y2 = 0.0f;
        int base;
        for (j = 0; j < p->nx + 1; j++) {
            p->pos[j].y = FGet(g42_init_y);
        }
        base = p->ny * (p->nx + 1);
        for (j = 0; j < p->nx + 1; j++) {
            p->pos[base + j].y = FGet(g42_init_y);
        }
        for (int i3 = 0; i3 < p->ny + 1; i3++) {
            p->pos[i3 * (p->nx + 1)].y = FGet(g42_init_y2);
        }
        for (int i4 = 0; i4 < p->ny + 1; i4++) {
            p->pos[i4 * (p->nx + 1) + p->nx].y = FGet(g42_init_y2);
        }
    }
    n = sizeof(Vec) * (p->nx + 1) * (p->ny + 1);
    DCStoreRange(p->pos, n);
    DCStoreRange(p->nrm, n);
    DCStoreRange(p->bump, sizeof(Vec) * (p->nx + 1) * (p->ny + 1));
    n = sizeof(f32) * (p->nx + 1) * (p->ny + 1);
    DCStoreRange(p->hA, n);
    DCStoreRange(p->hB, n);
    DCStoreRange(p->dl, p->dlSize);
    return w;
}

void Espgen42_Destruct(EspgenWork* w)
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
    g_pWater = NULL;
}

int Espgen42_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8)
{
    Espgen42Work* p = (Espgen42Work*) w->work;
    Vec r;
    u32 nx = 0x40;
    u32 ny = 0x40;
    f32 rate;

    if (EspGetTexObj(0xFE, 0) == NULL) {
        pLog->err(0, 0, "Espgen42 : WaterTex(0xfe) not found!");
        return 0;
    }
    if (rec->xFC != 0) {
        nx = rec->xFC;
        if (nx > 0xB8) {
            nx = 0xB8;
            pLog->warn(0, 0, "ESP_WATER : width > 184");
        }
    }
    if (rec->xFD != 0) {
        ny = rec->xFD;
        if (ny > 0xB8) {
            ny = 0xB8;
            pLog->warn(0, 0, "ESP_WATER : height > 184");
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
    PSVECScale(&rec->x58, &r, 6.28f / 360.0f);
    if (SetWaterWork(w, (Vec*) &rec->x0C, &r, rec->x88, nx, ny, rate) != NULL) {
        p->col.r = rec->x9C;
        p->col.g = rec->x9D;
        p->col.b = rec->x9E;
        p->col.a = rec->x9F;
        p->amb.r = rec->xA0 * 255.0f;
        p->amb.g = rec->xA4 * 255.0f;
        p->amb.b = rec->xA8 * 255.0f;
        p->amb.a = rec->xAC * 255.0f;
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
        g_pWater = w;
        Espgen42_Move(w);
        return 1;
    }
    return 0;
}

asm(".section .sdata; .balign 8");
