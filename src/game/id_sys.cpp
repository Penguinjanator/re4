#include "light.h"
#include "id_sys.h"
#include "global.h"
#include "main_mem.h"
#include "main_sub.h"
#include "db_log.h"
#include "camera.h"
#include "view.h"
#include "math_sub.h"
#include "texture.h"
#include "trans_ot.h"

extern "C" {
double tan(double);
double strtod(const char*, char**);
void OSReport(const char* msg, ...);
// game/path.cpp
int FuncPathParametrize(void* path, void* data);
int FuncPathCalc(void* path, void* data, Vec* out, f32 t);
}

extern GXTexObj g_Get_tex_obj;  // game/trans.cpp

Mtx IDSystem::m_scrn_mat;

// Bit tables indexed by table type (ck / disp).
#define ID_BIT_WORD(tbl, n) (*(u32*) (((n) >> 5 << 2) + (u32) (tbl)))
static inline u32 IdBitGet(u32* tbl, u8 n) { return ID_BIT_WORD(tbl, n) & (0x80000000 >> (n & 0x1F)); }
static inline int IdBitChk(u32* tbl, u8 n) { return IdBitGet(tbl, n) ? 1 : 0; }
static inline void IdBitOn(u32* tbl, u8 n) { ID_BIT_WORD(tbl, n) |= 0x80000000 >> (n & 0x1F); }
static inline void IdBitOff(u32* tbl, u8 n) { ID_BIT_WORD(tbl, n) &= ~(0x80000000 >> (n & 0x1F)); }
#define ID_UNIT(i) ((IdUnit*) ((i) * sizeof(IdUnit) + (u32) pUnit))

void IDSystem::gameInit(int n)
{
#line 66 "D:/Bio4/Prog/id_sys.cpp"
    pUnit = (IdUnit*) MEM_ALLOC(n * sizeof(IdUnit), 1, 0xD);
    num = n;
    if (pUnit == 0) {
        num = 0;
        if (n != 0) {
            pLog->err(0, 0, "IDSystem::gameInit() malloc failed");
        }
    }
    roomInit();
}

void IDSystem::roomInit()
{
    int i;

    active = 0;
    for (i = 0; i < num; i++) {
        memclr_asm(&pUnit[i], sizeof(IdUnit));
        pUnit[i].flags = 0xFF;
    }
    memclr_asm(disp, sizeof(disp));
    memclr_asm(ck, sizeof(ck));
}

void IDSystem::free()
{
    Mem_free(pUnit);
    pUnit = 0;
    num = 0;
}

int IDSystem::setCk(u8 type)
{
    u8 no = type;
    return IdBitChk(ck, no);
}

void IDSystem::dispSw(u8 type, int sw)
{
    switch (sw) {
    case 1:
        IdBitOff(disp, type);
        break;
    case 0:
        IdBitOn(disp, type);
        break;
    }
}

void IDSystem::unitPush(IdUnit* u)
{
    int i;

    if (u->flags == 0xFF) {
        return;
    }
    if (u->kind == 1) {
        for (i = 0; i < num; i++) {
            IdUnit* c = ID_UNIT(i);
            if (c->flags != 0xFF && u == c->parent) {
                unitPush(c);
            }
        }
    }
    if (u->flags & 0x10) {
        pLog->err(0, 0, "unitPush(0x%p):[%02x,%02x] ID_UNIT wait for being Drawn.", u, u->type, u->unitNo);
    }
    u->flags = 0xFF;
    active--;
}

IdUnit* IDSystem::unitPull()
{
    int i;
    IdUnit* u = pUnit;

    for (i = 0; i < num; i++, u++) {
        if (u->flags == 0xFF) {
            memclr_asm(u, sizeof(IdUnit));
            u->flags = 0xD;
            u->u0 = 0.0f;
            u->u1 = 1.0f;
            u->v0 = 0.0f;
            u->v1 = 1.0f;
            active++;
            return u;
        }
    }
    return 0;
}

void IDSystem::unitLevel(IdUnit* u, u8 level)
{
    int i;

    if (u->kind == 1) {
        for (i = 0; i < num; i++) {
            IdUnit* c = ID_UNIT(i);
            if (c->flags != 0xFF && u == c->parent) {
                unitLevel(c, level + 1);
            }
        }
    }
    if (level > maxLevel) {
        maxLevel = level;
    }
    u->level = level;
}

void IDSystem::unitParent(IdUnit* parent, IdUnit* child)
{
    child->parent = parent;
    unitLevel(child, parent->level + 1);
}

IdUnit* IDSystem::unitPtr(u8 id, u8 type)
{
    static IdUnit tmpId;
    int i;
    IdUnit* u = pUnit;

    for (i = 0; i < num; i++, u++) {
        if (u->flags != 0xFF && u->id == id && u->type == type) {
            return u;
        }
    }
    pLog->err(0, 0, "IDSystem::unitPtr(m[%02x],c[%02x]): Not found.", id, type);
    return &tmpId;
}

static int cmp_id_no(IdData2* d, u8 id, int mode)
{
    u8 no;

    switch (mode) {
    case 0:
        no = d->id;
        break;
    case 1:
        no = d->parentNo;
        break;
    default:
        no = d->id;
        break;
    }
    return no == id;
}

