#include "types.h"

struct GlobalWork {
    u8 pad_0[0x4F28];
    void* x4F28;
};

extern GlobalWork* pG;

struct CameraArea {
    u8 pad_0[5];
    u8 num;
};

class CameraControl {
public:
    u8 pad_0[0x2C];
    u32 flags_2C;
    u32 flags_30;
    u8 x34;
    u8 pad_35[0x5C - 0x35];
    CameraArea* area;
    u8 pad_60[0x690 - 0x60];
    s8 area_no;
    u8 x691;
    s8 camera_no;

    void Check();
    int IsChangeCamera();
    void Comeback();
    void Disable();
    void AreaCheckOnOff(int mode);
    u8 AreaNum();
    int CurrentAreaNo();
    int CurrentCameraNo();
};

int CameraControl::IsChangeCamera()
{
    if (flags_30 & 2) {
        return 1;
    }
    return 0;
}

void CameraControl::Comeback()
{
    area = (CameraArea*) pG->x4F28;
    flags_30 &= ~4;
    flags_2C = (flags_2C & ~8) | 0x10;
    if (flags_2C & 0x20) {
        flags_2C &= ~0x20;
    }
    Check();
}

void CameraControl::Disable()
{
    x34 = 0;
    flags_2C |= 8;
}

void CameraControl::AreaCheckOnOff(int mode)
{
    if (mode == 0) {
        flags_2C |= 8;
    } else if (mode == 1) {
        flags_2C = (flags_2C & ~8) | 0x10;
    }
}

u8 CameraControl::AreaNum()
{
    return area->num;
}

int CameraControl::CurrentAreaNo()
{
    return area_no;
}

int CameraControl::CurrentCameraNo()
{
    return camera_no;
}
