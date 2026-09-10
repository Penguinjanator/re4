#include "types.h"
#include "global.h"
#include "db_log.h"
#include "main_mem.h"
#include "vec.h"
#include "camera.h"
#include "cam_ctrl.h"
#include "cam_qfps.h"
#include "db_cam.h"
#include "player.h"
#include "eprintf.h"
#include "dbmodule.h"
#include "light.h"
#include "atari.h"
#include "t_camera.h"

extern "C" {
int sprintf(char* buf, const char* fmt, ...);
char* strncpy(char* dst, const char* src, unsigned int n);
}

// Camera tool: room camera data export/import, game camera bridge, drawing primitives (t_camera REL).

void tcGetFileName(char* path, int no, int flag)
{
    if (flag & 2) {
        if (flag & 1) {
            sprintf(path, "X:/Soft/Room/Etc/Core/core00.cam");
        } else {
            sprintf(path, "y:/Room/Etc/Core/core00.cam");
        }
    } else {
        if (flag & 1) {
            sprintf(path, "X:/Soft/Room/St%x/R%x%02x/r%x%02x%02d.cam", pG->stage_no, pG->stage_no, pG->room_no,
                    pG->stage_no, pG->room_no, no);
        } else {
            sprintf(path, "y:/Room/St%x/R%x%02x/r%x%02x%02d.cam", pG->stage_no, pG->stage_no, pG->room_no,
                    pG->stage_no, pG->room_no, no);
        }
    }
}

int tcDataExport(u8* buf)
{
    CameraDataHeader* hdr = (CameraDataHeader*) buf;
    CameraAreaRec* rec = (CameraAreaRec*) (buf + 0x10);
    CameraAreaRec* r;
    CameraAreaInfo* area;
    CameraCut* cut;
    CameraLerp* lerp;
    Vec* vp;
    Vec* pos;
    u16* fp;
    TcAdat* a;
    TcCdat* c;
    TcLdat* l;
    int i;
    int j;
    int num;
    int size;
    const char* const tag = "B404";  // parsed before "EMPT": the .rodata order is B404, EMPT (uses fold to the literal)

    memclr_asm(buf, 0x10);
    if (*(u16*) &pTc->cdatNum == 0) {
        strncpy((char*) buf, "EMPT", 4);
        return 4;
    }
    strncpy((char*) buf, tag, 4);
    hdr->numCut = pTc->cdatNum;
    hdr->numArea = pTc->adatNum;
    hdr->numLerp = pTc->ldatNum;
    area = (CameraAreaInfo*) (rec + hdr->numArea);
    cut = (CameraCut*) (area + hdr->numArea);
    lerp = (CameraLerp*) (cut + hdr->numCut);
    vp = (Vec*) (lerp + hdr->numLerp);

    {
        CameraAreaInfo* d = area;
        for (i = 0; i < 0x60; i++) {
            a = &tcAdat[i];
            if (a->enable != 0xFF) {
                d->enable = a->enable;
                d->area_no = a->area_no;
                d->camera_no = a->cam_no;
                d->attr = tcTypeTbl[a->area_no][0];
                d->dir = a->dir;
                d->attr2 = a->attr2;
                d->x9 = a->x9;
                d->height = a->height;
                d->base_y = a->base_y;
                num = a->num;
                d->points = (Vec*) ((u8*) vp - buf);
                d->num = num;
                for (j = 0; j < a->num; j++) {
                    *vp++ = a->pt[j];
                }
                d++;
            }
        }
    }
    pos = (Vec*) vp;
    {
        CameraCut* d = cut;
        for (i = 0; i < 0x40; i++) {
            TcCdat* cd = &tcCdat[i];
            if (cd->enable != 0xFF) {
                Vec* pp;
                Vec* at;
                f32* roll;
                f32* fovy;
                d->x0 = cd->enable;
                d->camera_no = cd->cam_no;
                d->type = cd->type;
                d->num = cd->num;
                d->flags = cd->flags;
                d->aim_ofs = cd->aim_ofs;
                switch (cd->type) {
                case 4:
                    *(Vec*) &d->floor_ratio = cd->u44.dir;
                    break;
                case 8:
                    d->floor_ratio = cd->u44.floor;
                    break;
                }
                pp = pos;
                at = pp + cd->num;
                roll = (f32*) (at + cd->num);
                fovy = roll + cd->num;
                d->pos = (Vec*) ((u8*) pp - buf);
                d->at = (Vec*) ((u8*) at - buf);
                d->roll = (f32*) ((u8*) roll - buf);
                d->fovy = (f32*) ((u8*) fovy - buf);
                for (j = 0; j < cd->num; j++) {
                    *pp++ = cd->pos[j];
                    *at++ = cd->at[j];
                    *roll++ = cd->roll[j];
                    *fovy++ = cd->fovy[j];
                }
                pos = (Vec*) fovy;
                d++;
            }
        }
    }
    {
        CameraLerp* d = lerp;
        for (i = 0; i < 0x40; i++) {
            l = &tcLdat[i];
            if (l->enable != 0xFF) {
                *d = *(CameraLerp*) l;
                d++;
            }
        }
    }
    fp = (u16*) pos;
    {
        CameraCut* d = cut;
        for (i = 0; i < pTc->cdatNum; i++) {
            c = &tcCdat[i];
            if (c->enable != 0xFF) {
                if (c->type == 6 || c->type == 7) {
                    d->frames = (u16*) ((u8*) fp - buf);
                    for (j = 0; j < c->num; j++) {
                        *fp++ = c->frame[j];
                    }
                }
                d++;
            }
        }
    }
    size = (u8*) fp - buf;
    {
        CameraAreaInfo* d = area;
        r = rec;
        for (i = 0; i < pTc->adatNum;) {
            int found = 0;
            s8 no = d->area_no;
            CameraCut* cc = cut;
            r->area = (CameraAreaInfo*) ((u8*) d - buf);
            i++;
            for (j = 0; j < pTc->cdatNum; j++, cc++) {
                if (no == cc->camera_no) {
                    r->cut = (CameraCut*) ((u8*) cc - buf);
                    found = 1;
                    r->type = tcTypeTbl[no][0];
                    break;
                }
            }
            if (found == 0) {
                r->cut = (CameraCut*) found;
                d->enable = found;
            }
            r++;
            d++;
        }
    }
    return size;
}

