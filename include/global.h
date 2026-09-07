#ifndef GLOBAL_H
#define GLOBAL_H

#include "types.h"
#include "vec.h"
#include "camera.h"

// Global game work (`pG`, game/main.cpp). Offsets come from the cam_ctrl unit; extend the
// pads as other units reveal more fields, never rewrite.
struct GlobalWork {
    u8 pad_0[0x54];
    u32 flags_54;          // 0x54
    u32 flags_58;          // 0x58
    u8 pad_5C[4];
    u32 flags_60;          // 0x60
    u32 flags_64;          // 0x64
    u8 pad_68[4];
    u32 flags_6C;          // 0x6C
    u8 pad_70[4];
    Camera Cam;            // 0x74 .. 0x16C  (Cam.param at 0x118)
    u8 pad_16C[0x4F24 - 0x16C];
    void* pCoreCamData;    // 0x4F24  core camera data ("B40x")
    void* pRoomCamData;    // 0x4F28  room camera data ("B40x")
    u8 pad_4F2C[0x4F70 - 0x4F2C];
    Vec quake_ofs;         // 0x4F70
    u8 pad_4F7C[0x4F93 - 0x4F7C];
    u8 x4F93;              // 0x4F93
    u8 pad_4F94[8];
    u8 stage_no;           // 0x4F9C
    u8 room_no;            // 0x4F9D
    u8 pad_4F9E[0x4FB8 - 0x4F9E];
    u8 x4FB8;              // 0x4FB8
    u8 pad_4FB9[0x500C - 0x4FB9];
    u32 flags_500C;        // 0x500C
    u8 pad_5010[4];
    u32 flags_5014;        // 0x5014
    u8 pad_5018[0x8678 - 0x5018];
    u8 debug_mode;         // 0x8678  debug page number (t_page), 0xF = camera rail debug draw
    u8 debug_mode_bak;     // 0x8679  page saved by t_page
};

extern GlobalWork* pG;

// Flag helpers. The original sets/clears bits through an inline helper taking a reference: the
// store is then a plain scalar access, so GCC 2.95 assumes it may clobber `pG` and reloads it
// (and does not merge consecutive updates). A direct `pG->flags |= x` compiles differently.
static inline void BitOn(u32& f, u32 b) { f |= b; }
static inline void BitOff(u32& f, u32 b) { f &= ~b; }

// Same effect for the room camera data pointer store in CameraControl::RoomDataRead.
#define G_ROOM_CAM_DATA (*(void**) ((u8*) pG + 0x4F28))

#endif
