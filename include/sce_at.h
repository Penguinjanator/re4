#ifndef SCE_AT_H
#define SCE_AT_H

#include "types.h"
#include "vec.h"
#include "scheduler.h"

class cObj;
class cModel;

// Scenario collision areas (game/sce_at.cpp).

// One area work (SceAtSys entries, linked through an ordering table; layout partially known).
struct SceAtWork {
    u32 next;         // 0x00  OTag link
    u8 pad_4[0x35 - 0x4];
    u8 x35;           // 0x35  door area: 1 = has a destination (SceChapterEnd)
    u8 no;            // 0x36  area number (SceAtPtr key)
    u8 x37;
    u8 x38;           // 0x38  type (8 = exec)
    u8 pad_39[0x44 - 0x39];
    u8 x44;           // 0x44  (5 for item events)
    u8 pad_45[0x4A - 0x45];
    u8 x4A;           // 0x4A  (0x10 for item events)
    u8 pad_4B[0x5C - 0x4B];
    Vec dstPos;       // 0x5C  door: destination position
    f32 dstAngle;     // 0x68
    u8 dstStage;      // 0x6C
    u8 dstRoom;       // 0x6D
    u8 pad_6E[0x74 - 0x6E];
    u8 dstX4F9E;      // 0x74
    u8 pad_75[2];
    u8 x77;           // 0x77  (2 = execute now, SceChapterEnd)
};

// Message request handed to SceAtSetMes (sce_com SceUpCut), 0xC bytes.
struct SceAtMesData {
    u16 type;         // 0x00
    u16 no;           // 0x02
    u8 x4;            // 0x04
    u8 x5;            // 0x05
    u16 x6;           // 0x06
    u8 x8;            // 0x08
    u8 pad_9[3];
};

extern "C" {
// Area `no`: run `func(obj)` (prio, otPrio) when the player enters it.
void SceAtDataSet_exec(int no, int prio, int a, TaskFunc func, void* obj, int b);
void SceAtSetEnable(int no, int on);
SceAtWork* SceAtPtr(int no);
cModel* SceAtItemModelPtr(int no);
void SceAtSetMes(SceAtMesData* m);
void SceAtExecute(int no);
void SceAtExecRoomJump(u16 room, Vec* pos, Vec* rot, int a);
void SceAtSetSaveItem();
void SceAtRoomSet();
void SceAtCheckMoveScrAt();
}

// Area `no` follows parts `parts` of `obj`; 0 when the area does not exist.
int SceAtSetParent(int no, cObj* obj, int parts);
int SceAtItemFlgCk(int no);

#endif
