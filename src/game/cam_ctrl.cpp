#include "types.h"
#include "vec.h"
#include "global.h"
#include "camera.h"
#include "cam_ctrl.h"
#include "cam_extra.h"
#include "cam_motion.h"
#include "db_log.h"
#include "light.h"
#include "model.h"
#include "player.h"
#include "em.h"

extern "C" {
int strncmp(const char* a, const char* b, unsigned int n);
void* memset(void* dst, int c, unsigned int n);
void OSReport(const char* fmt, ...);
f32 sinf(f32);
f32 cosf(f32);
}

extern f32 ZNEAR;
u32 SubCharGetStatus();
int GetWaterHeight(Vec* pos, f32* height);
void QuakeInit();
void eprintf(int x, int y, int color, int a, const char* fmt, ...);


#define PI 3.1415927f
#define PI2 6.2831855f

void* g_pToolCamData = NULL;

#define CAMERA_MOTION_BUFFER_SIZE 0x440
static u8 CameraMotionBuffer[CAMERA_MOTION_BUFFER_SIZE];
static CameraBSpline CamBSpline;

const f32 smooth_ratio[12] = {0.0f, 0.9f, 0.85f, 0.92f, 0.8f, 0.92f, 0.9f, 0.9f, 0.9f, 0.9f, 0.0f, 0.0f};

int CameraControl::IsChangeCamera()
{
    if (flags_30 & 2) {
        return 1;
    }
    return 0;
}

void CameraControl::Comeback(int)
{
    data = (CameraDataHeader*) pG->pRoomCamData;
    flags_30 &= ~4;
    flags_2C = (flags_2C & ~8) | 0x10;
    if (flags_2C & 0x20) {
        flags_2C &= ~0x20;
    }
    Check();
}

void CameraControl::Disable()
{
    state = 0;
    flags_2C |= 8;
}

void CameraControl::AreaCheckOnOff(int mode)
{
    switch (mode) {
    case 0:
        flags_2C |= 8;
        break;
    case 1:
        flags_2C = (flags_2C & ~8) | 0x10;
        break;
    }
}

u8 CameraControl::AreaNum()
{
    return data->numArea;
}

int CameraControl::CurrentAreaNo()
{
    return area_no;
}

int CameraControl::CurrentCameraNo()
{
    return camera_no;
}

CameraCut* CameraControl::DataSearch(int no)
{
    CameraAreaRec* rec = (CameraAreaRec*) (data + 1);
    CameraAreaInfo* area = (CameraAreaInfo*) (rec + data->numArea);
    CameraCut* cut = (CameraCut*) (area + data->numArea);
    int i = 0;

    while (i < data->numCut && no != cut->camera_no) {
        i++;
        cut++;
    }
    return cut;
}

CameraLerp* CameraControl::LerpDataSearch(int area_from, int cam_from, int area_to, int cam_to)
{
    CameraAreaRec* rec = (CameraAreaRec*) (data + 1);
    CameraAreaInfo* area = (CameraAreaInfo*) (rec + data->numArea);
    CameraCut* cut = (CameraCut*) (area + data->numArea);
    CameraLerp* lerp = (CameraLerp*) (cut + data->numCut);
    int i;

    for (i = 0; i < data->numLerp; i++, lerp++) {
        if (area_from == lerp->area_from && cam_from == lerp->cam_from && area_to == lerp->area_to &&
            cam_to == lerp->cam_to) {
            return lerp;
        }
    }
    return NULL;
}

CameraDataHeader* CameraControl::calcAddr(CameraDataHeader* d)
{
    int ver2 = 0;
    int i;
    CameraAreaRec* rec;
    CameraAreaInfo* area;
    CameraCut* cut;

    if (cameraDataVersion((char*) d) <= 1) {
        return d;
    }
    if (strncmp((char*) d, "B402", 4) == 0) {
        ver2 = 1;
        OSReport("CameraControl::calcAddr(): R%1d%02x Ver02", pG->stage_no, pG->room_no);
    }

    rec = (CameraAreaRec*) (d + 1);
    for (i = 0; i < d->numArea; i++, rec++) {
        if ((s32) rec->area < 0) {
            return d;
        }
        rec->area = (CameraAreaInfo*) ((u32) rec->area + (u32) d);
        if (rec->cut) {
            rec->cut = (CameraCut*) ((u32) rec->cut + (u32) d);
        }
    }

    area = (CameraAreaInfo*) rec;
    for (i = 0; i < d->numArea; i++, area++) {
        area->points = (Vec*) ((u32) area->points + (u32) d);
        if (ver2) {
            area->attr = 3;
        }
        if (area->attr & 8) {
            area->attr |= 0x20;
        }
        if (cameraDataVersion((char*) d) <= 3) {
            area->attr2 = 1;
            area->x9 = 0xFF;
            OSReport("CameraControl::calcAddr(): R%1d%02x Ver%02d", pG->stage_no, pG->room_no,
                     cameraDataVersion((char*) d));
        }
    }

    cut = (CameraCut*) area;
    for (i = 0; i < d->numCut; i++, cut++) {
        cut->pos = (Vec*) ((u32) cut->pos + (u32) d);
        cut->at = (Vec*) ((u32) cut->at + (u32) d);
        cut->roll = (f32*) ((u32) cut->roll + (u32) d);
        cut->fovy = (f32*) ((u32) cut->fovy + (u32) d);
        cut->frames = (u16*) ((u32) cut->frames + (u32) d);
    }
    return d;
}

