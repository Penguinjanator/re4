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
    Mtx v_mat;        // 0x30 look-at matrix (C_MTXLookAt)
    u8 pad_60[4];
    Mtx44 ProjMat;      // 0x64 projection matrix
    CameraParam param;  // 0xA4 (pos 0xA4, at 0xB0, roll 0xBC, fovy 0xC0)
    Vec up;             // 0xC4 up vector (C_MTXLookAt)
    Vec Look;            // 0xD0 pos - at, normalised (matrix column 2)
    Vec Right;          // 0xDC up x dir (matrix column 0)
    u8 pad_E8[0xF4 - 0xE8];
    f32 dist;           // 0xF4 |pos - at| (db_cam keeps it current for the debug camera)
};

// Matrix from four column vectors (Vec*: right, up, look, position), as twelve stores.
#define MTX_SET_COLUMNS(m, c0, c1, c2, c3)                                                    \
    (m)[0][0] = (c0)->x; (m)[1][0] = (c0)->y; (m)[2][0] = (c0)->z;                            \
    (m)[0][1] = (c1)->x; (m)[1][1] = (c1)->y; (m)[2][1] = (c1)->z;                            \
    (m)[0][2] = (c2)->x; (m)[1][2] = (c2)->y; (m)[2][2] = (c2)->z;                            \
    (m)[0][3] = (c3)->x; (m)[1][3] = (c3)->y; (m)[2][3] = (c3)->z

extern "C" {
// game/cam_sys.cpp
void CameraSetOrientationUp(Camera* cam);
void CameraSetOrientationRoll(Camera* cam);
void CameraSetOrientationZeroRoll(Camera* cam);
f32 CameraGetRoll(Camera* cam);
void CameraRotAxisPosRad(Camera* cam, Vec* axis, Vec* pos, f32 rad);
void CameraTargetRot(Camera* cam, char axis, f32 rad);
void CameraCamposRot(Camera* cam, char axis, f32 rad);
void CameraDolly(Camera* cam, Vec* speed);
void CameraCamposDistance(Camera* cam, f32 dist);
void CameraSetWithRoll(Camera* cam, Vec* pos, Vec* at, f32 roll, f32 fovy);
// game/camera.cpp
void CameraSetProjection(int type);
int CameraGetProjection();
void CameraGameInit();
void CameraRoomInit();
void CameraMove();
struct ViewFrustum* CameraViewFrustumPtr(Camera* cam);
void CameraGetUpVec(Camera* cam, Vec* up);
void CameraGetLookVec(Camera* cam, Vec* look);
void CameraGetLookVecInverse(Camera* cam, Vec* look);
void CamPos2ScrnVec(f32 sx, f32 sy, Vec* out);
}
// game/camera.cpp (C++ linkage): loads the current projection matrix into GX
void CameraCurrentProjection();
extern int ProjType;   // current projection type (db_cam.cpp toggles it)
// game/cam_sys.cpp, C++ linkage (`CameraTargetDistance__FP6Cameraf` in Bio4.sym, marked local there;
// the t_camera REL imports it, so cam_sys.cpp defines it non-static)
void CameraTargetDistance(Camera* cam, f32 dist);
struct JOY;
void CamStick2World(Camera* cam, JOY* joy, Vec* out);

#endif
