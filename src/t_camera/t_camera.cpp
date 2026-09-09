#include "types.h"
#include "light.h"
#include "atari.h"
#include "global.h"
#include "main_mem.h"
#include "vec.h"
#include "camera.h"
#include "cam_ctrl.h"
#include "view.h"
#include "db_cam.h"
#include "player.h"
#include "eprintf.h"
#include "scheduler.h"
#include "db_light.h"
#include "t_camera.h"

// Camera tool entry object (D:/Bio4/Prog/t_camera.cpp): the task loop, menus and the camera / area
// editors. PARTIAL: the editors (tcEdit_select, tcEdit_area .. tcDrawRail, tcLoad, tcSave) are not
// written yet; see the report.

extern "C" {
void memclr_asm(void* p, u32 size);
void* memset(void* p, int c, unsigned int n);
}

#define TC_ON (*(u32*) ((u8*) pTc + 0x11C))
#define TC_TRG (*(u32*) ((u8*) pTc + 0x120))
#define TC_REP (*(u32*) ((u8*) pTc + 0x128))

static void tcInit();
static void tcMenu();
static void tcEdit();
void tcLoad();
void tcSave();
static void tcQuit();
void tcSubMenu();
void tcEdit_select();
void tcEdit_area();
void tcDrawArea();
void tcEdit_camera();
void tcDrawOffset();
void tcDrawRail();
TcCdat* tcNextCdatPtr(s8 no, int dir);
TcAdat* tcNextAdatPtr(s8 area, int cam, int dir);
int head_suffix(s8 area);
int tail_suffix(s8 area);
int next_suffix(s8 cam, int dir);
void tcCameraPullPoint(TcCdat* c);
void tcToolCameraMove(Camera* cam);
void tcPreviewOnOff(int on);

// camera type names (tcTypeTbl[..][0]) and the on/off pair
const char* tcTypeName[9] = {"FIX     ", "PAN     ", "TRACK   ", "RAIL PAN", "BEHIND  ", "FREE    ", "MOTION  ",
                             "UP CUT  ", "BESIDE  "};
const char* tcOnOff[2] = {"OFF", "ON"};
// menu positions: {x, y} in 8 / 14 pixel units for the main menu, sub menu, edit header, ...
int tcMenuPos[12] = {3, 2, 0x12, 0x11, 0x1C, 0x14, 0x28, 5, 0x28, 7, 0x1B, 0x12};
u8 tcTypeTbl[64][16];
TcAdat tcAdat[0x60];
TcCdat tcCdat[0x40];
TcLdat tcLdat[0x40];
static TcWork tcWork;
TcWork* pTc = &tcWork;
static void (*tcRoutineTbl[6])() = {tcInit, tcMenu, tcEdit, tcLoad, tcSave, tcQuit};
static const char* tcMainMenuName[4] = {"EDIT", "LOAD", "SAVE", "EXIT"};

void ToolCamera()
{
    tcInit();
    for (;;) {
        TcWork* w = pTc;
        if (w->lightTool == 0) {
            memcpy((u8*) w + 0x10C, &Joy[0], sizeof(JOY));
            memcpy((u8*) pTc + 0x374, &Joy[1], sizeof(JOY));
            switch (pTc->mode) {
            case 0:
                if (pTc->preview == 0) {
                    tcRoutineTbl[pTc->routine]();
                    tcToolCameraMove(&pTc->cam);
                    tcCameraDebugMove();
                } else {
                    tcDataExport((u8*) g_pToolCamData);
                    CamCtrl.RoomDataRead((CameraDataHeader*) g_pToolCamData);
                    CamCtrl.Check();
                    tcPlayerMove();
                    CameraMove();
                    LightMgr.move();
                    pTc->x634 = CamCtrl.camera_no;
                    pTc->x635 = CamCtrl.area_no;
                    pTc->x636 = CamCtrl.x691;
                    tcGameCamera2ToolCamera();
                }
                if (TC_TRG & 0x1000) {
                    pTc->mode = 1;
                    pTc->previewReq = pTc->preview;
                }
                break;
            case 1:
                tcSubMenu();
                if (TC_TRG & 0x1200) {
                    pTc->mode = 0;
                }
                break;
            }
            tcCameraMove();
        } else {
            int ret = w->pLightTool->move();
            if (ret == 0) {
                if (pTc->pLightTool) {
                    delete pTc->pLightTool;
                }
                pTc->lightTool = ret;
            }
        }
        pTc->blink++;
        TaskSleep(1);
    }
}