void IDSystem::set(void* data, u8 id, u8 type, u8 ot, u8 prio, u8 mode)
{
    IdDataHeader* hdr = (IdDataHeader*) data;
    u8* p = (u8*) data + 8;
    int ver;
    int sysVer;
    int i;
    int j;
    IdUnit* u;

    setCk(type);
    IdBitOn(ck, type);

    ver = (int) (f32) strtod((char*) data, 0);
    sysVer = (int) (f32) strtod("2.00", 0);
    if (ver < sysVer) {
        OSReport("IDSystem::set(): Dat ver.%d < Sys ver.%d\n", ver, sysVer);
    }

    for (i = 0; i < hdr->num; i++) {
        switch (ver) {
        case 1: {
            IdData* d = (IdData*) p;
            p += sizeof(IdData);
            if (id != 0xFF && d->id != id) {
                break;
            }
            u = unitPull();
            if (u == 0) {
                pLog->err(0, 0, "IDSystem::set() work full (0x%02x miss)", hdr->num - i);
                break;
            }
            u->flags = d->flags;
            u->id = d->id;
            u->unitNo = d->no;
            u->level = d->level;
            u->parentNo = d->parentNo;
            u->x7 = d->x8;
            u->kind = d->kind;
            u->texId = d->texId;
            u->vtxType = d->vtxType;
            u->loop = d->loop;
            u->scaleType = d->scaleType;
            u->rotAxis = d->rotAxis;
            u->dir = d->dir;
            u->scr = d->pos;
            u->vtx[0] = d->vtx[0];
            u->vtx[1] = d->vtx[1];
            u->vtx[2] = d->vtx[2];
            u->vtx[3] = d->vtx[3];
            u->sizeX = d->sizeX;
            u->sizeY = d->sizeY;
            u->col0[0] = d->col0[0];
            u->col0[1] = d->col0[1];
            u->col0[2] = d->col0[2];
            u->col0[3] = d->col0[3];
            u->col1[0] = 0;
            u->col1[1] = 0;
            u->col1[2] = 0;
            u->col1[3] = 0;
            u->rot = d->rot;
            u->blendType = d->blendType;
            u->transType = d->transType;
            u->maskId = d->maskId;
            u->flags_7F = d->flags_7F;
            u->transSub = d->transSub;
            u->path0 = d->ofs[0] ? (u8*) data + d->ofs[0] : 0;
            u->path1 = d->ofs[1] ? (u8*) data + d->ofs[1] : 0;
            u->curve[0] = (Hermite1*) (d->ofs[2] ? (u8*) data + d->ofs[2] : 0);
            u->curve[1] = (Hermite1*) (d->ofs[3] ? (u8*) data + d->ofs[3] : 0);
            u->curve[2] = (Hermite1*) (d->ofs[4] ? (u8*) data + d->ofs[4] : 0);
            u->curve[3] = (Hermite1*) (d->ofs[5] ? (u8*) data + d->ofs[5] : 0);
            if ((s32) pG->flags_60 >= 0) {
                u->flags |= 0xD;
            }
            u->flags |= 0x2;
            u->parent = 0;
            if (d->parentNo != 0xFF) {
                for (j = 0; j < num; j++) {
                    IdUnit* c = &pUnit[j];
                    if (c->flags != 0xFF && (c->flags & 0x2) && c->unitNo == d->parentNo) {
                        u->parent = c;
                        break;
                    }
                }
            }
            if (u->path0 != 0) {
                if (FuncPathParametrize(u->path0, u->path1) == 0) {
                    pLog->err(0, 0, "IDSystem::set():[%02x,%02x] Path parametrization error!", u->type, u->unitNo);
                    u->path0 = 0;
                }
            }
            u->type = type;
            u->ot = ot;
            u->prio = prio;
            if (d->level > maxLevel) {
                maxLevel = d->level;
            }
            break;
        }
        case 2: {
            IdData2* d = (IdData2*) p;
            p += sizeof(IdData2);
            if (id != 0xFF) {
                if (cmp_id_no(d, id, mode) == 0) {
                    break;
                }
            }
            u = unitPull();
            if (u == 0) {
                pLog->err(0, 0, "IDSystem::set() work full (0x%02x miss)", hdr->num - i);
                break;
            }
            u->flags = d->flags;
            u->id = d->id;
            u->unitNo = d->no;
            u->level = d->level;
            u->parentNo = d->parentNo;
            u->x7 = d->x8;
            u->kind = d->kind;
            u->texId = d->texId;
            u->vtxType = d->vtxType;
            u->loop = d->loop;
            u->scaleType = d->scaleType;
            u->rotAxis = d->rotAxis;
            u->dir = d->dir;
            u->scr = d->pos;
            u->vtx[0] = d->vtx[0];
            u->vtx[1] = d->vtx[1];
            u->vtx[2] = d->vtx[2];
            u->vtx[3] = d->vtx[3];
            u->sizeX = d->sizeX;
            u->sizeY = d->sizeY;
            u->col0[0] = d->col0[0];
            u->col0[1] = d->col0[1];
            u->col0[2] = d->col0[2];
            u->col0[3] = d->col0[3];
            u->col1[0] = d->col1[0];
            u->col1[1] = d->col1[1];
            u->col1[2] = d->col1[2];
            u->col1[3] = d->col1[3];
            u->rot = d->rot;
            u->blendType = d->blendType;
            u->transType = d->transType;
            u->maskId = d->maskId;
            u->flags_7F = d->flags_7F;
            u->transSub = d->transSub;
            u->path0 = d->ofs[0] ? (u8*) data + d->ofs[0] : 0;
            u->path1 = d->ofs[1] ? (u8*) data + d->ofs[1] : 0;
            u->curve[0] = (Hermite1*) (d->ofs[2] ? (u8*) data + d->ofs[2] : 0);
            u->curve[1] = (Hermite1*) (d->ofs[3] ? (u8*) data + d->ofs[3] : 0);
            u->curve[2] = (Hermite1*) (d->ofs[4] ? (u8*) data + d->ofs[4] : 0);
            u->curve[3] = (Hermite1*) (d->ofs[5] ? (u8*) data + d->ofs[5] : 0);
            if ((s32) pG->flags_60 >= 0) {
                u->flags |= 0xD;
            }
            u->flags |= 0x2;
            u->parent = 0;
            if (d->parentNo != 0xFF) {
                for (j = 0; j < num; j++) {
                    IdUnit* c = &pUnit[j];
                    if (c->flags != 0xFF && (c->flags & 0x2) && c->unitNo == d->parentNo) {
                        u->parent = c;
                        break;
                    }
                }
            }
            if (u->path0 != 0) {
                if (FuncPathParametrize(u->path0, u->path1) == 0) {
                    pLog->err(0, 0, "IDSystem::set():[%02x,%02x] Path parametrization error!", u->type, u->unitNo);
                    u->path0 = 0;
                }
            }
            u->type = type;
            u->ot = ot;
            u->prio = prio;
            if (d->level > maxLevel) {
                maxLevel = d->level;
            }
            if (id != 0xFF) {
                set(data, d->no, type, ot, prio, 1);
            }
            break;
        }
        }
    }

    if (mode != 1) {
        for (i = 0; i < num; i++) {
            IdUnit* c = &pUnit[i];
            if (c->flags != 0xFF && (c->flags & 0x2)) {
                c->flags &= ~0x2;
            }
        }
    }
}

