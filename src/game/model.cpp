#include "atari.h"
#include "model.h"
#include "motion.h"
#include "global.h"
#include "db_log.h"
#include "va_ppc.h"
#include "main_mem.h"
#include "dbmodule.h"
#include "eprintf.h"
#include "scheduler.h"
#include "math_sub.h"
#include "tpl.h"

// Model / parts / model info (cModel, cParts, cModelInfo) and their pools (PartsMgr, ModInfoMgr).

#define HALT()                                                    \
    {                                                             \
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);            \
        *(volatile u32*) 0x11111111 = 0;                          \
    }

// A relocated pointer into main memory.
#define PTR_OK(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)

extern "C" {
void OSReport(const char* fmt, ...);
void PartsWorldPosCalc(cModel* m);
void calcModelAddr(ModelData* data);
void calcModelOffset(ModelData* data);
void calcTplOffset(TEXPalette* tpl);
void getBoundingBox(ModelData* data, ModelBound* bound);
void drawBoundingBox(Mtx m, ModelBound* bound);
int GetModelInfoNum(cModelInfo* info);
// MotionMove takes a second argument (pl_npc.cpp MotionMoveF)
int MotionMoveF(cModel* m, int flag) asm("MotionMove");
// cAtariInfo lives in cModel's union (no member constructor call): constructed by hand
cAtariInfo* AtariInfoConstruct(cAtariInfo* p) asm("__10cAtariInfo");
}
cModelInfo* GetModelInfoAddr(cModelInfo* info, int no);

cModInfoMgr* cModel::mm = &ModInfoMgr;
cPartsMgr* cModel::pm = &PartsMgr;

// `mm` read as a struct member: keeps its load below a preceding store through `this`
struct ModInfoMgrPtr {
    cModInfoMgr* p;
};
#define MM (((ModInfoMgrPtr*) &cModel::mm)->p)

static inline u32 U32Get(u32& v)
{
    return v;
}

// The 0.0 pool load of the light area sinks below the three word stores: an inlined helper
// (integrate.c drops RTX_UNCHANGING_P from the pool MEM).
static inline void LightAreaInit(EmLightArea* la)
{
    la->x0 = 0;
    la->flags = 0;
    la->lightNo = 0;
    la->scale = 0.0f;
}

// Byte stores through this setter come from a word-sized zero pseudo, which the word stores
// after the following `if` share (modelInit).
static inline void U8Set(u8& d, u8 v)
{
    d = v;
}

cModel::cModel()
{
    AtariInfoConstruct(&atari);
    LightAreaInit(&litArea);
    alpha_omit = 0xFF;
    speed.x = 0.0f;
    speed.y = 0.0f;
    speed.z = 0.0f;
    pos_old.x = 0.0f;
    pos_old.y = 0.0f;
    pos_old.z = 0.0f;
    Wall_norm.x = 0.0f;
    Wall_norm.y = 0.0f;
    Wall_norm.z = 0.0f;
    pParts = 0;
    r_no_0 = 0;
    r_no_1 = 0;
    r_no_2 = 0;
    r_no_3 = 0;
    id = 0;
    type = 0;
    nParts = 0;
    pFloor_norm = 0;
    TevScaleGroup = 0;
    kindid = 0;
    ot_type = 0;
    pCldShMd = 0;
    Shd_color = 0;
    CullMode = 0;
    Shader_type = 0;
    Refract_pow = 0;
    Fix_parts = 0;
    Fix_pos.x = 0.0f;
    invisible_trg = 0;
    invisible_old = 0;
    invisible_mode = 0;
    invisible_busy = 0;
    invisible_timer = 0;
    pModelInfo = 0;
    pShadowModelInfo = 0;
    Fix_pos.y = 0.0f;
    Fix_pos.z = 0.0f;
    invisible_factor = 0.0f;
    memclr_asm(&Motion, 0xD0);
    Motion.blend = 0;
    Motion.flip = 0;
    inscreen_pos = 0;
    pPath = 0;
    pTexChg = 0;
}

int cModel::modelInit(void* bin, void* tpl)
{
    cModelInfo* info;

    if (bin == NULL) {
        pLog->err(0, 0, "modelInit() : bin_addr == NULL.");
        return 0;
    }
    if (tpl == NULL) {
        pLog->err(0, 0, "modelInit() : tex_addr == NULL.");
        return 0;
    }
    calcModelAddr((ModelData*) bin);
    calcTplAddr((TEXPalette*) tpl);
    if (pModelInfo != NULL) {
        releaseModelInfo();
    }
    info = mm->create(bin, tpl);
    if (info == NULL) {
        pLog->err(0, 0, "modelInit() cModelInfo alloc failed.");
        return 0;
    }
    addModel(info);
    if (initJoint(bin) != 1) {
        pLog->err(0, 0, "ModelInit()  Parts allocate was failed.");
        releaseModelInfo();
        return 0;
    }
    if (pG->System_flg & 0x800000) {
        invisible_factor = 0.0f;
    } else {
        invisible_factor = 1.0f;
    }
    be_flag |= 6;
    invisible_factor2 = 1.0f;
    U8Set(TevScaleGroup, 0);
    U8Set(CullMode, 0);
    if (kindid == 0) {
        be_flag |= 0x10;
    }
    pShadowModelInfo = 0;
    Motion.Seq_speed = 1.0f;
    pCldShMd = 0;
    p2A4 = 0;
    return (int) info;
}

int cModel::initJoint(void* bin)
{
    releaseJoint();
    nParts = ((ModelData*) bin)->x19;
    if (nParts == 0) {
        return 1;
    }
    if (makePartsList(0) == 0) {
        nParts = 0;
        pParts = 0;
        return 0;
    }
    setPartsParent();
    setPartsOffset(bin);
    setJointInfo(bin);
    return 1;
}

