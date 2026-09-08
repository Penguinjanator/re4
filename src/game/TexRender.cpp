// game/TexRender: render-to-texture manager and helpers (D:/Bio4/Prog/TexRender.cpp).
#include "types.h"
#include "global.h"
#include "event.h"
#include "db_log.h"
#include "main_mem.h"
#include "main_sub.h"
#include "view.h"
#include "model.h"
#include "trans_ot.h"
#include "TexRender.h"

// game/model.cpp
cModelInfo* GetModelInfoAddr(cModelInfo* info, int no);
void ModelInfoRefrectOffAll(cModel* m);
void ModelInfoRefrectOn(cModel* m, int no);
// game/trans.cpp
int commonScreenMat(cModel* m);
void lightSetEm(cModel* m);
void ModelRender(cModel* m);
// game/mirror.cpp
void MirrorDraw2(cModel* m);

TexRenderMng g_RndMgr[8];
int g_RndMgrNum;
int g_draw = 0;
int g_TexUse = 0;

TexRenderMng* GetTexRenderMgrAddr(int no)
{
    return &g_RndMgr[no];
}

void TexRenderMgrInit()
{
    u32 i;

    g_RndMgrNum = 0;
    for (i = 0; i < 8; i++) {
        g_RndMgr[i].Init();
    }
    g_draw = 0;
}

void TexRenderMgrRoomInit()
{
    u32 i;

    g_RndMgrNum = 0;
    for (i = 0; i < 8; i++) {
        g_RndMgr[i].Init();
    }
    g_draw = 0;
}

int GetTexRenderMgr(TexRenderMng** out)
{
    if (g_RndMgrNum == 8) {
        pLog->err(0, 0, "GetTexRenderMgr() : Manager full!!");
        return 0;
    }
    *out = &g_RndMgr[g_RndMgrNum];
    (*out)->Init();
    if (!(*out)->AllocBuf()) {
        return 0;
    }
    (*out)->texId = (u8) g_RndMgrNum + 0xF8;
    (*out)->mask = 8 << g_RndMgrNum;
    g_RndMgrNum++;
    (*out)->used = 1;
    return 1;
}

// EFB x offset that centres a 2x copy of the texture in the resized frame.
static inline u32 texRenderOfs(TexRenderMng* m)
{
    if (m->sx == 0xE0) {
        return (u32) ((f32) m->sy * 2.0f / 0.875f - (f32) m->sx * 2.0f);
    }
    return (m->sx >> 2) + (m->sx >> 4);
}

void RenderTexRenderMgr(TexRenderMng* m)
{
    u32 ofs = texRenderOfs(m);
    u32 w = m->sx * 2 + ofs;
    u32 h = m->sy * 2;

    if (w > 0x280) {
        pLog->err(0, 0, "RenderTexRenderMgr:: Invalid SX[%d]", w);
        w = 0x280;
    }
    if (h > 0x210) {
        pLog->err(0, 0, "RenderTexRenderMgr:: Invalid SY[%d]", h);
        h = 0x210;
    }
    EFBReSize(w, h);
    GXSetScissor(ofs >> 1, 0, m->sx << 1, m->sy << 1);
}