void IDSystem::kill(u8 id, u8 type)
{
    int i;
    IdUnit* u = pUnit;

    for (i = 0; i < num; i++, u++) {
        if (u->flags == 0xFF) {
            continue;
        }
        if (type == 0xFF) {
            unitPush(u);
        } else if (u->type == type) {
            if (id == 0xFF) {
                unitPush(u);
            } else if (u->id == id) {
                unitPush(u);
            }
        }
    }
    IdBitOff(ck, type);
}

void IDSystem::stop()
{
    int i;
    IdUnit* u = pUnit;

    for (i = 0; i < num; i++, u++) {
        if (!(u->flags & 0x1)) {
            continue;
        }
        {
            if (u->dir & 0x1) {
                u->timer[0]++;
            } else {
                u->timer[0]--;
            }
            if (u->dir & 0x2) {
                u->timer[1]++;
            } else {
                u->timer[1]--;
            }
            if (u->dir & 0x4) {
                u->timer[2]++;
            } else {
                u->timer[2]--;
            }
            if (u->dir & 0x8) {
                u->timer[3]++;
            } else {
                u->timer[3]--;
            }
        }
    }
}

void IDSystem::move()
{
    int lv;
    int i;

    if (pG->flags_170 & 0x40) {
        return;
    }
    Vec v = { 0.0f, 0.0f, 1.0f };
    f32 dist = 240.0 / tan(pG->Cam.param.fovy * 0.5f * (PI / 180.0f));
    PSMTXIdentity(m_scrn_mat);
    PSVECScale(&v, &v, -dist);
    m_scrn_mat[0][3] = v.x;
    m_scrn_mat[1][3] = v.y;
    m_scrn_mat[2][3] = v.z;

    for (lv = 0; lv <= maxLevel; lv++) {
        IdUnit* u = pUnit;
        for (i = 0; i < num; i++, u++) {
            if (u->flags == 0xFF || !(u->flags & 0x1)) {
                continue;
            }
            if ((u->flags & 0x4) && lv == u->level) {
                idSysMove00(u);
                idSysMove01(u);
                idSysMove02(u);
                idSysMove03(u);
                idSysMove04(u);
            }
        }
    }
}

void IDSystem::beMove(IdUnit* u, int sw)
{
    int i;
    IdUnit* c = pUnit;

    for (i = 0; i < num; i++, c++) {
        if (c->flags == 0xFF || !(c->flags & 0x1)) {
            continue;
        }
        if (u == c->parent) {
            beMove(c, sw);
        }
    }
    switch (sw) {
    case 1:
        u->flags |= 0x4;
        break;
    case 0:
        u->flags &= ~0x4;
        break;
    }
}

void IDSystem::setTime(IdUnit* u, u16 time)
{
    int i;
    IdUnit* c = pUnit;

    for (i = 0; i < num; i++, c++) {
        if (c->flags == 0xFF || !(c->flags & 0x1)) {
            continue;
        }
        if (u == c->parent) {
            setTime(c, time);
        }
    }
    u->timer[3] = time;
    u->timer[2] = time;
    u->timer[1] = time;
    u->timer[0] = time;
}

void IDSystem::movePos(IdUnit* u)
{
    int i;
    IdUnit* c;

    idSysMove00(u);
    c = pUnit;
    for (i = 0; i < num; i++, c++) {
        if (c->flags == 0xFF || !(c->flags & 0x1)) {
            continue;
        }
        if (u == c->parent) {
            movePos(c);
        }
    }
}

// Advance one curve timer: returns 1 when the curve just ended (or looped).
#define ID_TIMER_STEP(u, n, bit)                                                              \
    h = u->curve[n];                                                                          \
    if (u->dir & bit) {                                                                       \
        u->timer[n]--;                                                                        \
        if ((s16) u->timer[n] <= 0) {                                                         \
            if (u->loop & bit) {                                                              \
                u->timer[n] = (u16) h->key[h->num - 1].t;                                     \
            } else {                                                                          \
                u->end |= bit;                                                                \
            }                                                                                 \
        }                                                                                     \
    } else {                                                                                  \
        u->timer[n]++;                                                                        \
        if ((f32) u->timer[n] >= h->key[h->num - 1].t) {                                      \
            if (u->loop & bit) {                                                              \
                u->timer[n] = 0;                                                              \
            } else {                                                                          \
                u->end |= bit;                                                                \
                u->timer[n] = (u16) h->key[h->num - 1].t;                                     \
            }                                                                                 \
        }                                                                                     \
    }

