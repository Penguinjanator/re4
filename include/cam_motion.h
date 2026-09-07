#ifndef CAM_MOTION_H
#define CAM_MOTION_H

#include "types.h"
#include "vec.h"
#include "cam_extra.h"

// Keyframed camera motion (game/cam_motion.cpp). Derives from cCamera (vptr at 0xF8).
class CameraMotion : public cCamera {
public:
    s32 end;                       // 0xFC  1 when the motion has finished
    u8 info[0x1D0 - 0x100];        // 0x100 motion info block (getMotionInfoPtr)
    Mtx* base_mat;                 // 0x1D0

    CameraMotion(void* data, int a, int b, f32 speed);
};

#endif