void cModel::releaseJoint()
{
    if (pParts != NULL) {
        releasePartsList(0);
    }
}

void cModel::setPartsOffset(void* bin)
{
    cParts* p = pList;
    ModelDataHead* rec;
    Vec pos;
    u32 i;

    calcModelAddr((ModelData*) bin);
    rec = ((ModelData*) bin)->pHead;
    for (i = 0; i < nParts; i++) {
        p->pos.x = rec->center.x;
        p->pos.y = rec->center.y;
        p->pos.z = rec->center.z;
        rec++;
        p = p->pList;
    }
    pos.x = this->pos.x;
    pos.y = this->pos.y;
    pos.z = this->pos.z;
    PSMTXIdentity(mat);
    PSMTXTrans(mat, 0.0f, 0.0f, 0.0f);
    partsMatCalc();
    partsWorldCalc();
    this->pos.x = pos.x;
    this->pos.y = pos.y;
    this->pos.z = pos.z;
    for (p = pList; p; p = p->pList) {
        PSMTXIdentity(p->lt_inv_mat);
        p->lt_inv_mat[0][3] = -p->mat[0][3];
        p->lt_inv_mat[1][3] = -p->mat[1][3];
        p->lt_inv_mat[2][3] = -p->mat[2][3];
    }
    PSMTXTrans(mat, this->pos.x, this->pos.y, this->pos.z);
    partsMatCalc();
    partsWorldCalc();
    for (p = pList; p; p = p->pList) {
        p->world_old = p->world;
        p->world_old2 = p->world;
    }
}

void cModel::setPartsParent()
{
    cParts* p = pList;
    ModelDataHead* rec = pModelInfo->pData->pHead;
    u32 i;

    for (i = 0; i < nParts; i++) {
        if (rec->parentNo > 0xFE) {
            p->pParent = this;
        } else {
            p->pParent = getPartsPtr(rec->parentNo);
        }
        p = p->pList;
        rec++;
    }
}

void cModel::partsMatCalc()
{
    cParts* p;

    for (p = pList; p; p = p->pList) {
        MtxPtr m = p->l_mat;
        RotMatrix(m, &p->ang);
        TransMatrix(m, &p->pos);
        ScaleMatrix(m, &p->scale);
        PSMTXCopy(m, p->mat);
    }
}

// Out-of-line inlines. Deferred-inline emission is definition order: ~cModelInfo, then the two
// managers (implicit dtor + in-class memAlloc/memFree/memClear, model.h), ~cParts, ~cModel, then
// these three and getPartsPtr, then the cManager::destroy instantiations and the ~cManager
// instantiations the synthesized manager dtors request in finish_file.
inline void cModel::move()
{
}

inline void cModel::setNoSuspend(int on)
{
    if (on) {
        be_flag |= 0x800;
    } else {
        be_flag &= ~0x800;
    }
}

inline int cModel::isTrans()
{
    int ret = 0;

    if ((be_flag & 2) && be_flag != 0) {
        ret = 1;
    }
    return ret;
}

// Parts `no` (-1: the model itself); NULL and a log when the chain is shorter. Defined here so
// that the callers below inline it while setPartsParent above calls it.
inline cModel* cModel::getPartsPtr(int no)
{
    cModel* p = pParts;
    int cnt;

    if (no < 0) {
        return this;
    }
    cnt = no;
    if (!PTR_OK(p)) {
        return 0;
    }
    if (be_flag & 0x2000) {
        p = (cModel*) ((cParts*) p + no);
    } else if (no--) {
        do {
            cModel* next = p->pParts;
            if (!PTR_OK(next)) {
                pLog->err(0, 0, "cModel::getPartsPtr() cParts NO ERROR %d", cnt);
                return 0;
            }
            p = next;
        } while (no--);
    }
    return p;
}

void cModel::matBlend(f32 rate)
{
    cParts* p;
    Mtx m;
    Quaternion q0;
    Quaternion q1;
    Quaternion q2;
    Vec pos;
    Vec len;
    Vec vx;
    Vec vy;
    Vec vz;
    Vec trans;

    for (p = pList; p; p = p->pList) {
        // COMPILER-DIFF: 3 (address-copy shape). The original computes `&p->worldMat` into a temp
        // and copies it (`addi r0,r31,60; mr r28,r0`). With the temp pinned to r0 and kept live
        // past the copy by the codeless anchor below (before the PSVECMag call), combine cannot
        // fold the copy into the addi (the r0 set is still needed) and regmove skips hard registers.
        register MtxPtr t asm("r0") = p->l_mat;
        MtxPtr wm = t;

        vx.x = p->l_mat[0][0];
        vx.y = p->l_mat[1][0];
        vx.z = p->l_mat[2][0];
        asm("" : "=m"(vx) : "r"(t)); // COMPILER-DIFF: 3 (keep-alive for the r0 temp)
        len.x = PSVECMag(&vx);
        vy.x = p->l_mat[0][1];
        vy.y = p->l_mat[1][1];
        vy.z = p->l_mat[2][1];
        len.y = PSVECMag(&vy);
        vz.x = p->l_mat[0][2];
        vz.y = p->l_mat[1][2];
        vz.z = p->l_mat[2][2];
        len.z = PSVECMag(&vz);
        trans.x = p->l_mat[0][3];
        trans.y = p->l_mat[1][3];
        trans.z = p->l_mat[2][3];
        if (len.x != 0.0f && len.y != 0.0f && len.z != 0.0f) {
            if (len.x != 1.0f) {
                PSVECScale(&vx, &vx, 1.0f / len.x);
            }
            if (len.y != 1.0f) {
                PSVECScale(&vy, &vy, 1.0f / len.y);
            }
            if (len.z != 1.0f) {
                PSVECScale(&vz, &vz, 1.0f / len.z);
            }
            p->l_mat[0][0] = vx.x;
            wm[1][0] = vx.y;
            wm[2][0] = vx.z;
            wm[0][1] = vy.x;
            wm[1][1] = vy.y;
            wm[2][1] = vy.z;
            wm[0][2] = vz.x;
            wm[1][2] = vz.y;
            wm[2][2] = vz.z;
            wm[0][3] = trans.x;
            wm[1][3] = trans.y;
            wm[2][3] = trans.z;
        }
        {
            f32 inv = 1.0f - rate;
            PSVECScale(&len, &len, inv);
            PSVECScale(&p->scale, &p->scale, rate);
            PSVECAdd(&p->scale, &len, &p->scale);
            pos.x = p->pos.x * rate + p->l_mat[0][3] * inv;
            pos.y = p->pos.y * rate + p->l_mat[1][3] * inv;
            pos.z = p->pos.z * rate + p->l_mat[2][3] * inv;
            p->pos = pos;
            PSMTXIdentity(m);
            RotMatrix(m, &p->ang);
            C_QUATMtx(&q0, m);
            C_QUATMtx(&q1, wm);
            C_QUATSlerp(&q0, &q1, &q2, inv);
            PSMTXQuat(wm, &q2);
            TransMatrix(wm, &p->pos);
            ScaleMatrix(wm, &p->scale);
        }
    }
}