void idSysMove00(IdUnit* u)
{
    Vec tmp;
    f32 t;

    if (u->path0 != 0 && ((u8*) u->path0)[7] != 0) {
        int num;
        if (u->curve[0] != 0 && (num = u->curve[0]->num) != -1) {
            t = Hermite_1CurveCalc(u->curve[0], (f32) u->timer[0]);
            u->end &= ~0x1;
            if (!(u->dir & 0x1)) {
                u->timer[0]++;
                f32 endT = u->curve[0]->key[num - 1].t;
                if ((f32) u->timer[0] >= endT) {
                    if (u->loop & 0x1) {
                        u->timer[0] = 0;
                    } else {
                        u->end |= 0x1;
                        u->timer[0] = (u16) endT;
                    }
                }
            } else {
                u->timer[0]--;
                if (u->timer[0] <= 0) {
                    if (u->loop & 0x1) {
                        u->timer[0] = (u16) u->curve[0]->key[num - 1].t;
                    } else {
                        u->end |= 0x1;
                    }
                }
            }
        } else {
            t = 0.0f;
        }
        if (FuncPathCalc(u->path0, u->path1, &u->pos, t) == 0 ||
            FuncPathCalc(u->path0, u->path1, &tmp, 0.0f) == 0) {
            memclr_asm(&u->pos, sizeof(Vec));
        } else {
            u->pos.x -= tmp.x;
            u->pos.y -= tmp.y;
            u->pos.z -= tmp.z;
        }
    } else {
        memclr_asm(&u->pos, sizeof(Vec));
    }
    PSVECAdd(&u->pos, &u->scr, &u->pos);
    if (u->kind != 1) {
        IdCalcVertex(u);
    }
}

void IdCalcVertex(IdUnit* u)
{
    switch (u->vtxType & 0xF) {
    case 0:
        u->vtx[0].x = -u->sizeX * 0.5f;
        u->vtx[0].y = u->sizeY * 0.5f;
        u->vtx[0].z = 0.0f;
        u->vtx[1].x = u->sizeX * 0.5f;
        u->vtx[1].y = u->sizeY * 0.5f;
        u->vtx[1].z = 0.0f;
        u->vtx[2].x = u->sizeX * 0.5f;
        u->vtx[2].y = -u->sizeY * 0.5f;
        u->vtx[2].z = 0.0f;
        u->vtx[3].x = -u->sizeX * 0.5f;
        u->vtx[3].y = -u->sizeY * 0.5f;
        u->vtx[3].z = 0.0f;
        break;
    case 1:
        u->vtx[0].x = u->vtx[0].y = u->vtx[0].z = 0.0f;
        u->vtx[1].x = u->sizeX;
        u->vtx[1].y = 0.0f;
        u->vtx[1].z = 0.0f;
        u->vtx[2].x = u->sizeX;
        u->vtx[2].y = -u->sizeY;
        u->vtx[2].z = 0.0f;
        u->vtx[3].x = 0.0f;
        u->vtx[3].y = -u->sizeY;
        u->vtx[3].z = 0.0f;
        break;
    case 2:
        u->vtx[0].x = -u->sizeX;
        u->vtx[0].y = 0.0f;
        u->vtx[0].z = 0.0f;
        u->vtx[1].x = u->vtx[1].y = u->vtx[1].z = 0.0f;
        u->vtx[2].x = 0.0f;
        u->vtx[2].y = -u->sizeY;
        u->vtx[2].z = 0.0f;
        u->vtx[3].x = -u->sizeX;
        u->vtx[3].y = -u->sizeY;
        u->vtx[3].z = 0.0f;
        break;
    case 3:
        u->vtx[0].x = -u->sizeX;
        u->vtx[0].y = u->sizeY;
        u->vtx[0].z = 0.0f;
        u->vtx[1].x = 0.0f;
        u->vtx[1].y = u->sizeY;
        u->vtx[1].z = 0.0f;
        u->vtx[2].x = u->vtx[2].y = u->vtx[2].z = 0.0f;
        u->vtx[3].x = -u->sizeX;
        u->vtx[3].y = 0.0f;
        u->vtx[3].z = 0.0f;
        break;
    case 4:
        u->vtx[0].x = 0.0f;
        u->vtx[0].y = u->sizeY;
        u->vtx[0].z = 0.0f;
        u->vtx[1].x = u->sizeX;
        u->vtx[1].y = u->sizeY;
        u->vtx[1].z = 0.0f;
        u->vtx[2].x = u->sizeX;
        u->vtx[2].y = 0.0f;
        u->vtx[2].z = 0.0f;
        u->vtx[3].x = u->vtx[3].y = u->vtx[3].z = 0.0f;
        break;
    }
}

void idSysMove01(IdUnit* u)
{
    f32 s;
    int i;
    int num;

    if (u->curve[1] == 0 || (num = u->curve[1]->num) == 0) {
        return;
    }
    s = Hermite_1CurveCalc(u->curve[1], (f32) u->timer[1]);
    u->end &= ~0x2;
    if (!(u->dir & 0x2)) {
        u->timer[1]++;
        f32 endT = u->curve[1]->key[num - 1].t;
        if ((f32) u->timer[1] >= endT) {
            if (u->loop & 0x2) {
                u->timer[1] = 0;
            } else {
                u->end |= 0x2;
                u->timer[1] = (u16) endT;
            }
        }
    } else {
        u->timer[1]--;
        if (u->timer[1] <= 0) {
            if (u->loop & 0x2) {
                u->timer[1] = (u16) u->curve[1]->key[num - 1].t;
            } else {
                u->end |= 0x2;
                u->timer[1] = u->timer[1];
            }
        }
    }
    if (u->scaleType & 0x10) {
        for (i = 0; i < 4; i++) {
            u->vtx[i].x *= s;
        }
    } else if (u->scaleType & 0x20) {
        for (i = 0; i < 4; i++) {
            u->vtx[i].y *= s;
        }
    } else {
        for (i = 0; i < 4; i++) {
            PSVECScale(&u->vtx[i], &u->vtx[i], s);
        }
    }
}

