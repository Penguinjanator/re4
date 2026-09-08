#include "light.h"
#include "atari.h"
#include "gx.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"
#include "espgen.h"
#include "obj.h"
#include "scroll.h"
#include "rnd.h"
#include "main_sub.h"
#include "db_log.h"

// game/trans_lit.cpp
void commonEspLightSet(cLight** list, int n);

extern "C" {
void EspCommonTransShimmer(cEsp* esp, int type);
void EspCommonTransNega(cEsp* esp, int type);
int EspEstSetSelect(int a, int b, int c, cEsp** out, int d);
void GetPosXY(Vec* p0, Vec* p1, Vec* p2, Vec* p3, f32 u, f32 v, Vec* out);
void Esp1b_SpTrans(cEsp* esp);
}

#define DEG2RAD (3.14f / 180.0f)

// The pulled effect is kept in a one-member struct: the original reloads the pointer from its
// stack slot after every store through it (a struct-member slot aliases the member stores).
struct EspPtr {
    cEsp* p;
};

// TODO: EspCommonTrans / EspCommonTransShimmer / EspCommonTransNega (0xA18 / 0x1088 / 0xB7C)
void EspCommonTrans(cEsp* esp)
{
}

void EspCommonTransNega(cEsp* esp, int type)
{
}

void cEsp::move()
{
    pLog->err(0, 0, "ESP : ESP_ID[%x] move() invalid", id);
}

int cEsp::CommonMove()
{
    if (parent != pEffParentWorld && parentCnt != 0xFF && parentCnt <= cnt) {
        ApplyMatrix(parent->mat);
        parent = pEffParentWorld;
    }
    if (spdCnt == 0 || spdCnt <= cnt) {
        PSVECAdd(&pos, &spd, &pos);
        PSVECAdd(&spd, &acc, &spd);
        PSVECScale(&spd, &spd, spdScale);
    }
    if (scaleCnt == 0 || scaleCnt <= cnt) {
        scale += scaleSpd;
        scaleSpd *= scaleScale;
        if (!(scale >= 0.0f)) {
            PushEsp(this);
            return 0;
        }
    }
    PSVECAdd(&rot, &rotSpd, &rot);
    if (!ColorUpdate()) {
        return 0;
    }
    if (life != 0 && life <= cnt) {
        PushEsp(this);
        return 0;
    }
    cnt++;
    xB8 = SQRTF(sizeX * sizeX + sizeY * sizeY) * scale;
    return 1;
}

int cEsp::ColorUpdate()
{
    if (xA8 < cnt) {
        if (xA8 + xAA <= cnt) {
            colR *= colRSpd;
            colG *= colGSpd;
            colB *= colBSpd;
            colA *= colASpd;
            if (colR > 255.0f) {
                colR = 255.0f;
            }
            if (colG > 255.0f) {
                colG = 255.0f;
            }
            if (colB > 255.0f) {
                colB = 255.0f;
            }
            if (colA > 255.0f) {
                colA = 255.0f;
            }
            if (colA < 4.0f) {
                PushEsp(this);
                return 0;
            }
        }
    } else if (xA8 != 0) {
        f32 rate = (f32) cnt / (f32) xA8;
        if (blendType == 3) {
            colR = (f32) x80 * rate;
            colG = (f32) x81 * rate;
            colB = (f32) x82 * rate;
        }
        colA = (f32) x83 * rate;
    }
    return 1;
}

int cEsp::SetFreeWork(EspGenWork* gen, u32* seed)
{
    return 1;
}

// One texture animation step; returns 0 when the animation ended.
static inline int AnmStep(cEsp* esp, EspAnmData* anm, u8& ptn, u16& count, u8 spd)
{
    u32 time;

    if (anm->xC == 0) {
        time = 1;
    } else {
        time = anm->ptnTime[ptn];
    }
    count += spd;
    while ((count >> 5) > time) {
        ptn++;
        count -= time << 5;
        if (ptn >= anm->nPtn) {
            switch (anm->xB & 3) {
            case 0:
                return 0;
            case 1:
                ptn = 0;
                break;
            case 2:
                ptn = anm->x9 - 1;
                break;
            }
        }
    }
    return 1;
}