void CameraControl::RoomDataRead(CameraDataHeader* room)
{
    G_ROOM_CAM_DATA = calcAddr(room);
    data = (CameraDataHeader*) pG->pRoomCamData;
}

void CameraControl::CoreDataRead(CameraDataHeader* core)
{
    pG->pCoreCamData = calcAddr(core);
}

void CameraSetCutData(Camera* cam, CameraCut* cut)
{
    cam->param.pos = *cut->pos;
    cam->param.at = *cut->at;
    cam->param.roll = *cut->roll;
    cam->param.fovy = *cut->fovy;
    CameraSetOrientationRoll(cam);
}

void CameraControl::AreaOnOff(int area_no, int camera_no, int on)
{
    CameraDataHeader* d = data;
    CameraAreaRec* rec = (CameraAreaRec*) (d + 1);
    s8 i;

    for (i = 0; i < d->numArea; i++, rec++) {
        if (area_no == rec->area->area_no && camera_no == rec->area->camera_no) {
            rec->area->enable = on;
            break;
        }
    }
}

void CameraControl::SetAreaAttr(int area_no, int camera_no, u8 attr)
{
    CameraDataHeader* d = data;
    CameraAreaRec* rec = (CameraAreaRec*) (d + 1);
    s8 i;

    for (i = 0; i < d->numArea; i++, rec++) {
        if (area_no == rec->area->area_no && camera_no == rec->area->camera_no) {
            rec->area->attr |= attr;
            break;
        }
    }
}

void CameraControl::UnsetAreaAttr(int area_no, int camera_no, u8 attr)
{
    CameraDataHeader* d = data;
    CameraAreaRec* rec = (CameraAreaRec*) (d + 1);
    s8 i;

    for (i = 0; i < d->numArea; i++, rec++) {
        if (area_no == rec->area->area_no && camera_no == rec->area->camera_no) {
            rec->area->attr &= ~attr;
            break;
        }
    }
}

void CameraControl::CutCall(int no)
{
    CameraDataHeader* d = data;
    CameraAreaRec* rec = (CameraAreaRec*) (d + 1);
    int found = 0;
    s8 i;

    for (i = 0; i < d->numArea; i++, rec++) {
        if (no == rec->cut->camera_no) {
            found = 1;
            break;
        }
    }
    if (found) {
        clearAttachCamera();
        interp.frame = 0;
        switchCamera(rec);
        flags_2C |= 8;
        flags_30 |= 4;
    } else {
        pLog->err(0, 0, "CameraControl::CutCall(): Cut %02d doesn't exist.", no);
    }
}