int tcDataImport(u8* buf)
{
    CameraDataHeader* hdr = (CameraDataHeader*) buf;
    CameraAreaRec* rec;
    CameraAreaInfo* area;
    CameraCut* cut;
    CameraLerp* lerp;
    u8 cnt[64];
    int ver;
    int lo;
    int i;
    int j;

    ver = cameraDataVersion((char*) buf);
    if (ver == -1) {
        pTc->adatNum = 0;
        return -1;
    }
    if (ver < -1) {
        pTc->adatNum = 0;
        return -1;
    }
    if (ver > 4) {
        pTc->adatNum = 0;
        return -1;
    }
    // `2` through a local: the tree folder would turn `ver < 2` into `ver <= 1`, but the target
    // shares one `cmpwi 2` (cr0 kept in r25) with the `ver <= 2` test inside the area loop.
    lo = 2;
    if (ver < lo) {
        pTc->adatNum = 0;
        return -1;
    }
    pTc->adatNum = 0;
    pTc->cdatNum = 0;
    pTc->ldatNum = 0;
    for (i = 0; i < 64; i++) cnt[i] = 0;
    rec = (CameraAreaRec*) (buf + 0x10);

    area = (CameraAreaInfo*) (rec + hdr->numArea);
    cut = (CameraCut*) (area + hdr->numArea);
    lerp = (CameraLerp*) (cut + hdr->numCut);

    {
        CameraAreaRec* r = rec;
        for (i = 0; i < hdr->numArea; i++) {
            if (r->cut) {
                tcTypeTbl[r->cut->camera_no][0] = r->type;
            } else {
                tcTypeTbl[r->area->area_no][0] = r->type;
            }
            r++;
        }
    }
    {
        CameraAreaInfo* s = area;
        for (i = 0; i < hdr->numArea; i++) {
            TcAdat* a = tcAdatNew();
            a->area_no = s->area_no;
            a->cam_no = s->camera_no;
            pTc->adatTypeNum[s->area_no]++;
            a->height = s->height;
            a->base_y = s->base_y;
            a->num = s->num;
            if (ver <= 2) {
                a->attr = 3;
                tcTypeTbl[s->area_no][0] = 3;
            } else {
                a->attr = s->attr;
            }
            a->dir = s->dir;
            if (ver <= 3) {
                a->attr2 = 1;
                a->x9 = 0xFF;
            } else {
                a->attr2 = s->attr2;
                a->x9 = s->x9;
            }
            {
                Vec* pt = s->points;
                for (j = 0; j < s->num; j++) {
                    a->pt[j] = *pt++;
                }
            }
            s++;
        }
    }
    {
        CameraCut* s = cut;
        for (i = 0; i < hdr->numCut; i++) {
            TcCdat* c = tcCdatNew();
            c->cam_no = s->camera_no;
            cnt[s->camera_no]++;
            if (cnt[s->camera_no] != 1) {
                pLog.p->err(0, 0, "Camera[%02d] is duplicate.", s->camera_no);
            }
            c->type = s->type;
            c->num = s->num;
            c->aim_ofs = s->aim_ofs;
            c->flags = s->flags;
            switch (s->type) {
            case 4:
                c->u44.dir = *(Vec*) &s->floor_ratio;
                break;
            case 8:
                c->u44.floor = s->floor_ratio;
                break;
            }
            {
                Vec* pos = s->pos;
                Vec* at = s->at;
                f32* roll = s->roll;
                f32* fovy = s->fovy;
                u16* frames = s->frames;
                for (j = 0; j < s->num; j++) {
                    c->pos[j] = *pos++;
                    c->at[j] = *at++;
                    c->roll[j] = *roll++;
                    c->fovy[j] = *fovy++;
                    if (s->type == 6 || s->type == 7) {
                        c->frame[j] = *frames++;
                    }
                }
            }
            s++;
        }
    }
    {
        CameraLerp* s = lerp;
        for (i = 0; i < hdr->numLerp; i++) {
            TcLdat* l = tcLdatNew();
            *(CameraLerp*) l = *s;
            l->x5 = 0;
            s++;
        }
    }
    if (hdr->numArea != 0) {
        pTc->cdatNo = rec->area->area_no;
        pTc->adatNo = pTc->cdatNo;
        pTc->x5E1 = 0;
        pTc->pAdat = tcAdatPtr(pTc->adatNo, pTc->x5E1);
    }
    return 0;
}