void cModel::zeroPartsPosInit(Vec* pos, Vec* rot)
{
    setPos(pos);
    setAng(rot);
    MotionClear(this, 0);
    matUpdate();
}

void cModel::partsWorldCalc()
{
    cParts* p;
    Vec tmp;
    Vec sc;
    Mtx m1;
    Mtx m2;

    r_scale = scale;
    p = pList;
    if (!PTR_OK(p)) {
        pLog->err(2, 0, "partsWorldCalc() MODEL HAS NO PARTS");
        return;
    }
    if (!PTR_OK(p->pParent)) {
        pLog->err(2, 0, "partsWorldCalc() PARENT ADDR ERR %08x", p->pParent);
        return;
    }
    for (; p; p = p->pList) {
        cCoord* parent = p->pParent;
        MtxPtr m;

        if (p->motParts.flags & 2) {
            continue;
        }
        tmp.x = p->l_mat[0][3];
        tmp.y = p->l_mat[1][3];
        tmp.z = p->l_mat[2][3];
        m = parent->mat;
        PSMTXMultVec(m, &tmp, &p->world);
        if (parent->r_scale.x != parent->r_scale.y || parent->r_scale.y != parent->r_scale.z) {
            sc.x = (parent->r_scale.x != 0.0f) ? 1.0f / parent->r_scale.x : 0.0f;
            sc.y = (parent->r_scale.y != 0.0f) ? 1.0f / parent->r_scale.y : 0.0f;
            sc.z = (parent->r_scale.z != 0.0f) ? 1.0f / parent->r_scale.z : 0.0f;
            PSMTXScale(m1, sc.x, sc.y, sc.z);
            PSMTXConcat(parent->mat, m1, m1);
            PSMTXConcat(m1, p->l_mat, p->mat);
            PSMTXScale(m1, parent->r_scale.x, parent->r_scale.y, parent->r_scale.z);
            PSMTXConcat(p->mat, m1, p->mat);
        } else {
            PSMTXConcat(m, p->l_mat, p->mat);
        }
        m = p->mat;
        if (p->motParts.flags & 0x40000000) {
            p->motParts.flags &= ~0x40000000;
            PSMTXRotRad(m2, 'x', p->addRot.x);
            PSMTXConcat(m, m2, m);
            PSMTXRotRad(m2, 'z', p->addRot.z);
            PSMTXConcat(m, m2, m);
            PSMTXRotRad(m2, 'y', p->addRot.y);
            PSMTXConcat(m2, m, m);
        }
        TransMatrix(m, &p->world);
        p->r_scale.x = parent->r_scale.x * p->scale.x;
        p->r_scale.y = parent->r_scale.y * p->scale.y;
        p->r_scale.z = parent->r_scale.z * p->scale.z;
    }
    Motion.Pos_world = pos;
}

void cModel::setParent(cModel* parent, Vec* pos, Vec* rot)
{
    cParts* p = pList;

    p->pParent = parent;
    p->pos = *pos;
    p->ang = *rot;
}

void cModel::setParent(cModel* parent, int partsNo, Vec* pos, Vec* rot)
{
    setParent(parent->getPartsPtr(partsNo), pos, rot);
}

void cModel::updateOldPos()
{
    cParts* p;

    pos_old = pos;
    for (p = pList; p; p = p->pList) {
        p->world_old2 = p->world_old;
        p->world_old = p->world;
    }
}

void cModel::setPos(Vec* pos)
{
    Vec d;
    cParts* p;

    PSVECSubtract(pos, &this->pos, &d);
    p = pList;
    if (PSVECMag(&d) != 0.0f) {
        PSVECAdd(&this->pos, &d, &this->pos);
        PSVECAdd(&pos_old, &d, &pos_old);
        for (; p; p = p->pList) {
            PSVECAdd(&p->world, &d, &p->world);
            p->world_old = p->world;
            p->mat[0][3] += d.x;
            p->mat[1][3] += d.y;
            p->mat[2][3] += d.z;
        }
        mat[0][3] = this->pos.x;
        mat[1][3] = this->pos.y;
        mat[2][3] = this->pos.z;
    }
    LightInfo.updateMatrix(this);
    atari.m_stat |= 1;
}

void cModel::setAng(Vec* ang)
{
    this->ang = *ang;
    matUpdate();
}