void CameraControl::switchCamera(CameraAreaRec* rec)
{
    CameraAreaInfo* area = rec->area;
    CameraCut* cut = rec->cut;
    CameraLerp* lerp = NULL;
    CameraDataHeader* d;
    int i;
    CameraAreaRec* r;
    int size;

    if (area_no != -1) {
        lerp = LerpDataSearch(area_no, x691, area->area_no, area->camera_no);
        if (lerp && lerp->enable == 1) {
            interp.set(lerp->frame, &cur);
        }
    } else {
        interp.frame = 0;
    }

    if (flags_2C & 2) {
        if (!(rec->area->attr & 8)) {
            d = data;
            r = (CameraAreaRec*) (d + 1);
            for (i = 0; i < d->numArea; i++, r++) {
                if (r->area->attr & 8) {
                    r->area->enable = 0;
                }
            }
        }
        flags_2C &= ~2;
    }

    if (area_rec != NULL) {
        CameraAreaInfo* a = area_rec->area;
        if (a->attr & 0x10) {
            a->enable = 0;
        } else if (a->attr & 8) {
            d = data;
            r = (CameraAreaRec*) (d + 1);
            for (i = 0; i < d->numArea; i++, r++) {
                if (r->area->attr & 8) {
                    r->area->enable = 0;
                }
            }
        }
    }

    area_no = area->area_no;
    x691 = area->camera_no;
    camera_no = cut->camera_no;
    area_rec = rec;

    if (area_no != -1) {
        if (flags_2C & 0x10) {
            if (!(flags_2C & 0x40)) {
                LightMgr.update(area_no, -1);
            }
        } else if (!(area->attr & 0x80)) {
            LightMgr.update(area_no, -1);
        }
    }
    flags_30 |= 2;

    switch (cut->type) {
    case 0:
        sub_state = 0;
        state = 1;
        break;
    case 1:
        state = 2;
        sub_state = 0;
        break;
    case 2:
        state = 3;
        sub_state = 0;
        break;
    case 3:
        state = 4;
        sub_state = 0;
        break;
    case 4:
        state = 6;
        sub_state = 0;
        break;
    case 5:
        state = 7;
        sub_state = 0;
        break;
    case 6:
        size = HermiteExport(cut, CameraMotionBuffer);
        if (size > CAMERA_MOTION_BUFFER_SIZE) {
            pLog->err(0, 0, "CameraControl::HermiteExport() = 0x%04x > 0x%04x", size, CAMERA_MOTION_BUFFER_SIZE);
        }
        if (extra) {
            delete extra;
        }
        extra = new (extra_buf) CameraMotion(CameraMotionBuffer, 0, 0, 100.0f);
        ((CameraMotion*) extra)->base_mat = NULL;
        state = 5;
        break;
    case 7:
        size = HermiteExport(cut, CameraMotionBuffer);
        if (size > CAMERA_MOTION_BUFFER_SIZE) {
            pLog->err(0, 0, "CameraControl::HermiteExport() = 0x%04x > 0x%04x", size, CAMERA_MOTION_BUFFER_SIZE);
        }
        if (extra) {
            delete extra;
        }
        extra = new (extra_buf) CameraMotion(CameraMotionBuffer, 0, 0, 100.0f);
        state = 9;
        break;
    case 8:
        if (qfps.blend_src && qfps.blend_dst) {
            qfps.setBlendData(qfps.blend_src, qfps.blend_dst);
        }
        qfps.setAreaData(area_rec->cut);
        qfps.bindAreaCamera(area_rec);
        if (prev_state == 10 && !(flags_2C & 0x10)) {
            qfps.setBlendCount(10);
        } else {
            qfps.init();
            sub_state = 0;
        }
        state = 10;
        break;
    }
    flags_2C &= ~0x10;
}

int areaAttr(CameraAreaInfo* area, u8 attr, u8 attr2)
{
    if ((area->enable & 1) && (area->attr & attr) && (area->attr2 & attr2)) {
        return 1;
    }
    return 0;
}

int areaHit(Vec* pos, CameraAreaInfo* area, f32 dir)
{
    int ret;

    if (area->attr & 4) {
        return 0;
    }
    if (area->attr & 0x40) {
        f32 d = area->dir;
        while (dir >= PI) {
            dir -= PI2;
        }
        while (dir < -PI) {
            dir += PI2;
        }
        dir -= d;
        while (dir >= PI) {
            dir -= PI2;
        }
        while (dir < -PI) {
            dir += PI2;
        }
        if (dir > 2.3561945f || dir < -2.3561945f) {
            return 0;
        }
    }
    if (area->num > 4) {
        ret = area_hit_pN(pos, area);
    } else {
        ret = area_hit_p3(pos, area);
    }
    return ret;
}

int area_hit_p3(Vec* pos, CameraAreaInfo* area)
{
    Vec c1, c0, v0, v2, v1;
    Vec *p0, *p1, *p2;
    f32 y = pos->y + 100.0f;
    int i, n, i0;

    if (y < area->base_y) {
        return 0;
    }
    if (y >= area->base_y + area->height) {
        return 0;
    }
    for (i = 0; i <= 1; i++) {
        n = area->num;
        i0 = (i + i + 1) % n;
        p0 = &area->points[i0];
        p2 = &area->points[(i0 + n - 1) % n];
        p1 = &area->points[(i0 + 1) % n];
        PSVECSubtract(pos, p0, &v0);
        PSVECSubtract(p2, p0, &v1);
        PSVECSubtract(p1, p0, &v2);
        PSVECCrossProduct(&v1, &v0, &c0);
        PSVECCrossProduct(&v2, &v0, &c1);
        if (c0.y > 0.0f) {
            return 0;
        }
        if (c1.y < 0.0f) {
            return 0;
        }
    }
    return 1;
}

