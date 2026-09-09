#ifndef T_CAMERA_H
#define T_CAMERA_H

#include "types.h"
#include "vec.h"
#include "camera.h"
#include "joy.h"
#include "cam_ctrl.h"
#include "db_light.h"

// game/cam_ctrl.cpp (declared here, not in cam_ctrl.h: a header extern would reorder cam_ctrl's .bss)
extern CameraBSpline CamBSpline;

// Camera tool (t_camera REL: t_camera.cpp, t_camera_data.cpp, t_camera_draw.cpp).

// One menu line of tcMenuSelect (8 bytes).
struct TcMenu {
    u8 enable;         // 0x00  0 = greyed out (colour 0x14) and not selectable
    const char* name;  // 0x04
};

// Tool-side records (the file records of cam_ctrl.h with the arrays inline).
struct TcPoly {
    s32 num;                     // 0x00
    Vec pt[16];                  // 0x04
};

struct TcAdat {                  // camera hit area, 0x11C
    u8 enable;                   // 0x00  0xFF = free
    s8 area_no;                  // 0x01
    s8 cam_no;                   // 0x02
    u8 attr;                     // 0x03
    f32 dir;                     // 0x04
    u8 attr2;                    // 0x08
    u8 x9;                       // 0x09
    u8 pad_A[0x20 - 0xA];
    Mtx mat;                     // 0x20
    f32 height;                  // 0x50
    f32 base_y;                  // 0x54
    union {
        struct {
            s32 num;             // 0x58
            Vec pt[16];          // 0x5C
        };
        TcPoly poly;             // 0x58  (tcAdatInit writes the corners through this view)
    };
};

struct TcCdat {                  // camera cut, 0x394
    u8 enable;                   // 0x00  0xFF = free
    s8 type;                     // 0x01
    s8 cam_no;                   // 0x02
    u8 flags;                    // 0x03
    Vec aim_ofs;                 // 0x04
    u16 frame[26];               // 0x10  rail key frames (type 6/7)
    union {
        f32 floor;               // 0x44  shoulder camera floor ratio (type 8: CameraCut::floor_ratio)
        Vec dir;                 // 0x44  type 4
    } u44;
    s32 num;                     // 0x50  key count
    Vec pos[26];                 // 0x54
    Vec at[26];                  // 0x18C
    f32 roll[26];                // 0x2C4
    f32 fovy[26];                // 0x32C
};

struct TcLdat {                  // camera lerp, 0x10 (CameraLerp)
    u8 enable;                   // 0x00  0xFF = free
    s8 area_from;                // 0x01
    s8 cam_from;                 // 0x02
    s8 area_to;                  // 0x03
    s8 cam_to;                   // 0x04
    u8 x5;                       // 0x05
    u8 pad_6[2];
    s32 frame;                   // 0x08
    u8 pad_C[4];
};

// Tool work (0x644 bytes, the static instance behind `pTc`).
struct TcWork {
    s8 routine;                  // 0x000  tcRoutineTbl index (tcInit, tcMenu, tcEdit, tcLoad, tcSave, tcQuit)
    s8 editMode;                 // 0x001  tcEdit: 0 select, 1 edit
    s8 x2;                       // 0x002
    u8 pad_3;
    s8 cursor;                   // 0x004  main menu cursor
    s8 subCursor;                // 0x005  sub menu cursor
    s8 x6;                       // 0x006
    u8 pad_7;
    TcAdat* pAdat;               // 0x008  current area
    u8 pad_C[0x10 - 0xC];
    Camera cam;                  // 0x010  tool copy of pG->Cam
    u8 pad_108[0x10C - 0x108];
    JOY joy;                     // 0x10C  pad snapshot (Joy[0]; trg at 0x120, rep at 0x128)
    JOY joy2;                    // 0x374  Joy[1]
    s8 mode;                     // 0x5DC  0 main menu, 1 sub menu
    s8 editSel;                  // 0x5DD  0 area, 1 camera
    u8 pad_5DE;
    s8 cdatNo;                   // 0x5DF  current camera data
    s8 adatNo;                   // 0x5E0  current area data
    s8 x5E1;                     // 0x5E1
    s8 cdatNum;                  // 0x5E2
    s8 adatNum;                  // 0x5E3
    s8 ldatNum;                  // 0x5E4
    u8 adatTypeNum[0x40];        // 0x5E5  per camera type
    u8 pad_625[0x627 - 0x625];
    s8 x627;                     // 0x627
    u8 pad_628[0x62E - 0x628];
    u8 viewMode;                 // 0x62E  1 = working view
    u8 x62F;                     // 0x62F
    u8 pad_630[0x632 - 0x630];
    u8 previewReq;               // 0x632
    u8 preview;                  // 0x633  preview on
    s8 x634;                     // 0x634  CamCtrl+0x692
    s8 x635;                     // 0x635  CamCtrl+0x690
    s8 x636;                     // 0x636  CamCtrl+0x691
    u8 x637;                     // 0x637  pSys->xB
    u8 lightTool;                // 0x638  1 while the light tool runs
    u8 areaDetail;               // 0x639
    u8 blink;                    // 0x63A  frame counter
    u8 pad_63B;
    cLightTool* pLightTool;      // 0x63C
    u8 pad_640[0x644 - 0x640];
};

extern TcWork* pTc;

// t_camera.cpp data pools (.bss)
extern u8 tcTypeTbl[64][16];     // [camera/area no][0] = camera type
extern TcAdat tcAdat[0x60];
extern TcCdat tcCdat[0x40];
extern TcLdat tcLdat[0x40];
TcCdat* tcCdatNew();
void tcCdatDel(TcCdat* c);
void tcCdatInit(TcCdat* c, int cam_no);
TcCdat* tcCdatPtr(int cam_no);
TcAdat* tcAdatNew();
void tcAdatDel(TcAdat* a);
void tcAdatInit(TcAdat* a, int area_no, int cam_no);
TcAdat* tcAdatPtr(int area_no, int cam_no);
TcLdat* tcLdatNew();
void tcLdatDel(TcLdat* l);
void tcLdatInit(TcLdat* l, int area_from, int cam_from, int area_to, int cam_to, int frame);
TcLdat* tcLdatPtr(int area_from, int cam_from, int area_to, int cam_to);

// t_camera_data.cpp
void tcGetFileName(char* path, int no, int flag);
int tcDataExport(u8* buf);
int tcDataImport(u8* buf);
f32 tcGetFloor();
void tcPlayerMove();
void tcCameraDebugMove();
void tcGameCamera2ToolCamera();
void tcToolCamera2GameCamera();
void tcGameCameraStore();
void tcGameCameraLoad();
void tcDrawLine3D(Vec* a, Vec* b, u32 color);
void tcDrawSphere(Vec* pos, u32 color, f32 r);
void tcDrawPoly(Vec* p, u32 color);
void tcSetBesideFloor(f32 ratio);
void tcSetBesideOffset(QfpsOfs (*ready)[3], QfpsOfs (*trans)[3]);
void tcSetBesideCamera();
extern Camera tcGameCamera;      // game camera saved while the tool runs

// t_camera_draw.cpp
void tcCameraMove();
struct TcNgon {
    int num;       // 0x00
    Vec vtx[1];    // 0x04
};
void tcDrawNgon(TcNgon* ngon, u32 color);
void tcFillNgon(TcNgon* ngon, u32 color);
int tcMenuSelect(int x, int y, int flag, TcMenu* tbl, int num, s8* cursor);
void tcDrawParametricCurve();

#endif