void cModel::setSca(Vec* sca)
{
    scale = *sca;
    matUpdate();
}

void cModel::debugSkeletonDisp()
{
    cParts* p;
    Vec scr;
    Vec ax;
    Vec ay;
    Vec az;
    Vec wp;
    u32 i;
    // COMPILER-DIFF: candidate (gcse PRE pseudo numbering). Four dead pool constants (labels consumed
    // at expand, the loads deleted before gcse, the entries never output) move the 10.0/1.0 pool
    // labels from .LC24/.LC25 to .LC28/.LC29: the PRE'd highs are numbered by hash bucket
    // ((h(name) + 81) % 151 here) and .LC29 wraps to bucket 0 below .LC28's 150, so it is
    // allocated first and takes r22, the 10.0 high r21, like the original (see exception.cpp).
    f32 lc0 = 101.125f;
    f32 lc1 = 102.125f;
    f32 lc2 = 103.125f;
    f32 lc3 = 104.125f;

    p = pList;
    if (p == NULL) {
        return;
    }
    Draw_sphere(&pos, 15.0f, 0xFFFF, 1, 1);
    Draw_line3d(&p->world, &pos, 0xFFFFFFFF, 0);
    ax.x = 25.0f;
    ax.y = 0.0f;
    ax.z = 0.0f;
    ay.x = 0.0f;
    ay.y = 25.0f;
    ay.z = 0.0f;
    az.x = 0.0f;
    az.y = 0.0f;
    az.z = 25.0f;
    PSMTXMultVec(p->mat, &ax, &ax);
    PSMTXMultVec(p->mat, &ay, &ay);
    PSMTXMultVec(p->mat, &az, &az);
    Draw_line3d(&p->world, &ax, 0xFFFF0000, 0);
    Draw_line3d(&p->world, &ay, 0xFF00FF00, 0);
    Draw_line3d(&p->world, &az, 0xFF0000FF, 0);
    Draw_sphere(&p->world, 15.0f, 0xFFFF, 1, 1);
    wp = p->world;
    GetScreenPos(&wp, &scr);
    scr.x += 10.0f;
    scr.y += 10.0f;
    if (scr.z < 1.0f) {
        eprintf2(8, 0xE, (int) scr.x, (int) scr.y, 1, 0, "[%02d]", 0);
    }
    p = p->pList;
    for (i = 1; p; p = p->pList, i++) {
        if (p->pParent) {
            Draw_line3d(&p->world, &p->pParent->world, 0xFFFF, 0);
        }
        Draw_sphere(&p->world, 10.0f, 0xFF0000FF, 1, 1);
        ax.x = 25.0f;
        ax.y = 0.0f;
        ax.z = 0.0f;
        ay.x = 0.0f;
        ay.y = 25.0f;
        ay.z = 0.0f;
        az.x = 0.0f;
        az.y = 0.0f;
        az.z = 25.0f;
        PSMTXMultVec(p->mat, &ax, &ax);
        PSMTXMultVec(p->mat, &ay, &ay);
        PSMTXMultVec(p->mat, &az, &az);
        Draw_line3d(&p->world, &ax, 0xFFFF0000, 0);
        Draw_line3d(&p->world, &ay, 0xFF00FF00, 0);
        Draw_line3d(&p->world, &az, 0xFF0000FF, 0);
        wp = p->world;
        GetScreenPos(&wp, &scr);
        if (scr.z < 1.0f) {
            int dx;
            int dy;

            switch (i & 7) {
            default:
                dx = 0;
                dy = 0;
                break;
            case 1:
                dx = -30;
                dy = 12;
                break;
            case 2:
                dx = 10;
                dy = -12;
                break;
            case 3:
                dx = -30;
                dy = -12;
                break;
            case 4:
                dx = -40;
                dy = 0;
                break;
            case 5:
                dx = 0;
                dy = 24;
                break;
            case 6:
                dx = 0;
                dy = -24;
                break;
            case 7:
                dx = 50;
                dy = 0;
                break;
            }
            eprintf2(8, 0xE, (int) scr.x + dx, (int) scr.y + dy, 4, 0, "[%02d]", i);
        }
        if (pG->debug_mode == 7) {
            wp.x = 100.0f;
            wp.y = 0.0f;
            wp.z = 0.0f;
            PSMTXMultVec(p->mat, &wp, &wp);
            Draw_line3d(&p->world, &wp, 0xFFFF0000, 0);
            wp.x = 0.0f;
            wp.y = 100.0f;
            wp.z = 0.0f;
            PSMTXMultVec(p->mat, &wp, &wp);
            Draw_line3d(&p->world, &wp, 0xFF00FF00, 0);
            wp.x = 0.0f;
            wp.y = 0.0f;
            wp.z = 100.0f;
            PSMTXMultVec(p->mat, &wp, &wp);
            Draw_line3d(&p->world, &wp, 0xFF0000FF, 0);
        }
    }
}

void cModel::addModel(cModelInfo* info)
{
    cModelInfo* p;

    if (pModelInfo == NULL) {
        pModelInfo = info;
        return;
    }
    for (p = pModelInfo; p->pList; p = p->pList) {
    }
    p->pList = info;
}

void cModel::partsFixMemory(int no)
{
    cModel* p;

    if (pParts == NULL) {
        Fix_parts = 0;
        return;
    }
    p = getPartsPtr(no);
    Fix_pos = p->world;
    Fix_parts = no + 1;
}

void cModel::partsFixAdjust()
{
    cModel* p;
    Vec d;

    if (Fix_parts == 0) {
        return;
    }
    p = getPartsPtr(Fix_parts - 1);
    d.x = p->world.x - Fix_pos.x;
    d.z = p->world.z - Fix_pos.z;
    pos.x -= d.x;
    pos.z -= d.z;
    PartsWorldPosCalc(this);
    Fix_parts = 0;
}