int area_hit_pN(Vec* pos, CameraAreaInfo* area)
{
    f32 y = pos->y + 100.0f;
    f32 px, c;
    f32 xi, zi, a, b, dx, dz, xmin, xmax, zmin, zmax;
    Vec *pi, *pj;
    int i, n, count, fx, fz;

    if (y < area->base_y) {
        return 0;
    }
    if (y >= area->base_y + area->height) {
        return 0;
    }
    px = pos->x;
    c = pos->x - pos->z;
    n = area->num;
    count = 0;
    for (i = 0; i < n; i++) {
        pi = &area->points[i];
        pj = &area->points[(i + 1) % n];
        dx = pj->x - pi->x;
        dz = pj->z - pi->z;
        if (dz != 0.0f) {
            a = dx / dz;
            b = pi->x - a * pi->z;
            if (1.0f == a) {
                continue;
            }
            xi = (1.0f * b - c * a) / (1.0f - a);
            zi = (b - c) / (1.0f - a);
        } else {
            zi = pi->z;
            xi = 1.0f * zi + c;
        }
        if (dx > 0.0f) {
            xmin = pi->x;
            xmax = pj->x;
            fx = 0;
        } else {
            xmin = pj->x;
            xmax = pi->x;
            fx = 1;
        }
        if (dz > 0.0f) {
            zmin = pi->z;
            zmax = pj->z;
            fz = 0;
        } else {
            zmin = pj->z;
            zmax = pi->z;
            fz = 1;
        }
        if (!(xi >= px)) {
            continue;
        }
        if (fx == 0) {
            if (!(xi >= xmin)) {
                continue;
            }
            if (!(xi < xmax)) {
                continue;
            }
        } else {
            if (!(xi > xmin)) {
                continue;
            }
            if (!(xi <= xmax)) {
                continue;
            }
        }
        if (fz == 0) {
            if (!(zi >= zmin)) {
                continue;
            }
            if (!(zi < zmax)) {
                continue;
            }
        } else {
            if (!(zi > zmin)) {
                continue;
            }
            if (!(zi <= zmax)) {
                continue;
            }
        }
        count++;
    }
    if (count & 1) {
        return 1;
    }
    return 0;
}

void CameraControl::areaHitCheck()
{
    static u8 blink = 0;
    CameraDataHeader* d;
    CameraAreaRec* rec;
    CameraAreaRec* first;
    CameraAreaInfo* area;
    CameraCut* cut;
    u8 attr = 1;
    u8 old_attr;
    int old_area = area_no;
    s8 i;

    if (pG->flags_60 & 0x800) {
        return;
    }
    d = data;
    if (d == NULL) {
        state = 0xA;
        camera_no = -1;
        area_no = -1;
        x691 = -1;
        if (old_area != -1 || (flags_2C & 0x10)) {
            qfps.init();
            flags_2C &= ~0x10;
        }
        area_rec = NULL;
        return;
    }
    if (flags_2C & 1) {
        if (flags_2C & 0x10) {
            state = 0xA;
            qfps.init();
            flags_2C &= ~0x10;
        }
        return;
    }
    if (cameraDataVersion((char*) d) <= 1) {
        state = 0xA;
        camera_no = -1;
        area_no = -1;
        x691 = -1;
        if (old_area != -1 || (flags_2C & 0x10)) {
            qfps.init();
            flags_2C &= ~0x10;
        }
        area_rec = NULL;
        return;
    }

    if (SubCharGetStatus() & 0x20000000) {
        attr = 2;
    } else {
        switch (pG->x4FB8) {
        case 0:
            break;
        case 1:
            if (pG->x4F93 == 0) {
                attr = 4;
            }
            break;
        case 2:
            attr = 8;
            break;
        case 3:
            attr = 0x20;
            break;
        case 5:
            attr = 0x10;
            break;
        case 4:
            attr = 0x40;
            break;
        }
    }

    first = rec = (CameraAreaRec*) (d + 1);
    for (i = 0; i < d->numArea; i++, rec++) {
        area = rec->area;
        cut = rec->cut;
        if (areaAttr(area, 0x20, attr) && areaHit(&pPL->pos, area, pPL->rot.y)) {
            if ((flags_2C & 0x10) || cut->camera_no != camera_no) {
                switchCamera(rec);
            }
            return;
        }
    }

    old_attr = area_attr;
    if (pG->flags_6C & 0x40000000) {
        area_attr = 2;
        if (blink++ & 0x18) {
            eprintf(27, 18, 22, 0, "[ BATTLE ]");
        }
    } else {
        if (battle_timer > 0) {
            battle_timer--;
        }
        if (battle_timer == 0 && EmMgr.isBattle()) {
            area_attr = 2;
        } else {
            area_attr = 1;
        }
    }
    if (flags_2C & 4) {
        area_attr = 2;
    }
    if (old_attr != area_attr) {
        flags_2C |= 0x10;
    }

    if (area_no != -1 && !(flags_2C & 0x10)) {
        area = area_rec->area;
        if (areaAttr(area, area_attr, attr) && areaHit(&pPL->pos, area, pPL->rot.y)) {
            return;
        }
    }

    rec = first;
    for (i = 0; i < d->numArea; i++, rec++) {
        area = rec->area;
        cut = rec->cut;
        if (areaAttr(area, area_attr, attr) && areaHit(&pPL->pos, area, pPL->rot.y)) {
            if ((flags_2C & 0x10) || cut->camera_no != camera_no) {
                switchCamera(rec);
            }
            return;
        }
    }

    state = 0xA;
    area_no = -1;
    x691 = -1;
    camera_no = -1;
    area_rec = NULL;
    if (flags_2C & 0x10) {
        flags_2C &= ~0x10;
        qfps.bindDefaultCamera();
        qfps.init();
        sub_state = 0;
        LightMgr.update(0, -1);
    } else if (old_area != -1) {
        CameraQuasiFPS* q = &qfps;
        if (prev_state == 0xA) {
            if (q->blend_src && q->blend_dst) {
                q->setBlendData(q->blend_src, q->blend_dst);
            }
            q->setBlendCount(10);
        }
        q->bindDefaultCamera();
        LightMgr.update(0, -1);
    }
}

