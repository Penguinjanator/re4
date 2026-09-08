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
    Mtx mat;            // 0x00 camera orientation matrix (QuakeMain rotates the quake offset by it)
    Mtx viewMat;        // 0x30 look-at matrix (C_MTXLookAt)
    u8 pad_60[4];
    Mtx44 projMat;      // 0x64 projection matrix
    CameraParam param;  // 0xA4 (pos 0xA4, at 0xB0, roll 0xBC, fovy 0xC0)
    Vec up;             // 0xC4 up vector (C_MTXLookAt)
    u8 pad_D0[0xF4 - 0xD0];
    f32 dist;           // 0xF4 |pos - at| (db_cam keeps it current for the debug camera)
};

extern "C" {
void CameraSetOrientationRoll(Camera* cam);
void CameraSetOrientationUp(Camera* cam);
void CameraMove();
void CameraGetUpVec(Camera* cam, Vec* up);
}
// game/camera.cpp (C++ linkage): loads the current projection matrix into GX
void CameraCurrentProjection();

#endif