void cModel::push()
{
    if (isAlive()) {
        be_flag &= ~0x22;
        releasePartsList(0);
        releaseModelInfo();
    }
}

void cModel::drawAllBoundingBox(cModelInfo* info)
{
    for (; info; info = info->pList) {
        drawBoundingBox(mat, &info->bound);
    }
}


cModelInfo::cModelInfo() : cUnit(1)
{
    static u32 col = 0xFFFFFFFF;

    colorWord = U32Get(col);
    PSMTXIdentity(mat);
    be_flag |= 8;
    xD8 = 1.0f;
}

void cModelInfo::setTplAddr(void* tpl)
{
    tpl_addr = tpl;
    calcTplAddr((TEXPalette*) tpl);
}

void cModelInfo::addTplAddr(void* tpl)
{
    pAddTpl = (TEXPalette*) tpl;
    calcTplAddr((TEXPalette*) tpl);
    nAddTex = pAddTpl->numDescriptors;
}

void cModelInfo::setTexBlendTbl(void* tbl)
{
    texBlendTbl = tbl;
    flagsDC |= 4;
}

void cModelInfo::resetTexBlendTbl()
{
    texBlendTbl = 0;
    flagsDC &= ~4;
}

void cModelInfo::setBlendRatio(u16 ratio)
{
    blendRatio = ratio;
}

void cModelInfo::setBlendType(u8 type)
{
    blendType = type;
}

void cModelInfo::setSpecular(u8 r, u8 g, u8 b)
{
    ModelData* d = pData;
    ModelPart* part = d->pParts;
    u32 n = d->nParts;
    u32 i;

    for (i = 0; i < n; i++) {
        part->specR = r;
        part->specG = g;
        part->specB = b;
        {
            u8* next = (u8*) (part + 1);
            part = (ModelPart*) (next + part->size);
        }
    }
}

// Never called (the original linker dropped the bodies): the "not bin data" wait loop
// cModInfoMgr::create inlines, and the two shadow model registrations.
static inline void notBinData()
{
    for (;;) {
        eprintf(0x64, 0x190, 0, 0, "not bin data");
        TaskSleep(1);
    }
}

static int lbl_80314C2C = 0;
// Dead like the two functions below; the linker dropped them but kept this table (the 0x10 zero
// bytes between the cCoord and cUnit vtable copies in .rodata).
static const s32 ShadowPtNum[4] = { 0, 0, 0, 0 };

static void ShadowModelInit(int em, int sh)
{
    if (lbl_80314C2C + ShadowPtNum[em] > sh) {
        pLog->err(0, 0, "ShadowModelInit():PtNum Over (Em:%d/Sh:%d)", em, sh);
    }
}

static void AddShadowModel(int em, int sh)
{
    if (lbl_80314C2C + ShadowPtNum[em] > sh) {
        pLog->err(0, 0, "AddShadowModel():PtNum over(Em:%d/Sh:%d)", em, sh);
    }
}

int cModel::deleteModelData(ModelData* data)
{
    cModelInfo* prev = NULL;
    cModelInfo* info = pModelInfo;

    if (info->pData == data) {
        pModelInfo = info->pList;
        MM->destroy(info);
        return 1;
    }
    while (info) {
        if (info->pData == data) {
            prev->pList = info->pList;
            MM->destroy(info);
            return 1;
        }
        prev = info;
        info = info->pList;
    }
    return 0;
}

int cModel::deleteModelInfo(cModelInfo* target)
{
    cModelInfo* prev = NULL;
    cModelInfo* info = pModelInfo;

    if (info == target) {
        pModelInfo = info->pList;
        MM->destroy(info);
        return 1;
    }
    while (info) {
        if (info == target) {
            prev->pList = info->pList;
            MM->destroy(info);
            return 1;
        }
        prev = info;
        info = info->pList;
    }
    return 0;
}

int cModel::swapModelInfo(ModelData* data, cModelInfo* newInfo)
{
    cModelInfo* prev = NULL;
    cModelInfo* info = pModelInfo;

    if (info->pData == data) {
        pModelInfo = info->pList;
        MM->destroy(info);
        addModel(newInfo);
        return 1;
    }
    while (info) {
        if (info->pData == data) {
            prev->pList = newInfo;
            newInfo->pList = info->pList;
            MM->destroy(info);
            return 1;
        }
        prev = info;
        info = info->pList;
    }
    return 0;
}

void cModel::moveDataAddr(int ofs)
{
    cModelInfo* info;

    if (be_flag & 0x80000) {
        return;
    }
    for (info = pModelInfo; info; info = info->pList) {
        if (ofs != 0) {
            info->pData = (ModelData*) ((u8*) info->pData + ofs);
            info->tpl_addr = (u8*) info->tpl_addr + ofs;
        } else {
            calcModelOffset(info->pData);
            calcTplOffset((TEXPalette*) info->tpl_addr);
        }
    }
}

// Relocates the file offsets of a model bin to pointers (once: pClr is a pointer afterwards).
void calcModelAddr(ModelData* d)
{
    u8* base = (u8*) d;

    if ((int) d->pClr < 0) {
        return;
    }
    d->pClr = base + (u32) d->pClr;
    d->pTex = base + (u32) d->pTex;
    d->pHead = (ModelDataHead*) (base + (u32) d->pHead);
    d->pWeight = base + (u32) d->pWeight;
    d->pParts = (ModelPart*) (base + (u32) d->pParts);
    d->vtxOrig = base + (u32) d->vtxOrig;
    d->nrmOrig = base + (u32) d->nrmOrig;
    if (d->version > 0x20030817) {
        if (d->blendTbl != 0) {
            d->blendTbl = (u32) base + d->blendTbl;
        }
        if (d->flipTbl != 0) {
            d->flipTbl = (u32) base + d->flipTbl;
        }
    }
    if ((u32) d->pParts & 0x1F) {
#line 1907 "D:/Bio4/Prog/model.cpp"
        HALT();
    }
}