void CameraControl::roomInit()
{
    s8 ver;

    BitOn(pG->flags_500C, 0x100);
    if (data == NULL) {
        flags_28 = 0;
    } else {
        flags_2C = 0x10;
        ver = cameraDataVersion((char*) data);
        switch (ver) {
        case 2:
            OSReport("CameraControl::roomInit(): Ver.02");
            break;
        case 1:
            pLog->warn(0, 0, "CameraControl::roomInit(): Ver.01");
            break;
        case 0:
            pLog->warn(0, 0, "CameraControl::roomInit(): Ver.00");
            break;
        case -1:
            pLog->warn(0, 0, "CameraControl::roomInit(): Empty!");
            break;
        }
        if (ver < -1) {
            flags_28 = 0;
        } else if (ver > 1) {
            if (ver <= 4) {
                flags_28 |= 1;
            } else {
                flags_28 = 0;
            }
        } else {
            flags_28 |= 1;
            flags_2C |= 1;
        }
    }
    flags_28 &= ~4;
    flags_30 &= ~4;
    area_rec = NULL;
    state = 0xA;
    qfps.offsetCorrection();
    qfps.bindDefaultCamera();
    qfps.setFloorRatio(0.33333334f);
    qfps.init();
    camera_no = -1;
    x6E0 = 0xF;
    area_no = -1;
    x691 = -1;
    x6CC = 60.0f;
    x6D0 = 600.0f;
    x6D4 = 400.0f;
    x6D8 = 0.7853982f;
    x6DC = 0.3926991f;
    x6E4 = 0.001f;
    x6E8 = 0.75f;
    clearAttachCamera();
    BitOn(flags_2C, 2);
    if (pG->flags_54 & 0x200000) {
        state = 0;
        BitOn(flags_2C, 8);
    }
    x250 = 0;
    extra = NULL;
    interp.frame = 0;
    Check();
    Move();
    CameraMove();
    QuakeInit();
    memset(extra_buf, 9, sizeof(extra_buf));
    g_pToolCamData = NULL;
}

void CameraControl::Check()
{
    Vec d;

    if (pG->flags_500C & 0x40000) {
        return;
    }
    if (!(flags_28 & 1)) {
        return;
    }
    if (flags_28 & 4) {
        return;
    }
    flags_30 &= ~2;
    prev_state = state;
    if (!(flags_2C & 8)) {
        areaHitCheck();
    }
    checkAttachCamera();
    if (pG->flags_64 & 0x800000) {
        return;
    }
    if (pG->flags_500C & 0x1000) {
        return;
    }
    if (x250 != 0) {
        return;
    }
    if (flags_30 & 4) {
        return;
    }
    if (pPL->p2A4 && pPL->p2A4->x5) {
        return;
    }
    PSVECSubtract(&pPL->getPartsPtr(1)->pos, &pPL->pos, &d);
    if (state != 0xB) {
        if (d.y <= 500.0f) {
            interp.set(3, &camera.param);
            state = 0xB;
            if (extra) {
                delete extra;
            }
            extra = new (extra_buf) CameraLookAt(&camera);
        }
    } else {
        if (d.y > 500.0f) {
            interp.set(30, &camera.param);
            flags_2C = 0x10;
        }
    }
}

void CameraControl::Move()
{
    static f32 gain = 2.0f;
    f32 water_y;
    f32 t;
    f32 lim;

    if (pG->flags_500C & 0x40000) {
        return;
    }
    if (!(flags_28 & 1)) {
        return;
    }
    flags_30 &= ~1;
    if (area_rec) {
        CalcAim(area_rec->cut);
    }
    switch (state) {
    case 0:
        r0_Wait();
        break;
    case 1:
        r0_Fix();
        break;
    case 2:
        r0_Pan();
        break;
    case 3:
        r0_Track();
        break;
    case 4:
        r0_RailPan();
        break;
    case 5:
        CamSmth.ratio = 0.0f;
        extra->move();
        cur = extra->param;
        if (((CameraMotion*) extra)->end == 1) {
            if (extra) {
                delete extra;
            }
            state = 0;
        }
        break;
    case 6:
        r0_RailBehind();
        break;
    case 7:
        r0_Free();
        break;
    case 8:
        r0_Debug();
        break;
    case 9:
        r0_UpCut();
        break;
    case 0xA:
        qfps.move();
        cur = qfps.cam.param;
        break;
    case 0xC:
        CamSmth.ratio = 0.0f;
        extra->move();
        cur = extra->param;
        break;
    case 0x10:
    case 0x11:
        CamSmth.ratio = 0.0f;
        extra->move();
        cur = extra->param;
        break;
    case 0xB:
    case 0xF:
        extra->move();
        cur = extra->param;
        break;
    case 0xD:
        CamSmth.ratio = 0.0f;
        extra->move();
        cur = extra->param;
        break;
    default:
        r0_Debug();
        break;
    }

    if (!(pG->flags_500C & 0x1000)) {
        if (GetWaterHeight(&cur.pos, &water_y)) {
            t = sinf(cur.fovy * PI / 360.0f) / cosf(cur.fovy * PI / 360.0f);
            lim = gain * (ZNEAR * t * 1.3333334f) + water_y;
            if (cur.pos.y < lim) {
                cur.pos.y = lim;
            }
        }
    }
    interp.move(&cur);
    if (interp.frame != 0) {
        CamSmth.flags &= ~1;
    }
    CamSmth.move(&interp.param);
    camera.param = *CamSmth.getParam();
    CameraSetOrientationRoll(&camera);
    if (!(pG->flags_60 & 0x10000000) && (flags_30 & 4)) {
        pG->Cam = CamCtrl.camera;
    }
}

