#include "atari.h"
#include "light.h"
#include "gx.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"

struct Esp12Work {
    u32 n;         // 0x00 number of trail points
    cEsp3f* buf;   // 0x04 vector buffer (esp3f)
};

// Ribbon trail: keeps the last n positions in an esp3f buffer and draws a textured strip
// through them.
class cEsp12 : public cEsp {
public:
    Esp12Work work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
    virtual void Destruct();
};

cEsp* Esp12_Create()
{
    return new cEsp12;
}

void cEsp12::move()
{
    Esp12Work* w = &work;
    Vec wpos;
    Vec* dst;
    Vec* src;
    u32 i;

    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        } else {
            FSet(xB8, 100000000.0f);
            if (parent == pEffParentWorld) {
                wpos = pos;
            } else {
                PSMTXMultVec(parent->mat, &pos, &wpos);
            }
            for (i = w->n - 1; i != 0; i--) {
                dst = Esp3f_GetVecPtr(w->buf, i);
                src = Esp3f_GetVecPtr(w->buf, i - 1);
                *dst = *src;
            }
            *Esp3f_GetVecPtr(w->buf, 0) = wpos;
        }
    }
}

extern "C" void Esp12_Trans(cEsp12* esp)
{
    Esp12Work* w = &esp->work;
    EspAnmData* anm;
    Mtx inv;
    Vec camPos;
    Vec v0;
    Vec v1;
    Vec dir;
    Vec toCam;
    Vec nrm;
    Vec cross;
    Vec* p;
    Vec* next;
    u32 n;
    u32 i;
    f32 t;
    f32 tstep;
    f32 r;
    f32 wid;

    if (esp->cnt < w->n) {
        n = esp->cnt + 1;
    } else {
        n = w->n;
    }
    if (!EspGetAnmAddr(esp->anmNo, &anm)) {
        pLog->err(0, 0, "ESP : TexId[%x] no data", esp->anmNo);
        return;
    }
    CameraCurrentProjection();
    EspTexSet(esp->anmNo, esp->anmPtn);
    esp->ChannelSet();
    GXSetBlendMode(esp->xA4, esp->xA5, esp->xA6, esp->xA7);
    esp->CommonStateSet();
    t = 0.0f;
    tstep = 1.0f / (f32)(int)n;
    if ((s8)esp->partsNo >= -8 && (s8)esp->partsNo <= -3) {
        pLog->err(0, 0, "ESP_12 : Parent is screen.");
        return;
    }
    PSMTXIdentity(esp->mat);
    RotMatrix(esp->mat, &esp->rot);
    PSMTXConcat(pG->Cam.viewMat, esp->mat, esp->mat);
    PSMTXInverse(esp->mat, inv);
    PSMTXTranspose(inv, inv);
    GXLoadNrmMtxImm(inv, 0);
    GXLoadPosMtxImm(esp->mat, 0);
    GXSetCurrentMtx(0);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xA, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xA, 0, 1, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    tstep = 1.0f / (f32)w->n;
    camPos = pG->Cam.param.pos;
    nrm.x = nrm.y = nrm.z = 0.0f;
    GXBegin(0x98, 0, n * 2);
    p = Esp3f_GetVecPtr(w->buf, 0);
    next = NULL;
    for (i = 0; i < n; i++) {
        if (i != n - 1) {
            next = Esp3f_GetVecPtr(w->buf, i + 1);
            PSVECSubtract(next, p, &dir);
        }
        PSVECSubtract(p, &camPos, &toCam);
        PSVECCrossProduct(&dir, &toCam, &cross);
        if (!(cross.x == 0.0f && cross.y == 0.0f && cross.z == 0.0f)) {
#line 208 "D:/Bio4/Prog/esp12.cpp"
            VECNormalize(&cross, &nrm);
        }
        r = (f32)i / (f32)(n - 1);
        wid = ((1.0f - r) * esp->sizeX + r * esp->sizeY) * esp->scale;
        PSVECScale(&nrm, &v0, wid);
        PSVECScale(&nrm, &v1, -wid);
        PSVECAdd(&v0, p, &v0);
        PSVECAdd(&v1, p, &v1);
        p = next;
        GXPosition3f32(v0.x, v0.y, v0.z);
        GXNormal3s8(0, 1, 0);
        GXTexCoord2f32(0.0f, t);
        GXPosition3f32(v1.x, v1.y, v1.z);
        GXNormal3s8(0, 1, 0);
        GXTexCoord2f32(1.0f, t);
        t += tstep;
    }
}

void cEsp12::Destruct()
{
    cEsp* b = (cEsp*)work.buf;

    if (b != NULL && (b->flag & 1)) {
        PushEsp(b);
    }
}

int cEsp12::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp12Work* w = &work;
    Vec wpos;
    int i;

    if (gen->xC8 > 0x7B) {
        pLog->err(0, 0, "ESP_12 : WK0 > 123.");
        return 0;
    }
    w->n = (s8)gen->xC8 + 2;
    if (!Esp3f_Alloc(sizeof(Vec), w->n, &w->buf, &info)) {
        pLog->err(0, 0, "ESP_12 : Buf alloc failed.");
        return 0;
    }
    FSet(xB8, 100000000.0f);
    BitOn16(dispFlag, 2);
    if (parent == pEffParentWorld) {
        wpos = pos;
    } else {
        PSMTXMultVec(parent->mat, &pos, &wpos);
    }
    for (i = w->n - 1; i >= 0; i--) {
        *Esp3f_GetVecPtr(w->buf, i) = wpos;
    }
    return 1;
}