void tcDataInitialize()
{
    int i;

    for (i = 0x3F; i >= 0; i--) {
        tcTypeTbl[i][0] = 0;
    }
    for (i = 0x5F; i >= 0; i--) {
        tcAdat[i].enable = 0xFF;
    }
    for (i = 0x3F; i >= 0; i--) {
        tcCdat[i].enable = 0xFF;
    }
    for (i = 0x3F; i >= 0; i--) {
        tcLdat[i].enable = 0xFF;
    }
    pTc->adatNum = 0;
    pTc->cdatNum = 0;
    pTc->ldatNum = 0;
    for (i = 0; i < 0x40; i++) {
        pTc->adatTypeNum[i] = 0;
    }
}

static void tcInit()
{
    if (g_pToolCamData == 0) {
        g_pToolCamData = Debug_alloc(0x19000, 0);
    }
    BitOn(pG->flags_60, 0x20000000);
    BitOn(pG->flags_60, 0x10000000);
    BitOn(pG->flags_170, 0x400000);
    TaskSuspend(0);
    tcGameCameraStore();
    memclr_asm(pTc, sizeof(TcWork));
    tcGameCamera2ToolCamera();
    pTc->routine = 1;
    pTc->x637 = pSys->key_type;
    tcDataInitialize();
    pTc->cdatNo = -1;
    pTc->adatNo = -1;
    pTc->x5E1 = -1;
    pTc->x634 = CamCtrl.camera_no;
    pTc->x635 = CamCtrl.area_no;
    pTc->x636 = CamCtrl.x691;
    if (CamCtrl.data) {
        if (cameraDataVersion((char*) CamCtrl.data) > 1) {
            tcDataImport((u8*) CamCtrl.data);
        }
    }
}

static void tcMenu()
{
    int x = tcMenuPos[0];
    int y = tcMenuPos[1];
    int i;
    int row;

    if (TC_REP & 0xC) {
        pTc->blink = 8;
    }
    if (TC_REP & 0x8) {
        pTc->cursor--;
    }
    if (TC_REP & 0x4) {
        pTc->cursor++;
    }
    pTc->cursor = pTc->cursor < 0 ? 3 : (pTc->cursor > 3 ? 0 : pTc->cursor);
    if (TC_TRG & 0x200) {
        pTc->blink = 8;
        pTc->cursor = 3;
    }
    if (TC_TRG & 0x100) {
        pTc->editSel = 1;
        switch (pTc->cursor) {
        case 0:
            pTc->routine = 2;
            break;
        case 1:
            pTc->routine = 3;
            break;
        case 2:
            pTc->routine = 4;
            break;
        case 3:
            pTc->routine = 5;
            break;
        }
    }
    eprintf(x * 8, y * 14, 4, 0, "Main Menu --- Tool Ver %1.2f", 1.1);
    y++;
    for (i = 0; i < 4; i++) {
        row = y * 14;
        if (i == pTc->cursor && (pTc->blink & 0x18)) {
            eprintf((x - 1) * 8, row, 0, 0, ">");
        }
        eprintf(x * 8, row, 0, 0, "%s", tcMainMenuName[i]);
        y++;
    }
}