void calcModelOffset(ModelData* d)
{
    u8* base = (u8*) d;

    if ((int) d->pClr >= 0) {
        return;
    }
    d->pClr = (void*) ((u8*) d->pClr - base);
    d->pTex = (void*) ((u8*) d->pTex - base);
    d->pHead = (ModelDataHead*) ((u8*) d->pHead - base);
    d->pWeight = (void*) ((u8*) d->pWeight - base);
    d->pParts = (ModelPart*) ((u8*) d->pParts - base);
    d->vtxOrig = (void*) ((u8*) d->vtxOrig - base);
    d->nrmOrig = (void*) ((u8*) d->nrmOrig - base);
    if (d->version > 0x20030817) {
        if (d->blendTbl != 0) {
            d->blendTbl -= (u32) base;
        }
        if (d->flipTbl != 0) {
            d->flipTbl -= (u32) base;
        }
    }
}

// The bin moved by `ofs` bytes (block.cpp compaction): shift its pointers.
void slideModelAddr(u32 addr, int ofs)
{
    ModelData* d = (ModelData*) addr;

    if ((int) d->pClr >= 0) {
        calcModelAddr(d);
    }
    d->pClr = (u8*) d->pClr + ofs;
    d->pTex = (u8*) d->pTex + ofs;
    d->pHead = (ModelDataHead*) ((u8*) d->pHead + ofs);
    d->pWeight = (u8*) d->pWeight + ofs;
    d->pParts = (ModelPart*) ((u8*) d->pParts + ofs);
    d->vtxOrig = (u8*) d->vtxOrig + ofs;
    d->nrmOrig = (u8*) d->nrmOrig + ofs;
    if (d->version > 0x20030817) {
        if (d->blendTbl != 0) {
            d->blendTbl += ofs;
        }
        if (d->flipTbl != 0) {
            d->flipTbl += ofs;
        }
    }
}

void calcTplAddr(TEXPalette* tpl)
{
    u32 i;

    if (tpl == NULL) {
        return;
    }
    if ((int) tpl->descriptorArray < 0) {
        return;
    }
    tpl->descriptorArray = (TEXDescriptor*) ((u32) tpl->descriptorArray + (u32) tpl);
    for (i = 0; i < tpl->numDescriptors; i++) {
        if (tpl->descriptorArray[i].textureHeader != NULL) {
            tpl->descriptorArray[i].textureHeader = (TEXHeader*) ((u32) tpl->descriptorArray[i].textureHeader + (u32) tpl);
            if (tpl->descriptorArray[i].textureHeader->unpacked == 0) {
                tpl->descriptorArray[i].textureHeader->data = (void*) ((u32) tpl->descriptorArray[i].textureHeader->data + (u32) tpl);
                tpl->descriptorArray[i].textureHeader->unpacked = 1;
            }
        }
    }
}

void calcTplOffset(TEXPalette* tpl)
{
    u32 i;

    if ((int) tpl->descriptorArray >= 0) {
        return;
    }
    for (i = 0; i < tpl->numDescriptors; i++) {
        if (tpl->descriptorArray[i].textureHeader != NULL) {
            if (tpl->descriptorArray[i].textureHeader->unpacked != 0) {
                tpl->descriptorArray[i].textureHeader->data = (void*) ((u8*) tpl->descriptorArray[i].textureHeader->data - (u8*) tpl);
                tpl->descriptorArray[i].textureHeader->unpacked = 0;
            }
            tpl->descriptorArray[i].textureHeader = (TEXHeader*) ((u8*) tpl->descriptorArray[i].textureHeader - (u8*) tpl);
        }
    }
    tpl->descriptorArray = (TEXDescriptor*) ((u8*) tpl->descriptorArray - (u8*) tpl);
}

void slideTplAddr(void* p, int ofs)
{
    TEXPalette* tpl = (TEXPalette*) p;
    u32 i;

    if ((int) tpl->descriptorArray >= 0) {
        calcTplAddr(tpl);
    }
    tpl->descriptorArray = (TEXDescriptor*) ((u8*) tpl->descriptorArray + ofs);
    for (i = 0; i < tpl->numDescriptors; i++) {
        if (tpl->descriptorArray[i].textureHeader != NULL) {
            tpl->descriptorArray[i].textureHeader = (TEXHeader*) ((u8*) tpl->descriptorArray[i].textureHeader + ofs);
            tpl->descriptorArray[i].textureHeader->data = (u8*) tpl->descriptorArray[i].textureHeader->data + ofs;
        }
    }
}

void cModel::releaseModelInfo()
{
    cModelInfo* info = pModelInfo;

    while (info) {
        cModelInfo* dead = info;
        info = info->pList;
        mm->destroy(dead);
    }
    pModelInfo = 0;
}

int cModel::makePartsList(int n)
{
    u32 num;
    cParts* p;
    u32 i;

    if (n == 0) {
        num = nParts;
    } else {
        num = n;
    }
    p = (cParts*) this;
    pList = pm->createSequential(num);
    if (pList != NULL) {
        be_flag |= 0x2000;
    } else {
        for (i = 0; i < num; i++) {
            cParts* np = pm->create();
            if (np == NULL) {
                releasePartsList(0);
                return 0;
            }
            p->pList = np;
            p = np;
        }
    }
    return 1;
}

