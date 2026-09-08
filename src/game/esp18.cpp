#include "light.h"
#include "atari.h"
#include "gx.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"
#include "main_sub.h"
#include "tpl.h"
#include "espgen.h"

extern f32 ZNEAR;
extern f32 ZFAR;

struct Esp18Work {
    Vec pos0;    // 0x00 initial position
    f32 depth;   // 0x0C -gen->xD8
};

// Heat shimmer: copies the frame buffer and redraws it through an indirect texture in
// esp18_lp layers.
class cEsp18 : public cEsp {
public:
    Esp18Work work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

extern "C" {
cEsp* Esp18_Create();
void Esp18_Trans(cEsp18* esp);
}
int GetDrawTmpBufType();       // game/TmpBuf.cpp (C++ linkage)
extern GXTexObj g_Get_tex_obj;  // game/trans.cpp

cEsp* Esp18_Create()
{
    return new cEsp18;
}

void cEsp18::move()
{
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        }
    }
}

int cEsp18::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp18Work* w = &work;

    w->pos0 = pos;
    w->depth = -gen->xD8;
    return 1;
}

// Heat shimmer: the frame is copied into a texture and drawn back esp18_lp times through an
// indirect texture, each layer scaled and offset a little more than the previous one.
void Esp18_Trans(cEsp18* esp)
{
    Esp18Work* w = &esp->work;
    Mtx44 proj;
    Mtx inv;
    EspAnmData* anm;
    GXColor fog;
    GXColor col;
    f32 sx;
    f32 sy;
    f32 ox;
    f32 oy;
    f32 x0;
    f32 y0;
    f32 z;
    f32 s0;
    f32 s1;
    f32 t0;
    f32 t1;
    f32 ang;
    f32 ofs = 0.0f;
    f32 rx;
    f32 ry;
    f32 mul;
    f32 a;
    u32 i;
    int copyOk;
    int stages;
    int texGens;
    void* buf;

    if (!esp->ChannelSetI()) {
        return;
    }
    if (!EspGetAnmAddr(esp->anmNo, &anm)) {
        pLog->err(0, 0, "ESP : TexId[%x] no data", esp->anmNo);
        return;
    }
    GXSetCullMode(0);
    GXSetAlphaCompare(4, 1, 1, 4, 1);
    GXSetZMode(1, 3, 0);
    CameraCurrentProjection();
    if ((s8) esp->partsNo >= -8 && (s8) esp->partsNo <= -3) {
        PSMTXIdentity(esp->mat);
        low_RotMatrix(esp->mat, &esp->rot);
        TransMatrix(esp->mat, &esp->pos);
        C_MTXOrtho(proj, 0.0f, 448.0f, 0.0f, 512.0f, 0.0f, -100.0f);
        GXSetProjection(proj, 1);
    } else if (!(esp->flags & 1)) {
        Vec p;
        Mtx m;

        PSMTXIdentity(esp->mat);
        PSMTXRotRad(esp->mat, 'z', esp->rot.z);
        PSMTXConcat(pG->Cam.viewMat, esp->parent->mat, m);
        PSMTXMultVec(m, &esp->pos, &p);
        esp->mat[0][3] = p.x;
        esp->mat[1][3] = p.y;
        esp->mat[2][3] = p.z;
    } else {
        Mtx m;

        PSMTXIdentity(esp->mat);
        low_RotMatrix(esp->mat, &esp->rot);
        TransMatrix(esp->mat, &esp->pos);
        PSMTXConcat(pG->Cam.viewMat, esp->parent->mat, m);
        PSMTXConcat(m, esp->mat, esp->mat);
    }
    PSMTXInverse(esp->mat, inv);
    PSMTXTranspose(inv, inv);
    GXLoadNrmMtxImm(inv, 0);
    GXLoadPosMtxImm(esp->mat, 0);
    GXSetCurrentMtx(0);
    EspTexSet(esp->anmNo, esp->anmPtn);
    GXSetAlphaCompare(4, 1, 1, 4, 1);
    GXSetBlendMode(esp->xA4, esp->xA5, esp->xA6, esp->xA7);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xA, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xA, 0, 1, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    sx = esp->sizeX * esp->scale;
    sy = esp->sizeY * esp->scale;
    ox = -anm->x4;
    oy = (f32) anm->x6;
    z = 1.0f;
    if (ox == 0.0f) {
        ox = -anm->x0 * 0.5f;
    }
    if (oy == 0.0f) {
        oy = anm->x2 * 0.5f;
    }
    x0 = ox * sx / anm->x0;
    y0 = oy * sy / anm->x2;
    if (esp->flags & 2) {
        if ((s8) esp->partsNo >= -8 && (s8) esp->partsNo <= -3) {
            if (!(esp->flags & 4)) {
                s1 = 0.0f;
                s0 = s1 + z;
                t0 = s0;
                t1 = s1;
            } else {
                s1 = 0.0f;
                s0 = s1 + z;
                t0 = s1;
                t1 = s0;
            }
        } else {
            if (esp->flags & 4) {
                s1 = 0.0f;
                s0 = s1 + z;
                t0 = s0;
                t1 = s1;
            } else {
                s1 = 0.0f;
                s0 = s1 + z;
                t0 = s1;
                t1 = s0;
            }
        }
    } else {
        if ((s8) esp->partsNo >= -8 && (s8) esp->partsNo <= -3) {
            if (!(esp->flags & 4)) {
                s0 = 0.0f;
                s1 = s0 + z;
                t1 = s0;
                t0 = s1;
            } else {
                s0 = 0.0f;
                s1 = s0 + z;
                t0 = s0;
                t1 = s1;
            }
        } else {
            if (esp->flags & 4) {
                s0 = 0.0f;
                s1 = s0 + z;
                t1 = s0;
                t0 = s1;
            } else {
                s0 = 0.0f;
                s1 = s0 + z;
                t0 = s0;
                t1 = s1;
            }
        }
    }
    static u32 esp18_lp = 8;
    static f32 esp18_div = 1.0f / (f32) esp18_lp;
    static f32 prm2 = 5.0f;
    static f32 prm3 = 5.0f;
    static f32 esp18_mul_rate = 0.1f;
    static f32 e18mx = 1.0f;
    static f32 e18my = 1.0f;
    ang = 0.0f;
    for (i = 0; i < esp18_lp; i++) {
        GXTexObj tex;
        GXTlutObj tlut;
        int type = 1;

        copyOk = 1;
        fog.r = fog.g = fog.b = fog.a = 0;
        GXSetFog(0, 0.0f, 0.0f, ZNEAR, ZFAR, fog);
        if (esp->flags & 0x1000) {
            if (GetDrawTmpBufType() != 2) {
                copyOk = 0;
            }
            type = 2;
        }
        buf = GetDrawTmpBufAddr(type);
        ofs = 56.0f;
        if (pG->flags_5010 & 0x08000000) {
            ofs = 0.0f;
        }
        if (copyOk && i == 0) {
            if (buf == NULL) {
                pLog->warn(0, 0, "Esp0d() : not enough memory");
                return;
            }
            GXSetTexCopySrc(0, (u32) ofs, (u32) Screen.width, (u32) (Screen.height - ofs));
            GXSetTexCopyDst((u32) Screen.width / 2, (u32) ((f32) ((u32) Screen.height / 2) - ofs), 6, 1);
            GXCopyTex(buf, 0);
            GXPixModeSync();
            GXInvalidateTexAll();
        }
        GXInitTexObj(&tex, buf, (u32) Screen.width / 2, (u32) ((f32) ((u32) Screen.height / 2) - ofs), 6, 0, 0, 0);
        GXLoadTexObj(&tex, 1);
        g_Get_tex_obj = tex;
        Mtx tm;
        Mtx pm;
        rx = sinf(ang) * prm2 * esp->colA * 0.01f;
        ry = cosf(ang) * prm3 * esp->colA * 0.01f;
        mul = esp18_mul_rate * 0.5f * esp18_div * (f32) i * w->depth * esp->colA * (1.0f / 255.0f) + 1.0f;
        {
            Mtx m1 = {
                {1.0f, 0.0f, -0.5f, 0.0f},
                {0.0f, 1.0f, -0.5f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
            };
            Mtx m2 = {{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
            Mtx m3 = {
                {1.0f, 0.0f, 0.5f, 0.0f},
                {0.0f, 1.0f, 0.5f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
            };

            m2[0][0] = mul;
            m2[1][1] = mul;
            m2[2][2] = 1.0f;
            if ((s8) esp->partsNo >= -8 && (s8) esp->partsNo <= -3) {
                Mtx m4 = {{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
                Mtx m5 = {
                    {0.001953125f, 0.0f, 0.0f, 0.0f},
                    {0.0f, 0.0029762f, -0.167f, 0.0f},
                    {0.0f, 0.0f, 1.0f, 0.0f},
                };

                m4[0][0] = 1.0f / 512.0f;
                m4[1][1] = 1.0f / 448.0f;
                m4[2][2] = 1.0f;
                if (pG->flags_5010 & 0x08000000) {
                    PSMTXConcat(m4, esp->mat, tm);
                } else {
                    PSMTXConcat(m5, esp->mat, tm);
                }
                if (w->depth != 0.0f) {
                    PSMTXConcat(m1, tm, tm);
                    PSMTXConcat(m2, tm, tm);
                    PSMTXConcat(m3, tm, tm);
                }
                GXLoadTexMtxImm(tm, 0x1E, 1);
                GXSetTexCoordGen(0, 1, 0, 0x1E);
            } else {
                C_MTXLightPerspective(pm, pG->Cam.param.fovy, 1.3333334f, 0.5f, -0.6666667f, rx * (1.0f / 512.0f) * e18mx + 0.5f,
                                      ry / 392.0f * e18my + 0.5f);
                PSMTXConcat(pm, esp->mat, tm);
                GXLoadTexMtxImm(tm, 0x1E, 0);
                GXSetTexCoordGen(0, 0, 0, 0x1E);
            }
        }
        GXSetNumIndStages(1);
        texGens = 2;
        GXSetTexCoordGen(1, 1, 4, 0x3C);
        GXSetIndTexOrder(0, 1, 0);
        GXSetIndTexCoordScale(0, 0, 0);
        a = esp->colA;
        if (a > 16.0f) {
            a = 255.0f;
        } else {
            a -= 4.0f;
            if (a < 0.0f) {
                a = 0.0f;
            } else {
                a *= 21.25f;
            }
        }
        col.r = (u8) esp->colR;
        col.g = (u8) esp->colG;
        col.b = (u8) esp->colB;
        col.a = (u8) (a * (1.0f / (f32) (i + 2)));
        GXSetChanMatColor(4, col);
        GXSetTevOrder(0, 0, 1, 4);
        GXSetTevColorIn(0, 0xF, 8, 0xA, 0xF);
        GXSetTevColorOp(0, 0, 0, 0, 1, 0);
        GXSetTevAlphaIn(0, 7, 7, 7, 5);
        GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
        stages = 1;
        {
            EspTexWk* tw = EspGetTexWk(esp->anmNo, 1);
            if (tw->owner == 0xD2) {
                pLog->err(0, 0, "ESP : TexId[%x] no data", esp->anmNo);
            } else {
                GXTexObj tex2;
                TEXDescriptor* td = TEXGet(tw->pTpl, esp->anmPtn);
                TEXHeader* th = td->textureHeader;

                if (th->format == 8 || th->format == 9) {
                    GXInitTexObjCI(&tex2, th->data, th->width, th->height, th->format, 0, 0, 0, 1);
                    GXInitTlutObj(&tlut, td->CLUTHeader->data, td->CLUTHeader->format, td->CLUTHeader->numEntries);
                    GXLoadTlut(&tlut, 1);
                } else {
                    GXInitTexObj(&tex2, th->data, th->width, th->height, th->format, 0, 0, 0);
                }
                GXLoadTexObj(&tex2, 2);
                GXLoadTexMtxImm(tw->mtx, 0x21, 1);
                GXSetTexCoordGen(texGens, 1, 4, 0x21);
                GXSetTevOrder(1, texGens, 2, 4);
                GXSetTevColorIn(1, 0xF, 0xF, 0xF, 0);
                GXSetTevColorOp(1, 0, 0, 0, 1, 0);
                GXSetTevAlphaIn(1, 7, 4, 5, 7);
                if (esp->flags & 0x20000) {
                    GXSetTevAlphaOp(1, 0, 0, 2, 1, 0);
                } else {
                    GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
                }
                stages = 2;
                texGens++;
            }
        }
        GXSetNumTevStages(stages);
        GXSetNumTexGens(texGens);
        GXBegin(0x80, 0, 4);
        GXPosition3f32(x0 + rx, y0 + ry, z);
        GXNormal3s8(0, 1, 0);
        GXTexCoord2f32(s0, t0);
        GXPosition3f32(x0 + sx + rx, y0 + ry, z);
        GXNormal3s8(0, 1, 0);
        GXTexCoord2f32(s1, t0);
        GXPosition3f32(x0 + sx + rx, y0 - sy + ry, z);
        GXNormal3s8(0, 1, 0);
        GXTexCoord2f32(s1, t1);
        GXPosition3f32(x0 + rx, y0 - sy + ry, z);
        GXNormal3s8(0, 1, 0);
        GXTexCoord2f32(s0, t1);
        GXSetNumTevStages(1);
        GXSetNumTexGens(0);
        GXSetNumIndStages(0);
        GXSetTevDirect(0);
        GXSetTevDirect(1);
        LightMgr.setFog();
        ang += 6.2831855f / (f32) esp18_lp;
    }
}