void tcSubMenu()
{
    TcMenu menu[6] = {{1, "PREVIEW    :"}, {1, "LIGHT TOOL :"}, {1, "VIEW MODE  :"},
                      {1, "AREA DETAIL:"}, {1, "BATTLE CAM :"}, {1, "PROJECTION :"}};
    int x = tcMenuPos[2];
    int y = tcMenuPos[3];
    int i;
    int row;

    eprintf(x * 8, y * 14, 5, 0, "SUB MENU ---------");
    y++;
    tcMenuSelect(x * 8, y * 14, 0, menu, 6, &pTc->subCursor);
    if (TC_TRG & 0x100) {
        pTc->mode = 0;
    }
    switch (pTc->subCursor) {
    case 0:
        if (TC_TRG & 0x1) {
            pTc->previewReq = 1;
        }
        if (TC_TRG & 0x2) {
            pTc->previewReq = 0;
        }
        if (TC_TRG & 0x100) {
            tcPreviewOnOff(pTc->previewReq);
        }
        break;
    case 1:
        if (TC_TRG & 0x100) {
            pTc->pLightTool = new cLightTool();
            pTc->lightTool = 1;
        }
        break;
    case 2:
        if (TC_TRG & 0x3) {
            pTc->viewMode = pTc->viewMode == 0;
            if (pTc->viewMode == 0) {
                tcCameraPullPoint(tcCdatPtr(pTc->cdatNo));
            }
        }
        break;
    case 3:
        if (TC_TRG & 0x1) {
            pTc->areaDetail = 1;
        }
        if (TC_TRG & 0x2) {
            pTc->areaDetail = 0;
        }
        break;
    case 4:
        if (pG->flags_6C & 0x40000000) {
            if (TC_TRG & 0x2) {
                pG->flags_6C &= ~0x40000000;
            }
        } else {
            if (TC_TRG & 0x1) {
                pG->flags_6C |= 0x40000000;
            }
        }
        break;
    case 5:
        switch (CameraGetProjection()) {
        case 1:
            if (TC_TRG & 0x3) {
                CameraSetProjection(2);
            }
            break;
        case 2:
            if (TC_TRG & 0x3) {
                CameraSetProjection(1);
            }
            break;
        }
        break;
    }
    x += 12;
    for (i = 0; i < 6; i++) {
        row = y * 14;
        switch (i) {
        case 0:
            if (pTc->previewReq == 0) {
                eprintf(x * 8, row, 0, 0, "---/OFF");
            } else {
                eprintf(x * 8, row, 0, 0, "ON-/---");
            }
            break;
        case 2:
            if (pTc->viewMode) {
                eprintf(x * 8, row, 0, 0, "------------/WORKING VIEW");
            } else {
                eprintf(x * 8, row, 0, 0, "CAMERA VIEW-/------------");
            }
            break;
        case 3:
            if (pTc->areaDetail == 0) {
                eprintf(x * 8, row, 0, 0, "---/OFF");
            } else {
                eprintf(x * 8, row, 0, 0, "ON-/---");
            }
            break;
        case 4:
            if (pG->flags_6C & 0x40000000) {
                eprintf(x * 8, row, 0, 0, "ON-/---");
            } else {
                eprintf(x * 8, row, 0, 0, "---/OFF");
            }
            break;
        case 5:
            if (CameraGetProjection() == 1) {
                eprintf(x * 8, row, 0, 0, "PERSPECTIVE");
            } else {
                eprintf(x * 8, row, 0, 0, "ORTHOGRAPHIC");
            }
            break;
        }
        y++;
    }
}