f32 tcGetFloor()
{
    return 100.0f;
}

struct TcPreviewWork {
    int blink;
    int x4;
};
static TcPreviewWork tcPreview = {8, 0};

void tcPlayerMove()
{
    if (tcPreview.blink <= 15) {
        eprintf(0xD8, 0xFC, 5, 0, "[ PREVIEW ]");
    }
    tcPreview.blink++;
    if (tcPreview.blink > 31) tcPreview.blink = 0;
    eprintf(0xD8, 0x10A, 0, 0, "C:%02d", pTc->x634);
    eprintf(0x100, 0x10A, 0, 0, "A:%02d-%1d", pTc->x635, pTc->x636);
    pPL->move();
}

void tcCameraDebugMove()
{
    tcToolCamera2GameCamera();
    CamDbg.move(&pG->Cam, &Joy[1], 0);
    tcGameCamera2ToolCamera();
}

void tcGameCamera2ToolCamera()
{
    pTc->cam = pG->Cam;
}

void tcToolCamera2GameCamera()
{
    pG->Cam = pTc->cam;
}

Camera tcGameCamera;

void tcGameCameraStore()
{
    tcGameCamera = pG->Cam;
}

void tcGameCameraLoad()
{
    pG->Cam = tcGameCamera;
}

void tcDrawLine3D(Vec* a, Vec* b, u32 color)
{
    Draw_line3d(a, b, (color >> 8) | (color << 24), 0);
}

void tcDrawSphere(Vec* pos, u32 color, f32 r)
{
    Draw_sphere(pos, r, color, 1, 1);
}

void tcDrawPoly(Vec* p, u32 color)
{
    Draw_poly(p, (color >> 8) | (color << 24), 1);
}

void tcSetBesideFloor(f32 ratio)
{
    tcCdatPtr(pTc->cdatNo)->u44.floor = ratio;
}

void tcSetBesideOffset(QfpsOfs (*ready)[3], QfpsOfs (*trans)[3])
{
    TcCdat* c = tcCdatPtr(pTc->cdatNo);
    int n = 0;
    int i;
    int j;

    c->num = 24;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            QfpsOfs* o = i <= 1 ? &ready[i][j] : &trans[i - 2][j];
            c->pos[n] = o->campos;
            c->at[n] = o->target;
            c->roll[n] = o->x24;
            c->fovy[n] = o->fovy;
            n++;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            QfpsOfs* o = i <= 1 ? &ready[i][j] : &trans[i - 2][j];
            c->pos[n++] = o->campos2;
        }
    }
}

void tcSetBesideCamera()
{
    QfpsOfs ready[2][3];
    QfpsOfs trans[2][3];
    TcCdat* c = tcCdatPtr(pTc->cdatNo);
    int i;
    int j;
    int n = 0;

    CamCtrl.qfps.setAreaData(g_readyOfs[0], g_transOfs[0]);
    CamCtrl.qfps.getAreaData(ready, trans);
    if (c->num == 24) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++, n++) {
                QfpsOfs* o = i <= 1 ? &ready[i][j] : &trans[i - 2][j];
                if (c->flags & 0x20) {
                    if (i > 1) continue;
                } else if (!(c->flags & 0x10)) {
                    if (i <= 1) continue;
                }
                o->campos = c->pos[n];
                o->target = c->at[n];
                o->x24 = c->roll[n];
                o->fovy = c->fovy[n];
            }
        }
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++, n++) {
                QfpsOfs* o = i <= 1 ? &ready[i][j] : &trans[i - 2][j];
                if (c->flags & 0x20) {
                    if (i > 1) continue;
                } else if (!(c->flags & 0x10)) {
                    if (i <= 1) continue;
                }
                o->campos2 = c->pos[n];
            }
        }
    } else {
        c->num = n;
    }
    CamCtrl.qfps.setAreaData(ready, trans);
    CamCtrl.qfps.setFloorRatio(c->u44.floor);
}