void CopyTexRenderMgr(TexRenderMng* m)
{
    static u8 vfilter[7] __attribute__((aligned(32))) = {32, 0, 0, 0, 0, 0, 32};

    if (pG->flags_5010 & 0x08000000) {
        u32 ofs, w, h;
        int wrap;

        GXSetCopyFilter(0, Rmode.sample_pattern, 0, vfilter);
        GXSetAlphaUpdate(1);
        ofs = texRenderOfs(m);
        w = m->sx * 2;
        h = m->sy * 2;
        if (w > 0x280) {
            pLog->err(0, 0, "CopyTexRenderMgr:: Invalid SX[%d]", w);
            w = 0x280;
        }
        if (h > 0x210) {
            pLog->err(0, 0, "CopyTexRenderMgr:: Invalid SY[%d]", h);
            h = 0x210;
        }
        GXSetTexCopySrc(ofs >> 1, 0, w, h);
        GXSetTexCopyDst(m->sx, m->sy, 6, 1);
        GXCopyTex(m->buf, 1);
        GXSetAlphaUpdate(0);
        GXSetCopyFilter(Rmode.aa, Rmode.sample_pattern, 1, Rmode.vfilter);
        GXPixModeSync();
        GXInvalidateTexAll();
        switch (m->repType) {
        case 1:
            wrap = 1;
            break;
        case 2:
            wrap = 0;
            break;
        default:
            pLog->err(0, 0, "TexRenderMng:: Invalid REPTYPE[%d]", m->repType);
        case 0:
            wrap = 2;
            break;
        }
        GXInitTexObj(&m->texObj, m->buf, m->sx, m->sy, 6, wrap, wrap, 0);
        GXInitTexObjLOD(&m->texObj, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
        g_draw = 1;
    }
    if (m == &g_RndMgr[g_RndMgrNum - 1]) {
        pG->flags_5010 &= ~0x08000000;
        ScreenReSize(0x200, 0x1C0);
        SetScissorState();
    }
}

void TransTexRenderMgr()
{
    u32 i;

    for (i = 0; i < (u32) g_RndMgrNum; i++) {
        if (g_RndMgr[i].used != 0) {
            AddOtDirect((u16) i, &g_RndMgr[i], (void (*)()) RenderTexRenderMgr, 9, 0x800, NULL, 0.0f);
            AddOtDirect((u16) i, &g_RndMgr[i], (void (*)()) CopyTexRenderMgr, 0, 0x800, NULL, 0.0f);
        }
    }
    g_TexUse = (pG->flags_60 & 0x80) ? 1 : 0;
}

TexRenderMng::TexRenderMng()
{
    Init();
}

void TexRenderMng::Init()
{
    used = 0;
    buf = NULL;
    texId = 0;
    x29 = 0;
    mask = 0;
    sx = 0x80;
    sy = 0x80;
    repType = 0;
}

int TexRenderMng::AllocBuf()
{
#line 323 "D:/Bio4/Prog/TexRender.cpp"
    buf = MEM_ALLOC(sx * sy * 4, 1, 13);
    if (buf == NULL) {
        pLog->err(0, 0, "TexRenderMng::AllocBuf() : not enough memory");
        return 0;
    }
    return 1;
}

void TexRenderMng::ReAllocBuf()
{
    if (buf != NULL) {
        Mem_free(buf);
    }
    AllocBuf();
}

void TexRenderInit(TexRenderMng** out, int size, int repType)
{
    if (!GetTexRenderMgr(out)) {
        pLog->err(0, 0, "TexRenderInit() : Manager alloc failed!!");
    }
    if (size != 0) {
        TexRenderMng* m = *out;
        m->sx = size;
        m->sy = size;
        (*out)->ReAllocBuf();
    }
    (*out)->repType = repType;
}

void TexRenderModSet(cModel* m, int parts, u8* tbl, TexRenderMng* mgr, int keepBlendType, int keepRefrect, int keepD6, int keep12C, f32 alpha)
{
    cModelInfo* info;

    if (mgr == NULL) {
        pLog->err(0, 0, "TexRenderModSet() : Manager alloc failed!!");
        return;
    }
    if (m == NULL) {
        pLog->err(0, 0, "TexRenderModSet() : failed!!");
        return;
    }
    tbl[1] = 0;
    tbl[4] = 0xF7;
    tbl[0] = 1;
    tbl[5] = mgr->texId;
    info = GetModelInfoAddr(m->pInfo, parts);
    if (info != NULL) {
        info->be_flag |= 8;
        info->setTexBlendTbl(tbl);
        info->setBlendRatio(0xFF);
        if (keepBlendType == 0) {
            info->setBlendType(1);
        }
        if (keepD6 == 0) {
            info->xD6 = 1;
        }
    }
    if (keepRefrect == 0) {
        m->x136 = 2;
        m->x137 = 0x10;
        m->x138 = 0x90;
        ModelInfoRefrectOffAll(m);
        ModelInfoRefrectOn(m, parts);
    }
    if (keep12C == 0) {
        m->x12C = 2;
    }
    m->alpha = alpha;
}

void TexRenderModRes(cModel* m)
{
    cModelInfo* info;
    int parts;  // never set in the original (GetModelInfoAddr gets whatever is in r4)

    if (m == NULL) {
        pLog->err(0, 0, "TexRenderModRes() : failed!!");
        return;
    }
    info = GetModelInfoAddr(m->pInfo, parts);
    if (info != NULL) {
        info->be_flag &= ~8;
        info->setBlendRatio(0);
        info->resetTexBlendTbl();
    }
    m->x136 = 0;
    m->x137 = 0x10;
    m->x138 = 0x90;
}

void TexRenderModAddOt(int ot, cModel* m)
{
    pG->flags_5010 |= 0x08000000;
    if (m == NULL) {
        pLog->err(0, 0, "TexRenderModSet() : failed!!");
        return;
    }
    if (commonScreenMat(m)) {
        lightSetEm(m);
        AddOtDirect(ot, m, (void (*)()) ModelRender, 3, 1, NULL, 0.0f);
    }
}

void TexRenderModAddOtMirror(int ot, cModel* m)
{
    pG->flags_5010 |= 0x08000000;
    if (m == NULL) {
        pLog->err(0, 0, "TexRenderModSet() : failed!!");
        return;
    }
    if (commonScreenMat(m)) {
        AddOtDirect(0x10, m, (void (*)()) MirrorDraw2, 0, 0x400, NULL, 0.0f);
    }
    m->x13B = -1;
    m->alpha = 0.4f;
    m->be_flag |= 8;
    m->x139 = -1;
    m->x13A = -1;
}

void TexRenderCamAddOt(int ot, TexRenderCam* c, TexRenderEvt* evt, void* data)
{
    c->pEvt = evt;
    c->data = data;
    AddOtDirect(ot, c, (void (*)()) CamRenderPrev, 4, 1, NULL, 0.0f);
    AddOtDirect(ot, c, (void (*)()) CamRenderAfter, 2, 1, NULL, 0.0f);
}

void CamRenderPrev(TexRenderCam* c)
{
    TexRenderEvt* e = c->pEvt;
    int frame = e->frame;
    bool b;

    b = e->flags & 0x40000000;
    if (b) {
        frame = e->frameB;
    }
    b = e->flags & 0x08000000;
    if (b) {
        frame = e->frameEnd - 1;
    }
    c->pCam = new (&c->cam) CameraMotion(c->data, 0, 0, (f32) frame);
    c->cam.move();
    c->save = pG->Cam;
    pG->Cam = *c->pCam;
    C_MTXPerspective(pG->Cam.projMat, pG->Cam.param.fovy, 4.0f / 3.0f, ZNEAR, ZFAR);
    C_MTXLookAt(pG->Cam.viewMat, &pG->Cam.param.pos, &pG->Cam.up, &pG->Cam.param.at);
}

void CamRenderAfter(TexRenderCam* c)
{
    pG->Cam = c->save;
}