static void tcEdit()
{
    TcWork* w = pTc;

    switch (w->editMode) {
    case 0:
        tcEdit_select();
        break;
    case 1:
        if (TC_ON & 0x10) {
            if (w->editSel == 1) {
                TcCdat* c = tcCdatPtr(w->cdatNo);
                int x;
                int y;
                if (TC_REP & 0x1) {
                    c = tcNextCdatPtr(pTc->cdatNo, -1);
                }
                if (TC_REP & 0x2) {
                    c = tcNextCdatPtr(pTc->cdatNo, 1);
                }
                pTc->cdatNo = c->cam_no;
                pTc->adatNo = pTc->cdatNo;
                pTc->x5E1 = head_suffix(pTc->adatNo);
                x = tcMenuPos[4];
                y = tcMenuPos[5] * 14;
                eprintf(x * 8, y, 5, 0, "Camera[  ]");
                x += 7;
                eprintf(x * 8, y, 0, 0, "%02d", pTc->cdatNo);
            } else if (w->editSel == 0) {
                int x;
                int y;
                if (TC_REP & 0x1) {
                    pTc->pAdat = tcNextAdatPtr(w->adatNo, w->x5E1, -1);
                }
                if (TC_REP & 0x2) {
                    pTc->pAdat = tcNextAdatPtr(pTc->adatNo, pTc->x5E1, 1);
                }
                pTc->adatNo = pTc->pAdat->area_no;
                pTc->x5E1 = pTc->pAdat->cam_no;
                pTc->cdatNo = pTc->adatNo;
                x = tcMenuPos[4];
                y = tcMenuPos[5] * 14;
                if ((s8) pTc->adatTypeNum[pTc->adatNo] <= 1) {
                    eprintf(x * 8, y, 5, 0, "Area[  ]");
                    x += 5;
                    eprintf(x * 8, y, 0, 0, "%02d", pTc->adatNo);
                } else {
                    eprintf(x * 8, y, 5, 0, "Area[    ]");
                    x += 5;
                    eprintf(x * 8, y, 0, 0, "%02d-%1d", pTc->adatNo, pTc->x5E1);
                }
            }
            if (pTc->editSel == 1 && (s8) pTc->viewMode == 0) {
                static s8 lastCam;
                if (lastCam != pTc->cdatNo) {
                    TcAdat* a;
                    int no;
                    lastCam = pTc->cdatNo;
                    pTc->x2 = 0;
                    pTc->x6 = 0;
                    pTc->x627 = 0;
                    tcCameraPullPoint(tcCdatPtr(pTc->cdatNo));
                    no = pTc->cdatNo;
                    a = tcAdatPtr(no, (s8) head_suffix(no));
                    if (!(a->attr & 0x80)) {
                        LightMgr.update(no, -1);
                    }
                }
            }
        } else {
            switch (w->editSel) {
            case 0:
                tcEdit_area();
                break;
            case 1:
                tcEdit_camera();
                break;
            }
        }
        break;
    }
    tcDrawArea();
    tcDrawOffset();
    tcDrawRail();
}

TcCdat* tcCdatNew()
{
    int i;

    for (i = 0; i < 0x40; i++) {
        if (tcCdat[i].enable == 0xFF) {
            tcCdat[i].enable = 1;
            pTc->cdatNum++;
            return &tcCdat[i];
        }
    }
    return 0;
}

void tcCdatDel(TcCdat* c)
{
    c->enable = 0xFF;
    pTc->cdatNum--;
}

void tcCdatInit(TcCdat* c, int cam_no)
{
    Camera* cam = &pTc->cam;
    int i;

    c->type = 2;
    c->cam_no = cam_no;
    c->flags = 0;
    c->aim_ofs.x = 0.0f;
    c->aim_ofs.y = 1000.0f;
    c->aim_ofs.z = 0.0f;
    c->num = 1;
    for (i = 0; i < 2; i++) {
        c->at[i] = cam->param.at;
        c->pos[i] = cam->param.pos;
        c->roll[i] = cam->param.roll;
        c->fovy[i] = cam->param.fovy;
    }
    for (i = 25; i >= 0; i--) {
        c->frame[i] = 0;
    }
}

TcCdat* tcCdatPtr(int cam_no)
{
    int i;

    for (i = 0; i < 0x40; i++) {
        TcCdat* c = &tcCdat[i];
        if (c->enable != 0xFF && cam_no == c->cam_no) {
            return c;
        }
    }
    return 0;
}

TcAdat* tcAdatNew()
{
    int i;

    for (i = 0; i < 0x60; i++) {
        if (tcAdat[i].enable == 0xFF) {
            tcAdat[i].enable = 1;
            pTc->adatNum++;
            return &tcAdat[i];
        }
    }
    return 0;
}

void tcAdatDel(TcAdat* a)
{
    a->enable = 0xFF;
    pTc->adatNum--;
}

