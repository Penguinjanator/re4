#include "types.h"
#include "vec.h"
#include "gx.h"
#include "global.h"
#include "light.h"
#include "model.h"
#include "em.h"
#include "db_log.h"
#include "math_sub.h"
#include "main_mem.h"

extern "C" {
void LightSetInit();
void LightSetModel(cModel* m);
void commonClothLightSet(cLight** list, int n, Vec* pos, f32 size);
void commonWaterLightSet(cLight** list, int n, u32 alpha);
void commonEspLightSet(cLight** list, int n);
void lightSetConstant(cLight* l, GXLightObj* obj);
void lightSetLinear(cLight* l, GXLightObj* obj);
static void lightSetQuadratic(cLight* l, GXLightObj* obj);
void lightSetSpotlight(cLight* l, GXLightObj* obj);
void lightSetCustom(cLight* l, GXLightObj* obj);
void lightSetParallel(cLight* l, GXLightObj* obj);
void lightSetSpotQuad(cLight* l, GXLightObj* obj);
void lightSetLocalAmb(cLight* l, GXColor* amb);
void lightSetColor(GXLightObj* obj, cLight* l, cEm* em);
void lightSetAmbient(GXColor* col0);
void LightDisable();
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline void ISet(int& d, int v) { d = v; }

#define LIGHT_FUNC_TABLE                                                                              \
    static void (*funcLightParam[16])(cLight*, GXLightObj*) = {                                      \
        lightSetConstant, lightSetLinear,   lightSetQuadratic, lightSetSpotlight, lightSetCustom,    \
        lightSetParallel, lightSetSpotQuad, lightSetConstant,  lightSetConstant,  lightSetConstant,  \
        lightSetConstant, lightSetConstant, lightSetConstant,  lightSetConstant,  lightSetConstant,  \
        lightSetConstant,                                                                             \
    }

static GXLightObj lightObjBlack;
Vec obj_pos;
int obj_flag;
f32 obj_size;
const GXColor colZero = {0, 0, 0, 0};

void LightSetInit()
{
    GXInitLightColor(&lightObjBlack, colZero);
    GXInitLightPos(&lightObjBlack, 0.0f, 0.0f, 0.0f);
    GXInitLightDir(&lightObjBlack, 0.0f, 0.0f, 1.0f);
    GXInitLightAttn(&lightObjBlack, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
}

void LightSetModel(cModel* m)
{
    LIGHT_FUNC_TABLE;
    GXLightObj lobj[8];
    Vec p;
    GXColor mat;
    GXColor amb;
    cModelInfo* info = m->pModelInfo;
    ModelData* data = info->pData;
    cLight** list = m->LightInfo.pLight;
    int n = (m->be_flag & 0x8000) ? 0 : 8;
    u32 mask;
    int i;
    cLightEnv* env;

    if (m->LightInfo.Flag & 4) {
        LightDisable();
        return;
    }
    obj_pos = m->pParts->world;
    obj_size = m->LightInfo.Size.x > m->LightInfo.Size.y ? m->LightInfo.Size.x : m->LightInfo.Size.y;
    if ((m->LightInfo.Flag & 3) == 2) {
        obj_flag = 0;
    } else {
        obj_flag = 1;
    }
    mask = 0;
    amb.a = amb.b = amb.g = amb.r = 0;
    for (i = 0; i < n; i++) {
        cLight* l = list[i];

        if (l == NULL) {
            continue;
        }
        if (l->xD == 7) {
            lightSetLocalAmb(l, &amb);
            continue;
        }
        l->getPos(&p);
        mask |= 1 << i;
        PSMTXMultVec(pG->Cam.v_mat, &p, &p);
        GXInitLightPos(&lobj[i], p.x, p.y, p.z);
        lightSetColor(&lobj[i], l, (cEm*) m);
        GXInitLightDir(&lobj[i], 0.0f, 0.0f, 1.0f);
        if (l->xD > 7) {
            pLog->err(0, 0, "LIGHT() INVALIED TYPE %d", l->xD);
            memclr_asm(l, 0x154);
        }
        funcLightParam[l->xD](l, &lobj[i]);
        GXLoadLightObjImm(&lobj[i], 1 << i);
    }
    if (mask == 0) {
        GXLoadLightObjImm(&lightObjBlack, 1);
        mask = 1;
    }
    GXSetNumChans(1);
    if (data->flags & 0x40000000) {
        GXSetChanCtrl(0, 1, 1, 0, mask, 2, 1);
        GXSetChanCtrl(2, 0, 1, 1, 0, 2, 2);
        mat = *(GXColor*) info->color;
        GXSetChanMatColor(4, mat);
        return;
    }
    GXSetChanCtrl(0, 1, 0, 0, mask, 2, 1);
    GXSetChanCtrl(2, 0, 0, 0, 0, 2, 2);
    if (m->LightInfo.EnableMask & 0x10) {
        amb.r = MAX(LightMgr.getEnvPtr()->AmbientScr.r, amb.r);
        amb.g = MAX(LightMgr.getEnvPtr()->AmbientScr.g, amb.g);
        amb.b = MAX(LightMgr.getEnvPtr()->AmbientScr.b, amb.b);
    } else if (m->LightInfo.EnableMask & 8) {
        amb.r = MAX(LightMgr.getEnvPtr()->AmbientEsp.r, amb.r);
        amb.g = MAX(LightMgr.getEnvPtr()->AmbientEsp.g, amb.g);
        amb.b = MAX(LightMgr.getEnvPtr()->AmbientEsp.b, amb.b);
    } else {
        amb.r = MAX(LightMgr.getEnvPtr()->AmbientEm.r, amb.r);
        amb.g = MAX(LightMgr.getEnvPtr()->AmbientEm.g, amb.g);
        amb.b = MAX(LightMgr.getEnvPtr()->AmbientEm.b, amb.b);
    }
    if (m->be_flag & 8) {
        amb.r += m->AddAmb_r;
        amb.g += m->AddAmb_g;
        amb.b += m->AddAmb_b;
    }
    lightSetAmbient(&amb);
    mat = *(GXColor*) info->color;
    GXSetChanMatColor(4, mat);
}

void commonClothLightSet(cLight** list, int n, Vec* pos, f32 size)
{
    LIGHT_FUNC_TABLE;
    GXLightObj lobj[8];
    Vec p;
    GXColor amb;
    u32 mask;
    int i;

    ISet(obj_flag, 0);
    obj_pos = *pos;
    obj_size = size;
    mask = 0;
    amb.a = amb.b = amb.g = amb.r = 0;
    for (i = 0; i < n; i++) {
        cLight* l = list[i];

        if (l == NULL) {
            continue;
        }
        if (l->xD == 7) {
            lightSetLocalAmb(l, &amb);
            continue;
        }
        l->getPos(&p);
        mask |= 1 << i;
        PSMTXMultVec(pG->Cam.v_mat, &p, &p);
        GXInitLightPos(&lobj[i], p.x, p.y, p.z);
        lightSetColor(&lobj[i], l, NULL);
        GXInitLightDir(&lobj[i], 0.0f, 0.0f, 1.0f);
        if (l->xD > 7) {
            pLog->err(0, 0, "LIGHT() INVALIED TYPE %d", l->xD);
            memclr_asm(l, 0x154);
        }
        funcLightParam[l->xD](l, &lobj[i]);
        GXLoadLightObjImm(&lobj[i], 1 << i);
    }
    if (mask == 0) {
        GXLoadLightObjImm(&lightObjBlack, 1);
        mask = 1;
    }
    GXSetNumChans(1);
    GXSetChanCtrl(0, 1, 0, 0, mask, 2, 1);
    GXSetChanCtrl(2, 0, 0, 0, 0, 2, 2);
    amb.r = MAX(LightMgr.getEnvPtr()->AmbientScr.r, amb.r);
    amb.g = MAX(LightMgr.getEnvPtr()->AmbientScr.g, amb.g);
    amb.b = MAX(LightMgr.getEnvPtr()->AmbientScr.b, amb.b);
    lightSetAmbient(&amb);
    GXColor mat = {0xFF, 0xFF, 0xFF, 0xFF};
    GXSetChanMatColor(4, mat);
    GXSetChanCtrl(2, 0, 0, 0, 0, 2, 2);
}

void commonWaterLightSet(cLight** list, int n, u32 alpha)
{
    LIGHT_FUNC_TABLE;
    GXLightObj lobj[8];
    Vec p;
    GXColor amb;
    u32 mask;
    int i;

    ISet(obj_flag, 0);
    mask = 0;
    amb.a = amb.b = amb.g = amb.r = 0;
    for (i = 0; i < n; i++) {
        cLight* l = list[i];
        u8 a;

        if (l == NULL) {
            continue;
        }
        if (l->xD == 7) {
            lightSetLocalAmb(l, &amb);
            continue;
        }
        l->getPos(&p);
        mask |= 1 << i;
        PSMTXMultVec(pG->Cam.v_mat, &p, &p);
        GXInitLightPos(&lobj[i], p.x, p.y, p.z);
        a = l->DispCol.a;
        l->DispCol.a = (a * alpha) >> 8;
        lightSetColor(&lobj[i], l, NULL);
        l->DispCol.a = a;
        GXInitLightDir(&lobj[i], 0.0f, 0.0f, 1.0f);
        if (l->xD > 7) {
            pLog->err(0, 0, "LIGHT() INVALIED TYPE %d", l->xD);
            memclr_asm(l, 0x154);
        }
        funcLightParam[l->xD](l, &lobj[i]);
        GXLoadLightObjImm(&lobj[i], 1 << i);
    }
    if (mask == 0) {
        GXLoadLightObjImm(&lightObjBlack, 1);
        mask = 1;
    }
    GXSetNumChans(1);
    GXSetChanCtrl(0, 1, 0, 0, mask, 2, 1);
    GXSetChanCtrl(2, 0, 0, 0, 0, 2, 2);
    amb.r = MAX(LightMgr.getEnvPtr()->AmbientScr.r, amb.r);
    amb.g = MAX(LightMgr.getEnvPtr()->AmbientScr.g, amb.g);
    amb.b = MAX(LightMgr.getEnvPtr()->AmbientScr.b, amb.b);
    lightSetAmbient(&amb);
    GXColor mat = {0xFF, 0xFF, 0xFF, 0xFF};
    GXSetChanMatColor(4, mat);
    GXSetChanCtrl(2, 0, 0, 0, 0, 2, 2);
}

void commonEspLightSet(cLight** list, int n)
{
    LIGHT_FUNC_TABLE;
    GXLightObj lobj[8];
    Vec p;
    u32 mask;
    int i;

    obj_flag = 0;
    mask = 0;
    for (i = 0; i < n; i++) {
        cLight* l = list[i];

        if (l == NULL) {
            continue;
        }
        mask |= 1 << i;
        PSMTXMultVec(pG->Cam.v_mat, &l->Pos, &p);
        GXInitLightPos(&lobj[i], p.x, p.y, p.z);
        lightSetColor(&lobj[i], l, NULL);
        GXInitLightDir(&lobj[i], 0.0f, 0.0f, 1.0f);
        if (l->xD > 7) {
            pLog->err(0, 0, "cLight() INVALIED TYPE %d", l->xD);
            memclr_asm(l, 0x154);
        }
        funcLightParam[l->xD](l, &lobj[i]);
        GXLoadLightObjImm(&lobj[i], 1 << i);
    }
    if (mask == 0) {
        GXLoadLightObjImm(&lightObjBlack, 1);
        mask = 1;
    }
    GXSetNumChans(1);
    GXSetChanCtrl(0, 1, 0, 0, mask, 0, 1);
    GXSetChanCtrl(2, 0, 0, 0, 0, 2, 2);
    lightSetAmbient(&LightMgr.getEnvPtr()->AmbientEsp);
}

void lightSetConstant(cLight* l, GXLightObj* obj)
{
    Vec p = l->World;
    f32 d = GetDistance3(&obj_pos, &p);
    f32 range = l->Radius + obj_size;
    f32 br;

    if (d < range - l->normal.x || !(obj_flag & 1) || l->Radius == 0.0f) {
        br = l->Intensity;
    } else if (d < range) {
        br = l->Intensity * (range - d) / l->normal.x;
    } else {
        br = 0.0f;
    }
    GXInitLightAttn(obj, br, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
}

void lightSetLinear(cLight* l, GXLightObj* obj)
{
    Vec p = l->World;
    f32 br;

    if (l->Radius != 0.0f) {
        f32 d = GetDistance3(&obj_pos, &p);

        br = l->Intensity * (l->Radius - d) / l->Radius;
    } else {
        br = l->Intensity;
    }
    GXInitLightAttn(obj, br, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
}

static void lightSetQuadratic(cLight* l, GXLightObj* obj)
{
    Vec p = l->World;
    f32 k2 = 0.1f;
    f32 d = GetDistance3(&obj_pos, &p);
    f32 range = l->Radius + obj_size;
    f32 br;

    if (d < range - l->normal.x || !(obj_flag & 1) || l->Radius == 0.0f) {
        br = l->Intensity;
    } else if (d < range) {
        br = l->Intensity * (range - d) / l->normal.x;
    } else {
        br = 0.001f;
    }
    f32 zero = 0.0f;
    f32 one = 1.0f;
    if (l->Radius != zero) {
        k2 = (l->Intensity - 0.1f) / 0.1f / (l->Radius * l->Radius);
    } else {
        k2 = zero;
    }
    GXInitLightAttn(obj, br, zero, zero, one, zero, k2);
}

void lightSetSpotlight(cLight* l, GXLightObj* obj)
{
    Vec cdir;
    Vec dir;
    Vec p;
    LightSpot* sp = &l->spot;
    f32 refDist = 5000.0f;
    f32 d;
    f32 range;
    f32 br;

    l->getPos(&p);
    l->getNormal(&sp->Normal, &dir);
    PSMTXMultVecSR(pG->Cam.v_mat, &dir, &cdir);
    GXInitLightDir(obj, cdir.x, cdir.y, cdir.z);
    d = GetDistance3(&obj_pos, &p);
    range = l->Radius + obj_size;
    if (d < range - sp->A1 || !(obj_flag & 1) || l->Radius == 0.0f) {
        br = l->Intensity;
    } else if (d < range) {
        br = l->Intensity * (range - d) / sp->A1;
    } else {
        br = 0.001f;
    }
    GXInitLightSpot(obj, sp->A0, 2);
    GXInitLightDistAttn(obj, 5000.0f, br, 2);
}

void lightSetCustom(cLight* l, GXLightObj* obj)
{
    Vec cdir;
    Vec dir;
    LightSpot* sp = &l->spot;

    l->getNormal(&sp->Normal, &dir);
    PSMTXMultVecSR(pG->Cam.v_mat, &dir, &cdir);
    GXInitLightDir(obj, cdir.x, cdir.y, cdir.z);
    GXInitLightAttn(obj, sp->A0, sp->A1, sp->A2, sp->K0, sp->K1, sp->K2);
}

void lightSetParallel(cLight* l, GXLightObj* obj)
{
    Vec p;
    LightSpot* sp = &l->spot;
    f32 d;
    f32 range;
    f32 br;

    if (sp->flags & 1) {
        Mtx inv;

        PSMTXInverse(pG->Cam.v_mat, inv);
        PSMTXMultVecSR(inv, &sp->Normal, &p);
    } else {
        p = sp->Normal;
    }
    PSVECAdd(&obj_pos, &p, &p);
    PSMTXMultVec(pG->Cam.v_mat, &p, &p);
    GXInitLightPos(obj, p.x, p.y, p.z);
    Vec q;
    l->getPos(&q);
    d = GetDistance3(&obj_pos, &q);
    range = l->Radius + obj_size;
    if (d < range - sp->A1 || !(obj_flag & 1) || l->Radius == 0.0f) {
        br = l->Intensity;
    } else if (d < range) {
        br = l->Intensity * (range - d) / sp->A1;
    } else {
        br = 0.001f;
    }
    GXInitLightAttn(obj, br, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
}

void lightSetSpotQuad(cLight* l, GXLightObj* obj)
{
    Vec cdir;
    Vec dir;
    Vec p;
    LightSpot* sp = &l->spot;
    f32 k2 = 0.1f;
    f32 d;
    f32 range;
    f32 br;

    l->getPos(&p);
    l->getNormal(&sp->Normal, &dir);
    PSMTXMultVecSR(pG->Cam.v_mat, &dir, &cdir);
    GXInitLightDir(obj, cdir.x, cdir.y, cdir.z);
    d = GetDistance3(&obj_pos, &p);
    range = l->Radius + obj_size;
    if (d < range - sp->A1 || !(obj_flag & 1) || l->Radius == 0.0f) {
        br = l->Intensity;
    } else if (d < range) {
        br = l->Intensity * (range - d) / sp->A1;
    } else {
        br = 0.001f;
    }
    GXInitLightSpot(obj, sp->A0, 2);
    if (l->Radius != 0.0f) {
        k2 = (l->Intensity - 0.1f) / 0.1f / (l->Radius * l->Radius) / br;
    } else {
        k2 = 0.0f;
    }
    GXInitLightAttnK(obj, 1.0f / br, 0.0f, k2);
}

void lightSetLocalAmb(cLight* l, GXColor* amb)
{
    Vec p = l->World;
    f32 d = GetDistance3(&obj_pos, &p);
    f32 range = l->Radius + obj_size;
    GXColor c;

    if (d < range - l->normal.x || !(obj_flag & 1) || l->Radius == 0.0f) {
        c = l->DispCol;
    } else if (d < range) {
        d = (range - d) / l->normal.x;
        c.r = (u8) (d * (f32) (int) l->DispCol.r);
        c.g = (u8) (d * (f32) (int) l->DispCol.g);
        c.b = (u8) (d * (f32) (int) l->DispCol.b);
    } else {
        c.a = c.b = c.g = c.r = 0;
    }
    amb->r = MAX(amb->r, c.r);
    amb->g = MAX(amb->g, c.g);
    amb->b = MAX(amb->b, c.b);
}

void lightSetColor(GXLightObj* obj, cLight* l, cEm* em)
{
    f32 col[3];
    GXColor c;
    f32 r = (f32) l->DispCol.r;
    f32 g = (f32) l->DispCol.g;
    f32 b = (f32) l->DispCol.b;
    f32 a = (f32) (int) l->DispCol.a;

    col[0] = r * a * 0.0078125f;
    col[1] = g * a * 0.0078125f;
    col[2] = b * a * 0.0078125f;
    if (em != NULL) {
        EmLightArea* la = &em->litArea;

        if (la->chk(1) == 1 && la->chk(2) == 1 && la->lightNo == l->LitIndex) {
            col[0] *= la->scale;
            col[1] *= la->scale;
            col[2] *= la->scale;
        }
    }
    col[0] = col[0] < 0.0f ? 0.0f : (col[0] > 255.0f ? 255.0f : col[0]);
    col[1] = col[1] < 0.0f ? 0.0f : (col[1] > 255.0f ? 255.0f : col[1]);
    col[2] = col[2] < 0.0f ? 0.0f : (col[2] > 255.0f ? 255.0f : col[2]);
    c.r = (u8) col[0];
    c.g = (u8) col[1];
    c.b = (u8) col[2];
    c.a = 0x80;
    GXInitLightColor(obj, c);
}

void lightSetAmbient(GXColor* col0)
{
    GXSetChanAmbColor(4, *col0);
}

void LightDisable()
{
    GXLoadLightObjImm(&lightObjBlack, 1);
    GXSetNumChans(1);
    GXSetChanCtrl(0, 1, 0, 0, 1, 2, 1);
    GXSetChanCtrl(2, 0, 0, 0, 0, 2, 2);
    GXColor c = {0, 0, 0, 0xFF};
    GXSetChanMatColor(4, c);
    GXSetChanAmbColor(4, c);
}

