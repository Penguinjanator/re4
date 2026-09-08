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

class IdScope {
public:
    void save(int);
    void load(int);
};

class CameraLookAt : public cCamera {
public:
    CameraLookAt(Camera* cam);
};

class CameraPushObject : public cCamera {
public:
    CameraPushObject();
};

class CameraLookDownEm : public cCamera {
public:
    CameraLookDownEm(void* em, Vec* ofs);
};

class CameraScope : public cCamera {
public:
    u8 pad_FC[0x158 - 0xFC];
    IdScope id; // 0x158

    CameraScope();
    void setParam(f32 a, f32 b);
    void getParam(f32* a, f32* b);
};

class CameraBinocular : public cCamera {
public:
    u8 pad_FC[0x19C - 0xFC];
    void* id_a; // 0x19C
    void* id_b; // 0x1A0

    CameraBinocular(void* a, void* b, void* c, void* d);
    void setRange(f32 range);
};

class CameraAttachedToMotion : public cCamera {
public:
    CameraAttachedToMotion(cModel* model);
};

#endif