void idSysMove02(IdUnit* u)
{
    f32 r;
    int num;

    if (u->curve[2] != 0 && (num = u->curve[2]->num) != 0) {
        r = Hermite_1CurveCalc(u->curve[2], (f32) u->timer[2]);
        if (*(u32*) u->col1 != 0) {
            u->col[0] = (1.0f - r) * u->col0[0] + r * u->col1[0];
            u->col[1] = (1.0f - r) * u->col0[1] + r * u->col1[1];
            u->col[2] = (1.0f - r) * u->col0[2] + r * u->col1[2];
            u->col[3] = (1.0f - r) * u->col0[3] + r * u->col1[3];
            if (u->col[0] > 255.0f) {
                u->col[0] = 255.0f;
            }
            if (u->col[1] > 255.0f) {
                u->col[1] = 255.0f;
            }
            if (u->col[2] > 255.0f) {
                u->col[2] = 255.0f;
            }
            if (u->col[3] > 255.0f) {
                u->col[3] = 255.0f;
            }
            if (u->col[0] < 0.0f) {
                u->col[0] = 0.0f;
            }
            if (u->col[1] < 0.0f) {
                u->col[1] = 0.0f;
            }
            if (u->col[2] < 0.0f) {
                u->col[2] = 0.0f;
            }
            if (u->col[3] < 0.0f) {
                u->col[3] = 0.0f;
            }
        } else {
            u->col[3] = r;
            u->col[0] = (f32) u->col0[0];
            u->col[1] = (f32) u->col0[1];
            u->col[2] = (f32) u->col0[2];
        }
        u->end &= ~0x3;
        if (!(u->dir & 0x4)) {
            u->timer[2]++;
            f32 endT = u->curve[2]->key[num - 1].t;
            if ((f32) u->timer[2] >= endT) {
                if (u->loop & 0x4) {
                    u->timer[2] = 0;
                } else {
                    u->end |= 0x3;
                    u->timer[2] = (u16) endT;
                }
            }
        } else {
            u->timer[2]--;
            if (u->timer[2] <= 0) {
                if (u->loop & 0x4) {
                    u->timer[2] = (u16) u->curve[2]->key[num - 1].t;
                } else {
                    u->end |= 0x3;
                    u->timer[2] = u->timer[2];
                }
            }
        }
    } else {
        u->col[0] = (f32) u->col0[0];
        u->col[1] = (f32) u->col0[1];
        u->col[2] = (f32) u->col0[2];
        u->col[3] = (f32) u->col0[3];
    }
    if (u->parent != 0) {
        IdUnit* p = u->parent;
        u->col[0] = (f32) (u8) (u->col[0] * p->col[0] / 255.0f);
        u->col[1] = (f32) (u8) (u->col[1] * p->col[1] / 255.0f);
        u->col[2] = (f32) (u8) (u->col[2] * p->col[2] / 255.0f);
        u->col[3] = (f32) (u8) (u->col[3] * p->col[3] / 255.0f);
    }
}

void idSysMove03(IdUnit* u)
{
    Vec rot;
    f32 a;
    int num;

    u->rotCur = u->rot;
    if (u->curve[3] != 0 && (num = u->curve[3]->num) != 0) {
        a = Hermite_1CurveCalc(u->curve[3], (f32) u->timer[3]);
        u->end &= ~0x4;
        if (!(u->dir & 0x8)) {
            u->timer[3]++;
            f32 endT = u->curve[3]->key[num - 1].t;
            if ((f32) u->timer[3] >= endT) {
                if (u->loop & 0x8) {
                    u->timer[3] = 0;
                } else {
                    u->end |= 0x4;
                    u->timer[3] = (u16) endT;
                }
            }
        } else {
            u->timer[3]--;
            if (u->timer[3] <= 0) {
                if (u->loop & 0x8) {
                    u->timer[3] = (u16) u->curve[3]->key[num - 1].t;
                } else {
                    u->end |= 0x4;
                    u->timer[3] = u->timer[3];
                }
            }
        }
        switch (u->rotAxis) {
        case 0:
            u->rotCur.x = a;
            break;
        case 1:
            u->rotCur.y = a;
            break;
        case 2:
            u->rotCur.z = a;
            break;
        }
    }
    rot.x = u->rotCur.x * PI / 180.0f;
    rot.y = u->rotCur.y * PI / 180.0f;
    rot.z = u->rotCur.z * PI / 180.0f;
    RotMatrix(u->localMat, &rot);
    PSMTXTransApply(u->localMat, u->localMat, u->pos.x, u->pos.y, u->pos.z);
    if (u->parent != 0 && u->parent->kind == 1) {
        PSMTXConcat(u->parent->mat, u->localMat, u->mat);
    } else {
        PSMTXCopy(u->localMat, u->mat);
    }
}

void idSysMove04(IdUnit* u)
{
    TexAnm* anm;

    if (u->texId == 0xFF) {
        return;
    }
    if (IdGetAnmAddr(u->texId, &anm) == 0) {
        u->flags &= ~0x8;
        pLog->err(0, 0, "idSysMove04():(c[%02x],u[%02x]) texId[%02x] No such Texture.", u->type, u->unitNo, u->texId);
        return;
    }
    if (!(u->flags_7F & 0x2)) {
        u->no = u->texCnt;
        u->texCnt++;
        if (u->texCnt >= anm->numTex) {
            u->texCnt = 0;
        }
    }
    if (!(u->flags_7F & 0x1)) {
        return;
    }
    {
        if (IdGetAnmAddr(u->maskId, &anm) == 0) {
            pLog->err(0, 0, "idSysMove04():[%02x,%02x] maskId[%x] No such Texture.", u->type, u->unitNo, u->maskId);
            return;
        }
        if (!(u->flags_7F & 0x4)) {
            u->maskNo = u->maskCnt;
            u->maskCnt++;
            if (u->maskCnt >= anm->numTex) {
                u->maskCnt = 0;
            }
        }
    }
}

