#ifndef CAM_EXTRA_H
#define CAM_EXTRA_H

#include "types.h"
#include "vec.h"
#include "camera.h"

class cModel;

// Placement new used to construct camera objects inside CameraControl::extra_buf.
#ifndef PLACEMENT_NEW_DEFINED
#define PLACEMENT_NEW_DEFINED
inline void* operator new(unsigned int, void* p) { return p; }
#endif

// Base class of the special-purpose cameras (game/cam_extra.cpp, cam_motion.cpp).
// GNU v2 layout: Camera data first, then the vtable pointer at 0xF8 (size 0xFC).
//   vtable slot 0 (0x08): virtual destructor
//   vtable slot 1 (0x10): move()
class cCamera : public Camera {
public:
    virtual ~cCamera() {}
    virtual void move() = 0;
    void operator delete(void*) {}
};

// Screen-id (widget) application base: init/move/quit driven by the owning camera.
class IDApplication {
public:
    virtual ~IDApplication() {}
    virtual void init(u8* type) {}
    virtual void move(f32* zoom) {}
    virtual void quit() {}
    void operator delete(void*) {}
};

// Scope reticle ids (IdSys unit 0x25).
class IdScope : public IDApplication {
public:
    s32 save_a;  // 0x04
    s32 save_b;  // 0x08

    virtual void init(u8* type);
    virtual void move(f32* zoom);
    virtual void quit();
    void save(int);
    void load(int);
};

// Binocular ids (IdSys unit 0x27).
class IdBinocular : public IDApplication {
public:
    u8 pad_4[0x40 - 0x04];

    virtual void init(u8* type);
    virtual void move(f32* zoom);
    virtual void quit();
    void cutin();
};

// Filter0a focus blur animation.
struct FocusAnimation {
    u8 state;    // 0x00
    s32 count;   // 0x04
    f32 frame;   // 0x08
    u8 alpha;    // 0x0C

    void init(int id);
    void move(int dir);
    void quit();
    void clear();
};

class CameraLookAt : public cCamera {
public:
    Vec ofs;  // 0xFC

    CameraLookAt(Camera* cam);
    virtual ~CameraLookAt();
    virtual void move();
};

class CameraPushObject : public cCamera {
public:
    CameraPushObject();
    virtual ~CameraPushObject();
    virtual void move();
};

class CameraLookDownEm : public cCamera {
public:
    void* em;  // 0xFC

    CameraLookDownEm(void* em, Vec* ofs);
    virtual ~CameraLookDownEm();
    virtual void move();
};

class CameraScope : public cCamera {
public:
    Vec pos_ofs;      // 0x0FC
    Vec dir;          // 0x108
    f32 angle_x;      // 0x114
    u8 pad_118[8];
    f32 angle_min;    // 0x120
    u8 pad_124[8];
    f32 angle_max;    // 0x12C
    u8 pad_130[8];
    f32 zoom;         // 0x138
    Vec yure;         // 0x13C
    f32 x148;         // 0x148
    u8 pad_14C[9];
    u8 type;          // 0x155
    u8 pad_156[2];
    IdScope id;       // 0x158
    FocusAnimation focus;  // 0x164

    CameraScope(Vec* pos, Vec* at);
    virtual ~CameraScope();
    virtual void move();
    void setParam(f32 a, f32 b);
    void getParam(f32* a, f32* b);
};

class CameraBinocular : public cCamera {
public:
    s32 mode;          // 0x0FC
    f32 x100;          // 0x100
    f32 x104;          // 0x104
    f32 x108;          // 0x108
    f32 x10C;          // 0x10C
    f32 x110;          // 0x110
    f32 x114;          // 0x114
    f32 x118;          // 0x118
    f32 x11C;          // 0x11C
    f32 x120;          // 0x120
    f32 x124;          // 0x124
    IdBinocular id;    // 0x128
    FocusAnimation focus;  // 0x168
    Vec pos_local;     // 0x178
    Vec at_local;      // 0x184
    Vec up_local;      // 0x190
    void* id_a;        // 0x19C
    void* id_b;        // 0x1A0

    CameraBinocular(Vec* pos, Vec* at, void* id_a, void* id_b);
    virtual ~CameraBinocular();
    virtual void move();
    void setRange(f32 a, f32 b, f32 c, f32 d);
};

class CameraAttachedToMotion : public cCamera {
public:
    cModel* model;  // 0xFC

    CameraAttachedToMotion(cModel* model);
    virtual ~CameraAttachedToMotion();
    virtual void move();
};

#endif
