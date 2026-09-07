#ifndef CAM_QFPS_H
#define CAM_QFPS_H

#include "types.h"
#include "vec.h"
#include "camera.h"

struct CameraAreaRec;

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
    f32 x1FC;           // 0x1FC
    f32 angle_y;        // 0x200
    f32 angle_x;        // 0x204
    u8 pad_208[4];
    s16 search_frame;   // 0x20C
    s16 search_count;   // 0x20E
    u8 pad_210[4];

    void setBlendCount(int n);
    void setFloorRatio(f32 ratio);
    void setBlendData(void* src, void* dst);
    void setAreaData(struct CameraCut* cut);
    void offsetCorrection();
    void bindDefaultCamera();
    void bindAreaCamera(CameraAreaRec* rec);
    void init();
    void move();
};

#endif