void tcAdatInit(TcAdat* a, int area_no, int cam_no)
{
    Mtx m;
    Vec axis = {0.0f, 1.0f, 0.0f};
    Vec pos;
    TcPoly* poly = &a->poly;
    int i;

    a->cam_no = cam_no;
    a->area_no = area_no;
    a->attr = 0;
    pTc->adatTypeNum[area_no]++;
    a->attr2 = 1;
    a->x9 = 0xFF;
    PSMTXIdentity(a->mat);
    a->height = 1000.0f;
    a->base_y = 0.0f;
    a->num = 4;
    poly->pt[0].z = 2000.0f;
    poly->pt[0].x = 2000.0f;
    poly->pt[1].z = -2000.0f;
    poly->pt[1].x = 2000.0f;
    poly->pt[2].z = -2000.0f;
    poly->pt[2].x = -2000.0f;
    poly->pt[3].x = -2000.0f;
    poly->pt[3].z = 2000.0f;
    pos = pPL->pos;
    PSMTXIdentity(m);
    PSMTXRotAxisRad(m, &axis, pPL->rot.y);
    PSMTXTransApply(m, m, pos.x, pos.y, pos.z);
    for (i = 0; i < 4; i++) {
        PSMTXMultVec(m, &a->pt[i], &a->pt[i]);
    }
    poly->pt[0].y = poly->pt[1].y = poly->pt[2].y = poly->pt[3].y = a->base_y;
}

TcAdat* tcAdatPtr(int area_no, int cam_no)
{
    int i;
    TcAdat* a = tcAdat;

    for (i = 0; i < 0x60; i++, a++) {
        if (a->enable != 0xFF && area_no == a->area_no && cam_no == a->cam_no) {
            return a;
        }
    }
    return 0;
}

TcLdat* tcLdatNew()
{
    int i;

    for (i = 0; i < 0x40; i++) {
        if (tcLdat[i].enable == 0xFF) {
            tcLdat[i].enable = 1;
            pTc->ldatNum++;
            return &tcLdat[i];
        }
    }
    return 0;
}

void tcLdatDel(TcLdat* l)
{
    l->enable = 0xFF;
    pTc->ldatNum--;
}

void tcLdatInit(TcLdat* l, int area_from, int cam_from, int area_to, int cam_to, int frame)
{
    l->area_from = area_from;
    l->cam_from = cam_from;
    l->area_to = area_to;
    l->cam_to = cam_to;
    l->frame = frame;
}

TcLdat* tcLdatPtr(int area_from, int cam_from, int area_to, int cam_to)
{
    int i;
    TcLdat* l = tcLdat;

    for (i = 0; i < 0x40; i++, l++) {
        if (l->enable != 0xFF && area_from == l->area_from && cam_from == l->cam_from && area_to == l->area_to &&
            cam_to == l->cam_to) {
            return l;
        }
    }
    return 0;
}

TcCdat* tcNextCdatPtr(s8 no, int dir)
{
    int i;
    s8 n = no;

    for (i = 0; i < 0x40; i++) {
        TcCdat* c;
        n = (s8) (n + dir);
        n = (n + 0x40) % 0x40;
        c = tcCdatPtr(n);
        if (c) {
            return c;
        }
    }
    return 0;
}

TcAdat* tcNextAdatPtr(s8 area, int cam, int dir)
{
    s8 n = area;
    int suffix = next_suffix(cam, dir);

    if ((dir > 0 && suffix > cam) || (dir < 0 && suffix < cam)) {
        return tcAdatPtr(n, suffix);
    }
    do {
        n = (s8) (n + dir);
        n = (n + 0x40) % 0x40;
    } while (pTc->adatTypeNum[n] == 0);
    if (dir > 0) {
        suffix = head_suffix(n);
    } else {
        suffix = 0;
        if (dir >= 0) {
            return 0;
        }
        suffix = tail_suffix(n);
    }
    return tcAdatPtr(n, suffix);
}

int head_suffix(s8 area)
{
    int i;

    for (i = 0; i < 8; i++) {
        s8 s = i;
        if (tcAdatPtr(area, s)) {
            return s;
        }
    }
    return -1;
}

int tail_suffix(s8 area)
{
    int i;

    for (i = 7; i >= 0; i--) {
        s8 s = i;
        if (tcAdatPtr(area, s)) {
            return s;
        }
    }
    return -1;
}