int cEsp::AnmMove()
{
    EspAnmData* anm;

    if (!EspGetAnmAddr(anmNo, &anm)) {
        pLog->err(0, 0, "ESP : TexId[%x] no data", anmNo);
        return 0;
    }
    if (!AnmStep(this, anm, anmPtn, anmCnt, anmSpd)) {
        return 0;
    }
    if (flags & 0x4000) {
        if (!EspGetAnmAddr(anmNo2, &anm)) {
            pLog->err(0, 0, "ESP : MaskTexId[%x] no data", anmNo);
            return 0;
        }
        if (!AnmStep(this, anm, anmPtn2, anmCnt2, anmSpd)) {
            return 0;
        }
    }
    return 1;
}

#line 1730 "D:/Bio4/Prog/esp_sub.cpp"
int cEsp::ChannelSetI()
{
    GXColor col;
    GXColor fin;
    Vec p;
    Vec dir;
    Vec d;

    if (flags & 0x40) {
        GXSetTevOp(0, 0);
        GXSetTevColorIn(0, 0xF, 8, 0xA, 0xF);
        GXSetTevColorOp(0, 0, 0, 2, 1, 0);
        commonEspLightSet(g_pEspSys->lightList.p, g_pEspSys->lightList.num);
    } else {
        GXSetTevOp(0, 0);
        if (flags & 0x80) {
            GXSetTevColorOp(0, 0, 0, 2, 1, 0);
        }
        if (flags & 0x20000) {
            GXSetTevAlphaOp(0, 0, 0, 2, 1, 0);
        }
        GXSetNumChans(1);
        GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    }
    if (dispFlag & 1) {
        col.r = (u8) (colR * colA * (1.0f / 255.0f));
        col.g = (u8) (colG * colA * (1.0f / 255.0f));
        col.b = (u8) (colB * colA * (1.0f / 255.0f));
        col.a = 0xFF;
    } else {
        col.r = (u8) colR;
        col.g = (u8) colG;
        col.b = (u8) colB;
        col.a = (u8) colA;
    }
    if (!(dispFlag & 4) && EffIsSetFinalCol()) {
        EffGetFinalCol(&fin);
        col.r = col.r * fin.r >> 8;
        col.g = col.g * fin.g >> 8;
        col.b = col.b * fin.b >> 8;
        col.a = col.a * fin.a >> 8;
    }
    if (x16 != 0) {
        Camera* cam;
        f32 dot;

        if (parent != pEffParentWorld) {
            PSMTXMultVec(parent->mat, &pos, &p);
        } else {
            p = pos;
        }
        cam = &pG->Cam;
        dir.x = cam->param.at.x - cam->param.pos.x;
        dir.y = cam->param.at.y - cam->param.pos.y;
        dir.z = cam->param.at.z - cam->param.pos.z;
#line 1798 "D:/Bio4/Prog/esp_sub.cpp"
        VECNormalize(&dir, &dir);
        d.x = p.x - cam->param.pos.x;
        d.y = p.y - cam->param.pos.y;
        d.z = p.z - cam->param.pos.z;
        dot = PSVECDotProduct(&dir, &d);
        if (dot < (f32) x16 * 10.0f) {
            f32 rate = 1.0f - ((f32) x16 * 10.0f - dot) / ((f32) (x16 - x14) * 10.0f);
            if (dispFlag & 1) {
                col.r = (u8) ((f32) col.r * rate);
                col.g = (u8) ((f32) col.g * rate);
                col.b = (u8) ((f32) col.b * rate);
            } else {
                col.a = (u8) ((f32) col.a * rate);
            }
        }
    }
    GXSetChanMatColor(4, col);
    return col.a != 0;
}

void cEsp::ApplyMatrix(Mtx m)
{
    Mtx r;

    PSMTXMultVec(m, &pos, &pos);
    PSMTXMultVecSR(m, &spd, &spd);
    PSMTXMultVecSR(m, &acc, &acc);
    if (flags & 1) {
        low_RotMatrix(r, &rot);
        PSMTXConcat(m, r, r);
        Matrix2AxisAngle(r, &rot);
    }
}

