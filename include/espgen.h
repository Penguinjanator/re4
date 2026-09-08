#ifndef ESPGEN_H
#define ESPGEN_H

#include "types.h"
#include "vec.h"
#include "model.h"
#include "esp.h"

// Optional 0x1C byte parameter block handed down the sequence calls (copied into the generator work).
struct EspSeqOpt {
    u32 x0;            // 0x00
    u32 x4;            // 0x04
    u32 x8;            // 0x08
    u32 xC;            // 0x0C
    u32 x10;           // 0x10
    u32 x14;           // 0x14
    u32 x18;           // 0x18
};

// Effect system work (game/eff_sys.cpp cEspSystem, g_pEspSys). Partial layout.
struct cEspSystem {
    u8 pad_0[0xC548];
    u32 xC548;         // 0xC548
    u8 pad_C54C[8];
    u32 xC554;         // 0xC554
};
extern cEspSystem* g_pEspSys;

// One effect generator instance (game/espgen.cpp array, stride 0xC8). Bytes 0x14.. are the
// per-generator work (Espgen00Work, Espgen10Work, Espgen44Work, ...).
struct EspgenWork {
    EspInfo info;      // 0x00 owner info (copied from the parent by SetEspCore)
    u8 flag;           // 0x0C bit0: in use, bit1: delete requested
    u8 id;             // 0x0D generator id (index into the Espgen*Tbl tables)
    u8 xE;             // 0x0E
    u8 xF;             // 0x0F
    u8 step;           // 0x10 move step (Espgen*MoveTbl index)
    u8 pad_11[3];
    u8 work[0xC8 - 0x14];  // 0x14
};

typedef void (*EspgenMoveFunc)(EspgenWork* w);
typedef void (*EspgenTransFunc)(EspgenWork* w);
typedef int (*EspgenSetFreeWorkFunc)(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts,
                                     Mtx* mtx, Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);
typedef void (*EspgenDestructFunc)(EspgenWork* w);

extern "C" {
// game/espgen.cpp
u32 GetEspgenIdMax();
int PullEspgen(EspgenWork** out);
int PullEspgenFront(EspgenWork** out);
void PushEspgen(EspgenWork* w);
int EspgenSetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                      Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);
int EspgenSeqSet(EspSeqData* head, int no, EspInfo* info, cModel* model, u16 parts, Mtx* mtx, Vec* pos, Vec* rot,
                 EspSeqOpt* p8, int flag);

// game/esp_sub.cpp
int EspSeqSet(EspGenWork* rec, EspInfo* info, u32* seed, cModel* model, Mtx* mtx, int a, cEsp** out, EspSeqOpt* p8,
              Vec* pos, f32 f);

// game/est.cpp
extern cModel* EspEvModList[0x80];

// game/espgen10.cpp
int EspgenDataSet(EspSeqData* head, int no, EspInfo* info, u32* seed, cModel* model, u16 parts, Mtx* mtx, Vec* pos,
                  Vec* rot, EspSeqOpt* p8, int flag);
void SetEspCore(EspgenWork* w, u16 a, u32 b, u8 c, u32 d, u8 e);
int PullEspEspgen(EspgenWork** out, u16 a, int c, u32 b, u32 d, u8 e, int front);
void Espgen10_Move(EspgenWork* w);

// game/espgen00.cpp
void Espgen00_Move(EspgenWork* w);
int Espgen00_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);

// game/espgen44.cpp
void Espgen44_Move(EspgenWork* w);
void Espgen44_Trans(EspgenWork* w);
void Espgen44_Destruct(EspgenWork* w);
int Espgen44_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);
}

#endif