void CameraControl::CalcAim(CameraCut* cut)
{
    static Vec offset0 = {0.0f, 1000.0f, 0.0f};

    switch (state) {
    case 0:
    case 1:
    case 5:
    case 9:
        break;
    default:
        if (cut->flags & 1) {
            PSVECAdd(&pPL->pos, &cut->aim_ofs, &aim);
        } else {
            PSVECAdd(&pPL->pos, &offset0, &aim);
        }
        break;
    }
}

f32 CameraControl::getCameraPitch()
{
    return 0.0f;
}

void CameraInterpolation::set(int f, CameraParam* p)
{
    frame = f;
    param = *p;
}

void CameraInterpolation::move(CameraParam* p)
{
    CameraParam tmp;
    f32 r, s;

    if (frame != 0) {
        r = 1.0f / (f32) frame;
        s = 1.0f - r;
        tmp = *p;
        PSVECScale(&param.pos, &param.pos, s);
        PSVECScale(&param.at, &param.at, s);
        param.roll *= s;
        param.fovy *= s;
        PSVECScale(&p->pos, &p->pos, r);
        PSVECScale(&p->at, &p->at, r);
        p->roll *= r;
        p->fovy *= r;
        PSVECAdd(&p->pos, &param.pos, &param.pos);
        PSVECAdd(&p->at, &param.at, &param.at);
        param.roll += p->roll;
        param.fovy += p->fovy;
        frame--;
    } else {
        frame = 0;
        param = *p;
    }
}

void CameraSmooth::init(CameraParam* p)
{
    param = *p;
}

void CameraSmooth::move(CameraParam* p)
{
    Vec tmp;

    if (flags & 1) {
        flags &= ~1;
        init(p);
        return;
    }
    PSVECAdd(&param.pos, &pG->quake_ofs, &param.pos);
    PSVECAdd(&param.at, &pG->quake_ofs, &param.at);
    PSVECScale(&param.pos, &param.pos, ratio);
    PSVECScale(&p->pos, &tmp, 1.0f - ratio);
    PSVECAdd(&param.pos, &tmp, &param.pos);
    PSVECScale(&param.at, &param.at, ratio);
    PSVECScale(&p->at, &tmp, 1.0f - ratio);
    PSVECAdd(&param.at, &tmp, &param.at);
    param.roll *= ratio;
    param.fovy *= ratio;
    param.roll = p->roll * (1.0f - ratio) + param.roll;
    param.fovy = p->fovy * (1.0f - ratio) + param.fovy;
}

void CameraControl::r0_Wait()
{
}

void CameraControl::r0_Fix()
{
    Camera cam;

    CameraSetCutData(&cam, area_rec->cut);
    cur = cam.param;
    CamSmth.flags |= 1;
    state = 0;
}

void CameraControl::r0_Pan()
{
    CameraParam p;
    CameraCut* cut = area_rec->cut;

    switch (sub_state) {
    case 0:
        p.pos = *cut->pos;
        p.roll = *cut->roll;
        p.fovy = *cut->fovy;
        p.at = aim;
        cur = p;
        CamSmth.ratio = smooth_ratio[1];
        CamSmth.flags |= 1;
        sub_state++;
    case 1:
        p.pos = camera.param.pos;
        p.roll = camera.param.roll;
        p.fovy = camera.param.fovy;
        p.at = aim;
        cur = p;
        break;
    }
}

void CameraControl::r0_Track()
{
    Camera cam;
    CameraBSpline* bs = &CamBSpline;
    CameraCut* cut = area_rec->cut;

    switch (sub_state) {
    case 0:
        Parametrize(cut, bs);
        searchRail(bs, cut, &aim, 0);
        BSpline(bs, &cam, 0);
        cur = cam.param;
        CamSmth.ratio = smooth_ratio[2];
        CamSmth.flags |= 1;
        sub_state++;
        break;
    case 1:
        searchRail(bs, cut, &aim, 0);
        BSpline(bs, &cam, 0);
        cur = cam.param;
        if (pG->debug_mode == 0xF) {
            debugDrawRail(cut);
        }
        break;
    }
}