void cModel::setJointInfo(void* bin)
{
    ModelData* d = (ModelData*) bin;

    if (d->version == 0x20030818) {
        if (d->blendTbl != 0) {
            Motion.blendTbl = (u16*) d->blendTbl;
        } else {
            Motion.blendTbl = 0;
        }
        if (d->flipTbl != 0) {
            Motion.flip = (u16*) (d->flipTbl + 4);
        } else {
            Motion.flip = 0;
        }
    } else {
        Motion.blendTbl = 0;
        Motion.flip = 0;
    }
}

void cModel::releasePartsList(int no)
{
    cParts* p;
    cParts* prev;

    p = (cParts*) getPartsPtr(no);
    if (!PTR_OK(p)) {
        if (no != 0) {
            pLog->err(0, 0, "releasePartsList() idx INVALID. %d", no);
        }
        return;
    }
    while (p) {
        cParts* dead = p;
        p = p->pList;
        pm->destroy(dead);
    }
    if (no == 0) {
        pParts = 0;
        nParts = 0;
        return;
    }
    prev = (cParts*) getPartsPtr(no - 1);
    prev->pList = 0;
}

void cModel::motionSet(void* data, int a, int b, int c, int d)
{
    MotionSetCore(this, &Motion, data, d, a, c, b);
}

int cModel::motionMove()
{
    return MotionMoveF(this, 0);
}

void cModel::motionPause()
{
    MotionPause(this);
}

void cModel::matUpdate()
{
    cCoord::matUpdate();
    if (pParts != NULL) {
        partsMatCalc();
        partsWorldCalc();
    }
    LightInfo.updateMatrix(this);
}

cParts::cParts()
{
}

// Bounding box of the original vertices (s16 * 2^-shift, 8 bytes each): centre and half size.
void getBoundingBox(ModelData* d, ModelBound* pBox)
{
    f32 maxZ = -65536.0f;
    f32 maxY = -65536.0f;
    f32 maxX = -65536.0f;
    f32 minZ = 65536.0f;
    f32 minY = 65536.0f;
    f32 minX = 65536.0f;
    u32 n = d->nVtx;
    u8 shift = d->shift;
    s16* v = (s16*) d->vtxOrig;
    u32 i;

    if (n != 0) {
        f32 sc = (f32) (1 << shift);
        i = n;
        do {
            f32 x = (f32) v[0] / sc;
            f32 y = (f32) v[1] / sc;
            f32 z = (f32) v[2] / sc;
            if (maxX < x) {
                maxX = x;
            }
            if (maxY < y) {
                maxY = y;
            }
            if (maxZ < z) {
                maxZ = z;
            }
            if (minX > x) {
                minX = x;
            }
            if (minY > y) {
                minY = y;
            }
            if (minZ > z) {
                minZ = z;
            }
            v += 4;
        } while (--i != 0);
    }
    pBox->size.x = (maxX - minX) * 0.5f;
    pBox->size.y = (maxY - minY) * 0.5f;
    pBox->size.z = (maxZ - minZ) * 0.5f;
    pBox->center.x = maxX - pBox->size.x;
    pBox->center.y = maxY - pBox->size.y;
    pBox->center.z = maxZ - pBox->size.z;
}

cPartsMgr::cPartsMgr() : cManager<cParts>(sizeof(cParts), 0)
{
    setName("cPartsMgr");
}

void cPartsMgr::log(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    pLog->vwarn(6, 0, fmt, ap);
}

int cPartsMgr::construct(cParts* p, u32 id)
{
    new (p) cParts;
    return 1;
}

// Work `no` of a parts manager, NULL when out of range.
static inline cParts* PartsMgrWork(cPartsMgr* m, u32 no)
{
    if (no >= m->nArray) {
        return 0;
    }
    return (cParts*) ((u8*) m->pArray + m->size * no);
}

cParts* cPartsMgr::createSequential(u32 n)
{
    u32 i;
    u32 j;
    u32 lim;

    // `lim` is recomputed in the loop test: the entry guard's `n + 1` and the hoisted loop copy
    // give the `addi r0; mr r7, r0` pair. A `lim = n + 1` before the loop folds them into one
    // register; `nArray - (n + 1)` is reassociated by fold to `(nArray - 1) - n`.
    for (i = 0; lim = n + 1, i < nArray - lim; i++) {
        int ok;

        if (PartsMgrWork(this, i)->be_flag & 0x601) {
            continue;
        }
        ok = 1;
        for (j = 0; j < n; j++) {
            if (PartsMgrWork(this, i + j)->be_flag & 0x601) {
                ok = 0;
            }
        }
        if (ok) {
            cParts* first;
            cParts* p;

            first = create(0, i);
            p = first;
            for (j = 1; j < n; j++) {
                cParts* np = create(0, i + j);
                p->pList = np;
                p = np;
            }
            return first;
        }
    }
    return 0;
}

cPartsMgr PartsMgr;

cModInfoMgr::cModInfoMgr() : cManager<cModelInfo>(sizeof(cModelInfo), 0)
{
    setName("cModInfoMgr");
}

void cModInfoMgr::log(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    pLog->vwarn(6, 0, fmt, ap);
}

int cModInfoMgr::construct(cModelInfo* p, u32 id)
{
    new (p) cModelInfo;
    return 1;
}

cModelInfo* cModInfoMgr::create(void* bin, void* tpl)
{
    cModelInfo* info = cManager<cModelInfo>::create();

    if (info != NULL) {
        ModelData* d = (ModelData*) bin;

        calcModelAddr(d);
        calcTplAddr((TEXPalette*) tpl);
        info->tpl_addr = tpl;
        info->pData = d;
        if (d->version != 0x20010801 && d->version != 0x20030818) {
            notBinData();
        }
        if (d->shapeOfs != 0) {
            info->be_flag |= 2;
        }
        getBoundingBox(info->pData, &info->bound);
    }
    return info;
}