void IDSystem::trans()
{
    int i;
    IdUnit* u;

    if (pG->flags_58 & 0x2000) {
        return;
    }
    if ((s32) pG->flags_64 < 0) {
        return;
    }
    u = pUnit;
    for (i = 0; i < num; i++, u++) {
        if ((pG->flags_58 & 0x10000) && u->ot == 0x13) {
            continue;
        }
        if (IdBitGet(disp, u->type)) {
            continue;
        }
        if (u->flags == 0xFF || !(u->flags & 0x1)) {
            continue;
        }
        if ((u->flags & 0x8) && u->parent == 0) {
            unitTrans(u);
        }
    }
}

void IDSystem::unitTrans(IdUnit* u)
{
    int i;
    int j;
    IdUnit* c = pUnit;

    for (i = 0; i < num; i++, c++) {
        if (c->flags == 0xFF || !(c->flags & 0x1)) {
            continue;
        }
        if (c->flags & 0x8) {
            switch (u->kind) {
            case 1:
                if (c->parent == u) {
                    unitTrans(c);
                }
                break;
            case 2: {
                IdUnit* g = pUnit;
                for (j = 0; j < num; j++, g++) {
                    if (g->flags != 0xFF && g->parent == c) {
                        unitTrans(g);
                    }
                }
                break;
            }
            }
        }
    }
    if (u != 0) {
        u->flags |= 0x10;
        AddOtDirect(u->ot, u, (void (*)()) IdGeneralTrans, u->prio, 0x1000, 0, 0.0f);
    }
}

void IdGeneralTrans(IdUnit* u)
{
    u->flags &= ~0x10;
    if (u->texId == 0xFF) {
        return;
    }
    GXColor col;
    col.g = 0;
    col.b = 0;
    col.r = 0;
    col.a = 0;
    GXSetFog(0, 0.0f, 0.0f, ZNEAR, ZFAR, col);
    switch (u->transType) {
    case 0:
        IdCommonTrans(u);
        break;
    case 1:
        if (u->transSub <= 1) {
            IdNegativeTrans(u, u->transSub);
        } else {
            IdNegativeTrans(u, 2);
        }
        break;
    case 2:
    case 3:
        IdShimmerTrans(u, u->transSub, u->transType);
        break;
    default:
        IdCommonTrans(u);
        break;
    }
    LightMgr.setFog();
}

// GX_QUADS in immediate mode: matrix index, position, normal, one texture coordinate.
static inline void IdVertex(Vec* v, f32 s, f32 t)
{
    GXMatrixIndex1u8(0);
    GXPosition3f32(v->x, v->y, v->z);
    GXNormal3s8(0, 1, 0);
    GXTexCoord2f32(s, t);
}

static inline void IdVtxFmt()
{
    GXClearVtxDesc();
    GXSetVtxDesc(0, 1);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(10, 1);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 10, 0, 1, 0);
    GXSetVtxAttrFmt(0, 13, 1, 4, 0);
}

void IdCommonTrans(IdUnit* u)
{
    int blend[5][4] = {
        { 0, 1, 4, 5 }, { 0, 1, 4, 1 }, { 0, 1, 1, 1 }, { 0, 1, 2, 1 }, { 0, 1, 2, 0 },
    };

    GXSetCullMode(0);
    CameraCurrentProjection();
    {
        Mtx m;
        PSMTXConcat(IDSystem::m_scrn_mat, u->mat, m);
        GXLoadPosMtxImm(m, 0);
        GXLoadNrmMtxImm(m, 0);
    }
    GXSetCurrentMtx(0);
    IdTexSet(u->texId, u->no);
    IdChannelSet(u);
    GXSetAlphaCompare(4, 1, 1, 4, 1);
    GXSetBlendMode(blend[u->blendType][0], blend[u->blendType][1], blend[u->blendType][2], blend[u->blendType][3]);
    if (u->flags_7F & 0x1) {
        TexWk* wk = IdGetTexWk(u->maskId, 1);
        if (wk != 0) {
            GXTexObj obj;
            GXTlutObj tlut;
            Mtx tm;
            TEXDescriptor* td = TEXGet(wk->pTpl, u->maskNo);
            TEXHeader* th = td->textureHeader;
            if (th->format == 8 || th->format == 9) {
                GXInitTexObjCI(&obj, th->data, th->width, th->height, th->format, 0, 0, 0, 1);
                GXInitTlutObj(&tlut, td->CLUTHeader->data, td->CLUTHeader->format, td->CLUTHeader->numEntries);
                GXLoadTlut(&tlut, 1);
            } else {
                GXInitTexObj(&obj, th->data, th->width, th->height, th->format, 0, 0, 0);
            }
            GXLoadTexObj(&obj, 1);
            PSMTXIdentity(tm);
            GXLoadTexMtxImm(tm, 0x21, 1);
            GXSetTexCoordGen(1, 1, 4, 0x21);
            GXSetNumTevStages(2);
            GXSetNumTexGens(2);
            GXSetTevOrder(1, 1, 1, 4);
            GXSetTevColorIn(1, 0xF, 0xF, 0xF, 0);
            GXSetTevColorOp(1, 0, 0, 0, 1, 0);
            GXSetTevAlphaIn(1, 7, 4, 5, 7);
            GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
        }
    }
    IdVtxFmt();
    GXBegin(0x80, 0, 4);
    IdVertex(&u->vtx[0], u->u0, u->v0);
    IdVertex(&u->vtx[1], u->u1, u->v0);
    IdVertex(&u->vtx[2], u->u1, u->v1);
    IdVertex(&u->vtx[3], u->u0, u->v1);
    GXSetAlphaUpdate(0);
}

