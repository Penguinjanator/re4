#ifndef CAM_QFPS_H
#define CAM_QFPS_H

#include "types.h"
#include "vec.h"
#include "camera.h"

struct CameraAreaRec;

// Shoulder camera offsets in player space (0x2C bytes), one per [left/right][up/mid/down] site.
// g_readyOfs[16]/g_transOfs[7] are the per-area tables (game/cam_qfps.cpp); db_cam edits a copy.
struct QfpsOfs {
    Vec campos;    // 0x00
    Vec campos2;   // 0x0C  close point
    Vec target;    // 0x18
    f32 x24;       // 0x24
    f32 fovy;      // 0x28
};

extern QfpsOfs g_readyOfs[16][2][3];
extern QfpsOfs g_transOfs[7][2][3];

// Over-the-shoulder ("quasi FPS") camera, game/cam_qfps.cpp. 0x214 bytes.
// Only the members touched by other units are named so far.
class CameraQuasiFPS {
public:
    Camera cam;         // 0x000 (cam.param at 0xA4 is what CameraControl::Move copies)
    u8 pad_F8[0x148 - 0xF8];
    void* blend_src;    // 0x148
    void* blend_dst;    // 0x14C
    u8 pad_150[0x1F0 - 0x150];
    Vec shoulder_aim;   // 0x1F0
    u8 site;            // 0x1FC  0 right/up-mid-down, 1 left, 2 right far, 3 left far (db_cam)
    u8 pad_1FD[3];
    f32 angle_y;        // 0x200
    f32 angle_x;        // 0x204
    u8 pad_208[4];
    s16 search_frame;   // 0x20C
    s16 search_count;   // 0x20E
    u8 pad_210[4];

    void setBlendCount(int n);
    f32 getFloorRatio();
    void setFloorRatio(f32 ratio);
    void setBlendData(void* src, void* dst);
    void getAreaData(QfpsOfs (*ready)[3], QfpsOfs (*trans)[3]);
    void setAreaData(QfpsOfs (*ready)[3], QfpsOfs (*trans)[3]);
    void setAreaData(struct CameraCut* cut);
    void offsetCorrection();
    void bindDefaultCamera();
    void bindAreaCamera(CameraAreaRec* rec);
    void init();
    void move();
};

#endif