cModInfoMgr ModInfoMgr;

cModel* GetPartsAddr(cModel* parts, int no)
{
    int cnt = no;

    if (!PTR_OK(parts)) {
        pLog->err(0, 0, "GetPartsAddr() PTR ERROR %08X", parts);
        return 0;
    }
    if (no--) {
        do {
            cModel* next = parts->pParts;
            if (!PTR_OK(next)) {
                pLog->err(0, 0, "GetPartsAddr() cParts NO ERROR %d", cnt);
                break;
            }
            parts = next;
        } while (no--);
    }
    return parts;
}

cModelInfo* GetModelInfoAddr(cModelInfo* info, int no)
{
    int cnt = no;

    if (!PTR_OK(info)) {
        pLog->err(0, 0, "GetModelInfoAddr() PTR ERROR %08X", info);
        return 0;
    }
    if (no--) {
        do {
            cModelInfo* next = info->pList;
            if (!PTR_OK(next)) {
                pLog->err(0, 0, "GetModelInfoAddr() cModelInfo NO ERROR %d", cnt);
                break;
            }
            info = next;
        } while (no--);
    }
    return info;
}

int GetModelInfoNum(cModelInfo* info)
{
    int n = 1;
    int i;

    if (!PTR_OK(info)) {
        pLog->err(0, 0, "GetModelInfoNum() PTR ERROR %08X", info);
        return 0;
    }
    for (i = 0; i < 100; i++) {
        cModelInfo* next = info->pList;
        if (!PTR_OK(next)) {
            return n;
        }
        info = next;
        n++;
    }
    pLog->err(0, 0, "GetModelInfoNum() PTR NUM 100 ?");
    return 0;
}

void ModelInfoRefrectOffAll(cModel* m)
{
    int n;
    int i;

    if (m == NULL) {
        pLog->err(0, 0, "ModelInfoRefrectOffAll() : failed!!");
        return;
    }
    n = GetModelInfoNum(m->pModelInfo);
    for (i = 0; i < n; i++) {
        cModelInfo* info = GetModelInfoAddr(m->pModelInfo, i);
        if (info) {
            info->be_flag |= 4;
        }
    }
}

void ModelInfoRefrectOn(cModel* m, int no)
{
    cModelInfo* info;

    if (m == NULL) {
        pLog->err(0, 0, "ModelInfoRefrectOn() : failed!!");
        return;
    }
    info = GetModelInfoAddr(m->pModelInfo, no);
    if (info) {
        info->be_flag &= ~4;
    }
}

void ModelInfoSetTrans(cModel* m, int no, int on)
{
    cModelInfo* info = GetModelInfoAddr(m->pModelInfo, no);

    if (info) {
        if (on == 1) {
            info->be_flag |= 8;
        } else {
            info->be_flag &= ~8;
        }
    }
}

static inline void VecSet(Vec* v, f32 x, f32 y, f32 z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

void drawBoundingBox(Mtx m, ModelBound* bound)
{
    static u8 ptbl[6][4] = {
        {0, 1, 3, 2}, {4, 5, 7, 6}, {0, 1, 5, 4}, {3, 2, 6, 7}, {1, 3, 7, 5}, {2, 0, 4, 6},
    };
    Vec v[8];
    Vec q[4];
    Vec* c;
    int i;
    u32 j;
    f32 sx = bound->size.x;
    f32 sy = bound->size.y;
    f32 sz = bound->size.z;

    c = v;
    VecSet(c, -sx, -sy, -sz);
    c++;
    VecSet(c, sx, -sy, -sz);
    c++;
    VecSet(c, -sx, -sy, sz);
    c++;
    VecSet(c, sx, -sy, sz);
    c++;
    VecSet(c, -sx, sy, -sz);
    c++;
    VecSet(c, sx, sy, -sz);
    c++;
    VecSet(c, -sx, sy, sz);
    c++;
    VecSet(c, sx, sy, sz);
    // A counted loop: loop.c's giv init for `&v[j]` is emitted in the preheader after gcse's
    // `&q[k]` insertions (LUID order decides the sched2 slot of `mr r30,r24`) and the eliminated biv
    // compares the stepped pointer against `&v[7]` (`cmplw; ble`), which the do-while form with an
    // explicit end pointer gave as well but with the copy issued before the addis.
    for (j = 0; j < 8; j++) {
        PSVECAdd(&v[j], &bound->center, &v[j]);
    }
    PSMTXMultVecArray(m, v, v, 8);
    for (i = 0; i < 6; i++) {
        q[0] = v[ptbl[i][0]];
        q[1] = v[ptbl[i][1]];
        q[2] = v[ptbl[i][2]];
        q[3] = v[ptbl[i][3]];
        Draw_line3d(&q[0], &q[1], 0x20FFFFFF, 0);
        Draw_line3d(&q[1], &q[2], 0x20FFFFFF, 0);
        Draw_line3d(&q[2], &q[3], 0x20FFFFFF, 0);
        Draw_line3d(&q[3], &q[0], 0x20FFFFFF, 0);
    }
}

// Too many lights on the model: flag it, draw a marker line and its bounding boxes in red.
void cModel::error()
{
    Vec v;

    if (pG->Debug_flg[3] & 0x400000) {
        be_flag |= 0x80000000;
        v.x = pos.x;
        v.y = pos.y + 50000.0f;
        v.z = pos.z;
        Draw_line3d(&pos, &v, 0xFFFFFFFF, 0);
        if (pModelInfo) {
            drawAllBoundingBox(pModelInfo);
            pModelInfo->color[0] = 0xFF;
            {
                cModelInfo* info = pModelInfo;
                info->color[2] = 0x40;
                info->color[1] = 0x40;
            }
        }
    }
}