void CameraControl::r0_RailPan()
{
    Camera cam;
    CameraBSpline* bs = &CamBSpline;
    CameraCut* cut = area_rec->cut;

    switch (sub_state) {
    case 0:
        Parametrize(cut, bs);
        searchRail(bs, cut, &aim, 0);
        BSpline(bs, &cam, 0);
        cam.param.at = aim;
        cur = cam.param;
        CamSmth.ratio = smooth_ratio[2];
        CamSmth.flags |= 1;
        sub_state++;
        break;
    case 1:
        searchRail(bs, cut, &aim, 0);
        BSpline(bs, &cam, 0);
        cam.param.at = aim;
        cur = cam.param;
        if (pG->debug_mode == 0xF) {
            debugDrawRail(cut);
        }
        break;
    }
}

void CameraControl::r0_UpCut()
{
}

void CamCtrlShoulderSetSearchFrame(s16 frame)
{
    CamCtrl.qfps.search_frame = frame;
    CamCtrl.qfps.search_count = 0;
}

void CamCtrlShoulderSetAim(Vec* aim)
{
    CamCtrl.qfps.shoulder_aim = *aim;
}

void CameraControl::resetCameraAngle()
{
    CameraQuasiFPS* q = &CamCtrl.qfps;

    q->angle_y = 0.0f;
    q->angle_x = 0.0f;
}

f32 CameraControl::getCameraDirection()
{
    f32 dir = CamCtrl.qfps.angle_x;
    CamCtrl.qfps.angle_x = 0.0f;
    return dir;
}

void CameraControl::UpCutCall(int no, Vec* pos, Vec* at, Vec* up, int sel)
{
    switch (sel) {
    case 0:
        data = (CameraDataHeader*) pG->pCoreCamData;
        break;
    case 1:
        data = (CameraDataHeader*) pG->pRoomCamData;
        break;
    }
    if (pos) {
        up_pos = *pos;
    }
    if (at) {
        up_at = *at;
    }
    if (up) {
        up_vec = *up;
    }
    CutCall(no);
}

void CameraControl::startPushObject()
{
    extra = new (extra_buf) CameraPushObject();
    state = 0xF;
    AreaCheckOnOff(0);
}

void CameraControl::endPushObject()
{
    if (extra) {
        delete extra;
    }
    Comeback(0);
}

void CameraControl::StartLookDownEm(void* em)
{
    Vec c;

    PSVECAdd(&pPL->getPartsPtr(0x20)->pos, &pPL->getPartsPtr(0x21)->pos, &c);
    PSVECScale(&c, &c, 0.5f);
    extra = new (extra_buf) CameraLookDownEm(em, &c);
    state = 0xD;
    AreaCheckOnOff(0);
    BitOff(pG->flags_500C, 0x2000000);
}

void CameraControl::EndLookDownEm()
{
    if (extra) {
        delete extra;
    }
    Comeback(0);
    BitOn(pG->flags_500C, 0x2000000);
}

void CameraControl::startScope()
{
    if (!(pG->flags_500C & 0x40)) {
        BitOn(pG->flags_500C, 0x40);
        BitOn(pG->flags_500C, 0x8000);
        extra = new (extra_buf) CameraScope();
        state = 0x10;
        BitOn(pG->flags_58, 0x40000000);
        AreaCheckOnOff(0);
    }
}

void CameraControl::endScope()
{
    if (pG->flags_500C & 0x40) {
        BitOff(pG->flags_500C, 0x40);
        BitOff(pG->flags_500C, 0x8000);
        BitOff(pG->flags_58, 0x40000000);
        if (extra) {
            delete extra;
        }
        Comeback(0);
    }
}

void CameraControl::getTrajectory(Vec* pos, Vec* at)
{
    if (pG->flags_500C & 0x40) {
        cCamera* c = extra;
        *pos = c->param.pos;
        *at = c->param.at;
    }
}

void CameraControl::saveScopeParam()
{
    ((CameraScope*) extra)->getParam(&scope_param0, &scope_param1);
    ((CameraScope*) extra)->id.save(0);
}

void CameraControl::loadScopeParam()
{
    ((CameraScope*) extra)->setParam(scope_param0, scope_param1);
    ((CameraScope*) extra)->id.load(0);
}

void CameraControl::SetBinocularRange(f32 range)
{
    ((CameraBinocular*) extra)->setRange(range);
}

void CameraControl::HoldBinocular(void* a, void* b, void* c, void* d)
{
    BitOn(pG->flags_500C, 0x400);
    BitOn(pG->flags_500C, 0x8000);
    extra = new (extra_buf) CameraBinocular(c, d, a, b);
    state = 0xC;
    BitOn(pG->flags_58, 0x40000000);
    AreaCheckOnOff(0);
}

