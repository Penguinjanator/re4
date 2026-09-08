#ifndef ESP_H
#define ESP_H

#include "types.h"
#include "vec.h"
#include "model.h"

struct EspGenPrmW {
    u32 xCC;           // 0xCC
    u32 xD0;           // 0xD0
};
struct EspGenPrmH {
    u16 xCC;           // 0xCC
    u16 xCE;           // 0xCE
    u16 xD0;           // 0xD0
    u16 xD2;           // 0xD2
};
union EspGenPrm {
    EspGenPrmW w;
    EspGenPrmH h;
};

// Effect generator work (game/eff_sys.cpp, game/espgen*.cpp). Layout known only partially.
struct EspGenWork {
    u8 pad_0[2];
    u8 x2;             // 0x02
    u8 pad_3[5];
    u32 flags;         // 0x08
    u8 pad_0C[4];
    f32 x10;           // 0x10
    u8 pad_14[0x20 - 0x14];
    f32 x20;           // 0x20
    u8 pad_24[0x58 - 0x24];
    Vec x58;           // 0x58
    u8 pad_64[0xC2 - 0x64];
    u8 xC2;            // 0xC2
    u8 pad_C3[2];
    u8 xC5;            // 0xC5
    u8 pad_C6[2];
    u8 xC8;            // 0xC8 per-effect parameters (SE number, area number, type, ...)
    u8 xC9;            // 0xC9
    u8 xCA;            // 0xCA
    u8 xCB;            // 0xCB
    EspGenPrm prm;     // 0xCC .. 0xD4: per-effect integer parameters (word or halfword view)
    u32 xD4;           // 0xD4
    f32 xD8;           // 0xD8 per-effect float parameters
    f32 xDC;           // 0xDC
    f32 xE0;           // 0xE0
    f32 xE4;           // 0xE4
    f32 xE8;           // 0xE8
    f32 xEC;           // 0xEC
    f32 xF0;           // 0xF0
    f32 xF4;           // 0xF4
    u8 pad_F8[4];
    u8 xFC;            // 0xFC
    u8 xFD;            // 0xFD
    u8 xFE;            // 0xFE
    u8 pad_FF[0x100 - 0xFF];
};

// Texture animation data returned by EspGetAnmAddr (eff_sys.cpp). Partial layout.
struct EspAnmData {
    u16 x0;            // 0x00 texture width
    u16 x2;            // 0x02 texture height
    s16 x4;            // 0x04 sprite width
    s16 x6;            // 0x06 sprite height
    u16 nPtn;          // 0x08 number of patterns
    u8 x9;             // 0x09
    u8 xA;
    u8 xB;             // 0x0B bits 0-1: loop mode
    u8 xC;             // 0x0C 0 = fixed pattern time
    u8 pad_0D[3];
    u8 ptnTime[1];     // 0x10 per-pattern display time
};

// Effect owner info at the head of every cEsp (copied as a block by esp3f).
struct EspInfo {
    u16 x0;            // 0x00
    u8 x2;             // 0x02
    u8 x3;             // 0x03
    u32 x4;            // 0x04
    u32 x8;            // 0x08
};

// One effect sprite (game/esp.cpp, game/esp_sub.cpp). sizeof 0xF8; the vptr sits at 0xF4.
class cEsp {
public:
    EspInfo info;      // 0x00
    u8 flag;           // 0x0C bit0: in use
    u8 id;             // 0x0D effect id
    u8 anmNo;          // 0x0E texture animation id
    u8 xF;             // 0x0F
    u8 x10;            // 0x10
    u8 x11;            // 0x11
    u16 x12;           // 0x12
    u16 x14;           // 0x14
    u16 x16;           // 0x16
    u32 flags;         // 0x18 effect option bits
    cModel* pModel;    // 0x1C model the effect is attached to
    u32 x20;           // 0x20
    cCoord* parent;    // 0x24 parent coordinate (pEffParentWorld = world)
    u8 partsNo;        // 0x28 parts of pModel the effect follows
    u8 parentCnt;      // 0x29 frames to stay attached to parent (0xFF = forever)
    u16 dispFlag;      // 0x2A bit1: sizeY is a world-space length (beam sprites)
    Vec pos;           // 0x2C
    Vec spd;           // 0x38
    f32 spdScale;      // 0x44
    Vec acc;           // 0x48
    Vec rot;           // 0x54
    Vec rotSpd;        // 0x60
    f32 sizeX;         // 0x6C
    f32 sizeY;         // 0x70
    f32 scale;         // 0x74
    f32 scaleSpd;      // 0x78
    f32 scaleScale;    // 0x7C
    u8 pad_80[4];
    f32 colR;          // 0x84
    f32 colG;          // 0x88
    f32 colB;          // 0x8C
    f32 colA;          // 0x90
    f32 colRSpd;       // 0x94
    f32 colGSpd;       // 0x98
    f32 colBSpd;       // 0x9C
    f32 colASpd;       // 0xA0
    u8 xA4;            // 0xA4
    u8 xA5;            // 0xA5
    u8 xA6;            // 0xA6
    u8 xA7;            // 0xA7
    u8 pad_A8[2];
    u16 xAA;           // 0xAA
    u16 spdCnt;        // 0xAC frames the speed is applied (0 = always)
    u16 scaleCnt;      // 0xAE frames the scale speed is applied (0 = always)
    u16 life;          // 0xB0 life time in frames (0 = infinite)
    u16 cnt;           // 0xB2 frame counter
    u8 anmPtn;         // 0xB4 current animation pattern
    u8 anmSpd;         // 0xB5
    u16 anmCnt;        // 0xB6
    f32 xB8;           // 0xB8
    u8 pad_BC[0xF4 - 0xBC];
    // 0xF4 vptr

    void* operator new(unsigned int size);
    cEsp();
    virtual ~cEsp();
    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
    virtual void Destruct();

    int CommonMove();
    int AnmMove();
    void ColorUpdate();
    void ApplyMatrix(Mtx m);
    void CommonStateSet();
};

// game/esp.cpp
void PushEsp(cEsp* esp);
extern "C" {
int PullEsp(cEsp** out, int id);
cEsp* EspGetDmyPtr();
void EspAddOtAfterRender(cEsp* esp, void (*func)(cEsp*));
// game/esp_sub.cpp
void EspCommonTrans(cEsp* esp);
// game/eff_sys.cpp
int EspGetAnmAddr(int no, EspAnmData** out);
int EspGetTplAddr(int no, void** out);
// game/est.cpp
int EstSet(int a, int b, Vec* pos, Vec* rot, int c, int d, int e, int f, u32 g, u32 h);
}
// game/eff_sys.cpp
int EspGenGetMoveLoop();
extern cCoord* pEffParentWorld;
// game/esp_app.cpp
void EffCallRoomSeFunc(int no, Vec* pos);
int EffAreaCheckNo(Vec* pos, u8 areaNo);
// game/Espgen42.cpp
int GetWaterHeight(Vec* pos, f32* height);

#endif