void cEsp::CommonStateSet()
{
    GXSetCullMode(0);
    GXSetAlphaCompare(4, 1, 1, 4, 1);
    GXSetZMode(1, 3, 0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(0, 1, 4, 0x1E);
}

cEsp::cEsp()
{
}

cEsp::~cEsp()
{
}

void cEsp::Destruct()
{
}

int EspEstSetSelect(int a, int b, int c, cEsp** out, int d)
{
    EspSeqData* head;
    EspGenWork* rec;
    EspInfo info;
    Mtx m;
    u32 seed = 0x12345678;
    u8 type;

    head = EspGetEstAddr(a, b, 0);
    if (head == 0) {
        pLog->err(0, 0, "EspEstSetSelect:[%x/0x%02x] OWNER Invalid", a, b);
        return 0;
    }
    if ((u32) c >= head->num) {
        pLog->err(0, 0, "EspEstSetSelect() : invalid no[%d] MAX=%d", c, head->num);
        return 0;
    }
    rec = &head->rec[c];
    type = rec->type;
    if (type == 0) {
        PSMTXIdentity(m);
        memclr_asm(&info, sizeof(info));
        if (d == 1) {
            info.x0 |= 1;
        }
        return EspSeqSet(rec, &info, &seed, 0, &m, 0, out, 0, 0, 0.0f) == 1;
    }
    if (type == 1) {
        pLog->err(0, 0, "EspEstSetSelect : can't call Espgen.");
        return 0;
    }
    pLog->err(0, 0, "EspEstSetSelect : KIND[%d] is invalid.", type);
    return 0;
}

int EspSeqSet(EspGenWork* rec, EspInfo* info, u32* seed, cModel* model, Mtx* mtx, int a, cEsp** out, EspSeqOpt* p8,
              Vec* pos, f32 f)
{
    static int bl[6][4] = {
        {1, 4, 5, 0}, {1, 4, 1, 0}, {1, 1, 1, 0}, {1, 2, 1, 0}, {1, 2, 0, 0}, {1, 4, 3, 0},
    };
    EspPtr e;
    Vec v;
    Mtx m;
    Mtx m2;
    Vec r;
    f32 rnd;
    int ret;
    cModel* parts;

    if ((u8) (rec->x1 + 4) <= 3) {
        EfmSeqSet(rec, (EfmCore*) info, seed, model, *mtx, 0, 0.0f, pos);
        *out = EspGetDmyPtr();
        return 1;
    }
    if (rec->x1 == 0x3F) {
        pLog->err(0, 0, "ESP : ESP_ID[%x] is invalid.", rec->x1);
        *out = EspGetDmyPtr();
        return 0;
    }
    if (rec->x1 != 3 && rec->x1 != 9 && rec->x1 != 0xC && !EspChkTexId(rec->x2)) {
        pLog->err(0, 0, "ESP : TEX_ID[%x] not initialized.", rec->x2);
        *out = EspGetDmyPtr();
        return 0;
    }
    if (PullEsp(&e.p, rec->x1) == 0) {
        ret = 0;
    } else {
        e.p->info = *info;
        e.p->id = rec->x1;
        e.p->anmNo = rec->x2;
        e.p->xF = rec->x3;
        e.p->partsNo = rec->x7;
        if (!(info->x0 & 0x1000) && rec->x6 != 0) {
            model = SmdGetObjPtr(rec->x6 - 1);
            if (model == 0) {
                pLog->err(0, 0, "ESP : PARENT_NO[%d] Invalid.", rec->x6);
                PushEsp(e.p);
                *out = EspGetDmyPtr();
                return 0;
            }
        }
        e.p->flags = rec->flags;
        if (e.p->flags & 8) {
            if (fRandSeed1_1(seed) > 0.0f) {
                e.p->flags |= 2;
            } else {
                e.p->flags &= ~2;
            }
        }
        if (e.p->flags & 0x10) {
            if (fRandSeed1_1(seed) > 0.0f) {
                e.p->flags |= 4;
            } else {
                e.p->flags &= ~4;
            }
        }
        e.p->pos = rec->pos;
        e.p->pos.x += rec->x18 * fRandSeed1_1(seed);
        e.p->pos.y += rec->x1C * fRandSeed1_1(seed);
        e.p->pos.z += rec->x20 * fRandSeed1_1(seed);
        e.p->spd = rec->x24;
        e.p->spd.x += rec->x34.x * fRandSeed1_1(seed);
        e.p->spd.y += rec->x34.y * fRandSeed1_1(seed);
        e.p->spd.z += rec->x34.z * fRandSeed1_1(seed);
        if (a) {
            r.x = 0.0f;
            r.y = f;
            r.z = 0.0f;
            RotMatrixZXY(m, &r);
            PSMTXMultVecSR(m, &e.p->spd, &e.p->spd);
        }
        e.p->spdScale = rec->x30;
        e.p->acc = rec->x40;
        e.p->acc.x += rec->x4C.x * fRandSeed1_1(seed);
        e.p->acc.y += rec->x4C.y * fRandSeed1_1(seed);
        e.p->acc.z += rec->x4C.z * fRandSeed1_1(seed);
        e.p->rot = rec->x58;
        e.p->rot.x += rec->x64.x * fRandSeed1_1(seed);
        e.p->rot.y += rec->x64.y * fRandSeed1_1(seed);
        e.p->rot.z += rec->x64.z * fRandSeed1_1(seed);
        PSVECScale(&e.p->rot, &e.p->rot, DEG2RAD);
        e.p->rotSpd = rec->x70;
        e.p->rotSpd.x += rec->x7C.x * fRandSeed1_1(seed);
        e.p->rotSpd.y += rec->x7C.y * fRandSeed1_1(seed);
        e.p->rotSpd.z += rec->x7C.z * fRandSeed1_1(seed);
        PSVECScale(&e.p->rotSpd, &e.p->rotSpd, DEG2RAD);
        e.p->sizeX = rec->x88;
        e.p->sizeY = rec->x8C;
        e.p->scale = 1.0f;
        rnd = rec->x90 * fRandSeed1_1(seed);
        e.p->sizeX += rnd;
        e.p->sizeY += rnd;
        e.p->scaleSpd = rec->x94;
        e.p->scaleScale = rec->x98;
        e.p->x80 = rec->x9C;
        e.p->x81 = rec->x9D;
        e.p->x82 = rec->x9E;
        e.p->x83 = rec->x9F;
        e.p->colR = (f32) rec->x9C;
        e.p->colG = (f32) rec->x9D;
        e.p->colB = (f32) rec->x9E;
        e.p->colA = (f32) rec->x9F;
        e.p->colRSpd = rec->xA0;
        e.p->colGSpd = rec->xA4;
        e.p->colBSpd = rec->xA8;
        e.p->colASpd = rec->xAC;
        if (rec->xC2 > 5) {
            pLog->err(0, 0, "ESP : BLEND_TYPE[%d] Invalid.", rec->xC2);
            PushEsp(e.p);
            *out = EspGetDmyPtr();
            return 0;
        }
        e.p->blendType = rec->xC2;
        e.p->xA4 = bl[rec->xC2][0];
        e.p->xA5 = bl[rec->xC2][1];
        e.p->xA6 = bl[rec->xC2][2];
        e.p->xA7 = bl[rec->xC2][3];
        if (rec->xC2 == 4) {
            e.p->dispFlag |= 1;
        }
        e.p->xA8 = rec->xB0;
        e.p->xAA = rec->xB2;
        e.p->spdCnt = rec->xB4;
        e.p->scaleCnt = rec->xB6;
        e.p->life = rec->xB8;
        e.p->cnt = rec->xBA;
        e.p->anmPtn = rec->xBC;
        e.p->anmSpd = rec->xBD + 0x20;
        e.p->anmCnt = rec->xBE;
        e.p->parentCnt = rec->xC0;
        e.p->xEC = rec->xC3;
        e.p->xED = rec->xC4;
        e.p->anmNo2 = rec->xC5;
        e.p->x16 = rec->xC6 * 10;
        e.p->x14 = rec->xC7 * 10;
        if (e.p->xEC != 0) {
            e.p->dispFlag |= 4;
        }
        switch (e.p->partsNo) {
        case 0xFF:
            e.p->parent = pEffParentWorld;
            e.p->ApplyMatrix(*mtx);
            break;
        case 0xF8:
        case 0xF9:
        case 0xFA:
        case 0xFB:
        case 0xFC:
        case 0xFD:
            e.p->parent = pEffParentWorld;
            e.p->pos.x += (*mtx)[0][3];
            e.p->pos.y += (*mtx)[1][3];
            e.p->pos.z += (*mtx)[2][3];
            break;
        case 0xFE:
            e.p->parent = pEffParentWorld;
            if (rec->xC0 != 0) {
                pLog->warn(0, 0, "ESP:ReleaseTime not 0 but no parent.");
            }
            break;
        default:
            if (model == 0) {
                pLog->err(0, 0, "ESP : PARTS_NO[%d] but Not on parts.", e.p->partsNo);
                PushEsp(e.p);
                *out = EspGetDmyPtr();
                return 0;
            }
            if (e.p->partsNo < model->nParts) {
                if (e.p->flags & 0x20) {
                    parts = model->getPartsPtr(e.p->partsNo);
                    PSMTXIdentity(m2);
                    low_RotMatrix(m2, &model->rot);
                    PSMTXMultVecSR(m2, &rec->pos, &v);
                    m2[0][3] = parts->mat[0][3] + v.x;
                    m2[1][3] = parts->mat[1][3] + v.y;
                    m2[2][3] = parts->mat[2][3] + v.z;
                    e.p->parent = pEffParentWorld;
                    e.p->pos.x = 0.0f;
                    e.p->pos.y = 0.0f;
                    e.p->pos.z = 0.0f;
                    e.p->ApplyMatrix(m2);
                    e.p->pos.x += rec->x18 * fRandSeed1_1(seed);
                    e.p->pos.y += rec->x1C * fRandSeed1_1(seed);
                    e.p->pos.z += rec->x20 * fRandSeed1_1(seed);
                } else {
                    e.p->pModel = model;
                    e.p->x20 = model->serial;
                    e.p->parent = model->getPartsPtr(e.p->partsNo);
                    if (pos) {
                        PSVECAdd(&e.p->pos, pos, &e.p->pos);
                    }
                }
            } else {
                pLog->err(0, 0, "ESP : PARTS_NO[%d] is invalid(MAX:%d).", e.p->partsNo, model->nParts);
                PushEsp(e.p);
                *out = EspGetDmyPtr();
                return 0;
            }
            break;
        }
        ret = e.p->SetFreeWork(rec, seed);
        if (p8) {
            if (p8->set & 1) {
                e.p->spd = p8->spd;
                e.p->spd.x += rec->x34.x * fRandSeed1_1(seed);
                e.p->spd.y += rec->x34.y * fRandSeed1_1(seed);
                e.p->spd.z += rec->x34.z * fRandSeed1_1(seed);
            }
            if (p8->set & 2) {
                e.p->sizeX = p8->sizeX;
                e.p->sizeY = p8->sizeY;
            }
            if (p8->set & 4) {
                e.p->x80 = p8->r;
                e.p->x81 = p8->g;
                e.p->x82 = p8->b;
                e.p->x83 = p8->a;
                e.p->colR = (f32) p8->r;
                e.p->colG = (f32) p8->g;
                e.p->colB = (f32) p8->b;
                e.p->colA = (f32) p8->a;
            }
            if (p8->mul & 1) {
                e.p->spd.x *= p8->spd.x;
                e.p->spd.y *= p8->spd.y;
                e.p->spd.z *= p8->spd.z;
            }
            if (p8->mul & 2) {
                e.p->sizeX *= p8->sizeX;
                e.p->sizeY *= p8->sizeY;
            }
            if (p8->mul & 4) {
                f32 c;
                c = (f32) e.p->x80 * (f32) (int) p8->r * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x80 = (u8) c;
                c = (f32) e.p->x81 * (f32) (int) p8->g * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x81 = (u8) c;
                c = (f32) e.p->x82 * (f32) (int) p8->b * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x82 = (u8) c;
                c = (f32) e.p->x83 * (f32) (int) p8->a * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x83 = (u8) c;
                e.p->colR = (f32) e.p->x80;
                e.p->colG = (f32) e.p->x81;
                e.p->colB = (f32) e.p->x82;
                e.p->colA = (f32) e.p->x83;
            }
            if (p8->add & 1) {
                e.p->spd.x += p8->spd.x;
                e.p->spd.y += p8->spd.y;
                e.p->spd.z += p8->spd.z;
            }
            if (p8->add & 2) {
                e.p->sizeX += p8->sizeX;
                e.p->sizeY += p8->sizeY;
            }
            if (p8->add & 4) {
                f32 c;
                c = (f32) e.p->x80 + (f32) (int) p8->r * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x80 = (u8) c;
                c = (f32) e.p->x81 + (f32) (int) p8->g * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x81 = (u8) c;
                c = (f32) e.p->x82 + (f32) (int) p8->b * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x82 = (u8) c;
                c = (f32) e.p->x83 + (f32) (int) p8->a * (1.0f / 255.0f);
                if (c > 255.0f) {
                    c = 255.0f;
                }
                if (c < 0.0f) {
                    c = 0.0f;
                }
                e.p->x83 = (u8) c;
                e.p->colR = (f32) e.p->x80;
                e.p->colG = (f32) e.p->x81;
                e.p->colB = (f32) e.p->x82;
                e.p->colA = (f32) e.p->x83;
            }
        }
    }
    if (ret == 0) {
        *out = EspGetDmyPtr();
        if (e.p->flag & 1) {
            PushEsp(e.p);
        }
        return 0;
    }
    *out = e.p;
    return ret;
}

// Bilinear point of the quad p0 p1 p2 p3 at (u, v).
void GetPosXY(Vec* p0, Vec* p1, Vec* p2, Vec* p3, f32 u, f32 v, Vec* out)
{
    static Vec v0;
    static Vec v1;
    static Vec v2;

    PSVECSubtract(p1, p0, &v0);
    PSVECSubtract(p2, p3, &v1);
    PSVECScale(&v0, &v0, u);
    PSVECScale(&v1, &v1, u);
    PSVECAdd(p0, &v0, &v0);
    PSVECAdd(p3, &v1, &v1);
    PSVECSubtract(&v1, &v0, &v2);
    PSVECScale(&v2, &v2, v);
    PSVECAdd(&v0, &v2, out);
}

// esp1b work at cEsp+0xF8: the quad is subdivided div x div times.
struct Esp1bSpWork {
    u32 div;   // 0x00
    Vec p1;    // 0x04 corner offsets from the unit quad (0,1,1) (1,1,1) (1,0,1) (0,0,1)
    Vec p2;    // 0x10
    Vec p3;    // 0x1C
};

void Esp1b_SpTrans(cEsp* esp)
{
    static Vec p0;
    static Vec p1;
    static Vec p2;
    static Vec p3;
    static Vec ret;
    Esp1bSpWork* w = (Esp1bSpWork*) &esp->pad_EC[0xC];
    u32 div;
    u32 i;
    u32 j;
    f32 step;
    f32 u;
    f32 u1;
    f32 v;
    f32 v1;
    f32 tu;
    f32 tu1;
    f32 tv;
    f32 tv1;

    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xA, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xA, 0, 1, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    div = w->div;
    p0.x = 0.0f;
    p0.z = 1.0f;
    p0.y = 1.0f;
    p1.x = w->p1.x + 1.0f;
    p1.y = w->p1.y + 1.0f;
    p1.z = w->p1.z + 1.0f;
    p2.x = w->p2.x + 1.0f;
    p2.y = w->p2.y + 0.0f;
    p2.z = w->p2.z + 1.0f;
    p3.x = w->p3.x + 0.0f;
    p3.y = w->p3.y + 0.0f;
    p3.z = w->p3.z + 1.0f;
    GXBegin(0x80, 0, div * div * 4);
    step = 1.0f / (f32) div;
    v = 0.0f;
    tv = 0.0f;
    for (i = 0; i < div; i++) {
        v1 = v + step;
        tv1 = tv + step;
        u = 0.0f;
        tu = 0.0f;
        for (j = 0; j < div; j++) {
            u1 = u + step;
            tu1 = tu + step;
            GetPosXY(&p0, &p1, &p2, &p3, u, v, &ret);
            GXPosition3f32(ret.x, ret.y, ret.z);
            GXNormal3s8(0, 1, 0);
            GXTexCoord2f32(tu, tv);
            GetPosXY(&p0, &p1, &p2, &p3, u1, v, &ret);
            GXPosition3f32(ret.x, ret.y, ret.z);
            GXNormal3s8(0, 1, 0);
            GXTexCoord2f32(tu1, tv);
            GetPosXY(&p0, &p1, &p2, &p3, u1, v1, &ret);
            GXPosition3f32(ret.x, ret.y, ret.z);
            GXNormal3s8(0, 1, 0);
            GXTexCoord2f32(tu1, tv1);
            GetPosXY(&p0, &p1, &p2, &p3, u, v1, &ret);
            GXPosition3f32(ret.x, ret.y, ret.z);
            GXNormal3s8(0, 1, 0);
            GXTexCoord2f32(tu, tv1);
            u = u1;
            tu = tu1;
        }
        v = v1;
        tv = tv1;
    }
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xA, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 1, 1, 0);
    GXSetVtxAttrFmt(0, 0xA, 0, 1, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 1, 0);
}
