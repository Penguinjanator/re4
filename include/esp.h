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
struct EspGenPrmB {
    u8 xCC, xCD, xCE, xCF;  // 0xCC
    u8 xD0, xD1, xD2, xD3;  // 0xD0
};
union EspGenPrm {
    EspGenPrmW w;
    EspGenPrmH h;
    EspGenPrmB b;
};

// Effect generator work (game/eff_sys.cpp, game/espgen*.cpp). Layout known only partially.
struct EspGenWork {
    u8 x0;             // 0x00
    u8 x1;             // 0x01 esp id / generator sub type
    u8 x2;             // 0x02
    u8 x3;             // 0x03
    u16 x4;            // 0x04 sequence time (espgen10 compares it with the frame counter)
    u8 x6;             // 0x06 event model index (EspEvModList)
    u8 x7;             // 0x07
    u32 flags;         // 0x08
    f32 x0C;           // 0x0C generator position (x0C, x10, x14 form a Vec)
    f32 x10;           // 0x10
    f32 x14;           // 0x14
    f32 x18;           // 0x18 (esp1a: min distance factor)
    f32 x1C;           // 0x1C (esp1a: max distance factor)
    f32 x20;           // 0x20
    Vec x24;           // 0x24
    f32 x30;           // 0x30
    Vec x34;           // 0x34
    u8 pad_40[0x58 - 0x40];
    Vec x58;           // 0x58
    u8 pad_64[0x88 - 0x64];
    f32 x88;           // 0x88
    u8 pad_8C[0x9C - 0x8C];
    u8 x9C;            // 0x9C colour r
    u8 x9D;            // 0x9D colour g
    u8 x9E;            // 0x9E colour b
    u8 x9F;            // 0x9F colour a
    u8 pad_A0[0xAC - 0xA0];
    f32 xAC;           // 0xAC
    u8 pad_B0[0xC2 - 0xB0];
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
    f32 xF8;           // 0xF8 (esp0e: visible cone angle in degrees)
    u8 xFC;            // 0xFC
    u8 xFD;            // 0xFD
    u8 xFE;            // 0xFE
    u8 pad_FF[0x100 - 0xFF];
    // 0x100..0x12C: sequence record tail (records of an EspSeqData are 0x12C bytes)
    u8 pad_100[0x108 - 0x100];
    u8 type;           // 0x108 0 = esp, 1 = espgen
    u8 genId;          // 0x109 generator id (0xFF = loop marker)
    u8 x10A;           // 0x10A
    u8 x10B;           // 0x10B
    u8 x10C;           // 0x10C
    u8 x10D;           // 0x10D
    s8 x10E;           // 0x10E
    u8 x10F;           // 0x10F
    s16 x110;          // 0x110
    u8 pad_112[0x124 - 0x112];
    u8 x124;           // 0x124
    u8 x125;           // 0x125
    u8 x126;           // 0x126
    u8 x127;           // 0x127
    u8 x128;           // 0x128
    u8 pad_129[0x12C - 0x129];
};

// Effect sequence data block: 0x30 byte header followed by 0x12C byte records.
struct EspSeqData {
    u16 num;           // 0x00 number of records
    u8 pad_2[6];
    u16 flags;         // 0x08
    u8 pad_A[0x30 - 0xA];
    EspGenWork rec[1]; // 0x30
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
    union {
        u32 x4;        // 0x04
        struct {
            u8 x4;     // 0x04
            u8 x5;     // 0x05
            u8 x6;     // 0x06
            u8 x7;     // 0x07
        } b;
    };
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
    u8 x80;            // 0x80 (esp0c: copied into the est work colour bytes)
    u8 x81;            // 0x81
    u8 x82;            // 0x82
    u8 x83;            // 0x83
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
    u16 xA8;           // 0xA8
    u16 xAA;           // 0xAA
    u16 spdCnt;        // 0xAC frames the speed is applied (0 = always)
    u16 scaleCnt;      // 0xAE frames the scale speed is applied (0 = always)
    u16 life;          // 0xB0 life time in frames (0 = infinite)
    u16 cnt;           // 0xB2 frame counter
    u8 anmPtn;         // 0xB4 current animation pattern
    u8 anmSpd;         // 0xB5
    u16 anmCnt;        // 0xB6
    f32 xB8;           // 0xB8
    Mtx mat;           // 0xBC model matrix built by the Trans functions
    u8 pad_EC[0xF4 - 0xEC];
    // 0xF4 vptr

    void* operator new(unsigned int size);
    cEsp();
    virtual ~cEsp();
    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
    virtual void Destruct();

    int CommonMove();
    int AnmMove();
    int ColorUpdate();
    void ApplyMatrix(Mtx m);
    void CommonStateSet();
    void ChannelSet();
};

// game/esp3f.cpp: vector buffer owned by an effect (see esp3f.cpp for the class)
class cEsp3f;
int Esp3f_Alloc(u32 size, u32 num, cEsp3f** out, EspInfo* info);
Vec* Esp3f_GetVecPtr(cEsp3f* p, u32 no);

// game/esp.cpp
void PushEsp(cEsp* esp);
extern "C" {
int PullEsp(cEsp** out, int id);
cEsp* EspGetDmyPtr();
void EspAddOtAfterRender(cEsp* esp, void (*func)(cEsp*));
// game/esp01.cpp
void EspStrip_draw_poly(cEsp* esp, int no, Vec* v, u8 texRepeat, int flag);
// game/trans_ot.cpp
void AddOtWorldPos(cEsp* esp, void (*func)(cEsp*), Vec* pos, int prio, f32 ofs);
// game/esp_sub.cpp
void EspCommonTrans(cEsp* esp);
// game/eff_sys.cpp
int EspGetAnmAddr(int no, EspAnmData** out);
void EspTexSet(int anmNo, int ptn);
void* EspGetPathAddr(int id, int no);
// game/path.cpp
int PathHasWeight(void* path);
f32 PathGetLength(void* path);
int PathGetPos(void* path, u32* seg, Vec* out, f32 dist);
int PathGetPosEm(void* path, cModel* model, u32* seg, Vec* out, f32 dist);
int EspGetTplAddr(int no, void** out);
// game/est.cpp. void: no caller reads r3 after the call, and with an `int` result the call's
// set of r3 changes the haifa depend counts, moving `li r3,0` to the end of the arg setup
// (obj01/obj10 move00, obj10AddSpeed).
void EstSet(int a, int b, Vec* pos, Vec* rot, int c, int d, int e, int f, u32 g, void* h);
}
// game/eff_sys.cpp
int EspGenGetMoveLoop();
extern cCoord* pEffParentWorld;
// Struct-member view of the same pointer (the pLog trick, db_log.h): a load through it is not
// hoisted above a preceding struct copy through `this` (esp01 move: `w->pos0 = pos; parent =
// pEffParentWorld`). Only use where the target shows the load after such stores; wrapping the
// global itself changes load order in units that already match (esp0b, esp1a, esp40).
struct EffParentWorldPtr {
    cCoord* p;
};
#define pEffParentWorldS (((EffParentWorldPtr*) &pEffParentWorld)->p)
// game/esp_app.cpp
extern "C" void EspCallSeType(int type, Vec* pos);
void EffCallRoomSeFunc(int no, Vec* pos);
int EffAreaCheckNo(Vec* pos, u8 areaNo);
// game/Espgen42.cpp
int GetWaterHeight(Vec* pos, f32* height);
extern "C" void AddWaterPower(Vec* pos, f32 power);

#endif