void CameraControl::LowerBinocular()
{
    BitOff(pG->flags_500C, 0x400);
    BitOff(pG->flags_500C, 0x8000);
    BitOff(pG->flags_58, 0x40000000);
    if (extra) {
        delete extra;
    }
    Comeback(0);
}

void CameraControl::GetBinocularIDAddr(void** a, void** b)
{
    *a = ((CameraBinocular*) extra)->id_a;
    *b = ((CameraBinocular*) extra)->id_b;
}

void CameraControl::MotionSet(void* motion, int frame, f32 speed)
{
    BitOn(flags_2C, 0x28);
    BitOn(pG->flags_5014, 0x10000000);
    extra = new (extra_buf) CameraMotion(motion, 0, 0, speed);
    ((CameraMotion*) extra)->base_mat = NULL;
    state = 5;
    interp.set(frame, &pG->Cam.param);
}

int CameraControl::IsMotionSet()
{
    if (flags_2C & 0x20) {
        return 1;
    }
    return 0;
}

int CameraControl::IsMotionEnd()
{
    if (state != 5) {
        return 1;
    }
    return ((CameraMotion*) extra)->end == 1;
}

void CameraControl::setMotionBaseMatPtr(Mtx* mat)
{
    ((CameraMotion*) extra)->base_mat = mat;
}

void* CameraControl::getMotionInfoPtr()
{
    return &((CameraMotion*) extra)->info;
}

void CameraControl::clearAttachCamera()
{
    int i;

    attach_num = 0;
    attach_cur = NULL;
    for (i = 0; i < 3; i++) {
        attach_model[i] = NULL;
        attach_cam[i] = NULL;
    }
}

void CameraControl::registAttachCamera(AttachCamera* cam, cModel* model)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (model == attach_model[i]) {
            attach_cam[i] = cam;
            return;
        }
    }
    for (i = 0; i < 3; i++) {
        if (attach_model[i] == NULL) {
            attach_num++;
            attach_model[i] = model;
            attach_cam[i] = cam;
            return;
        }
    }
    pLog->err(0, 0, "registAttachCamera(): lack of ptr table.");
}

void CameraControl::deleteAttachCamera(AttachCamera* cam, cModel* model)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (model == attach_model[i] && cam == attach_cam[i]) {
            attach_num--;
            attach_model[i] = NULL;
            attach_cam[i] = NULL;
            return;
        }
    }
}

cModel* CameraControl::getAttachModel(cModel* model)
{
    int i;

    if (model == NULL) {
        for (i = 0; i < 3; i++) {
            if (attach_model[i] != NULL) {
                return attach_model[i];
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (model == attach_model[i]) {
                return model;
            }
        }
    }
    return NULL;
}

AttachCamera* CameraControl::getAttachCamera(cModel* model)
{
    int i;

    if (model == NULL) {
        for (i = 0; i < 3; i++) {
            if (attach_model[i] != NULL) {
                return attach_cam[i];
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (model == attach_model[i]) {
                return attach_cam[i];
            }
        }
    }
    return NULL;
}

void CameraControl::checkAttachCamera()
{
    static int inter_frame;
    cModel* em[3] = {NULL, NULL, NULL};
    cModel* model = NULL;
    AttachCamera* ac;
    int i;

    if (pG->flags_500C & 0x40) {
        return;
    }
    if (flags_30 & 4) {
        return;
    }
    switch (attach_num) {
    case 0:
        break;
    case 1:
        model = getAttachModel(NULL);
        break;
    default:
        for (i = 0; i < 2; i++) {
            model = getAttachModel(em[i]);
            if (model) {
                break;
            }
        }
        break;
    }
    if (model) {
        ac = getAttachCamera(model);
        if (attach_cur != model) {
            BitOn(flags_2C, 8);
            interp.set(ac->frame, &pG->Cam.param);
            state = 0x11;
            if (extra) {
                delete extra;
            }
            extra = new (extra_buf) CameraAttachedToMotion(model);
            extra->param.pos = pG->Cam.param.pos;
            extra->param.at = pG->Cam.param.at;
            extra->param.roll = pG->Cam.param.roll;
            extra->param.fovy = pG->Cam.param.fovy;
        }
        inter_frame = ac->frame;
    } else if (attach_cur) {
        flags_2C = 0x10;
        interp.set(inter_frame, &pG->Cam.param);
    }
    attach_cur = model;
}

int cameraDataVersion(char* data)
{
    if (strncmp(data, "B404", 4) == 0) {
        return 4;
    }
    if (strncmp(data, "B403", 4) == 0) {
        return 3;
    }
    if (strncmp(data, "B402", 4) == 0) {
        return 2;
    }
    if (strncmp(data, "B401", 4) == 0) {
        return 1;
    }
    if (strncmp(data, "B400", 4) == 0) {
        return 0;
    }
    strncmp(data, "EMPT", 4);
    return -1;
}