void IdNegativeTrans(IdUnit* u, int mode)
{
    int blend[5][4] = {
        { 0, 1, 4, 5 }, { 0, 1, 4, 1 }, { 0, 1, 1, 1 }, { 0, 1, 2, 1 }, { 0, 1, 2, 0 },
    };
    GXColor col = { 0, 0, 0, 0 };
    void* buf;
    Mtx pm;
    Mtx tm;
    Mtx tm2;

    GXSetCullMode(0);
    CameraCurrentProjection();
    {
        Mtx m;
        PSMTXConcat(IDSystem::m_scrn_mat, u->mat, m);
        GXLoadPosMtxImm(m, 0);
        GXLoadNrmMtxImm(m, 0);
    }
    GXSetCurrentMtx(0);
    IdTexSet(u->texId, u->no);
    IdChannelSet(u);
    GXSetAlphaCompare(4, 1, 1, 4, 1);
    GXSetBlendMode(blend[u->blendType][0], blend[u->blendType][1], blend[u->blendType][2], blend[u->blendType][3]);
    IdVtxFmt();
    GXSetFog(0, 0.0f, 0.0f, ZNEAR, ZFAR, col);

    buf = IdGetBufferAddr(1);
    GXSetTexCopySrc(0, 0, (u32) Screen.width, (u32) Screen.height);
    GXSetTexCopyDst((u32) Screen.width >> 1, (u32) Screen.height >> 1, 6, 1);
    GXCopyTex(buf, 0);
    GXPixModeSync();
    GXInvalidateTexAll();
    {
        GXTexObj obj;
        GXInitTexObj(&obj, buf, (u32) Screen.width >> 1, (u32) Screen.height >> 1, 6, 0, 0, 0);
        GXLoadTexObj(&obj, 1);
    }
    C_MTXLightPerspective(pm, pG->Cam.param.fovy, 1.3333334f, 0.5f, -0.5f, 0.5f, 0.5f);
    PSMTXConcat(IDSystem::m_scrn_mat, u->mat, tm);
    PSMTXConcat(pm, tm, tm2);
    GXLoadTexMtxImm(tm2, 0x1E, 0);
    GXSetTexCoordGen(0, 0, 0, 0x1E);
    GXSetTevOrder(0, 0, 1, 4);
    GXSetTevColorIn(0, 0xF, 0xF, 0xF, 8);
    switch (mode) {
    case 0:
        GXSetTevColorOp(0, 0, 0, 0, 1, 0);
        break;
    case 1:
        GXSetTevColorOp(0, 0, 0, 1, 1, 0);
        break;
    case 2:
        GXSetTevColorOp(0, 0, 0, 2, 1, 0);
        break;
    }
    GXSetTevAlphaIn(0, 7, 7, 7, 5);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    GXSetTevOrder(1, 0xFF, 0xFF, 4);
    GXSetTevColorIn(1, 0xA, 0xF, 0, 0xF);
    GXSetTevColorOp(1, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(1, 7, 7, 7, 5);
    GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
    GXSetNumTexGens(2);
    GXSetTexCoordGen(1, 1, 4, 0x3C);
    GXSetTevOrder(2, 1, 0, 4);
    GXSetTevColorIn(2, 0xF, 0xF, 0xF, 0);
    GXSetTevColorOp(2, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(2, 7, 0, 4, 7);
    GXSetTevAlphaOp(2, 0, 0, 0, 1, 0);
    GXSetNumTevStages(3);
    GXBegin(0x80, 0, 4);
    IdVertex(&u->vtx[0], 0.0f, 0.0f);
    IdVertex(&u->vtx[1], 1.0f, 0.0f);
    IdVertex(&u->vtx[2], 1.0f, 1.0f);
    IdVertex(&u->vtx[3], 0.0f, 1.0f);
    GXSetNumTevStages(1);
    GXSetNumTexGens(0);
    GXSetNumIndStages(0);
    GXSetTevDirect(0);
    GXSetTevDirect(1);
    LightMgr.setFog();
    GXSetAlphaUpdate(0);
}

void IdShimmerTrans(IdUnit* u, int sub, int type)
{
    int blend[5][4] = {
        { 0, 1, 4, 5 }, { 0, 1, 4, 1 }, { 0, 1, 1, 1 }, { 0, 1, 2, 1 }, { 0, 1, 2, 0 },
    };
    GXTexObj obj;
    GXColor col = { 0, 0, 0, 0 };
    void* buf;
    Mtx pm;
    Mtx tm;
    Mtx tm2;
    f32 scale;
    u8 nStage = 1;
    u8 nGen;
    Vec zv = { 0.0f, 0.0f, -1.0f };
    Vec* pz = &zv;
    Vec dir;
    Vec d2;
    Vec d3;
    f32 dot;

    scale = (f32) sub * (1.0f / 32.0f) + 1.0f;
    GXSetCullMode(0);
    CameraCurrentProjection();
    {
        Mtx m;
        PSMTXConcat(IDSystem::m_scrn_mat, u->mat, m);
        GXLoadPosMtxImm(m, 0);
        GXLoadNrmMtxImm(m, 0);
    }
    GXSetCurrentMtx(0);
    IdTexSet(u->texId, u->no);
    IdChannelSet(u);
    GXSetAlphaCompare(4, 1, 1, 4, 1);
    GXSetBlendMode(blend[u->blendType][0], blend[u->blendType][1], blend[u->blendType][2], blend[u->blendType][3]);
    IdVtxFmt();
    GXSetFog(0, 0.0f, 0.0f, ZNEAR, ZFAR, col);

    buf = IdGetBufferAddr(2);
    GXSetTexCopySrc(0, 0, (u32) Screen.width, (u32) Screen.height);
    GXSetTexCopyDst((u32) Screen.width >> 1, (u32) Screen.height >> 1, 6, 1);
    GXCopyTex(buf, 0);
    GXPixModeSync();
    GXInvalidateTexAll();
    GXInitTexObj(&obj, buf, (u32) Screen.width >> 1, (u32) Screen.height >> 1, 6, 0, 0, 0);
    GXInitTexObjLOD(&obj, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    GXLoadTexObj(&obj, 1);
    g_Get_tex_obj = obj;
    C_MTXLightPerspective(pm, pG->Cam.param.fovy, 1.3333334f, 0.5f, -0.5f, 0.5f, 0.5f);
    nGen = 2;
    PSMTXConcat(IDSystem::m_scrn_mat, u->mat, tm);
    PSMTXConcat(pm, tm, tm2);
    GXLoadTexMtxImm(tm2, 0x1E, 0);
    GXSetTexCoordGen(0, 0, 0, 0x1E);
    GXSetNumIndStages(1);
    GXSetTexCoordGen(1, 1, 4, 0x3C);
    GXSetIndTexOrder(0, 1, 0);
    GXSetIndTexCoordScale(0, 0, 0);
    {
        f32 indMtx[2][3];
        d2 = dir;
        d3 = d2;
        dot = PSVECDotProduct(pz, &d3);
        if (dot < 1500.0f) {
            dot = 1500.0f;
        }
        indMtx[0][0] = indMtx[1][1] = u->col[3] * (1.0f / 255.0f) * 0.04f * 1000.0f / dot * scale;
        indMtx[0][1] = 0.0f;
        indMtx[0][2] = 0.0f;
        indMtx[1][0] = 0.0f;
        indMtx[1][2] = 0.0f;
        GXSetIndTexMtx(1, indMtx, 1);
    }
    switch (type) {
    case 2:
        GXSetTevIndWarp(0, 0, 0, 0, 1);
        break;
    case 3:
        GXSetTevIndWarp(0, 0, 1, 0, 1);
        break;
    default:
        pLog->err(0, 0, "IdShimmerTrans:[%02x,%02x] BLUR_TYPE[%x] invalid", u->type, u->unitNo, type);
        GXSetTevIndWarp(0, 0, 0, 1, 1);
        break;
    }
    GXSetTevOrder(0, 0, 1, 4);
    GXSetTevColorIn(0, 0xF, 8, 0xA, 0xF);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(0, 7, 7, 7, 5);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    if (u->flags_7F & 0x1) {
        TexWk* wk = IdGetTexWk(u->maskId, 1);
        if (wk != 0) {
            GXTexObj mobj;
            GXTlutObj tlut;
            TEXDescriptor* td = TEXGet(wk->pTpl, u->maskNo);
            TEXHeader* th = td->textureHeader;
            if (th->format == 8 || th->format == 9) {
                GXInitTexObjCI(&mobj, th->data, th->width, th->height, th->format, 0, 0, 0, 1);
                GXInitTlutObj(&tlut, td->CLUTHeader->data, td->CLUTHeader->format, td->CLUTHeader->numEntries);
                GXLoadTlut(&tlut, 1);
            } else {
                GXInitTexObj(&mobj, th->data, th->width, th->height, th->format, 0, 0, 0);
            }
            th = td->textureHeader;
            GXInitTexObjLOD(&mobj, 1, 1, (f32) th->minLOD, (f32) th->maxLOD, th->LODBias, 0, th->edgeLODEnable, 0);
            nStage = 2;
            GXLoadTexObj(&mobj, 2);
            {
                Mtx im;
                PSMTXIdentity(im);
                GXLoadTexMtxImm(im, 0x21, 1);
            }
            GXSetTexCoordGen(nGen, 1, 4, 0x21);
            GXSetTevOrder(1, nGen, 2, 4);
            nGen++;
            GXSetTevColorIn(1, 0xF, 0xF, 0xF, 0);
            GXSetTevColorOp(1, 0, 0, 0, 1, 0);
            GXSetTevAlphaIn(1, 7, 4, 5, 7);
            GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
        }
    }
    GXSetNumTevStages(nStage);
    GXSetNumTexGens(nGen);
    GXBegin(0x80, 0, 4);
    IdVertex(&u->vtx[0], 0.0f, 0.0f);
    IdVertex(&u->vtx[1], 1.0f, 0.0f);
    IdVertex(&u->vtx[2], 1.0f, 1.0f);
    IdVertex(&u->vtx[3], 0.0f, 1.0f);
    GXSetNumTevStages(1);
    GXSetNumTexGens(0);
    GXSetNumIndStages(0);
    GXSetTevDirect(0);
    GXSetTevDirect(1);
    LightMgr.setFog();
}

void IdAllocBuffer()
{
#line 2779 "D:/Bio4/Prog/id_sys.cpp"
    g_pIdBuff = MEM_ALLOC(0x46000, 1, 0xD);
}

void IdFreeBuffer()
{
    if (g_pIdBuff != 0) {
        Mem_free(g_pIdBuff);
    }
    g_pIdBuff = 0;
}

void IdDebugAllocBuffer()
{
    g_pIdBuff = Debug_alloc(0x46000, 1);
}

void IdDebugFreeBuffer()
{
    Debug_free(g_pIdBuff);
    g_pIdBuff = 0;
}

void* IdGetBufferAddr(int type)
{
    if ((pG->flags_64 & 0x100000) || (pG->flags_500C & 0x40000) || (pG->flags_5014 & 0x8000)) {
        IdSetBufferType(type);
        return g_pIdBuff;
    }
    return GetDrawTmpBufAddr(0xF);
}

void IdSetBufferType(int type)
{
    IdBuffType = type;
}

IDSystem IdSys;
void* g_pIdBuff = 0;
int IdBuffType;