int next_suffix(s8 cam, int dir)
{
    int i;
    s8 n = cam;

    if (n == -1) {
        return -1;
    }
    for (i = 0; i < 8; i++) {
        n = (s8) (n + dir);
        n = (n + 8) % 8;
        if (tcAdatPtr(pTc->adatNo, n)) {
            return n;
        }
    }
    return 0;
}

static void tcQuit()
{
    if (*(u16*) &pTc->cdatNum != 0) {
        tcDataExport((u8*) g_pToolCamData);
        CamCtrl.RoomDataRead((CameraDataHeader*) g_pToolCamData);
        CamCtrl.flags_2C = (CamCtrl.flags_2C & ~1) | 0x10;
    }
    BitOff(pG->flags_60, 0x80000000);
    BitOff(pG->flags_60, 0x20000000);
    BitOff(pG->flags_60, 0x10000000);
    BitOff(pG->flags_170, 0x400000);
    pSys->key_type = pTc->x637;
    CameraSetProjection(1);
    if (!(Joy[0].on & 0x400)) {
        CamCtrl.Comeback(0);
    }
    tcGameCameraLoad();
    TaskSignal(0);
    TaskExit();
}

static f32 tcDollySpeed = 5.0f;
static f32 tcDollyDummy = 0.0f;

void tcToolCameraMove(Camera* cam)
{
    Vec d = {0.0f, 0.0f, 0.0f};
    Vec axis = {0.0f, 1.0f, 0.0f};
    f32 z;

    if (TC_ON & 0x60) {
        if (TC_ON & 0x20) {
            z = (f32) -(int) pTc->joy.trigR;
        } else {
            z = (f32) pTc->joy.trigL;
        }
        z *= tcDollySpeed;
        switch (CameraGetProjection()) {
        case 1: {
            f32 dist = cam->dist + z;
            if (dist < 500.0f) {
                d.z = dist - 500.0f;
                dist = 500.0f;
            }
            if (pTc->x62F) {
                CameraTargetDistance(cam, dist);
            } else {
                CameraCamposDistance(cam, dist);
            }
            break;
        }
        case 2:
            ORTHO_T = z * 3.0f * 0.25f + ORTHO_T;
            ORTHO_L = ORTHO_L - z;
            ORTHO_B = -ORTHO_T;
            ORTHO_R = -ORTHO_L;
            break;
        }
    }
    if (pTc->joy.ssx) {
        d.x = (f32) pTc->joy.ssx * 5.0f;
    }
    if (pTc->joy.ssy) {
        d.y = (f32) pTc->joy.ssy * 5.0f;
    }
    if (d.x != 0.0f || d.y != 0.0f || d.z != 0.0f) {
        PSMTXMultVecSR(cam->mat, &d, &d);
        CameraDolly(cam, &d);
    }
    if (pTc->joy.sx) {
        if (pTc->x62F) {
            CameraRotAxisPosRad(cam, &axis, &cam->param.pos, (f32) pTc->joy.sx / 20.0f * 0.017453292f);
        } else {
            CameraRotAxisPosRad(cam, &axis, &cam->param.at, (f32) pTc->joy.sx / 20.0f * 0.017453292f);
        }
    }
    if (pTc->joy.sy) {
        if (pTc->x62F) {
            CameraTargetRot(cam, 'x', (f32) pTc->joy.sy / -20.0f * 0.017453292f);
        } else {
            CameraCamposRot(cam, 'x', (f32) pTc->joy.sy / -20.0f * 0.017453292f);
        }
    }
    CameraDrawTarget(cam, 1);
    drawGround(0);
}

void tcPreviewOnOff(int on)
{
    pTc->preview = on;
    CamCtrl.flags_2C = (CamCtrl.flags_2C & ~1) | 0x10;
    if (pTc->preview) {
        pG->flags_60 &= ~0x10000000;
    } else {
        pG->flags_60 |= 0x10000000;
    }
}

int tcCurrentCameraNo()
{
    return pTc->cdatNo;
}
