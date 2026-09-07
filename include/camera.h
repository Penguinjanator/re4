#ifndef CAMERA_H
#define CAMERA_H

#include "types.h"
#include "vec.h"

// Position/target/roll/fov set that the camera system interpolates and copies around
// (0x20 bytes; the compiler copies it with a 0x18-stride loop + 8 bytes).
struct CameraParam {
    Vec pos;   // 0x00
    Vec at;    // 0x0C
    f32 roll;  // 0x18
    f32 fovy;  // 0x1C
};

// Camera state block used by camera.cpp / cam_sys.cpp (0xF8 bytes). Only the fields the
// cam_ctrl unit touches are named; extend this, do not rewrite it.
struct Camera {
    u8 pad_0[0xA4];
    CameraParam param;  // 0xA4 (pos 0xA4, at 0xB0, roll 0xBC, fovy 0xC0)
    u8 pad_C4[0xF8 - 0xC4];
};

extern "C" {
void CameraSetOrientationRoll(Camera* cam);
void CameraMove();
}

#endif
